
#ifndef FILEBUF_HPP_INCLUDED
#define FILEBUF_HPP_INCLUDED

#include "common.hpp"
#include "fp32vec4.hpp"

class FileBuffer
{
 public:
  // blockSize | (isCompressed << 7), or 0 if the format is invalid/unsupported
  static const unsigned char  dxgiFormatSizeTable[128];
 protected:
  const unsigned char *fileBuf;
  size_t  fileBufSize;
  size_t  filePos;
  std::uintptr_t  fileStream;   // HANDLE on Windows, int on other systems
  inline void closeFileStream();
 public:
  static inline std::uint32_t swapUInt32(unsigned int n);
  // the fast versions of the functions are inline
  // and do not check if the read position is valid
  inline unsigned char readUInt8Fast()
  {
    return fileBuf[filePos++];
  }
  unsigned char readUInt8();
  signed char readInt8();
  inline std::uint16_t readUInt16Fast();
  std::uint16_t readUInt16();
  std::int16_t readInt16();
  inline std::uint32_t readUInt32Fast();
  std::uint32_t readUInt32();
  std::int32_t readInt32();
  float readFloat();
  FloatVector4 readFloatVector4();
  FloatVector4 readFloat16Vector4();
  std::uint64_t readUInt64();
  float readFloat16();
  // read n bytes of string data, or until '\0' or end of buffer by default
  void readString(std::string& s, size_t n = std::string::npos);
  // read string with upper case letters converted to lower case, '\\'
  // to '/', and ensuring that it begins with prefix and ends with suffix
  void readPath(std::string& s, size_t n = std::string::npos,
                const char *prefix = 0, const char *suffix = 0);
  // read data from specified offset or pointer
  unsigned char readUInt8(size_t offs) const;
  std::uint16_t readUInt16(size_t offs) const;
  std::uint32_t readUInt32(size_t offs) const;
  static inline std::uint16_t readUInt16Fast(const void *p);
  static inline std::uint32_t readUInt32Fast(const void *p);
  static inline std::uint64_t readUInt64Fast(const void *p);
  static inline void writeUInt16Fast(void *p, std::uint16_t n);
  static inline void writeUInt32Fast(void *p, std::uint32_t n);
  static inline void writeUInt64Fast(void *p, std::uint64_t n);
  static inline bool checkType(unsigned int id, const char *s);
  inline size_t size() const
  {
    return fileBufSize;
  }
  inline const unsigned char *data() const
  {
    return fileBuf;
  }
  inline const unsigned char& front() const
  {
    return fileBuf[0];
  }
  inline const unsigned char& back() const
  {
    return fileBuf[fileBufSize - 1];
  }
  inline unsigned char operator[](size_t n) const
  {
    return fileBuf[n];
  }
  inline size_t getPosition() const
  {
    return filePos;
  }
  inline void setPosition(size_t n)
  {
    filePos = n;
  }
  inline const unsigned char *getDataPtr() const
  {
    return fileBuf;
  }
  inline const unsigned char *getReadPtr() const
  {
    return fileBuf + filePos;
  }
  void setBuffer(const unsigned char *fileData, size_t fileSize);
  FileBuffer();
  FileBuffer(const unsigned char *fileData, size_t fileSize);
  FileBuffer(const char *fileName);
  inline FileBuffer(const unsigned char *fileData,
                    size_t fileSize, size_t filePosition)
    : fileBuf(fileData),
      fileBufSize(fileSize),
      filePos(filePosition),
      fileStream(std::uintptr_t(-1))
  {
  }
  virtual ~FileBuffer();
  static bool getDefaultDataPath(std::string& dataPath);
 protected:
  // on Windows: returns HANDLE from CreateFile()
  // on other systems: returns int from open()
  // a negative value cast to std::uintptr_t is returned on error
  static std::uintptr_t openFileInDataPath(const char *fileName);
 public:
  static void printHexData(std::string& s, const unsigned char *p, size_t n,
                           size_t bytesPerLine = 16);
  // 'p' must have enough space for 148 bytes
  // returns false if dxgiFmt is invalid or unsupported
  static bool writeDDSHeader(unsigned char *p, unsigned char dxgiFmt,
                             int width, int height, int mipCnt = 0,
                             bool isCubeMap = false, int arraySize = 0);
};

inline std::uint32_t FileBuffer::swapUInt32(unsigned int n)
{
  std::uint32_t tmp = std::uint32_t(n);
#if ENABLE_X86_64_SIMD
  __asm__ ("bswap %0" : "+r" (tmp));
#else
  tmp = ((tmp & 0x000000FFU) << 24) | ((tmp & 0x0000FF00U) << 8)
        | ((tmp & 0x00FF0000U) >> 8) | ((tmp & 0xFF000000U) >> 24);
#endif
  return tmp;
}

inline std::uint16_t FileBuffer::readUInt16Fast()
{
  const unsigned char *p = fileBuf + filePos;
  filePos = filePos + 2;
#if defined(__i386__) || defined(__x86_64__) || defined(__x86_64)
  return *(reinterpret_cast< const std::uint16_t * >(p));
#else
  return (std::uint16_t(p[0]) | (std::uint16_t(p[1]) << 8));
#endif
}

inline std::uint32_t FileBuffer::readUInt32Fast()
{
  const unsigned char *p = fileBuf + filePos;
  filePos = filePos + 4;
#if defined(__i386__) || defined(__x86_64__) || defined(__x86_64)
  return *(reinterpret_cast< const std::uint32_t * >(p));
#else
  return (std::uint32_t(p[0]) | (std::uint32_t(p[1]) << 8)
          | (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24));
#endif
}

inline std::uint16_t FileBuffer::readUInt16Fast(const void *p)
{
#if defined(__i386__) || defined(__x86_64__) || defined(__x86_64)
  return *(reinterpret_cast< const std::uint16_t * >(p));
#else
  const unsigned char *q = reinterpret_cast< const unsigned char * >(p);
  return (std::uint16_t(q[0]) | (std::uint16_t(q[1]) << 8));
#endif
}

inline std::uint32_t FileBuffer::readUInt32Fast(const void *p)
{
#if defined(__i386__) || defined(__x86_64__) || defined(__x86_64)
  return *(reinterpret_cast< const std::uint32_t * >(p));
#else
  const unsigned char *q = reinterpret_cast< const unsigned char * >(p);
  return (std::uint32_t(q[0]) | (std::uint32_t(q[1]) << 8)
          | (std::uint32_t(q[2]) << 16) | (std::uint32_t(q[3]) << 24));
#endif
}

inline std::uint64_t FileBuffer::readUInt64Fast(const void *p)
{
#if defined(__i386__) || defined(__x86_64__) || defined(__x86_64)
  return *(reinterpret_cast< const std::uint64_t * >(p));
#else
  const unsigned char *q = reinterpret_cast< const unsigned char * >(p);
  return (std::uint64_t(q[0]) | (std::uint64_t(q[1]) << 8)
          | (std::uint64_t(q[2]) << 16) | (std::uint64_t(q[3]) << 24)
          | (std::uint64_t(q[4]) << 32) | (std::uint64_t(q[5]) << 40)
          | (std::uint64_t(q[6]) << 48) | (std::uint64_t(q[7]) << 56));
#endif
}

inline void FileBuffer::writeUInt16Fast(void *p, std::uint16_t n)
{
#if defined(__i386__) || defined(__x86_64__) || defined(__x86_64)
  *(reinterpret_cast< std::uint16_t * >(p)) = n;
#else
  unsigned char *q = reinterpret_cast< unsigned char * >(p);
  q[0] = (unsigned char) (n & 0xFFU);
  q[1] = (unsigned char) ((n >> 8) & 0xFFU);
#endif
}

inline void FileBuffer::writeUInt32Fast(void *p, std::uint32_t n)
{
#if defined(__i386__) || defined(__x86_64__) || defined(__x86_64)
  *(reinterpret_cast< std::uint32_t * >(p)) = n;
#else
  unsigned char *q = reinterpret_cast< unsigned char * >(p);
  q[0] = (unsigned char) (n & 0xFFU);
  q[1] = (unsigned char) ((n >> 8) & 0xFFU);
  q[2] = (unsigned char) ((n >> 16) & 0xFFU);
  q[3] = (unsigned char) ((n >> 24) & 0xFFU);
#endif
}

inline void FileBuffer::writeUInt64Fast(void *p, std::uint64_t n)
{
#if defined(__i386__) || defined(__x86_64__) || defined(__x86_64)
  *(reinterpret_cast< std::uint64_t * >(p)) = n;
#else
  unsigned char *q = reinterpret_cast< unsigned char * >(p);
  q[0] = (unsigned char) (n & 0xFFU);
  q[1] = (unsigned char) ((n >> 8) & 0xFFU);
  q[2] = (unsigned char) ((n >> 16) & 0xFFU);
  q[3] = (unsigned char) ((n >> 24) & 0xFFU);
  q[4] = (unsigned char) ((n >> 32) & 0xFFU);
  q[5] = (unsigned char) ((n >> 40) & 0xFFU);
  q[6] = (unsigned char) ((n >> 48) & 0xFFU);
  q[7] = (unsigned char) ((n >> 56) & 0xFFU);
#endif
}

inline bool FileBuffer::checkType(unsigned int id, const char *s)
{
#if defined(__i386__) || defined(__x86_64__) || defined(__x86_64)
  return (id == *(reinterpret_cast< const std::uint32_t * >(s)));
#else
  return (char(id & 0xFF) == s[0] && char((id >> 8) & 0xFF) == s[1] &&
          char((id >> 16) & 0xFF) == s[2] && char((id >> 24) & 0xFF) == s[3]);
#endif
}

class OutputFile
{
 protected:
  unsigned char *buf;
  size_t        bufferSize;
  unsigned int  bufWritePos;
  int           fileDesc;
  std::FILE     *f;
  void flushBuffer();
 public:
  OutputFile(const char *fileName, size_t bufSize = 4096);
  virtual ~OutputFile();
  void writeData(const void *p, size_t n);
  inline void writeByte(unsigned char c)
  {
    buf[bufWritePos] = c;
    if (++bufWritePos >= bufferSize)
      flushBuffer();
  }
  void flush();
};

class DDSInputFile : public FileBuffer
{
 protected:
  size_t  ddsHeaderSize;
  void readDDSHeader(int& width, int& height, int& pixelFormat,
                     unsigned int *hdrReserved = nullptr);
 public:
  enum
  {
    // Also used for color mapped images, the luminance is the palette index.
    pixelFormatGRAY8 = 1,
    pixelFormatGRAY16 = 2,
    pixelFormatRGBA16 = 3,
    // For ground cover, this format is used similarly to pixelFormatL8A24,
    // but with 1-bit opacities.
    pixelFormatL8A8 = 4,
    pixelFormatRGB24 = 5,
    pixelFormatRGBA32 = 6,
    // Special 32 bits per pixel format for encoding up to 8 texture layers
    // of a landscape with 3-bit opacities. The lowest 8 bits define a texture
    // for layer x & 7 in a 8x1 block, and the upper 24 bits are the 8
    // opacities at this vertex, layer 0 is in bits 8 to 10, and layer 7 is
    // in bits 29 to 31.
    pixelFormatL8A24 = 7,
    // Ground cover opacities only (8 layers).
    pixelFormatA8 = 8,
    // 3-bit opacities for 5 additional texture layers and one with 1-bit alpha.
    // b0 to b2 = top layer.
    pixelFormatA16 = 9,
    // 4-bit opacities for 8 additional texture layers.
    pixelFormatA32 = 10,
    // 8-bit texture IDs, 16 for the texture layers of each cell quadrant.
    // Image width is nCellsX * 2 * 16, height is nCellsY * 2. Layer 0 is the
    // base texture, layers 1 to 5 or 8 are additional textures.
    // If ground cover is present, it is defined in layers 8 to 15.
    pixelFormatR8 = 11,
    // Similar to pixelFormatR8, but with 32-bit texture form IDs.
    pixelFormatR32 = 12,
    pixelFormatA2R10G10B10 = 13,
    pixelFormatUnknown = 0x40000000
  };
  // If the input file is in an uncompressed format that is not one of the
  // supported types listed above, then pixelFormat is set to
  // pixelFormatUnknown + the number of bits per pixel in the lowest 6 bits.
  //
  // The reserved part of the header from file offset 32 to 75 is stored
  // as 11 32-bit unsigned integers in hdrReserved if it is not NULL.
  // Files created by btddump or fo4land use this for storing information
  // about the landscape:
  //   0-1:  "FO76LAND" or "FO4_LAND", the origin of the world is in the
  //         center or the SW corner of cell 0,0, respectively
  //   2-4:  minimum X, Y, and Z coordinates (X and Y in cells)
  //   5-7:  maximum X, Y, and Z coordinates
  //     8:  water level
  //     9:  cell size in vertices (8 to 128 for FO76, 32 for other games,
  //         2 for texture set images)
  //    10:  offset added to texture numbers (non-zero for ground cover),
  //         or default land level if the image is a height map
  DDSInputFile(const unsigned char *fileData, size_t fileSize,
               int& width, int& height, int& pixelFormat,
               unsigned int *hdrReserved = nullptr);
  DDSInputFile(const char *fileName,
               int& width, int& height, int& pixelFormat,
               unsigned int *hdrReserved = nullptr);
  virtual ~DDSInputFile();
  inline const unsigned char *getDDSHeader() const
  {
    return (fileBuf - ddsHeaderSize);
  }
};

class DDSOutputFile : public OutputFile
{
 public:
  // If hdrReserved is not NULL, it contains 11 32-bit integers to be written
  // to the reserved part of the header from file offset 32 to 75.
  DDSOutputFile(const char *fileName,
                int width, int height, int pixelFormat,
                const unsigned int *hdrReserved = nullptr,
                size_t bufSize = 16384);
  virtual ~DDSOutputFile();
  // pixelFormatIn must be either pixelFormatRGBA32 or pixelFormatA2R10G10B10
  void writeImageData(const std::uint32_t *p, size_t n, int pixelFormatOut,
                      int pixelFormatIn =
#if USE_PIXELFMT_RGB10A2
                          DDSInputFile::pixelFormatA2R10G10B10
#else
                          DDSInputFile::pixelFormatRGBA32
#endif
                      );
};

#endif

