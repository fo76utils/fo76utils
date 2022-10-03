
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>

// flags & 1:   multiply error near x0
// flags & 2:   multiply error near (x0 + x1) / 2
// flags & 4:   multiply error near x1
// flags & 8:   high multiplier for bits 0 to 2 (100 instead of 10)
// flags & 16:  monotonically decreasing function if bit 5 is set
// flags & 32:  require monotonic function

static double calculateError(const double *f, int n, const double *a, int k,
                             double x0, double x1, double maxErr,
                             unsigned int flags)
{
  double  err = 0.0;
  double  prv = (!(flags & 0x10) ? -1000000.0 : 1000000.0);
  for (int i = 0; i <= n; i++)
  {
    double  x = (double(i) / double(n)) * (x1 - x0) + x0;
    double  y = f[i];
    double  z = 0.0;
    switch (k)
    {
      case 8:
        z = z * x + a[8];
      case 7:
        z = z * x + a[7];
      case 6:
        z = z * x + a[6];
      case 5:
        z = z * x + a[5];
      case 4:
        z = z * x + a[4];
      case 3:
        z = z * x + a[3];
      case 2:
        z = z * x + a[2];
      case 1:
        z = z * x + a[1];
      case 0:
        z = z * x + a[0];
        break;
    }
    double  tmpErr = (z - y) * (z - y);
    if (((flags & 0x09) == 0x01 && i < (n >> 3)) ||
        ((flags & 0x0A) == 0x02 && std::abs(i - (n >> 1)) <= (n >> 4)) ||
        ((flags & 0x0C) == 0x04 && (n - i) < (n >> 3)))
    {
      tmpErr *= 10.0;
    }
    if (((flags & 0x09) == 0x09 && i < (n >> 6)) ||
        ((flags & 0x0A) == 0x0A && std::abs(i - (n >> 1)) <= (n >> 7)) ||
        ((flags & 0x0C) == 0x0C && (n - i) < (n >> 6)))
    {
      tmpErr *= 100.0;
    }
    err += tmpErr;
    if (err > maxErr)
      return 1000000000.0;
    if (((flags & 0x30) == 0x20 && z < prv) ||
        ((flags & 0x30) == 0x30 && z > prv))
    {
      err += 1.0;
    }
    prv = z;
  }
  return err;
}

static void solveEquationSystem(double *m, int w, int h)
{
  // Gaussian elimination
  for (int i = 0; i < h; i++)
  {
    double  a = m[i * w + i];
    int     l = i;
    for (int j = i + 1; j < h; j++)
    {
      if (std::fabs(m[j * w + i]) > std::fabs(a))
      {
        a = m[j * w + i];
        l = j;
      }
    }
    if (l != i)
    {
      for (int j = 0; j < w; j++)
      {
        double  tmp = m[i * w + j];
        m[i * w + j] = m[l * w + j];
        m[l * w + j] = tmp;
      }
    }
    for (int j = 0; j < w; j++)
      m[i * w + j] = m[i * w + j] / a;
    m[i * w + i] = 1.0;
    for (int j = i + 1; j < h; j++)
    {
      a = m[j * w + i];
      for (int k = 0; k < w; k++)
        m[j * w + k] = m[j * w + k] - (m[i * w + k] * a);
      m[j * w + i] = 0.0;
    }
  }
  for (int i = h; --i >= 0; )
  {
    for (int j = i - 1; j >= 0; j--)
    {
      double  a = m[j * w + i];
      for (int k = 0; k < w; k++)
        m[j * w + k] = m[j * w + k] - (m[i * w + k] * a);
      m[j * w + i] = 0.0;
    }
  }
}

static double interpolateFunction(const double *f, int n, int k, double *m,
                                  double x)
{
  int     n0 = int(std::floor(x - double(k >> 1)));
  n0 = (n0 > 0 ? n0 : 0);
  int     n1 = n0 + k;
  n1 = (n1 < n ? n1 : n);
  n0 = n1 - k;
  x = x - double(n0);
  for (int i = 0; i <= k; i++)
  {
    m[i * (k + 2)] = 1.0;
    for (int j = 1; j <= k; j++)
      m[i * (k + 2) + j] = m[i * (k + 2) + (j - 1)] * double(i);
    m[i * (k + 2) + (k + 1)] = f[n0 + i];
  }
  solveEquationSystem(m, k + 2, k + 1);
  double  y = 0.0;
  for (int i = k; i >= 0; i--)
    y = y * x + m[i * (k + 2) + (k + 1)];
  return y;
}

static double getRandom(double minVal, double maxVal)
{
  static unsigned int seed = 0x7FFFFFFEU;
  seed = (unsigned int) ((seed * 16807ULL) % 0x7FFFFFFFU);
  double  x = double(int(seed) - 1) / double(0x7FFFFFFD);
  return ((maxVal - minVal) * x + minVal);
}

// flags & 1:   multiply error near x0
// flags & 2:   multiply error near (x0 + x1) / 2
// flags & 4:   multiply error near x1
// flags & 8:   high multiplier for bits 0 to 2 (100 instead of 10)
// flags & 16:  monotonically decreasing function if bit 5 is set
// flags & 32:  require monotonic function
// flags & 64:  require exact match at x0
// flags & 128: require exact match at (x0 + x1) / 2
// flags & 256: require exact match at x1

static void findPolynomial(double *a, int k, const double *f, int n,
                           double x0, double x1, unsigned int flags,
                           size_t maxIterations = 0x7FFFFFFF)
{
  if (k > n)
    return;
  std::vector< double > m(size_t((k + 2) * (k + 1)), 0.0);
  std::vector< double > p(size_t(k + 1));
  std::vector< double > bestP(size_t(k + 1), double(n >> 1));
  std::vector< double > y(size_t(k + 1));
  std::vector< double > aTmp(size_t(k + 1), 0.0);
  double  bestError = 1000000000.0;
  double  pRange = double(n);
  while (maxIterations > 0 && bestError > 0.0)
  {
    for (int i = 0; i <= k; i++)
    {
      if ((flags & 0x0040) && i == 0)
        p[i] = 0.0;
      else if ((flags & 0x0080) && i == 1)
        p[i] = double(n >> 1);
      else if ((flags & 0x0100) && i == 2)
        p[i] = double(n);
      else
        p[i] = bestP[i] + getRandom(-pRange, pRange);
      p[i] = (p[i] > 0.0 ? (p[i] < double(n) ? p[i] : double(n)) : 0.0);
    }
    double  minDist = double(n);
    for (int i = 0; i <= k; i++)
    {
      for (int j = i + 1; j <= k; j++)
      {
        double  d = std::fabs(p[j] - p[i]);
        minDist = (d < minDist ? d : minDist);
      }
    }
    if (minDist < 1.0)
      continue;
    maxIterations--;
    if (pRange > (double(n) * 0.0001))
      pRange = pRange * 0.9999;
    for (int i = 0; i <= k; i++)
      y[i] = interpolateFunction(f, n, k, &(m.front()), p[i]);
    for (int i = 0; i <= k; i++)
    {
      m[i * (k + 2)] = 1.0;
      double  x = (p[i] / double(n)) * (x1 - x0) + x0;
      for (int j = 1; j <= k; j++)
        m[i * (k + 2) + j] = m[i * (k + 2) + (j - 1)] * x;
      m[i * (k + 2) + (k + 1)] = y[i];
    }
    solveEquationSystem(&(m.front()), k + 2, k + 1);
    for (int i = 0; i <= k; i++)
      aTmp[i] = m[i * (k + 2) + (k + 1)];
    double  err =
        calculateError(f, n, &(aTmp.front()), k, x0, x1, bestError, flags);
    if ((err + 0.000000000001) < bestError)
    {
      bestError = err;
      for (int i = k; i >= 0; i--)
      {
        a[i] = aTmp[i];
        bestP[i] = p[i];
      }
      if (maxIterations > 0 && maxIterations < 0x10000000)
        continue;
    }
    else if (maxIterations > 0)
    {
      continue;
    }
    for (int i = k; i >= 0; i--)
      std::printf(" %11.8f", a[i]);
    std::printf(", err =%11.8f\n", std::sqrt(bestError / double(n + 1)));
  }
}

static inline double fresnel(double x, double n1, double n2)
{
  // x = NdotV
  if (!(x > 0.0))
    return 1.0;
  if (!(x < 1.0))
    return (((n2 - n1) * (n2 - n1)) / ((n1 + n2) * (n1 + n2)));
  // sqrt(1.0 - ((n1 / n2) * sin(acos(x)))^2)
  double  y = std::sqrt(1.0 - ((n1 / n2) * (n1 / n2) * (1.0 - (x * x))));
  double  r_s = ((n1 * x) - (n2 * y)) / ((n1 * x) + (n2 * y));
  double  r_p = ((n1 * y) - (n2 * x)) / ((n1 * y) + (n2 * x));
  return (((r_s * r_s) + (r_p * r_p)) * 0.5);
}

static double fresnelGGX(double v_r, double n1, double n2, double r)
{
  // v_r = NdotV, r = roughness
  v_r = (v_r > 0.0000001 ? (v_r < 0.9999999 ? v_r : 0.9999999) : 0.0000001);
  double  v_i = std::sqrt(1.0 - (v_r * v_r));
  if (r < (1.0 / 64.0))
    r = 1.0 / 64.0;
  else if (r > 1.0)
    r = 1.0;
  double  a2 = r * r * r * r;
  double  k = (r + 1.0) * (r + 1.0) * 0.125;
  double  g = v_r / (v_r * (1.0 - k) + k);
  double  s = 0.0;
  double  w = 0.0;
  double  pi = std::atan(1.0) * 4.0;
  int     i = 4096;
  double  f_r = std::cos(pi * 2.0 / double(i));
  double  f_i = std::sin(pi * 2.0 / double(i));
  double  l_r = 1.0;
  double  l_i = 0.0;
  while (--i >= 0)
  {
    double  h_r = v_r + l_r;
    double  h_i = v_i + l_i;
    double  tmp = (h_r * h_r) + (h_i * h_i);
    if (tmp > 0.0)
      tmp = 1.0 / std::sqrt(tmp);
    h_r *= tmp;
    h_i *= tmp;
    double  nDotL = (l_r > 0.0 ? l_r : 0.0);
    double  nDotH = (h_r > 0.0 ? h_r : 0.0);
    double  vDotH = std::fabs((v_r * h_r) + (v_i * h_i));
    double  d = nDotH * nDotH * (a2 - 1.0) + 1.0;
    d = a2 / (pi * d * d);
    d = d * g * (nDotL / (nDotL * (1.0 - k) + k));
    w += d;
    s += (fresnel(vDotH, n1, n2) * d);
    tmp = (l_r * f_r) - (l_i * f_i);
    l_i = (l_r * f_i) + (l_i * f_r);
    l_r = tmp;
  }
  return (s / w);
}

struct FunctionDesc
{
  const char  *name;
  int     type;
  int     k;
  int     n;
  unsigned int  flags;
  double  x0;
  double  x1;
};

static const FunctionDesc functionTable[10] =
{
  // name,           type, k,    n,  flags,   x0,  x1
  { "lpoly",            0, 5,  512, 0x00A5, -1.0, 1.0   },
  { "log2",             1, 4,  512, 0x002D,  1.0, 2.0   },
  { "exp2",             2, 3,  512, 0x006D,  0.0, 1.0   },
  { "fresnel",          3, 3,  512, 0x0175,  0.0, 1.0   },
  { "fresnel_n",        4, 3,  512, 0x0175,  0.0, 1.0   },
  { "fresnelggx",       5, 3,  512, 0x0139,  0.0, 1.0   },
  { "srgb_expand",      6, 2,  512, 0x0160,  0.0, 1.0   },
  { "srgb_compress",    7, 2,  512, 0x0160,  0.0, 1.0   },
  { "srgb_exp_inv",     8, 3,  512, 0x0160,  0.0, 1.0   },
  { (char *) 0,         0, 0,    0,      0,  0.0, 0.0   }
};

int main(int argc, char **argv)
{
  int     funcType = 0;
  if (argc > 1)
  {
    for (int i = 0; functionTable[i].name; i++)
    {
      if (std::strcmp(argv[1], functionTable[i].name) == 0)
      {
        funcType = i;
        argc--;
        argv++;
        break;
      }
    }
  }
  int     k = functionTable[funcType].k;
  int     n = functionTable[funcType].n;
  unsigned int  flags = functionTable[funcType].flags;
  double  x0 = functionTable[funcType].x0;
  double  x1 = functionTable[funcType].x1;
  funcType = functionTable[funcType].type;
  // gamma or Fresnel n2 (water: 1.3325, glass: 1.5)
  double  arg1 = (funcType == 0 ? 2.2 : 1.5);
  // ambient1 or roughness
  double  arg2 = (funcType == 0 ? 0.2 : 0.5);
  // ambient2
  double  arg3 = 0.4;
  if (argc > 1)
    arg1 = std::atof(argv[1]);
  if (argc > 2)
    arg2 = std::atof(argv[2]);
  if (argc > 3)
    arg3 = std::atof(argv[3]);
  std::vector< double > a(size_t(k + 1));
  std::vector< double > f(size_t(n + 1));
  for (int i = 0; i <= n; i++)
  {
    double  x = (double(i) / double(n)) * (x1 - x0) + x0;
    double  y = 0.0;
    switch (funcType)
    {
      case 0:                           // lpoly
        {
          double  offs = std::pow(arg2, arg1);
          if (x >= 0.0)
            x = x * (2.0 - arg3) + arg3;
          else
            x = (x + 1.0) * (x + 1.0) * (x + 1.0) * (x + 1.0) * arg3;
          y = std::pow(x * (1.0 - offs) + offs, 1.0 / arg1);
        }
        break;
      case 1:                           // log2
        y = std::log2(x);
        break;
      case 2:                           // expsqrt2
        y = std::exp2(x / 2.0);
        break;
      case 3:                           // sqrt(Fresnel)
        {
          double  r = fresnel(x, 1.0, arg1);
          y = std::sqrt(r > 0.0 ? (r < 1.0 ? r : 1.0) : 0.0);
        }
        break;
      case 4:                           // sqrt(Fresnel normalized to 0.0 - 1.0)
        {
          double  f0 = fresnel(1.0, 1.0, arg1);
          double  r = fresnel(x, 1.0, arg1);
          r = (r - f0) / (1.0 - f0);
          y = std::sqrt(r > 0.0 ? (r < 1.0 ? r : 1.0) : 0.0);
        }
        break;
      case 5:                           // sqrt(fresnelggx)
        {
          double  f0 = fresnel(1.0, 1.0, arg1);
          double  r = fresnelGGX(x, 1.0, arg1, arg2);
          r = (r - f0) / (1.0 - f0);
          y = std::sqrt(r > 0.0 ? (r < 1.0 ? r : 1.0) : 0.0);
        }
        break;
      case 6:                           // srgb_expand
        y = std::sqrt(x > 0.04045 ?
                      std::pow((x + 0.055) / 1.055, 2.4) : (x / 12.92));
        break;
      case 7:                           // srgb_compress
        y = ((x * x) > 0.0031308 ?
             (std::pow(x * x, 1.0 / 2.4) * 1.055 - 0.055) : (x * x * 12.92));
        break;
      case 8:                           // srgb_exp_inv
        y = (std::sqrt(0.86054450 * 0.86054450 + (4.0 * 0.13945550 * x))
             - 0.86054450)
            / (2.0 * 0.13945550);
        break;
    }
    f[i] = y;
  }
  if (funcType == 5)
    findPolynomial(&(a.front()), k, &(f.front()), n, x0, x1, flags, 1000000);
  else
    findPolynomial(&(a.front()), k, &(f.front()), n, x0, x1, flags);
  return 0;
}

