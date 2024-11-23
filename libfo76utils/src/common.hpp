
#ifndef COMMON_HPP_INCLUDED
#define COMMON_HPP_INCLUDED

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <cstdint>
#include <cmath>
#include <string>
#include <string_view>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <exception>
#include <stdexcept>
#include <bit>

// ENABLE_X86_64_SIMD = 0: do not use GCC x86_64 SIMD extensions
// ENABLE_X86_64_SIMD = 1: use SSE 4.2
// ENABLE_X86_64_SIMD = 2: use AVX
// ENABLE_X86_64_SIMD = 3: use AVX + F16C
// ENABLE_X86_64_SIMD = 4: use Haswell instructions (AVX, F16C, AVX2, FMA, BMI)

#ifndef ENABLE_X86_64_SIMD
#  if defined(__GNUC__) && (defined(__x86_64__) || defined(__x86_64))
#    if defined(__AVX__) && defined(__AVX2__)
#      define ENABLE_X86_64_SIMD    4
#    elif defined(__AVX__) && defined(__F16C__)
#      define ENABLE_X86_64_SIMD    3
#    elif defined(__AVX__)
#      define ENABLE_X86_64_SIMD    2
#    elif defined(__SSE4_2__)
#      define ENABLE_X86_64_SIMD    1
#    else
#      define ENABLE_X86_64_SIMD    0
#    endif
#  else
#    define ENABLE_X86_64_SIMD  0
#  endif
#endif

class FO76UtilsError : public std::exception
{
 protected:
  static const char *defaultErrorMessage;
  const char  *s;
  char    *buf;
  void storeMessage(const char *msg, bool copyFlag) noexcept;
 public:
  FO76UtilsError() noexcept
  {
    s = defaultErrorMessage;
    buf = nullptr;
  }
  FO76UtilsError(const FO76UtilsError& r) noexcept
  {
    storeMessage(r.s, bool(r.buf));
  }
  FO76UtilsError(int copyFlag, const char *msg) noexcept
  {
    storeMessage(msg, bool(copyFlag));
  }
#ifdef __GNUC__
  __attribute__ ((__format__ (__printf__, 2, 3)))
#endif
  FO76UtilsError(const char *fmt, ...) noexcept;
  virtual ~FO76UtilsError() noexcept;
  FO76UtilsError& operator=(const FO76UtilsError& r) noexcept
  {
    storeMessage(r.s, bool(r.buf));
    return (*this);
  }
  virtual const char *what() const noexcept
  {
    return s;
  }
};

[[noreturn]] inline void errorMessage(const char *msg)
{
  throw FO76UtilsError(0, msg);
}

inline int roundFloat(float x)
{
#if ENABLE_X86_64_SIMD >= 2
  int     tmp;
  __asm__ ("vcvtss2si %1, %0" : "=r" (tmp) : "x" (x));
  return tmp;
#else
  return int(std::rintf(x));
#endif
}

inline int roundDouble(double x)
{
#if ENABLE_X86_64_SIMD >= 2
  int     tmp;
  __asm__ ("vcvtsd2si %1, %0" : "=r" (tmp) : "x" (x));
  return tmp;
#else
  return int(std::rint(x));
#endif
}

inline unsigned char floatToUInt8Clamped(float x, float scale)
{
  float   tmp = x * scale;
  tmp = (tmp > 0.0f ? (tmp < 255.0f ? tmp : 255.0f) : 0.0f);
  return (unsigned char) roundFloat(tmp);
}

inline std::int16_t uint16ToSigned(unsigned short x)
{
  return std::int16_t(x);
}

inline std::int32_t uint32ToSigned(unsigned int x)
{
  return std::int32_t(x);
}

inline float convertFloat16(unsigned short n)
{
#if ENABLE_X86_64_SIMD >= 3
  short   tmp __attribute__ ((__vector_size__ (16))) =
  {
    std::int16_t(n), 0, 0, 0, 0, 0, 0, 0
  };
  return __builtin_ia32_vcvtph2ps(tmp)[0];
#elif defined(__i386__) || defined(__x86_64__) || defined(__x86_64)
  std::uint32_t m = (std::uint32_t) int((std::int16_t) n);
  std::uint32_t i = ((m << 13) & 0x8FFFE000U) + 0x38000000U;
  float   r = std::bit_cast< float >(i);
  if (!(m & 0x7C00U)) [[unlikely]]
  {
    // zero or denormal
    i = std::bit_cast< std::uint32_t >(r) & 0xFF800000U;
    r = r - std::bit_cast< float >(i);
    r = r + r;
  }
  return r;
#else
  unsigned char e = (unsigned char) ((n >> 10) & 0x1F);
  if (!e)
    return 0.0f;
  long long m = (long long) ((n & 0x03FF) | 0x0400) << e;
  return (float(!(n & 0x8000) ? m : -m) * (1.0f / 33554432.0f));
#endif
}

#if ENABLE_X86_64_SIMD >= 3
inline std::uint16_t convertToFloat16(float x)
{
  std::uint16_t tmp __attribute__ ((__vector_size__ (16)));
  __asm__ ("vcvtps2ph $0x00, %1, %0" : "=x" (tmp) : "x" (x));
  return tmp[0];
}
#else
std::uint16_t convertToFloat16(float x);
#endif

inline void hashFunctionUInt32(std::uint32_t& h, const std::uint32_t& m)
{
#if ENABLE_X86_64_SIMD
  h = __builtin_ia32_crc32si(h, m);
#else
  std::uint64_t tmp = (h ^ m) * std::uint64_t(0xEE088D97U);
  h = std::uint32_t((tmp + (tmp >> 32)) & 0xFFFFFFFFU);
#endif
}

inline void hashFunctionUInt64(std::uint64_t& h, const std::uint64_t& m)
{
#if ENABLE_X86_64_SIMD
  h = __builtin_ia32_crc32di(h, m);
#else
  const std::uint64_t multValue = 0xEE088D97U;
  h = std::uint32_t((h ^ m) & 0xFFFFFFFFU) * multValue;
  h = h + (h >> 32);
  h = std::uint32_t((h ^ (m >> 32)) & 0xFFFFFFFFU) * multValue;
  h = h + (h >> 32);
#endif
}

std::uint32_t hashFunctionUInt32(const void *p, size_t nBytes);

extern const std::uint32_t crc32Table_EDB88320[256];
#if !ENABLE_X86_64_SIMD
extern const std::uint32_t crc32Table_82F63B78[256];
#endif

inline void hashFunctionCRC32(std::uint32_t& h, unsigned char c)
{
  h = (h >> 8) ^ crc32Table_EDB88320[(h ^ c) & 0xFFU];
}

template< typename T > inline void hashFunctionCRC32C(
    std::uint32_t& h, const T& c)
{
#if ENABLE_X86_64_SIMD
  if (sizeof(T) == 8)
    h = std::uint32_t(__builtin_ia32_crc32di(h, c));
  else if (sizeof(T) == 4)
    h = std::uint32_t(__builtin_ia32_crc32si(h, c));
  else if (sizeof(T) == 2)
    h = std::uint32_t(__builtin_ia32_crc32hi(h, c));
  else if (sizeof(T) == 1)
    h = std::uint32_t(__builtin_ia32_crc32qi(h, c));
#else
  T   b = c;
  for (size_t i = 0; i < sizeof(T); i++, b = b >> 8)
    h = (h >> 8) ^ crc32Table_82F63B78[(h ^ b) & 0xFFU];
#endif
}

inline unsigned long long rgba32ToRBGA64(unsigned int c)
{
  return ((unsigned long long) (c & 0x00FF00FFU)
          | ((unsigned long long) (c & 0xFF00FF00U) << 24));
}

inline unsigned int rbga64ToRGBA32(unsigned long long c)
{
  return (((unsigned int) c & 0x00FF00FFU)
          | ((unsigned int) (c >> 24) & 0xFF00FF00U));
}

inline unsigned long long blendRBGA64(unsigned long long c0,
                                      unsigned long long c1, int f)
{
  unsigned int  m = (unsigned int) f;
  unsigned long long  tmp = (c0 * (256U - m)) + (c1 * m);
  return (((tmp + 0x0080008000800080ULL) >> 8) & 0x00FF00FF00FF00FFULL);
}

inline unsigned long long blendRGBA32ToRBGA64(unsigned int c0,
                                              unsigned int c1, int f)
{
  return blendRBGA64(rgba32ToRBGA64(c0), rgba32ToRBGA64(c1), f);
}

inline unsigned int blendRGBA32(unsigned int c0, unsigned int c1, int f)
{
  unsigned long long  tmp0 = rgba32ToRBGA64(c0);
  unsigned long long  tmp1 = rgba32ToRBGA64(c1);
  unsigned int  m = (unsigned int) f;
  tmp0 = (tmp0 * (256U - m)) + (tmp1 * m) + 0x0080008000800080ULL;
  return (((unsigned int) (tmp0 & 0xFF00FF00U) >> 8)
          | ((unsigned int) (tmp0 >> 32) & 0xFF00FF00U));
}

inline unsigned long long rgb24ToRBG64(unsigned int c)
{
  return ((unsigned long long) ((c & 0x000000FF) | ((c & 0x00FF0000) << 4))
          | ((unsigned long long) (c & 0x0000FF00) << 32));
}

inline unsigned long long blendRBG64(unsigned long long a, unsigned long long b,
                                     unsigned int opacityB)
{
  unsigned long long  tmp = (a << 8) + ((b - a) * opacityB);
  return (((tmp + 0x0000800008000080ULL) >> 8) & 0x000FFF00FFF00FFFULL);
}

inline unsigned char blendDithered(unsigned char a, unsigned char b,
                                   unsigned char opacityB, int x, int y)
{
  static const unsigned char  t[64] =
  {
     0, 48, 12, 60,  3, 51, 15, 63, 32, 16, 44, 28, 35, 19, 47, 31,
     8, 56,  4, 52, 11, 59,  7, 55, 40, 24, 36, 20, 43, 27, 39, 23,
     2, 50, 14, 62,  1, 49, 13, 61, 34, 18, 46, 30, 33, 17, 45, 29,
    10, 58,  6, 54,  9, 57,  5, 53, 42, 26, 38, 22, 41, 25, 37, 21
  };
  unsigned char d = t[((y & 7) << 3) | (x & 7)];
  return (((((unsigned int) opacityB + 2) >> 2) + d) < 64 ? a : b);
}

long parseInteger(const char *s, int base = 0, const char *errMsg = nullptr,
                  long minVal = long((~0UL >> 1) + 1UL),
                  long maxVal = long(~0UL >> 1));

double parseFloat(const char *s, const char *errMsg = nullptr,
                  double minVal = -1.0e38, double maxVal = 1.0e38);

inline std::uint64_t timerFunctionRDTSC()
{
#if defined(__GNUC__) && (defined(__x86_64__) || defined(__x86_64))
  std::uint32_t tmp1, tmp2;
  __asm__ __volatile__ ("lfence\n\t"
                        "rdtsc" : "=a" (tmp1), "=d" (tmp2));
  return ((std::uint64_t(tmp2) << 32) | tmp1);
#else
  return 0UL;
#endif
}

void memsetUInt32(std::uint32_t *p, std::uint32_t c, size_t n);
void memsetUInt64(std::uint64_t *p, std::uint64_t c, size_t n);
void memsetFloat(float *p, float c, size_t n);

#ifdef __GNUC__
__attribute__ ((__format__ (__printf__, 2, 3)))
#endif
void printToString(std::string& s, const char *fmt, ...);

class AllocBuffers
{
 protected:
  struct DataBuf
  {
    std::uint32_t bytesUsed;
    std::uint32_t minCapacity;
    DataBuf *prv;
    inline unsigned char *data()
    {
      return (reinterpret_cast< unsigned char * >(this) + sizeof(DataBuf));
    }
  };
  static const DataBuf  emptyBuf;
  DataBuf *lastBuf;
  DataBuf *allocateBuffer(size_t nBytes);
 public:
  AllocBuffers()
    : lastBuf(const_cast< DataBuf * >(&emptyBuf))
  {
  }
  AllocBuffers(size_t capacity);
  ~AllocBuffers();
  void *allocateSpace(size_t nBytes, size_t alignBytes = 16);
  void clear();
  template< typename T > inline T *allocateObject()
  {
    return reinterpret_cast< T * >(allocateSpace(sizeof(T), alignof(T)));
  }
  template< typename T > inline T *constructObject()
  {
    return new(allocateObject< T >()) T();
  }
  template< typename T > inline T *allocateObjects(size_t n)
  {
    return reinterpret_cast< T * >(allocateSpace(sizeof(T) * n, alignof(T)));
  }
};

// NOTE: 'len' should include the terminating null character if there is one
void convertStringToUInt16(std::uint16_t *dst, const char *src, size_t len);

#endif

