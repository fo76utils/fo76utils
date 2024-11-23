
#include "common.hpp"
#include "filebuf.hpp"

#if defined(_WIN32) || defined(_WIN64)
#  include <windows.h>
#  include <io.h>
extern "C"
{
// mman-win32 is required on Windows (https://github.com/alitrack/mman-win32)
#  include "mman.c"
}
#else
#  include <sys/stat.h>
#  include <fcntl.h>
#  include <unistd.h>
#  include <sys/mman.h>
#endif

inline void FileBuffer::closeFileStream()
{
  if (std::intptr_t(fileStream) >= 0L) [[unlikely]]
  {
    munmap((void *) fileBuf, fileBufSize);
#if defined(_WIN32) || defined(_WIN64)
    CloseHandle(HANDLE(fileStream));
#else
    close(int(fileStream));
#endif
    fileStream = std::uintptr_t(-1);
  }
}

unsigned char FileBuffer::readUInt8()
{
  if ((filePos + 1) > fileBufSize)
    errorMessage("end of input file");
  return readUInt8Fast();
}

signed char FileBuffer::readInt8()
{
  if ((filePos + 1) > fileBufSize)
    errorMessage("end of input file");
  return (signed char) ((int(readUInt8Fast()) ^ 0x80) - 0x80);
}

std::uint16_t FileBuffer::readUInt16()
{
  if ((filePos + 2) > fileBufSize)
    errorMessage("end of input file");
  return readUInt16Fast();
}

std::int16_t FileBuffer::readInt16()
{
  if ((filePos + 2) > fileBufSize)
    errorMessage("end of input file");
  return uint16ToSigned(readUInt16Fast());
}

std::uint32_t FileBuffer::readUInt32()
{
  if ((filePos + 4) > fileBufSize)
    errorMessage("end of input file");
  return readUInt32Fast();
}

std::int32_t FileBuffer::readInt32()
{
  if ((filePos + 4) > fileBufSize)
    errorMessage("end of input file");
  return uint32ToSigned(readUInt32Fast());
}

float FileBuffer::readFloat()
{
  if ((filePos + 4) > fileBufSize)
    errorMessage("end of input file");
  std::uint32_t tmp = readUInt32Fast();
#if defined(__i386__) || defined(__x86_64__) || defined(__x86_64)
  if (!((tmp + 0x00800000U) & 0x7F000000U))
    return 0.0f;
  return std::bit_cast< float, std::uint32_t >(tmp);
#else
  int     e = int((tmp >> 23) & 0xFF);
  if (e == 0x00 || e == 0xFF)
    return 0.0f;
  double  m = double(int((tmp & 0x007FFFFF) | 0x00800000));
  m = std::ldexp(m, e - 150);
  if (tmp & 0x80000000U)
    m = -m;
  return float(m);
#endif
}

FloatVector4 FileBuffer::readFloatVector4()
{
#if ENABLE_X86_64_SIMD >= 2
  if ((filePos + 16) > fileBufSize)
    errorMessage("end of input file");
  const std::int32_t  *p =
      reinterpret_cast< const std::int32_t * >(fileBuf + filePos);
  filePos = filePos + 16;
  XMM_Int32 tmp = { p[0], p[1], p[2], p[3] };
  tmp &= (((tmp + 0x00800000) & 0x7F000000) != 0);
  return FloatVector4(std::bit_cast< XMM_Float >(tmp));
#else
  float   v0 = readFloat();
  float   v1 = readFloat();
  float   v2 = readFloat();
  float   v3 = readFloat();
  return FloatVector4(v0, v1, v2, v3);
#endif
}

FloatVector4 FileBuffer::readFloat16Vector4()
{
  if ((filePos + 8) > fileBufSize)
    errorMessage("end of input file");
  std::uint64_t tmp;
#if defined(__i386__) || defined(__x86_64__) || defined(__x86_64)
  tmp = *(reinterpret_cast< const std::uint64_t * >(fileBuf + filePos));
  filePos = filePos + 8;
#else
  tmp = (std::uint64_t) readUInt32Fast();
  tmp = tmp | ((std::uint64_t) readUInt32Fast() << 32);
#endif
  return FloatVector4::convertFloat16(tmp, true);
}

std::uint64_t FileBuffer::readUInt64()
{
  if ((filePos + 8) > fileBufSize)
    errorMessage("end of input file");
#if defined(__i386__) || defined(__x86_64__) || defined(__x86_64)
  filePos = filePos + 8;
  return *(reinterpret_cast< const std::uint64_t * >(fileBuf + (filePos - 8)));
#else
  std::uint64_t tmp = (std::uint64_t) readUInt32Fast();
  tmp = tmp | ((std::uint64_t) readUInt32Fast() << 32);
  return tmp;
#endif
}

float FileBuffer::readFloat16()
{
  if ((filePos + 2) > fileBufSize)
    errorMessage("end of input file");
  return convertFloat16(readUInt16Fast());
}

void FileBuffer::readString(std::string& s, size_t n)
{
  s.clear();
  const unsigned char *p, *endPtr;
  if (n == std::string::npos)
  {
    if (filePos >= fileBufSize)
      return;
    p = fileBuf + filePos;
    n = fileBufSize - filePos;
    if (!(n > 0 && *p))
    {
      filePos += size_t(n > 0);
      return;
    }
    endPtr = reinterpret_cast< const unsigned char * >(std::memchr(p, '\0', n));
    if (!endPtr)
      endPtr = p + n;
    else
      n = size_t(endPtr - p) + 1;
    filePos = filePos + n;
  }
  else
  {
    if (((unsigned long long) filePos + n) > fileBufSize) [[unlikely]]
      errorMessage("end of input file");
    p = fileBuf + filePos;
    filePos = filePos + n;
    if (!(n > 0 && *p))
      return;
    endPtr = reinterpret_cast< const unsigned char * >(std::memchr(p, '\0', n));
    if (!endPtr)
      endPtr = p + n;
  }
  s.resize(size_t(endPtr - p), ' ');
  char    *t = &(s.front());
  do
  {
    unsigned char c = *p;
    if (c >= 0x20) [[likely]]
      *t = char(c);
    t++;
  }
  while (++p < endPtr);
}

void FileBuffer::readPath(std::string& s, size_t n,
                          const char *prefix, const char *suffix)
{
  readString(s, n);
  if (s.empty())
    return;
  for (size_t i = 0; i < s.length(); i++)
  {
    if (s[i] >= 'A' && s[i] <= 'Z')
      s[i] = s[i] + ('a' - 'A');
    else if (s[i] == '\\')
      s[i] = '/';
  }
  if (prefix && prefix[0] != '\0')
  {
    size_t  p = 0;
    size_t  l = std::strlen(prefix);
    while (true)
    {
      if ((p + l) > s.length())
      {
        p = std::string::npos;
        break;
      }
      if (std::memcmp(s.c_str() + p, prefix, l * sizeof(char)) == 0)
      {
        if (p > 0)
          s.erase(0, p);
        break;
      }
      p = s.find('/', p);
      if (p == std::string::npos)
        break;
      p++;
    }
    if (p == std::string::npos)
      s.insert(0, prefix);
  }
  if (suffix && suffix[0] != '\0' && !s.ends_with(suffix))
    s += suffix;
}

unsigned char FileBuffer::readUInt8(size_t offs) const
{
  if ((offs + 1) > fileBufSize)
    errorMessage("end of input file");
  return fileBuf[offs];
}

std::uint16_t FileBuffer::readUInt16(size_t offs) const
{
  if ((offs + 2) > fileBufSize)
    errorMessage("end of input file");
  return readUInt16Fast(fileBuf + offs);
}

std::uint32_t FileBuffer::readUInt32(size_t offs) const
{
  if ((offs + 4) > fileBufSize)
    errorMessage("end of input file");
  return readUInt32Fast(fileBuf + offs);
}

void FileBuffer::setBuffer(const unsigned char *fileData, size_t fileSize)
{
  closeFileStream();
  fileBuf = fileData;
  fileBufSize = fileSize;
  filePos = 0;
}

FileBuffer::FileBuffer()
  : fileBuf(nullptr),
    fileBufSize(0),
    filePos(0),
    fileStream(std::uintptr_t(-1))
{
}

FileBuffer::FileBuffer(const unsigned char *fileData, size_t fileSize)
  : fileBuf(fileData),
    fileBufSize(fileSize),
    filePos(0),
    fileStream(std::uintptr_t(-1))
{
}

FileBuffer::FileBuffer(const char *fileName)
  : filePos(0),
    fileStream(std::uintptr_t(-1))
{
  if (!fileName || *fileName == '\0')
    errorMessage("empty input file name");
  try
  {
    fileStream = openFileInDataPath(fileName);
    if (std::intptr_t(fileStream) < 0L)
      throw FO76UtilsError("error opening input file \"%s\"", fileName);
#if defined(_WIN32) || defined(_WIN64)
    LARGE_INTEGER fsize;
    fsize.QuadPart = -1L;
    if (!GetFileSizeEx(HANDLE(fileStream), &fsize) || fsize.QuadPart < 0L)
      throw FO76UtilsError("error seeking input file \"%s\"", fileName);
    fileBufSize = size_t(fsize.QuadPart);
    fileBuf = (unsigned char *) mmap(0, fileBufSize, PROT_READ, MAP_PRIVATE,
                                     fileStream, 0);
#else
    std::int64_t  fsize =
        std::int64_t(lseek(int(fileStream), off_t(0), SEEK_END));
    if (fsize < 0L || lseek(int(fileStream), off_t(0), SEEK_SET) < 0L)
      throw FO76UtilsError("error seeking input file \"%s\"", fileName);
    fileBufSize = size_t(fsize);
    fileBuf = (unsigned char *) mmap(0, fileBufSize, PROT_READ, MAP_PRIVATE,
                                     int(fileStream), 0);
#endif
    if ((void *) fileBuf == MAP_FAILED)
      throw FO76UtilsError("error mapping input file \"%s\"", fileName);
  }
  catch (...)
  {
    if (std::intptr_t(fileStream) >= 0L)
#if defined(_WIN32) || defined(_WIN64)
      CloseHandle(HANDLE(fileStream));
#else
      close(int(fileStream));
#endif
    throw;
  }
}

FileBuffer::~FileBuffer()
{
  closeFileStream();
}

bool FileBuffer::getDefaultDataPath(std::string& dataPath)
{
  dataPath.clear();
#ifdef BUILD_CE2UTILS
  const char  *s = std::getenv("CE2UTILS_DATAPATH");
#elif !defined(NIFSKOPE_VERSION)
  const char  *s = std::getenv("FO76UTILS_DATAPATH");
#else
  const char  *s = nullptr;
#endif
  if (!s)
    return false;
  if (!(s[0] == '.' || s[0] == '/'
#if defined(_WIN32) || defined(_WIN64)
        || s[0] == '\\' ||
        (((s[0] >= 'A' && s[0] <= 'Z') ||
          (s[0] >= 'a' && s[0] <= 'z')) && s[1] == ':')
#endif
        ))
  {
    return false;
  }
  dataPath = s;
  while (dataPath.length() > 0 &&
         (dataPath.back() == '/' || dataPath.back() == '\\'))
  {
    dataPath.resize(dataPath.length() - 1);
  }
  return (dataPath.length() > 0);
}

std::uintptr_t FileBuffer::openFileInDataPath(const char *fileName)
{
  if (!fileName)
    return std::uintptr_t(-1);
  std::string nameBuf;
  size_t  dataPathOffset = 0;
  if (!(fileName[0] == '.' || fileName[0] == '/'
#if defined(_WIN32) || defined(_WIN64)
        || fileName[0] == '\\' ||
        (((fileName[0] >= 'A' && fileName[0] <= 'Z') ||
          (fileName[0] >= 'a' && fileName[0] <= 'z')) && fileName[1] == ':')
#endif
        ))
  {
    if (getDefaultDataPath(nameBuf))
    {
#if defined(_WIN32) || defined(_WIN64)
      nameBuf += '\\';
#else
      nameBuf += '/';
#endif
      dataPathOffset = nameBuf.length();
      nameBuf += fileName;
      fileName = nameBuf.c_str();
    }
  }
  std::uintptr_t  f;
#if defined(_WIN32) || defined(_WIN64)
  f = std::uintptr_t(CreateFileA(fileName, GENERIC_READ, FILE_SHARE_READ,
                                 LPSECURITY_ATTRIBUTES(0), OPEN_EXISTING, 0,
                                 HANDLE(0)));
  if (HANDLE(f) == INVALID_HANDLE_VALUE && dataPathOffset)
  {
    fileName = fileName + dataPathOffset;
    f = std::uintptr_t(CreateFileA(fileName, GENERIC_READ, FILE_SHARE_READ,
                                   LPSECURITY_ATTRIBUTES(0), OPEN_EXISTING, 0,
                                   HANDLE(0)));
  }
  f = (HANDLE(f) != INVALID_HANDLE_VALUE ? f : std::uintptr_t(-1));
#else
  f = std::uintptr_t(open(fileName, O_RDONLY));
  if (int(f) < 0 && dataPathOffset)
  {
    fileName = fileName + dataPathOffset;
    f = std::uintptr_t(open(fileName, O_RDONLY));
  }
  f = (int(f) >= 0 ? f : std::uintptr_t(-1));
#endif
  return f;
}

void FileBuffer::printHexData(std::string& s, const unsigned char *p, size_t n,
                              size_t bytesPerLine)
{
  for (size_t i = 0; i < n; i++)
  {
    size_t  column = i % bytesPerLine;
    if (!column)
      printToString(s, "%08X", (unsigned int) i);
    printToString(s, "%s%02X",
                  (!(column & 7) ? "  " : " "), (unsigned int) p[i]);
    if (column != (bytesPerLine - 1) && (i + 1) < n)
      continue;
    for (size_t j = column; ++j < bytesPerLine; )
      printToString(s, "%s", (!(j & 7) ? "    " : "   "));
    printToString(s, "  |");
    do
    {
      unsigned char c = p[i - column];
      if (c < 0x20 || c >= 0x7F)
        c = 0x2E;
      s += char(c);
    }
    while (column--);
    printToString(s, "|\n");
  }
}

const unsigned char FileBuffer::dxgiFormatSizeTable[128] =
{
  // block size | (isCompressed << 7)
  0x00, 0x10, 0x10, 0x10, 0x10, 0x0C, 0x0C, 0x0C,       // 0x00
  0x0C, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
  0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x04,       // 0x10
  0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
  0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,       // 0x20
  0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
  0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,       // 0x30
  0x02, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01, 0x01,
  0x01, 0x01, 0x00, 0x04, 0x00, 0x00, 0x88, 0x88,       // 0x40
  0x88, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x88,
  0x88, 0x88, 0x90, 0x90, 0x90, 0x02, 0x02, 0x04,       // 0x50
  0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x90, 0x90,
  0x90, 0x90, 0x90, 0x90, 0x00, 0x00, 0x00, 0x00,       // 0x60
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00,       // 0x70
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

bool FileBuffer::writeDDSHeader(unsigned char *p, unsigned char dxgiFmt,
                                int width, int height, int mipCnt,
                                bool isCubeMap, int arraySize)
{
  if (dxgiFmt >= 0x80 || !dxgiFormatSizeTable[dxgiFmt])
    return false;
  mipCnt = std::max< int >(mipCnt, 1);
  arraySize = std::max< int >(arraySize, 1);
  std::uint32_t pitch = dxgiFormatSizeTable[dxgiFmt];
  bool    compressedTexture = bool(pitch & 0x80U);
  pitch = pitch & 0x7FU;
  if (!compressedTexture)
    pitch *= std::uint32_t(width);
  else
    pitch *= std::uint32_t((width + 3) >> 2) * std::uint32_t((height + 3) >> 2);
  writeUInt32Fast(p, 0x20534444);       // "DDS "
  writeUInt32Fast(p + 4, 124);          // size of DDS_HEADER
  // DDSD_CAPS, DDSD_HEIGHT, DDSD_WIDTH, DDSD_PIXELFORMAT, DDSD_MIPMAPCOUNT
  if (!compressedTexture)
    writeUInt32Fast(p + 8, 0x0002100F); // + DDSD_PITCH, or
  else
    writeUInt32Fast(p + 8, 0x000A1007); //   DDSD_LINEARSIZE
  writeUInt32Fast(p + 12, height);      // height
  writeUInt32Fast(p + 16, width);       // width
  writeUInt32Fast(p + 20, pitch);
  writeUInt32Fast(p + 24, 0);           // depth
  writeUInt32Fast(p + 28, std::uint32_t(mipCnt));       // mipmap count
  writeUInt64Fast(p + 32, 0);           // unused
  writeUInt64Fast(p + 40, 0);
  writeUInt64Fast(p + 48, 0);
  writeUInt64Fast(p + 56, 0);
  writeUInt64Fast(p + 64, 0);
  writeUInt32Fast(p + 72, 0);
  writeUInt32Fast(p + 76, 32);          // size of DDS_PIXELFORMAT
  writeUInt32Fast(p + 80, 0x04);        // DDPF_FOURCC
  writeUInt32Fast(p + 84, 0x30315844);  // "DX10"
  writeUInt64Fast(p + 88, 0);
  writeUInt64Fast(p + 96, 0);
  writeUInt32Fast(p + 104, 0);
  writeUInt32Fast(p + 108, 0x00401008); // DDSCAPS_COMPLEX, TEXTURE, MIPMAP
  if (!isCubeMap)
  {
    writeUInt64Fast(p + 112, 0);
    writeUInt32Fast(p + 136, 0);
  }
  else
  {
    writeUInt64Fast(p + 112, 0xFE00);   // DDSCAPS2_CUBEMAP*
    writeUInt32Fast(p + 136, 0x04);     // DDS_RESOURCE_MISC_TEXTURECUBE
  }
  writeUInt64Fast(p + 120, 0);
  writeUInt32Fast(p + 128, dxgiFmt);    // DXGI_FORMAT
  writeUInt32Fast(p + 132, 3);          // D3D10_RESOURCE_DIMENSION_TEXTURE2D
  writeUInt64Fast(p + 140, (unsigned int) arraySize);   // arraySize
  return true;
}

void OutputFile::flushBuffer()
{
  size_t  n = bufWritePos;
  bufWritePos = 0;
#if defined(_WIN32) || defined(_WIN64)
  if (size_t(_write(fileDesc, buf, n)) != n)
#else
  if (size_t(write(fileDesc, buf, n)) != n)
#endif
  {
    errorMessage("error writing output file");
  }
}

OutputFile::OutputFile(const char *fileName, size_t bufSize)
  : buf(nullptr),
    bufferSize(bufSize),
    bufWritePos(0)
{
  if (bufSize > 0)
    buf = new unsigned char[bufSize];
#if defined(_WIN32) || defined(_WIN64)
  f = fopen64(fileName, "wb");
#else
  f = std::fopen(fileName, "wb");
#endif
  if (!f)
  {
    if (buf)
      delete[] buf;
    throw FO76UtilsError("error opening output file \"%s\"", fileName);
  }
#if defined(_WIN32) || defined(_WIN64)
  fileDesc = _fileno(f);
#else
  fileDesc = fileno(f);
#endif
}

OutputFile::~OutputFile()
{
  if (bufWritePos)
  {
    try
    {
      flushBuffer();
    }
    catch (...)
    {
    }
  }
  (void) std::fclose(f);
  if (buf)
    delete[] buf;
}

void OutputFile::writeData(const void *p, size_t n)
{
  if (n < bufferSize)
  {
    if (bufWritePos < bufferSize)
    {
      size_t  m = bufferSize - bufWritePos;
      m = std::min(m, n);
      if (m)
      {
        std::memcpy(buf + bufWritePos, p, m);
        bufWritePos = bufWritePos + m;
        p = reinterpret_cast< const unsigned char * >(p) + m;
        n = n - m;
      }
    }
    if (!n)
      return;
  }
  if (bufWritePos)
    flushBuffer();
  const unsigned char *bufp = reinterpret_cast< const unsigned char * >(p);
  while (n > 0)
  {
    size_t  tmp = n;
    if (tmp > 0x40000000)
      tmp = 0x40000000;
#if defined(_WIN32) || defined(_WIN64)
    if (size_t(_write(fileDesc, bufp, tmp)) != tmp)
#else
    if (size_t(write(fileDesc, bufp, tmp)) != tmp)
#endif
    {
      errorMessage("error writing output file");
    }
    bufp = bufp + tmp;
    n = n - tmp;
  }
}

void OutputFile::flush()
{
  if (bufWritePos)
    flushBuffer();
}

void DDSInputFile::readDDSHeader(int& width, int& height, int& pixelFormat,
                                 unsigned int *hdrReserved)
{
  width = 0;
  height = 0;
  pixelFormat = pixelFormatUnknown;
  try
  {
    if (fileBufSize < 148)
      errorMessage("invalid DDS input file size");
    unsigned int  hdrBuf[37];
    unsigned int  tmp = 0;
    for (unsigned int i = 0; i < 148; i++)
    {
      tmp = (tmp >> 8) | ((unsigned int) fileBuf[i] << 24);
      if ((i & 3) == 3)
      {
        hdrBuf[i >> 2] = tmp;
        if (hdrReserved && (i >= 32 && i < 76))
          hdrReserved[(i >> 2) - 8] = tmp;
      }
    }
    if (hdrBuf[0] != 0x20534444)        // "DDS "
      errorMessage("input file is not in DDS format");
    if (hdrBuf[1] != 0x7C)              // size of DDS_HEADER
      errorMessage("invalid DDS header size");
    if ((hdrBuf[2] & 0x100F) != 0x100F) // caps, height, width, pitch, pixelfmt
      errorMessage("invalid or unsupported DDS format");
    height = int(hdrBuf[3]);
    width = int(hdrBuf[4]);
    if (height < 1 || height > 65536 || width < 8 || width > 65536 ||
        (width & 7) != 0 || (hdrBuf[5] % (unsigned int) width) != 0)
    {
      errorMessage("invalid or unsupported DDS image dimensions");
    }
    int     bitsPerPixel = int((hdrBuf[5] / (unsigned int) width) << 3);
    if (bitsPerPixel < 8 || bitsPerPixel > 32)
      errorMessage("invalid or unsupported DDS image bits per pixel");
    pixelFormat = pixelFormatUnknown | bitsPerPixel;
    if (hdrBuf[19] != 0x20)
      errorMessage("invalid DDS_PIXELFORMAT size");
    ddsHeaderSize = 128;
    if (hdrBuf[20] & 0x04)              // DDPF_FOURCC
    {
      unsigned int  fourCC = hdrBuf[21];
      if (fourCC == 0x30315844)         // "DX10"
      {
        if (hdrBuf[33] != 3)            // D3D10_RESOURCE_DIMENSION_TEXTURE2D
          errorMessage("invalid or unsupported DDS format");
        ddsHeaderSize = 148;
        fourCC = hdrBuf[32];
      }
      switch (fourCC)
      {
        case 0x3D:                      // DXGI_FORMAT_R8_UNORM
        case 0x3E:                      // DXGI_FORMAT_R8_UINT
          if (bitsPerPixel == 8)
            pixelFormat = pixelFormatGRAY8;
          break;
        case 0x38:                      // DXGI_FORMAT_R16_UNORM
        case 0x39:                      // DXGI_FORMAT_R16_UINT
          if (bitsPerPixel == 16)
            pixelFormat = pixelFormatGRAY16;
          break;
        case 0x56:                      // DXGI_FORMAT_B5G5R5A1_UNORM
          if (bitsPerPixel == 16)
            pixelFormat = pixelFormatRGBA16;
          break;
        case 0x57:                      // DXGI_FORMAT_B8G8R8A8_UNORM
        case 0x5B:                      // DXGI_FORMAT_B8G8R8A8_UNORM_SRGB
          if (bitsPerPixel == 32)
            pixelFormat = pixelFormatRGBA32;
          break;
        case 0x41:                      // DXGI_FORMAT_A8_UNORM
          if (bitsPerPixel == 8)
            pixelFormat = pixelFormatA8;
          break;
        case 0x2A:                      // DXGI_FORMAT_R32_UINT
          if (bitsPerPixel == 32)
            pixelFormat = pixelFormatR32;
          break;
      }
    }
    else
    {
      if (!(hdrBuf[20] & 0x00020243))   // RGB, YUV, luminance, alpha
        errorMessage("invalid or unsupported DDS format");
      if (!(hdrBuf[20] & 0x03))         // no alpha channel
        hdrBuf[26] = 0;
      if (hdrBuf[22] != (unsigned int) bitsPerPixel)
        errorMessage("DDS_PIXELFORMAT bit count inconsistent with pitch");
      unsigned long long  rgMask, baMask;
      rgMask = ((unsigned long long) hdrBuf[24] << 32) | hdrBuf[23];
      baMask = ((unsigned long long) hdrBuf[26] << 32) | hdrBuf[25];
      switch (bitsPerPixel)
      {
        case 8:
          if (hdrBuf[23] == 0xFF)
            pixelFormat = pixelFormatGRAY8;
          else if (hdrBuf[26] == 0xFF)
            pixelFormat = pixelFormatA8;
          break;
        case 16:
          if (hdrBuf[23] == 0xFFFF)
            pixelFormat = pixelFormatGRAY16;
          else if (rgMask == 0x03E000007C00ULL && baMask == 0x80000000001FULL)
            pixelFormat = pixelFormatRGBA16;
          else if (hdrBuf[23] == 0x00FF && hdrBuf[26] == 0xFF00)
            pixelFormat = pixelFormatL8A8;
          else if (hdrBuf[26] == 0xFFFF)
            pixelFormat = pixelFormatA16;
          break;
        case 24:
          if (rgMask == 0xFF0000FF0000ULL && hdrBuf[25] == 0x000000FF)
            pixelFormat = pixelFormatRGB24;
          break;
        case 32:
          if (rgMask == 0xFF0000FF0000ULL && baMask == 0xFF000000000000FFULL)
            pixelFormat = pixelFormatRGBA32;
          else if (hdrBuf[23] == 0x000000FF && hdrBuf[26] == 0xFFFFFF00U)
            pixelFormat = pixelFormatL8A24;
          else if (hdrBuf[26] == 0xFFFFFFFFU)
            pixelFormat = pixelFormatA32;
          else if (hdrBuf[23] == 0xFFFFFFFFU)
            pixelFormat = pixelFormatR32;
          if (rgMask == 0x0FFC003FF00000ULL && baMask == 0xC0000000000003FFULL)
            pixelFormat = pixelFormatA2R10G10B10;
          break;
      }
    }
    if (fileBufSize < (ddsHeaderSize + (size_t(height) * hdrBuf[5])))
      errorMessage("DDS file is shorter than expected");
    if (!(hdrBuf[27] & 0x1000))         // DDSCAPS_TEXTURE
      errorMessage("invalid or unsupported DDS format");
    if (pixelFormat == pixelFormatGRAY8 &&
        hdrBuf[9] == 0x444E414C && hdrBuf[17] == 2)     // "LAND"
    {
      pixelFormat = pixelFormatR8;
    }
  }
  catch (...)
  {
    closeFileStream();
    throw;
  }
  fileBuf = fileBuf + ddsHeaderSize;
  fileBufSize = fileBufSize - ddsHeaderSize;
}

DDSInputFile::DDSInputFile(const unsigned char *fileData, size_t fileSize,
                           int& width, int& height, int& pixelFormat,
                           unsigned int *hdrReserved)
  : FileBuffer(fileData, fileSize),
    ddsHeaderSize(0)
{
  readDDSHeader(width, height, pixelFormat, hdrReserved);
}

DDSInputFile::DDSInputFile(const char *fileName,
                           int& width, int& height, int& pixelFormat,
                           unsigned int *hdrReserved)
  : FileBuffer(fileName),
    ddsHeaderSize(0)
{
  readDDSHeader(width, height, pixelFormat, hdrReserved);
}

DDSInputFile::~DDSInputFile()
{
  closeFileStream();
}

DDSOutputFile::DDSOutputFile(const char *fileName,
                             int width, int height, int pixelFormat,
                             const unsigned int *hdrReserved, size_t bufSize)
  : OutputFile(fileName, bufSize)
{
  unsigned char hdrBuf[128];
  unsigned int  rMask = 0;
  unsigned int  gMask = 0;
  unsigned int  bMask = 0;
  unsigned int  aMask = 0;
  int           bitsPerPixel = 0;
  switch (pixelFormat)
  {
    case DDSInputFile::pixelFormatGRAY8:
      rMask = 0x000000FF;
      bitsPerPixel = 8;
      break;
    case DDSInputFile::pixelFormatGRAY16:
      rMask = 0x0000FFFF;
      bitsPerPixel = 16;
      break;
    case DDSInputFile::pixelFormatRGBA16:
      rMask = 0x00007C00;
      gMask = 0x000003E0;
      bMask = 0x0000001F;
      aMask = 0x00008000;
      bitsPerPixel = 16;
      break;
    case DDSInputFile::pixelFormatL8A8:
      rMask = 0x000000FF;
      aMask = 0x0000FF00;
      bitsPerPixel = 16;
      break;
    case DDSInputFile::pixelFormatRGB24:
      rMask = 0x00FF0000;
      gMask = 0x0000FF00;
      bMask = 0x000000FF;
      bitsPerPixel = 24;
      break;
    case DDSInputFile::pixelFormatRGBA32:
      rMask = 0x00FF0000;
      gMask = 0x0000FF00;
      bMask = 0x000000FF;
      aMask = 0xFF000000U;
      bitsPerPixel = 32;
      break;
    case DDSInputFile::pixelFormatL8A24:
      rMask = 0x000000FF;
      aMask = 0xFFFFFF00U;
      bitsPerPixel = 32;
      break;
    case DDSInputFile::pixelFormatA8:
      aMask = 0x000000FF;
      bitsPerPixel = 8;
      break;
    case DDSInputFile::pixelFormatA16:
      aMask = 0x0000FFFF;
      bitsPerPixel = 16;
      break;
    case DDSInputFile::pixelFormatA32:
      aMask = 0xFFFFFFFFU;
      bitsPerPixel = 32;
      break;
    case DDSInputFile::pixelFormatR8:
      rMask = 0x000000FF;
      bitsPerPixel = 8;
      break;
    case DDSInputFile::pixelFormatR32:
      rMask = 0xFFFFFFFFU;
      bitsPerPixel = 32;
      break;
    case DDSInputFile::pixelFormatA2R10G10B10:
      rMask = 0x3FF00000U;
      gMask = 0x000FFC00U;
      bMask = 0x000003FFU;
      aMask = 0xC0000000U;
      bitsPerPixel = 32;
      break;
    default:
      errorMessage("DDSOutputFile: invalid pixel format");
  }
  for (int i = 0; i < 32; i++)
  {
    unsigned int  tmp = 0;
    switch (i)
    {
      case 0:
        tmp = 0x20534444;       // "DDS "
        break;
      case 1:
        tmp = 0x7C;             // size of DDS_HEADER
        break;
      case 2:
        tmp = 0x0000100F;       // caps, height, width, pitch, pixel format
        break;
      case 3:
        tmp = (unsigned int) height;
        break;
      case 4:
        tmp = (unsigned int) width;
        break;
      case 5:
        tmp = (unsigned int) width * ((unsigned int) bitsPerPixel >> 3);
        break;
      case 19:
        tmp = 0x20;             // size of DDS_PIXELFORMAT
        break;
      case 20:
        if (aMask)
          tmp = 0x01;           // DDPF_ALPHAPIXELS
        if (bMask)
          tmp = tmp | 0x40;     // DDPF_RGB
        else
          tmp = tmp | 0x00020000;       // DDPF_LUMINANCE
        break;
      case 22:
        tmp = (unsigned int) bitsPerPixel;
        break;
      case 23:
        tmp = rMask;
        break;
      case 24:
        tmp = gMask;
        break;
      case 25:
        tmp = bMask;
        break;
      case 26:
        tmp = aMask;
        break;
      case 27:
        tmp = 0x1000;           // DDSCAPS_TEXTURE
        break;
      default:
        if (hdrReserved && (i >= 8 && i < 19))
          tmp = hdrReserved[i - 8];
        break;
    }
    hdrBuf[i << 2] = (unsigned char) (tmp & 0xFF);
    hdrBuf[(i << 2) + 1] = (unsigned char) ((tmp >> 8) & 0xFF);
    hdrBuf[(i << 2) + 2] = (unsigned char) ((tmp >> 16) & 0xFF);
    hdrBuf[(i << 2) + 3] = (unsigned char) ((tmp >> 24) & 0xFF);
  }
  writeData(hdrBuf, 128);
}

DDSOutputFile::~DDSOutputFile()
{
}

void DDSOutputFile::writeImageData(
    const std::uint32_t *p, size_t n, int pixelFormatOut, int pixelFormatIn)
{
  if (pixelFormatIn == DDSInputFile::pixelFormatA2R10G10B10)
  {
    if (pixelFormatOut == DDSInputFile::pixelFormatA2R10G10B10)
    {
#if defined(__i386__) || defined(__x86_64__) || defined(__x86_64)
      writeData(p, n * sizeof(std::uint32_t));
#else
      for ( ; n > 0; p++, n--)
      {
        std::uint32_t c = *p;
        writeByte((unsigned char) (c & 0xFF));
        writeByte((unsigned char) ((c >> 8) & 0xFF));
        writeByte((unsigned char) ((c >> 16) & 0xFF));
        writeByte((unsigned char) ((c >> 24) & 0xFF));
      }
#endif
      return;
    }
  }
  else if (pixelFormatOut == DDSInputFile::pixelFormatRGB24)
  {
    for ( ; n > 0; p++, n--)
    {
      std::uint32_t c = *p;
      writeByte((unsigned char) ((c >> 16) & 0xFF));    // B
      writeByte((unsigned char) ((c >> 8) & 0xFF));     // G
      writeByte((unsigned char) (c & 0xFF));            // R
    }
    return;
  }
  else if (pixelFormatOut == DDSInputFile::pixelFormatRGBA32)
  {
    for ( ; n > 0; p++, n--)
    {
      std::uint32_t c = *p;
      writeByte((unsigned char) ((c >> 16) & 0xFF));    // B
      writeByte((unsigned char) ((c >> 8) & 0xFF));     // G
      writeByte((unsigned char) (c & 0xFF));            // R
      writeByte((unsigned char) ((c >> 24) & 0xFF));    // A
    }
    return;
  }
  for ( ; n > 0; p++, n--)
  {
    FloatVector4  tmp;
    if (pixelFormatIn == DDSInputFile::pixelFormatA2R10G10B10)
      tmp = FloatVector4::convertA2R10G10B10(*p);
    else
      tmp = FloatVector4(p);
    if (pixelFormatOut == DDSInputFile::pixelFormatRGB24)
    {
      std::uint32_t c = std::uint32_t(tmp);
      writeByte((unsigned char) ((c >> 16) & 0xFF));
      writeByte((unsigned char) ((c >> 8) & 0xFF));
      writeByte((unsigned char) (c & 0xFF));
    }
    else if (pixelFormatOut == DDSInputFile::pixelFormatRGBA32)
    {
      std::uint32_t c = std::uint32_t(tmp);
      writeByte((unsigned char) ((c >> 16) & 0xFF));
      writeByte((unsigned char) ((c >> 8) & 0xFF));
      writeByte((unsigned char) (c & 0xFF));
      writeByte((unsigned char) ((c >> 24) & 0xFF));
    }
    else if (pixelFormatOut == DDSInputFile::pixelFormatA2R10G10B10)
    {
      std::uint32_t c = tmp.convertToA2R10G10B10(true);
      writeByte((unsigned char) (c & 0xFF));
      writeByte((unsigned char) ((c >> 8) & 0xFF));
      writeByte((unsigned char) ((c >> 16) & 0xFF));
      writeByte((unsigned char) ((c >> 24) & 0xFF));
    }
    else if (pixelFormatOut == DDSInputFile::pixelFormatRGBA16)
    {
      tmp *= FloatVector4(31.0f / 255.0f, 31.0f / 255.0f, 31.0f / 255.0f, 1.0f);
      std::uint32_t c = std::uint32_t(tmp);
      c = ((c << 10) & 0x7C00U) | ((c >> 3) & 0x03E0U) | ((c >> 16) & 0x801FU);
      writeByte((unsigned char) (c & 0xFF));
      writeByte((unsigned char) ((c >> 8) & 0xFF));
    }
  }
}

