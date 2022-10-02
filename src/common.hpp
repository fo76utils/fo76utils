
#ifndef COMMON_HPP_INCLUDED
#define COMMON_HPP_INCLUDED

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <cstdint>
#include <cmath>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <exception>
#include <stdexcept>

#if defined(__GNUC__) && (defined(__x86_64__) || defined(__x86_64))
#  if defined(__AVX__) && !defined(ENABLE_X86_64_AVX)
#    define ENABLE_X86_64_AVX   1
#  endif
#endif
#ifndef ENABLE_X86_64_AVX
#  define ENABLE_X86_64_AVX     0
#endif

class FO76UtilsError : public std::exception
{
 protected:
  static const char *defaultErrorMessage;
  const char  *s;
  char    *buf;
  void storeMessage(const char *msg, bool copyFlag) noexcept;
 public:
  FO76UtilsError() noexcept
  {
    s = defaultErrorMessage;
    buf = (char *) 0;
  }
  FO76UtilsError(const FO76UtilsError& r) noexcept
  {
    storeMessage(r.s, bool(r.buf));
  }
  FO76UtilsError(int copyFlag, const char *msg) noexcept
  {
    storeMessage(msg, bool(copyFlag));
  }
  FO76UtilsError(const char *fmt, ...) noexcept;
  virtual ~FO76UtilsError() noexcept;
  FO76UtilsError& operator=(const FO76UtilsError& r) noexcept
  {
    storeMessage(r.s, bool(r.buf));
    return (*this);
  }
  virtual const char *what() const noexcept
  {
    return s;
  }
};

#ifdef __GNUC__
__attribute__ ((__noreturn__))
#endif
inline void errorMessage(const char *msg)
{
  throw FO76UtilsError(0, msg);
}

#if defined(__GNUC__)
#  define BRANCH_EXPECT(x, y)   (__builtin_expect(long(bool(x)), long(y)))
#  define BRANCH_LIKELY(x)      (__builtin_expect(long(bool(x)), 1L))
#  define BRANCH_UNLIKELY(x)    (__builtin_expect(long(bool(x)), 0L))
#else
#  define BRANCH_EXPECT(x, y)   (x)
#  define BRANCH_LIKELY(x)      (x)
#  define BRANCH_UNLIKELY(x)    (x)
#endif

inline int roundFloat(float x)
{
#if ENABLE_X86_64_AVX
  int     tmp;
  __asm__ ("vcvtss2si %1, %0" : "=r" (tmp) : "x" (x));
  return tmp;
#else
  return int(std::rintf(x));
#endif
}

inline int roundDouble(double x)
{
#if ENABLE_X86_64_AVX
  int     tmp;
  __asm__ ("vcvtsd2si %1, %0" : "=r" (tmp) : "x" (x));
  return tmp;
#else
  return int(std::rint(x));
#endif
}

inline unsigned char floatToUInt8Clamped(float x, float scale)
{
  float   tmp = x * scale;
  tmp = (tmp > 0.0f ? (tmp < 255.0f ? tmp : 255.0f) : 0.0f);
  return (unsigned char) roundFloat(tmp);
}

inline std::int16_t uint16ToSigned(unsigned short x)
{
  return std::int16_t(x);
}

inline std::int32_t uint32ToSigned(unsigned int x)
{
  return std::int32_t(x);
}

inline float convertFloat16(unsigned short n)
{
#if defined(__i386__) || defined(__x86_64__) || defined(__x86_64)
  std::uint32_t m = (std::uint32_t) int((std::int16_t) n);
  union
  {
    std::uint32_t i;
    float   f;
  }
  tmp;
  tmp.i = ((m << 13) & 0x8FFFE000U) + 0x38000000U;
  float   r = tmp.f;
  if (BRANCH_UNLIKELY(!(m & 0x7C00U)))
  {
    // zero or denormal
    tmp.i = tmp.i & 0xFF800000U;
    r = r - tmp.f;
    r = r + r;
  }
  return r;
#else
  unsigned char e = (unsigned char) ((n >> 10) & 0x1F);
  if (!e)
    return 0.0f;
  long long m = (long long) ((n & 0x03FF) | 0x0400) << e;
  return (float(!(n & 0x8000) ? m : -m) * (1.0f / 33554432.0f));
#endif
}

std::uint16_t convertToFloat16(float x);

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

inline unsigned long long rgb24ToRBG64(unsigned int c)
{
  return ((unsigned long long) ((c & 0x000000FF) | ((c & 0x00FF0000) << 4))
          | ((unsigned long long) (c & 0x0000FF00) << 32));
}

inline unsigned long long blendRBG64(unsigned long long a, unsigned long long b,
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

#endif

