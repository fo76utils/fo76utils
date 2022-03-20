
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
    if (x0 == x1 && x0 == x2 && y0 == y1 && y0 == y2)
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

struct ColorZUVL
{
  float   z;
  float   u;
  float   v;
  float   l;
  inline ColorZUVL& operator+=(const ColorZUVL& d)
  {
    z += d.z;
    u += d.u;
    v += d.v;
    l += d.l;
    return (*this);
  }
  inline ColorZUVL& operator-=(const ColorZUVL& d)
  {
    z -= d.z;
    u -= d.u;
    v -= d.v;
    l -= d.l;
    return (*this);
  }
  inline ColorZUVL& operator*=(float d)
  {
    z *= d;
    u *= d;
    v *= d;
    l *= d;
    return (*this);
  }
};

class Plot3D_TriShape : public NIFFile::NIFTriShape
{
 private:
  class Plot3DTS_Base
  {
   public:
    unsigned int  *outBufRGBW;
    float   *outBufZ;
    int     width;
    int     height;
    float   mipLevel;
    float   normalX;
    float   normalY;
    float   normalZ;
    const DDSTexture  *textureD;
    const DDSTexture  *textureN;
    static inline unsigned int multiplyWithLight(unsigned int c,
                                                 float lightLevel);
  };
  class Plot3DTS_Water : public Plot3DTS_Base
  {
   public:
    // fill with water
    inline void drawPixel(int x, int y, const ColorZUVL& z);
  };
  class Plot3DTS_TextureN : public Plot3DTS_Base
  {
   public:
    // fill with solid color (1x1 texture)
    inline void drawPixel(int x, int y, const ColorZUVL& z);
  };
  class Plot3DTS_TextureB : public Plot3DTS_Base
  {
   public:
    // diffuse texture with bilinear filtering
    inline void drawPixel(int x, int y, const ColorZUVL& z);
  };
  class Plot3DTS_TextureT : public Plot3DTS_Base
  {
   public:
    // diffuse texture with trilinear filtering
    inline void drawPixel(int x, int y, const ColorZUVL& z);
  };
  class Plot3DTS_NormalsT : public Plot3DTS_Base
  {
   public:
    // diffuse + normal map with trilinear filtering
    inline void drawPixel(int x, int y, const ColorZUVL& z);
  };
 protected:
  unsigned int  *bufRGBW;
  float   *bufZ;
  int     width;
  int     height;
  int     xMin;
  int     yMin;
  int     zMin;
  int     xMax;
  int     yMax;
  int     zMax;
  std::vector< NIFFile::NIFVertex >   vertexBuf;
  std::vector< NIFFile::NIFTriangle > triangleBuf;
 public:
  // the alpha channel is 255 for solid geometry, 0 for air, and 1 to 254
  // for water with light level converted to RGB multiplier of (alpha - 1) / 64
  Plot3D_TriShape(unsigned int *outBufRGBW, float *outBufZ,
                  int imageWidth, int imageHeight,
                  const NIFFile::NIFTriShape& t,
                  const NIFFile::NIFVertexTransform& viewTransform);
  virtual ~Plot3D_TriShape();
  inline void getBounds(int& x0, int& y0, int& z0,
                        int& x1, int& y1, int& z1) const
  {
    x0 = xMin;
    y0 = yMin;
    z0 = zMin;
    x1 = xMax;
    y1 = yMax;
    z1 = zMax;
  }
  // calculate RGB multiplier from normals and light direction
  virtual float
      calculateLighting(float normalX, float normalY, float normalZ,
                        float lightX, float lightY, float lightZ) const;
  void drawTriShape(float lightX, float lightY, float lightZ,
                    const DDSTexture *textureD,
                    const DDSTexture *textureN = (DDSTexture *) 0);
};

#endif

