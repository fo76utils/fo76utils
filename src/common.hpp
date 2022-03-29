
#ifndef COMMON_HPP_INCLUDED
#define COMMON_HPP_INCLUDED

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <cmath>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <exception>
#include <stdexcept>

std::runtime_error errorMessage(const char *fmt, ...);

inline int roundFloat(float x)
{
#if 0
  return int(x + (x < 0.0f ? -0.5f : 0.5f));
#else
  return int(std::lrintf(x));
#endif
}

inline int uint16ToSigned(unsigned short x)
{
  if (sizeof(short) == 2)
    return int(short(x));
  return (int((unsigned int) x ^ 0x8000U) + int(-0x8000));
}

inline int uint32ToSigned(unsigned int x)
{
  if (sizeof(int) == 4)
    return int(x);
  return (int(x ^ 0x80000000U) + int(-0x80000000));
}

inline unsigned long long rgba32ToRBGA64(unsigned int c)
{
  return ((unsigned long long) (c & 0x00FF00FFU)
          | ((unsigned long long) (c & 0xFF00FF00U) << 24));
}

inline unsigned int rbga64ToRGBA32(unsigned long long c)
{
  return (((unsigned int) c & 0x00FF00FFU)
          | ((unsigned int) (c >> 24) & 0xFF00FF00U));
}

inline unsigned long long blendRBGA64(unsigned long long c0,
                                      unsigned long long c1, int f)
{
  unsigned int  m = (unsigned int) f;
  unsigned long long  tmp = (c0 * (256U - m)) + (c1 * m);
  return (((tmp + 0x0080008000800080ULL) >> 8) & 0x00FF00FF00FF00FFULL);
}

inline unsigned long long blendRGBA32ToRBGA64(unsigned int c0,
                                              unsigned int c1, int f)
{
  return blendRBGA64(rgba32ToRBGA64(c0), rgba32ToRBGA64(c1), f);
}

inline unsigned int blendRGBA32(unsigned int c0, unsigned int c1, int f)
{
  unsigned long long  tmp0 = rgba32ToRBGA64(c0);
  unsigned long long  tmp1 = rgba32ToRBGA64(c1);
  unsigned int  m = (unsigned int) f;
  tmp0 = (tmp0 * (256U - m)) + (tmp1 * m) + 0x0080008000800080ULL;
  return (((unsigned int) (tmp0 & 0xFF00FF00U) >> 8)
          | ((unsigned int) (tmp0 >> 32) & 0xFF00FF00U));
}

inline unsigned long long blendRGB64(unsigned long long a, unsigned long long b,
                                     unsigned int opacityB)
{
  unsigned long long  tmp = (a << 8) + ((b - a) * opacityB);
  return (((tmp + 0x0000800008000080ULL) >> 8) & 0x000FFF00FFF00FFFULL);
}

inline unsigned char blendDithered(unsigned char a, unsigned char b,
                                   unsigned char opacityB, int x, int y)
{
  static const unsigned char  t[64] =
  {
     0, 48, 12, 60,  3, 51, 15, 63, 32, 16, 44, 28, 35, 19, 47, 31,
     8, 56,  4, 52, 11, 59,  7, 55, 40, 24, 36, 20, 43, 27, 39, 23,
     2, 50, 14, 62,  1, 49, 13, 61, 34, 18, 46, 30, 33, 17, 45, 29,
    10, 58,  6, 54,  9, 57,  5, 53, 42, 26, 38, 22, 41, 25, 37, 21
  };
  unsigned char d = t[((y & 7) << 3) | (x & 7)];
  return (((((unsigned int) opacityB + 2) >> 2) + d) < 64 ? a : b);
}

long parseInteger(const char *s, int base = 0, const char *errMsg = (char *) 0,
                  long minVal = long((~0UL >> 1) + 1UL),
                  long maxVal = long(~0UL >> 1));

double parseFloat(const char *s, const char *errMsg = (char *) 0,
                  double minVal = -1.0e38, double maxVal = 1.0e38);

#if defined(__GNUC__)
#  define BRANCH_EXPECT(x, y)   (__builtin_expect(long(bool(x)), long(y)))
#else
#  define BRANCH_EXPECT(x, y)   (x)
#endif

#endif

