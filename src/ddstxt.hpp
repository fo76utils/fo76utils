
#ifndef DDSTXT_HPP_INCLUDED
#define DDSTXT_HPP_INCLUDED

#include "common.hpp"
#include "filebuf.hpp"

class DDSTexture
{
 protected:
  unsigned int  xSizeMip0;
  unsigned int  ySizeMip0;
  int           mipLevelCnt;
  bool          haveAlpha;
  bool          isCubeMap;              // true if textureCnt == 6
  unsigned short  textureCnt;
  unsigned int  *textureData[20];
  size_t        textureDataSize;        // size of textureDataBuf / textureCnt
  unsigned int  *textureDataBuf;
  static size_t decodeBlock_BC1(unsigned int *dst, const unsigned char *src,
                                unsigned int w);
  static size_t decodeBlock_BC2(unsigned int *dst, const unsigned char *src,
                                unsigned int w);
  static size_t decodeBlock_BC3(unsigned int *dst, const unsigned char *src,
                                unsigned int w);
  static size_t decodeBlock_BC4(unsigned int *dst, const unsigned char *src,
                                unsigned int w);
  static size_t decodeBlock_BC4S(unsigned int *dst, const unsigned char *src,
                                 unsigned int w);
  static size_t decodeBlock_BC5(unsigned int *dst, const unsigned char *src,
                                unsigned int w);
  static size_t decodeBlock_BC5S(unsigned int *dst, const unsigned char *src,
                                 unsigned int w);
  static size_t decodeLine_RGB(unsigned int *dst, const unsigned char *src,
                               unsigned int w);
  static size_t decodeLine_BGR(unsigned int *dst, const unsigned char *src,
                               unsigned int w);
  static size_t decodeLine_RGB32(unsigned int *dst, const unsigned char *src,
                                 unsigned int w);
  static size_t decodeLine_BGR32(unsigned int *dst, const unsigned char *src,
                                 unsigned int w);
  static size_t decodeLine_RGBA(unsigned int *dst, const unsigned char *src,
                                unsigned int w);
  static size_t decodeLine_BGRA(unsigned int *dst, const unsigned char *src,
                                unsigned int w);
  static size_t decodeLine_R8G8(unsigned int *dst, const unsigned char *src,
                                unsigned int w);
  void loadTextureData(const unsigned char *srcPtr, int n, size_t blockSize,
                       size_t (*decodeFunction)(unsigned int *,
                                                const unsigned char *,
                                                unsigned int));
  void loadTexture(FileBuffer& buf, int mipOffset);
 public:
  DDSTexture(const char *fileName, int mipOffset = 0);
  DDSTexture(const unsigned char *buf, size_t bufSize, int mipOffset = 0);
  DDSTexture(FileBuffer& buf, int mipOffset = 0);
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
  inline unsigned int getPixelN(int x, int y, int mipLevel) const
  {
    int     xMask = int((xSizeMip0 - 1) >> mipLevel);
    int     yMask = int((ySizeMip0 - 1) >> mipLevel);
    return textureData[mipLevel][(y & yMask) * (xMask + 1) + (x & xMask)];
  }
  inline unsigned int getPixelN(int x, int y, int mipLevel, int n) const
  {
    int     xMask = int((xSizeMip0 - 1) >> mipLevel);
    int     yMask = int((ySizeMip0 - 1) >> mipLevel);
    const unsigned int  *p =
        textureData[mipLevel] + (textureDataSize * size_t(n));
    return p[(y & yMask) * (xMask + 1) + (x & xMask)];
  }
  // getPixelN() with mirrored instead of wrapped texture coordinates
  inline unsigned int getPixelM(int x, int y, int mipLevel) const
  {
    int     xMaskM = int(xSizeMip0 >> mipLevel);
    int     yMaskM = int(ySizeMip0 >> mipLevel);
    x = (!(x & xMaskM) ? x : ~x) & (xMaskM - 1);
    y = (!(y & yMaskM) ? y : ~y) & (yMaskM - 1);
    return textureData[mipLevel][y * xMaskM + x];
  }
  // getPixelN() with clamped texture coordinates
  inline unsigned int getPixelC(int x, int y, int mipLevel) const
  {
    int     xMask = int((xSizeMip0 - 1) >> mipLevel);
    int     yMask = int((ySizeMip0 - 1) >> mipLevel);
    x = (x > 0 ? (x < xMask ? x : xMask) : 0);
    y = (y > 0 ? (y < yMask ? y : yMask) : 0);
    return textureData[mipLevel][y * (xMask + 1) + x];
  }
  // bilinear filtering
  unsigned int getPixelB(float x, float y, int mipLevel) const;
  // trilinear filtering
  unsigned int getPixelT(float x, float y, float mipLevel) const;
  // bilinear filtering with mirrored texture coordinates
  unsigned int getPixelBM(float x, float y, int mipLevel) const;
  // trilinear filtering with mirrored texture coordinates
  unsigned int getPixelTM(float x, float y, float mipLevel) const;
  // bilinear filtering with clamped texture coordinates
  unsigned int getPixelBC(float x, float y, int mipLevel) const;
  // trilinear filtering with clamped texture coordinates
  unsigned int getPixelTC(float x, float y, float mipLevel) const;
  // cube map with bilinear filtering, faces are expected to be
  // in Fallout 4 order (right, left, front, back, bottom, top)
  // x = -1.0 to 1.0: left to right
  // y = -1.0 to 1.0: back to front
  // z = -1.0 to 1.0: bottom to top
  unsigned int cubeMap(float x, float y, float z, float mipLevel) const;
};

unsigned int downsample2xFilter(const unsigned int *buf,
                                int imageWidth, int imageHeight, int x, int y);

#endif

