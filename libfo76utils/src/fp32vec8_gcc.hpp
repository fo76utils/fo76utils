
#ifndef FP32VEC8_GCC_HPP_INCLUDED
#define FP32VEC8_GCC_HPP_INCLUDED

inline FloatVector8::FloatVector8(const float& x)
#if ENABLE_X86_64_SIMD >= 4
{
  __asm__ ("vbroadcastss %1, %t0" : "=x" (v) : "xm" (x));
}
#else
  : v { x, x, x, x, x, x, x, x }
{
}
#endif

inline FloatVector8::FloatVector8(const std::uint64_t *p)
{
#if ENABLE_X86_64_SIMD >= 4
  __asm__ ("vpmovzxbd %1, %t0" : "=x" (v) : "m" (*p));
  __asm__ ("vcvtdq2ps %t0, %t0" : "+x" (v));
#else
  const unsigned char *b = reinterpret_cast< const unsigned char * >(p);
  XMM_UInt32  tmp;
  __asm__ ("vpmovzxbd %1, %x0" : "=x" (v) : "m" (*p));
  __asm__ ("vpmovzxbd %1, %0" : "=x" (tmp) : "m" (*(b + 4)));
  __asm__ ("vcvtdq2ps %x0, %x0" : "+x" (v));
  __asm__ ("vcvtdq2ps %0, %0" : "+x" (tmp));
  __asm__ ("vinsertf128 $0x01, %1, %t0, %t0" : "+x" (v) : "x" (tmp));
#endif
}

inline FloatVector8::FloatVector8(const FloatVector4 *p)
{
  __asm__ ("vmovups %1, %t0" : "=x" (v) : "m" (*p));
}

inline FloatVector8::FloatVector8(FloatVector4 v0, FloatVector4 v1)
{
  __asm__ ("vinsertf128 $0x01, %x2, %t1, %t0"
           : "=x" (v) : "x" (v0.v), "xm" (v1.v));
}

inline FloatVector8::FloatVector8(float v0, float v1, float v2, float v3,
                                  float v4, float v5, float v6, float v7)
  : v { v0, v1, v2, v3, v4, v5, v6, v7 }
{
}

inline FloatVector8::FloatVector8(const float *p)
{
  __asm__ ("vmovups %1, %t0" : "=x" (v) : "m" (*p));
}

inline FloatVector8::FloatVector8(const std::int16_t *p)
{
#if ENABLE_X86_64_SIMD >= 4
  __asm__ ("vpmovsxwd %1, %t0" : "=x" (v) : "m" (*p));
  __asm__ ("vcvtdq2ps %t0, %t0" : "+x" (v));
#else
  XMM_UInt32  tmp;
  __asm__ ("vpmovsxwd %1, %x0" : "=x" (v) : "m" (*p));
  __asm__ ("vpmovsxwd %1, %0" : "=x" (tmp) : "m" (*(p + 4)));
  __asm__ ("vcvtdq2ps %x0, %x0" : "+x" (v));
  __asm__ ("vcvtdq2ps %0, %0" : "+x" (tmp));
  __asm__ ("vinsertf128 $0x01, %1, %t0, %t0" : "+x" (v) : "x" (tmp));
#endif
}

inline FloatVector8::FloatVector8(const std::int32_t *p)
{
  __asm__ ("vmovdqu %1, %t0" : "=x" (v) : "m" (*p));
  __asm__ ("vcvtdq2ps %t0, %t0" : "+x" (v));
}

inline FloatVector8::FloatVector8(const std::uint16_t *p, bool noInfNaN)
{
#if ENABLE_X86_64_SIMD >= 3
  if (!noInfNaN)
  {
    __asm__ ("vcvtph2ps %1, %t0" : "=x" (v) : "m" (*p));
  }
  else
  {
    __asm__ ("vmovdqu %1, %x0" : "=x" (v) : "m" (*p));
    const XMM_UInt16  expMaskTbl =
    {
      0x7C00, 0x7C00, 0x7C00, 0x7C00, 0x7C00, 0x7C00, 0x7C00, 0x7C00
    };
    XMM_UInt16  tmp;
    __asm__ ("vpand %2, %x1, %0" : "=x" (tmp) : "x" (v), "x" (expMaskTbl));
    __asm__ ("vpcmpeqw %1, %0, %0" : "+x" (tmp) : "x" (expMaskTbl));
    __asm__ ("vpandn %x0, %1, %x0" : "+x" (v) : "x" (tmp));
    __asm__ ("vcvtph2ps %x0, %t0" : "+x" (v));
  }
#else
  std::uint64_t v0, v1;
  __asm__ ("mov %1, %0" : "=r" (v0) : "m" (*p));
  __asm__ ("mov %1, %0" : "=r" (v1) : "m" (*(p + 4)));
  v = FloatVector8(FloatVector4::convertFloat16(v0, noInfNaN),
                   FloatVector4::convertFloat16(v1, noInfNaN)).v;
#endif
}

inline void FloatVector8::convertToFloats(float *p) const
{
  __asm__ ("vmovups %t1, %0" : "=m" (*p) : "x" (v));
}

inline void FloatVector8::convertToFloatVector4(FloatVector4 *p) const
{
  __asm__ ("vmovups %t1, %0" : "=m" (*p) : "x" (v));
}

inline void FloatVector8::convertToInt16(std::int16_t *p) const
{
#if ENABLE_X86_64_SIMD >= 4
  YMM_UInt32  tmp;
  __asm__ ("vcvtps2dq %t1, %t0" : "=x" (tmp) : "xm" (v));
  __asm__ ("vpackssdw %t0, %t0, %t0" : "+x" (tmp));
  __asm__ ("vmovdqu %x1, %0" : "=m" (*p) : "x" (tmp));
#else
  XMM_UInt32  tmp1, tmp2;
  __asm__ ("vextractf128 $0x01, %t1, %0" : "=x" (tmp2) : "x" (v));
  __asm__ ("vcvtps2dq %x1, %0" : "=x" (tmp1) : "x" (v));
  __asm__ ("vcvtps2dq %0, %0" : "+x" (tmp2));
  __asm__ ("vpackssdw %1, %0, %0" : "+x" (tmp1) : "x" (tmp2));
  __asm__ ("vmovdqu %1, %0" : "=m" (*p) : "x" (tmp1));
#endif
}

inline void FloatVector8::convertToInt32(std::int32_t *p) const
{
  YMM_UInt32  tmp;
  __asm__ ("vcvtps2dq %t1, %t0" : "=x" (tmp) : "xm" (v));
  __asm__ ("vmovdqu %t1, %0" : "=m" (*p) : "x" (tmp));
}

inline void FloatVector8::convertToFloat16(std::uint16_t *p) const
{
#if ENABLE_X86_64_SIMD >= 3
  __asm__ ("vcvtps2ph $0x00, %t1, %0" : "=m" (*p) : "x" (v));
#else
  p[0] = ::convertToFloat16(v[0]);
  p[1] = ::convertToFloat16(v[1]);
  p[2] = ::convertToFloat16(v[2]);
  p[3] = ::convertToFloat16(v[3]);
  p[4] = ::convertToFloat16(v[4]);
  p[5] = ::convertToFloat16(v[5]);
  p[6] = ::convertToFloat16(v[6]);
  p[7] = ::convertToFloat16(v[7]);
#endif
}

inline FloatVector8& FloatVector8::operator+=(const FloatVector8& r)
{
  v += r.v;
  return (*this);
}

inline FloatVector8& FloatVector8::operator-=(const FloatVector8& r)
{
  v -= r.v;
  return (*this);
}

inline FloatVector8& FloatVector8::operator*=(const FloatVector8& r)
{
  v *= r.v;
  return (*this);
}

inline FloatVector8& FloatVector8::operator/=(const FloatVector8& r)
{
  v /= r.v;
  return (*this);
}

inline FloatVector8 FloatVector8::operator+(const FloatVector8& r) const
{
  FloatVector8  tmp(*this);
  return (tmp += r);
}

inline FloatVector8 FloatVector8::operator-(const FloatVector8& r) const
{
  FloatVector8  tmp(*this);
  return (tmp -= r);
}

inline FloatVector8 FloatVector8::operator*(const FloatVector8& r) const
{
  FloatVector8  tmp(*this);
  return (tmp *= r);
}

inline FloatVector8 FloatVector8::operator/(const FloatVector8& r) const
{
  FloatVector8  tmp(*this);
  return (tmp /= r);
}

inline FloatVector8& FloatVector8::operator+=(float r)
{
  v += r;
  return (*this);
}

inline FloatVector8& FloatVector8::operator-=(float r)
{
  v -= r;
  return (*this);
}

inline FloatVector8& FloatVector8::operator*=(float r)
{
  v *= r;
  return (*this);
}

inline FloatVector8& FloatVector8::operator/=(float r)
{
  v /= r;
  return (*this);
}

inline FloatVector8 FloatVector8::operator+(const float& r) const
{
  FloatVector8  tmp(*this);
  return (tmp += r);
}

inline FloatVector8 FloatVector8::operator-(const float& r) const
{
  FloatVector8  tmp(*this);
  return (tmp -= r);
}

inline FloatVector8 FloatVector8::operator*(const float& r) const
{
  FloatVector8  tmp(*this);
  return (tmp *= r);
}

inline FloatVector8 FloatVector8::operator/(const float& r) const
{
  FloatVector8  tmp(*this);
  return (tmp /= r);
}

inline FloatVector8& FloatVector8::shuffleValues(unsigned char mask)
{
  v = __builtin_ia32_shufps256(v, v, mask);
  return (*this);
}

inline FloatVector8& FloatVector8::blendValues(
    const FloatVector8& r, unsigned char mask)
{
  v = __builtin_ia32_blendps256(v, r.v, mask);
  return (*this);
}

inline FloatVector8& FloatVector8::blendValues(
    const FloatVector8& r, const FloatVector8& mask)
{
  v = __builtin_ia32_blendvps256(v, r.v, mask.v);
  return (*this);
}

inline FloatVector8& FloatVector8::minValues(const FloatVector8& r)
{
  v = __builtin_ia32_minps256(v, r.v);
  return (*this);
}

inline FloatVector8& FloatVector8::maxValues(const FloatVector8& r)
{
  v = __builtin_ia32_maxps256(v, r.v);
  return (*this);
}

inline FloatVector8& FloatVector8::floorValues()
{
  v = __builtin_ia32_roundps256(v, 0x09);
  return (*this);
}

inline FloatVector8& FloatVector8::ceilValues()
{
  v = __builtin_ia32_roundps256(v, 0x0A);
  return (*this);
}

inline FloatVector8& FloatVector8::roundValues()
{
  v = __builtin_ia32_roundps256(v, 0x08);
  return (*this);
}

inline FloatVector8& FloatVector8::absValues()
{
  YMM_Int32 m =
  {
    0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF,
    0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF
  };
  v = __builtin_ia32_andps256(v, std::bit_cast< YMM_Float >(m));
  return (*this);
}

inline float FloatVector8::dotProduct(const FloatVector8& r) const
{
  YMM_Float tmp1 = v * r.v;
  XMM_Float tmp2 = __builtin_ia32_vextractf128_ps256(tmp1, 0x01);
  __asm__ ("vaddps %0, %x1, %0" : "+x" (tmp2) : "x" (tmp1));
  tmp2 = tmp2 + __builtin_ia32_movshdup(tmp2);
  tmp2 = tmp2 + __builtin_ia32_shufps(tmp2, tmp2, 0x4E);
  return tmp2[0];
}

inline FloatVector8& FloatVector8::squareRoot()
{
  YMM_Float tmp = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
  v = __builtin_ia32_sqrtps256(__builtin_ia32_maxps256(v, tmp));
  return (*this);
}

inline FloatVector8& FloatVector8::squareRootFast()
{
  YMM_Float tmp1 = __builtin_ia32_maxps256(v, floatMinValV);
  v = __builtin_ia32_rsqrtps256(tmp1) * tmp1;
  return (*this);
}

inline FloatVector8& FloatVector8::rsqrtFast()
{
  v = __builtin_ia32_rsqrtps256(v);
  return (*this);
}

inline FloatVector8& FloatVector8::rcpFast()
{
  v = __builtin_ia32_rcpps256(v);
  return (*this);
}

inline FloatVector8& FloatVector8::rcpSqr()
{
  YMM_Float tmp1(v * v);
  YMM_Float tmp2 = __builtin_ia32_rcpps256(tmp1);
  v = (2.0f - tmp1 * tmp2) * tmp2;
  return (*this);
}

inline FloatVector8& FloatVector8::log2V()
{
#if ENABLE_X86_64_SIMD >= 4
  YMM_Float e, m, tmp;
  __asm__ ("vpsrld $0x17, %t1, %t0" : "=x" (e) : "x" (v));
  __asm__ ("vmovd %1, %x0" : "=x" (tmp) : "r" (0x3F800000U));
  __asm__ ("vpslld $0x17, %t1, %t0" : "=x" (m) : "x" (e));
  __asm__ ("vcvtdq2ps %t0, %t0" : "+x" (e));
  __asm__ ("vpbroadcastd %x0, %t0" : "+x" (tmp));
  __asm__ ("vpxor %t1, %t0, %t0" : "+x" (m) : "x" (v));
  __asm__ ("vpor %t1, %t0, %t0" : "+x" (m) : "x" (tmp));
  YMM_Float m2 = m * m;
  tmp = m * 4.05608897f - (127.0f + 2.50847106f);
  tmp = tmp + (((m2 * -0.08021013f) + (m * 0.63728021f) - 2.10465275f) * m2);
  v = tmp + e;
#else
  FloatVector4  tmp1(v[0], v[1], v[2], v[3]);
  FloatVector4  tmp2(v[4], v[5], v[6], v[7]);
  v = FloatVector8(tmp1.log2V(), tmp2.log2V()).v;
#endif
  return (*this);
}

inline FloatVector8& FloatVector8::exp2V()
{
#if ENABLE_X86_64_SIMD >= 4
  YMM_Float e;
  __asm__ ("vroundps $0x09, %t1, %t0" : "=x" (e) : "x" (v));
  YMM_Float m = v - e;          // e = floor(v)
  __asm__ ("vcvtps2dq %t0, %t0" : "+x" (e));
  __asm__ ("vpslld $0x17, %t0, %t0" : "+x" (e));
  m = (m * 0.00825060f + 0.05924474f) * (m * m) + (m * 0.34671664f + 1.0f);
  m = m * m;
  __asm__ ("vpaddd %t2, %t1, %t0" : "=x" (v) : "x" (m), "x" (e));
#else
  FloatVector4  tmp1(v[0], v[1], v[2], v[3]);
  FloatVector4  tmp2(v[4], v[5], v[6], v[7]);
  v = FloatVector8(tmp1.exp2V(), tmp2.exp2V()).v;
#endif
  return (*this);
}

inline FloatVector8::operator std::uint64_t() const
{
  std::uint64_t c;
#if ENABLE_X86_64_SIMD >= 4
  YMM_UInt32  tmp;
  __asm__ ("vcvtps2dq %t1, %t0" : "=x" (tmp) : "xm" (v));
  __asm__ ("vpackssdw %t0, %t0, %t0" : "+x" (tmp));
  __asm__ ("vpackuswb %x0, %x0, %x0" : "+x" (tmp));
  __asm__ ("vmovq %x1, %0" : "=r" (c) : "x" (tmp));
#else
  XMM_Float tmp1, tmp2;
  __asm__ ("vextractf128 $0x01, %t1, %0" : "=x" (tmp2) : "x" (v));
  __asm__ ("vcvtps2dq %x1, %0" : "=x" (tmp1) : "x" (v));
  __asm__ ("vcvtps2dq %0, %0" : "+x" (tmp2));
  __asm__ ("vpackssdw %1, %0, %0" : "+x" (tmp1) : "x" (tmp2));
  __asm__ ("vpackuswb %0, %0, %0" : "+x" (tmp1));
  __asm__ ("vmovq %1, %0" : "=r" (c) : "x" (tmp1));
#endif
  return c;
}

inline std::uint32_t FloatVector8::getSignMask() const
{
  return std::uint32_t(__builtin_ia32_movmskps256(v));
}

#endif

