
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

