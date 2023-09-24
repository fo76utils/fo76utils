
#ifndef PLOT3D_HPP_INCLUDED
#define PLOT3D_HPP_INCLUDED

#include "common.hpp"
#include "fp32vec4.hpp"
#include "ddstxt.hpp"
#include "material.hpp"
#include "nif_file.hpp"

class Plot3D_TriShape : public NIFFile::NIFTriShape
{
 protected:
  static const float  fresnelRoughTable[1024];
  static const float  fresnelPoly3N_Glass[4];
  static const float  fresnelPoly3_Water[4];
  struct Vertex
  {
    FloatVector4  xyz;
    FloatVector4  tangent;              // tangent[3] = texture U
    FloatVector4  bitangent;            // bitangent[3] = texture V
    FloatVector4  normal;
    FloatVector4  vertexColor;
    inline const float& u() const
    {
      return tangent[3];
    }
    inline const float& v() const
    {
      return bitangent[3];
    }
  };
  struct Fragment : public Vertex
  {
    // pointers to pixel in depth and frame buffer
    float   *zPtr;
    std::uint32_t *cPtr;
    float   mipLevel;
    bool    invNormals;
    // returns normal
    inline FloatVector4 normalMap(FloatVector4 n);
  };
  struct Triangle
  {
    // n = NIFFile::NIFTriShape::triangleData index | (sort_Z << 32)
    std::int64_t  n;
    inline Triangle(size_t i, float z);
    inline bool operator<(const Triangle& r) const
    {
      return (n < r.n);
    }
    // for reverse sorting
    static inline bool gt(const Triangle& r1, const Triangle& r2)
    {
      return (r2.n < r1.n);
    }
    inline operator size_t() const
    {
      return size_t(n & 0xFFFFFFFFLL);
    }
  };
  float   *bufZ;
  std::uint32_t *bufRGBA;
  std::uint32_t *bufN;                  // normals for decal rendering
  int     width;
  int     height;
  const DDSTexture  *textures[9];
  const DDSTexture  *textureE;          // environment map
  float   mipOffsets[8];                // mipOffsets[N] is for textures[N + 1]
  DDSTexture    *defaultTextures;
  unsigned int  renderMode;
  unsigned int  debugMode;
  FloatVector4  lightVector;
  FloatVector4  lightColor;             // lightColor[3] = overall RGB scale
  FloatVector4  ambientLight;
  FloatVector4  envColor;
  FloatVector4  envMapOffs;
  FloatVector4  viewTransformInvX;
  FloatVector4  viewTransformInvY;
  FloatVector4  viewTransformInvZ;
  void    (*drawPixelFunction)(Plot3D_TriShape& p, Fragment& z);
  bool    (*getDiffuseColorFunc)(const Plot3D_TriShape& p,
                                 FloatVector4& c, Fragment& z);
  FloatVector4  halfwayVector;          // V + L normalized
  float   vDotL;
  float   vDotH;
  std::vector< Vertex > vertexBuf;
  std::vector< Triangle > triangleBuf;
  NIFFile::NIFVertexTransform viewTransform;
  float   waterDepthScale;              // -1.0 / viewTransform.scale
  union MaterialParams                  // material properties
  {
    struct                              // shader material
    {
      float   alphaThreshold;           // scaled to 255.0
      std::uint32_t alphaBlendMode;
      float   unused1;
      float   unused2;
      FloatVector4  uvScaleAndOffset;
      FloatVector4  baseColor;
      FloatVector4  emissiveColor;
      FloatVector4  unused3;
    }
    s;
    struct                              // effect material
    {
      float   alphaThreshold;
      std::uint32_t alphaBlendMode;
      float   baseColorScale;
      float   lightingInfluence;
      FloatVector4  uvScaleAndOffset;
      FloatVector4  baseColor;
      FloatVector4  emissiveColor;
      FloatVector4  falloffParams;
    }
    e;
    struct                              // water material
    {
      float   normalScale;
      std::uint32_t unused;             // always set to 0xFFFFFFFF for water
      float   reflectivity;
      float   specularScale;
      FloatVector4  uvScaleAndOffset;
      FloatVector4  baseColor;
      FloatVector4  transparencyDepth0;
      FloatVector4  depthMult;
    }
    w;
    inline MaterialParams()
    {
    }
  };
  MaterialParams  mp;
  static FloatVector4 colorToSRGB(FloatVector4 c);
  size_t transformVertexData(const NIFFile::NIFVertexTransform& modelTransform);
  inline bool glowEnabled() const
  {
    return bool(flags & CE2Material::Flag_Glow);
  }
  inline bool alphaBlendingEnabled() const
  {
    return bool(flags & CE2Material::Flag_AlphaBlending);
  }
  inline void storeNormal(FloatVector4 n, const std::uint32_t *cPtr);
  // a = alpha * 255
  FloatVector4 alphaBlend(FloatVector4 c, float a, const Fragment& z) const;
  // c = albedo, e = environment, f0 = reflectance
  // r = roughness, s_i = smoothness * 255
  // returns sRGB color
  inline FloatVector4 calculateLighting_SF(
      FloatVector4 c, FloatVector4 e, FloatVector4 specular,
      float ao, FloatVector4 f0, const Fragment& z,
      float nDotL, float nDotV, float r, int s_i) const;
  // simplified functions for diffuse texture/normal map only (n = normal)
  inline FloatVector4 calculateLighting_SF(
      FloatVector4 c, const Fragment& z) const;
  // returns reflected view vector
  inline FloatVector4 calculateReflection(const Vertex& v) const;
  inline FloatVector4 environmentMap(
      FloatVector4 reflectedView, float smoothness) const;
  inline FloatVector4 specularGGX(
      FloatVector4 reflectedView, float roughness, float nDotL, float nDotV,
      const float *fresnelPoly, FloatVector4 f0) const;
  // c0 = terrain color (scaled to 1.0), a = water transparency
  inline FloatVector4 calculateLighting_Water(
      FloatVector4 c0, FloatVector4 a, Fragment& z) const;
  // fill with water
  static void drawPixel_Water(Plot3D_TriShape& p, Fragment& z);
  static void drawPixel_Effect(Plot3D_TriShape& p, Fragment& z);
  FloatVector4 glowMap(const Fragment& z) const;
  // returns true if the pixel is visible (alpha >= threshold)
  static bool getDiffuseColor_Effect(
      const Plot3D_TriShape& p, FloatVector4& c, Fragment& z);
  static bool getDiffuseColor_C(
      const Plot3D_TriShape& p, FloatVector4& c, Fragment& z);
  static bool getDiffuseColor_Water(    // for debug mode only
      const Plot3D_TriShape& p, FloatVector4& c, Fragment& z);
  inline bool getDiffuseColor(FloatVector4& c, Fragment& z) const
  {
    return getDiffuseColorFunc(*this, c, z);
  }
  static void drawPixel_Debug(Plot3D_TriShape& p, Fragment& z);
  // diffuse texture only
  static void drawPixel_D_SF(Plot3D_TriShape& p, Fragment& z);
  // diffuse + normal map only
  static void drawPixel_N_SF(Plot3D_TriShape& p, Fragment& z);
  static void drawPixel_SF(Plot3D_TriShape& p, Fragment& z);
  inline void drawPixel(int x, int y, Fragment& v,
                        const Vertex& v0, const Vertex& v1, const Vertex& v2,
                        float w0f, float w1f, float w2f);
  void drawLine(Fragment& v, const Vertex& v0, const Vertex& v1);
  void drawTriangles();
  void setMaterialProperties(const DDSTexture * const *textureSet,
                             unsigned int textureMask);
 public:
  // mode & 3 = 0: diffuse texture only
  //            1: enable normal mapping
  //            3: enable full quality
  Plot3D_TriShape(std::uint32_t *outBufRGBA, float *outBufZ,
                  int imageWidth, int imageHeight, unsigned int mode);
  virtual ~Plot3D_TriShape();
  // outBufN (optional for decal rendering) contains normals in a format
  // that can be decoded with FloatVector4::uint32ToNormal().
  inline void setBuffers(std::uint32_t *outBufRGBA, float *outBufZ,
                         int imageWidth, int imageHeight,
                         std::uint32_t *outBufN = (std::uint32_t *) 0)
  {
    bufZ = outBufZ;
    bufRGBA = outBufRGBA;
    width = imageWidth;
    height = imageHeight;
    bufN = outBufN;
  }
  inline void setRenderMode(unsigned int mode)
  {
    renderMode = (unsigned char) (mode & 3U);
  }
  void setViewAndLightVector(const NIFFile::NIFVertexTransform& vt,
                             float lightX, float lightY, float lightZ);
  inline void setEnvironmentMap(const DDSTexture *t)
  {
    textureE = t;
  }
  // Vector to be added to (image X, image Y, 0.0) for cube mapping.
  // Defaults to -(imageWidth / 2), -(imageHeight / 2), imageHeight.
  inline void setEnvMapOffset(float x, float y, float z)
  {
    envMapOffs = FloatVector4(x, y, z, 0.0f);
  }
  // deepColor is the base color in sRGB color space (0xBBGGRR)
  // for channel N, alpha =
  //     1.0 - ((1.0 - alphaDepth0[N]) * exp2(depthMult[N] * -depth))
  void setWaterProperties(
      std::uint32_t deepColor, FloatVector4 alphaDepth0, FloatVector4 depthMult,
      float uvScale = 0.032258f, float normalScale = 1.0f,
      float reflectivity = 0.02032076f, float specularScale = 1.0f);
  // c = light source (multiplies diffuse and specular, but not glow/cube maps)
  // a = ambient light (added to diffuse light level after multiplying with c)
  // e = environment map color and level, also multiplies ambient light
  // s = overall RGB scale to multiply the final color with (linear color space)
  // setLighting() needs to be called again if the game type is changed.
  void setLighting(FloatVector4 c, FloatVector4 a, FloatVector4 e, float s);
  Plot3D_TriShape& operator=(const NIFFile::NIFTriShape& t);
  // textureSet[0] = albedo (_color.dds)
  // textureSet[1] = normal (_normal.dds)
  // textureSet[2] = opacity map (_opacity.dds)
  // textureSet[3] = roughness map (_rough.dds)
  // textureSet[4] = metalness map (_metal.dds)
  // textureSet[5] = ambient occlusion map (_ao.dds)
  // textureSet[6] = height map (_height.dds)
  // textureSet[7] = glow map (_emissive.dds)
  // textureSet[8] = translucency map (_transmissive.dds)
  void drawTriShape(const NIFFile::NIFVertexTransform& modelTransform,
                    const DDSTexture * const *textureSet,
                    unsigned int textureMask);
  // n = 0: default mode
  // n = 1: render c as a solid color (0xRRGGBB)
  // n = 2: Z * 16 (blue = LSB), or Z * 64 if USE_PIXELFMT_RGB10A2 is enabled
  // n = 3: normal map
  // n = 4: disable lighting and reflections
  // n = 5: lighting only (white texture)
  inline void setDebugMode(unsigned int n, std::uint32_t c)
  {
    debugMode = (n <= 5U ? (n != 1U ? n : (0x80000000U | c)) : 0U);
  }
  // Calculate ambient light from the average color of a cube map.
  FloatVector4 cubeMapToAmbient(const DDSTexture *e) const;
  // Try to find the point of impact for a decal within the specified bounds.
  // Returns the (possibly incorrect) Y offset, or a large negative value that
  // is out of bounds if the search is not successful.
  float findDecalYOffset(const NIFFile::NIFVertexTransform& modelTransform,
                         const NIFFile::NIFBounds& b) const;
  // b = decal bounds (model space), X,Z are mapped to texture U,V, Y to depth
  // c = color (sRGB) in bits 0 to 23, flags in bits 24 to 31
  //     subtextures (selected with b30 and b31) are enabled if b27 is not set
  void drawDecal(const NIFFile::NIFVertexTransform& modelTransform,
                 const DDSTexture * const *textureSet, unsigned int textureMask,
                 const NIFFile::NIFBounds& b, std::uint32_t c = 0xFFFFFFFFU);
};

template< typename T, typename ColorType > class Plot3D : public T
{
 protected:
  struct Line
  {
    int     x;
    int     y;
    int     xEnd;
    int     yEnd;
    int     dx;
    int     dy;
    bool    verticalFlag;
    signed char xStep;
    signed char yStep;
    int     f;
    ColorType z;
    ColorType zStep;
    Line(int x0, int y0, ColorType z0, int x1, int y1, ColorType z1)
      : x(x0),
        y(y0),
        xEnd(x1),
        yEnd(y1),
        dx(x1 >= x0 ? (x1 - x0) : (x0 - x1)),
        dy(y1 >= y0 ? (y1 - y0) : (y0 - y1)),
        verticalFlag(dy > dx),
        xStep((signed char) (x1 > x0 ? 1 : (x1 < x0 ? -1 : 0))),
        yStep((signed char) (y1 > y0 ? 1 : (y1 < y0 ? -1 : 0))),
        f(verticalFlag ? dy : dx),
        z(z0),
        zStep(z1)
    {
      zStep -= z0;
      if (f)
        zStep *= (1.0f / float(f));
      f = f >> 1;
    }
    inline bool step()
    {
      z += zStep;
      if (verticalFlag)
      {
        f += dx;
        if (f >= dy)
        {
          f -= dy;
          x += int(xStep);
        }
        y += int(yStep);
        return (y != yEnd);
      }
      f += dy;
      if (f >= dx)
      {
        f -= dx;
        y += int(yStep);
      }
      x += int(xStep);
      return (x != xEnd);
    }
  };
  static inline void vertexSwap(int& x0, int& y0, ColorType& z0,
                                int& x1, int& y1, ColorType& z1)
  {
    {
      int     tmp = x0;
      x0 = x1;
      x1 = tmp;
      tmp = y0;
      y0 = y1;
      y1 = tmp;
    }
    ColorType tmp = z0;
    z0 = z1;
    z1 = tmp;
  }
 public:
#if 0
  // implemented in T
  inline void drawPixel(int x, int y, ColorType z);
#endif
  void drawLineH(int x0, int y0, ColorType z0, int x1, ColorType z1)
  {
    T::drawPixel(x0, y0, z0);
    if (x0 == x1)
    {
      T::drawPixel(x1, y0, z1);
      return;
    }
    int       xStep = (x1 > x0 ? 1 : -1);
    float     d = float(xStep) / float(x1 - x0);
    ColorType zStep(z1);
    zStep -= z0;
    zStep *= d;
    do
    {
      x0 += xStep;
      z0 += zStep;
      T::drawPixel(x0, y0, z0);
    }
    while (x0 != x1);
  }
  void drawLineV(int x0, int y0, ColorType z0, int y1, ColorType z1)
  {
    T::drawPixel(x0, y0, z0);
    if (y0 == y1)
    {
      T::drawPixel(x0, y1, z1);
      return;
    }
    int       yStep = (y1 > y0 ? 1 : -1);
    float     d = float(yStep) / float(y1 - y0);
    ColorType zStep(z1);
    zStep -= z0;
    zStep *= d;
    do
    {
      y0 += yStep;
      z0 += zStep;
      T::drawPixel(x0, y0, z0);
    }
    while (y0 != y1);
  }
  void drawLine(int x0, int y0, ColorType z0, int x1, int y1, ColorType z1)
  {
    if (y0 == y1)
    {
      drawLineH(x0, y0, z0, x1, z1);
      return;
    }
    if (x0 == x1)
    {
      drawLineV(x0, y0, z0, y1, z1);
      return;
    }
    Line    l(x0, y0, z0, x1, y1, z1);
    do
    {
      T::drawPixel(l.x, l.y, l.z);
    }
    while (l.step());
    T::drawPixel(x1, y1, z1);
  }
  void drawTriangle(int x0, int y0, ColorType z0,
                    int x1, int y1, ColorType z1,
                    int x2, int y2, ColorType z2)
  {
    T::drawPixel(x0, y0, z0);
    if ((std::abs(x1 - x0) | std::abs(x2 - x0) | std::abs(x2 - x1)
         | std::abs(y1 - y0) | std::abs(y2 - y0) | std::abs(y2 - y1)) <= 1)
    {
      T::drawPixel(x1, y1, z1);
      T::drawPixel(x2, y2, z2);
      return;
    }
    // sort vertices by Y coordinate
    if (y0 > y1)
      vertexSwap(x0, y0, z0, x1, y1, z1);
    if (y1 > y2)
    {
      if (y0 > y2)
        vertexSwap(x0, y0, z0, x2, y2, z2);
      vertexSwap(x1, y1, z1, x2, y2, z2);
    }
    Line    l01(x0, y0, z0, x1, y1, z1);
    Line    l02(x0, y0, z0, x2, y2, z2);
    Line    l12(x1, y1, z1, x2, y2, z2);
    bool    continueFlag01 = (y1 > y0 || x1 != x0);
    bool    continueFlag02 = (y2 > y0 || x2 != x0);
    bool    continueFlag12 = (y2 > y1 || x2 != x1);
    do
    {
      if (continueFlag01 && l01.y == l02.y)
        drawLineH(l01.x, l01.y, l01.z, l02.x, l02.z);
      if (continueFlag12 && l12.y == l02.y)
        drawLineH(l12.x, l12.y, l12.z, l02.x, l02.z);
      int     prvY = l02.y;
      while (continueFlag02 && l02.y <= prvY)
      {
        continueFlag02 = l02.step();
        T::drawPixel(l02.x, l02.y, l02.z);
      }
      while (continueFlag01 && l01.y <= prvY)
      {
        continueFlag01 = l01.step();
        T::drawPixel(l01.x, l01.y, l01.z);
      }
      while (continueFlag12 && l12.y <= prvY)
      {
        continueFlag12 = l12.step();
        T::drawPixel(l12.x, l12.y, l12.z);
      }
    }
    while (continueFlag01 | continueFlag02 | continueFlag12);
  }
  void drawRectangle(int x0, int y0, int x1, int y1, ColorType z)
  {
    if (x0 > x1)
    {
      int     tmp = x0;
      x0 = x1;
      x1 = tmp;
    }
    if (y0 > y1)
    {
      int     tmp = y0;
      y0 = y1;
      y1 = tmp;
    }
    do
    {
      int     x = x0;
      do
      {
        T::drawPixel(x, y0, z);
      }
      while (++x <= x1);
    }
    while (++y0 <= y1);
  }
};

struct ColorV2
{
  // Z, light
  float   v0;
  float   v1;
  inline ColorV2& operator+=(const ColorV2& d)
  {
    v0 += d.v0;
    v1 += d.v1;
    return (*this);
  }
  inline ColorV2& operator-=(const ColorV2& d)
  {
    v0 -= d.v0;
    v1 -= d.v1;
    return (*this);
  }
  inline ColorV2& operator*=(float d)
  {
    v0 *= d;
    v1 *= d;
    return (*this);
  }
};

struct ColorV4 : public ColorV2
{
  // U, V
  float   v2;
  float   v3;
  inline ColorV4& operator+=(const ColorV4& d)
  {
    v0 += d.v0;
    v1 += d.v1;
    v2 += d.v2;
    v3 += d.v3;
    return (*this);
  }
  inline ColorV4& operator-=(const ColorV4& d)
  {
    v0 -= d.v0;
    v1 -= d.v1;
    v2 -= d.v2;
    v3 -= d.v3;
    return (*this);
  }
  inline ColorV4& operator*=(float d)
  {
    v0 *= d;
    v1 *= d;
    v2 *= d;
    v3 *= d;
    return (*this);
  }
};

#endif

