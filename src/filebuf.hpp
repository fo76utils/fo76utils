
#ifndef FILEBUF_HPP_INCLUDED
#define FILEBUF_HPP_INCLUDED

#include "common.hpp"

class FileBuffer
{
 protected:
  const unsigned char *fileBuf;
  size_t  fileBufSize;
  size_t  filePos;
  std::FILE *fileStream;
 public:
  static unsigned int swapUInt32(unsigned int n);
  // the fast versions of the functions are inline
  // and do not check if the read position is valid
  inline unsigned char readUInt8Fast()
  {
    return fileBuf[filePos++];
  }
  unsigned char readUInt8();
  signed char readInt8();
  inline unsigned short readUInt16Fast();
  unsigned short readUInt16();
  short readInt16();
  inline unsigned int readUInt32Fast();
  unsigned int readUInt32();
  int readInt32();
  float readFloat();
  unsigned long long readUInt64();
  float readFloat16();
  // read n bytes of string data, or until '\0' or end of buffer by default
  void readString(std::string& s, size_t n = std::string::npos);
  // read string with upper case letters converted to lower case, '\\'
  // to '/', and ensuring that it begins with prefix and ends with suffix
  void readPath(std::string& s, size_t n = std::string::npos,
                const char *prefix = 0, const char *suffix = 0);
  // read data from specified offset or pointer
  unsigned char readUInt8(size_t offs) const;
  unsigned short readUInt16(size_t offs) const;
  unsigned int readUInt32(size_t offs) const;
  static inline unsigned short readUInt16Fast(const void *p);
  static inline unsigned int readUInt32Fast(const void *p);
  static inline bool checkType(unsigned int id, const char *s);
  inline size_t size() const
  {
    return fileBufSize;
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
  void setBuffer(const unsigned char *fileData, size_t fileSize);
  FileBuffer();
  FileBuffer(const unsigned char *fileData, size_t fileSize);
  FileBuffer(const char *fileName);
  virtual ~FileBuffer();
};

inline unsigned short FileBuffer::readUInt16Fast()
{
  const unsigned char *p = fileBuf + filePos;
  filePos = filePos + 2;
#if defined(__i386__) || defined(__x86_64__) || defined(__x86_64)
  return *(reinterpret_cast< const unsigned short * >(p));
#else
  return ((unsigned short) p[0] | ((unsigned short) p[1] << 8));
#endif
}

inline unsigned int FileBuffer::readUInt32Fast()
{
  const unsigned char *p = fileBuf + filePos;
  filePos = filePos + 4;
#if defined(__i386__) || defined(__x86_64__) || defined(__x86_64)
  return *(reinterpret_cast< const unsigned int * >(p));
#else
  return ((unsigned int) p[0] | ((unsigned int) p[1] << 8)
          | ((unsigned int) p[2] << 16) | ((unsigned int) p[3] << 24));
#endif
}

inline unsigned short FileBuffer::readUInt16Fast(const void *p)
{
#if defined(__i386__) || defined(__x86_64__) || defined(__x86_64)
  return *(reinterpret_cast< const unsigned short * >(p));
#else
  const unsigned char *q = reinterpret_cast< const unsigned char * >(p);
  return ((unsigned short) q[0] | ((unsigned short) q[1] << 8));
#endif
}

inline unsigned int FileBuffer::readUInt32Fast(const void *p)
{
#if defined(__i386__) || defined(__x86_64__) || defined(__x86_64)
  return *(reinterpret_cast< const unsigned int * >(p));
#else
  const unsigned char *q = reinterpret_cast< const unsigned char * >(p);
  return ((unsigned int) q[0] | ((unsigned int) q[1] << 8)
          | ((unsigned int) q[2] << 16) | ((unsigned int) q[3] << 24));
#endif
}

inline bool FileBuffer::checkType(unsigned int id, const char *s)
{
#if defined(__i386__) || defined(__x86_64__) || defined(__x86_64)
  return (id == *(reinterpret_cast< const unsigned int * >(s)));
#else
  return (char(id & 0xFF) == s[0] && char((id >> 8) & 0xFF) == s[1] &&
          char((id >> 16) & 0xFF) == s[2] && char((id >> 24) & 0xFF) == s[3]);
#endif
}

class OutputFile
{
 protected:
  std::FILE     *f;
  int           fileDesc;
  unsigned int  bufWritePos;
  std::vector< unsigned char >  buf;
  void flushBuffer();
 public:
  OutputFile(const char *fileName, size_t bufSize = 4096);
  virtual ~OutputFile();
  void writeData(const void *p, size_t n);
  inline void writeByte(unsigned char c)
  {
    buf[bufWritePos] = c;
    if (++bufWritePos >= buf.size())
      flushBuffer();
  }
  void flush();
};

class DDSInputFile : public FileBuffer
{
 protected:
  size_t  ddsHeaderSize;
  void readDDSHeader(int& width, int& height, int& pixelFormat,
                     unsigned int *hdrReserved = (unsigned int *) 0);
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
    // 3-bit opacities for 5 additional texture layers.
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
               unsigned int *hdrReserved = (unsigned int *) 0);
  DDSInputFile(const char *fileName,
               int& width, int& height, int& pixelFormat,
               unsigned int *hdrReserved = (unsigned int *) 0);
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
                const unsigned int *hdrReserved = (unsigned int *) 0,
                size_t bufSize = 16384);
  virtual ~DDSOutputFile();
};

#endif

