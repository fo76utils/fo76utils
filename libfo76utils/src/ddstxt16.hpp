
#ifndef DDSTXT16_HPP_INCLUDED
#define DDSTXT16_HPP_INCLUDED

#include "common.hpp"
#include "filebuf.hpp"
#include "fp32vec4.hpp"
#include "fp32vec8.hpp"

class DDSTexture16
{
 protected:
  struct DXGIFormatInfo
  {
    size_t  (*decodeFunction)(std::uint64_t *,
                              const unsigned char *, unsigned int);
    const char  *name;
    bool    isCompressed;
    bool    isSRGB;
    unsigned char channelCnt;
    unsigned char blockSize;
  };
  static const DXGIFormatInfo dxgiFormatInfoTable[36];
  static const unsigned char  dxgiFormatMap[128];
  static const unsigned char  cubeWrapTable[24];
  std::uint32_t xMaskMip0;              // width - 1
  std::uint32_t yMaskMip0;              // height - 1
  unsigned char maxMipLevel;
  unsigned char channelCntFlags;        // b0-b2: channels, b7: valid cube map
  unsigned char maxTextureNum;
  unsigned char dxgiFormat;             // 0 if constructed from a color
  std::uint32_t textureDataSize;        // total data size / (maxTextureNum + 1)
  std::uint64_t textureColor;           // for 1x1 texture without allocation
  std::uint64_t *textureData[19];
  static size_t decodeBlock_BC1(
      std::uint64_t *dst, const unsigned char *src, unsigned int w);
  static size_t decodeBlock_BC2(
      std::uint64_t *dst, const unsigned char *src, unsigned int w);
  static size_t decodeBlock_BC3(
      std::uint64_t *dst, const unsigned char *src, unsigned int w);
  static size_t decodeBlock_BC4(
      std::uint64_t *dst, const unsigned char *src, unsigned int w);
  static size_t decodeBlock_BC4S(
      std::uint64_t *dst, const unsigned char *src, unsigned int w);
  static size_t decodeBlock_BC5(
      std::uint64_t *dst, const unsigned char *src, unsigned int w);
  static size_t decodeBlock_BC5S(
      std::uint64_t *dst, const unsigned char *src, unsigned int w);
  static size_t decodeBlock_BC6U(
      std::uint64_t *dst, const unsigned char *src, unsigned int w);
  static size_t decodeBlock_BC6S(
      std::uint64_t *dst, const unsigned char *src, unsigned int w);
  static size_t decodeBlock_BC7(
      std::uint64_t *dst, const unsigned char *src, unsigned int w);
  static size_t decodeLine_RGB(
      std::uint64_t *dst, const unsigned char *src, unsigned int w);
  static size_t decodeLine_BGR(
      std::uint64_t *dst, const unsigned char *src, unsigned int w);
  static size_t decodeLine_RGB32(
      std::uint64_t *dst, const unsigned char *src, unsigned int w);
  static size_t decodeLine_BGR32(
      std::uint64_t *dst, const unsigned char *src, unsigned int w);
  static size_t decodeLine_RGBA(
      std::uint64_t *dst, const unsigned char *src, unsigned int w);
  static size_t decodeLine_BGRA(
      std::uint64_t *dst, const unsigned char *src, unsigned int w);
  static size_t decodeLine_R8G8(
      std::uint64_t *dst, const unsigned char *src, unsigned int w);
  static size_t decodeLine_R8(
      std::uint64_t *dst, const unsigned char *src, unsigned int w);
  static size_t decodeLine_RGBA64F(
      std::uint64_t *dst, const unsigned char *src, unsigned int w);
  static size_t decodeLine_RGB9E5(
      std::uint64_t *dst, const unsigned char *src, unsigned int w);
  static size_t decodeLine_R10G10B10A2(
      std::uint64_t *dst, const unsigned char *src, unsigned int w);
  static size_t decodeLine_R8G8S(
      std::uint64_t *dst, const unsigned char *src, unsigned int w);
  static size_t decodeLine_RGBA32S(
      std::uint64_t *dst, const unsigned char *src, unsigned int w);
  static size_t decodeLine_RGBA64(
      std::uint64_t *dst, const unsigned char *src, unsigned int w);
  void loadTextureData(const unsigned char *srcPtr, int n,
                       const DXGIFormatInfo& formatInfo, bool noSRGBExpand);
  void loadTexture(FileBuffer& buf, int mipOffset, bool noSRGBExpand);
  // X, Y coordinates are scaled to -0.5 to xMask + 0.5, -0.5 to yMask + 0.5
  inline bool convertTexCoord(
      int& x0, int& y0, float& xf, float& yf,
      unsigned int& xMask, unsigned int& yMask,
      float x, float y, int mipLevel) const;
  static inline void getNextMipTexCoord(int& x0, int& y0, float& xf, float& yf);
  static inline FloatVector4 getPixelBFloat16(
      const std::uint64_t *p0, const std::uint64_t *p1,
      const std::uint64_t *p2, const std::uint64_t *p3, float xf, float yf);
  static inline FloatVector4 getPixelBFloat16(
      const std::uint64_t *p0, const std::uint64_t *p2, float xf, float yf);
  static inline FloatVector4 getPixelB_Wrap(
      const std::uint64_t *p, int x0, int y0,
      float xf, float yf, unsigned int xMask, unsigned int yMask);
  static inline FloatVector4 getPixelB_Clamp(
      const std::uint64_t *p, int x0, int y0,
      float xf, float yf, unsigned int xMask, unsigned int yMask);
  inline bool convertTexCoord_Cube(
      int& x0, int& y0, float& xf, float& yf,
      unsigned int& xMask, float x, float y, int mipLevel) const;
  static inline void getPixel_CubeWrap(
      FloatVector4& c, float& scale, float weight,
      const std::uint64_t *p, int x, int y, int n, size_t faceDataSize,
      unsigned int xMask);
  static FloatVector4 getPixelB_CubeWrap(
      const std::uint64_t *p, int x0, int y0, int n, size_t faceDataSize,
      float xf, float yf, unsigned int xMask);
  static inline FloatVector4 getPixelB_Cube(
      const std::uint64_t *p, int x0, int y0, int n, size_t faceDataSize,
      float xf, float yf, unsigned int xMask);
 public:
  // mipOffset < 0: use mip level 0 only, and always generate mipmaps
  DDSTexture16(const char *fileName, int mipOffset = 0,
               bool noSRGBExpand = false);
  DDSTexture16(const unsigned char *buf, size_t bufSize, int mipOffset = 0,
               bool noSRGBExpand = false);
  DDSTexture16(FileBuffer& buf, int mipOffset = 0, bool noSRGBExpand = false);
  // create 1x1 texture of color c without allocating memory
  DDSTexture16(FloatVector4 c, bool srgbColor = false);
  ~DDSTexture16();
  inline int getWidth() const
  {
    return int(xMaskMip0 + 1U);
  }
  inline int getHeight() const
  {
    return int(yMaskMip0 + 1U);
  }
  inline int getMaxMipLevel() const
  {
    return int(maxMipLevel);
  }
  inline bool isSRGBTexture() const
  {
    return dxgiFormatInfoTable[dxgiFormatMap[dxgiFormat]].isSRGB;
  }
  inline unsigned char getChannelCount() const
  {
    return channelCntFlags & 7;
  }
  inline bool getIsCubeMap() const
  {
    return (maxTextureNum >= 5);
  }
  inline size_t getTextureCount() const
  {
    return (size_t(maxTextureNum) + 1);
  }
  inline unsigned char getDXGIFormat() const
  {
    return dxgiFormat;
  }
  inline const char *getFormatName() const
  {
    return dxgiFormatInfoTable[dxgiFormatMap[dxgiFormat]].name;
  }
  // get pointer to raw texture data and its total size
  inline const std::uint64_t *data() const
  {
    return textureData[0];
  }
  inline size_t size() const
  {
    return (size_t(textureDataSize) * (maxTextureNum + 1U));
  }
  // no interpolation, returns color in RGBA format (LSB = red, MSB = alpha)
  inline const std::uint64_t& getPixelN(int x, int y, int mipLevel) const
  {
    unsigned int  xMask = xMaskMip0 >> (unsigned char) mipLevel;
    unsigned int  yMask = yMaskMip0 >> (unsigned char) mipLevel;
    return textureData[mipLevel][((unsigned int) y & yMask) * (xMask + 1U)
                                 + ((unsigned int) x & xMask)];
  }
  inline const std::uint64_t& getPixelN(int x, int y, int mipLevel, int n) const
  {
    unsigned int  xMask = xMaskMip0 >> (unsigned char) mipLevel;
    unsigned int  yMask = yMaskMip0 >> (unsigned char) mipLevel;
    const std::uint64_t *p =
        textureData[mipLevel] + (size_t(textureDataSize) * size_t(n));
    return p[((unsigned int) y & yMask) * (xMask + 1U)
             + ((unsigned int) x & xMask)];
  }
  // getPixelN() with mirrored instead of wrapped texture coordinates
  inline const std::uint64_t& getPixelM(int x, int y, int mipLevel) const
  {
    unsigned int  xMask = xMaskMip0 >> (unsigned char) mipLevel;
    unsigned int  yMask = yMaskMip0 >> (unsigned char) mipLevel;
    unsigned int  xc = (unsigned int) x;
    unsigned int  yc = (unsigned int) y;
    xc = (!(xc & (xMask + 1U)) ? xc : ~xc) & xMask;
    yc = (!(yc & (yMask + 1U)) ? yc : ~yc) & yMask;
    return textureData[mipLevel][yc * (xMask + 1U) + xc];
  }
  // getPixelN() with clamped texture coordinates
  inline const std::uint64_t& getPixelC(int x, int y, int mipLevel) const
  {
    unsigned int  xMask = xMaskMip0 >> (unsigned char) mipLevel;
    unsigned int  yMask = yMaskMip0 >> (unsigned char) mipLevel;
    x = (x > 0 ? (x < int(xMask) ? x : int(xMask)) : 0);
    y = (y > 0 ? (y < int(yMask) ? y : int(yMask)) : 0);
    const std::uint64_t *p = textureData[mipLevel];
    return p[(unsigned int) y * (xMask + 1U) + (unsigned int) x];
  }
  // bilinear filtering (getPixelB/getPixelT use normalized texture coordinates)
  FloatVector4 getPixelB(float x, float y, int mipLevel) const;
  // trilinear filtering
#ifdef __GNUC__
  __attribute__ ((__noinline__))
#endif
  FloatVector4 getPixelT(float x, float y, float mipLevel) const;
  // bilinear filtering with mirrored texture coordinates
  FloatVector4 getPixelBM(float x, float y, int mipLevel) const;
  // trilinear filtering with mirrored texture coordinates
  FloatVector4 getPixelTM(float x, float y, float mipLevel) const;
  // bilinear filtering with clamped texture coordinates
  FloatVector4 getPixelBC(float x, float y, int mipLevel) const;
  // trilinear filtering with clamped texture coordinates
  FloatVector4 getPixelTC(float x, float y, float mipLevel) const;
  // cube map with trilinear filtering, faces are expected to be
  // in +X, -X, +Y, -Y, +Z, -Z order (E, W, N, S, top, bottom)
  // x = -1.0 to 1.0: W to E
  // y = -1.0 to 1.0: S to N
  // z = -1.0 to 1.0: bottom to top
  FloatVector4 cubeMap(float x, float y, float z, float mipLevel) const;
  // Calculate a weighted sum of cube map samples, each element of sampleBuf
  // is a vector (X, Y, Z) and a mip level (W) in the range 0.0 to 16.0:
  //     v = (t * X) + (b * Y) + (n * Z)
  //     sample = cubeMap(v[0], v[1], v[2], W) * Z
  FloatVector4 cubeMapImportanceSample(
      FloatVector4 t, FloatVector4 b, FloatVector4 n,
      const FloatVector4 *sampleBuf, size_t sampleCnt) const;
  // Wrap cube map texture coordinates for seamless filtering (n = face number
  // from 0 to 5, xMask = face width - 1). If x and y are both out of range,
  // false is returned, and x, y and n are not changed (the sample should be
  // discarded).
  static inline bool wrapCubeMapCoord(int& x, int& y, int& n, int xMask);
  inline FloatVector4 getPixelB_Inline(float x, float y, int mipLevel) const;
  inline FloatVector4 getPixelT_Inline(float x, float y, float mipLevel) const;
  inline FloatVector4 getPixelBM_Inline(float x, float y, int mipLevel) const;
  inline FloatVector4 getPixelBC_Inline(float x, float y, int mipLevel) const;
  // sRGB / linear color space conversion with normalized colors, better
  // accuracy than FloatVector4 functions, and correct handling of alpha
  static inline FloatVector4 srgbExpand(FloatVector4 c);
  static inline FloatVector8 srgbExpand(FloatVector8 c);
  static inline FloatVector4 srgbCompress(FloatVector4 c);
};

inline bool DDSTexture16::convertTexCoord(
    int& x0, int& y0, float& xf, float& yf,
    unsigned int& xMask, unsigned int& yMask,
    float x, float y, int mipLevel) const
{
  xMask = xMaskMip0 >> (unsigned char) mipLevel;
  yMask = yMaskMip0 >> (unsigned char) mipLevel;
  xf = x * float(int(xMask + 1U)) - 0.5f;
  yf = y * float(int(yMask + 1U)) - 0.5f;
  float   xi = float(std::floor(xf));
  float   yi = float(std::floor(yf));
  x0 = int(xi);
  y0 = int(yi);
  xf -= xi;
  yf -= yi;
  return bool(xMask | yMask);
}

inline void DDSTexture16::getNextMipTexCoord(
    int& x0, int& y0, float& xf, float& yf)
{
  xf = (xf + float(x0)) * 0.5f - 0.25f;
  yf = (yf + float(y0)) * 0.5f - 0.25f;
  float   xi = float(std::floor(xf));
  float   yi = float(std::floor(yf));
  x0 = int(xi);
  y0 = int(yi);
  xf = xf - xi;
  yf = yf - yi;
}

inline FloatVector4 DDSTexture16::getPixelBFloat16(
    const std::uint64_t *p0, const std::uint64_t *p1,
    const std::uint64_t *p2, const std::uint64_t *p3, float xf, float yf)
{
  FloatVector4  c0, c1, c2, c3;
#if ENABLE_X86_64_SIMD >= 3
  __asm__ ("vcvtph2ps %1, %0" : "=x" (c0.v) : "m" (*p0));
  __asm__ ("vcvtph2ps %1, %0" : "=x" (c1.v) : "m" (*p1));
  __asm__ ("vcvtph2ps %1, %0" : "=x" (c2.v) : "m" (*p2));
  __asm__ ("vcvtph2ps %1, %0" : "=x" (c3.v) : "m" (*p3));
#else
  c0 = FloatVector4::convertFloat16(*p0);
  c1 = FloatVector4::convertFloat16(*p1);
  c2 = FloatVector4::convertFloat16(*p2);
  c3 = FloatVector4::convertFloat16(*p3);
#endif
  c0 = (c0 * (1.0f - xf)) + (c1 * xf);
  c2 = (c2 * (1.0f - xf)) + (c3 * xf);
  return (c0 + ((c2 - c0) * yf));
}

inline FloatVector4 DDSTexture16::getPixelBFloat16(
    const std::uint64_t *p0, const std::uint64_t *p2, float xf, float yf)
{
  return getPixelBFloat16(p0, p0 + 1, p2, p2 + 1, xf, yf);
}

inline FloatVector4 DDSTexture16::getPixelB_Wrap(
    const std::uint64_t *p, int x0, int y0,
    float xf, float yf, unsigned int xMask, unsigned int yMask)
{
  unsigned int  w = xMask + 1U;
  unsigned int  x0u = (unsigned int) x0;
  unsigned int  y0u = (unsigned int) y0;
  unsigned int  x1 = (x0u + 1U) & xMask;
  unsigned int  y1 = (y0u + 1U) & yMask;
  x0u = x0u & xMask;
  y0u = y0u & yMask;
  return getPixelBFloat16(p + (y0u * w + x0u), p + (y0u * w + x1),
                          p + (y1 * w + x0u), p + (y1 * w + x1), xf, yf);
}

inline FloatVector4 DDSTexture16::getPixelB_Clamp(
    const std::uint64_t *p, int x0, int y0,
    float xf, float yf, unsigned int xMask, unsigned int yMask)
{
  int     x1 = std::min< int >(std::max< int >(x0 + 1, 0), int(xMask));
  int     y1 = std::min< int >(std::max< int >(y0 + 1, 0), int(yMask));
  x0 = std::min< int >(std::max< int >(x0, 0), int(xMask));
  y0 = std::min< int >(std::max< int >(y0, 0), int(yMask));
  unsigned int  w = xMask + 1U;
  return getPixelBFloat16(p + ((unsigned int) y0 * w + (unsigned int) x0),
                          p + ((unsigned int) y0 * w + (unsigned int) x1),
                          p + ((unsigned int) y1 * w + (unsigned int) x0),
                          p + ((unsigned int) y1 * w + (unsigned int) x1),
                          xf, yf);
}

inline FloatVector4 DDSTexture16::getPixelB_Inline(
    float x, float y, int mipLevel) const
{
  mipLevel = (mipLevel > 0 ? mipLevel : 0);
  int     x0, y0;
  float   xf, yf;
  unsigned int  xMask, yMask;
  (void) convertTexCoord(x0, y0, xf, yf, xMask, yMask, x, y, mipLevel);
  return getPixelB_Wrap(textureData[mipLevel], x0, y0, xf, yf, xMask, yMask);
}

inline FloatVector4 DDSTexture16::getPixelT_Inline(
    float x, float y, float mipLevel) const
{
  mipLevel = std::max(mipLevel, 0.0f);
  int     m0 = int(mipLevel);
  int     x0, y0;
  float   xf, yf;
  unsigned int  xMask, yMask;
  if (!convertTexCoord(x0, y0, xf, yf, xMask, yMask, x, y, m0)) [[unlikely]]
    return FloatVector4::convertFloat16(*(textureData[m0]));
  FloatVector4  c0(getPixelB_Wrap(textureData[m0], x0, y0, xf, yf,
                                  xMask, yMask));
  float   mf = float(m0);
  if (mf != mipLevel) [[likely]]
  {
    mf = mipLevel - mf;
    getNextMipTexCoord(x0, y0, xf, yf);
    FloatVector4  c1(getPixelB_Wrap(textureData[m0 + 1], x0, y0, xf, yf,
                                    xMask >> 1, yMask >> 1));
    c0 = (c0 * (1.0f - mf)) + (c1 * mf);
  }
  return c0;
}

inline FloatVector4 DDSTexture16::getPixelBM_Inline(
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
  (void) convertTexCoord(x0, y0, xf, yf, xMask, yMask, x, y, mipLevel);
  return getPixelB_Clamp(textureData[mipLevel], x0, y0, xf, yf, xMask, yMask);
}

inline FloatVector4 DDSTexture16::getPixelBC_Inline(
    float x, float y, int mipLevel) const
{
  mipLevel = (mipLevel > 0 ? mipLevel : 0);
  int     x0, y0;
  float   xf, yf;
  unsigned int  xMask, yMask;
  (void) convertTexCoord(x0, y0, xf, yf, xMask, yMask, x, y, mipLevel);
  return getPixelB_Clamp(textureData[mipLevel], x0, y0, xf, yf, xMask, yMask);
}

inline bool DDSTexture16::wrapCubeMapCoord(int& x, int& y, int& n, int xMask)
{
  if (!((x | y) & ~xMask)) [[likely]]
    return true;
  if (x & y & ~xMask) [[unlikely]]
    return false;
  const unsigned char *p = &(cubeWrapTable[n << 2]);
  p = p + ((x | y) < 0 ? 1 : 0) + ((unsigned int) x < (unsigned int) y ? 2 : 0);
  unsigned char tmp = *p;
  x = (!(tmp & 0x20) ? x : ~x) & xMask;
  y = (!(tmp & 0x40) ? y : ~y) & xMask;
  if (tmp & 0x80)
    std::swap(x, y);
  n = tmp & 7;
  return true;
}

inline FloatVector4 DDSTexture16::srgbExpand(FloatVector4 c)
{
  const FloatVector4  a4(-0.13984761f, -0.13984761f, -0.13984761f, 0.0f);
  const FloatVector4  a3(0.58740202f, 0.58740202f, 0.58740202f, 0.0f);
  const FloatVector4  a2(0.50849240f, 0.50849240f, 0.50849240f, 0.0f);
  const FloatVector4  a1(0.04395319f, 0.04395319f, 0.04395319f, 1.0f);
  FloatVector4  c2(c * c);
  return (c2 * a4 + (c * a3 + a2)) * c2 + (c * a1);
}

inline FloatVector8 DDSTexture16::srgbExpand(FloatVector8 c)
{
  const FloatVector8  a4(-0.13984761f, -0.13984761f, -0.13984761f, 0.0f,
                         -0.13984761f, -0.13984761f, -0.13984761f, 0.0f);
  const FloatVector8  a3(0.58740202f, 0.58740202f, 0.58740202f, 0.0f,
                         0.58740202f, 0.58740202f, 0.58740202f, 0.0f);
  const FloatVector8  a2(0.50849240f, 0.50849240f, 0.50849240f, 0.0f,
                         0.50849240f, 0.50849240f, 0.50849240f, 0.0f);
  const FloatVector8  a1(0.04395319f, 0.04395319f, 0.04395319f, 1.0f,
                         0.04395319f, 0.04395319f, 0.04395319f, 1.0f);
  FloatVector8  c2(c * c);
  return (c2 * a4 + (c * a3 + a2)) * c2 + (c * a1);
}

inline FloatVector4 DDSTexture16::srgbCompress(FloatVector4 c)
{
  FloatVector4  tmp(c.maxValues(FloatVector4(0.0f)) + 0.000858025f);
  FloatVector4  tmp1(tmp);
  FloatVector4  tmp2(tmp * tmp);
  tmp = tmp.rsqrtFast() * tmp1;
  tmp1 = tmp1 * 0.59302883f + 1.42598062f;
  tmp2 = tmp2 * -0.18732371f - 0.04110602f;
  tmp = (tmp * -0.79095451f + tmp1) * tmp + tmp2;
  return tmp.blendValues(c, 0x08);
}

extern "C" bool detexDecompressBlockBPTC_FLOAT(
    const std::uint8_t *bitstring, std::uint32_t mode_mask,
    std::uint32_t flags, std::uint8_t *pixel_buffer);           // BC6U
extern "C" bool detexDecompressBlockBPTC_SIGNED_FLOAT(
    const std::uint8_t *bitstring, std::uint32_t mode_mask,
    std::uint32_t flags, std::uint8_t *pixel_buffer);           // BC6S
extern "C" bool detexDecompressBlockBPTC(
    const std::uint8_t *bitstring, std::uint32_t mode_mask,
    std::uint32_t flags, std::uint8_t *pixel_buffer);           // BC7

#endif

