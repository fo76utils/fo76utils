
#include "common.hpp"
#include "filebuf.hpp"

#if defined(_WIN32) || defined(_WIN64)
#  include <io.h>
extern "C"
{
// mman-win32 is required on Windows (https://github.com/alitrack/mman-win32)
#  include "mman.c"
}
#else
#  include <unistd.h>
#  include <sys/mman.h>
#endif

unsigned int FileBuffer::swapUInt32(unsigned int n)
{
  n = ((n & 0xFF00FF00U) >> 8) | ((n & 0x00FF00FFU) << 8);
  n = ((n & 0xFFFF0000U) >> 16) | ((n & 0x0000FFFFU) << 16);
  return n;
}

unsigned char FileBuffer::readUInt8()
{
  if ((filePos + 1) > fileBufSize)
    throw errorMessage("end of input file");
  return readUInt8Fast();
}

signed char FileBuffer::readInt8()
{
  if ((filePos + 1) > fileBufSize)
    throw errorMessage("end of input file");
  return (signed char) ((int(readUInt8Fast()) ^ 0x80) - 0x80);
}

unsigned short FileBuffer::readUInt16()
{
  if ((filePos + 2) > fileBufSize)
    throw errorMessage("end of input file");
  return readUInt16Fast();
}

short FileBuffer::readInt16()
{
  if ((filePos + 2) > fileBufSize)
    throw errorMessage("end of input file");
  return uint16ToSigned(readUInt16Fast());
}

unsigned int FileBuffer::readUInt32()
{
  if ((filePos + 4) > fileBufSize)
    throw errorMessage("end of input file");
  return readUInt32Fast();
}

int FileBuffer::readInt32()
{
  if ((filePos + 4) > fileBufSize)
    throw errorMessage("end of input file");
  return uint32ToSigned(readUInt32Fast());
}

float FileBuffer::readFloat()
{
  if ((filePos + 4) > fileBufSize)
    throw errorMessage("end of input file");
  unsigned int  tmp = readUInt32Fast();
  int     e = int((tmp >> 23) & 0xFF);
  if (e == 0x00 || e == 0xFF)
    return 0.0f;
  double  m = double(int((tmp & 0x007FFFFF) | 0x00800000));
  m = std::ldexp(m, e - 150);
  if (tmp & 0x80000000U)
    m = -m;
  return float(m);
}

unsigned long long FileBuffer::readUInt64()
{
  if ((filePos + 8) > fileBufSize)
    throw errorMessage("end of input file");
  unsigned long long  tmp = (unsigned long long) readUInt32Fast();
  tmp = tmp | ((unsigned long long) readUInt32Fast() << 32);
  return tmp;
}

float FileBuffer::readFloat16()
{
  if ((filePos + 2) > fileBufSize)
    throw errorMessage("end of input file");
  unsigned int  tmp = readUInt16Fast();
  unsigned char e = (unsigned char) ((tmp >> 10) & 0x1F);
  if (!e)
    return 0.0f;
  long long m = (long long) ((tmp & 0x03FF) | 0x0400) << e;
  return (float(!(tmp & 0x8000) ? m : -m) * (1.0f / 33554432.0f));
}

void FileBuffer::readString(std::string& s, size_t n)
{
  s.clear();
  if (n == std::string::npos)
  {
    while (filePos < fileBufSize)
    {
      char    c = char(fileBuf[filePos++]);
      if (!c)
        break;
      if ((unsigned char) c < 0x20)
        c = ' ';
      s += c;
    }
    return;
  }
  if (((unsigned long long) filePos + n) > fileBufSize)
    throw errorMessage("end of input file");
  for (size_t i = 0; i < n; i++)
  {
    char    c = char(fileBuf[filePos + i]);
    if (!c)
      break;
    if ((unsigned char) c < 0x20)
      c = ' ';
    s += c;
  }
  filePos = filePos + n;
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
    if (std::strncmp(s.c_str(), prefix, std::strlen(prefix)) != 0)
      s.insert(0, prefix);
  }
  if (suffix && suffix[0] != '\0')
  {
    size_t  suffixLen = std::strlen(suffix);
    if (s.length() < suffixLen ||
        std::strcmp(s.c_str() + (s.length() - suffixLen), suffix) != 0)
    {
      s += suffix;
    }
  }
}

unsigned char FileBuffer::readUInt8(size_t offs) const
{
  if ((offs + 1) > fileBufSize)
    throw errorMessage("end of input file");
  return fileBuf[offs];
}

unsigned short FileBuffer::readUInt16(size_t offs) const
{
  if ((offs + 2) > fileBufSize)
    throw errorMessage("end of input file");
  return readUInt16Fast(fileBuf + offs);
}

unsigned int FileBuffer::readUInt32(size_t offs) const
{
  if ((offs + 4) > fileBufSize)
    throw errorMessage("end of input file");
  return readUInt32Fast(fileBuf + offs);
}

void FileBuffer::setBuffer(const unsigned char *fileData, size_t fileSize)
{
  if (fileStream)
  {
    munmap((void *) fileBuf, fileBufSize);
    std::fclose(fileStream);
    fileStream = (std::FILE *) 0;
  }
  fileBuf = fileData;
  fileBufSize = fileSize;
  filePos = 0;
}

FileBuffer::FileBuffer()
  : fileBuf((unsigned char *) 0),
    fileBufSize(0),
    filePos(0),
    fileStream((std::FILE *) 0)
{
}

FileBuffer::FileBuffer(const unsigned char *fileData, size_t fileSize)
  : fileBuf(fileData),
    fileBufSize(fileSize),
    filePos(0),
    fileStream((std::FILE *) 0)
{
}

FileBuffer::FileBuffer(const char *fileName)
  : filePos(0),
    fileStream((std::FILE *) 0)
{
  if (!fileName || *fileName == '\0')
    throw errorMessage("empty input file name");
  try
  {
#if defined(_WIN32) || defined(_WIN64)
    fileStream = fopen64(fileName, "rb");
    if (!fileStream)
      throw errorMessage("error loading input file");
    if (_fseeki64(fileStream, 0LL, SEEK_END) < 0)
      throw errorMessage("error loading input file");
    long long fsize = _ftelli64(fileStream);
    if (fsize < 0LL || _fseeki64(fileStream, 0LL, SEEK_SET) < 0)
      throw errorMessage("error loading input file");
    fileBufSize = size_t(fsize);
    int     fileDesc = _fileno(fileStream);
#else
    fileStream = std::fopen(fileName, "rb");
    if (!fileStream)
      throw errorMessage("error loading input file");
    if (std::fseek(fileStream, 0L, SEEK_END) < 0)
      throw errorMessage("error loading input file");
    long    fsize = std::ftell(fileStream);
    if (fsize < 0L || std::fseek(fileStream, 0L, SEEK_SET) < 0)
      throw errorMessage("error loading input file");
    fileBufSize = size_t(fsize);
    int     fileDesc = fileno(fileStream);
#endif
    fileBuf = (unsigned char *) mmap(0, fileBufSize, PROT_READ, MAP_PRIVATE,
                                     fileDesc, 0);
    if ((void *) fileBuf == MAP_FAILED)
      throw errorMessage("error loading input file");
  }
  catch (...)
  {
    if (fileStream)
      std::fclose(fileStream);
    throw;
  }
}

FileBuffer::~FileBuffer()
{
  if (fileStream)
  {
    munmap((void *) fileBuf, fileBufSize);
    std::fclose(fileStream);
  }
}

void OutputFile::flushBuffer()
{
  unsigned int  n = bufWritePos;
  bufWritePos = 0;
  writeData(&(buf.front()), sizeof(unsigned char) * n);
}

OutputFile::OutputFile(const char *fileName, size_t bufSize)
  : bufWritePos(0),
    buf(bufSize)
{
#if defined(_WIN32) || defined(_WIN64)
  f = fopen64(fileName, "wb");
#else
  f = std::fopen(fileName, "wb");
#endif
  if (!f)
    throw errorMessage("error opening output file \"%s\"", fileName);
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
}

void OutputFile::writeData(const void *p, size_t n)
{
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
      throw errorMessage("error writing output file");
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
      throw errorMessage("invalid DDS input file size");
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
      throw errorMessage("input file is not in DDS format");
    if (hdrBuf[1] != 0x7C)              // size of DDS_HEADER
      throw errorMessage("invalid DDS header size");
    if ((hdrBuf[2] & 0x100F) != 0x100F) // caps, height, width, pitch, pixelfmt
      throw errorMessage("invalid or unsupported DDS format");
    height = int(hdrBuf[3]);
    width = int(hdrBuf[4]);
    if (height < 1 || height > 65536 || width < 8 || width > 65536 ||
        (width & 7) != 0 || (hdrBuf[5] % (unsigned int) width) != 0)
    {
      throw errorMessage("invalid or unsupported DDS image dimensions");
    }
    int     bitsPerPixel = int((hdrBuf[5] / (unsigned int) width) << 3);
    if (bitsPerPixel < 8 || bitsPerPixel > 32)
      throw errorMessage("invalid or unsupported DDS image bits per pixel");
    pixelFormat = pixelFormatUnknown | bitsPerPixel;
    if (hdrBuf[19] != 0x20)
      throw errorMessage("invalid DDS_PIXELFORMAT size");
    ddsHeaderSize = 128;
    if (hdrBuf[20] & 0x04)              // DDPF_FOURCC
    {
      unsigned int  fourCC = hdrBuf[21];
      if (fourCC == 0x30315844)         // "DX10"
      {
        if (hdrBuf[33] != 3)            // D3D10_RESOURCE_DIMENSION_TEXTURE2D
          throw errorMessage("invalid or unsupported DDS format");
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
        throw errorMessage("invalid or unsupported DDS format");
      if (!(hdrBuf[20] & 0x03))         // no alpha channel
        hdrBuf[26] = 0;
      if (hdrBuf[22] != (unsigned int) bitsPerPixel)
        throw errorMessage("DDS_PIXELFORMAT bit count inconsistent with pitch");
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
          break;
      }
    }
    if (fileBufSize < (ddsHeaderSize + (size_t(height) * hdrBuf[5])))
      throw errorMessage("DDS file is shorter than expected");
    if (!(hdrBuf[27] & 0x1000))         // DDSCAPS_TEXTURE
      throw errorMessage("invalid or unsupported DDS format");
    if (pixelFormat == pixelFormatGRAY8 &&
        hdrBuf[9] == 0x444E414C && hdrBuf[17] == 2)     // "LAND"
    {
      pixelFormat = pixelFormatR8;
    }
  }
  catch (...)
  {
    if (fileStream)
    {
      munmap((void *) fileBuf, fileBufSize);
      fileBuf = (unsigned char *) 0;
      std::fclose(fileStream);
      fileStream = (std::FILE *) 0;
    }
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
  if (fileStream)
  {
    munmap((void *) (fileBuf - ddsHeaderSize), fileBufSize + ddsHeaderSize);
    fileBuf = (unsigned char *) 0;
    std::fclose(fileStream);
    fileStream = (std::FILE *) 0;
  }
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
    default:
      throw errorMessage("DDSOutputFile: invalid pixel format");
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

