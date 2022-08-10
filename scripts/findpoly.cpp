
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
                                  double x, double x0, double x1)
{
  x = (x > 0.0 ? (x < double(n) ? x : double(n)) : 0.0);
  int     n0 = int(std::floor(x - double(k >> 1)));
  n0 = (n0 > 0 ? n0 : 0);
  int     n1 = n0 + k;
  n1 = (n1 < n ? n1 : n);
  n0 = n1 - k;
  for (int i = 0; i <= k; i++)
  {
    m[i * (k + 2)] = 1.0;
    double  xTmp = (double(n0 + i) / double(n)) * (x1 - x0) + x0;
    for (int j = 1; j <= k; j++)
      m[i * (k + 2) + j] = m[i * (k + 2) + (j - 1)] * xTmp;
    m[i * (k + 2) + (k + 1)] = f[n0 + i];
  }
  solveEquationSystem(m, k + 2, k + 1);
  x = (x / double(n)) * (x1 - x0) + x0;
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
    for (int i = 0; i <= k; i++)
      y[i] = interpolateFunction(f, n, k, &(m.front()), p[i], x0, x1);
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
        std::printf(" %11.8f", a[i]);
      }
      std::printf(", err =%11.8f\n", std::sqrt(bestError / double(n + 1)));
    }
    if (pRange > (double(n) * 0.0001))
      pRange = pRange * 0.9999;
  }
}

int main(int argc, char **argv)
{
  int     funcType = 0;                 // 0: lpoly, 1: log2, 2: expsqrt2
  int     k = 5;
  int     n = 512;
  unsigned int  flags = 0x00A5;
  double  x0 = -1.0;
  double  x1 = 1.0;
  if (argc > 1 && std::strcmp(argv[1], "log2") == 0)
  {
    funcType = 1;
    k = 4;
    flags = 0x002D;
    x0 = 1.0;
    x1 = 2.0;
    argc--;
    argv++;
  }
  else if (argc > 1 && std::strcmp(argv[1], "exp2") == 0)
  {
    funcType = 2;
    k = 3;
    flags = 0x006D;
    x0 = 0.0;
    x1 = 1.0;
    argc--;
    argv++;
  }
  std::vector< double > a(size_t(k + 1));
  std::vector< double > f(size_t(n + 1));
  if (funcType == 0)
  {
    double  gamma = 2.2;
    double  ambient1 = 0.2;
    double  ambient2 = 0.4;
    if (argc > 1)
      gamma = std::atof(argv[1]);
    if (argc > 2)
      ambient1 = std::atof(argv[2]);
    if (argc > 3)
      ambient2 = std::atof(argv[3]);
    double  offs = std::pow(ambient1, gamma);
    for (int i = 0; i <= n; i++)
    {
      double  x = (double(i) / double(n)) * (x1 - x0) + x0;
      if (x >= 0.0)
        x = x * (2.0 - ambient2) + ambient2;
      else
        x = (x + 1.0) * (x + 1.0) * (x + 1.0) * (x + 1.0) * ambient2;
      f[i] = std::pow(x * (1.0 - offs) + offs, 1.0 / gamma);
    }
  }
  else if (funcType == 1)
  {
    for (int i = 0; i <= n; i++)
    {
      double  x = (double(i) / double(n)) * (x1 - x0) + x0;
      f[i] = std::log2(x);
    }
  }
  else if (funcType == 2)
  {
    for (int i = 0; i <= n; i++)
    {
      double  x = (double(i) / double(n)) * (x1 - x0) + x0;
      f[i] = std::exp2(x / 2.0);
    }
  }
  findPolynomial(&(a.front()), k, &(f.front()), n, x0, x1, flags);
  return 0;
}

