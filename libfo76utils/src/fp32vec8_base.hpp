
#ifndef FP32VEC8_BASE_HPP_INCLUDED
#define FP32VEC8_BASE_HPP_INCLUDED

inline FloatVector8::FloatVector8(const float& x)
{
  v[0] = x;
  v[1] = x;
  v[2] = x;
  v[3] = x;
  v[4] = x;
  v[5] = x;
  v[6] = x;
  v[7] = x;
}

inline FloatVector8::FloatVector8(const std::uint64_t *p)
{
  std::uint64_t tmp = *p;
  v[0] = float(int(tmp & 0xFF));
  v[1] = float(int((tmp >> 8) & 0xFF));
  v[2] = float(int((tmp >> 16) & 0xFF));
  v[3] = float(int((tmp >> 24) & 0xFF));
  v[4] = float(int((tmp >> 32) & 0xFF));
  v[5] = float(int((tmp >> 40) & 0xFF));
  v[6] = float(int((tmp >> 48) & 0xFF));
  v[7] = float(int((tmp >> 56) & 0xFF));
}

inline FloatVector8::FloatVector8(const FloatVector4 *p)
{
  v[0] = p[0][0];
  v[1] = p[0][1];
  v[2] = p[0][2];
  v[3] = p[0][3];
  v[4] = p[1][0];
  v[5] = p[1][1];
  v[6] = p[1][2];
  v[7] = p[1][3];
}

inline FloatVector8::FloatVector8(FloatVector4 v0, FloatVector4 v1)
{
  v[0] = v0[0];
  v[1] = v0[1];
  v[2] = v0[2];
  v[3] = v0[3];
  v[4] = v1[0];
  v[5] = v1[1];
  v[6] = v1[2];
  v[7] = v1[3];
}

inline FloatVector8::FloatVector8(float v0, float v1, float v2, float v3,
                                  float v4, float v5, float v6, float v7)
{
  v[0] = v0;
  v[1] = v1;
  v[2] = v2;
  v[3] = v3;
  v[4] = v4;
  v[5] = v5;
  v[6] = v6;
  v[7] = v7;
}

inline FloatVector8::FloatVector8(const float *p)
{
  FloatVector8  tmp;
  tmp.v[0] = p[0];
  tmp.v[1] = p[1];
  tmp.v[2] = p[2];
  tmp.v[3] = p[3];
  tmp.v[4] = p[4];
  tmp.v[5] = p[5];
  tmp.v[6] = p[6];
  tmp.v[7] = p[7];
  *this = tmp;
}

inline FloatVector8::FloatVector8(const std::int16_t *p)
{
  v[0] = float(p[0]);
  v[1] = float(p[1]);
  v[2] = float(p[2]);
  v[3] = float(p[3]);
  v[4] = float(p[4]);
  v[5] = float(p[5]);
  v[6] = float(p[6]);
  v[7] = float(p[7]);
}

inline FloatVector8::FloatVector8(const std::int32_t *p)
{
  v[0] = float(p[0]);
  v[1] = float(p[1]);
  v[2] = float(p[2]);
  v[3] = float(p[3]);
  v[4] = float(p[4]);
  v[5] = float(p[5]);
  v[6] = float(p[6]);
  v[7] = float(p[7]);
}

inline FloatVector8::FloatVector8(const std::uint16_t *p, bool noInfNaN)
{
  (void) noInfNaN;
  v[0] = ::convertFloat16(p[0]);
  v[1] = ::convertFloat16(p[1]);
  v[2] = ::convertFloat16(p[2]);
  v[3] = ::convertFloat16(p[3]);
  v[4] = ::convertFloat16(p[4]);
  v[5] = ::convertFloat16(p[5]);
  v[6] = ::convertFloat16(p[6]);
  v[7] = ::convertFloat16(p[7]);
}

inline void FloatVector8::convertToFloats(float *p) const
{
  FloatVector8  tmp(*this);
  p[0] = tmp.v[0];
  p[1] = tmp.v[1];
  p[2] = tmp.v[2];
  p[3] = tmp.v[3];
  p[4] = tmp.v[4];
  p[5] = tmp.v[5];
  p[6] = tmp.v[6];
  p[7] = tmp.v[7];
}

inline void FloatVector8::convertToFloatVector4(FloatVector4 *p) const
{
  p[0][0] = v[0];
  p[0][1] = v[1];
  p[0][2] = v[2];
  p[0][3] = v[3];
  p[1][0] = v[4];
  p[1][1] = v[5];
  p[1][2] = v[6];
  p[1][3] = v[7];
}

inline void FloatVector8::convertToInt16(std::int16_t *p) const
{
  p[0] = std::int16_t(std::min< int >(std::max< int >(roundFloat(v[0]), -32768),
                                      32767));
  p[1] = std::int16_t(std::min< int >(std::max< int >(roundFloat(v[1]), -32768),
                                      32767));
  p[2] = std::int16_t(std::min< int >(std::max< int >(roundFloat(v[2]), -32768),
                                      32767));
  p[3] = std::int16_t(std::min< int >(std::max< int >(roundFloat(v[3]), -32768),
                                      32767));
  p[4] = std::int16_t(std::min< int >(std::max< int >(roundFloat(v[4]), -32768),
                                      32767));
  p[5] = std::int16_t(std::min< int >(std::max< int >(roundFloat(v[5]), -32768),
                                      32767));
  p[6] = std::int16_t(std::min< int >(std::max< int >(roundFloat(v[6]), -32768),
                                      32767));
  p[7] = std::int16_t(std::min< int >(std::max< int >(roundFloat(v[7]), -32768),
                                      32767));
}

inline void FloatVector8::convertToInt32(std::int32_t *p) const
{
  p[0] = std::int32_t(roundFloat(v[0]));
  p[1] = std::int32_t(roundFloat(v[1]));
  p[2] = std::int32_t(roundFloat(v[2]));
  p[3] = std::int32_t(roundFloat(v[3]));
  p[4] = std::int32_t(roundFloat(v[4]));
  p[5] = std::int32_t(roundFloat(v[5]));
  p[6] = std::int32_t(roundFloat(v[6]));
  p[7] = std::int32_t(roundFloat(v[7]));
}

inline void FloatVector8::convertToFloat16(std::uint16_t *p) const
{
  p[0] = ::convertToFloat16(v[0]);
  p[1] = ::convertToFloat16(v[1]);
  p[2] = ::convertToFloat16(v[2]);
  p[3] = ::convertToFloat16(v[3]);
  p[4] = ::convertToFloat16(v[4]);
  p[5] = ::convertToFloat16(v[5]);
  p[6] = ::convertToFloat16(v[6]);
  p[7] = ::convertToFloat16(v[7]);
}

inline FloatVector8& FloatVector8::operator+=(const FloatVector8& r)
{
  FloatVector8  tmp(r);
  v[0] += tmp.v[0];
  v[1] += tmp.v[1];
  v[2] += tmp.v[2];
  v[3] += tmp.v[3];
  v[4] += tmp.v[4];
  v[5] += tmp.v[5];
  v[6] += tmp.v[6];
  v[7] += tmp.v[7];
  return (*this);
}

inline FloatVector8& FloatVector8::operator-=(const FloatVector8& r)
{
  FloatVector8  tmp(r);
  v[0] -= tmp.v[0];
  v[1] -= tmp.v[1];
  v[2] -= tmp.v[2];
  v[3] -= tmp.v[3];
  v[4] -= tmp.v[4];
  v[5] -= tmp.v[5];
  v[6] -= tmp.v[6];
  v[7] -= tmp.v[7];
  return (*this);
}

inline FloatVector8& FloatVector8::operator*=(const FloatVector8& r)
{
  FloatVector8  tmp(r);
  v[0] *= tmp.v[0];
  v[1] *= tmp.v[1];
  v[2] *= tmp.v[2];
  v[3] *= tmp.v[3];
  v[4] *= tmp.v[4];
  v[5] *= tmp.v[5];
  v[6] *= tmp.v[6];
  v[7] *= tmp.v[7];
  return (*this);
}

inline FloatVector8& FloatVector8::operator/=(const FloatVector8& r)
{
  FloatVector8  tmp(r);
  v[0] /= tmp.v[0];
  v[1] /= tmp.v[1];
  v[2] /= tmp.v[2];
  v[3] /= tmp.v[3];
  v[4] /= tmp.v[4];
  v[5] /= tmp.v[5];
  v[6] /= tmp.v[6];
  v[7] /= tmp.v[7];
  return (*this);
}

inline FloatVector8 FloatVector8::operator+(const FloatVector8& r) const
{
  return FloatVector8(v[0] + r.v[0], v[1] + r.v[1],
                      v[2] + r.v[2], v[3] + r.v[3],
                      v[4] + r.v[4], v[5] + r.v[5],
                      v[6] + r.v[6], v[7] + r.v[7]);
}

inline FloatVector8 FloatVector8::operator-(const FloatVector8& r) const
{
  return FloatVector8(v[0] - r.v[0], v[1] - r.v[1],
                      v[2] - r.v[2], v[3] - r.v[3],
                      v[4] - r.v[4], v[5] - r.v[5],
                      v[6] - r.v[6], v[7] - r.v[7]);
}

inline FloatVector8 FloatVector8::operator*(const FloatVector8& r) const
{
  return FloatVector8(v[0] * r.v[0], v[1] * r.v[1],
                      v[2] * r.v[2], v[3] * r.v[3],
                      v[4] * r.v[4], v[5] * r.v[5],
                      v[6] * r.v[6], v[7] * r.v[7]);
}

inline FloatVector8 FloatVector8::operator/(const FloatVector8& r) const
{
  return FloatVector8(v[0] / r.v[0], v[1] / r.v[1],
                      v[2] / r.v[2], v[3] / r.v[3],
                      v[4] / r.v[4], v[5] / r.v[5],
                      v[6] / r.v[6], v[7] / r.v[7]);
}

inline FloatVector8& FloatVector8::operator+=(float r)
{
  v[0] += r;
  v[1] += r;
  v[2] += r;
  v[3] += r;
  v[4] += r;
  v[5] += r;
  v[6] += r;
  v[7] += r;
  return (*this);
}

inline FloatVector8& FloatVector8::operator-=(float r)
{
  v[0] -= r;
  v[1] -= r;
  v[2] -= r;
  v[3] -= r;
  v[4] -= r;
  v[5] -= r;
  v[6] -= r;
  v[7] -= r;
  return (*this);
}

inline FloatVector8& FloatVector8::operator*=(float r)
{
  v[0] *= r;
  v[1] *= r;
  v[2] *= r;
  v[3] *= r;
  v[4] *= r;
  v[5] *= r;
  v[6] *= r;
  v[7] *= r;
  return (*this);
}

inline FloatVector8& FloatVector8::operator/=(float r)
{
  v[0] /= r;
  v[1] /= r;
  v[2] /= r;
  v[3] /= r;
  v[4] /= r;
  v[5] /= r;
  v[6] /= r;
  v[7] /= r;
  return (*this);
}

inline FloatVector8 FloatVector8::operator+(const float& r) const
{
  return FloatVector8(v[0] + r, v[1] + r, v[2] + r, v[3] + r,
                      v[4] + r, v[5] + r, v[6] + r, v[7] + r);
}

inline FloatVector8 FloatVector8::operator-(const float& r) const
{
  return FloatVector8(v[0] - r, v[1] - r, v[2] - r, v[3] - r,
                      v[4] - r, v[5] - r, v[6] - r, v[7] - r);
}

inline FloatVector8 FloatVector8::operator*(const float& r) const
{
  return FloatVector8(v[0] * r, v[1] * r, v[2] * r, v[3] * r,
                      v[4] * r, v[5] * r, v[6] * r, v[7] * r);
}

inline FloatVector8 FloatVector8::operator/(const float& r) const
{
  return FloatVector8(v[0] / r, v[1] / r, v[2] / r, v[3] / r,
                      v[4] / r, v[5] / r, v[6] / r, v[7] / r);
}

inline FloatVector8& FloatVector8::shuffleValues(unsigned char mask)
{
  FloatVector8  tmp(*this);
  v[0] = tmp.v[mask & 3];
  v[1] = tmp.v[(mask >> 2) & 3];
  v[2] = tmp.v[(mask >> 4) & 3];
  v[3] = tmp.v[(mask >> 6) & 3];
  v[4] = tmp.v[(mask & 3) | 4];
  v[5] = tmp.v[((mask >> 2) & 3) | 4];
  v[6] = tmp.v[((mask >> 4) & 3) | 4];
  v[7] = tmp.v[((mask >> 6) & 3) | 4];
  return (*this);
}

inline FloatVector8& FloatVector8::blendValues(
    const FloatVector8& r, unsigned char mask)
{
  FloatVector8  tmp(r);
  v[0] = (!(mask & 0x01) ? v[0] : tmp.v[0]);
  v[1] = (!(mask & 0x02) ? v[1] : tmp.v[1]);
  v[2] = (!(mask & 0x04) ? v[2] : tmp.v[2]);
  v[3] = (!(mask & 0x08) ? v[3] : tmp.v[3]);
  v[4] = (!(mask & 0x10) ? v[4] : tmp.v[4]);
  v[5] = (!(mask & 0x20) ? v[5] : tmp.v[5]);
  v[6] = (!(mask & 0x40) ? v[6] : tmp.v[6]);
  v[7] = (!(mask & 0x80) ? v[7] : tmp.v[7]);
  return (*this);
}

inline FloatVector8& FloatVector8::blendValues(
    const FloatVector8& r, const FloatVector8& mask)
{
  FloatVector8  tmp(r);
  FloatVector8  tmp2(mask);
  v[0] = (tmp2[0] < 0.0f ? tmp.v[0] : v[0]);
  v[1] = (tmp2[1] < 0.0f ? tmp.v[1] : v[1]);
  v[2] = (tmp2[2] < 0.0f ? tmp.v[2] : v[2]);
  v[3] = (tmp2[3] < 0.0f ? tmp.v[3] : v[3]);
  v[4] = (tmp2[4] < 0.0f ? tmp.v[4] : v[4]);
  v[5] = (tmp2[5] < 0.0f ? tmp.v[5] : v[5]);
  v[6] = (tmp2[6] < 0.0f ? tmp.v[6] : v[6]);
  v[7] = (tmp2[7] < 0.0f ? tmp.v[7] : v[7]);
  return (*this);
}

inline FloatVector8& FloatVector8::minValues(const FloatVector8& r)
{
  FloatVector8  tmp(r);
  v[0] = (v[0] < tmp.v[0] ? v[0] : tmp.v[0]);
  v[1] = (v[1] < tmp.v[1] ? v[1] : tmp.v[1]);
  v[2] = (v[2] < tmp.v[2] ? v[2] : tmp.v[2]);
  v[3] = (v[3] < tmp.v[3] ? v[3] : tmp.v[3]);
  v[4] = (v[4] < tmp.v[4] ? v[4] : tmp.v[4]);
  v[5] = (v[5] < tmp.v[5] ? v[5] : tmp.v[5]);
  v[6] = (v[6] < tmp.v[6] ? v[6] : tmp.v[6]);
  v[7] = (v[7] < tmp.v[7] ? v[7] : tmp.v[7]);
  return (*this);
}

inline FloatVector8& FloatVector8::maxValues(const FloatVector8& r)
{
  FloatVector8  tmp(r);
  v[0] = (v[0] > tmp.v[0] ? v[0] : tmp.v[0]);
  v[1] = (v[1] > tmp.v[1] ? v[1] : tmp.v[1]);
  v[2] = (v[2] > tmp.v[2] ? v[2] : tmp.v[2]);
  v[3] = (v[3] > tmp.v[3] ? v[3] : tmp.v[3]);
  v[4] = (v[4] > tmp.v[4] ? v[4] : tmp.v[4]);
  v[5] = (v[5] > tmp.v[5] ? v[5] : tmp.v[5]);
  v[6] = (v[6] > tmp.v[6] ? v[6] : tmp.v[6]);
  v[7] = (v[7] > tmp.v[7] ? v[7] : tmp.v[7]);
  return (*this);
}

inline FloatVector8& FloatVector8::floorValues()
{
  v[0] = float(std::floor(v[0]));
  v[1] = float(std::floor(v[1]));
  v[2] = float(std::floor(v[2]));
  v[3] = float(std::floor(v[3]));
  v[4] = float(std::floor(v[4]));
  v[5] = float(std::floor(v[5]));
  v[6] = float(std::floor(v[6]));
  v[7] = float(std::floor(v[7]));
  return (*this);
}

inline FloatVector8& FloatVector8::ceilValues()
{
  v[0] = float(std::ceil(v[0]));
  v[1] = float(std::ceil(v[1]));
  v[2] = float(std::ceil(v[2]));
  v[3] = float(std::ceil(v[3]));
  v[4] = float(std::ceil(v[4]));
  v[5] = float(std::ceil(v[5]));
  v[6] = float(std::ceil(v[6]));
  v[7] = float(std::ceil(v[7]));
  return (*this);
}

inline FloatVector8& FloatVector8::roundValues()
{
  v[0] = float(std::round(v[0]));
  v[1] = float(std::round(v[1]));
  v[2] = float(std::round(v[2]));
  v[3] = float(std::round(v[3]));
  v[4] = float(std::round(v[4]));
  v[5] = float(std::round(v[5]));
  v[6] = float(std::round(v[6]));
  v[7] = float(std::round(v[7]));
  return (*this);
}

inline FloatVector8& FloatVector8::absValues()
{
  v[0] = float(std::fabs(v[0]));
  v[1] = float(std::fabs(v[1]));
  v[2] = float(std::fabs(v[2]));
  v[3] = float(std::fabs(v[3]));
  v[4] = float(std::fabs(v[4]));
  v[5] = float(std::fabs(v[5]));
  v[6] = float(std::fabs(v[6]));
  v[7] = float(std::fabs(v[7]));
  return (*this);
}

inline float FloatVector8::dotProduct(const FloatVector8& r) const
{
  FloatVector8  tmp(*this);
  tmp *= r;
  return (tmp[0] + tmp[1] + tmp[2] + tmp[3]
          + tmp[4] + tmp[5] + tmp[6] + tmp[7]);
}

inline FloatVector8& FloatVector8::squareRoot()
{
  maxValues(FloatVector8(0.0f));
  v[0] = float(std::sqrt(v[0]));
  v[1] = float(std::sqrt(v[1]));
  v[2] = float(std::sqrt(v[2]));
  v[3] = float(std::sqrt(v[3]));
  v[4] = float(std::sqrt(v[4]));
  v[5] = float(std::sqrt(v[5]));
  v[6] = float(std::sqrt(v[6]));
  v[7] = float(std::sqrt(v[7]));
  return (*this);
}

inline FloatVector8& FloatVector8::squareRootFast()
{
  maxValues(FloatVector8(0.0f));
  v[0] = float(std::sqrt(v[0]));
  v[1] = float(std::sqrt(v[1]));
  v[2] = float(std::sqrt(v[2]));
  v[3] = float(std::sqrt(v[3]));
  v[4] = float(std::sqrt(v[4]));
  v[5] = float(std::sqrt(v[5]));
  v[6] = float(std::sqrt(v[6]));
  v[7] = float(std::sqrt(v[7]));
  return (*this);
}

inline FloatVector8& FloatVector8::rsqrtFast()
{
  v[0] = 1.0f / float(std::sqrt(v[0]));
  v[1] = 1.0f / float(std::sqrt(v[1]));
  v[2] = 1.0f / float(std::sqrt(v[2]));
  v[3] = 1.0f / float(std::sqrt(v[3]));
  v[4] = 1.0f / float(std::sqrt(v[4]));
  v[5] = 1.0f / float(std::sqrt(v[5]));
  v[6] = 1.0f / float(std::sqrt(v[6]));
  v[7] = 1.0f / float(std::sqrt(v[7]));
  return (*this);
}

inline FloatVector8& FloatVector8::rcpFast()
{
  v[0] = 1.0f / v[0];
  v[1] = 1.0f / v[1];
  v[2] = 1.0f / v[2];
  v[3] = 1.0f / v[3];
  v[4] = 1.0f / v[4];
  v[5] = 1.0f / v[5];
  v[6] = 1.0f / v[6];
  v[7] = 1.0f / v[7];
  return (*this);
}

inline FloatVector8& FloatVector8::rcpSqr()
{
  v[0] = 1.0f / (v[0] * v[0]);
  v[1] = 1.0f / (v[1] * v[1]);
  v[2] = 1.0f / (v[2] * v[2]);
  v[3] = 1.0f / (v[3] * v[3]);
  v[4] = 1.0f / (v[4] * v[4]);
  v[5] = 1.0f / (v[5] * v[5]);
  v[6] = 1.0f / (v[6] * v[6]);
  v[7] = 1.0f / (v[7] * v[7]);
  return (*this);
}

inline FloatVector8& FloatVector8::log2V()
{
  v[0] = float(std::log2(v[0]));
  v[1] = float(std::log2(v[1]));
  v[2] = float(std::log2(v[2]));
  v[3] = float(std::log2(v[3]));
  v[4] = float(std::log2(v[4]));
  v[5] = float(std::log2(v[5]));
  v[6] = float(std::log2(v[6]));
  v[7] = float(std::log2(v[7]));
  return (*this);
}

inline FloatVector8& FloatVector8::exp2V()
{
  v[0] = float(std::exp2(v[0]));
  v[1] = float(std::exp2(v[1]));
  v[2] = float(std::exp2(v[2]));
  v[3] = float(std::exp2(v[3]));
  v[4] = float(std::exp2(v[4]));
  v[5] = float(std::exp2(v[5]));
  v[6] = float(std::exp2(v[6]));
  v[7] = float(std::exp2(v[7]));
  return (*this);
}

inline FloatVector8::operator std::uint64_t() const
{
  std::uint64_t c0 = floatToUInt8Clamped(v[0], 1.0f);
  std::uint64_t c1 = floatToUInt8Clamped(v[1], 1.0f);
  std::uint64_t c2 = floatToUInt8Clamped(v[2], 1.0f);
  std::uint64_t c3 = floatToUInt8Clamped(v[3], 1.0f);
  std::uint64_t c4 = floatToUInt8Clamped(v[4], 1.0f);
  std::uint64_t c5 = floatToUInt8Clamped(v[5], 1.0f);
  std::uint64_t c6 = floatToUInt8Clamped(v[6], 1.0f);
  std::uint64_t c7 = floatToUInt8Clamped(v[7], 1.0f);
  return (c0 | (c1 << 8) | (c2 << 16) | (c3 << 24)
          | (c4 << 32) | (c5 << 40) | (c6 << 48) | (c7 << 56));
}

inline std::uint32_t FloatVector8::getSignMask() const
{
  std::uint32_t tmp;
  tmp = std::uint32_t(v[0] < 0.0f) | (std::uint32_t(v[1] < 0.0f) << 1)
        | (std::uint32_t(v[2] < 0.0f) << 2) | (std::uint32_t(v[3] < 0.0f) << 3)
        | (std::uint32_t(v[4] < 0.0f) << 4) | (std::uint32_t(v[5] < 0.0f) << 5)
        | (std::uint32_t(v[6] < 0.0f) << 6) | (std::uint32_t(v[7] < 0.0f) << 7);
  return tmp;
}

#endif

