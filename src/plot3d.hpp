
#ifndef PLOT3D_HPP_INCLUDED
#define PLOT3D_HPP_INCLUDED

#include "common.hpp"
#include "ddstxt.hpp"
#include "nif_file.hpp"

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
 public:
  static inline unsigned int multiplyWithLight(unsigned int c, int lightLevel)
  {
    unsigned long long  tmp =
        (unsigned long long) ((c & 0x000000FFU) | ((c & 0x0000FF00U) << 12))
        | ((unsigned long long) (c & 0x00FF0000U) << 24);
    tmp = (tmp * (unsigned int) lightLevel) + 0x0000800008000080ULL;
    tmp = tmp | (((tmp >> 10) & 0x0001C0001C0001C0ULL) * 0x03FFU);
    return ((unsigned int) ((tmp >> 8) & 0x000000FFU)
            | (unsigned int) ((tmp >> 20) & 0x0000FF00U)
            | (unsigned int) ((tmp >> 32) & 0x00FF0000U) | 0xFF000000U);
  }
 protected:
  static const float  defaultLightingPolynomial[6];
  unsigned int  *bufRGBW;
  float   *bufZ;
  int     width;
  int     height;
  const DDSTexture  *textureD;
  const DDSTexture  *textureG;
  const DDSTexture  *textureN;
  const DDSTexture  *textureE;
  const DDSTexture  *textureR;
  float   textureScaleN;
  float   textureScaleR;
  float   mipLevel;
  float   light_X;
  float   light_Y;
  float   light_Z;
  int     reflectionLevel;
  float   envMapUVScale;
  void    (*drawPixelFunction)(Plot3D_TriShape& p,
                               int x, int y, float txtU, float txtV,
                               const NIFFile::NIFVertex& z);
  std::vector< NIFFile::NIFVertex > vertexBuf;
  std::vector< unsigned int > triangleBuf;
  float   lightingPolynomial[6];
  std::vector< unsigned short > vclrTable;
  void calculateWaterUV(const NIFFile::NIFVertexTransform& modelTransform);
  void sortTriangles(size_t n0, size_t n2);
  size_t transformVertexData(const NIFFile::NIFVertexTransform& modelTransform,
                             const NIFFile::NIFVertexTransform& viewTransform,
                             float& lightX, float& lightY, float& lightZ);
  unsigned int gradientMapAndVColor(unsigned int c, unsigned int vColor) const;
  // returns light level
  inline int normalMap(float& normalX, float& normalY, float& normalZ,
                       unsigned int n) const;
  inline unsigned int environmentMap(
      unsigned int c, float normalX, float normalY, float normalZ,
      int x, int y) const;
  inline unsigned int reflectionMap(
      unsigned int c, float normalX, float normalY, float normalZ,
      int x, int y, float txtU, float txtV) const;
  // fill with water
  static void drawPixel_Water(Plot3D_TriShape& p,
                              int x, int y, float txtU, float txtV,
                              const NIFFile::NIFVertex& z);
  // diffuse texture with trilinear filtering
  static void drawPixel_Diffuse(Plot3D_TriShape& p,
                                int x, int y, float txtU, float txtV,
                                const NIFFile::NIFVertex& z);
  // diffuse + normal map with trilinear filtering
  static void drawPixel_Normal(Plot3D_TriShape& p,
                               int x, int y, float txtU, float txtV,
                               const NIFFile::NIFVertex& z);
  // diffuse + normal and environment map with trilinear filtering
  static void drawPixel_NormalEnv(Plot3D_TriShape& p,
                                  int x, int y, float txtU, float txtV,
                                  const NIFFile::NIFVertex& z);
  // diffuse + normal and reflection map with trilinear filtering
  static void drawPixel_NormalRefl(Plot3D_TriShape& p,
                                   int x, int y, float txtU, float txtV,
                                   const NIFFile::NIFVertex& z);
  static unsigned int interpVertexColors(
      const NIFFile::NIFVertex& v0, const NIFFile::NIFVertex& v1,
      const NIFFile::NIFVertex& v2, float w0, float w1, float w2);
  inline void drawPixel(
      int x, int y, float z, float txtU, float txtV,
      NIFFile::NIFVertex& v, const NIFFile::NIFVertex& v0,
      const NIFFile::NIFVertex& v1, const NIFFile::NIFVertex& v2,
      float w0, float w1, float w2);
  void drawLine(const NIFFile::NIFVertex *v0, const NIFFile::NIFVertex *v1);
  void drawTriangle(const NIFFile::NIFVertex *v0, const NIFFile::NIFVertex *v1,
                    const NIFFile::NIFVertex *v2);
 public:
  // The alpha channel is 255 for solid geometry, 0 for air, and 1 to 254
  // for water. In the case of water, the pixel format changes to R5G6B5X8Y8,
  // from LSB to MSB, representing the color of the terrain under the water,
  // and the X and Y normals of the water surface (mapped to the range of
  // 2 to 254).
  Plot3D_TriShape(unsigned int *outBufRGBW, float *outBufZ,
                  int imageWidth, int imageHeight);
  virtual ~Plot3D_TriShape();
  Plot3D_TriShape& operator=(const NIFFile::NIFTriShape& t);
  // set polynomial a[0..5] for mapping dot product (-1.0 to 1.0)
  // to RGB multiplier
  void setLightingFunction(const float *a);
  void getLightingFunction(float *a) const;
  static void getDefaultLightingFunction(float *a);
  // calculate RGB multiplier from normals and light direction
  static float calculateLighting(float normalX, float normalY, float normalZ,
                                 float lightX, float lightY, float lightZ,
                                 const float *a);
  // textures[0] = diffuse
  // textures[1] = normal
  // textures[4] = environment map
  // textures[8] = Fallout 76 reflection map
  void drawTriShape(const NIFFile::NIFVertexTransform& modelTransform,
                    const NIFFile::NIFVertexTransform& viewTransform,
                    float lightX, float lightY, float lightZ,
                    const DDSTexture * const *textures, size_t textureCnt);
  // convert all water surfaces after draw operations are completed
  void renderWater(const NIFFile::NIFVertexTransform& viewTransform,
                   unsigned int waterColor,     // 0xAABBGGRR format
                   float lightX, float lightY, float lightZ,
                   const DDSTexture *envMap = (DDSTexture *) 0,
                   float envMapLevel = 1.0f);
};

static inline unsigned int downsample2xFilter(const unsigned int *buf,
                                              int imageWidth, int imageHeight,
                                              int x, int y)
{
  static const int  filterTable[105] =
  {
    // Y offset, (X offset, value) * 7
    -5, -5,   1, -3,  -3, -1,  10,  0,  16,  1,  10,  3,  -3,  5,   1,
    -3, -5,  -3, -3,   9, -1, -30,  0, -48,  1, -30,  3,   9,  5,  -3,
    -1, -5,  10, -3, -30, -1, 100,  0, 160,  1, 100,  3, -30,  5,  10,
     0, -5,  16, -3, -48, -1, 160,  0, 256,  1, 160,  3, -48,  5,  16,
     1, -5,  10, -3, -30, -1, 100,  0, 160,  1, 100,  3, -30,  5,  10,
     3, -5,  -3, -3,   9, -1, -30,  0, -48,  1, -30,  3,   9,  5,  -3,
     5, -5,   1, -3,  -3, -1,  10,  0,  16,  1,  10,  3,  -3,  5,   1
  };
  const int   *p = filterTable;
  int     r = 0;
  int     g = 0;
  int     b = 0;
  for (int i = 0; i < 7; i++)
  {
    int     yy = y + p[0];
    p++;
    yy = (yy > 0 ? (yy < (imageHeight - 1) ? yy : (imageHeight - 1)) : 0);
    const unsigned int  *bufp = buf + (size_t(yy) * size_t(imageWidth));
    if (BRANCH_EXPECT(x < 5 || x >= (imageWidth - 5), false))
    {
      for (int j = 0; j < 7; j++, p = p + 2)
      {
        int     xx = x + p[0];
        int     m = p[1];
        xx = (xx > 0 ? (xx < (imageWidth - 1) ? xx : (imageWidth - 1)) : 0);
        unsigned int  c = bufp[xx];
        r = r + (int(c & 0xFF) * m);
        g = g + (int((c >> 8) & 0xFF) * m);
        b = b + (int((c >> 16) & 0xFF) * m);
      }
    }
    else
    {
      bufp = bufp + x;
      for (int j = 0; j < 7; j++, p = p + 2)
      {
        unsigned int  c = *(bufp + p[0]);
        int     m = p[1];
        r = r + (int(c & 0xFF) * m);
        g = g + (int((c >> 8) & 0xFF) * m);
        b = b + (int((c >> 16) & 0xFF) * m);
      }
    }
  }
  unsigned int  c = buf[size_t(y) * size_t(imageWidth) + size_t(x)];
  r = (r > 0 ? (r < 0x0003FC00 ? r : 0x0003FC00) : 0) + 512;
  g = ((g > 0 ? (g < 0x0003FC00 ? g : 0x0003FC00) : 0) + 512) & 0x0003FC00;
  b = ((b > 0 ? (b < 0x0003FC00 ? b : 0x0003FC00) : 0) + 512) & 0x0003FC00;
  return ((c & 0xFF000000U) | (unsigned int) ((r >> 10) | (g >> 2) | (b << 6)));
}

#endif

