
#ifndef FP32VEC8_HPP_INCLUDED
#define FP32VEC8_HPP_INCLUDED

#include "fp32vec4.hpp"

struct FloatVector8
{
#if ENABLE_X86_64_SIMD >= 2
 private:
  static constexpr float      floatMinVal = 5.16987883e-26f;
  static constexpr YMM_Float  floatMinValV =
  {
    floatMinVal, floatMinVal, floatMinVal, floatMinVal,
    floatMinVal, floatMinVal, floatMinVal, floatMinVal
  };
 public:
  YMM_Float v;
  inline FloatVector8(const YMM_Float& r)
    : v(r)
  {
  }
#else
  float   v[8];
#endif
  inline FloatVector8()
  {
  }
  inline FloatVector8(const float& x);
  // construct from 8 packed 8-bit integers
  inline FloatVector8(const std::uint64_t *p);
  // construct from a pair of FloatVector4
  inline FloatVector8(const FloatVector4 *p);
  inline FloatVector8(FloatVector4 v0, FloatVector4 v1);
  inline FloatVector8(float v0, float v1, float v2, float v3,
                      float v4, float v5, float v6, float v7);
  inline FloatVector8(const float *p);
  inline FloatVector8(const std::int16_t *p);
  inline FloatVector8(const std::int32_t *p);
  // construct from 8 half precision floats
  // if noInfNaN is true, Inf and NaN values are never created
  inline FloatVector8(const std::uint16_t *p, bool noInfNaN);
  // store as 8 floats (does not require p to be aligned to 32 bytes)
  inline void convertToFloats(float *p) const;
  inline void convertToFloatVector4(FloatVector4 *p) const;
  inline void convertToInt16(std::int16_t *p) const;
  inline void convertToInt32(std::int32_t *p) const;
  inline void convertToFloat16(std::uint16_t *p) const;
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
  inline FloatVector8& operator+=(const FloatVector8& r);
  inline FloatVector8& operator-=(const FloatVector8& r);
  inline FloatVector8& operator*=(const FloatVector8& r);
  inline FloatVector8& operator/=(const FloatVector8& r);
  inline FloatVector8 operator+(const FloatVector8& r) const;
  inline FloatVector8 operator-(const FloatVector8& r) const;
  inline FloatVector8 operator*(const FloatVector8& r) const;
  inline FloatVector8 operator/(const FloatVector8& r) const;
  inline FloatVector8& operator+=(float r);
  inline FloatVector8& operator-=(float r);
  inline FloatVector8& operator*=(float r);
  inline FloatVector8& operator/=(float r);
  inline FloatVector8 operator+(const float& r) const;
  inline FloatVector8 operator-(const float& r) const;
  inline FloatVector8 operator*(const float& r) const;
  inline FloatVector8 operator/(const float& r) const;
  // v[n] = v[((mask >> ((n & 3) * 2)) & 3) | (n & 4)]
  inline FloatVector8& shuffleValues(unsigned char mask);
  // v[n] = mask & (1 << n) ? r.v[n] : v[n]
  inline FloatVector8& blendValues(const FloatVector8& r, unsigned char mask);
  // v[n] = mask[n] < 0.0 ? r.v[n] : v[n]
  inline FloatVector8& blendValues(
      const FloatVector8& r, const FloatVector8& mask);
  inline FloatVector8& minValues(const FloatVector8& r);
  inline FloatVector8& maxValues(const FloatVector8& r);
  inline FloatVector8& floorValues();
  inline FloatVector8& ceilValues();
  inline FloatVector8& roundValues();
  inline FloatVector8& absValues();
  inline float dotProduct(const FloatVector8& r) const;
  inline FloatVector8& squareRoot();
  inline FloatVector8& squareRootFast();
  inline FloatVector8& rsqrtFast();     // elements must be positive
  inline FloatVector8& rcpFast();       // approximates 1.0 / v
  inline FloatVector8& rcpSqr();        // 1.0 / v²
  inline FloatVector8& log2V();
  inline FloatVector8& exp2V();
  // convert to 8 packed 8-bit integers
  inline operator std::uint64_t() const;
  inline std::uint32_t getSignMask() const;
};

#if ENABLE_X86_64_SIMD < 2
#  include "fp32vec8_base.hpp"
#elif defined(__clang__)
#  include "fp32vec8_clang.hpp"
#else
#  include "fp32vec8_gcc.hpp"
#endif

#endif

