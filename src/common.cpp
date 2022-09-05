
#include "common.hpp"

std::runtime_error errorMessage(const char *fmt, ...)
{
  char    buf[512];
  std::va_list  ap;
  va_start(ap, fmt);
  std::vsnprintf(buf, 512, fmt, ap);
  va_end(ap);
  buf[511] = '\0';
  return std::runtime_error(std::string(buf));
}

std::uint16_t convertToFloat16(float x)
{
#if defined(__i386__) || defined(__x86_64__) || defined(__x86_64)
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
#else
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
#endif
}

long parseInteger(const char *s, int base, const char *errMsg,
                  long minVal, long maxVal)
{
  char    *endp = (char *) 0;
  long    tmp = long(std::strtoll(s, &endp, base));
  if (!endp || endp == s || *endp != '\0' || !(tmp >= minVal && tmp <= maxVal))
  {
    if (!errMsg)
      errMsg = "invalid integer argument";
    throw errorMessage("%s: %s", errMsg, s);
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
    throw errorMessage("%s: %s", errMsg, s);
  }
  return tmp;
}

