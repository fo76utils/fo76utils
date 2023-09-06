
#ifndef LANDTXT_HPP_INCLUDED
#define LANDTXT_HPP_INCLUDED

#include "common.hpp"
#include "ddstxt.hpp"
#include "landdata.hpp"
#include "fp32vec4.hpp"

struct LandscapeTextureSet
{
  const DDSTexture  *textures[10];
  LandscapeTextureSet()
  {
    for (size_t i = 0; i < 10; i++)
      textures[i] = (DDSTexture *) 0;
  }
  inline const DDSTexture*& operator[](size_t n)
  {
    return textures[n];
  }
  inline const DDSTexture * const& operator[](size_t n) const
  {
    return textures[n];
  }
};

class LandscapeTexture
{
 protected:
  const unsigned char *txtSetData;
  const unsigned char *ltexData32;
  const unsigned char *vclrData24;
  const unsigned char *ltexData16;
  FloatVector4 (*renderPixelFunction)(
      const LandscapeTexture& p, FloatVector4 *n,
      int x, int y, int txtX, int txtY);
  LandscapeTextureSet *landTextures;
  size_t  landTextureCnt;
  float   mipLevel;
  float   rgbScale;
  int     width;
  int     height;
  float   txtScale;
  bool    integerMip;
  bool    isFO76;
  unsigned char txtSetMip;
  std::uint32_t defaultColor;
  static inline FloatVector4 rotateNormalFO76(const FloatVector4& n);
  static inline FloatVector4 blendColors(FloatVector4 c0, FloatVector4 c1,
                                         float f);
  static inline void blendNormals(FloatVector4 *n, FloatVector4 *n2, float f);
  inline FloatVector4 getNormal(size_t t, int x, int y, int m) const;
  inline void getNormalSpecI(FloatVector4 *n,
                             size_t t, int x, int y, int m) const;
  inline void getNormalSpecF(FloatVector4 *n,
                             size_t t, float x, float y, float m) const;
  inline void getNormalReflLightI(FloatVector4 *n,
                                  size_t t, int x, int y, int m) const;
  inline void getNormalReflLightF(FloatVector4 *n,
                                  size_t t, float x, float y, float m) const;
  static inline std::uint32_t normalToColor(FloatVector4 n);
  inline std::uint32_t getTES4VertexColor(size_t offs) const;
  inline FloatVector4 getTES4VertexColor(int x, int y, int renderScale) const;
  static FloatVector4 renderPixelFO76I_NoNormals(
      const LandscapeTexture& p, FloatVector4 *n,
      int x, int y, int txtX, int txtY);
  static FloatVector4 renderPixelFO76I_NoPBR(
      const LandscapeTexture& p, FloatVector4 *n,
      int x, int y, int txtX, int txtY);
  static FloatVector4 renderPixelFO76I(
      const LandscapeTexture& p, FloatVector4 *n,
      int x, int y, int txtX, int txtY);
  static FloatVector4 renderPixelFO76F(
      const LandscapeTexture& p, FloatVector4 *n,
      int x, int y, int txtX, int txtY);
  static FloatVector4 renderPixelTES4I_NoNormals(
      const LandscapeTexture& p, FloatVector4 *n,
      int x, int y, int txtX, int txtY);
  static FloatVector4 renderPixelTES4I_NoPBR(
      const LandscapeTexture& p, FloatVector4 *n,
      int x, int y, int txtX, int txtY);
  static FloatVector4 renderPixelTES4I(
      const LandscapeTexture& p, FloatVector4 *n,
      int x, int y, int txtX, int txtY);
  static FloatVector4 renderPixelTES4F(
      const LandscapeTexture& p, FloatVector4 *n,
      int x, int y, int txtX, int txtY);
  void setRenderPixelFunction(bool haveNormalMaps, bool havePBRMaps);
  void copyTextureSet(const LandscapeTextureSet *landTxts, size_t landTxtCnt);
 public:
  LandscapeTexture(const unsigned char *txtSetPtr,
                   const unsigned char *ltex32Ptr,
                   const unsigned char *vclr24Ptr,
                   const unsigned char *ltex16Ptr,
                   const unsigned char *vclr16Ptr, const unsigned char *gcvrPtr,
                   int vertexCntX, int vertexCntY, int cellResolution,
                   const LandscapeTextureSet *landTxts, size_t landTxtCnt);
  LandscapeTexture(const LandscapeData& landData,
                   const LandscapeTextureSet *landTxts, size_t landTxtCnt);
  virtual ~LandscapeTexture();
  void setMipLevel(float n);
  void setRGBScale(float n);
  // 0x00RRGGBB
  void setDefaultColor(std::uint32_t c);
  inline FloatVector4 renderPixel(FloatVector4 *n,
                                  int x, int y, int txtX, int txtY) const
  {
    return renderPixelFunction(*this, n, x, y, txtX, txtY);
  }
 protected:
  void renderTexture_NoNormals(unsigned char *outBuf, int renderScale,
                               int x0, int y0, int x1, int y1);
  void renderTexture_NoPBR(unsigned char *outBuf, int renderScale,
                           int x0, int y0, int x1, int y1,
                           unsigned char *outBufN);
 public:
  // output resolution is 2^renderScale pixels per vertex
  // outBuf: diffuse texture in B8G8R8 format
  // outBufN: normal map in X8Y8 format
  // outBufS: specular or lighting map (2 bytes per pixel)
  // outBufR: reflectance map in B8G8R8 format
  void renderTexture(unsigned char *outBuf, int renderScale,
                     int x0, int y0, int x1, int y1,
                     unsigned char *outBufN = (unsigned char *) 0,
                     unsigned char *outBufS = (unsigned char *) 0,
                     unsigned char *outBufR = (unsigned char *) 0);
};

#endif

