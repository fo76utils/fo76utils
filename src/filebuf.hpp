
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
  //   0-1:  "FO76LAND" or "FO4_LAND"
  //   2-4:  minimum X, Y, and Z coordinates (X and Y in cells)
  //   5-7:  maximum X, Y, and Z coordinates
  //     8:  water level
  //     9:  cell size in vertices (128 for FO76, 32 for others)
  //    10:  offset added to texture numbers (non-zero for ground cover)
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

