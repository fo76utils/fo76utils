
#ifndef FP32VEC4_HPP_INCLUDED
#define FP32VEC4_HPP_INCLUDED

struct FloatVector4
{
#if defined(__GNUC__) && (defined(__x86_64__) || defined(__x86_64))
  float   v __attribute__ ((__vector_size__ (16)));
  FloatVector4(unsigned int c)
  {
    float   tmp __attribute__ ((__vector_size__ (16)));
    __asm__ ("vmovd %1, %0\n\t"
             "vpmovzxbd %0, %0\n\t"
             "vcvtdq2ps %0, %0" : "=x" (tmp) : "rm" (c));
    v = tmp;
  }
  FloatVector4(float v0, float v1, float v2, float v3)
    : v { v0, v1, v2, v3 }
  {
  }
  inline float operator[](size_t n) const
  {
    return v[n];
  }
  inline void setElement(size_t n, float x)
  {
    v[n] = x;
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
    float   tmp __attribute__ ((__vector_size__ (16)));
    __asm__ ("vdpps $0xf1, %1, %2, %0" : "=x" (tmp) : "xm" (r.v), "x" (v));
    return tmp[0];
  }
  inline void squareRoot()
  {
    float   tmp __attribute__ ((__vector_size__ (16))) =
    {
      0.0f, 0.0f, 0.0f, 0.0f
    };
    __asm__ ("vmaxps %0, %1, %1\n\t"
             "vsqrtps %1, %0" : "+x" (v), "+x" (tmp));
  }
  static inline float squareRootFast(float x)
  {
    float   tmp1, tmp2;
    const float tmp3 =
        1.0f / (float(0x0000040000000000LL) * float(0x0000040000000000LL));
    __asm__ (
        "vmaxss %2, %3, %1\n\t"
        "vrsqrtss %1, %1, %0\n\t"
        "vmulss %1, %0, %0" : "=x" (tmp1), "=x" (tmp2) : "xm" (tmp3), "x" (x));
    return tmp1;
  }
  static inline float log2Fast(float x)
  {
    union
    {
      float   f;
      unsigned int  n;
    }
    tmp;
    tmp.f = x;
    unsigned int  n = tmp.n;
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
    union
    {
      float   f;
      unsigned int  n;
    }
    tmp;
    tmp.f = float(x);
    return (int(tmp.n >> 23) - 127);
  }
  static inline float exp2Fast(float x)
  {
    int     e = int(std::floor(x));
    float   m = x - float(e);
    float   m2 = m * m;
    e = e << 23;
    float   tmp;
    __asm__ ("vmovd %1, %0" : "=x" (tmp) : "r" (e));
    FloatVector4  tmp2(1.0f, m, m2, m * m2);
    FloatVector4  tmp3(1.00000000f, 0.34671664f, 0.05924474f, 0.00825060f);
    m = tmp2.dotProduct(tmp3);
    m = m * m;
    __asm__ ("vpaddd %1, %0, %0" : "+x" (m) : "x" (tmp));
    return m;
  }
  inline void normalize(bool invFlag = false)
  {
    static const float  invMultTable[2] = { -0.5f, 0.5f };
    float   tmp = dotProduct(*this);
    float   tmp2 =
        1.0f / (float(0x0000040000000000LL) * float(0x0000040000000000LL));
    __asm__ ("vmaxss %0, %1, %0\n\t"
             "vrsqrtss %0, %0, %0" : "+x" (tmp2) : "x" (tmp));
    v *= ((tmp * tmp2 * tmp2 - 3.0f) * (tmp2 * invMultTable[int(invFlag)]));
  }
  inline void normalizeFast()
  {
    float   tmp = dotProduct(*this);
    const float tmp2 =
        1.0f / (float(0x0000040000000000LL) * float(0x0000040000000000LL));
    __asm__ ("vmaxss %1, %0, %0\n\t"
             "vrsqrtss %0, %0, %0" : "+x" (tmp) : "xm" (tmp2));
    v *= tmp;
  }
  inline void minValues(const FloatVector4& r)
  {
    __asm__ ("vminps %1, %0, %0" : "+x" (v) : "xm" (r.v));
  }
  inline void maxValues(const FloatVector4& r)
  {
    __asm__ ("vmaxps %1, %0, %0" : "+x" (v) : "xm" (r.v));
  }
  inline operator unsigned int() const
  {
    float   tmp __attribute__ ((__vector_size__ (16))) = v;
    unsigned int  c;
    __asm__ ("vcvtps2dq %1, %1\n\t"
             "vpackusdw %1, %1, %1\n\t"
             "vpackuswb %1, %1, %1\n\t"
             "vmovd %1, %0" : "=r" (c), "+x" (tmp));
    return c;
  }
#else
  float   v[4];
  FloatVector4(unsigned int c)
  {
    v[0] = float(int(c & 0xFF));
    v[1] = float(int((c >> 8) & 0xFF));
    v[2] = float(int((c >> 16) & 0xFF));
    v[3] = float(int((c >> 24) & 0xFF));
  }
  FloatVector4(float v0, float v1, float v2, float v3)
  {
    v[0] = v0;
    v[1] = v1;
    v[2] = v2;
    v[3] = v3;
  }
  inline const float& operator[](size_t n) const
  {
    return v[n];
  }
  inline void setElement(size_t n, float x)
  {
    v[n] = x;
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
  inline void squareRoot()
  {
    v[0] = (v[0] > 0.0f ? float(std::sqrt(v[0])) : 0.0f);
    v[1] = (v[1] > 0.0f ? float(std::sqrt(v[1])) : 0.0f);
    v[2] = (v[2] > 0.0f ? float(std::sqrt(v[2])) : 0.0f);
    v[3] = (v[3] > 0.0f ? float(std::sqrt(v[3])) : 0.0f);
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
  inline void normalize(bool invFlag = false)
  {
    float   tmp = dotProduct(*this);
    if (tmp > 0.0f)
    {
      tmp = (!invFlag ? 1.0f : -1.0f) / float(std::sqrt(tmp));
      *this *= tmp;
    }
  }
  inline void normalizeFast()
  {
    float   tmp = dotProduct(*this);
    if (tmp > 0.0f)
      *this *= (1.0f / float(std::sqrt(tmp)));
  }
  inline void minValues(const FloatVector4& r)
  {
    v[0] = (r.v[0] < v[0] ? r.v[0] : v[0]);
    v[1] = (r.v[1] < v[1] ? r.v[1] : v[1]);
    v[2] = (r.v[2] < v[2] ? r.v[2] : v[2]);
    v[3] = (r.v[3] < v[3] ? r.v[3] : v[3]);
  }
  inline void maxValues(const FloatVector4& r)
  {
    v[0] = (r.v[0] > v[0] ? r.v[0] : v[0]);
    v[1] = (r.v[1] > v[1] ? r.v[1] : v[1]);
    v[2] = (r.v[2] > v[2] ? r.v[2] : v[2]);
    v[3] = (r.v[3] > v[3] ? r.v[3] : v[3]);
  }
  inline operator unsigned int() const
  {
    int     c0 = roundFloat(v[0]);
    int     c1 = roundFloat(v[1]);
    int     c2 = roundFloat(v[2]);
    int     c3 = roundFloat(v[3]);
    c0 = (c0 > 0 ? (c0 < 255 ? c0 : 255) : 0);
    c1 = (c1 > 0 ? (c1 < 255 ? c1 : 255) : 0);
    c2 = (c2 > 0 ? (c2 < 255 ? c2 : 255) : 0);
    c3 = (c3 > 0 ? (c3 < 255 ? c3 : 255) : 0);
    return ((unsigned int) c0 | ((unsigned int) c1 << 8)
            | ((unsigned int) c2 << 16) | ((unsigned int) c3 << 24));
  }
#endif
};

#endif

