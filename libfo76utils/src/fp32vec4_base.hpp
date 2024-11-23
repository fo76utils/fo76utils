
#ifndef FP32VEC4_BASE_HPP_INCLUDED
#define FP32VEC4_BASE_HPP_INCLUDED

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
  FloatVector4  tmp;
  tmp.v[0] = p[0];
  tmp.v[1] = p[1];
  tmp.v[2] = p[2];
  tmp.v[3] = p[3];
  *this = tmp;
}

inline FloatVector4 FloatVector4::convertInt16(const std::uint64_t& n)
{
  return FloatVector4(float(std::int16_t(n & 0xFFFFU)),
                      float(std::int16_t((n >> 16) & 0xFFFFU)),
                      float(std::int16_t((n >> 32) & 0xFFFFU)),
                      float(std::int16_t((n >> 48) & 0xFFFFU)));
}

inline FloatVector4 FloatVector4::convertFloat16(std::uint64_t n, bool noInfNaN)
{
  (void) noInfNaN;
  return FloatVector4(::convertFloat16(std::uint16_t(n & 0xFFFFU)),
                      ::convertFloat16(std::uint16_t((n >> 16) & 0xFFFFU)),
                      ::convertFloat16(std::uint16_t((n >> 32) & 0xFFFFU)),
                      ::convertFloat16(std::uint16_t((n >> 48) & 0xFFFFU)));
}

inline std::uint64_t FloatVector4::convertToFloat16(unsigned int mask) const
{
  std::uint64_t r = 0U;
  if (mask & 1U)
    r = ::convertToFloat16(v[0]);
  if (mask & 2U)
    r = r | (std::uint64_t(::convertToFloat16(v[1])) << 16);
  if (mask & 4U)
    r = r | (std::uint64_t(::convertToFloat16(v[2])) << 32);
  if (mask & 8U)
    r = r | (std::uint64_t(::convertToFloat16(v[3])) << 48);
  return r;
}

inline void FloatVector4::convertToFloats(float *p) const
{
  FloatVector4  tmp(*this);
  p[0] = tmp.v[0];
  p[1] = tmp.v[1];
  p[2] = tmp.v[2];
  p[3] = tmp.v[3];
}

inline FloatVector4 FloatVector4::convertVector3(const float *p)
{
  return FloatVector4(p[0], p[1], p[2], 0.0f);
}

inline void FloatVector4::convertToVector3(float *p) const
{
  FloatVector4  tmp(*this);
  p[0] = tmp.v[0];
  p[1] = tmp.v[1];
  p[2] = tmp.v[2];
}

inline FloatVector4& FloatVector4::operator+=(const FloatVector4& r)
{
  FloatVector4  tmp(r);
  v[0] += tmp.v[0];
  v[1] += tmp.v[1];
  v[2] += tmp.v[2];
  v[3] += tmp.v[3];
  return (*this);
}

inline FloatVector4& FloatVector4::operator-=(const FloatVector4& r)
{
  FloatVector4  tmp(r);
  v[0] -= tmp.v[0];
  v[1] -= tmp.v[1];
  v[2] -= tmp.v[2];
  v[3] -= tmp.v[3];
  return (*this);
}

inline FloatVector4& FloatVector4::operator*=(const FloatVector4& r)
{
  FloatVector4  tmp(r);
  v[0] *= tmp.v[0];
  v[1] *= tmp.v[1];
  v[2] *= tmp.v[2];
  v[3] *= tmp.v[3];
  return (*this);
}

inline FloatVector4& FloatVector4::operator/=(const FloatVector4& r)
{
  FloatVector4  tmp(r);
  v[0] /= tmp.v[0];
  v[1] /= tmp.v[1];
  v[2] /= tmp.v[2];
  v[3] /= tmp.v[3];
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

inline FloatVector4& FloatVector4::shuffleValues(unsigned char mask)
{
  FloatVector4  tmp(*this);
  v[0] = tmp.v[mask & 3];
  v[1] = tmp.v[(mask >> 2) & 3];
  v[2] = tmp.v[(mask >> 4) & 3];
  v[3] = tmp.v[(mask >> 6) & 3];
  return (*this);
}

inline FloatVector4& FloatVector4::blendValues(
    const FloatVector4& r, unsigned char mask)
{
  FloatVector4  tmp(r);
  v[0] = (!(mask & 0x01) ? v[0] : tmp.v[0]);
  v[1] = (!(mask & 0x02) ? v[1] : tmp.v[1]);
  v[2] = (!(mask & 0x04) ? v[2] : tmp.v[2]);
  v[3] = (!(mask & 0x08) ? v[3] : tmp.v[3]);
  return (*this);
}

inline FloatVector4& FloatVector4::blendValues(
    const FloatVector4& r, const FloatVector4& mask)
{
  FloatVector4  tmp(r);
  FloatVector4  tmp2(mask);
  v[0] = (tmp2[0] < 0.0f ? tmp.v[0] : v[0]);
  v[1] = (tmp2[1] < 0.0f ? tmp.v[1] : v[1]);
  v[2] = (tmp2[2] < 0.0f ? tmp.v[2] : v[2]);
  v[3] = (tmp2[3] < 0.0f ? tmp.v[3] : v[3]);
  return (*this);
}

inline FloatVector4& FloatVector4::minValues(const FloatVector4& r)
{
  FloatVector4  tmp(r);
  v[0] = (v[0] < tmp.v[0] ? v[0] : tmp.v[0]);
  v[1] = (v[1] < tmp.v[1] ? v[1] : tmp.v[1]);
  v[2] = (v[2] < tmp.v[2] ? v[2] : tmp.v[2]);
  v[3] = (v[3] < tmp.v[3] ? v[3] : tmp.v[3]);
  return (*this);
}

inline FloatVector4& FloatVector4::maxValues(const FloatVector4& r)
{
  FloatVector4  tmp(r);
  v[0] = (v[0] > tmp.v[0] ? v[0] : tmp.v[0]);
  v[1] = (v[1] > tmp.v[1] ? v[1] : tmp.v[1]);
  v[2] = (v[2] > tmp.v[2] ? v[2] : tmp.v[2]);
  v[3] = (v[3] > tmp.v[3] ? v[3] : tmp.v[3]);
  return (*this);
}

inline FloatVector4& FloatVector4::floorValues()
{
  v[0] = float(std::floor(v[0]));
  v[1] = float(std::floor(v[1]));
  v[2] = float(std::floor(v[2]));
  v[3] = float(std::floor(v[3]));
  return (*this);
}

inline FloatVector4& FloatVector4::ceilValues()
{
  v[0] = float(std::ceil(v[0]));
  v[1] = float(std::ceil(v[1]));
  v[2] = float(std::ceil(v[2]));
  v[3] = float(std::ceil(v[3]));
  return (*this);
}

inline FloatVector4& FloatVector4::roundValues()
{
  v[0] = float(std::round(v[0]));
  v[1] = float(std::round(v[1]));
  v[2] = float(std::round(v[2]));
  v[3] = float(std::round(v[3]));
  return (*this);
}

inline FloatVector4& FloatVector4::absValues()
{
  v[0] = float(std::fabs(v[0]));
  v[1] = float(std::fabs(v[1]));
  v[2] = float(std::fabs(v[2]));
  v[3] = float(std::fabs(v[3]));
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

inline FloatVector4 FloatVector4::crossProduct3(const FloatVector4& r) const
{
  return FloatVector4((v[1] * r.v[2]) - (v[2] * r.v[1]),
                      (v[2] * r.v[0]) - (v[0] * r.v[2]),
                      (v[0] * r.v[1]) - (v[1] * r.v[0]), 0.0f);
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

inline FloatVector4& FloatVector4::rcpFast()
{
  v[0] = 1.0f / v[0];
  v[1] = 1.0f / v[1];
  v[2] = 1.0f / v[2];
  v[3] = 1.0f / v[3];
  return (*this);
}

inline FloatVector4& FloatVector4::rcpSqr()
{
  v[0] = 1.0f / (v[0] * v[0]);
  v[1] = 1.0f / (v[1] * v[1]);
  v[2] = 1.0f / (v[2] * v[2]);
  v[3] = 1.0f / (v[3] * v[3]);
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

inline FloatVector4& FloatVector4::log2V()
{
  v[0] = float(std::log2(v[0]));
  v[1] = float(std::log2(v[1]));
  v[2] = float(std::log2(v[2]));
  v[3] = float(std::log2(v[3]));
  return (*this);
}

inline float FloatVector4::polynomial3(const float *p, float x)
{
  return (((x * p[0] + p[1]) * x + p[2]) * x + p[3]);
}

inline float FloatVector4::exp2Fast(float x)
{
  return float(std::exp2(x));
}

inline FloatVector4& FloatVector4::exp2V()
{
  v[0] = float(std::exp2(v[0]));
  v[1] = float(std::exp2(v[1]));
  v[2] = float(std::exp2(v[2]));
  v[3] = float(std::exp2(v[3]));
  return (*this);
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
  // more accurate inverse function of srgbExpand() in 10 bits per channel mode
  FloatVector4  tmp2 =
      *this * float(0.03876962 * 255.0) + float(1.15864660 * 255.0);
  tmp.squareRootFast();
  *this = (*this * float(-0.19741622 * 255.0)) + (tmp * tmp2);
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

inline FloatVector4 FloatVector4::convertA2R10G10B10(
    const std::uint32_t& c, bool expandSRGB)
{
  float   r = float(int((c >> 20) & 0x03FF));
  float   g = float(int((c >> 10) & 0x03FF));
  float   b = float(int(c & 0x03FF));
  float   a = float(int((c >> 30) & 0x0003));
  FloatVector4  tmp(r, g, b, a);
  tmp *= FloatVector4(255.0f / 1023.0f, 255.0f / 1023.0f,
                      255.0f / 1023.0f, 255.0f / 3.0f);
  if (expandSRGB)
    tmp.srgbExpand();
  return tmp;
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

inline std::uint32_t FloatVector4::normalToUInt32() const
{
  int     x, y;
  x = roundFloat(std::min(std::max(v[0], -1.0f), 1.0f) * 32767.0f);
  y = roundFloat(std::min(std::max(v[1], -1.0f), 1.0f) * 32767.0f);
  if (v[2] > 0.0f) [[unlikely]]
  {
    x = -x;
    y = -y;
  }
  return (std::uint32_t(x & 0xFFFF) | (std::uint32_t(y & 0xFFFF) << 16));
}

inline FloatVector4 FloatVector4::uint32ToNormal(const std::uint32_t& n)
{
  float   x = float(std::int16_t(n & 0xFFFFU));
  float   y = float(std::int16_t(n >> 16));
  float   z = -(float(std::sqrt(std::max((32767.0f * 32767.0f)
                                         - ((x * x) + (y * y)), 0.0f))));
  return FloatVector4(x, y, z, 0.0f).normalize3Fast();
}

inline std::uint32_t FloatVector4::convertToX10Y10Z10() const
{
  FloatVector4  n(*this);
  n = n * 511.5f + 511.5f;
  n.maxValues(FloatVector4(0.0f)).minValues(FloatVector4(1023.0f));
  std::uint32_t tmp0 = std::uint32_t(roundFloat(n[0]));
  std::uint32_t tmp1 = std::uint32_t(roundFloat(n[1]));
  std::uint32_t tmp2 = std::uint32_t(roundFloat(n[2]));
  return (tmp0 | (tmp1 << 10) | (tmp2 << 20));
}

inline FloatVector4 FloatVector4::convertX10Y10Z10(const std::uint32_t& n)
{
  FloatVector4  tmp;
  tmp[0] = float(int(n & 0x000003FFU)) * float(2.0 / 1023.0) - 1.0f;
  tmp[1] = float(int(n & 0x000FFC00U)) * float(2.0 / 1047552.0) - 1.0f;
  tmp[2] = float(int(n & 0x3FF00000U)) * float(2.0 / 1072693248.0) - 1.0f;
  tmp[3] = 0.0f;
  return tmp;
}

inline std::uint32_t FloatVector4::convertToX10Y10Z10W2() const
{
  FloatVector4  n(*this);
  n = n * FloatVector4(511.5f, 511.5f, 511.5f, 1.5f)
      + FloatVector4(511.5f, 511.5f, 511.5f, 1.5f);
  n.maxValues(FloatVector4(0.0f));
  n.minValues(FloatVector4(1023.0f, 1023.0f, 1023.0f, 3.0f));
  std::uint32_t tmp0 = std::uint32_t(roundFloat(n[0]));
  std::uint32_t tmp1 = std::uint32_t(roundFloat(n[1]));
  std::uint32_t tmp2 = std::uint32_t(roundFloat(n[2]));
  std::uint32_t tmp3 = std::uint32_t(roundFloat(n[3]));
  return (tmp0 | (tmp1 << 10) | (tmp2 << 20) | (tmp3 << 30));
}

inline FloatVector4 FloatVector4::convertX10Y10Z10W2(const std::uint32_t& n)
{
  FloatVector4  tmp;
  tmp[0] = float(int(n & 0x000003FFU)) * float(2.0 / 1023.0) - 1.0f;
  tmp[1] = float(int(n & 0x000FFC00U)) * float(2.0 / 1047552.0) - 1.0f;
  tmp[2] = float(int(n & 0x3FF00000U)) * float(2.0 / 1072693248.0) - 1.0f;
  tmp[3] = float(int(n >> 30)) * float(2.0 / 3.0) - 1.0f;
  return tmp;
}

inline std::uint32_t FloatVector4::getSignMask() const
{
  std::uint32_t tmp;
  tmp = std::uint32_t(v[0] < 0.0f) | (std::uint32_t(v[1] < 0.0f) << 1)
        | (std::uint32_t(v[2] < 0.0f) << 2) | (std::uint32_t(v[3] < 0.0f) << 3);
  return tmp;
}

inline FloatVector4 FloatVector4::convertR9G9B9E5(const std::uint32_t& n)
{
  FloatVector4  tmp;
  int     i = int(n >> 27) & 0x1F;
  float   e = (i < 31 ? float(1 << i) : float(2147483648.0));
  tmp[0] = float(int(n & 0x000001FFU)) * e * float(1.0 / 16777216.0);
  tmp[1] = float(int(n & 0x0003FE00U)) * e * float(1.0 / 8589934592.0);
  tmp[2] = float(int(n & 0x07FC0000U)) * e * float(1.0 / 4398046511104.0);
  tmp[3] = 1.0f;
  return tmp;
}

inline std::uint32_t FloatVector4::convertToR9G9B9E5() const
{
  FloatVector4  c(*this);
  c.maxValues(FloatVector4(0.0f));
  float   maxVal = std::max(c[0], c[1]);
  maxVal = std::max(maxVal, c[2]);
  int     e;
  if (maxVal >= (1.0f / 32768.0f) && maxVal < 32768.0f)
    e = int(float(std::log2(maxVal))) + 16;
  else if (!(maxVal >= (1.0f / 32768.0f)))
    e = 0;
  else
    e = 31;
  if (e < 25)
    c *= float(1 << (24 - e));
  else
    c /= float(1 << (e - 24));
  c.minValues(FloatVector4(511.0f));
  std::uint32_t r = std::uint32_t(roundFloat(c[0]));
  std::uint32_t g = std::uint32_t(roundFloat(c[1]));
  std::uint32_t b = std::uint32_t(roundFloat(c[2]));
  return (r | (g << 9) | (b << 18) | (std::uint32_t(e) << 27));
}

#endif

