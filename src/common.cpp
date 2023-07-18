
#include "common.hpp"

const char * FO76UtilsError::defaultErrorMessage = "unknown error";

void FO76UtilsError::storeMessage(const char *msg, bool copyFlag) noexcept
{
  if (!msg || msg[0] == '\0')
  {
    s = defaultErrorMessage;
    buf = (char *) 0;
  }
  else if (!copyFlag)
  {
    s = msg;
    buf = (char *) 0;
  }
  else
  {
    size_t  n = std::strlen(msg) + 1;
    buf = reinterpret_cast< char * >(std::malloc(n));
    if (!buf)
    {
      s = "memory allocation error";
    }
    else
    {
      std::memcpy(buf, msg, n * sizeof(char));
      s = buf;
    }
  }
}

#ifdef __GNUC__
__attribute__ ((__format__ (__printf__, 2, 3)))
#endif
FO76UtilsError::FO76UtilsError(const char *fmt, ...) noexcept
{
  char    tmpBuf[2048];
  std::va_list  ap;
  va_start(ap, fmt);
  std::vsnprintf(tmpBuf, 2048, fmt, ap);
  va_end(ap);
  tmpBuf[2047] = '\0';
  storeMessage(tmpBuf, true);
}

FO76UtilsError::~FO76UtilsError() noexcept
{
  if (buf)
    std::free(buf);
}

#if !ENABLE_X86_64_AVX2
std::uint16_t convertToFloat16(float x)
{
#  if defined(__i386__) || defined(__x86_64__) || defined(__x86_64)
  union
  {
    float   f;
    std::uint32_t i;
  }
  tmp;
  tmp.f = x;
  std::uint32_t n = tmp.i;
  std::uint32_t s = (n & 0x80000000U) >> 16;
  n = n & 0x7FFFFFFFU;
  if (n < 0x39000000U)                  // zero or denormal
  {
    tmp.i = n;
    return std::uint16_t(std::uint32_t(roundFloat(tmp.f * float(1 << 24))) | s);
  }
  n = (n - 0x37FFF000U) >> 13;
  n = (n < 0x7FFFU ? n : 0x7FFFU);
  return std::uint16_t(n | s);
#  else
  int     e = 0;
  int     m = roundFloat(float(std::frexp(x, &e)) * 2048.0f);
  if (!m)
    return 0;
  int     s = m & 0x8000;
  m = std::abs(m);
  e = e + 14 + (m >> 11);
  if (e <= 0 || e > 31)
    return std::uint16_t(s | (e <= 0 ? 0x0000 : 0x7FFF));
  return std::uint16_t(s | (e << 10) | (m & 0x03FF));
#  endif
}
#endif

long parseInteger(const char *s, int base, const char *errMsg,
                  long minVal, long maxVal)
{
  char    *endp = (char *) 0;
  long    tmp = long(std::strtoll(s, &endp, base));
  if (!endp || endp == s || *endp != '\0' || !(tmp >= minVal && tmp <= maxVal))
  {
    if (!errMsg)
      errMsg = "invalid integer argument";
    throw FO76UtilsError("%s: %s", errMsg, s);
  }
  return tmp;
}

double parseFloat(const char *s, const char *errMsg,
                  double minVal, double maxVal)
{
  char    *endp = (char *) 0;
  double  tmp = std::strtod(s, &endp);
  if (!endp || endp == s || *endp != '\0' || !(tmp >= minVal && tmp <= maxVal))
  {
    if (!errMsg)
      errMsg = "invalid floating point argument";
    throw FO76UtilsError("%s: %s", errMsg, s);
  }
  return tmp;
}

