
#ifndef FP32VEC4_HPP_INCLUDED
#define FP32VEC4_HPP_INCLUDED

#include "common.hpp"

#if defined(__GNUC__) && (defined(__x86_64__) || defined(__x86_64))
typedef float   XMM_Float __attribute__ ((__vector_size__ (16)));
typedef double  XMM_Double __attribute__ ((__vector_size__ (16)));
typedef std::uint8_t  XMM_UInt8 __attribute__ ((__vector_size__ (16)));
typedef std::int8_t   XMM_Int8 __attribute__ ((__vector_size__ (16)));
typedef std::uint16_t XMM_UInt16 __attribute__ ((__vector_size__ (16)));
typedef std::int16_t  XMM_Int16 __attribute__ ((__vector_size__ (16)));
typedef std::uint32_t XMM_UInt32 __attribute__ ((__vector_size__ (16)));
typedef std::int32_t  XMM_Int32 __attribute__ ((__vector_size__ (16)));
typedef std::uint64_t XMM_UInt64 __attribute__ ((__vector_size__ (16)));
typedef std::int64_t  XMM_Int64 __attribute__ ((__vector_size__ (16)));
#  if ENABLE_X86_64_AVX
typedef float   YMM_Float __attribute__ ((__vector_size__ (32)));
typedef double  YMM_Double __attribute__ ((__vector_size__ (32)));
typedef std::uint8_t  YMM_UInt8 __attribute__ ((__vector_size__ (32)));
typedef std::int8_t   YMM_Int8 __attribute__ ((__vector_size__ (32)));
typedef std::uint16_t YMM_UInt16 __attribute__ ((__vector_size__ (32)));
typedef std::int16_t  YMM_Int16 __attribute__ ((__vector_size__ (32)));
typedef std::uint32_t YMM_UInt32 __attribute__ ((__vector_size__ (32)));
typedef std::int32_t  YMM_Int32 __attribute__ ((__vector_size__ (32)));
typedef std::uint64_t YMM_UInt64 __attribute__ ((__vector_size__ (32)));
typedef std::int64_t  YMM_Int64 __attribute__ ((__vector_size__ (32)));
#  endif
#endif

#ifndef USE_PIXELFMT_RGB10A2
#  define USE_PIXELFMT_RGB10A2  0
#endif

struct FloatVector4
{
#if ENABLE_X86_64_AVX
 private:
  static constexpr float      floatMinVal = 5.16987883e-26f;
  static constexpr XMM_Float  floatMinValV =
  {
    floatMinVal, floatMinVal, floatMinVal, floatMinVal
  };
 public:
  XMM_Float v;
  inline FloatVector4(const XMM_Float& r)
    : v(r)
  {
  }
#else
  float   v[4];
#endif
  inline FloatVector4()
  {
  }
  // construct from R8G8B8A8 color
  inline FloatVector4(unsigned int c);
  inline FloatVector4(float x);
  inline FloatVector4(const std::uint32_t *p);
  // the first two elements are taken from p1, and the third and fourth from p2
  inline FloatVector4(const std::uint32_t *p1, const std::uint32_t *p2);
  inline FloatVector4(float v0, float v1, float v2, float v3);
  inline FloatVector4(const float *p);
  // construct from 4 R8G8B8A8 colors with bilinear interpolation
  inline FloatVector4(unsigned int c0, unsigned int c1,
                      unsigned int c2, unsigned int c3,
                      float xf, float yf, bool isSRGB = false);
  inline FloatVector4(const std::uint32_t *p0, const std::uint32_t *p1,
                      const std::uint32_t *p2, const std::uint32_t *p3,
                      float xf, float yf, bool isSRGB = false);
  inline FloatVector4(const std::uint32_t *p1_0, const std::uint32_t *p2_0,
                      const std::uint32_t *p1_1, const std::uint32_t *p2_1,
                      const std::uint32_t *p1_2, const std::uint32_t *p2_2,
                      const std::uint32_t *p1_3, const std::uint32_t *p2_3,
                      float xf, float yf, bool isSRGB = false);
  static inline FloatVector4 convertFloat16(std::uint64_t n);
  static inline FloatVector4 convertA2R10G10B10(const std::uint32_t& c);
  // convert from the pixel format selected with USE_PIXELFMT_RGB10A2
  static inline FloatVector4 convertRGBA32(const std::uint32_t& c);
  inline float& operator[](size_t n)
  {
    return v[n];
  }
  inline const float& operator[](size_t n) const
  {
    return v[n];
  }
  inline FloatVector4& operator+=(const FloatVector4& r);
  inline FloatVector4& operator-=(const FloatVector4& r);
  inline FloatVector4& operator*=(const FloatVector4& r);
  inline FloatVector4& operator/=(const FloatVector4& r);
  inline FloatVector4 operator+(const FloatVector4& r) const;
  inline FloatVector4 operator-(const FloatVector4& r) const;
  inline FloatVector4 operator*(const FloatVector4& r) const;
  inline FloatVector4 operator/(const FloatVector4& r) const;
  inline FloatVector4& operator+=(float r);
  inline FloatVector4& operator-=(float r);
  inline FloatVector4& operator*=(float r);
  inline FloatVector4& operator/=(float r);
  inline FloatVector4 operator+(const float& r) const;
  inline FloatVector4 operator-(const float& r) const;
  inline FloatVector4 operator*(const float& r) const;
  inline FloatVector4 operator/(const float& r) const;
  inline FloatVector4& clearV3();
  inline FloatVector4& minValues(const FloatVector4& r);
  inline FloatVector4& maxValues(const FloatVector4& r);
  inline float dotProduct(const FloatVector4& r) const;
  // dot product of first three elements
  inline float dotProduct3(const FloatVector4& r) const;
  // dot product of first two elements
  inline float dotProduct2(const FloatVector4& r) const;
  inline FloatVector4& squareRoot();
  inline FloatVector4& squareRootFast();
  inline FloatVector4& rsqrtFast();     // elements must be positive
  static inline float squareRootFast(float x);
  static inline float log2Fast(float x);
  static inline int log2Int(int x);
  // ((x * p[0] + p[1]) * x + p[2]) * x + p[3]
  static inline float polynomial3(const float *p, float x);
  static inline float exp2Fast(float x);
  inline FloatVector4& normalize(bool invFlag = false);
  // normalize first three elements (v[3] is cleared)
  inline FloatVector4& normalize3Fast();
  // returns a vector of values a, b, and c need to be multiplied with
  static inline FloatVector4 normalize3x3Fast(
      const FloatVector4& a, const FloatVector4& b, const FloatVector4& c);
  // 0.0 - 255.0 sRGB -> 0.0 - 1.0 linear
  inline FloatVector4& srgbExpand();
  // 0.0 - 1.0 linear -> 0.0 - 255.0 sRGB (clamps input to 0.0 - 1.0)
  inline FloatVector4& srgbCompress();
  inline operator std::uint32_t() const;
  inline std::uint32_t convertToA2R10G10B10(bool noClamp = false) const;
  // convert to the pixel format selected with USE_PIXELFMT_RGB10A2
  inline std::uint32_t convertToRGBA32(bool noAlpha = false,
                                       bool noClamp = false) const;
};

#if ENABLE_X86_64_AVX

inline FloatVector4::FloatVector4(unsigned int c)
{
  XMM_Float tmp;
  __asm__ ("vmovd %1, %0" : "=x" (tmp) : "rm" (c));
  __asm__ ("vpmovzxbd %0, %0" : "+x" (tmp));
  __asm__ ("vcvtdq2ps %1, %0" : "=x" (v) : "x" (tmp));
}

inline FloatVector4::FloatVector4(float x)
  : v { x, x, x, x }
{
}

inline FloatVector4::FloatVector4(const std::uint32_t *p)
{
  XMM_Float tmp;
  __asm__ ("vpmovzxbd %1, %0" : "=x" (tmp) : "m" (*p));
  __asm__ ("vcvtdq2ps %1, %0" : "=x" (v) : "x" (tmp));
}

inline FloatVector4::FloatVector4(const std::uint32_t *p1,
                                  const std::uint32_t *p2)
{
  XMM_Float tmp1, tmp2;
  __asm__ ("vpmovzxbd %1, %0" : "=x" (tmp1) : "m" (*p1));
  __asm__ ("vpmovzxbd %1, %0" : "=x" (tmp2) : "m" (*p2));
  __asm__ ("vpunpcklqdq %1, %0, %0" : "+x" (tmp1) : "x" (tmp2));
  __asm__ ("vcvtdq2ps %1, %0" : "=x" (v) : "x" (tmp1));
}

inline FloatVector4::FloatVector4(float v0, float v1, float v2, float v3)
  : v { v0, v1, v2, v3 }
{
}

inline FloatVector4::FloatVector4(const float *p)
  : v { p[0], p[1], p[2], p[3] }
{
}

inline FloatVector4 FloatVector4::convertFloat16(std::uint64_t n)
{
  XMM_Float v;
#if ENABLE_X86_64_AVX2
  __asm__ ("vmovq %1, %0" : "=x" (v) : "rm" (n));
  __asm__ ("vcvtph2ps %0, %0" : "+x" (v));
#else
  XMM_Int32 tmp1;
  XMM_Int32 tmp2 =
  {
    std::int32_t(0x8FFFE000U), 0x38000000, std::int32_t(0xB8000000U), 0
  };
  __asm__ ("vmovq %1, %0" : "=x" (v) : "rm" (n));
  __asm__ ("vpmovsxwd %0, %0" : "+x" (v));
  __asm__ ("vpshufd $0x00, %1, %0" : "=x" (tmp1) : "x" (tmp2));
  __asm__ ("vpslld $0x0d, %0, %0" : "+x" (v));
  __asm__ ("vpand %1, %0, %0" : "+x" (v) : "x" (tmp1));
  __asm__ ("vpshufd $0x55, %1, %0" : "=x" (tmp1) : "x" (tmp2));
  __asm__ ("vpshufd $0xaa, %0, %0" : "+x" (tmp2));
  __asm__ ("vpaddd %1, %0, %0" : "+x" (v) : "x" (tmp1));
  __asm__ ("vpand %1, %0, %0" : "+x" (tmp2) : "x" (v));
  __asm__ ("vsubps %0, %1, %0" : "+x" (tmp2) : "x" (v));
  __asm__ ("vaddps %0, %0, %0" : "+x" (tmp2));
  __asm__ ("vpminud %1, %0, %0" : "+x" (v) : "x" (tmp2));
#endif
  return FloatVector4(v);
}

inline FloatVector4 FloatVector4::convertA2R10G10B10(const std::uint32_t& c)
{
  static const XMM_UInt8  shufTbl =
  {
    2, 3, 0x80, 0x80, 1, 2, 0x80, 0x80, 0, 1, 0x80, 0x80, 3, 0x80, 0x80, 0x80
  };
  static const XMM_UInt32 maskTbl =
  {
    0x00003FF0U, 0x00000FFCU, 0x000003FFU, 0x000000C0U
  };
  static const XMM_Float  multTbl =
  {
    255.0f / 16368.0f, 255.0f / 4092.0f, 255.0f / 1023.0f, 255.0f / 192.0f
  };
  XMM_Float v;
  __asm__ ("vmovd %1, %0" : "=x" (v) : "rm" (c));
  __asm__ ("vpshufb %1, %0, %0" : "+x" (v) : "xm" (shufTbl));
  __asm__ ("vpand %1, %0, %0" : "+x" (v) : "xm" (maskTbl));
  __asm__ ("vcvtdq2ps %0, %0" : "+x" (v));
  __asm__ ("vmulps %1, %0, %0" : "+x" (v) : "xm" (multTbl));
  return FloatVector4(v);
}

inline FloatVector4& FloatVector4::operator+=(const FloatVector4& r)
{
  v += r.v;
  return (*this);
}

inline FloatVector4& FloatVector4::operator-=(const FloatVector4& r)
{
  v -= r.v;
  return (*this);
}

inline FloatVector4& FloatVector4::operator*=(const FloatVector4& r)
{
  v *= r.v;
  return (*this);
}

inline FloatVector4& FloatVector4::operator/=(const FloatVector4& r)
{
  v /= r.v;
  return (*this);
}

inline FloatVector4 FloatVector4::operator+(const FloatVector4& r) const
{
  FloatVector4  tmp(*this);
  return (tmp += r);
}

inline FloatVector4 FloatVector4::operator-(const FloatVector4& r) const
{
  FloatVector4  tmp(*this);
  return (tmp -= r);
}

inline FloatVector4 FloatVector4::operator*(const FloatVector4& r) const
{
  FloatVector4  tmp(*this);
  return (tmp *= r);
}

inline FloatVector4 FloatVector4::operator/(const FloatVector4& r) const
{
  FloatVector4  tmp(*this);
  return (tmp /= r);
}

inline FloatVector4& FloatVector4::operator+=(float r)
{
  v += r;
  return (*this);
}

inline FloatVector4& FloatVector4::operator-=(float r)
{
  v -= r;
  return (*this);
}

inline FloatVector4& FloatVector4::operator*=(float r)
{
  v *= r;
  return (*this);
}

inline FloatVector4& FloatVector4::operator/=(float r)
{
  v /= r;
  return (*this);
}

inline FloatVector4 FloatVector4::operator+(const float& r) const
{
  FloatVector4  tmp(*this);
  return (tmp += r);
}

inline FloatVector4 FloatVector4::operator-(const float& r) const
{
  FloatVector4  tmp(*this);
  return (tmp -= r);
}

inline FloatVector4 FloatVector4::operator*(const float& r) const
{
  FloatVector4  tmp(*this);
  return (tmp *= r);
}

inline FloatVector4 FloatVector4::operator/(const float& r) const
{
  FloatVector4  tmp(*this);
  return (tmp /= r);
}

inline FloatVector4& FloatVector4::clearV3()
{
  const XMM_Float tmp = { 0.0f, 0.0f, 0.0f, 0.0f };
  __asm__ ("vblendps $0x08, %1, %0, %0" : "+x" (v) : "xm" (tmp));
  return (*this);
}

inline FloatVector4& FloatVector4::minValues(const FloatVector4& r)
{
  __asm__ ("vminps %1, %0, %0" : "+x" (v) : "xm" (r.v));
  return (*this);
}

inline FloatVector4& FloatVector4::maxValues(const FloatVector4& r)
{
  __asm__ ("vmaxps %1, %0, %0" : "+x" (v) : "xm" (r.v));
  return (*this);
}

inline float FloatVector4::dotProduct(const FloatVector4& r) const
{
  XMM_Float tmp1;
  __asm__ ("vmulps %2, %1, %0" : "=x" (tmp1) : "x" (v), "xm" (r.v));
  XMM_Float tmp2;
  __asm__ ("vmovshdup %1, %0" : "=x" (tmp2) : "x" (tmp1));
  __asm__ ("vaddps %1, %0, %0" : "+x" (tmp1) : "x" (tmp2));
  __asm__ ("vshufps $0x4e, %1, %1, %0" : "=x" (tmp2) : "x" (tmp1));
  __asm__ ("vaddps %1, %0, %0" : "+x" (tmp1) : "x" (tmp2));
  return tmp1[0];
}

inline float FloatVector4::dotProduct3(const FloatVector4& r) const
{
  XMM_Float tmp1;
  __asm__ ("vmulps %2, %1, %0" : "=x" (tmp1) : "x" (v), "xm" (r.v));
  XMM_Float tmp2;
  __asm__ ("vmovshdup %1, %0" : "=x" (tmp2) : "x" (tmp1));
  __asm__ ("vaddps %1, %0, %0" : "+x" (tmp2) : "x" (tmp1));
  __asm__ ("vshufps $0x4e, %0, %0, %0" : "+x" (tmp1));
  __asm__ ("vaddps %1, %0, %0" : "+x" (tmp1) : "x" (tmp2));
  return tmp1[0];
}

inline float FloatVector4::dotProduct2(const FloatVector4& r) const
{
  XMM_Float tmp1;
  __asm__ ("vmulps %2, %1, %0" : "=x" (tmp1) : "x" (v), "xm" (r.v));
  XMM_Float tmp2;
  __asm__ ("vmovshdup %1, %0" : "=x" (tmp2) : "x" (tmp1));
  __asm__ ("vaddps %1, %0, %0" : "+x" (tmp1) : "x" (tmp2));
  return tmp1[0];
}

inline FloatVector4& FloatVector4::squareRoot()
{
  XMM_Float tmp = { 0.0f, 0.0f, 0.0f, 0.0f };
  __asm__ ("vmaxps %1, %0, %0" : "+x" (tmp) : "x" (v));
  __asm__ ("vsqrtps %1, %0" : "=x" (v) : "x" (tmp));
  return (*this);
}

inline FloatVector4& FloatVector4::squareRootFast()
{
  XMM_Float tmp1 = v;
  __asm__ ("vmaxps %1, %0, %0" : "+x" (tmp1) : "xm" (floatMinValV));
  __asm__ ("vrsqrtps %1, %0" : "=x" (v) : "x" (tmp1));
  v *= tmp1;
  return (*this);
}

inline FloatVector4& FloatVector4::rsqrtFast()
{
  __asm__ ("vrsqrtps %0, %0" : "+x" (v));
  return (*this);
}

inline float FloatVector4::squareRootFast(float x)
{
  float   tmp1 = x;
  const float tmp2 = floatMinVal;
  __asm__ ("vmaxss %1, %0, %0" : "+x" (tmp1) : "xm" (tmp2));
  float   tmp3;
  __asm__ ("vrsqrtss %1, %1, %0" : "=x" (tmp3) : "x" (tmp1));
  return (tmp1 * tmp3);
}

inline float FloatVector4::log2Fast(float x)
{
  union
  {
    float   f;
    std::uint32_t n;
  }
  tmp;
  tmp.f = x;
  std::uint32_t n = tmp.n;
  float   e = float(int(n >> 23));
  tmp.n = (n & 0x007FFFFFU) | 0x3F800000U;
  float   m = tmp.f;
  float   m2 = m * m;
  FloatVector4  tmp2(m, m2, m * m2, m2 * m2);
  FloatVector4  tmp3(4.05608897f, -2.10465275f, 0.63728021f, -0.08021013f);
  return (tmp2.dotProduct(tmp3) + e - (127.0f + 2.50847106f));
}

inline int FloatVector4::log2Int(int x)
{
  int     tmp = x;              // return 0 if x == 0
  __asm__ ("bsr %0, %0" : "+r" (tmp) : : "cc");
  return tmp;
}

inline float FloatVector4::polynomial3(const float *p, float x)
{
  return ((x * p[0] + p[1]) * (x * x) + (x * p[2] + p[3]));
}

inline float FloatVector4::exp2Fast(float x)
{
  static const float  exp2Polynomial[4] =
  {
    0.00825060f, 0.05924474f, 0.34671664f, 1.0f
  };
  float   e = float(std::floor(x));
  float   m = x - e;
  __asm__ ("vcvtps2dq %0, %0" : "+x" (e));
  __asm__ ("vpslld $0x17, %0, %0" : "+x" (e));
  m = polynomial3(exp2Polynomial, m);
  m = m * m;
  __asm__ ("vpaddd %1, %0, %0" : "+x" (m) : "x" (e));
  return m;
}

inline FloatVector4& FloatVector4::normalize(bool invFlag)
{
  static const float  invMultTable[2] = { -0.5f, 0.5f };
  float   tmp = dotProduct(*this);
  float   tmp2;
  __asm__ ("vmaxss %2, %1, %0"
           : "=x" (tmp2) : "x" (tmp), "xm" (floatMinValV[0]));
  __asm__ ("vrsqrtss %0, %0, %0" : "+x" (tmp2));
  v *= ((tmp * tmp2 * tmp2 - 3.0f) * (tmp2 * invMultTable[int(invFlag)]));
  return (*this);
}

inline FloatVector4& FloatVector4::normalize3Fast()
{
  const FloatVector4  minVal(1.0f / float(0x0000040000000000LL));
  XMM_Float tmp = v;
  __asm__ ("vblendps $0x08, %1, %0, %0" : "+x" (tmp) : "xm" (minVal.v));
  XMM_Float tmp2 = tmp * tmp;
  XMM_Float tmp3;
  __asm__ ("vshufps $0xb1, %1, %1, %0" : "=x" (tmp3) : "x" (tmp2));
  __asm__ ("vaddps %1, %0, %0" : "+x" (tmp2) : "x" (tmp3));
  __asm__ ("vshufps $0x4e, %1, %1, %0" : "=x" (tmp3) : "x" (tmp2));
  __asm__ ("vaddps %1, %0, %0" : "+x" (tmp2) : "x" (tmp3));
  __asm__ ("vrsqrtps %0, %0" : "+x" (tmp2));
  v = tmp * tmp2;
  return (*this);
}

inline FloatVector4 FloatVector4::normalize3x3Fast(
    const FloatVector4& a, const FloatVector4& b, const FloatVector4& c)
{
  XMM_Float tmp1, tmp2, tmp3;
  // tmp1 = b[0], b[0], c[0], c[0]
  __asm__ ("vshufps $0x00, %2, %1, %0" : "=x" (tmp1) : "x" (b), "xm" (c));
  // tmp2 = a[1], a[1], c[1], c[1]
  __asm__ ("vshufps $0x55, %2, %1, %0" : "=x" (tmp2) : "x" (a), "xm" (c));
  // tmp3 = a[2], b[2], a[3], b[3]
  __asm__ ("vunpckhps %2, %1, %0" : "=x" (tmp3) : "x" (a), "xm" (b));
  __asm__ ("vblendps $0x09, %1, %0, %0" : "+x" (tmp1) : "xm" (a));
  __asm__ ("vblendps $0x0a, %1, %0, %0" : "+x" (tmp2) : "xm" (b));
  __asm__ ("vblendps $0x0c, %1, %0, %0" : "+x" (tmp3) : "xm" (c));
  tmp1 = (tmp1 * tmp1) + (tmp2 * tmp2) + (tmp3 * tmp3);
  __asm__ ("vmaxps %1, %0, %0" : "+x" (tmp1) : "xm" (floatMinValV));
  __asm__ ("vrsqrtps %0, %0" : "+x" (tmp1));
  return tmp1;
}

inline FloatVector4& FloatVector4::srgbExpand()
{
  v *= (v * float(0.13945550 / 65025.0) + float(0.86054450 / 255.0));
  v *= v;
  return (*this);
}

inline FloatVector4& FloatVector4::srgbCompress()
{
  minValues(FloatVector4(1.0f));
  maxValues(FloatVector4(1.0f / float(0x40000000)));
  XMM_Float tmp;
#if USE_PIXELFMT_RGB10A2
  // more accurate inverse function of srgbExpand() in 10 bits per channel mode
  XMM_Float tmp2 = v * float(0.03876962 * 255.0) + float(1.15864660 * 255.0);
#endif
  __asm__ ("vrsqrtps %1, %0" : "=x" (tmp) : "x" (v));
#if USE_PIXELFMT_RGB10A2
  v *= (tmp * tmp2 - float(0.19741622 * 255.0));
#else
  v *= (float(-0.13942692 * 255.0) + (tmp * float(1.13942692 * 255.0)));
#endif
  return (*this);
}

inline FloatVector4::operator std::uint32_t() const
{
  XMM_Float tmp = v;
  std::uint32_t c;
  __asm__ ("vcvtps2dq %0, %0" : "+x" (tmp));
  __asm__ ("vpackssdw %0, %0, %0" : "+x" (tmp));
  __asm__ ("vpackuswb %0, %0, %0" : "+x" (tmp));
  __asm__ ("vmovd %1, %0" : "=r" (c) : "x" (tmp));
  return c;
}

inline std::uint32_t FloatVector4::convertToA2R10G10B10(bool noClamp) const
{
  static const XMM_Float  multTbl1 =
  {
    1023.0f / 255.0f, 1023.0f / 255.0f, 1023.0f / 255.0f, 3.0f / 255.0f
  };
  static const XMM_UInt8  shufTbl =
  {
    4, 5, 0, 1, 8, 9, 12, 13, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80
  };
  static const XMM_Int16  multTbl2 = { 64, 1, 1, 16384, 0, 0, 0, 0 };
  XMM_Float   tmp1;
  XMM_UInt32  tmp2;
  if (noClamp)
  {
    __asm__ ("vmulps %2, %1, %0" : "=x" (tmp1) : "x" (v), "xm" (multTbl1));
  }
  else
  {
    static const XMM_Float  maxTbl = { 255.0f, 255.0f, 255.0f, 255.0f };
    __asm__ ("vxorps %0, %0, %0" : "=x" (tmp2));
    __asm__ ("vminps %2, %1, %0" : "=x" (tmp1) : "x" (v), "xm" (maxTbl));
    __asm__ ("vmaxps %1, %0, %0" : "+x" (tmp1) : "x" (tmp2));
    __asm__ ("vmulps %1, %0, %0" : "+x" (tmp1) : "xm" (multTbl1));
  }
  __asm__ ("vcvtps2dq %0, %0" : "+x" (tmp1));
  __asm__ ("vpshufb %1, %0, %0" : "+x" (tmp1) : "xm" (shufTbl));
  __asm__ ("vpmullw %1, %0, %0" : "+x" (tmp1) : "xm" (multTbl2));
  __asm__ ("vpsrlq $0x20, %1, %0" : "=x" (tmp2) : "x" (tmp1));
  __asm__ ("vpslld $0x04, %0, %0" : "+x" (tmp1));
  __asm__ ("vpor %1, %0, %0" : "+x" (tmp2) : "x" (tmp1));
  return tmp2[0];
}

#else                                   // !ENABLE_X86_64_AVX

inline FloatVector4::FloatVector4(unsigned int c)
{
  v[0] = float(int(c & 0xFF));
  v[1] = float(int((c >> 8) & 0xFF));
  v[2] = float(int((c >> 16) & 0xFF));
  v[3] = float(int((c >> 24) & 0xFF));
}

inline FloatVector4::FloatVector4(float x)
{
  v[0] = x;
  v[1] = x;
  v[2] = x;
  v[3] = x;
}

inline FloatVector4::FloatVector4(const std::uint32_t *p)
{
  std::uint32_t c = *p;
  v[0] = float(int(c & 0xFF));
  v[1] = float(int((c >> 8) & 0xFF));
  v[2] = float(int((c >> 16) & 0xFF));
  v[3] = float(int((c >> 24) & 0xFF));
}

inline FloatVector4::FloatVector4(const std::uint32_t *p1,
                                  const std::uint32_t *p2)
{
  std::uint32_t c1 = *p1;
  std::uint32_t c2 = *p2;
  v[0] = float(int(c1 & 0xFF));
  v[1] = float(int((c1 >> 8) & 0xFF));
  v[2] = float(int(c2 & 0xFF));
  v[3] = float(int((c2 >> 8) & 0xFF));
}

inline FloatVector4::FloatVector4(float v0, float v1, float v2, float v3)
{
  v[0] = v0;
  v[1] = v1;
  v[2] = v2;
  v[3] = v3;
}

inline FloatVector4::FloatVector4(const float *p)
{
  v[0] = p[0];
  v[1] = p[1];
  v[2] = p[2];
  v[3] = p[3];
}

inline FloatVector4 FloatVector4::convertFloat16(std::uint64_t n)
{
  return FloatVector4(::convertFloat16(std::uint16_t(n & 0xFFFFU)),
                      ::convertFloat16(std::uint16_t((n >> 16) & 0xFFFFU)),
                      ::convertFloat16(std::uint16_t((n >> 32) & 0xFFFFU)),
                      ::convertFloat16(std::uint16_t((n >> 48) & 0xFFFFU)));
}

inline FloatVector4 FloatVector4::convertA2R10G10B10(const std::uint32_t& c)
{
  float   r = float(int((c >> 20) & 0x03FF));
  float   g = float(int((c >> 10) & 0x03FF));
  float   b = float(int(c & 0x03FF));
  float   a = float(int((c >> 30) & 0x0003));
  FloatVector4  tmp(r, g, b, a);
  tmp *= FloatVector4(255.0f / 1023.0f, 255.0f / 1023.0f,
                      255.0f / 1023.0f, 255.0f / 3.0f);
  return tmp;
}

inline FloatVector4& FloatVector4::operator+=(const FloatVector4& r)
{
  v[0] += r.v[0];
  v[1] += r.v[1];
  v[2] += r.v[2];
  v[3] += r.v[3];
  return (*this);
}

inline FloatVector4& FloatVector4::operator-=(const FloatVector4& r)
{
  v[0] -= r.v[0];
  v[1] -= r.v[1];
  v[2] -= r.v[2];
  v[3] -= r.v[3];
  return (*this);
}

inline FloatVector4& FloatVector4::operator*=(const FloatVector4& r)
{
  v[0] *= r.v[0];
  v[1] *= r.v[1];
  v[2] *= r.v[2];
  v[3] *= r.v[3];
  return (*this);
}

inline FloatVector4& FloatVector4::operator/=(const FloatVector4& r)
{
  v[0] /= r.v[0];
  v[1] /= r.v[1];
  v[2] /= r.v[2];
  v[3] /= r.v[3];
  return (*this);
}

inline FloatVector4 FloatVector4::operator+(const FloatVector4& r) const
{
  return FloatVector4(v[0] + r.v[0], v[1] + r.v[1],
                      v[2] + r.v[2], v[3] + r.v[3]);
}

inline FloatVector4 FloatVector4::operator-(const FloatVector4& r) const
{
  return FloatVector4(v[0] - r.v[0], v[1] - r.v[1],
                      v[2] - r.v[2], v[3] - r.v[3]);
}

inline FloatVector4 FloatVector4::operator*(const FloatVector4& r) const
{
  return FloatVector4(v[0] * r.v[0], v[1] * r.v[1],
                      v[2] * r.v[2], v[3] * r.v[3]);
}

inline FloatVector4 FloatVector4::operator/(const FloatVector4& r) const
{
  return FloatVector4(v[0] / r.v[0], v[1] / r.v[1],
                      v[2] / r.v[2], v[3] / r.v[3]);
}

inline FloatVector4& FloatVector4::operator+=(float r)
{
  v[0] += r;
  v[1] += r;
  v[2] += r;
  v[3] += r;
  return (*this);
}

inline FloatVector4& FloatVector4::operator-=(float r)
{
  v[0] -= r;
  v[1] -= r;
  v[2] -= r;
  v[3] -= r;
  return (*this);
}

inline FloatVector4& FloatVector4::operator*=(float r)
{
  v[0] *= r;
  v[1] *= r;
  v[2] *= r;
  v[3] *= r;
  return (*this);
}

inline FloatVector4& FloatVector4::operator/=(float r)
{
  v[0] /= r;
  v[1] /= r;
  v[2] /= r;
  v[3] /= r;
  return (*this);
}

inline FloatVector4 FloatVector4::operator+(const float& r) const
{
  return FloatVector4(v[0] + r, v[1] + r, v[2] + r, v[3] + r);
}

inline FloatVector4 FloatVector4::operator-(const float& r) const
{
  return FloatVector4(v[0] - r, v[1] - r, v[2] - r, v[3] - r);
}

inline FloatVector4 FloatVector4::operator*(const float& r) const
{
  return FloatVector4(v[0] * r, v[1] * r, v[2] * r, v[3] * r);
}

inline FloatVector4 FloatVector4::operator/(const float& r) const
{
  return FloatVector4(v[0] / r, v[1] / r, v[2] / r, v[3] / r);
}

inline FloatVector4& FloatVector4::clearV3()
{
  v[3] = 0.0f;
  return (*this);
}

inline FloatVector4& FloatVector4::minValues(const FloatVector4& r)
{
  v[0] = (r.v[0] < v[0] ? r.v[0] : v[0]);
  v[1] = (r.v[1] < v[1] ? r.v[1] : v[1]);
  v[2] = (r.v[2] < v[2] ? r.v[2] : v[2]);
  v[3] = (r.v[3] < v[3] ? r.v[3] : v[3]);
  return (*this);
}

inline FloatVector4& FloatVector4::maxValues(const FloatVector4& r)
{
  v[0] = (r.v[0] > v[0] ? r.v[0] : v[0]);
  v[1] = (r.v[1] > v[1] ? r.v[1] : v[1]);
  v[2] = (r.v[2] > v[2] ? r.v[2] : v[2]);
  v[3] = (r.v[3] > v[3] ? r.v[3] : v[3]);
  return (*this);
}

inline float FloatVector4::dotProduct(const FloatVector4& r) const
{
  FloatVector4  tmp(*this);
  tmp *= r;
  return (tmp[0] + tmp[1] + tmp[2] + tmp[3]);
}

inline float FloatVector4::dotProduct3(const FloatVector4& r) const
{
  FloatVector4  tmp(*this);
  tmp *= r;
  return (tmp[0] + tmp[1] + tmp[2]);
}

inline float FloatVector4::dotProduct2(const FloatVector4& r) const
{
  return ((v[0] * r.v[0]) + (v[1] * r.v[1]));
}

inline FloatVector4& FloatVector4::squareRoot()
{
  maxValues(FloatVector4(0.0f));
  v[0] = float(std::sqrt(v[0]));
  v[1] = float(std::sqrt(v[1]));
  v[2] = float(std::sqrt(v[2]));
  v[3] = float(std::sqrt(v[3]));
  return (*this);
}

inline FloatVector4& FloatVector4::squareRootFast()
{
  maxValues(FloatVector4(0.0f));
  v[0] = float(std::sqrt(v[0]));
  v[1] = float(std::sqrt(v[1]));
  v[2] = float(std::sqrt(v[2]));
  v[3] = float(std::sqrt(v[3]));
  return (*this);
}

inline FloatVector4& FloatVector4::rsqrtFast()
{
  v[0] = 1.0f / float(std::sqrt(v[0]));
  v[1] = 1.0f / float(std::sqrt(v[1]));
  v[2] = 1.0f / float(std::sqrt(v[2]));
  v[3] = 1.0f / float(std::sqrt(v[3]));
  return (*this);
}

inline float FloatVector4::squareRootFast(float x)
{
  if (x > 0.0f)
    return float(std::sqrt(x));
  return 0.0f;
}

inline float FloatVector4::log2Fast(float x)
{
  return float(std::log2(x));
}

inline int FloatVector4::log2Int(int x)
{
  unsigned int  n = (unsigned int) x;
  int     r = int(bool(n & 0xFFFF0000U)) << 4;
  r = r | (int(bool(n & (0xFF00U << r))) << 3);
  r = r | (int(bool(n & (0xF0U << r))) << 2);
  r = r | (int(bool(n & (0x0CU << r))) << 1);
  r = r | int(bool(n & (0x02U << r)));
  return r;
}

inline float FloatVector4::polynomial3(const float *p, float x)
{
  return (((x * p[0] + p[1]) * x + p[2]) * x + p[3]);
}

inline float FloatVector4::exp2Fast(float x)
{
  return float(std::exp2(x));
}

inline FloatVector4& FloatVector4::normalize(bool invFlag)
{
  float   tmp = dotProduct(*this);
  if (tmp > 0.0f)
  {
    tmp = (!invFlag ? 1.0f : -1.0f) / float(std::sqrt(tmp));
    *this *= tmp;
  }
  return (*this);
}

inline FloatVector4& FloatVector4::normalize3Fast()
{
  v[3] = 1.0f / float(0x0000040000000000LL);
  float   tmp = dotProduct(*this);
  *this *= (1.0f / float(std::sqrt(tmp)));
  return (*this);
}

inline FloatVector4 FloatVector4::normalize3x3Fast(
    const FloatVector4& a, const FloatVector4& b, const FloatVector4& c)
{
  FloatVector4  tmp(a.dotProduct3(a), b.dotProduct3(b), c.dotProduct3(c), 0.0f);
  tmp.maxValues(FloatVector4(1.0f / (float(0x0000040000000000LL)
                                     * float(0x0000040000000000LL))));
  return tmp.rsqrtFast();
}

inline FloatVector4& FloatVector4::srgbExpand()
{
  *this *= (*this * float(0.13945550 / 65025.0) + float(0.86054450 / 255.0));
  *this *= *this;
  return (*this);
}

inline FloatVector4& FloatVector4::srgbCompress()
{
  minValues(FloatVector4(1.0f));
  FloatVector4  tmp(*this);
#if USE_PIXELFMT_RGB10A2
  // more accurate inverse function of srgbExpand() in 10 bits per channel mode
  FloatVector4  tmp2 =
      *this * float(0.03876962 * 255.0) + float(1.15864660 * 255.0);
#endif
  tmp.squareRootFast();
#if USE_PIXELFMT_RGB10A2
  *this = (*this * float(-0.19741622 * 255.0)) + (tmp * tmp2);
#else
  *this = (*this * float(-0.13942692 * 255.0))
          + (tmp * float(1.13942692 * 255.0));
#endif
  return (*this);
}

inline FloatVector4::operator std::uint32_t() const
{
  std::uint32_t c0 = floatToUInt8Clamped(v[0], 1.0f);
  std::uint32_t c1 = floatToUInt8Clamped(v[1], 1.0f);
  std::uint32_t c2 = floatToUInt8Clamped(v[2], 1.0f);
  std::uint32_t c3 = floatToUInt8Clamped(v[3], 1.0f);
  return (c0 | (c1 << 8) | (c2 << 16) | (c3 << 24));
}

inline std::uint32_t FloatVector4::convertToA2R10G10B10(bool noClamp) const
{
  FloatVector4  tmp(*this);
  if (!noClamp)
    tmp.minValues(FloatVector4(255.0f));
  tmp.maxValues(FloatVector4(0.0f));
  tmp *= FloatVector4(1023.0f / 255.0f, 1023.0f / 255.0f,
                      1023.0f / 255.0f, 3.0f / 255.0f);
  std::uint32_t r = std::uint32_t(roundFloat(tmp[0]));
  std::uint32_t g = std::uint32_t(roundFloat(tmp[1]));
  std::uint32_t b = std::uint32_t(roundFloat(tmp[2]));
  std::uint32_t a = std::uint32_t(roundFloat(tmp[3]));
  return ((r << 20) | (g << 10) | b | (a << 30));
}

#endif                                  // ENABLE_X86_64_AVX

inline FloatVector4::FloatVector4(unsigned int c0, unsigned int c1,
                                  unsigned int c2, unsigned int c3,
                                  float xf, float yf, bool isSRGB)
{
  FloatVector4  v0(c0);
  FloatVector4  v1(c1);
  FloatVector4  v2(c2);
  FloatVector4  v3(c3);
  if (isSRGB)
  {
    v0 *= v0;
    v1 *= v1;
    v2 *= v2;
    v3 *= v3;
  }
  v1 -= v0;
  v3 -= v2;
  v1 *= xf;
  v3 *= xf;
  v0 += v1;
  v0 += ((v2 + v3 - v0) * yf);
  if (isSRGB)
  {
    v0 *= (v0 * float(0.16916572 / (65025.0 * 65025.0))
           + float(0.83083428 / 65025.0));
  }
  *this = v0;
}

inline FloatVector4::FloatVector4(
    const std::uint32_t *p0, const std::uint32_t *p1,
    const std::uint32_t *p2, const std::uint32_t *p3,
    float xf, float yf, bool isSRGB)
{
  FloatVector4  v0(p0);
  FloatVector4  v1(p1);
  FloatVector4  v2(p2);
  FloatVector4  v3(p3);
  if (isSRGB)
  {
    v0 *= v0;
    v1 *= v1;
    v2 *= v2;
    v3 *= v3;
  }
  v1 -= v0;
  v3 -= v2;
  v1 *= xf;
  v3 *= xf;
  v0 += v1;
  v0 += ((v2 + v3 - v0) * yf);
  if (isSRGB)
  {
    v0 *= (v0 * float(0.16916572 / (65025.0 * 65025.0))
           + float(0.83083428 / 65025.0));
  }
  *this = v0;
}

inline FloatVector4::FloatVector4(
    const std::uint32_t *p1_0, const std::uint32_t *p2_0,
    const std::uint32_t *p1_1, const std::uint32_t *p2_1,
    const std::uint32_t *p1_2, const std::uint32_t *p2_2,
    const std::uint32_t *p1_3, const std::uint32_t *p2_3,
    float xf, float yf, bool isSRGB)
{
  FloatVector4  v0(p1_0, p2_0);
  FloatVector4  v1(p1_1, p2_1);
  FloatVector4  v2(p1_2, p2_2);
  FloatVector4  v3(p1_3, p2_3);
  if (isSRGB)
  {
    v0 *= v0;
    v1 *= v1;
    v2 *= v2;
    v3 *= v3;
  }
  v1 -= v0;
  v3 -= v2;
  v1 *= xf;
  v3 *= xf;
  v0 += v1;
  v0 += ((v2 + v3 - v0) * yf);
  if (isSRGB)
  {
    v0 *= (v0 * float(0.16916572 / (65025.0 * 65025.0))
           + float(0.83083428 / 65025.0));
  }
  *this = v0;
}

inline FloatVector4 FloatVector4::convertRGBA32(const std::uint32_t& c)
{
#if USE_PIXELFMT_RGB10A2
  return FloatVector4::convertA2R10G10B10(c);
#else
  return FloatVector4(&c);
#endif
}

inline std::uint32_t FloatVector4::convertToRGBA32(
    bool noAlpha, bool noClamp) const
{
#if USE_PIXELFMT_RGB10A2
  return (convertToA2R10G10B10(noClamp) | (!noAlpha ? 0U : 0xC0000000U));
#else
  (void) noClamp;
  return (std::uint32_t(*this) | (!noAlpha ? 0U : 0xFF000000U));
#endif
}

#endif

