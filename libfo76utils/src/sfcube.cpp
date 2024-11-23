
#include "sfcube.hpp"
#include "filebuf.hpp"
#include "fp32vec8.hpp"
#include "ddstxt.hpp"
#include "ddstxt16.hpp"

#include <thread>

const float SFCubeMapFilter::defaultRoughnessTable[7] =
{
  0.00000000f, 0.10435608f, 0.21922359f, 0.34861218f, 0.50000000f, 0.69098301f,
  1.00000000f
};

void SFCubeMapFilter::processImage_Specular(
    unsigned char *outBufP, int w, size_t startPos, size_t endPos,
    float roughness)
{
  int     w2 = std::max< int >(w, filterMinWidth);
  std::uint8_t  l2w = std::uint8_t(std::bit_width((unsigned int) w) - 1);
  size_t  cubeFilterTableSize = (size_t(w2) * size_t(w2) * 30) >> 3;
  std::vector< FloatVector4 > tmpBuf(endPos - startPos, FloatVector4(0.0f));
  float   a = roughness * roughness;
  float   a2 = a * a;
  for (size_t j0 = 0; (j0 + 10) <= cubeFilterTableSize; )
  {
    // accumulate convolution output in tmpBuf using smaller partitions of the
    // impulse response so that data used from cubeCoordTable and inBuf fits in
    // L1 cache
    size_t  j1 = j0 + 400;
    j1 = std::min(j1, cubeFilterTableSize);
    FloatVector4  *tmpBufPtr = tmpBuf.data();
    for (size_t i = startPos; i < endPos; i++, tmpBufPtr++)
    {
      int     x = int(i & (size_t(w) - 1));
      int     y = int((i >> l2w) & (size_t(w) - 1));
      int     n = int(i >> (l2w + l2w));
      // v1 = reflected view vector (R), assume V = N = R
      FloatVector4  v1(convertCoord(x, y, w, n));
      FloatVector8  v1x(v1[0]);
      FloatVector8  v1y(v1[1]);
      FloatVector8  v1z(v1[2]);
      FloatVector8  c_r(0.0f);
      FloatVector8  c_g(0.0f);
      FloatVector8  c_b(0.0f);
      FloatVector8  totalWeight(0.0f);
      FloatVector8  a2m1(a2 - 1.0f);
      FloatVector8  a2p1(a2 + 1.0f);
      const FloatVector8  *j =
          reinterpret_cast< const FloatVector8 * >(cubeFilterTable) + j0;
      const FloatVector8  *endPtr = j + (j1 - j0);
      for ( ; (j + 10) <= endPtr; j += 10)
      {
        // v2 = light vector
        FloatVector8  v2x(j[0]);
        FloatVector8  v2y(j[1]);
        FloatVector8  v2z(j[2]);
        // d = N·L = R·L = 2.0 * N·H * N·H - 1.0
        FloatVector8  lDotR = (v1x * v2x) + (v1y * v2y) + (v1z * v2z);
        std::uint32_t signMask = lDotR.getSignMask();
        FloatVector8  v2w(j[3]);
        if (signMask != 255U)
        {
          FloatVector8  d(lDotR);       // face +X, +Y or +Z
          d.maxValues(FloatVector8(0.0f));
          FloatVector8  weight(d * v2w);
          // D denominator = (N·H * N·H * (a2 - 1.0) + 1.0)² * 4.0
          //               = ((R·L + 1.0) * (a2 - 1.0) + 2.0)²
          weight *= (d * a2m1 + a2p1).rcpSqr();
          c_r += (j[4] * weight);
          c_g += (j[5] * weight);
          c_b += (j[6] * weight);
          totalWeight += weight;
          if (signMask == 0U) [[likely]]
            continue;
        }
        if (signMask != 0U)
        {
          FloatVector8  d(lDotR);       // face -X, -Y or -Z: invert dot product
          d.minValues(FloatVector8(0.0f));
          FloatVector8  weight(d * v2w);
          weight *= (a2p1 - (d * a2m1)).rcpSqr();
          c_r -= (j[7] * weight);
          c_g -= (j[8] * weight);
          c_b -= (j[9] * weight);
          totalWeight -= weight;
        }
      }
      FloatVector4  c(c_r.dotProduct(FloatVector8(1.0f)),
                      c_g.dotProduct(FloatVector8(1.0f)),
                      c_b.dotProduct(FloatVector8(1.0f)),
                      totalWeight.dotProduct(FloatVector8(1.0f)));
      *tmpBufPtr = *tmpBufPtr + c;
    }
    j0 = j1;
  }
  FloatVector4  *tmpBufPtr = tmpBuf.data();
  FloatVector4  *endPtr = tmpBufPtr + (endPos - startPos);
  for ( ; (tmpBufPtr + 2) <= endPtr; tmpBufPtr = tmpBufPtr + 2)
  {
    FloatVector8  c(tmpBufPtr);
    FloatVector8  d(c);
    c = c / d.shuffleValues(0xFF);
    c.convertToFloatVector4(tmpBufPtr);
  }
  processImage_Copy(outBufP, w, w, startPos, endPos, tmpBuf.data());
}

void SFCubeMapFilter::processImage_Copy(
    unsigned char *outBufP, int w, int w2, size_t startPos, size_t endPos,
    const FloatVector4 *inBufP)
{
  std::uint8_t  l2w = std::uint8_t(std::bit_width((unsigned int) w) - 1);
  size_t  m = (size_t(1) << (l2w + l2w)) - 1;
  size_t  f = faceDataSize;
  if (w < w2)
  {
    std::uint8_t  l2w2 = std::uint8_t(std::bit_width((unsigned int) w2) - 1);
    int     mipLevel = l2w2 - l2w;
    int     y0 = int(startPos >> std::uint8_t(mipLevel + l2w2));
    int     y1 = int(endPos >> std::uint8_t(mipLevel + l2w2));
    int     n = 1 << (mipLevel + mipLevel);
    float   scale = 1.0f / float(n);
    for (int y = y0; y < y1; y++)
    {
      for (int x = 0; x < w; x++)
      {
        FloatVector4  c(0.0f);
        for (int i = 0; i < n; i++)
        {
          int     xc = (x << mipLevel) + (i & ((1 << mipLevel) - 1));
          int     yc = (y << mipLevel) + (i >> mipLevel);
          c += inBufP[size_t((yc << l2w2) + xc) - startPos];
        }
        size_t  offs = size_t(y * w + x);
        unsigned char *p = outBufP + ((offs >> (l2w + l2w)) * f);
        pixelStoreFunction(p + ((offs & m) * sizeof(std::uint32_t)), c * scale);
      }
    }
  }
  else
  {
    for (size_t i = startPos; i < endPos; i++, inBufP++)
    {
      size_t  n = i >> (l2w + l2w);
      unsigned char *p = outBufP + (n * f);
      pixelStoreFunction(p + ((i & m) * sizeof(std::uint32_t)), *inBufP);
    }
  }
}

void SFCubeMapFilter::pixelStore_R8G8B8A8(unsigned char *p, FloatVector4 c)
{
  std::uint32_t tmp = std::uint32_t(c.srgbCompress()) | 0xFF000000U;
  FileBuffer::writeUInt32Fast(p, tmp);
}

void SFCubeMapFilter::pixelStore_R9G9B9E5(unsigned char *p, FloatVector4 c)
{
  FileBuffer::writeUInt32Fast(p, c.convertToR9G9B9E5());
}

void SFCubeMapFilter::threadFunction(
    SFCubeMapFilter *p, unsigned char *outBufP,
    int w, size_t startPos, size_t endPos, float roughness, bool enableFilter)
{
  if (!enableFilter)
  {
    int     w2 = std::max< int >(w, filterMinWidth);
    std::uint8_t  mipLevel = std::uint8_t(std::bit_width((unsigned int) w2)
                                          - std::bit_width((unsigned int) w));
    p->processImage_Copy(outBufP, w, filterMinWidth, startPos, endPos,
                         p->imageBuf + (startPos << (mipLevel + mipLevel)));
  }
  else
  {
    p->processImage_Specular(outBufP, w, startPos, endPos, roughness);
  }
}

void SFCubeMapFilter::createFilterTable(int w)
{
  for (int n = 0; n < 6; n = n + 2)
  {
    for (int y = 0; y < w; y++)
    {
      for (int x = 0; x < w; x++)
      {
        FloatVector4  v(convertCoord(x, y, w, n));
        int     x2 = (n != 2 ? x : ((w - 1) - x));
        int     y2 = (n == 2 ? y : ((w - 1) - y));
        FloatVector4  c(imageBuf[(n * w + y) * w + x]);
        FloatVector4  c2(imageBuf[((n + 1) * w + y2) * w + x2]);
        float   *p = cubeFilterTable;
        // reorder data for more efficient use of SIMD
        p = p + ((((n >> 1) * w + y) * w + (x & ~7)) * 10 + (x & 7));
        p[0] = v[0];
        p[8] = v[1];
        p[16] = v[2];
        p[24] = v[3];
        p[32] = c[0];
        p[40] = c[1];
        p[48] = c[2];
        p[56] = c2[0];
        p[64] = c2[1];
        p[72] = c2[2];
      }
    }
  }
}

int SFCubeMapFilter::readImageData(
    std::vector< FloatVector4 >& imageData,
    const unsigned char *buf, size_t bufSize)
{
  size_t  w0 = FileBuffer::readUInt32Fast(buf + 16);
  size_t  h0 = FileBuffer::readUInt32Fast(buf + 12);
  unsigned char dxgiFmt = buf[128];
  if (dxgiFmt >= sizeof(FileBuffer::dxgiFormatSizeTable))
    return 0;
  unsigned char blkSize = FileBuffer::dxgiFormatSizeTable[dxgiFmt];
  if (!blkSize)
    return 0;
  size_t  sizeRequired = (w0 * h0 * 6) >> (!(blkSize & 0x80) ? 0 : 4);
  blkSize = blkSize & 0x7F;
  sizeRequired = sizeRequired * size_t(blkSize) + 148;
  if (bufSize < sizeRequired || ((bufSize - 148) % (size_t(blkSize) * 6)) != 0)
    return 0;
  imageData.resize(w0 * h0 * 6, FloatVector4(0.0f));
  faceDataSize = 0;
  for (std::uint32_t w2 = width * width; w2; w2 = w2 >> 2)
    faceDataSize += w2;
  faceDataSize = faceDataSize * sizeof(std::uint32_t);
  FloatVector4  txSum(0.0f);
  if (dxgiFmt == 0x0A)
  {
    // DXGI_FORMAT_R16G16B16A16_FLOAT
    for (size_t n = 0; n < 6; n++)
    {
      const unsigned char *p1 = buf + ((n * ((bufSize - 148) / 6)) + 148);
      FloatVector4  *p2 = imageData.data() + (n * w0 * h0);
      for (size_t y = 0; y < h0; y++)
      {
        FloatVector4  tmpSum(0.0f);
        for (size_t x = 0; x < w0; x++, p1 = p1 + sizeof(std::uint64_t), p2++)
        {
          std::uint64_t tmp = FileBuffer::readUInt64Fast(p1);
          FloatVector4  c(FloatVector4::convertFloat16(tmp));
          c.maxValues(FloatVector4(0.0f)).minValues(FloatVector4(65536.0f));
          *p2 = c;
          tmpSum += c;
        }
        txSum += tmpSum;
      }
    }
  }
  else if (dxgiFmt == 0x5F || dxgiFmt == 0x60)
  {
    // DXGI_FORMAT_BC6H_UF16 or DXGI_FORMAT_BC6H_SF16
    bool    (*decompressFunc)(const std::uint8_t *,
                              std::uint32_t, std::uint32_t, std::uint8_t *);
    if (dxgiFmt != 0x60)
      decompressFunc = &detexDecompressBlockBPTC_FLOAT;
    else
      decompressFunc = &detexDecompressBlockBPTC_SIGNED_FLOAT;
    std::uint64_t tmpBuf[16];
    for (size_t n = 0; n < 6; n++)
    {
      const unsigned char *p1 = buf + ((n * ((bufSize - 148) / 6)) + 148);
      FloatVector4  *p2 = imageData.data() + (n * w0 * h0);
      for (size_t y = 0; y < h0; y = y + 4, p2 = p2 + (w0 * 3))
      {
        FloatVector4  tmpSum(0.0f);
        for (size_t x = 0; x < w0; x = x + 4, p1 = p1 + 16, p2 = p2 + 4)
        {
          (void) decompressFunc(
                     reinterpret_cast< const std::uint8_t * >(p1),
                     0xFFFFFFFFU, 0U,
                     reinterpret_cast< std::uint8_t * >(&(tmpBuf[0])));
          for (size_t i = 0; i < 16; i++)
          {
            FloatVector4  c(FloatVector4::convertFloat16(tmpBuf[i]));
            c.maxValues(FloatVector4(0.0f)).minValues(FloatVector4(65536.0f));
            p2[((i >> 2) * w0) + (i & 3)] = c;
            tmpSum += c;
          }
        }
        txSum += tmpSum;
      }
    }
  }
  else if (dxgiFmt == 0x43)
  {
    // DXGI_FORMAT_R9G9B9E5_SHAREDEXP: assume the texture is already filtered,
    // no normalization
    for (size_t n = 0; n < 6; n++)
    {
      const unsigned char *p1 = buf + ((n * ((bufSize - 148) / 6)) + 148);
      FloatVector4  *p2 = imageData.data() + (n * w0 * h0);
      for (size_t y = 0; y < h0; y++)
      {
        for (size_t x = 0; x < w0; x++, p1 = p1 + sizeof(std::uint32_t), p2++)
          *p2 = FloatVector4::convertR9G9B9E5(FileBuffer::readUInt32Fast(p1));
      }
    }
  }
  else
  {
    try
    {
      DDSTexture  t(buf, bufSize, 0);
      if (t.getWidth() != int(w0) || t.getHeight() != int(h0) ||
          !t.getIsCubeMap())
      {
        return 0;
      }
      const std::uint32_t *p1 = t.data();
      FloatVector4  *p2 = imageData.data();
      size_t  n = w0 * h0;
      size_t  m = size_t(&(t.getPixelN(0, 0, 0, 1)) - p1);
      bool    isSRGB = t.isSRGBTexture();
      for (int i = 0; i < 6; i++, p1 = p1 + m, p2 = p2 + n)
      {
        for (size_t j = 0; (j + 2) <= n; j = j + 2)
        {
          FloatVector8  c(FloatVector4(p1 + j), FloatVector4(p1 + (j + 1)));
          c *= (1.0f / 255.0f);
          if (isSRGB)
            c = DDSTexture16::srgbExpand(c);
          c.convertToFloatVector4(p2 + j);
        }
      }
    }
    catch (FO76UtilsError&)
    {
      return 0;
    }
  }
  if (dxgiFmt == 0x0A || dxgiFmt == 0x5F || dxgiFmt == 0x60)
  {
    // normalize float formats
    float   tmp = txSum[0] + txSum[1] + txSum[2];
    tmp = normalizeLevel * tmp / float(int(imageData.size()));
    if (tmp > 1.0f)
    {
      FloatVector8  scale(1.0f / std::min(tmp, 65536.0f));
      FloatVector4  *p1 = imageData.data();
      FloatVector4  *p2 = p1 + imageData.size();
      for ( ; (p1 + 2) <= p2; p1 = p1 + 2)
        (FloatVector8(p1) * scale).convertToFloatVector4(p1);
    }
  }
  return int(std::bit_width((unsigned long) w0));
}

static inline void getCubeMapPixel(
    FloatVector4& c, float& scale, float weight,
    const FloatVector4 *inBuf, int x, int y, int n, int w)
{
  if (!DDSTexture::wrapCubeMapCoord(x, y, n, w - 1)) [[unlikely]]
  {
    scale = 1.0f / (1.0f - weight);
    return;
  }
  c += (inBuf[(size_t(n) * size_t(w) + size_t(y)) * size_t(w) + size_t(x)]
        * weight);
}

void SFCubeMapFilter::upsampleImage(
    std::vector< FloatVector4 >& outBuf, int outWidth,
    const std::vector< FloatVector4 >& inBuf, int inWidth)
{
  outBuf.resize(size_t(outWidth) * size_t(outWidth) * 6, FloatVector4(0.0f));
  const FloatVector4  *inPtr = inBuf.data();
  FloatVector4  *outPtr = outBuf.data();
  float   xyScale = float(inWidth) / float(outWidth);
  float   offset = xyScale * 0.5f - 0.5f;
  for (int n = 0; n < 6; n++)
  {
    for (int y = 0; y < outWidth; y++)
    {
      float   yf = float(y) * xyScale + offset;
      float   yi = float(std::floor(yf));
      int     y0 = int(yi);
      yf -= yi;
      int     y1 = y0 + 1;
      for (int x = 0; x < outWidth; x++, outPtr++)
      {
        float   xf = float(x) * xyScale + offset;
        float   xi = float(std::floor(xf));
        int     x0 = int(xi);
        xf -= xi;
        int     x1 = x0 + 1;
        FloatVector4  c(0.0f);
        float   weight3 = xf * yf;
        float   weight2 = yf - weight3;
        float   weight1 = xf - weight3;
        float   weight0 = (1.0f - xf) - weight2;
        if (!((x0 | y0 | x1 | y1) & ~(inWidth - 1))) [[likely]]
        {
          c = inPtr[(size_t(n) * size_t(inWidth) + size_t(y0))
                    * size_t(inWidth) + size_t(x0)] * weight0;
          c += (inPtr[(size_t(n) * size_t(inWidth) + size_t(y0))
                      * size_t(inWidth) + size_t(x1)] * weight1);
          c += (inPtr[(size_t(n) * size_t(inWidth) + size_t(y1))
                      * size_t(inWidth) + size_t(x0)] * weight2);
          c += (inPtr[(size_t(n) * size_t(inWidth) + size_t(y1))
                      * size_t(inWidth) + size_t(x1)] * weight3);
          *outPtr = c;
        }
        else
        {
          float   scale = 1.0f;
          getCubeMapPixel(c, scale, weight0, inPtr, x0, y0, n, inWidth);
          getCubeMapPixel(c, scale, weight1, inPtr, x1, y0, n, inWidth);
          getCubeMapPixel(c, scale, weight2, inPtr, x0, y1, n, inWidth);
          getCubeMapPixel(c, scale, weight3, inPtr, x1, y1, n, inWidth);
          *outPtr = c * scale;
        }
      }
    }
  }
}

SFCubeMapFilter::SFCubeMapFilter(size_t outputWidth)
{
  setOutputWidth(outputWidth);
  roughnessTable = &(defaultRoughnessTable[0]);
  roughnessTableSize = int(sizeof(defaultRoughnessTable) / sizeof(float));
  normalizeLevel = float(12.5 / 3.0);
}

SFCubeMapFilter::~SFCubeMapFilter()
{
}

size_t SFCubeMapFilter::convertImage(
    unsigned char *buf, size_t bufSize, bool outFmtFloat, size_t bufCapacity)
{
  if (bufSize < 148)
    return 0;
  size_t  w0 = FileBuffer::readUInt32Fast(buf + 16);
  size_t  h0 = FileBuffer::readUInt32Fast(buf + 12);
  if (FileBuffer::readUInt32Fast(buf) != 0x20534444 ||          // "DDS "
      FileBuffer::readUInt32Fast(buf + 84) != 0x30315844 ||     // "DX10"
      w0 != h0 || w0 < minWidth || w0 > 32768 || (w0 & (w0 - 1)))
  {
    return 0;
  }
  std::vector< FloatVector4 > inBuf1;
  int     mipCnt = readImageData(inBuf1, buf, bufSize);
  size_t  newSize = faceDataSize * 6 + 148;
  if (mipCnt < 1 || std::max(bufSize, bufCapacity) < newSize)
    return 0;
  imageBuf = inBuf1.data();
  std::vector< FloatVector4 > inBuf2;
  std::vector< FloatVector8 > cubeFilterTableBuf;
  cubeFilterTable = nullptr;

  if (!outFmtFloat)
  {
    pixelStoreFunction = &pixelStore_R8G8B8A8;
    buf[128] = 0x1D;                    // DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
  }
  else
  {
    pixelStoreFunction = &pixelStore_R9G9B9E5;
    buf[128] = 0x43;                    // DXGI_FORMAT_R9G9B9E5_SHAREDEXP
  }

  unsigned char *outBufP = buf + 148;
  int     threadCnt = int(std::thread::hardware_concurrency());
  threadCnt = std::min< int >(std::max< int >(threadCnt, 1), 16);
  std::thread *threads[16];
  for (int i = 0; i < 16; i++)
    threads[i] = nullptr;
  int     w = int(width);
  for (int m = 0; w > 0; m++, w = w >> 1)
  {
    int     w2 = std::max< int >(w, filterMinWidth);
    if (int(w0) < w2)
    {
      upsampleImage(inBuf2, w2, inBuf1, int(w0));
      imageBuf = inBuf2.data();
    }
    else
    {
      imageBuf = inBuf1.data();
      while (int(w0) > w2)
      {
        // calculate mipmaps
        size_t  w1 = w0 >> 1;
        for (size_t y = 0; y < (w1 * 6); y++)
        {
          for (size_t x = 0; x < w1; x++)
          {
            size_t  x0 = x << 1;
            size_t  x1 = x0 + 1;
            size_t  y0 = y << 1;
            size_t  y1 = y0 + 1;
            FloatVector4  c0(imageBuf[y0 * w0 + x0]);
            FloatVector4  c1(imageBuf[y0 * w0 + x1]);
            FloatVector4  c2(imageBuf[y1 * w0 + x0]);
            FloatVector4  c3(imageBuf[y1 * w0 + x1]);
            imageBuf[y * w1 + x] = (c0 + c1 + c2 + c3) * 0.25f;
          }
        }
        w0 = w1;
      }
    }
    float   roughness = 1.0f;
    if (m < roughnessTableSize)
      roughness = roughnessTable[m];
    if (w >= filterMinWidth)
      cubeFilterTable = nullptr;
    bool    enableFilter = (roughness >= (3.0f / 128.0f));
    if (enableFilter && !cubeFilterTable)
    {
      cubeFilterTableBuf.resize((size_t(w2) * size_t(w2) * 30) >> 3,
                                FloatVector8(0.0f));
      cubeFilterTable = reinterpret_cast< float * >(cubeFilterTableBuf.data());
      createFilterTable(w2);
    }
    try
    {
      threadCnt = std::min< int >(threadCnt, std::max< int >(w >> 3, 1));
      int     y0 = 0;
      for (int i = 0; i < threadCnt; i++)
      {
        int     y1 = (w * 6 * (i + 1)) / threadCnt;
        size_t  startPos = size_t(y0) * size_t(w);
        size_t  endPos = size_t(y1) * size_t(w);
        y0 = y1;
        threads[i] = new std::thread(threadFunction, this, outBufP, w,
                                     startPos, endPos, roughness, enableFilter);
      }
      for (int i = 0; i < threadCnt; i++)
      {
        threads[i]->join();
        delete threads[i];
        threads[i] = nullptr;
      }
    }
    catch (...)
    {
      for (int i = 0; i < 16; i++)
      {
        if (threads[i])
        {
          threads[i]->join();
          delete threads[i];
        }
      }
      throw;
    }
    outBufP = outBufP + (size_t(w * w) * sizeof(std::uint32_t));
  }

  // DDSD_CAPS, DDSD_HEIGHT, DDSD_WIDTH, DDSD_PITCH,
  // DDSD_PIXELFORMAT, DDSD_MIPMAPCOUNT
  FileBuffer::writeUInt32Fast(buf + 8, 0x0002100FU);
  FileBuffer::writeUInt32Fast(buf + 12, width); // height
  FileBuffer::writeUInt32Fast(buf + 16, width); // width
  FileBuffer::writeUInt32Fast(buf + 20,         // pitch
                              std::uint32_t(width * sizeof(std::uint32_t)));
  buf[28] = (unsigned char) std::bit_width(width);
  buf[108] = buf[108] | 0x08;           // DDSCAPS_COMPLEX
  buf[113] = buf[113] | 0xFE;           // DDSCAPS2_CUBEMAP*
  return newSize;
}

void SFCubeMapFilter::setRoughnessTable(const float *p, size_t n)
{
  if (!p)
  {
    p = &(defaultRoughnessTable[0]);
    n = std::min< size_t >(n, sizeof(defaultRoughnessTable) / sizeof(float));
  }
  roughnessTable = p;
  roughnessTableSize = int(n);
}

SFCubeMapCache::SFCubeMapCache()
  : SFCubeMapFilter(256)
{
}

SFCubeMapCache::~SFCubeMapCache()
{
}

size_t SFCubeMapCache::convertImage(
    unsigned char *buf, size_t bufSize, bool outFmtFloat, size_t bufCapacity,
    size_t outputWidth)
{
  if (outputWidth)
    setOutputWidth(outputWidth);
  std::uint32_t h1 = width;
  std::uint32_t h2 = ~h1;
  size_t  i = 0;
  for ( ; (i + 16) <= bufSize; i = i + 16)
  {
    std::uint64_t tmp1 = FileBuffer::readUInt64Fast(buf + i);
    std::uint64_t tmp2 = FileBuffer::readUInt64Fast(buf + (i + 8));
    hashFunctionCRC32C< std::uint64_t >(h1, tmp1);
    hashFunctionCRC32C< std::uint64_t >(h2, tmp2);
  }
  for ( ; i < bufSize; i++)
  {
    std::uint32_t&  h = (!(i & 8) ? h1 : h2);
    hashFunctionCRC32C< unsigned char >(h, buf[i]);
  }
  std::uint64_t k = (std::uint64_t(h2) << 32) | h1;
  std::vector< unsigned char >& v = cachedTextures[k];
  if (v.size() > 0)
  {
    std::memcpy(buf, v.data(), v.size());
    return v.size();
  }
  size_t  newSize =
      SFCubeMapFilter::convertImage(buf, bufSize, outFmtFloat, bufCapacity);
  if (!newSize && bufSize >= 11 &&
      FileBuffer::readUInt64Fast(buf) == 0x4E41494441523F23ULL) // "#?RADIAN"
  {
    std::vector< unsigned char >  tmpBuf;
    if (convertHDRToDDS(tmpBuf, buf, bufSize, 2048, false, 65504.0f, 0x0A))
    {
      newSize = SFCubeMapFilter::convertImage(tmpBuf.data(), tmpBuf.size(),
                                              outFmtFloat, tmpBuf.size());
      if (newSize && newSize <= std::max(bufSize, bufCapacity))
        std::memcpy(buf, tmpBuf.data(), newSize);
      else
        newSize = 0;
    }
  }
  if (newSize)
  {
    v.resize(newSize);
    std::memcpy(v.data(), buf, newSize);
    return newSize;
  }
  return bufSize;
}

static inline float atan2NormFast(float y, float x, bool xNonNegative = false)
{
  // assumes x² + y² = 1.0, returns atan2(y, x) / π
  float   xAbs = (xNonNegative ? x : float(std::fabs(x)));
  float   yAbs = float(std::fabs(y));
  float   tmp = std::min(xAbs, yAbs);
  float   tmp2 = tmp * tmp;
  float   tmp3 = tmp2 * tmp;
  tmp = (((0.39603792f * tmp3) + (-0.98507216f * tmp2) + (1.09851059f * tmp)
          - 0.65754361f) * tmp3
         + (0.26000725f * tmp2) + (-0.05051132f * tmp) + 0.05919720f) * tmp3
        + (-0.00037558f * tmp2) + (0.31831810f * tmp);
  if (xAbs < yAbs)
    tmp = 0.5f - tmp;
  if (!xNonNegative && x < 0.0f)
    tmp = 1.0f - tmp;
  return (y < 0.0f ? -tmp : tmp);
}

void SFCubeMapCache::convertHDRToDDSThread(
    unsigned char *outBuf, size_t outPixelSize, int cubeWidth,
    int yStart, int yEnd,
    const FloatVector4 *hdrTexture, int w, int h, float maxLevel)
{
  unsigned char *p =
      outBuf + (size_t(yStart) * size_t(cubeWidth) * outPixelSize);
  for ( ; yStart < yEnd; yStart++)
  {
    int     n = yStart / cubeWidth;
    int     y = yStart % cubeWidth;
    for (int x = 0; x < cubeWidth; x++, p = p + outPixelSize)
    {
      FloatVector4  v(SFCubeMapFilter::convertCoord(x, y, cubeWidth, n));
      // convert to spherical coordinates
      float   xy = float(std::sqrt(v.dotProduct2(v)));
      float   z = v[2];
      v /= xy;
      float   xf = atan2NormFast(v[0], v[1]) * 0.5f + 0.5f;
      float   yf = atan2NormFast(z, xy, true) + 0.5f;
      xf = xf * float(w) - 0.5f;
      yf = yf * float(h) - 0.5f;
      float   xi = float(std::floor(xf));
      float   yi = float(std::floor(yf));
      xf -= xi;
      yf -= yi;
      int     x0 = int(xi);
      int     y0 = int(yi);
      x0 = (x0 <= (w - 1) ? (x0 >= 0 ? x0 : (w - 1)) : 0);
      int     x1 = (x0 < (w - 1) ? (x0 + 1) : 0);
      int     y1 = std::min< int >(std::max< int >(y0 + 1, 0), h - 1);
      y0 = std::min< int >(std::max< int >(y0, 0), h - 1);
      // bilinear interpolation
      const FloatVector4  *inPtr = hdrTexture + (y0 * w);
      FloatVector4  c(inPtr[x0] * (1.0f - xf) + (inPtr[x1] * xf));
      inPtr = inPtr + (y1 > y0 ? w : 0);
      c = c + (((inPtr[x0] * (1.0f - xf) + (inPtr[x1] * xf)) - c) * yf);
      c.maxValues(FloatVector4(0.0f));
      if (maxLevel < 0.0f)
        c = c * FloatVector4(maxLevel) / (FloatVector4(maxLevel) - c);
      else
        c.minValues(FloatVector4(maxLevel));
      c[3] = 1.0f;
      if (outPixelSize == sizeof(std::uint64_t))
        FileBuffer::writeUInt64Fast(p, c.convertToFloat16());
      else
        FileBuffer::writeUInt32Fast(p, c.convertToR9G9B9E5());
    }
  }
}

bool SFCubeMapCache::convertHDRToDDS(
    std::vector< unsigned char >& outBuf,
    const unsigned char *inBufData, size_t inBufSize,
    int cubeWidth, bool invertCoord, float maxLevel, unsigned char outFmt)
{
  // file should begin with "#?RADIANCE\n"
  if (inBufSize < 11 ||
      FileBuffer::readUInt64Fast(inBufData) != 0x4E41494441523F23ULL ||
      FileBuffer::readUInt32Fast(inBufData + 7) != 0x0A45434EU)
  {
    return false;
  }
  FileBuffer  inBuf(inBufData, inBufSize, 11);
  const char  *lineBuf = reinterpret_cast< const char * >(inBuf.getReadPtr());
  int     w = 0;
  int     h = 0;
  while (inBuf.getPosition() < inBuf.size())
  {
    unsigned char c = inBuf.readUInt8Fast();
    if (c < 0x08)
      break;
    if (c != 0x0A)
      continue;
    if ((lineBuf[0] == '-' || lineBuf[0] == '+') && lineBuf[1] == 'Y')
    {
      if (lineBuf[0] == '+')
        invertCoord = !invertCoord;
      const char  *s = lineBuf + 2;
      while (*s == '\t' || *s == ' ')
        s++;
      long    n = 0;
      for ( ; *s >= '0' && *s <= '9'; s++)
        n = n * 10L + long(*s - '0');
      if (n < 8 || n > 32768)
        break;
      h = int(n);
      while (*s == '\t' || *s == ' ')
        s++;
      if (!(s[0] == '+' && s[1] == 'X'))
        break;
      s = s + 2;
      while (*s == '\t' || *s == ' ')
        s++;
      n = 0;
      for ( ; *s >= '0' && *s <= '9'; s++)
        n = n * 10L + long(*s - '0');
      if ((*s == '\t' || *s == '\n' || *s == '\r' || *s == ' ') &&
          n >= 8 && n <= 32768)
      {
        w = int(n);
      }
      break;
    }
    lineBuf = reinterpret_cast< const char * >(inBuf.getReadPtr());
  }
  if (!w || !h)
    return false;
  std::vector< std::uint32_t >  tmpBuf(size_t(w * h), 0U);
  for (int y = 0; y < h; y++)
  {
    if ((inBuf.getPosition() + 4ULL) > inBuf.size())
      return false;
    std::uint32_t *p =
        tmpBuf.data() + size_t((invertCoord ? y : ((h - 1) - y)) * w);
    std::uint32_t tmp =
        FileBuffer::readUInt32Fast(inBuf.data() + inBuf.getPosition());
    if (tmp != ((std::uint32_t(w & 0xFF) << 24) | (std::uint32_t(w >> 8) << 16)
                | 0x0202U))
    {
      // old RLE format
      unsigned char lenShift = 0;
      for (int x = 0; x < w; )
      {
        if ((inBuf.getPosition() + 4ULL) > inBuf.size())
          return false;
        std::uint32_t c = inBuf.readUInt32Fast();
        if ((c & 0x00FFFFFFU) != 0x00010101U || x < 1)
        {
          lenShift = 0;
          p[x] = c;
          x++;
        }
        else
        {
          size_t  l = (c >> 24) << lenShift;
          lenShift = 8;
          for ( ; l; l--, x++)
          {
            if (x >= w)
              return false;
            p[x] = p[x - 1];
          }
        }
      }
    }
    else
    {
      // new RLE format
      inBuf.setPosition(inBuf.getPosition() + 4);
      for (unsigned char c = 0; c < 32; c = c + 8)
      {
        for (int x = 0; x < w; )
        {
          if (inBuf.getPosition() >= inBuf.size())
            return false;
          unsigned char l = inBuf.readUInt8Fast();
          if (l <= 0x80)
          {
            // copy literals
            for ( ; l; l--, x++)
            {
              if (x >= w || inBuf.getPosition() >= inBuf.size())
                return false;
              p[x] |= (std::uint32_t(inBuf.readUInt8Fast()) << c);
            }
          }
          else
          {
            // RLE
            if (inBuf.getPosition() >= inBuf.size())
              return false;
            std::uint32_t b = std::uint32_t(inBuf.readUInt8Fast()) << c;
            for ( ; l > 0x80; l--, x++)
            {
              if (x >= w)
                return false;
              p[x] |= b;
            }
          }
        }
      }
    }
  }
  std::vector< FloatVector4 > tmpBuf2(tmpBuf.size(), FloatVector4(0.0f));
  for (size_t i = 0; i < tmpBuf.size(); i++)
  {
    std::uint32_t b = tmpBuf[i];
    FloatVector4  c(b);
    int     e = int(b >> 24);
#if defined(__i386__) || defined(__x86_64__) || defined(__x86_64)
    e = std::min< int >(std::max< int >(e, 16), 240) - 9;
    c *= std::bit_cast< float >(std::uint32_t(e << 23));
#else
    e = std::min< int >(std::max< int >(e, 103), 165) - 103;
    c *= float(std::int64_t(1) << e) * float(0.5 / (65536.0 * 65536.0));
#endif
    c[3] = 1.0f;
    tmpBuf2[i] = c;
  }
  size_t  outPixelSize =        // 8 bytes for DXGI_FORMAT_R16G16B16A16_FLOAT
      (outFmt == 0x0A ? sizeof(std::uint64_t) : sizeof(std::uint32_t));
  outBuf.resize(size_t(cubeWidth * cubeWidth) * 6 * outPixelSize + 148, 0);
  unsigned char *p = outBuf.data();
  (void) FileBuffer::writeDDSHeader(p, outFmt, cubeWidth, cubeWidth, 1, true);
  p = p + 148;

  int     threadCnt = int(std::thread::hardware_concurrency());
  threadCnt = std::min< int >(threadCnt, std::min< int >(cubeWidth >> 3, 16));
  threadCnt = std::max< int >(threadCnt, 1);
  std::thread *threads[16];
  for (int i = 0; i < 16; i++)
    threads[i] = nullptr;
  try
  {
    int     y0 = 0;
    for (int i = 0; i < threadCnt; i++)
    {
      int     y1 = (cubeWidth * 6 * (i + 1)) / threadCnt;
      threads[i] = new std::thread(convertHDRToDDSThread, p, outPixelSize,
                                   cubeWidth, y0, y1, tmpBuf2.data(), w, h,
                                   maxLevel);
      y0 = y1;
    }
    for (int i = 0; i < threadCnt; i++)
    {
      threads[i]->join();
      delete threads[i];
      threads[i] = nullptr;
    }
  }
  catch (...)
  {
    for (int i = 0; i < 16; i++)
    {
      if (threads[i])
      {
        threads[i]->join();
        delete threads[i];
      }
    }
    throw;
  }
  return true;
}

