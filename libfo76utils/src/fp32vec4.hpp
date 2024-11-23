
#ifndef FP32VEC4_HPP_INCLUDED
#define FP32VEC4_HPP_INCLUDED

#include "common.hpp"

#if ENABLE_X86_64_SIMD
typedef float   XMM_Float __attribute__ ((__vector_size__ (16)));
typedef double  XMM_Double __attribute__ ((__vector_size__ (16)));
typedef std::uint8_t  XMM_UInt8 __attribute__ ((__vector_size__ (16)));
typedef std::int8_t   XMM_Int8 __attribute__ ((__vector_size__ (16)));
typedef std::uint16_t XMM_UInt16 __attribute__ ((__vector_size__ (16)));
typedef std::int16_t  XMM_Int16 __attribute__ ((__vector_size__ (16)));
typedef std::uint32_t XMM_UInt32 __attribute__ ((__vector_size__ (16)));
typedef std::int32_t  XMM_Int32 __attribute__ ((__vector_size__ (16)));
typedef unsigned long long  XMM_UInt64 __attribute__ ((__vector_size__ (16)));
typedef long long     XMM_Int64 __attribute__ ((__vector_size__ (16)));
#  if ENABLE_X86_64_SIMD >= 2
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
#if ENABLE_X86_64_SIMD >= 2
 private:
  static constexpr float      floatMinVal = 5.16987883e-26f;
  static constexpr XMM_Float  floatMinValV =
  {
    floatMinVal, floatMinVal, floatMinVal, floatMinVal
  };
  typedef char  XMM_Char __attribute__ ((__vector_size__ (16)));
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
  static inline FloatVector4 convertInt16(const std::uint64_t& n);
  // if noInfNaN is true, Inf and NaN values are never returned
  static inline FloatVector4 convertFloat16(std::uint64_t n,
                                            bool noInfNaN = false);
  // v[N] is converted if mask bit N is set, otherwise the output for v[N]
  // is zero if mask < 15 or undetermined if mask > 15
  inline std::uint64_t convertToFloat16(unsigned int mask = 15U) const;
  inline void convertToFloats(float *p) const;
  // construct from 3 floats in p, v[3] = 0.0f
  static inline FloatVector4 convertVector3(const float *p);
  inline void convertToVector3(float *p) const;
  inline float& operator[](size_t n)
  {
#if !(ENABLE_X86_64_SIMD >= 2 && defined(__clang__))
    return v[n];
#else
    return (reinterpret_cast< float * >(&v))[n];
#endif
  }
  inline const float& operator[](size_t n) const
  {
#if !(ENABLE_X86_64_SIMD >= 2 && defined(__clang__))
    return v[n];
#else
    return (reinterpret_cast< const float * >(&v))[n];
#endif
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
  // v[n] = v[(mask >> (n * 2)) & 3]
  inline FloatVector4& shuffleValues(unsigned char mask);
  // v[n] = mask & (1 << n) ? r.v[n] : v[n]
  inline FloatVector4& blendValues(const FloatVector4& r, unsigned char mask);
  // v[n] = mask[n] < 0.0 ? r.v[n] : v[n]
  inline FloatVector4& blendValues(
      const FloatVector4& r, const FloatVector4& mask);
  inline FloatVector4& minValues(const FloatVector4& r);
  inline FloatVector4& maxValues(const FloatVector4& r);
  inline FloatVector4& floorValues();
  inline FloatVector4& ceilValues();
  inline FloatVector4& roundValues();
  inline FloatVector4& absValues();
  inline float dotProduct(const FloatVector4& r) const;
  // dot product of first three elements
  inline float dotProduct3(const FloatVector4& r) const;
  // dot product of first two elements
  inline float dotProduct2(const FloatVector4& r) const;
  inline FloatVector4 crossProduct3(const FloatVector4& r) const;
  inline FloatVector4& squareRoot();
  inline FloatVector4& squareRootFast();
  inline FloatVector4& rsqrtFast();     // elements must be positive
  inline FloatVector4& rcpFast();       // approximates 1.0 / v
  inline FloatVector4& rcpSqr();        // 1.0 / v²
  static inline float squareRootFast(float x);
  static inline float log2Fast(float x);
  inline FloatVector4& log2V();
  // ((x * p[0] + p[1]) * x + p[2]) * x + p[3]
  static inline float polynomial3(const float *p, float x);
  static inline float exp2Fast(float x);
  inline FloatVector4& exp2V();
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
  static inline FloatVector4 convertA2R10G10B10(const std::uint32_t& c,
                                                bool expandSRGB = false);
  // convert from the pixel format selected with USE_PIXELFMT_RGB10A2
  static inline FloatVector4 convertRGBA32(const std::uint32_t& c,
                                           bool expandSRGB = false);
  inline std::uint32_t convertToA2R10G10B10(bool noClamp = false) const;
  // convert to the pixel format selected with USE_PIXELFMT_RGB10A2
  inline std::uint32_t convertToRGBA32(bool noAlpha = false,
                                       bool noClamp = false) const;
  // encode a three-dimensional unit vector (Z <= 0) as a 32-bit integer
  inline std::uint32_t normalToUInt32() const;
  // decode the output of normalToUInt32()
  static inline FloatVector4 uint32ToNormal(const std::uint32_t& n);
  // encode unit vector as a 32-bit integer in X10Y10Z10 format
  inline std::uint32_t convertToX10Y10Z10() const;
  // decode the output of convertToX10Y10Z10()
  static inline FloatVector4 convertX10Y10Z10(const std::uint32_t& n);
  // encode vector as a 32-bit integer in X10Y10Z10W2 format
  inline std::uint32_t convertToX10Y10Z10W2() const;
  // decode the output of convertToX10Y10Z10W2()
  static inline FloatVector4 convertX10Y10Z10W2(const std::uint32_t& n);
  inline std::uint32_t getSignMask() const;
  // decode/encode color in R9G9B9E5_SHAREDEXP format
  static inline FloatVector4 convertR9G9B9E5(const std::uint32_t& n);
  inline std::uint32_t convertToR9G9B9E5() const;
};

#if ENABLE_X86_64_SIMD < 2
#  include "fp32vec4_base.hpp"
#elif defined(__clang__)
#  include "fp32vec4_clang.hpp"
#else
#  include "fp32vec4_gcc.hpp"
#endif

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

inline FloatVector4 FloatVector4::convertRGBA32(
    const std::uint32_t& c, bool expandSRGB)
{
#if USE_PIXELFMT_RGB10A2
  return FloatVector4::convertA2R10G10B10(c, expandSRGB);
#else
  if (expandSRGB)
    return FloatVector4(&c).srgbExpand();
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

