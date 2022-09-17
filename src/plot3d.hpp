
#ifndef PLOT3D_HPP_INCLUDED
#define PLOT3D_HPP_INCLUDED

#include "common.hpp"
#include "ddstxt.hpp"
#include "nif_file.hpp"
#include "fp32vec4.hpp"

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

class Plot3D_TriShape : public NIFFile::NIFTriShape
{
 protected:
  static const float  fresnelRoughTable[1024];
  struct Vertex
  {
    FloatVector4  xyz;
    FloatVector4  bitangent;            // bitangent[3] = texture U
    FloatVector4  tangent;              // tangent[3] = texture V
    FloatVector4  normal;
    FloatVector4  vertexColor;
    Vertex()
      : xyz(0.0f),
        bitangent(0.0f),
        tangent(0.0f),
        normal(0.0f),
        vertexColor(1.0f)
    {
    }
    Vertex(const NIFFile::NIFVertex& r)
      : xyz(r.x, r.y, r.z, 0.0f),
        bitangent(&(r.bitangent)), tangent(&(r.tangent)), normal(&(r.normal)),
        vertexColor(&(r.vertexColor))
    {
      bitangent *= (1.0f / 127.5f);
      tangent *= (1.0f / 127.5f);
      normal *= (1.0f / 127.5f);
      bitangent -= 1.0f;
      tangent -= 1.0f;
      normal -= 1.0f;
      r.getUV(bitangent[3], tangent[3]);
      normal[3] = 0.0f;
      vertexColor *= (1.0f / 255.0f);
    }
    inline const float& u() const
    {
      return bitangent[3];
    }
    inline const float& v() const
    {
      return tangent[3];
    }
  };
  struct Triangle
  {
    float   z;                  // Z coordinate for sorting
    unsigned int  n;            // NIFFile::NIFTriShape::triangleData index
    inline bool operator<(const Triangle& r) const
    {
      return (z < r.z);
    }
    static inline bool gt(const Triangle& r1, const Triangle& r2)
    {
      return (r1.z > r2.z);
    }
  };
  std::uint32_t *bufRGBA;
  float   *bufZ;
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
  float   mipLevel;
  float   alphaThresholdFloat;
  float   reflectionLevel;
  bool    invNormals;
  unsigned char renderMode;
  bool    usingSRGBColorSpace;
  FloatVector4  lightVector;
  FloatVector4  lightColor;             // lightColor[3] = overall RGB scale
  FloatVector4  ambientLight;
  FloatVector4  envColor;
  FloatVector4  envMapOffs;
  FloatVector4  viewTransformInvX;
  FloatVector4  viewTransformInvY;
  FloatVector4  viewTransformInvZ;
  FloatVector4  specularColorFloat;     // water color for drawWater()
  float   viewScale;
  unsigned int  debugMode;
  // offs = frame buffer offset (y * width + x)
  void    (*drawPixelFunction)(Plot3D_TriShape& p, size_t offs, Vertex& z);
  std::vector< Vertex > vertexBuf;
  std::vector< Triangle > triangleBuf;
  static FloatVector4 colorToSRGB(FloatVector4 c);
  size_t transformVertexData(const NIFFile::NIFVertexTransform& modelTransform,
                             const NIFFile::NIFVertexTransform& viewTransform);
  // a = alpha * 255
  FloatVector4 alphaBlend(FloatVector4 c, float a, const Vertex& z) const;
  // c = albedo, e = environment, f0 = reflectance
  // r = roughness, s_i = smoothness * 255
  // returns sRGB color
  inline FloatVector4 calculateLighting_FO76(
      FloatVector4 c, FloatVector4 e, float specular, float ao, FloatVector4 f0,
      const Vertex& z, float nDotL, float nDotV, float r, int s_i) const;
  inline FloatVector4 calculateLighting_FO4(
      FloatVector4 c, FloatVector4 e, float specular, float specLevel,
      const Vertex& z, float nDotL, float nDotV, float smoothness) const;
  inline FloatVector4 calculateLighting_TES5(
      FloatVector4 c, FloatVector4 e, float specular, float envLevel,
      float specLevel, const Vertex& z, float nDotL, float nDotV) const;
  // simplified functions for diffuse texture/normal map only (n = normal)
  inline FloatVector4 calculateLighting_FO76(
      FloatVector4 c, const Vertex& z) const;
  inline FloatVector4 calculateLighting_FO4(
      FloatVector4 c, const Vertex& z) const;
  inline FloatVector4 calculateLighting_TES5(
      FloatVector4 c, const Vertex& z) const;
  // returns normal
  inline FloatVector4 normalMap(Vertex& v, FloatVector4 n) const;
  // returns reflected view vector
  inline FloatVector4 calculateReflection(const Vertex& v) const;
  inline FloatVector4 environmentMap(
      FloatVector4 reflectedView, float smoothness,
      bool isSRGB, bool invZ) const;
  inline float specularPhong(FloatVector4 reflectedView, float smoothness,
                             float nDotL, bool isNormalized = false) const;
  inline float specularGGX(FloatVector4 reflectedView, float roughness,
                           float nDotL) const;
  // c0 = terrain color, a = water transparency
  inline FloatVector4 calculateLighting_Water(
      FloatVector4 c0, FloatVector4 n, FloatVector4 reflectedView, float a,
      bool isFO76) const;
  // fill with water (first and second pass)
  static void drawPixel_Water(Plot3D_TriShape& p, size_t offs, Vertex& z);
  static void drawPixel_Water_2(Plot3D_TriShape& p, size_t offs, Vertex& z);
  // with GGX
  static void drawPixel_Water_2G(Plot3D_TriShape& p, size_t offs, Vertex& z);
  // alphaFlag is set to true if the pixel is visible (alpha >= threshold)
  inline FloatVector4 getDiffuseColor(
      float txtU, float txtV, const Vertex& z, bool& alphaFlag,
      bool isFO76) const;
  static void drawPixel_Debug(Plot3D_TriShape& p, size_t offs, Vertex& z);
  // diffuse texture only
  static void drawPixel_D_TES5(Plot3D_TriShape& p, size_t offs, Vertex& z);
  // diffuse + normal map only
  static void drawPixel_N_TES5(Plot3D_TriShape& p, size_t offs, Vertex& z);
  static void drawPixel_TES5(Plot3D_TriShape& p, size_t offs, Vertex& z);
  static void drawPixel_D_FO4(Plot3D_TriShape& p, size_t offs, Vertex& z);
  static void drawPixel_N_FO4(Plot3D_TriShape& p, size_t offs, Vertex& z);
  static void drawPixel_FO4(Plot3D_TriShape& p, size_t offs, Vertex& z);
  static void drawPixel_D_FO76(Plot3D_TriShape& p, size_t offs, Vertex& z);
  static void drawPixel_N_FO76(Plot3D_TriShape& p, size_t offs, Vertex& z);
  static void drawPixel_FO76(Plot3D_TriShape& p, size_t offs, Vertex& z);
  inline void drawPixel(int x, int y, Vertex& v,
                        const Vertex& v0, const Vertex& v1, const Vertex& v2,
                        float w0f, float w1f, float w2f);
  void drawLine(Vertex& v, const Vertex& v0, const Vertex& v1);
  void drawTriangles();
 public:
  // alpha = 255 - sqrt(waterDepth*16) after first pass water rendering.
  // mode =  4: Skyrim
  // mode =  8: Fallout 4
  // mode = 12: Fallout 76
  //       | 1: enable normal mapping
  //       | 3: enable full quality
  Plot3D_TriShape(std::uint32_t *outBufRGBA, float *outBufZ,
                  int imageWidth, int imageHeight, unsigned int mode);
  virtual ~Plot3D_TriShape();
  inline void setBuffers(std::uint32_t *outBufRGBA, float *outBufZ,
                         int imageWidth, int imageHeight)
  {
    bufRGBA = outBufRGBA;
    bufZ = outBufZ;
    width = imageWidth;
    height = imageHeight;
  }
  inline void setRenderMode(unsigned int mode)
  {
    renderMode = (unsigned char) (mode & 15U);
    usingSRGBColorSpace = (renderMode < 12);
  }
  // Vector to be added to (image X, image Y, 0.0) for cube mapping.
  // Defaults to -(imageWidth / 2), -(imageHeight / 2), imageHeight.
  inline void setEnvMapOffset(float x, float y, float z)
  {
    envMapOffs = FloatVector4(x, y, z, 0.0f);
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
                    const NIFFile::NIFVertexTransform& viewTransform,
                    float lightX, float lightY, float lightZ,
                    const DDSTexture * const *textures,
                    unsigned int textureMask);
  // drawWater() should be called to render water meshes in a second pass.
  // waterColor is in 0xAABBGGRR format, the alpha channel contains the depth
  // beyond which the water has maximum opacity, as alpha = 256 - sqrt(depth*8).
  void drawWater(const NIFFile::NIFVertexTransform& modelTransform,
                 const NIFFile::NIFVertexTransform& viewTransform,
                 float lightX, float lightY, float lightZ,
                 const DDSTexture * const *textures, unsigned int textureMask,
                 std::uint32_t waterColor, float envMapLevel = 1.0f);
  // n = 0: default mode
  // n = 1: render c as a solid color (0x00RRGGBB)
  // n = 2: Z * 16 (blue = LSB, red = MSB)
  // n = 3: normal map
  // n = 4: disable lighting and reflections
  // n = 5: lighting only (white texture)
  void setDebugMode(unsigned int n, std::uint32_t c)
  {
    debugMode = (n <= 5U ? (n != 1U ? n : (0xFF000000U | c)) : 0U);
  }
  // Calculate ambient light from the average color of a cube map.
  FloatVector4 cubeMapToAmbient(const DDSTexture *e) const;
};

#endif

