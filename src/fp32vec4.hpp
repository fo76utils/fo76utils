
#ifndef FP32VEC4_HPP_INCLUDED
#define FP32VEC4_HPP_INCLUDED

struct FloatVector4
{
#if defined(__GNUC__) && (defined(__x86_64__) || defined(__x86_64))
  float   v __attribute__ ((__vector_size__ (16)));
  FloatVector4(unsigned int c)
    : v { float(int(c & 0xFF)), float(int((c >> 8) & 0xFF)),
          float(int((c >> 16) & 0xFF)), float(int((c >> 24) & 0xFF)) }
  {
  }
  FloatVector4(float v0, float v1, float v2, float v3)
    : v { v0, v1, v2, v3 }
  {
  }
  inline float operator[](size_t n) const
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
#endif
  inline operator unsigned int() const
  {
    return ((unsigned int) roundFloat(v[0])
            | ((unsigned int) roundFloat(v[1]) << 8)
            | ((unsigned int) roundFloat(v[2]) << 16)
            | ((unsigned int) roundFloat(v[3]) << 24));
  }
};

#endif

