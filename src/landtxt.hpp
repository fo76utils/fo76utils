
#ifndef LANDTXT_HPP_INCLUDED
#define LANDTXT_HPP_INCLUDED

#include "common.hpp"
#include "ddstxt.hpp"
#include "landdata.hpp"
#include "fp32vec4.hpp"
#include "material.hpp"

struct LandscapeTextureSet
{
  // textures[0] = albedo (_color.dds)
  // textures[1] = normal map (_normal.dds)
  // textures[2] = alpha (_opacity.dds)
  // textures[3] = roughness (_rough.dds)
  // textures[4] = metalness (_metal.dds)
  // textures[5] = ambient occlusion (_ao.dds)
  // textures[6] = height map (_height.dds)
  // textures[7] = glow map (_emissive.dds)
  const DDSTexture  *textures[8];
  LandscapeTextureSet()
  {
    for (size_t i = 0; i < 8; i++)
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
  unsigned char txtSetMip;
  std::uint32_t defaultColor;
  static inline FloatVector4 rotateNormalSF(const FloatVector4& n);
  static inline FloatVector4 blendColors(FloatVector4 c0, FloatVector4 c1,
                                         float f);
  static inline void blendNormals(FloatVector4 *n, FloatVector4 *n2, float f);
  inline FloatVector4 getNormal(size_t t, int x, int y, int m) const;
  inline void getPBRMapsI(FloatVector4 *n, size_t t, int x, int y, int m) const;
  inline void getPBRMapsF(FloatVector4 *n,
                          size_t t, float x, float y, float m) const;
  static inline std::uint32_t normalToColor(FloatVector4 n);
  static FloatVector4 renderPixelSF_I_NoNormals(
      const LandscapeTexture& p, FloatVector4 *n,
      int x, int y, int txtX, int txtY);
  static FloatVector4 renderPixelSF_I_NoPBR(
      const LandscapeTexture& p, FloatVector4 *n,
      int x, int y, int txtX, int txtY);
  static FloatVector4 renderPixelSF_I(
      const LandscapeTexture& p, FloatVector4 *n,
      int x, int y, int txtX, int txtY);
  static FloatVector4 renderPixelSF_F(
      const LandscapeTexture& p, FloatVector4 *n,
      int x, int y, int txtX, int txtY);
  void setRenderPixelFunction(bool haveNormalMaps, bool havePBRMaps);
  void copyTextureSet(const LandscapeTextureSet *landTxts, size_t landTxtCnt);
 public:
  LandscapeTexture(const unsigned char *txtSetPtr,
                   const unsigned char *ltex16Ptr,
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
  void renderTexture_NoNormals(unsigned char **outBufs, int renderScale,
                               int x0, int y0, int x1, int y1);
  void renderTexture_NoPBR(unsigned char **outBufs, int renderScale,
                           int x0, int y0, int x1, int y1);
 public:
  // output resolution is 2^renderScale pixels per vertex
  // outBufs[0]: diffuse texture in B8G8R8 format
  // outBufs[1]: normal map in X8Y8 format
  // outBufs[3]: roughness map in R8 (grayscale) format
  // outBufs[4]: metalness map in R8 (grayscale) format
  // outBufs[5]: ambient occlusion map in R8 (grayscale) format
  void renderTexture(unsigned char **outBufs, int renderScale,
                     int x0, int y0, int x1, int y1);
};

#endif

