
#ifndef PLOT3D_HPP_INCLUDED
#define PLOT3D_HPP_INCLUDED

struct ColorZRGB
{
  float   z;
  float   r;
  float   g;
  float   b;
  inline ColorZRGB& operator+=(const ColorZRGB& d)
  {
    z += d.z;
    r += d.r;
    g += d.g;
    b += d.b;
    return (*this);
  }
  inline ColorZRGB& operator-=(const ColorZRGB& d)
  {
    z -= d.z;
    r -= d.r;
    g -= d.g;
    b -= d.b;
    return (*this);
  }
  inline ColorZRGB& operator*=(float d)
  {
    z *= d;
    r *= d;
    g *= d;
    b *= d;
    return (*this);
  }
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

#endif

