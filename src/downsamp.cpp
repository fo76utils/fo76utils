
#include "common.hpp"
#include "downsamp.hpp"

#include <thread>

static const float  downsample2xFilterTable[5] =
{
  // x = 0, 1, 3, 5, 7
  // pow(cos(x * PI / 18.0), 1.87795546) * sin(x * PI / 2.0) / (x * PI / 2.0)
  1.00000000f, 0.61857799f, -0.16197358f, 0.05552255f, -0.01212696f
};

static inline FloatVector4 downsample2xFunc_R8G8B8A8(
    const std::uint32_t * const *inBufPtrs, int x)
{
  FloatVector4  c, c1, c2;
  c = FloatVector4(inBufPtrs[0] + x).srgbExpand();
  c1 = FloatVector4(inBufPtrs[1] + x);
  c2 = FloatVector4(inBufPtrs[2] + x);
  c += ((c1.srgbExpand() + c2.srgbExpand()) * downsample2xFilterTable[1]);
  c1 = FloatVector4(inBufPtrs[3] + x);
  c2 = FloatVector4(inBufPtrs[4] + x);
  c += ((c1.srgbExpand() + c2.srgbExpand()) * downsample2xFilterTable[2]);
  c1 = FloatVector4(inBufPtrs[5] + x);
  c2 = FloatVector4(inBufPtrs[6] + x);
  c += ((c1.srgbExpand() + c2.srgbExpand()) * downsample2xFilterTable[3]);
  c1 = FloatVector4(inBufPtrs[7] + x);
  c2 = FloatVector4(inBufPtrs[8] + x);
  c += ((c1.srgbExpand() + c2.srgbExpand()) * downsample2xFilterTable[4]);
  return c;
}

static inline FloatVector4 downsample2xFunc_A2R10G10B10(
    const std::uint32_t * const *inBufPtrs, int x)
{
  FloatVector4  c, c1, c2;
  c = FloatVector4::convertA2R10G10B10(inBufPtrs[0][x], true);
  c1 = FloatVector4::convertA2R10G10B10(inBufPtrs[1][x], true);
  c2 = FloatVector4::convertA2R10G10B10(inBufPtrs[2][x], true);
  c += ((c1 + c2) * downsample2xFilterTable[1]);
  c1 = FloatVector4::convertA2R10G10B10(inBufPtrs[3][x], true);
  c2 = FloatVector4::convertA2R10G10B10(inBufPtrs[4][x], true);
  c += ((c1 + c2) * downsample2xFilterTable[2]);
  c1 = FloatVector4::convertA2R10G10B10(inBufPtrs[5][x], true);
  c2 = FloatVector4::convertA2R10G10B10(inBufPtrs[6][x], true);
  c += ((c1 + c2) * downsample2xFilterTable[3]);
  c1 = FloatVector4::convertA2R10G10B10(inBufPtrs[7][x], true);
  c2 = FloatVector4::convertA2R10G10B10(inBufPtrs[8][x], true);
  c += ((c1 + c2) * downsample2xFilterTable[4]);
  return c;
}

void downsample2xFilter_Line(std::uint32_t *linePtr, const std::uint32_t *inBuf,
                             int imageWidth, int imageHeight, int y,
                             unsigned char fmtFlags)
{
  // buffer for vertically filtered line data
  FloatVector4  tmpBuf[15];
  const std::uint32_t *inBufPtrs[9];
  inBufPtrs[0] = inBuf + (size_t(y) * size_t(imageWidth));
  for (int i = 0; i < 8; i = i + 2)
  {
    int     yc = y - (i + 1);
    yc = (yc > 0 ? yc : 0);
    inBufPtrs[i + 1] = inBuf + (size_t(yc) * size_t(imageWidth));
    yc = y + (i + 1);
    yc = (yc < (imageHeight - 1) ? yc : (imageHeight - 1));
    inBufPtrs[i + 2] = inBuf + (size_t(yc) * size_t(imageWidth));
  }
  int     xc = 0;
  for (int i = 7; i < 15; i++, xc += int(xc < (imageWidth - 1)))
  {
    if (!(fmtFlags & 1))
      tmpBuf[i] = downsample2xFunc_R8G8B8A8(inBufPtrs, xc);
    else
      tmpBuf[i] = downsample2xFunc_A2R10G10B10(inBufPtrs, xc);
  }
  for (int i = 0; i < 7; i++)
    tmpBuf[i] = tmpBuf[7];
  for (int i = 0; i < imageWidth; i = i + 2, linePtr++)
  {
    FloatVector4  c(tmpBuf[7]);
    c += ((tmpBuf[6] + tmpBuf[8]) * downsample2xFilterTable[1]);
    c += ((tmpBuf[4] + tmpBuf[10]) * downsample2xFilterTable[2]);
    c += ((tmpBuf[2] + tmpBuf[12]) * downsample2xFilterTable[3]);
    c += ((tmpBuf[0] + tmpBuf[14]) * downsample2xFilterTable[4]);
    c.maxValues(FloatVector4(1.0f / (float(1LL << 42) * float(1LL << 42))));
    for (int j = 0; j < 12; j = j + 4)
    {
      tmpBuf[j] = tmpBuf[j + 2];
      tmpBuf[j + 1] = tmpBuf[j + 3];
      tmpBuf[j + 2] = tmpBuf[j + 4];
      tmpBuf[j + 3] = tmpBuf[j + 5];
    }
    tmpBuf[12] = tmpBuf[14];
    // convert to sRGB color space
    FloatVector4  tmp1(c * (0.03876962f * (255.0f / 8.0f))
                       + (1.15864660f * (255.0f / 2.0f)));
    FloatVector4  tmp2(c);
    tmp2.rsqrtFast();
    c = c * (tmp1 * tmp2 - (0.19741622f * (255.0f / 4.0f)));
    if (!(fmtFlags & 2))
      *linePtr = std::uint32_t(c);
    else
      *linePtr = c.convertToA2R10G10B10();
    if (!(fmtFlags & 1))
    {
      tmpBuf[13] = downsample2xFunc_R8G8B8A8(inBufPtrs, xc);
      xc += int(xc < (imageWidth - 1));
      tmpBuf[14] = downsample2xFunc_R8G8B8A8(inBufPtrs, xc);
    }
    else
    {
      tmpBuf[13] = downsample2xFunc_A2R10G10B10(inBufPtrs, xc);
      xc += int(xc < (imageWidth - 1));
      tmpBuf[14] = downsample2xFunc_A2R10G10B10(inBufPtrs, xc);
    }
    xc += int(xc < (imageWidth - 1));
  }
}

static void downsample2xThread(
    std::uint32_t *outBuf, const std::uint32_t *inBuf,
    int w, int h, int y0, int y1, int pitch, unsigned char fmtFlags)
{
  std::uint32_t *p = outBuf + ((size_t(y0) >> 1) * size_t(pitch));
  for (int y = y0; y < y1; y = y + 2, p = p + pitch)
    downsample2xFilter_Line(p, inBuf, w, h, y, fmtFlags);
}

void downsample2xFilter(std::uint32_t *outBuf, const std::uint32_t *inBuf,
                        int imageWidth, int imageHeight, int pitch,
                        unsigned char fmtFlags)
{
  std::thread *threads[32];
  int     threadCnt = int(std::thread::hardware_concurrency());
  threadCnt = (threadCnt > 1 ? (threadCnt < 32 ? threadCnt : 32) : 1);
  threadCnt = (threadCnt < (imageHeight >> 1) ? threadCnt : (imageHeight >> 1));
  int     y0 = 0;
  for (int i = 0; i < threadCnt; i++)
  {
    int     y1 = ((imageHeight * (i + 1)) / threadCnt) & ~1;
    try
    {
      threads[i] = new std::thread(downsample2xThread,
                                   outBuf, inBuf, imageWidth, imageHeight,
                                   y0, y1, pitch, fmtFlags);
    }
    catch (...)
    {
      threads[i] = (std::thread *) 0;
      downsample2xThread(outBuf, inBuf, imageWidth, imageHeight,
                         y0, y1, pitch, fmtFlags);
    }
    y0 = y1;
  }
  for (int i = 0; i < threadCnt; i++)
  {
    if (threads[i])
    {
      threads[i]->join();
      delete threads[i];
    }
  }
}

static const float  downsample4xFilterTable[13] =
{
  // x = 0, 1, 2, 3, 5, 6, 7, 9, 10, 11, 13, 14, 15
  // pow(cos(x * PI / 35.0), 1.67593545) * sin(x * PI / 4.0) / (x * PI / 4.0)
  1.00000000f,  0.89425031f,  0.61956700f,  0.28220192f,
               -0.15118950f, -0.16431169f, -0.09016543f,
                0.05385105f,  0.05768426f,  0.03013375f,
               -0.01447859f, -0.01270649f, -0.00483659f
};

static FloatVector4 downsample4xFunc_R8G8B8A8(
    const std::uint32_t * const *inBufPtrs, int x)
{
  FloatVector4  c(inBufPtrs[0] + x);
  c.srgbExpand();
  int     i = 1;
  for (int j = 1; j < 13; i = i + 6, j = j + 3)
  {
    FloatVector4  c1(inBufPtrs[i] + x);
    FloatVector4  c2(inBufPtrs[i + 1] + x);
    c += ((c1.srgbExpand() + c2.srgbExpand()) * downsample4xFilterTable[j]);
    c1 = FloatVector4(inBufPtrs[i + 2] + x);
    c2 = FloatVector4(inBufPtrs[i + 3] + x);
    c += ((c1.srgbExpand() + c2.srgbExpand()) * downsample4xFilterTable[j + 1]);
    c1 = FloatVector4(inBufPtrs[i + 4] + x);
    c2 = FloatVector4(inBufPtrs[i + 5] + x);
    c += ((c1.srgbExpand() + c2.srgbExpand()) * downsample4xFilterTable[j + 2]);
  }
  return c;
}

static FloatVector4 downsample4xFunc_A2R10G10B10(
    const std::uint32_t * const *inBufPtrs, int x)
{
  FloatVector4  c(FloatVector4::convertA2R10G10B10(inBufPtrs[0][x], true));
  int     i = 1;
  for (int j = 1; j < 13; i = i + 6, j = j + 3)
  {
    FloatVector4  c1(FloatVector4::convertA2R10G10B10(inBufPtrs[i][x], true));
    FloatVector4  c2(FloatVector4::convertA2R10G10B10(inBufPtrs[i + 1][x],
                                                      true));
    c += ((c1 + c2) * downsample4xFilterTable[j]);
    c1 = FloatVector4::convertA2R10G10B10(inBufPtrs[i + 2][x], true);
    c2 = FloatVector4::convertA2R10G10B10(inBufPtrs[i + 3][x], true);
    c += ((c1 + c2) * downsample4xFilterTable[j + 1]);
    c1 = FloatVector4::convertA2R10G10B10(inBufPtrs[i + 4][x], true);
    c2 = FloatVector4::convertA2R10G10B10(inBufPtrs[i + 5][x], true);
    c += ((c1 + c2) * downsample4xFilterTable[j + 2]);
  }
  return c;
}

void downsample4xFilter_Line(std::uint32_t *linePtr, const std::uint32_t *inBuf,
                             int imageWidth, int imageHeight, int y,
                             unsigned char fmtFlags)
{
  // buffer for vertically filtered line data
  FloatVector4  tmpBuf[31];
  const std::uint32_t *inBufPtrs[25];
  FloatVector4  (*downsample4xFuncPtr)(const std::uint32_t * const *, int) =
      (!(fmtFlags & 1) ?
       &downsample4xFunc_R8G8B8A8 : &downsample4xFunc_A2R10G10B10);
  inBufPtrs[0] = inBuf + (size_t(y) * size_t(imageWidth));
  for (int i = 1; i < 16; i++)
  {
    if (!(i & 3))
      continue;
    int     n = (i - (i >> 2)) << 1;
    int     yc = y - i;
    yc = (yc > 0 ? yc : 0);
    inBufPtrs[n - 1] = inBuf + (size_t(yc) * size_t(imageWidth));
    yc = y + i;
    yc = (yc < (imageHeight - 1) ? yc : (imageHeight - 1));
    inBufPtrs[n] = inBuf + (size_t(yc) * size_t(imageWidth));
  }
  int     xc = 0;
  for (int i = 15; i < 31; i++, xc += int(xc < (imageWidth - 1)))
    tmpBuf[i] = downsample4xFuncPtr(inBufPtrs, xc);
  for (int i = 0; i < 15; i++)
    tmpBuf[i] = tmpBuf[15];
  for (int i = 0; i < imageWidth; i = i + 4, linePtr++)
  {
    FloatVector4  c(tmpBuf[15]);
    c += ((tmpBuf[14] + tmpBuf[16]) * downsample4xFilterTable[1]);
    c += ((tmpBuf[13] + tmpBuf[17]) * downsample4xFilterTable[2]);
    c += ((tmpBuf[12] + tmpBuf[18]) * downsample4xFilterTable[3]);
    c += ((tmpBuf[10] + tmpBuf[20]) * downsample4xFilterTable[4]);
    c += ((tmpBuf[9] + tmpBuf[21]) * downsample4xFilterTable[5]);
    c += ((tmpBuf[8] + tmpBuf[22]) * downsample4xFilterTable[6]);
    c += ((tmpBuf[6] + tmpBuf[24]) * downsample4xFilterTable[7]);
    c += ((tmpBuf[5] + tmpBuf[25]) * downsample4xFilterTable[8]);
    c += ((tmpBuf[4] + tmpBuf[26]) * downsample4xFilterTable[9]);
    c += ((tmpBuf[2] + tmpBuf[28]) * downsample4xFilterTable[10]);
    c += ((tmpBuf[1] + tmpBuf[29]) * downsample4xFilterTable[11]);
    c += ((tmpBuf[0] + tmpBuf[30]) * downsample4xFilterTable[12]);
    c.maxValues(FloatVector4(1.0f / (float(1LL << 42) * float(1LL << 42))));
    for (int j = 0; j < 24; j = j + 4)
    {
      tmpBuf[j] = tmpBuf[j + 4];
      tmpBuf[j + 1] = tmpBuf[j + 5];
      tmpBuf[j + 2] = tmpBuf[j + 6];
      tmpBuf[j + 3] = tmpBuf[j + 7];
    }
    tmpBuf[24] = tmpBuf[28];
    tmpBuf[25] = tmpBuf[29];
    tmpBuf[26] = tmpBuf[30];
    // convert to sRGB color space
    FloatVector4  tmp1(c * (0.03876962f * (255.0f / 64.0f))
                       + (1.15864660f * (255.0f / 4.0f)));
    FloatVector4  tmp2(c);
    tmp2.rsqrtFast();
    c = c * (tmp1 * tmp2 - (0.19741622f * (255.0f / 16.0f)));
    if (!(fmtFlags & 2))
      *linePtr = std::uint32_t(c);
    else
      *linePtr = c.convertToA2R10G10B10();
    tmpBuf[27] = downsample4xFuncPtr(inBufPtrs, xc);
    xc += int(xc < (imageWidth - 1));
    tmpBuf[28] = downsample4xFuncPtr(inBufPtrs, xc);
    xc += int(xc < (imageWidth - 1));
    tmpBuf[29] = downsample4xFuncPtr(inBufPtrs, xc);
    xc += int(xc < (imageWidth - 1));
    tmpBuf[30] = downsample4xFuncPtr(inBufPtrs, xc);
    xc += int(xc < (imageWidth - 1));
  }
}

static void downsample4xThread(
    std::uint32_t *outBuf, const std::uint32_t *inBuf,
    int w, int h, int y0, int y1, int pitch, unsigned char fmtFlags)
{
  std::uint32_t *p = outBuf + ((size_t(y0) >> 2) * size_t(pitch));
  for (int y = y0; y < y1; y = y + 4, p = p + pitch)
    downsample4xFilter_Line(p, inBuf, w, h, y, fmtFlags);
}

void downsample4xFilter(std::uint32_t *outBuf, const std::uint32_t *inBuf,
                        int imageWidth, int imageHeight, int pitch,
                        unsigned char fmtFlags)
{
  std::thread *threads[32];
  int     threadCnt = int(std::thread::hardware_concurrency());
  threadCnt = (threadCnt > 1 ? (threadCnt < 32 ? threadCnt : 32) : 1);
  threadCnt = (threadCnt < (imageHeight >> 2) ? threadCnt : (imageHeight >> 2));
  int     y0 = 0;
  for (int i = 0; i < threadCnt; i++)
  {
    int     y1 = ((imageHeight * (i + 1)) / threadCnt) & ~3;
    try
    {
      threads[i] = new std::thread(downsample4xThread,
                                   outBuf, inBuf, imageWidth, imageHeight,
                                   y0, y1, pitch, fmtFlags);
    }
    catch (...)
    {
      threads[i] = (std::thread *) 0;
      downsample4xThread(outBuf, inBuf, imageWidth, imageHeight,
                         y0, y1, pitch, fmtFlags);
    }
    y0 = y1;
  }
  for (int i = 0; i < threadCnt; i++)
  {
    if (threads[i])
    {
      threads[i]->join();
      delete threads[i];
    }
  }
}

