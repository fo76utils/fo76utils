
#ifndef PLOT3D_HPP_INCLUDED
#define PLOT3D_HPP_INCLUDED

#include "common.hpp"
#include "ddstxt.hpp"
#include "nif_file.hpp"
#include "fp32vec4.hpp"

class Plot3D_TriShape : public NIFFile::NIFTriShape
{
 protected:
  struct Vertex
  {
    FloatVector4  xyz;
    FloatVector4  bitangent;            // bitangent[3] = texture U
    FloatVector4  tangent;              // tangent[3] = texture V
    FloatVector4  normal;
    FloatVector4  vertexColor;
    inline const float& u() const
    {
      return bitangent[3];
    }
    inline const float& v() const
    {
      return tangent[3];
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
  int     width;
  int     height;
  const DDSTexture  *textureD;          // diffuse texture
  const DDSTexture  *textureG;          // gradient (grayscale to palette) map
  const DDSTexture  *textureN;          // normal map
  const DDSTexture  *textureE;          // environment map
  const DDSTexture  *textureS;          // TES5 _em.dds, FO4 _s.dds, FO76 _l.dds
  const DDSTexture  *textureR;          // Fallout 76 reflection map
  const DDSTexture  *textureGlow;       // glow map
  float   mipOffsetN;
  float   mipOffsetS;
  float   mipOffsetR;
  float   mipOffsetG;                   // for the glow map
  unsigned char renderMode;
  bool    usingSRGBColorSpace;
  std::uint16_t waterUVScale;
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
  std::uint32_t *bufN;                  // normals for decal rendering
  std::vector< Vertex > vertexBuf;
  std::vector< Triangle > triangleBuf;
  NIFFile::NIFVertexTransform viewTransform;
  static FloatVector4 colorToSRGB(FloatVector4 c);
  size_t transformVertexData(const NIFFile::NIFVertexTransform& modelTransform);
  inline bool glowEnabled() const
  {
    return bool(m.flags & BGSMFile::Flag_Glow);
  }
  inline bool alphaBlendingEnabled() const
  {
    return bool(m.flags & BGSMFile::Flag_TSAlphaBlending);
  }
  inline void storeNormal(FloatVector4 n, const std::uint32_t *cPtr);
  // a = alpha * 255
  FloatVector4 alphaBlend(FloatVector4 c, float a, const Fragment& z) const;
  // c = albedo, e = environment, f0 = reflectance
  // r = roughness, s_i = smoothness * 255
  // returns sRGB color
  inline FloatVector4 calculateLighting_FO76(
      FloatVector4 c, FloatVector4 e, FloatVector4 specular,
      float ao, FloatVector4 f0, const Fragment& z,
      float nDotL, float nDotV, float r, int s_i) const;
  inline FloatVector4 calculateLighting_FO4(
      FloatVector4 c, FloatVector4 e, float specular, float specLevel,
      const Fragment& z, float nDotL, float nDotV, float smoothness) const;
  inline FloatVector4 calculateLighting_TES5(
      FloatVector4 c, FloatVector4 e, float specular, float envLevel,
      float specLevel, const Fragment& z, float nDotL, float nDotV) const;
  // simplified functions for diffuse texture/normal map only (n = normal)
  inline FloatVector4 calculateLighting_FO76(
      FloatVector4 c, const Fragment& z) const;
  inline FloatVector4 calculateLighting_FO4(
      FloatVector4 c, const Fragment& z) const;
  inline FloatVector4 calculateLighting_TES5(
      FloatVector4 c, const Fragment& z) const;
  // returns reflected view vector
  inline FloatVector4 calculateReflection(const Vertex& v) const;
  inline FloatVector4 environmentMap(
      FloatVector4 reflectedView, float smoothness,
      bool isSRGB, bool invZ) const;
  // fresnelPoly != NULL also enables normalization
  inline float specularPhong(
      FloatVector4 reflectedView, float smoothness, float nDotL, float nDotV,
      const float *fresnelPoly = nullptr) const;
  inline FloatVector4 specularGGX(
      FloatVector4 reflectedView, float roughness, float nDotL, float nDotV,
      const float *fresnelPoly, FloatVector4 f0) const;
  // c0 = terrain color (scaled to 1.0), a = water transparency
  inline FloatVector4 calculateLighting_Water(
      FloatVector4 c0, float a, Fragment& z, bool isFO76) const;
  // fill with water
  static void drawPixel_Water(Plot3D_TriShape& p, Fragment& z);
  static void drawPixel_Effect(Plot3D_TriShape& p, Fragment& z);
  // for Fallout 76 with GGX specular
  static void drawPixel_Water_G(Plot3D_TriShape& p, Fragment& z);
  static void drawPixel_Effect_G(Plot3D_TriShape& p, Fragment& z);
  FloatVector4 glowMap(const Fragment& z) const;
  // returns true if the pixel is visible (alpha >= threshold)
  static bool getDiffuseColor_Effect(
      const Plot3D_TriShape& p, FloatVector4& c, Fragment& z);
  static bool getDiffuseColor_sRGB_G(
      const Plot3D_TriShape& p, FloatVector4& c, Fragment& z);
  static bool getDiffuseColor_sRGB(
      const Plot3D_TriShape& p, FloatVector4& c, Fragment& z);
  static bool getDiffuseColor_Linear_G(
      const Plot3D_TriShape& p, FloatVector4& c, Fragment& z);
  static bool getDiffuseColor_Linear(
      const Plot3D_TriShape& p, FloatVector4& c, Fragment& z);
  inline bool getDiffuseColor(FloatVector4& c, Fragment& z) const
  {
    return getDiffuseColorFunc(*this, c, z);
  }
  static void drawPixel_Debug(Plot3D_TriShape& p, Fragment& z);
  // diffuse texture only
  static void drawPixel_D_TES5(Plot3D_TriShape& p, Fragment& z);
  // diffuse + normal map only
  static void drawPixel_N_TES5(Plot3D_TriShape& p, Fragment& z);
  static void drawPixel_TES5(Plot3D_TriShape& p, Fragment& z);
  static void drawPixel_D_FO4(Plot3D_TriShape& p, Fragment& z);
  static void drawPixel_N_FO4(Plot3D_TriShape& p, Fragment& z);
  static void drawPixel_FO4(Plot3D_TriShape& p, Fragment& z);
  static void drawPixel_D_FO76(Plot3D_TriShape& p, Fragment& z);
  static void drawPixel_N_FO76(Plot3D_TriShape& p, Fragment& z);
  static void drawPixel_FO76(Plot3D_TriShape& p, Fragment& z);
  inline void drawPixel(int x, int y, Fragment& v,
                        const Vertex& v0, const Vertex& v1, const Vertex& v2,
                        float w0f, float w1f, float w2f);
  void drawLine(Fragment& v, const Vertex& v0, const Vertex& v1);
  void drawTriangles();
 public:
  // mode =  4: Skyrim
  // mode =  8: Fallout 4
  // mode = 12: Fallout 76
  //       | 1: enable normal mapping
  //       | 3: enable full quality
  Plot3D_TriShape(std::uint32_t *outBufRGBA, float *outBufZ,
                  int imageWidth, int imageHeight, unsigned int mode);
  virtual ~Plot3D_TriShape();
  // outBufN (optional for decal rendering) contains normals in a format
  // that can be decoded with FloatVector4::uint32ToNormal().
  inline void setBuffers(std::uint32_t *outBufRGBA, float *outBufZ,
                         int imageWidth, int imageHeight,
                         std::uint32_t *outBufN = nullptr)
  {
    bufZ = outBufZ;
    bufRGBA = outBufRGBA;
    width = imageWidth;
    height = imageHeight;
    bufN = outBufN;
  }
  inline void setRenderMode(unsigned int mode)
  {
    renderMode = (unsigned char) (mode & 15U);
    usingSRGBColorSpace = (renderMode < 12);
  }
  void setViewAndLightVector(const NIFFile::NIFVertexTransform& vt,
                             float lightX, float lightY, float lightZ);
  // Vector to be added to (image X, image Y, 0.0) for cube mapping.
  // Defaults to -(imageWidth / 2), -(imageHeight / 2), imageHeight.
  inline void setEnvMapOffset(float x, float y, float z)
  {
    envMapOffs = FloatVector4(x, y, z, 0.0f);
  }
  // water U, V = world X, Y * s (default: 1.0 / 2048.0)
  inline void setWaterUVScale(float s)
  {
    waterUVScale = std::uint16_t(roundFloat(s * 65536.0f));
  }
  // c = light source (multiplies diffuse and specular, but not glow/cube maps)
  // a = ambient light (added to diffuse light level after multiplying with c)
  // e = environment map color and level, also multiplies ambient light
  // s = overall RGB scale to multiply the final color with (linear color space)
  // setLighting() needs to be called again if the game type is changed.
  void setLighting(FloatVector4 c, FloatVector4 a, FloatVector4 e, float s);
  Plot3D_TriShape& operator=(const NIFFile::NIFTriShape& t);
  // textures[0] = diffuse
  // textures[1] = normal
  // textures[2] = glow map
  // textures[3] = gradient map
  // textures[4] = environment map
  // textures[5] = Skyrim environment mask
  // textures[6] = Fallout 4 specular map
  // textures[7] = wrinkles (unused)
  // textures[8] = Fallout 76 reflection map
  // textures[9] = Fallout 76 lighting map
  void drawTriShape(const NIFFile::NIFVertexTransform& modelTransform,
                    const DDSTexture * const *textures,
                    unsigned int textureMask);
 protected:
  void drawEffect(const DDSTexture * const *textures, unsigned int textureMask);
  void drawWater(const DDSTexture * const *textures, unsigned int textureMask,
                 float viewScale);
 public:
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
                 const DDSTexture * const *textures, unsigned int textureMask,
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

