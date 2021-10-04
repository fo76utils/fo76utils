
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
  unsigned int  *textureData[20];
  std::vector< unsigned int > textureDataBuf;
  static void decodeBlock_BC1(unsigned int *dst, const unsigned char *src);
  static void decodeBlock_BC3(unsigned int *dst, const unsigned char *src);
  void loadTexture(FileBuffer& buf);
 public:
  DDSTexture(const char *fileName);
  DDSTexture(const unsigned char *buf, size_t bufSize);
  DDSTexture(FileBuffer& buf);
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
  // no interpolation, returns color in RGBA format (LSB = red, MSB = alpha)
  inline unsigned int getPixelN(int x, int y, int mipLevel) const
  {
    int     xMask = int((xSizeMip0 - 1) >> mipLevel);
    int     yMask = int((ySizeMip0 - 1) >> mipLevel);
    return textureData[mipLevel][(y & yMask) * (xMask + 1) + (x & xMask)];
  }
  // bilinear filtering
  unsigned int getPixelB(float x, float y, int mipLevel) const;
  // trilinear filtering
  unsigned int getPixelT(float x, float y, float mipLevel) const;
};

#endif

