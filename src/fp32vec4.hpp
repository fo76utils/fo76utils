
#ifndef FP32VEC4_HPP_INCLUDED
#define FP32VEC4_HPP_INCLUDED

#include "common.hpp"

struct FloatVector4
{
#if defined(__GNUC__) && (defined(__x86_64__) || defined(__x86_64))
 private:
  static constexpr float  floatMinVal = 5.16987883e-26f;
 public:
  float   v __attribute__ ((__vector_size__ (16)));
  inline FloatVector4(unsigned int c)
  {
    float   tmp __attribute__ ((__vector_size__ (16)));
    __asm__ ("vmovd %1, %0" : "=x" (tmp) : "rm" (c));
    __asm__ ("vpmovzxbd %0, %0" : "+x" (tmp));
    __asm__ ("vcvtdq2ps %1, %0" : "=x" (v) : "x" (tmp));
  }
  inline FloatVector4(float x)
    : v { x, x, x, x }
  {
  }
  inline FloatVector4(const std::uint32_t *p)
  {
    float   tmp __attribute__ ((__vector_size__ (16)));
    __asm__ ("vpmovzxbd %1, %0" : "=x" (tmp) : "m" (*p));
    __asm__ ("vcvtdq2ps %1, %0" : "=x" (v) : "x" (tmp));
  }
  // the first two elements are taken from p1, and the third and fourth from p2
  inline FloatVector4(const std::uint32_t *p1, const std::uint32_t *p2)
  {
    float   tmp1 __attribute__ ((__vector_size__ (16)));
    float   tmp2 __attribute__ ((__vector_size__ (16)));
    __asm__ ("vpmovzxbd %1, %0" : "=x" (tmp1) : "m" (*p1));
    __asm__ ("vpmovzxbd %1, %0" : "=x" (tmp2) : "m" (*p2));
    __asm__ ("vpunpcklqdq %1, %0, %0" : "+x" (tmp1) : "x" (tmp2));
    __asm__ ("vcvtdq2ps %1, %0" : "=x" (v) : "x" (tmp1));
  }
  inline FloatVector4(float v0, float v1, float v2, float v3)
    : v { v0, v1, v2, v3 }
  {
  }
  static inline FloatVector4 convertFloat16(std::uint64_t n)
  {
    float   v __attribute__ ((__vector_size__ (16)));
    std::int32_t  tmp1 __attribute__ ((__vector_size__ (16)));
    std::int32_t  tmp2 __attribute__ ((__vector_size__ (16))) =
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
    return FloatVector4(v[0], v[1], v[2], v[3]);
  }
  inline float& operator[](size_t n)
  {
    return v[n];
  }
  inline const float& operator[](size_t n) const
  {
    return v[n];
  }
  inline FloatVector4& operator+=(const FloatVector4& r)
  {
    v += r.v;
    return (*this);
  }
  inline FloatVector4& operator-=(const FloatVector4& r)
  {
    v -= r.v;
    return (*this);
  }
  inline FloatVector4& operator*=(const FloatVector4& r)
  {
    v *= r.v;
    return (*this);
  }
  inline FloatVector4& operator/=(const FloatVector4& r)
  {
    v /= r.v;
    return (*this);
  }
  inline FloatVector4 operator+(const FloatVector4& r) const
  {
    FloatVector4  tmp(*this);
    return (tmp += r);
  }
  inline FloatVector4 operator-(const FloatVector4& r) const
  {
    FloatVector4  tmp(*this);
    return (tmp -= r);
  }
  inline FloatVector4 operator*(const FloatVector4& r) const
  {
    FloatVector4  tmp(*this);
    return (tmp *= r);
  }
  inline FloatVector4 operator/(const FloatVector4& r) const
  {
    FloatVector4  tmp(*this);
    return (tmp /= r);
  }
  inline FloatVector4& operator+=(float r)
  {
    v += r;
    return (*this);
  }
  inline FloatVector4& operator-=(float r)
  {
    v -= r;
    return (*this);
  }
  inline FloatVector4& operator*=(float r)
  {
    v *= r;
    return (*this);
  }
  inline FloatVector4& operator/=(float r)
  {
    v /= r;
    return (*this);
  }
  inline float dotProduct(const FloatVector4& r) const
  {
    float   tmp1 __attribute__ ((__vector_size__ (16)));
    __asm__ ("vmulps %2, %1, %0" : "=x" (tmp1) : "x" (v), "xm" (r.v));
    float   tmp2 __attribute__ ((__vector_size__ (16)));
    __asm__ ("vshufps $0xb1, %1, %1, %0" : "=x" (tmp2) : "x" (tmp1));
    __asm__ ("vaddps %1, %0, %0" : "+x" (tmp1) : "x" (tmp2));
    __asm__ ("vshufps $0x4e, %1, %1, %0" : "=x" (tmp2) : "x" (tmp1));
    __asm__ ("vaddps %1, %0, %0" : "+x" (tmp1) : "x" (tmp2));
    return tmp1[0];
  }
  inline float dotProduct3(const FloatVector4& r) const
  {
    float   tmp1 __attribute__ ((__vector_size__ (16)));
    __asm__ ("vmulps %2, %1, %0" : "=x" (tmp1) : "x" (v), "xm" (r.v));
    float   tmp2 __attribute__ ((__vector_size__ (16)));
    __asm__ ("vshufps $0xe1, %1, %1, %0" : "=x" (tmp2) : "x" (tmp1));
    __asm__ ("vaddps %1, %0, %0" : "+x" (tmp1) : "x" (tmp2));
    __asm__ ("vshufps $0x4e, %0, %0, %0" : "+x" (tmp2));
    __asm__ ("vaddps %1, %0, %0" : "+x" (tmp1) : "x" (tmp2));
    return tmp1[0];
  }
  inline float dotProduct2(const FloatVector4& r) const
  {
    float   tmp1 __attribute__ ((__vector_size__ (16)));
    __asm__ ("vmulps %2, %1, %0" : "=x" (tmp1) : "x" (v), "xm" (r.v));
    float   tmp2 __attribute__ ((__vector_size__ (16)));
    __asm__ ("vshufps $0xb1, %1, %1, %0" : "=x" (tmp2) : "x" (tmp1));
    __asm__ ("vaddps %1, %0, %0" : "+x" (tmp1) : "x" (tmp2));
    return tmp1[0];
  }
  inline FloatVector4& squareRoot()
  {
    float   tmp __attribute__ ((__vector_size__ (16))) =
    {
      0.0f, 0.0f, 0.0f, 0.0f
    };
    __asm__ ("vmaxps %1, %0, %0" : "+x" (tmp) : "x" (v));
    __asm__ ("vsqrtps %1, %0" : "=x" (v) : "x" (tmp));
    return (*this);
  }
  inline FloatVector4& squareRootFast()
  {
    float   tmp1 __attribute__ ((__vector_size__ (16))) = v;
    const float tmp2 __attribute__ ((__vector_size__ (16))) =
    {
      floatMinVal, floatMinVal, floatMinVal, floatMinVal
    };
    __asm__ ("vmaxps %1, %0, %0" : "+x" (tmp1) : "xm" (tmp2));
    __asm__ ("vrsqrtps %1, %0" : "=x" (v) : "x" (tmp1));
    v *= tmp1;
    return (*this);
  }
  static inline float squareRootFast(float x)
  {
    float   tmp1 = x;
    const float tmp2 = floatMinVal;
    __asm__ ("vmaxss %1, %0, %0" : "+x" (tmp1) : "xm" (tmp2));
    float   tmp3;
    __asm__ ("vrsqrtss %1, %1, %0" : "=x" (tmp3) : "x" (tmp1));
    return (tmp1 * tmp3);
  }
  static inline float log2Fast(float x)
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
  static inline int log2Int(int x)
  {
    int     tmp = x;            // return 0 if x == 0
    __asm__ ("bsr %0, %0" : "+r" (tmp) : : "cc");
    return tmp;
  }
  static inline float exp2Fast(float x)
  {
    float   e = float(std::floor(x));
    float   m = x - e;
    __asm__ ("vcvtps2dq %0, %0" : "+x" (e));
    __asm__ ("vpslld $0x17, %0, %0" : "+x" (e));
    float   m2 __attribute__ ((__vector_size__ (16)));
    float   m3 __attribute__ ((__vector_size__ (16)));
    // m2 = { m, m, 1, 1 }
    __asm__ ("vshufps $0x00, %2, %1, %0" : "=x" (m2) : "x" (m), "xm" (1.0f));
    // m3 = { m, 1, m, 1 }
    __asm__ ("vshufps $0x88, %1, %1, %0" : "=x" (m3) : "x" (m2));
    m3 *= m2;
    m3 *= m2;
    m = FloatVector4(m3[0], m3[1], m3[2], m3[3]).dotProduct(
            FloatVector4(0.00825060f, 0.05924474f, 0.34671664f, 1.0f));
    m = m * m;
    __asm__ ("vpaddd %1, %0, %0" : "+x" (m) : "x" (e));
    return m;
  }
  inline FloatVector4& normalize(bool invFlag = false)
  {
    static const float  invMultTable[2] = { -0.5f, 0.5f };
    float   tmp = dotProduct(*this);
    float   tmp2 = floatMinVal;
    __asm__ ("vmaxss %0, %1, %0" : "+x" (tmp2) : "x" (tmp));
    __asm__ ("vrsqrtss %0, %0, %0" : "+x" (tmp2));
    v *= ((tmp * tmp2 * tmp2 - 3.0f) * (tmp2 * invMultTable[int(invFlag)]));
    return (*this);
  }
  inline FloatVector4& normalizeFast()
  {
    float   tmp = dotProduct(*this);
    const float tmp2 = floatMinVal;
    __asm__ ("vmaxss %1, %0, %0" : "+x" (tmp) : "xm" (tmp2));
    __asm__ ("vrsqrtss %0, %0, %0" : "+x" (tmp));
    v *= tmp;
    return (*this);
  }
  inline FloatVector4& normalize3Fast()
  {
    float   tmp __attribute__ ((__vector_size__ (16))) = v;
    tmp[3] = 1.0f / float(0x0000040000000000LL);
    float   tmp2 __attribute__ ((__vector_size__ (16))) = tmp * tmp;
    float   tmp3 __attribute__ ((__vector_size__ (16)));
    __asm__ ("vshufps $0xb1, %1, %1, %0" : "=x" (tmp3) : "x" (tmp2));
    __asm__ ("vaddps %1, %0, %0" : "+x" (tmp2) : "x" (tmp3));
    __asm__ ("vshufps $0x4e, %1, %1, %0" : "=x" (tmp3) : "x" (tmp2));
    __asm__ ("vaddps %1, %0, %0" : "+x" (tmp2) : "x" (tmp3));
    __asm__ ("vrsqrtps %0, %0" : "+x" (tmp2));
    v = tmp * tmp2;
    return (*this);
  }
  inline FloatVector4& minValues(const FloatVector4& r)
  {
    __asm__ ("vminps %1, %0, %0" : "+x" (v) : "xm" (r.v));
    return (*this);
  }
  inline FloatVector4& maxValues(const FloatVector4& r)
  {
    __asm__ ("vmaxps %1, %0, %0" : "+x" (v) : "xm" (r.v));
    return (*this);
  }
  // 0.0 - 255.0 sRGB -> 0.0 - 1.0 linear
  inline FloatVector4& srgbExpand()
  {
    v *= (v * float(0.13945550 / 65025.0) + float(0.86054450 / 255.0));
    v *= v;
    return (*this);
  }
  // 0.0 - 1.0 linear -> 0.0 - 255.0 sRGB
  inline FloatVector4& srgbCompress()
  {
    minValues(FloatVector4(1.0f));
    maxValues(FloatVector4(1.0f / float(0x40000000)));
    float   tmp __attribute__ ((__vector_size__ (16)));
    __asm__ ("vrsqrtps %1, %0" : "=x" (tmp) : "x" (v));
    v *= (float(-0.13942692 * 255.0) + (tmp * float(1.13942692 * 255.0)));
    return (*this);
  }
  inline operator std::uint32_t() const
  {
    float   tmp __attribute__ ((__vector_size__ (16))) = v;
    std::uint32_t c;
    __asm__ ("vcvtps2dq %0, %0" : "+x" (tmp));
    __asm__ ("vpackssdw %0, %0, %0" : "+x" (tmp));
    __asm__ ("vpackuswb %0, %0, %0" : "+x" (tmp));
    __asm__ ("vmovd %1, %0" : "=r" (c) : "x" (tmp));
    return c;
  }
#else
  float   v[4];
  inline FloatVector4(unsigned int c)
  {
    v[0] = float(int(c & 0xFF));
    v[1] = float(int((c >> 8) & 0xFF));
    v[2] = float(int((c >> 16) & 0xFF));
    v[3] = float(int((c >> 24) & 0xFF));
  }
  inline FloatVector4(float x)
  {
    v[0] = x;
    v[1] = x;
    v[2] = x;
    v[3] = x;
  }
  inline FloatVector4(const std::uint32_t *p)
  {
    std::uint32_t c = *p;
    v[0] = float(int(c & 0xFF));
    v[1] = float(int((c >> 8) & 0xFF));
    v[2] = float(int((c >> 16) & 0xFF));
    v[3] = float(int((c >> 24) & 0xFF));
  }
  inline FloatVector4(const std::uint32_t *p1, const std::uint32_t *p2)
  {
    std::uint32_t c1 = *p1;
    std::uint32_t c2 = *p2;
    v[0] = float(int(c1 & 0xFF));
    v[1] = float(int((c1 >> 8) & 0xFF));
    v[2] = float(int(c2 & 0xFF));
    v[3] = float(int((c2 >> 8) & 0xFF));
  }
  inline FloatVector4(float v0, float v1, float v2, float v3)
  {
    v[0] = v0;
    v[1] = v1;
    v[2] = v2;
    v[3] = v3;
  }
  static inline FloatVector4 convertFloat16(std::uint64_t n)
  {
    return FloatVector4(::convertFloat16(std::uint16_t(n & 0xFFFFU)),
                        ::convertFloat16(std::uint16_t((n >> 16) & 0xFFFFU)),
                        ::convertFloat16(std::uint16_t((n >> 32) & 0xFFFFU)),
                        ::convertFloat16(std::uint16_t((n >> 48) & 0xFFFFU)));
  }
  inline float& operator[](size_t n)
  {
    return v[n];
  }
  inline const float& operator[](size_t n) const
  {
    return v[n];
  }
  inline FloatVector4& operator+=(const FloatVector4& r)
  {
    v[0] = v[0] + r.v[0];
    v[1] = v[1] + r.v[1];
    v[2] = v[2] + r.v[2];
    v[3] = v[3] + r.v[3];
    return (*this);
  }
  inline FloatVector4& operator-=(const FloatVector4& r)
  {
    v[0] = v[0] - r.v[0];
    v[1] = v[1] - r.v[1];
    v[2] = v[2] - r.v[2];
    v[3] = v[3] - r.v[3];
    return (*this);
  }
  inline FloatVector4& operator*=(const FloatVector4& r)
  {
    v[0] = v[0] * r.v[0];
    v[1] = v[1] * r.v[1];
    v[2] = v[2] * r.v[2];
    v[3] = v[3] * r.v[3];
    return (*this);
  }
  inline FloatVector4& operator/=(const FloatVector4& r)
  {
    v[0] = v[0] / r.v[0];
    v[1] = v[1] / r.v[1];
    v[2] = v[2] / r.v[2];
    v[3] = v[3] / r.v[3];
    return (*this);
  }
  inline FloatVector4 operator+(const FloatVector4& r) const
  {
    return FloatVector4(v[0] + r.v[0], v[1] + r.v[1],
                        v[2] + r.v[2], v[3] + r.v[3]);
  }
  inline FloatVector4 operator-(const FloatVector4& r) const
  {
    return FloatVector4(v[0] - r.v[0], v[1] - r.v[1],
                        v[2] - r.v[2], v[3] - r.v[3]);
  }
  inline FloatVector4 operator*(const FloatVector4& r) const
  {
    return FloatVector4(v[0] * r.v[0], v[1] * r.v[1],
                        v[2] * r.v[2], v[3] * r.v[3]);
  }
  inline FloatVector4 operator/(const FloatVector4& r) const
  {
    return FloatVector4(v[0] / r.v[0], v[1] / r.v[1],
                        v[2] / r.v[2], v[3] / r.v[3]);
  }
  inline FloatVector4& operator+=(float r)
  {
    v[0] = v[0] + r;
    v[1] = v[1] + r;
    v[2] = v[2] + r;
    v[3] = v[3] + r;
    return (*this);
  }
  inline FloatVector4& operator-=(float r)
  {
    v[0] = v[0] - r;
    v[1] = v[1] - r;
    v[2] = v[2] - r;
    v[3] = v[3] - r;
    return (*this);
  }
  inline FloatVector4& operator*=(float r)
  {
    v[0] = v[0] * r;
    v[1] = v[1] * r;
    v[2] = v[2] * r;
    v[3] = v[3] * r;
    return (*this);
  }
  inline FloatVector4& operator/=(float r)
  {
    v[0] = v[0] / r;
    v[1] = v[1] / r;
    v[2] = v[2] / r;
    v[3] = v[3] / r;
    return (*this);
  }
  inline float dotProduct(const FloatVector4& r) const
  {
    return ((v[0] * r.v[0]) + (v[1] * r.v[1]) + (v[2] * r.v[2])
            + (v[3] * r.v[3]));
  }
  inline float dotProduct3(const FloatVector4& r) const
  {
    return ((v[0] * r.v[0]) + (v[1] * r.v[1]) + (v[2] * r.v[2]));
  }
  inline float dotProduct2(const FloatVector4& r) const
  {
    return ((v[0] * r.v[0]) + (v[1] * r.v[1]));
  }
  inline FloatVector4& squareRoot()
  {
    v[0] = (v[0] > 0.0f ? float(std::sqrt(v[0])) : 0.0f);
    v[1] = (v[1] > 0.0f ? float(std::sqrt(v[1])) : 0.0f);
    v[2] = (v[2] > 0.0f ? float(std::sqrt(v[2])) : 0.0f);
    v[3] = (v[3] > 0.0f ? float(std::sqrt(v[3])) : 0.0f);
    return (*this);
  }
  inline FloatVector4& squareRootFast()
  {
    v[0] = (v[0] > 0.0f ? float(std::sqrt(v[0])) : 0.0f);
    v[1] = (v[1] > 0.0f ? float(std::sqrt(v[1])) : 0.0f);
    v[2] = (v[2] > 0.0f ? float(std::sqrt(v[2])) : 0.0f);
    v[3] = (v[3] > 0.0f ? float(std::sqrt(v[3])) : 0.0f);
    return (*this);
  }
  static inline float squareRootFast(float x)
  {
    if (x > 0.0f)
      return float(std::sqrt(x));
    return 0.0f;
  }
  static inline float log2Fast(float x)
  {
    return float(std::log2(x));
  }
  static inline int log2Int(int x)
  {
    unsigned int  n = (unsigned int) x;
    int     r = int(bool(n & 0xFFFF0000U)) << 4;
    r = r | (int(bool(n & (0xFF00U << r))) << 3);
    r = r | (int(bool(n & (0xF0U << r))) << 2);
    r = r | (int(bool(n & (0x0CU << r))) << 1);
    r = r | int(bool(n & (0x02U << r)));
    return r;
  }
  static inline float exp2Fast(float x)
  {
    return float(std::exp2(x));
  }
  inline FloatVector4& normalize(bool invFlag = false)
  {
    float   tmp = dotProduct(*this);
    if (tmp > 0.0f)
    {
      tmp = (!invFlag ? 1.0f : -1.0f) / float(std::sqrt(tmp));
      *this *= tmp;
    }
    return (*this);
  }
  inline FloatVector4& normalizeFast()
  {
    float   tmp = dotProduct(*this);
    if (tmp > 0.0f)
      *this *= (1.0f / float(std::sqrt(tmp)));
    return (*this);
  }
  inline FloatVector4& normalize3Fast()
  {
    v[3] = 1.0f / float(0x0000040000000000LL);
    float   tmp = dotProduct(*this);
    *this *= (1.0f / float(std::sqrt(tmp)));
    return (*this);
  }
  inline FloatVector4& minValues(const FloatVector4& r)
  {
    v[0] = (r.v[0] < v[0] ? r.v[0] : v[0]);
    v[1] = (r.v[1] < v[1] ? r.v[1] : v[1]);
    v[2] = (r.v[2] < v[2] ? r.v[2] : v[2]);
    v[3] = (r.v[3] < v[3] ? r.v[3] : v[3]);
    return (*this);
  }
  inline FloatVector4& maxValues(const FloatVector4& r)
  {
    v[0] = (r.v[0] > v[0] ? r.v[0] : v[0]);
    v[1] = (r.v[1] > v[1] ? r.v[1] : v[1]);
    v[2] = (r.v[2] > v[2] ? r.v[2] : v[2]);
    v[3] = (r.v[3] > v[3] ? r.v[3] : v[3]);
    return (*this);
  }
  inline FloatVector4& srgbExpand()
  {
    *this *= (*this * FloatVector4(float(0.13945550 / 65025.0))
              + FloatVector4(float(0.86054450 / 255.0)));
    *this *= *this;
    return (*this);
  }
  inline FloatVector4& srgbCompress()
  {
    minValues(FloatVector4(1.0f));
    FloatVector4  tmp(*this);
    tmp.squareRootFast();
    *this = (*this * FloatVector4(float(-0.13942692 * 255.0)))
            + (tmp * FloatVector4(float(1.13942692 * 255.0)));
    return (*this);
  }
  inline operator std::uint32_t() const
  {
    int     c0 = roundFloat(v[0]);
    int     c1 = roundFloat(v[1]);
    int     c2 = roundFloat(v[2]);
    int     c3 = roundFloat(v[3]);
    c0 = (c0 > 0 ? (c0 < 255 ? c0 : 255) : 0);
    c1 = (c1 > 0 ? (c1 < 255 ? c1 : 255) : 0);
    c2 = (c2 > 0 ? (c2 < 255 ? c2 : 255) : 0);
    c3 = (c3 > 0 ? (c3 < 255 ? c3 : 255) : 0);
    return ((std::uint32_t) c0 | ((std::uint32_t) c1 << 8)
            | ((std::uint32_t) c2 << 16) | ((std::uint32_t) c3 << 24));
  }
#endif
  inline FloatVector4(unsigned int c0, unsigned int c1,
                      unsigned int c2, unsigned int c3,
                      float xf, float yf, bool isSRGB = false)
  {
    // bilinear filtering
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
    v0 += ((v2 + v3 - v0) * FloatVector4(yf));
    if (isSRGB)
    {
      v0 *= (v0 * FloatVector4(float(0.16916572 / (65025.0 * 65025.0)))
             + FloatVector4(float(0.83083428 / 65025.0)));
    }
    *this = v0;
  }
  inline FloatVector4(const std::uint32_t *p0, const std::uint32_t *p1,
                      const std::uint32_t *p2, const std::uint32_t *p3,
                      float xf, float yf, bool isSRGB = false)
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
    v0 += ((v2 + v3 - v0) * FloatVector4(yf));
    if (isSRGB)
    {
      v0 *= (v0 * FloatVector4(float(0.16916572 / (65025.0 * 65025.0)))
             + FloatVector4(float(0.83083428 / 65025.0)));
    }
    *this = v0;
  }
  inline FloatVector4(const std::uint32_t *p1_0, const std::uint32_t *p2_0,
                      const std::uint32_t *p1_1, const std::uint32_t *p2_1,
                      const std::uint32_t *p1_2, const std::uint32_t *p2_2,
                      const std::uint32_t *p1_3, const std::uint32_t *p2_3,
                      float xf, float yf, bool isSRGB = false)
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
    v0 += ((v2 + v3 - v0) * FloatVector4(yf));
    if (isSRGB)
    {
      v0 *= (v0 * FloatVector4(float(0.16916572 / (65025.0 * 65025.0)))
             + FloatVector4(float(0.83083428 / 65025.0)));
    }
    *this = v0;
  }
};

#endif

