
#ifndef FP32VEC4_CLANG_HPP_INCLUDED
#define FP32VEC4_CLANG_HPP_INCLUDED

inline FloatVector4::FloatVector4(unsigned int c)
{
  XMM_Int32 tmp;
  __asm__ ("vpmovzxbd %1, %0" : "=x" (tmp) : "rm" (c));
  __asm__ ("vcvtdq2ps %1, %0" : "=x" (v) : "x" (tmp));
}

inline FloatVector4::FloatVector4(float x)
  : v { x, x, x, x }
{
}

inline FloatVector4::FloatVector4(const std::uint32_t *p)
{
  XMM_Int32 tmp;
  __asm__ ("vpmovzxbd %1, %0" : "=x" (tmp) : "m" (*p));
  __asm__ ("vcvtdq2ps %1, %0" : "=x" (v) : "x" (tmp));
}

inline FloatVector4::FloatVector4(const std::uint32_t *p1,
                                  const std::uint32_t *p2)
{
  XMM_Int32 tmp1, tmp2;
  __asm__ ("vpmovzxbd %1, %0" : "=x" (tmp1) : "m" (*p1));
  __asm__ ("vpmovzxbd %1, %0" : "=x" (tmp2) : "m" (*p2));
  XMM_Int32 tmp3 = { tmp1[0], tmp1[1], tmp2[0], tmp2[1] };
  __asm__ ("vcvtdq2ps %1, %0" : "=x" (v) : "x" (tmp3));
}

inline FloatVector4::FloatVector4(float v0, float v1, float v2, float v3)
  : v { v0, v1, v2, v3 }
{
}

inline FloatVector4::FloatVector4(const float *p)
  : v { p[0], p[1], p[2], p[3] }
{
}

inline FloatVector4 FloatVector4::convertInt16(const std::uint64_t& n)
{
  XMM_Float v;
  __asm__ ("vpmovsxwd %1, %0" : "=x" (v) : "xm" (n));
  __asm__ ("vcvtdq2ps %0, %0" : "+x" (v));
  return FloatVector4(v);
}

inline FloatVector4 FloatVector4::convertFloat16(
    std::uint64_t n, [[maybe_unused]] bool noInfNaN)
{
  XMM_Float v;
  XMM_UInt64  tmp = { n, 0U };
#if ENABLE_X86_64_SIMD >= 3
  if (noInfNaN)
  {
    const XMM_UInt64  expMaskTbl =
    {
      0x7C007C007C007C00ULL, 0ULL
    };
    XMM_UInt64  tmp2 = tmp & expMaskTbl;
    tmp2 = std::bit_cast< XMM_UInt64 >(
               std::bit_cast< XMM_UInt16 >(tmp2)
               == std::bit_cast< XMM_UInt16 >(expMaskTbl));
    tmp = tmp & ~tmp2;
  }
  v = __builtin_ia32_vcvtph2ps(std::bit_cast< XMM_Int16 >(tmp));
#else
  XMM_Int32 tmp2 =
  {
    std::int32_t(0x8FFFE000U), 0x38000000, std::int32_t(0xB8000000U), 0
  };
  XMM_Int32 tmp3;
  __asm__ ("vpmovsxwd %1, %0" : "=x" (tmp3) : "x" (tmp));
  tmp3 = (tmp3 << 13) & __builtin_ia32_pshufd(tmp2, 0x00);
  tmp3 += __builtin_ia32_pshufd(tmp2, 0x55);
  v = std::bit_cast< XMM_Float >(__builtin_ia32_pshufd(tmp2, 0xAA) & tmp3);
  v = std::bit_cast< XMM_Float >(tmp3) - v;
  v = v + v;
  __asm__ ("vpminud %1, %0, %0" : "+x" (v) : "x" (tmp3));
#endif
  return FloatVector4(v);
}

inline std::uint64_t FloatVector4::convertToFloat16(unsigned int mask) const
{
#if ENABLE_X86_64_SIMD >= 3
  XMM_UInt64  tmp;
  if (mask < 15U)
  {
    __asm__ ("vmovd %1, %0" : "=x" (tmp) : "r" (mask * 0x10204080U));
    __asm__ ("vpmovsxbd %0, %0" : "+x" (tmp));
    __asm__ ("vpsrad $0x1f, %0, %0" : "+x" (tmp));
    __asm__ ("vpand %1, %0, %0" : "+x" (tmp) : "xm" (v));
    __asm__ ("vcvtps2ph $0x00, %0, %0" : "+x" (tmp));
  }
  else
  {
    __asm__ ("vcvtps2ph $0x00, %1, %0" : "=x" (tmp) : "x" (v));
  }
  return tmp[0];
#else
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
#endif
}

inline void FloatVector4::convertToFloats(float *p) const
{
  __asm__ ("vmovups %1, %0" : "=m" (*p) : "x" (v));
}

inline FloatVector4 FloatVector4::convertVector3(const float *p)
{
  XMM_Float tmp;
  __asm__ ("vmovq %1, %0" : "=x" (tmp) : "m" (*p));
  __asm__ ("vinsertps $0x20, %1, %0, %0" : "+x" (tmp) : "m" (p[2]));
  return FloatVector4(tmp);
}

inline void FloatVector4::convertToVector3(float *p) const
{
  __asm__ ("vmovq %1, %0" : "=m" (*p) : "x" (v));
  __asm__ ("vextractps $0x02, %1, %0" : "=m" (p[2]) : "x" (v));
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
  v = __builtin_ia32_blendps(v, tmp, 0x08);
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
  v = __builtin_ia32_blendvps(v, r.v, mask.v);
  return (*this);
}

inline FloatVector4& FloatVector4::minValues(const FloatVector4& r)
{
  v = __builtin_ia32_minps(v, r.v);
  return (*this);
}

inline FloatVector4& FloatVector4::maxValues(const FloatVector4& r)
{
  v = __builtin_ia32_maxps(v, r.v);
  return (*this);
}

inline FloatVector4& FloatVector4::floorValues()
{
  v = __builtin_ia32_roundps(v, 0x09);
  return (*this);
}

inline FloatVector4& FloatVector4::ceilValues()
{
  v = __builtin_ia32_roundps(v, 0x0A);
  return (*this);
}

inline FloatVector4& FloatVector4::roundValues()
{
  v = __builtin_ia32_roundps(v, 0x08);
  return (*this);
}

inline FloatVector4& FloatVector4::absValues()
{
  XMM_Int32 m = { 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF };
  __asm__ ("vandps %1, %0, %0" : "+x" (v) : "xm" (m));
  return (*this);
}

inline float FloatVector4::dotProduct(const FloatVector4& r) const
{
  XMM_Float tmp1 = v * r.v;
  tmp1 = tmp1 + __builtin_ia32_shufps(tmp1, tmp1, 0xB1);
  tmp1 = tmp1 + __builtin_ia32_shufps(tmp1, tmp1, 0x4E);
  return tmp1[0];
}

inline float FloatVector4::dotProduct3(const FloatVector4& r) const
{
  XMM_Float tmp1 = v * r.v;
  XMM_Float tmp2 = __builtin_ia32_shufps(tmp1, tmp1, 0xB1);
  tmp1 = tmp1 + tmp2 + __builtin_ia32_shufps(tmp1, tmp1, 0x4E);
  return tmp1[0];
}

inline float FloatVector4::dotProduct2(const FloatVector4& r) const
{
  XMM_Float tmp1 = v * r.v;
  tmp1 = tmp1 + __builtin_ia32_shufps(tmp1, tmp1, 0xB1);
  return tmp1[0];
}

inline FloatVector4 FloatVector4::crossProduct3(const FloatVector4& r) const
{
  XMM_Float tmp1 = __builtin_ia32_shufps(v, v, 0xC9);
  XMM_Float tmp2 = __builtin_ia32_shufps(r.v, r.v, 0xC9);
  tmp1 = (tmp2 * v) - (tmp1 * r.v);
  tmp1 = __builtin_ia32_shufps(tmp1, tmp1, 0xC9);
  return FloatVector4(tmp1);
}

inline FloatVector4& FloatVector4::squareRoot()
{
  XMM_Float tmp = { 0.0f, 0.0f, 0.0f, 0.0f };
  v = __builtin_ia32_sqrtps(__builtin_ia32_maxps(v, tmp));
  return (*this);
}

inline FloatVector4& FloatVector4::squareRootFast()
{
  XMM_Float tmp1 = __builtin_ia32_maxps(v, floatMinValV);
  v = __builtin_ia32_rsqrtps(tmp1) * tmp1;
  return (*this);
}

inline FloatVector4& FloatVector4::rsqrtFast()
{
  v = __builtin_ia32_rsqrtps(v);
  return (*this);
}

inline FloatVector4& FloatVector4::rcpFast()
{
  v = __builtin_ia32_rcpps(v);
  return (*this);
}

inline FloatVector4& FloatVector4::rcpSqr()
{
  XMM_Float tmp1(v * v);
  XMM_Float tmp2 = __builtin_ia32_rcpps(tmp1);
  v = (2.0f - tmp1 * tmp2) * tmp2;
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
  std::uint32_t n = std::bit_cast< std::uint32_t >(x);
  float   e = float(int(n >> 23));
  n = (n & 0x007FFFFFU) | 0x3F800000U;
  float   m = std::bit_cast< float >(n);
  float   m2 = m * m;
  FloatVector4  tmp1(m, m2, m * m2, m2 * m2);
  FloatVector4  tmp2(4.05608897f, -2.10465275f, 0.63728021f, -0.08021013f);
  return (tmp1.dotProduct(tmp2) + e - (127.0f + 2.50847106f));
}

inline FloatVector4& FloatVector4::log2V()
{
  XMM_Float e, m, tmp;
  __asm__ ("vpsrld $0x17, %1, %0" : "=x" (e) : "x" (v));
  __asm__ ("vmovd %1, %0" : "=x" (tmp) : "r" (0x3F800000U));
  __asm__ ("vpslld $0x17, %1, %0" : "=x" (m) : "x" (e));
  __asm__ ("vcvtdq2ps %0, %0" : "+x" (e));
  __asm__ ("vpshufd $0x00, %0, %0" : "+x" (tmp));
  __asm__ ("vpxor %1, %0, %0" : "+x" (m) : "x" (v));
  __asm__ ("vpor %1, %0, %0" : "+x" (m) : "x" (tmp));
  XMM_Float m2 = m * m;
  tmp = m * 4.05608897f - (127.0f + 2.50847106f);
  tmp = tmp + (((m2 * -0.08021013f) + (m * 0.63728021f) - 2.10465275f) * m2);
  v = tmp + e;
  return (*this);
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

inline FloatVector4& FloatVector4::exp2V()
{
  XMM_Float e;
  __asm__ ("vroundps $0x09, %1, %0" : "=x" (e) : "x" (v));
  XMM_Float m = v - e;          // e = floor(v)
  __asm__ ("vcvtps2dq %0, %0" : "+x" (e));
  __asm__ ("vpslld $0x17, %0, %0" : "+x" (e));
  m = (m * 0.00825060f + 0.05924474f) * (m * m) + (m * 0.34671664f + 1.0f);
  m = m * m;
  __asm__ ("vpaddd %2, %1, %0" : "=x" (v) : "x" (m), "x" (e));
  return (*this);
}

inline FloatVector4& FloatVector4::normalize(bool invFlag)
{
  static const float  invMultTable[2] = { -0.5f, 0.5f };
  float   tmp = dotProduct(*this);
  float   tmp2;
  __asm__ ("vmaxss %2, %1, %0"
           : "=x" (tmp2) : "x" (tmp), "xm" (floatMinVal));
  __asm__ ("vrsqrtss %0, %0, %0" : "+x" (tmp2));
  v *= ((tmp * tmp2 * tmp2 - 3.0f) * (tmp2 * invMultTable[int(invFlag)]));
  return (*this);
}

inline FloatVector4& FloatVector4::normalize3Fast()
{
  const FloatVector4  minVal(1.0f / float(0x0000040000000000LL));
  XMM_Float tmp = __builtin_ia32_blendps(v, minVal.v, 0x08);
  XMM_Float tmp2 = tmp * tmp;
  tmp2 = tmp2 + __builtin_ia32_shufps(tmp2, tmp2, 0xB1);
  tmp2 = __builtin_ia32_rsqrtps(tmp2 + __builtin_ia32_shufps(tmp2, tmp2, 0x4E));
  v = tmp * tmp2;
  return (*this);
}

inline FloatVector4 FloatVector4::normalize3x3Fast(
    const FloatVector4& a, const FloatVector4& b, const FloatVector4& c)
{
  // tmp1 = b[0], b[0], c[0], c[0]
  XMM_Float tmp1 = __builtin_ia32_shufps(b.v, c.v, 0x00);
  // tmp2 = a[1], a[1], c[1], c[1]
  XMM_Float tmp2 = __builtin_ia32_shufps(a.v, c.v, 0x55);
  // tmp3 = a[2], b[2], a[3], b[3]
  XMM_Float tmp3 = { a[2], b[2], a[3], b[3] };
  tmp1 = __builtin_ia32_blendps(tmp1, a.v, 0x09);
  tmp2 = __builtin_ia32_blendps(tmp2, b.v, 0x0A);
  tmp3 = __builtin_ia32_blendps(tmp3, c.v, 0x0C);
  tmp1 = (tmp1 * tmp1) + (tmp2 * tmp2) + (tmp3 * tmp3);
  tmp1 = __builtin_ia32_rsqrtps(__builtin_ia32_maxps(tmp1, floatMinValV));
  return FloatVector4(tmp1);
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
  XMM_Float tmp2 = v * float(0.03876962 * 255.0) + float(1.15864660 * 255.0);
  XMM_Float tmp = __builtin_ia32_rsqrtps(v);
  v *= (tmp * tmp2 - float(0.19741622 * 255.0));
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

inline FloatVector4 FloatVector4::convertA2R10G10B10(
    const std::uint32_t& c, bool expandSRGB)
{
  XMM_Float v;
#if ENABLE_X86_64_SIMD >= 4
  static const XMM_UInt32 maskTbl =
  {
    0x3FF00000U, 0x000FFC00U, 0x000003FFU, 0x0000C000U
  };
  static const XMM_Float  srgbTbl1 =
  {
    float(0.13945550 / (1072693248.0 * 1072693248.0)),
    float(0.13945550 / (1047552.0 * 1047552.0)),
    float(0.13945550 / (1023.0 * 1023.0)),
    float(0.13945550 / (49152.0 * 49152.0))
  };
  static const XMM_Float  srgbTbl2 =
  {
    float(0.86054450 / 1072693248.0), float(0.86054450 / 1047552.0),
    float(0.86054450 / 1023.0), float(0.86054450 / 49152.0)
  };
  static const XMM_Float  multTbl =
  {
    float(255.0 / 1072693248.0), float(255.0 / 1047552.0),
    float(255.0 / 1023.0), float(255.0 / 49152.0)
  };
  __asm__ ("vpbroadcastd %1, %0" : "=x" (v) : "m" (c));
  __asm__ ("vpshufhw $0xb4, %0, %0" : "+x" (v));
#else
  static const XMM_UInt8  shufTbl =
  {
    2, 3, 0x80, 0x80, 1, 2, 0x80, 0x80, 0, 1, 0x80, 0x80, 3, 0x80, 0x80, 0x80
  };
  static const XMM_UInt32 maskTbl =
  {
    0x00003FF0U, 0x00000FFCU, 0x000003FFU, 0x000000C0U
  };
  static const XMM_Float  srgbTbl1 =
  {
    float(0.13945550 / (16368.0 * 16368.0)),
    float(0.13945550 / (4092.0 * 4092.0)),
    float(0.13945550 / (1023.0 * 1023.0)), float(0.13945550 / (192.0 * 192.0))
  };
  static const XMM_Float  srgbTbl2 =
  {
    float(0.86054450 / 16368.0), float(0.86054450 / 4092.0),
    float(0.86054450 / 1023.0), float(0.86054450 / 192.0)
  };
  static const XMM_Float  multTbl =
  {
    255.0f / 16368.0f, 255.0f / 4092.0f, 255.0f / 1023.0f, 255.0f / 192.0f
  };
  __asm__ ("vmovd %1, %0" : "=x" (v) : "rm" (c));
  __asm__ ("vpshufb %1, %0, %0" : "+x" (v) : "xm" (shufTbl));
#endif
  __asm__ ("vpand %1, %0, %0" : "+x" (v) : "xm" (maskTbl));
  __asm__ ("vcvtdq2ps %0, %0" : "+x" (v));
  if (expandSRGB)
  {
    // gamma expand and scale to 1.0
    v *= (v * srgbTbl1 + srgbTbl2);
    v *= v;
  }
  else
  {
    // scale to 255.0
    v *= multTbl;
  }
  return FloatVector4(v);
}

inline std::uint32_t FloatVector4::convertToA2R10G10B10(bool noClamp) const
{
  static const XMM_Float  multTbl1 =
  {
    1023.0f / 255.0f, 1023.0f / 255.0f, 1023.0f / 255.0f, 3.0f / 255.0f
  };
#if ENABLE_X86_64_SIMD >= 4
  static const XMM_UInt32 sllTbl = { 20U, 10U, 0U, 30U };
#else
  static const XMM_UInt8  shufTbl =
  {
    4, 5, 0, 1, 8, 9, 12, 13, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80
  };
  static const XMM_Int16  multTbl2 = { 64, 1, 1, 16384, 0, 0, 0, 0 };
#endif
  XMM_Float   tmp1;
  XMM_UInt32  tmp2;
  __asm__ ("vmulps %2, %1, %0" : "=x" (tmp1) : "x" (v), "xm" (multTbl1));
  __asm__ ("vcvtps2dq %0, %0" : "+x" (tmp1));
  if (!noClamp)
  {
    static const XMM_Int32  minTbl = { 0, 0, 0, 0 };
    static const XMM_Int32  maxTbl = { 1023, 1023, 1023, 3 };
    __asm__ ("vpmaxsd %1, %0, %0" : "+x" (tmp1) : "xm" (minTbl));
    __asm__ ("vpminsd %1, %0, %0" : "+x" (tmp1) : "xm" (maxTbl));
  }
#if ENABLE_X86_64_SIMD >= 4
  __asm__ ("vpsllvd %1, %0, %0" : "+x" (tmp1) : "xm" (sllTbl));
  __asm__ ("vpshufd $0x4e, %1, %0" : "=x" (tmp2) : "x" (tmp1));
  __asm__ ("vpor %1, %0, %0" : "+x" (tmp2) : "x" (tmp1));
  __asm__ ("vpshufd $0xb1, %1, %0" : "=x" (tmp1) : "x" (tmp2));
#else
  __asm__ ("vpshufb %1, %0, %0" : "+x" (tmp1) : "xm" (shufTbl));
  __asm__ ("vpmullw %1, %0, %0" : "+x" (tmp1) : "xm" (multTbl2));
  __asm__ ("vpsrlq $0x20, %1, %0" : "=x" (tmp2) : "x" (tmp1));
  __asm__ ("vpslld $0x04, %0, %0" : "+x" (tmp1));
#endif
  __asm__ ("vpor %1, %0, %0" : "+x" (tmp2) : "x" (tmp1));
  return tmp2[0];
}

inline std::uint32_t FloatVector4::normalToUInt32() const
{
#if ENABLE_X86_64_SIMD >= 3
  static const XMM_UInt32 signMask =
  {
    0x80000000U, 0x80000000U, 0x80000000U, 0x80000000U
  };
  XMM_UInt32  tmp;
  // negate if Z < 0
  __asm__ ("vpand %2, %1, %0" : "=x" (tmp) : "x" (v), "m" (signMask));
  __asm__ ("vpshufd $0xaa, %0, %0" : "+x" (tmp));
  __asm__ ("vpxor %1, %0, %0" : "+x" (tmp) : "x" (v));
  // use FP16 format if F16C instructions are available
  __asm__ ("vcvtps2ph $0x00, %0, %0" : "+x" (tmp));
  return tmp[0];
#else
  // use 16-bit signed integers otherwise
  static const float  int16MultTable[2] = { -32767.0f, 32767.0f };
  size_t  n;
  __asm__ ("vmovmskps %1, %0" : "=r" (n) : "x" (v));
  XMM_UInt32  tmp =
      std::bit_cast< XMM_UInt32 >(v * int16MultTable[(n & 4) >> 2]);
  __asm__ ("vcvtps2dq %0, %0" : "+x" (tmp));
  __asm__ ("vpackssdw %0, %0, %0" : "+x" (tmp));
  return tmp[0];
#endif
}

inline FloatVector4 FloatVector4::uint32ToNormal(const std::uint32_t& n)
{
#if ENABLE_X86_64_SIMD >= 3
  FloatVector4  tmp;
  __asm__ ("vmovd %1, %0" : "=x" (tmp.v) : "rm" (n));
  __asm__ ("vcvtph2ps %0, %0" : "+x" (tmp.v));
  tmp[2] = float(std::sqrt(std::max(1.0f - tmp.dotProduct2(tmp), 0.0f)));
  tmp *= -1.0f;
  return tmp.normalize3Fast();
#else
  FloatVector4  tmp;
  __asm__ ("vmovd %1, %0" : "=x" (tmp.v) : "rm" (n));
  __asm__ ("vpmovsxwd %0, %0" : "+x" (tmp.v));
  __asm__ ("vcvtdq2ps %0, %0" : "+x" (tmp.v));
  tmp[2] = -(float(std::sqrt(std::max((32767.0f * 32767.0f)
                                      - tmp.dotProduct2(tmp), 0.0f))));
  return tmp.normalize3Fast();
#endif
}

inline std::uint32_t FloatVector4::convertToX10Y10Z10() const
{
  static const XMM_Float  multTbl = { 511.5f, 511.5f, 511.5f, 0.0f };
  static const XMM_UInt32 maxTbl = { 1023U, 1023U, 1023U, 0U };
  XMM_Float   tmp1 = v;
  XMM_UInt32  tmp2;
  tmp1 = tmp1 * multTbl + multTbl;
  __asm__ ("vcvtps2dq %1, %0" : "=x" (tmp2) : "x" (tmp1));
  __asm__ ("vpminsd %1, %0, %0" : "+x" (tmp2) : "xm" (maxTbl));
#if ENABLE_X86_64_SIMD >= 4
  static const XMM_UInt32 sllTbl = { 0U, 2U, 4U, 0U };
  __asm__ ("vpsllvd %1, %0, %0" : "+x" (tmp2) : "xm" (sllTbl));
#endif
  __asm__ ("vpackusdw %0, %0, %0" : "+x" (tmp2));
#if ENABLE_X86_64_SIMD < 4
  static const XMM_UInt16 mulTbl2 = { 1, 4, 16, 0 };
  __asm__ ("vpmullw %1, %0, %0" : "+x" (tmp2) : "xm" (mulTbl2));
#endif
  __asm__ ("vpshuflw $0x78, %0, %0" : "+x" (tmp2));
  __asm__ ("vpsrlq $0x28, %1, %0" : "=x" (tmp1) : "x" (tmp2));
  __asm__ ("vpor %1, %0, %0" : "+x" (tmp2) : "x" (tmp1));
  return tmp2[0];
}

inline FloatVector4 FloatVector4::convertX10Y10Z10(const std::uint32_t& n)
{
  FloatVector4  tmp;
  static const XMM_UInt32 maskTbl =
  {
    0x000003FFU, 0x000FFC00U, 0x3FF00000U, 0U
  };
  static const XMM_Float  multTbl =
  {
    float(2.0 / 1023.0), float(2.0 / 1047552.0), float(2.0 / 1072693248.0), 0.0f
  };
  static const XMM_Float  offsTbl = { -1.0f, -1.0f, -1.0f, 0.0f };
#if ENABLE_X86_64_SIMD >= 4
  __asm__ ("vpbroadcastd %1, %0" : "=x" (tmp.v) : "m" (n));
#else
  __asm__ ("vmovd %1, %0" : "=x" (tmp.v) : "rm" (n));
  __asm__ ("vpshufd $0x00, %0, %0" : "+x" (tmp.v));
#endif
  __asm__ ("vpand %1, %0, %0" : "+x" (tmp.v) : "xm" (maskTbl));
  __asm__ ("vcvtdq2ps %0, %0" : "+x" (tmp.v));
  tmp.v = tmp.v * multTbl + offsTbl;
  return tmp;
}

inline std::uint32_t FloatVector4::convertToX10Y10Z10W2() const
{
  static const XMM_Float  multTbl = { 511.5f, 511.5f, 511.5f, 1.5f };
  static const XMM_UInt32 maxTbl = { 1023U, 1023U, 1023U, 3U };
  XMM_Float   tmp1 = v;
  XMM_UInt32  tmp2;
  tmp1 = tmp1 * multTbl + multTbl;
  __asm__ ("vcvtps2dq %1, %0" : "=x" (tmp2) : "x" (tmp1));
  __asm__ ("vpminsd %1, %0, %0" : "+x" (tmp2) : "xm" (maxTbl));
  __asm__ ("vpslld $0x04, %1, %0" : "=x" (tmp1) : "x" (tmp2));
  __asm__ ("vpblendw $0xf0, %1, %0, %0" : "+x" (tmp2) : "x" (tmp1));
  __asm__ ("vpackusdw %0, %0, %0" : "+x" (tmp2));
  __asm__ ("vpshuflw $0xdf, %1, %0" : "=x" (tmp1) : "x" (tmp2));
  __asm__ ("vpshuflw $0xd8, %0, %0" : "+x" (tmp2));
  __asm__ ("vpsrlq $0x16, %0, %0" : "+x" (tmp1));
  __asm__ ("vpor %1, %0, %0" : "+x" (tmp2) : "x" (tmp1));
  return tmp2[0];
}

inline FloatVector4 FloatVector4::convertX10Y10Z10W2(const std::uint32_t& n)
{
  FloatVector4  tmp;
  static const XMM_UInt32 maskTbl =
  {
    0x000003FFU, 0x000FFC00U, 0x3FF00000U, 0xC0000000U
  };
  static const XMM_Float  multTbl =
  {
    float(2.0 / 1023.0), float(2.0 / 1047552.0), float(2.0 / 1072693248.0),
    float(2.0 / 3221225472.0)
  };
  static const XMM_Float  offsTbl = { -1.0f, -1.0f, -1.0f, float(1.0 / 3.0) };
  std::uint32_t m = n ^ 0x80000000U;
  XMM_UInt32  tmp2 = { m, m, m, m };
  tmp2 = tmp2 & maskTbl;
  __asm__ ("vcvtdq2ps %1, %0" : "=x" (tmp.v) : "x" (tmp2));
  tmp.v = tmp.v * multTbl + offsTbl;
  return tmp;
}

inline std::uint32_t FloatVector4::getSignMask() const
{
  return std::uint32_t(__builtin_ia32_movmskps(v));
}

inline FloatVector4 FloatVector4::convertR9G9B9E5(const std::uint32_t& n)
{
  FloatVector4  tmp;
  static const XMM_UInt32 maskTbl =
  {
    0x000001FFU, 0x0003FE00U, 0x07FC0000U, 0xF8000000U
  };
  const XMM_Float multTbl =
  {
    float(1.0 / 16777216.0), float(1.0 / 8589934592.0),
    float(1.0 / 4398046511104.0), 1.0f
  };
  XMM_UInt32  tmp2;
  __asm__ ("vmovd %1, %0" : "=x" (tmp.v) : "rm" (n));
  __asm__ ("vpshufd $0x00, %0, %0" : "+x" (tmp.v));
  __asm__ ("vpand %1, %0, %0" : "+x" (tmp.v) : "xm" (maskTbl));
  __asm__ ("vpshufd $0xff, %1, %0" : "=x" (tmp2) : "x" (tmp.v));
  __asm__ ("vpsrld $0x04, %0, %0" : "+x" (tmp2));
  __asm__ ("vcvtdq2ps %0, %0" : "+x" (tmp.v));
  __asm__ ("vpaddd %1, %0, %0" : "+x" (tmp2) : "x" (multTbl));
  __asm__ ("vmulps %1, %0, %0" : "+x" (tmp.v) : "x" (tmp2));
  __asm__ ("vblendps $0x08, %1, %0, %0" : "+x" (tmp.v) : "x" (multTbl));
  return tmp;
}

inline std::uint32_t FloatVector4::convertToR9G9B9E5() const
{
  FloatVector4  c(*this);
  c.maxValues(FloatVector4(0.0f));
  const XMM_UInt32  maskMultSubMaxTbl =
  {
    0x7F800000U, 0x4B800000U, 0x37800000U, 0x0F800000U
  };
  XMM_UInt32  tmp1, tmp2, tmp3;
  __asm__ ("vpshufd $0x00, %1, %0" : "=x" (tmp1) : "x" (maskMultSubMaxTbl));
  __asm__ ("vpand %2, %1, %0" : "=x" (tmp2) : "x" (c.v), "x" (tmp1));
  __asm__ ("vpshufd $0xc9, %1, %0" : "=x" (tmp1) : "x" (tmp2));
  __asm__ ("vpmaxud %1, %0, %0" : "+x" (tmp1) : "x" (tmp2));
  __asm__ ("vpshufd $0xd2, %0, %0" : "+x" (tmp2));
  __asm__ ("vpmaxud %1, %0, %0" : "+x" (tmp1) : "x" (tmp2));
  __asm__ ("vpsubusw %2, %1, %0"
           : "=x" (tmp3) : "x" (tmp1), "x" (maskMultSubMaxTbl));
  __asm__ ("vpshufd $0xaa, %0, %0" : "+x" (tmp3));
  __asm__ ("vpshufd $0xff, %1, %0" : "=x" (tmp1) : "x" (maskMultSubMaxTbl));
  __asm__ ("vpshufd $0x55, %1, %0" : "=x" (tmp2) : "x" (maskMultSubMaxTbl));
  __asm__ ("vpminuw %1, %0, %0" : "+x" (tmp3) : "x" (tmp1));
  __asm__ ("vpsubd %1, %0, %0" : "+x" (tmp2) : "x" (tmp3));
  __asm__ ("vmulps %1, %0, %0" : "+x" (c.v) : "x" (tmp2));
  c.minValues(FloatVector4(511.0f));
  __asm__ ("vcvtps2dq %1, %0" : "=x" (tmp2) : "x" (c.v));
  return (tmp2[0] | (tmp2[1] << 9) | (tmp2[2] << 18) | (tmp3[0] << 4));
}

#endif

