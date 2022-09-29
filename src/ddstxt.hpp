
#ifndef DDSTXT_HPP_INCLUDED
#define DDSTXT_HPP_INCLUDED

#include "common.hpp"
#include "filebuf.hpp"
#include "fp32vec4.hpp"

class DDSTexture
{
 protected:
  unsigned int  xSizeMip0;
  unsigned int  ySizeMip0;
  int           mipLevelCnt;
  bool          haveAlpha;
  bool          isCubeMap;              // true if textureCnt == 6
  unsigned short  textureCnt;
  std::uint32_t *textureData[20];
  size_t        textureDataSize;        // size of textureDataBuf / textureCnt
  std::uint32_t *textureDataBuf;
  static size_t decodeBlock_BC1(
      std::uint32_t *dst, const unsigned char *src, unsigned int w);
  static size_t decodeBlock_BC2(
      std::uint32_t *dst, const unsigned char *src, unsigned int w);
  static size_t decodeBlock_BC3(
      std::uint32_t *dst, const unsigned char *src, unsigned int w);
  static size_t decodeBlock_BC4(
      std::uint32_t *dst, const unsigned char *src, unsigned int w);
  static size_t decodeBlock_BC4S(
      std::uint32_t *dst, const unsigned char *src, unsigned int w);
  static size_t decodeBlock_BC5(
      std::uint32_t *dst, const unsigned char *src, unsigned int w);
  static size_t decodeBlock_BC5S(
      std::uint32_t *dst, const unsigned char *src, unsigned int w);
  static size_t decodeLine_RGB(
      std::uint32_t *dst, const unsigned char *src, unsigned int w);
  static size_t decodeLine_BGR(
      std::uint32_t *dst, const unsigned char *src, unsigned int w);
  static size_t decodeLine_RGB32(
      std::uint32_t *dst, const unsigned char *src, unsigned int w);
  static size_t decodeLine_BGR32(
      std::uint32_t *dst, const unsigned char *src, unsigned int w);
  static size_t decodeLine_RGBA(
      std::uint32_t *dst, const unsigned char *src, unsigned int w);
  static size_t decodeLine_BGRA(
      std::uint32_t *dst, const unsigned char *src, unsigned int w);
  static size_t decodeLine_R8G8(
      std::uint32_t *dst, const unsigned char *src, unsigned int w);
  void loadTextureData(const unsigned char *srcPtr, int n, size_t blockSize,
                       size_t (*decodeFunction)(std::uint32_t *,
                                                const unsigned char *,
                                                unsigned int));
  void loadTexture(FileBuffer& buf, int mipOffset);
  // X,Y coordinates are scaled to width,height - int(m)
  inline bool convertTexCoord(
      int& x0, int& y0, float& xf, float& yf,
      unsigned int& xMask, unsigned int& yMask,
      float x, float y, int mipLevel, bool m = false) const;
  static inline FloatVector4 getPixelB(
      const std::uint32_t *p, int x0, int y0,
      float xf, float yf, unsigned int xMask, unsigned int yMask);
  static inline FloatVector4 getPixelB_2(
      const std::uint32_t *p1, const std::uint32_t *p2, int x0, int y0,
      float xf, float yf, unsigned int xMask, unsigned int yMask);
 public:
  DDSTexture(const char *fileName, int mipOffset = 0);
  DDSTexture(const unsigned char *buf, size_t bufSize, int mipOffset = 0);
  DDSTexture(FileBuffer& buf, int mipOffset = 0);
  DDSTexture(std::uint32_t c);          // create 1x1 texture of color c
  virtual ~DDSTexture();
  inline int getWidth() const
  {
    return int(xSizeMip0);
  }
  inline int getHeight() const
  {
    return int(ySizeMip0);
  }
  inline int getMaxMipLevel() const
  {
    return (mipLevelCnt - 1);
  }
  inline bool isRGBATexture() const
  {
    return haveAlpha;
  }
  inline bool getIsCubeMap() const
  {
    return isCubeMap;
  }
  inline size_t getTextureCount() const
  {
    return textureCnt;
  }
  // no interpolation, returns color in RGBA format (LSB = red, MSB = alpha)
  inline std::uint32_t getPixelN(int x, int y, int mipLevel) const
  {
    unsigned int  xMask = (xSizeMip0 - 1U) >> (unsigned char) mipLevel;
    unsigned int  yMask = (ySizeMip0 - 1U) >> (unsigned char) mipLevel;
    return textureData[mipLevel][((unsigned int) y & yMask) * (xMask + 1U)
                                 + ((unsigned int) x & xMask)];
  }
  inline std::uint32_t getPixelN(int x, int y, int mipLevel, int n) const
  {
    unsigned int  xMask = (xSizeMip0 - 1U) >> (unsigned char) mipLevel;
    unsigned int  yMask = (ySizeMip0 - 1U) >> (unsigned char) mipLevel;
    const std::uint32_t *p =
        textureData[mipLevel] + (textureDataSize * size_t(n));
    return p[((unsigned int) y & yMask) * (xMask + 1U)
             + ((unsigned int) x & xMask)];
  }
  // getPixelN() with mirrored instead of wrapped texture coordinates
  inline std::uint32_t getPixelM(int x, int y, int mipLevel) const
  {
    int     xMask = int((xSizeMip0 - 1U) >> (unsigned char) mipLevel);
    int     yMask = int((ySizeMip0 - 1U) >> (unsigned char) mipLevel);
    x = (!(x & (xMask + 1)) ? x : ~x) & xMask;
    y = (!(y & (yMask + 1)) ? y : ~y) & yMask;
    return textureData[mipLevel][y * (xMask + 1) + x];
  }
  // getPixelN() with clamped texture coordinates
  inline std::uint32_t getPixelC(int x, int y, int mipLevel) const
  {
    int     xMask = int((xSizeMip0 - 1U) >> (unsigned char) mipLevel);
    int     yMask = int((ySizeMip0 - 1U) >> (unsigned char) mipLevel);
    x = (x > 0 ? (x < xMask ? x : xMask) : 0);
    y = (y > 0 ? (y < yMask ? y : yMask) : 0);
    return textureData[mipLevel][y * (xMask + 1) + x];
  }
  // bilinear filtering (getPixelB/getPixelT use normalized texture coordinates)
  FloatVector4 getPixelB(float x, float y, int mipLevel) const;
  // trilinear filtering
  FloatVector4 getPixelT(float x, float y, float mipLevel) const;
  // trilinear filtering with the blue and alpha channels taken from the
  // red and green channels of 't'
  FloatVector4 getPixelT_2(float x, float y, float mipLevel,
                           const DDSTexture& t) const;
  // trilinear filtering without normalized texture coordinates
  FloatVector4 getPixelT_N(float x, float y, float mipLevel) const;
  // bilinear filtering with mirrored texture coordinates
  FloatVector4 getPixelBM(float x, float y, int mipLevel) const;
  // trilinear filtering with mirrored texture coordinates
  FloatVector4 getPixelTM(float x, float y, float mipLevel) const;
  // bilinear filtering with clamped texture coordinates
  FloatVector4 getPixelBC(float x, float y, int mipLevel) const;
  // trilinear filtering with clamped texture coordinates
  FloatVector4 getPixelTC(float x, float y, float mipLevel) const;
  // cube map with bilinear filtering, faces are expected to be
  // in Fallout 4 order (right, left, front, back, bottom, top)
  // x = -1.0 to 1.0: left to right
  // y = -1.0 to 1.0: back to front
  // z = -1.0 to 1.0: bottom to top
  FloatVector4 cubeMap(float x, float y, float z, float mipLevel) const;
};

// linePtr: line pointer in output buffer
// imageWidth, imageHeight: dimensions of input image
// y: line number on input image
// fmtFlags & 1: input pixel format (0: R8G8B8A8, 1: A2R10G10B10)
// fmtFlags & 2: output pixel format
void downsample2xFilter_Line(
    std::uint32_t *linePtr, const std::uint32_t *inBuf,
    int imageWidth, int imageHeight, int y,
    unsigned char fmtFlags = (USE_PIXELFMT_RGB10A2 * 3));

// imageWidth, imageHeight: dimensions of input image
// pitch: number of elements per line in outBuf
void downsample2xFilter(
    std::uint32_t *outBuf, const std::uint32_t *inBuf,
    int imageWidth, int imageHeight, int pitch,
    unsigned char fmtFlags = (USE_PIXELFMT_RGB10A2 * 3));

#endif

