
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
#ifdef __GNUC__
  __attribute__ ((__noinline__))
#endif
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
  inline FloatVector4 getPixelB_Inline(float x, float y, int mipLevel) const;
  inline FloatVector4 getPixelT_Inline(float x, float y, float mipLevel) const;
  inline FloatVector4 getPixelBM_Inline(float x, float y, int mipLevel) const;
  inline FloatVector4 getPixelBC_Inline(float x, float y, int mipLevel) const;
};

inline bool DDSTexture::convertTexCoord(
    int& x0, int& y0, float& xf, float& yf,
    unsigned int& xMask, unsigned int& yMask,
    float x, float y, int mipLevel, bool m) const
{
  xMask = (xSizeMip0 - 1U) >> (unsigned char) mipLevel;
  yMask = (ySizeMip0 - 1U) >> (unsigned char) mipLevel;
  xf = x * float(int(xMask + (!m ? 1U : 0U)));
  yf = y * float(int(yMask + (!m ? 1U : 0U)));
  float   xi = float(std::floor(xf));
  float   yi = float(std::floor(yf));
  x0 = int(xi);
  y0 = int(yi);
  xf -= xi;
  yf -= yi;
  return bool(xMask | yMask);
}

inline FloatVector4 DDSTexture::getPixelB(
    const std::uint32_t *p, int x0, int y0,
    float xf, float yf, unsigned int xMask, unsigned int yMask)
{
  unsigned int  w = xMask + 1U;
  unsigned int  x0u = (unsigned int) x0;
  unsigned int  y0u = (unsigned int) y0;
  unsigned int  x1 = (x0u + 1U) & xMask;
  unsigned int  y1 = (y0u + 1U) & yMask;
  x0u = x0u & xMask;
  y0u = y0u & yMask;
  return FloatVector4(p + (y0u * w + x0u), p + (y0u * w + x1),
                      p + (y1 * w + x0u), p + (y1 * w + x1), xf, yf);
}

inline FloatVector4 DDSTexture::getPixelB_Inline(
    float x, float y, int mipLevel) const
{
  mipLevel = (mipLevel > 0 ? mipLevel : 0);
  int     x0, y0;
  float   xf, yf;
  unsigned int  xMask, yMask;
  (void) convertTexCoord(x0, y0, xf, yf, xMask, yMask, x, y, mipLevel);
  return getPixelB(textureData[mipLevel], x0, y0, xf, yf, xMask, yMask);
}

inline FloatVector4 DDSTexture::getPixelT_Inline(
    float x, float y, float mipLevel) const
{
  mipLevel = (mipLevel > 0.0f ? mipLevel : 0.0f);
  int     m0 = int(mipLevel);
  float   mf = mipLevel - float(m0);
  int     x0, y0;
  float   xf, yf;
  unsigned int  xMask, yMask;
  if (BRANCH_UNLIKELY(!convertTexCoord(x0, y0, xf, yf, xMask, yMask, x, y, m0)))
    return FloatVector4(textureData[m0]);
  FloatVector4  c0(getPixelB(textureData[m0], x0, y0, xf, yf, xMask, yMask));
  if (BRANCH_LIKELY(mf >= (1.0f / 512.0f) && mf < (511.0f / 512.0f)))
  {
    xf = (xf + float(x0 & 1)) * 0.5f;
    yf = (yf + float(y0 & 1)) * 0.5f;
    x0 = int((unsigned int) x0 >> 1);
    y0 = int((unsigned int) y0 >> 1);
    FloatVector4  c1(getPixelB(textureData[m0 + 1], x0, y0, xf, yf,
                               xMask >> 1, yMask >> 1));
    c0 = (c0 * (1.0f - mf)) + (c1 * mf);
  }
  return c0;
}

inline FloatVector4 DDSTexture::getPixelBM_Inline(
    float x, float y, int mipLevel) const
{
  mipLevel = (mipLevel > 0 ? mipLevel : 0);
  int     x0, y0;
  float   xf = float(std::floor(x));
  float   yf = float(std::floor(y));
  x = x - xf;
  y = y - yf;
  x = (!(int(xf) & 1) ? x : (1.0f - x));
  y = (!(int(yf) & 1) ? y : (1.0f - y));
  unsigned int  xMask, yMask;
  (void) convertTexCoord(x0, y0, xf, yf, xMask, yMask, x, y, mipLevel, true);
  return getPixelB(textureData[mipLevel], x0, y0, xf, yf, xMask, yMask);
}

inline FloatVector4 DDSTexture::getPixelBC_Inline(
    float x, float y, int mipLevel) const
{
  mipLevel = (mipLevel > 0 ? mipLevel : 0);
  x = (x > 0.0f ? (x < 1.0f ? x : 1.0f) : 0.0f);
  y = (y > 0.0f ? (y < 1.0f ? y : 1.0f) : 0.0f);
  int     x0, y0;
  float   xf, yf;
  unsigned int  xMask, yMask;
  (void) convertTexCoord(x0, y0, xf, yf, xMask, yMask, x, y, mipLevel, true);
  return getPixelB(textureData[mipLevel], x0, y0, xf, yf, xMask, yMask);
}

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

