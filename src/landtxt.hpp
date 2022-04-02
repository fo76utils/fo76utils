
#ifndef LANDTXT_HPP_INCLUDED
#define LANDTXT_HPP_INCLUDED

#include "common.hpp"
#include "ddstxt.hpp"
#include "landdata.hpp"

class LandscapeTexture
{
 protected:
  const unsigned char *txtSetData;
  const unsigned char *ltexData32;
  const unsigned char *vclrData24;
  const unsigned char *ltexData16;
  const unsigned char *vclrData16;
  const unsigned char *gcvrData;
  const DDSTexture * const  *landTextures;
  size_t  landTextureCnt;
  float   mipLevel;
  unsigned int  rgbScale;
  unsigned int  defaultColorScaled;
  int     width;
  int     height;
  float   txtScale;
  bool    integerMip;
  bool    isFO76;
  unsigned char txtSetMip;
  unsigned char fo76VClrMip;
  unsigned int  defaultColor;
  static inline unsigned long long rgb24ToRGB64(unsigned int c)
  {
    return ((unsigned long long) (c & 0x000000FF)
            | ((unsigned long long) (c & 0x0000FF00) << 12)
            | ((unsigned long long) (c & 0x00FF0000) << 24));
  }
  inline unsigned int getFO76VertexColor(size_t offs) const
  {
    unsigned int  c = FileBuffer::readUInt16Fast(vclrData16 + (offs << 1));
    c = ((c & 0x7C00) << 6) | ((c & 0x03E0) << 3) | (c & 0x001F);
    return (c * 6U + 0x00202020);
  }
  void getFO76VertexColor(unsigned int& r, unsigned int& g, unsigned int& b,
                          int x, int y) const;
  unsigned long long renderPixelFO76(int x, int y, int txtX, int txtY) const;
  unsigned long long renderPixelTES4(int x, int y, int txtX, int txtY) const;
 public:
  LandscapeTexture(const unsigned char *txtSetPtr,
                   const unsigned char *ltex32Ptr,
                   const unsigned char *vclr24Ptr,
                   const unsigned char *ltex16Ptr,
                   const unsigned char *vclr16Ptr, const unsigned char *gcvrPtr,
                   int vertexCntX, int vertexCntY, int cellResolution,
                   const DDSTexture * const *landTxts, size_t landTxtCnt);
  LandscapeTexture(const LandscapeData& landData,
                   const DDSTexture * const *landTxts, size_t landTxtCnt);
  void setMipLevel(float n);
  void setRGBScale(float n);
  // 0x00RRGGBB
  void setDefaultColor(unsigned int c);
  // returns color in 0x000000GG00BB00RR format
  inline unsigned long long renderPixel(int x, int y, int txtX, int txtY) const
  {
    if (isFO76)
      return renderPixelFO76(x, y, txtX, txtY);
    return renderPixelTES4(x, y, txtX, txtY);
  }
  // output resolution is 2^renderScale pixels per vertex
  void renderTexture(unsigned char *outBuf, int renderScale,
                     int x0, int y0, int x1, int y1) const;
};

#endif

