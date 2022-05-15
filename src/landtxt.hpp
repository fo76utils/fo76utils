
#ifndef LANDTXT_HPP_INCLUDED
#define LANDTXT_HPP_INCLUDED

#include "common.hpp"
#include "ddstxt.hpp"
#include "landdata.hpp"

class LandscapeTexture
{
 protected:
  static const unsigned short fo76VClrTable[32];
  const unsigned char *txtSetData;
  const unsigned char *ltexData32;
  const unsigned char *vclrData24;
  const unsigned char *ltexData16;
  const unsigned char *vclrData16;
  const unsigned char *gcvrData;
  unsigned long long (*renderPixelFunction)(const LandscapeTexture& p,
                                            unsigned int& n,
                                            int x, int y, int txtX, int txtY);
  const DDSTexture * const  *landTextures;
  const DDSTexture * const  *landTexturesN;
  size_t  landTextureCnt;
  float   mipLevel;
  unsigned int  rgbScale;
  int     width;
  int     height;
  float   txtScale;
  bool    integerMip;
  bool    isFO76;
  unsigned char txtSetMip;
  unsigned char fo76VClrMip;
  unsigned int  defaultColor;
  static inline unsigned int blendNormals(unsigned int n0, unsigned int n1,
                                          unsigned int f)
  {
    return ((((n0 * (256U - f)) + (n1 * f) + 0x00800080U) & 0xFF00FF00U) >> 8);
  }
  static inline void rotateNormalFO76(unsigned int& n);
  static inline void rotateNormalTES4(unsigned int& n);
  inline unsigned long long getFO76VertexColor(size_t offs) const;
  inline unsigned long long getTES4VertexColor(size_t offs) const;
  inline void getFO76VertexColor(
      unsigned int& r, unsigned int& g, unsigned int& b,
      int x, int y, int renderScale) const;
  inline void getTES4VertexColor(
      unsigned int& r, unsigned int& g, unsigned int& b,
      int x, int y, int renderScale) const;
  static unsigned long long renderPixelFO76I_NoNormals(
      const LandscapeTexture& p, unsigned int& n,
      int x, int y, int txtX, int txtY);
  static unsigned long long renderPixelFO76I(
      const LandscapeTexture& p, unsigned int& n,
      int x, int y, int txtX, int txtY);
  static unsigned long long renderPixelFO76F(
      const LandscapeTexture& p, unsigned int& n,
      int x, int y, int txtX, int txtY);
  static unsigned long long renderPixelFO76I_GCVR(
      const LandscapeTexture& p, unsigned int& n,
      int x, int y, int txtX, int txtY);
  static unsigned long long renderPixelFO76F_GCVR(
      const LandscapeTexture& p, unsigned int& n,
      int x, int y, int txtX, int txtY);
  static unsigned long long renderPixelTES4I_NoNormals(
      const LandscapeTexture& p, unsigned int& n,
      int x, int y, int txtX, int txtY);
  static unsigned long long renderPixelTES4I(
      const LandscapeTexture& p, unsigned int& n,
      int x, int y, int txtX, int txtY);
  static unsigned long long renderPixelTES4F(
      const LandscapeTexture& p, unsigned int& n,
      int x, int y, int txtX, int txtY);
  void setRenderPixelFunction();
 public:
  LandscapeTexture(const unsigned char *txtSetPtr,
                   const unsigned char *ltex32Ptr,
                   const unsigned char *vclr24Ptr,
                   const unsigned char *ltex16Ptr,
                   const unsigned char *vclr16Ptr, const unsigned char *gcvrPtr,
                   int vertexCntX, int vertexCntY, int cellResolution,
                   const DDSTexture * const *landTxts, size_t landTxtCnt,
                   const DDSTexture * const *landTxtsN = (DDSTexture **) 0);
  LandscapeTexture(const LandscapeData& landData,
                   const DDSTexture * const *landTxts, size_t landTxtCnt,
                   const DDSTexture * const *landTxtsN = (DDSTexture **) 0);
  void setMipLevel(float n);
  void setRGBScale(float n);
  // 0x00RRGGBB
  void setDefaultColor(unsigned int c);
  // returns color in 0x000GGG00BBB00RRR format
  inline unsigned long long renderPixel(unsigned int& n,
                                        int x, int y, int txtX, int txtY) const
  {
    return renderPixelFunction(*this, n, x, y, txtX, txtY);
  }
  // output resolution is 2^renderScale pixels per vertex
  // outBuf: diffuse texture in B8G8R8 format
  // outBufN: normal map in X8Y8 format
  void renderTexture(unsigned char *outBuf, int renderScale,
                     int x0, int y0, int x1, int y1,
                     unsigned char *outBufN = (unsigned char *) 0) const;
};

#endif

