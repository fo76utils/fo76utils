
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
             "vcvtdq2ps %0, %0" : "=x" (tmp) : "r" (c));
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
    float   tmp __attribute__ ((__vector_size__ (16))) = v;
    float   tmp2 __attribute__ ((__vector_size__ (16))) = r.v;
    __asm__ ("vdpps $0xf1, %0, %1, %0" : "+x" (tmp) : "x" (tmp2));
    return tmp[0];
  }
  inline void squareRoot()
  {
    float   tmp __attribute__ ((__vector_size__ (16))) = v;
    float   tmp2 __attribute__ ((__vector_size__ (16)));
    __asm__ ("vxorps %1, %1, %1\n\t"
             "vmaxps %0, %1, %0\n\t"
             "vsqrtps %0, %0" : "+x" (tmp), "=x" (tmp2));
    v = tmp;
  }
  inline void normalize(bool invFlag = false)
  {
    float   tmp = dotProduct(*this);
    if (__builtin_expect(long(tmp > 0.0f), 1L))
    {
      float   tmp2;
      __asm__ ("vrsqrtss %1, %1, %0" : "=x" (tmp2) : "x" (tmp));
      v *= ((tmp * tmp2 * tmp2 - 3.0f) * (tmp2 * (!invFlag ? -0.5f : 0.5f)));
    }
  }
  inline void normalizeFast()
  {
    float   tmp = dotProduct(*this);
    if (__builtin_expect(long(tmp > 0.0f), 1L))
    {
      float   tmp2;
      __asm__ ("vrsqrtss %1, %1, %0" : "=x" (tmp2) : "x" (tmp));
      v *= tmp2;
    }
  }
  inline void minValues(const FloatVector4& r)
  {
    float   tmp __attribute__ ((__vector_size__ (16))) = v;
    float   tmp2 __attribute__ ((__vector_size__ (16))) = r.v;
    __asm__ ("vminps %0, %1, %0" : "+x" (tmp) : "x" (tmp2));
    v = tmp;
  }
  inline void maxValues(const FloatVector4& r)
  {
    float   tmp __attribute__ ((__vector_size__ (16))) = v;
    float   tmp2 __attribute__ ((__vector_size__ (16))) = r.v;
    __asm__ ("vmaxps %0, %1, %0" : "+x" (tmp) : "x" (tmp2));
    v = tmp;
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

