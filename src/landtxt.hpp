
#ifndef LANDTXT_HPP_INCLUDED
#define LANDTXT_HPP_INCLUDED

#include "common.hpp"
#include "ddstxt.hpp"
#include "landdata.hpp"
#include "fp32vec4.hpp"

class LandscapeTexture
{
 protected:
  const unsigned char *txtSetData;
  const unsigned char *ltexData32;
  const unsigned char *vclrData24;
  const unsigned char *ltexData16;
  const unsigned char *vclrData16;
  const unsigned char *gcvrData;
  SYSV_ABI FloatVector4 (*renderPixelFunction)(
      const LandscapeTexture& p, FloatVector4& n,
      int x, int y, int txtX, int txtY);
  const DDSTexture * const  *landTextures;
  const DDSTexture * const  *landTexturesN;
  size_t  landTextureCnt;
  float   mipLevel;
  float   rgbScale;
  int     width;
  int     height;
  float   txtScale;
  bool    integerMip;
  bool    isFO76;
  unsigned char txtSetMip;
  unsigned char fo76VClrMip;
  unsigned int  defaultColor;
  static inline FloatVector4 rotateNormalFO76(FloatVector4 n);
  static inline FloatVector4 blendColors(FloatVector4 c0, FloatVector4 c1,
                                         float f);
  inline FloatVector4 getFO76VertexColor(size_t offs) const;
  inline FloatVector4 getTES4VertexColor(size_t offs) const;
  inline FloatVector4 getFO76VertexColor(int x, int y, int renderScale) const;
  inline FloatVector4 getTES4VertexColor(int x, int y, int renderScale) const;
  static SYSV_ABI FloatVector4 renderPixelFO76I_NoNormals(
      const LandscapeTexture& p, FloatVector4& n,
      int x, int y, int txtX, int txtY);
  static SYSV_ABI FloatVector4 renderPixelFO76I(
      const LandscapeTexture& p, FloatVector4& n,
      int x, int y, int txtX, int txtY);
  static SYSV_ABI FloatVector4 renderPixelFO76F(
      const LandscapeTexture& p, FloatVector4& n,
      int x, int y, int txtX, int txtY);
  static SYSV_ABI FloatVector4 renderPixelFO76I_GCVR(
      const LandscapeTexture& p, FloatVector4& n,
      int x, int y, int txtX, int txtY);
  static SYSV_ABI FloatVector4 renderPixelFO76F_GCVR(
      const LandscapeTexture& p, FloatVector4& n,
      int x, int y, int txtX, int txtY);
  static SYSV_ABI FloatVector4 renderPixelTES4I_NoNormals(
      const LandscapeTexture& p, FloatVector4& n,
      int x, int y, int txtX, int txtY);
  static SYSV_ABI FloatVector4 renderPixelTES4I(
      const LandscapeTexture& p, FloatVector4& n,
      int x, int y, int txtX, int txtY);
  static SYSV_ABI FloatVector4 renderPixelTES4F(
      const LandscapeTexture& p, FloatVector4& n,
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
  inline FloatVector4 renderPixel(FloatVector4& n,
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

