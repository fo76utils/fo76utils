
#include "common.hpp"
#include "ddstxt.hpp"

static inline unsigned long long decodeBC3Alpha(unsigned int *a,
                                                const unsigned char *src,
                                                bool isSigned = false)
{
  a[0] = src[0] ^ (!isSigned ? 0U : 0x80U);
  a[1] = src[1] ^ (!isSigned ? 0U : 0x80U);
  if (a[0] > a[1])
  {
    for (unsigned int i = 1; i < 7; i++)
      a[i + 1] = ((a[0] * (7 - i)) + (a[1] * i) + 3) / 7;
  }
  else
  {
    for (unsigned int i = 1; i < 5; i++)
      a[i + 1] = ((a[0] * (5 - i)) + (a[1] * i) + 2) / 5;
    a[6] = 0;
    a[7] = 255;
  }
  unsigned long long  ba = 0;
  for (unsigned int i = 0; i < 6; i++)
    ba = ba | ((unsigned long long) src[i + 2] << (i << 3));
  return ba;
}

static inline unsigned int decodeBC1Colors(unsigned int *c,
                                           const unsigned char *src,
                                           unsigned int a = 0U)
{
  unsigned int  c0 = ((unsigned int) src[1] << 8) | src[0];
  unsigned int  c1 = ((unsigned int) src[3] << 8) | src[2];
  unsigned int  r0 = (((c0 >> 11) & 0x1F) * 255 + 15) / 31;
  unsigned int  g0 = (((c0 >> 5) & 0x3F) * 255 + 31) / 63;
  unsigned int  b0 = ((c0 & 0x1F) * 255 + 15) / 31;
  unsigned int  r1 = (((c1 >> 11) & 0x1F) * 255 + 15) / 31;
  unsigned int  g1 = (((c1 >> 5) & 0x3F) * 255 + 31) / 63;
  unsigned int  b1 = ((c1 & 0x1F) * 255 + 15) / 31;
  c[0] = r0 | (g0 << 8) | (b0 << 16) | a;
  c[1] = r1 | (g1 << 8) | (b1 << 16) | a;
  c[2] = ((r0 + r0 + r1 + 1) / 3) | (((g0 + g0 + g1 + 1) / 3) << 8)
         | (((b0 + b0 + b1 + 1) / 3) << 16) | a;
  c[3] = ((r0 + r1 + r1 + 1) / 3) | (((g0 + g1 + g1 + 1) / 3) << 8)
         | (((b0 + b1 + b1 + 1) / 3) << 16) | a;
  unsigned int  bc = 0;
  for (unsigned int i = 0; i < 4; i++)
    bc = bc | ((unsigned long long) src[i + 4] << (i << 3));
  return bc;
}

size_t DDSTexture::decodeBlock_BC1(unsigned int *dst, const unsigned char *src,
                                   unsigned int w)
{
  unsigned int  c[4];
  unsigned int  bc = decodeBC1Colors(c, src, 0xFF000000U);
  for (unsigned int i = 0; i < 16; i++)
  {
    dst[(i >> 2) * w + (i & 3)] = c[bc & 3];
    bc = bc >> 2;
  }
  return 8;
}

size_t DDSTexture::decodeBlock_BC2(unsigned int *dst, const unsigned char *src,
                                   unsigned int w)
{
  unsigned int  c[4];
  unsigned int  bc = decodeBC1Colors(c, src + 8);
  for (unsigned int i = 0; i < 4; i++)
  {
    unsigned char a = src[i << 1];
    dst[i * w] = c[bc & 3] | ((a & 0x0FU) * 0x11000000U);
    bc = bc >> 2;
    dst[i * w + 1] = c[bc & 3] | (((a >> 4) & 0x0FU) * 0x11000000U);
    bc = bc >> 2;
    a = src[(i << 1) + 1];
    dst[i * w + 2] = c[bc & 3] | ((a & 0x0FU) * 0x11000000U);
    bc = bc >> 2;
    dst[i * w + 3] = c[bc & 3] | (((a >> 4) & 0x0FU) * 0x11000000U);
    bc = bc >> 2;
  }
  return 16;
}

size_t DDSTexture::decodeBlock_BC3(unsigned int *dst, const unsigned char *src,
                                   unsigned int w)
{
  unsigned int  c[4];
  unsigned int  a[8];
  unsigned long long  ba = decodeBC3Alpha(a, src);
  unsigned int  bc = decodeBC1Colors(c, src + 8);
  for (unsigned int i = 0; i < 16; i++)
  {
    dst[(i >> 2) * w + (i & 3)] = c[bc & 3] | (a[ba & 7] << 24);
    ba = ba >> 3;
    bc = bc >> 2;
  }
  return 16;
}

size_t DDSTexture::decodeBlock_BC4(unsigned int *dst, const unsigned char *src,
                                   unsigned int w)
{
  unsigned int  a[8];
  unsigned long long  ba = decodeBC3Alpha(a, src);
  for (unsigned int i = 0; i < 16; i++)
  {
    dst[(i >> 2) * w + (i & 3)] = (a[ba & 7] * 0x00010101U) | 0xFF000000U;
    ba = ba >> 3;
  }
  return 8;
}

size_t DDSTexture::decodeBlock_BC4S(unsigned int *dst, const unsigned char *src,
                                    unsigned int w)
{
  unsigned int  a[8];
  unsigned long long  ba = decodeBC3Alpha(a, src, true);
  for (unsigned int i = 0; i < 16; i++)
  {
    dst[(i >> 2) * w + (i & 3)] = (a[ba & 7] * 0x00010101U) | 0xFF000000U;
    ba = ba >> 3;
  }
  return 8;
}

size_t DDSTexture::decodeBlock_BC5(unsigned int *dst, const unsigned char *src,
                                   unsigned int w)
{
  unsigned int  a1[8];
  unsigned int  a2[8];
  unsigned long long  ba1 = decodeBC3Alpha(a1, src);
  unsigned long long  ba2 = decodeBC3Alpha(a2, src + 8);
  for (unsigned int i = 0; i < 16; i++)
  {
    dst[(i >> 2) * w + (i & 3)] =
        a1[ba1 & 7] | (a2[ba2 & 7] << 8) | 0xFF000000U;
    ba1 = ba1 >> 3;
    ba2 = ba2 >> 3;
  }
  return 16;
}

size_t DDSTexture::decodeBlock_BC5S(unsigned int *dst, const unsigned char *src,
                                    unsigned int w)
{
  unsigned int  a1[8];
  unsigned int  a2[8];
  unsigned long long  ba1 = decodeBC3Alpha(a1, src, true);
  unsigned long long  ba2 = decodeBC3Alpha(a2, src + 8, true);
  for (unsigned int i = 0; i < 16; i++)
  {
    dst[(i >> 2) * w + (i & 3)] =
        a1[ba1 & 7] | (a2[ba2 & 7] << 8) | 0xFF000000U;
    ba1 = ba1 >> 3;
    ba2 = ba2 >> 3;
  }
  return 16;
}

void DDSTexture::loadTexture(FileBuffer& buf, int mipOffset)
{
  buf.setPosition(0);
  if (buf.size() < 148 || !FileBuffer::checkType(buf.readUInt32(), "DDS "))
    throw errorMessage("unsupported texture file format");
  if (buf.readUInt32() != 0x7C)         // size of DDS_HEADER
    throw errorMessage("unsupported texture file format");
  unsigned int  flags = buf.readUInt32();
  if ((flags & 0x1006) != 0x1006)       // height, width, pixel format required
    throw errorMessage("unsupported texture file format");
  mipLevelCnt = 1;
  haveAlpha = false;
  ySizeMip0 = buf.readUInt32();
  xSizeMip0 = buf.readUInt32();
  if (ySizeMip0 < 4 || ySizeMip0 > 32768 ||
      xSizeMip0 < 4 || xSizeMip0 > 32768 ||
      (ySizeMip0 & (ySizeMip0 - 1)) != 0 || (xSizeMip0 & (xSizeMip0 - 1)) != 0)
  {
    throw errorMessage("invalid or unsupported texture dimensions");
  }
  (void) buf.readUInt32();              // dwPitchOrLinearSize
  (void) buf.readUInt32();              // dwDepth
  if (flags & 0x00020000)               // DDSD_MIPMAPCOUNT
  {
    mipLevelCnt = buf.readInt32();
    if (mipLevelCnt < 1 || mipLevelCnt > 16)
      throw errorMessage("invalid mipmap count");
  }
  buf.setPosition(0x4C);                // ddspf
  if (buf.readUInt32() != 0x20)         // size of DDS_PIXELFORMAT
    throw errorMessage("unsupported texture file format");
  if (!(buf.readUInt32() & 0x04))       // DDPF_FOURCC is required
    throw errorMessage("unsupported texture file format");
  unsigned int  fourCC = buf.readUInt32();
  unsigned int  dataOffs = 128;
  size_t  blockSize = 8;
  size_t  (*decodeFunction)(unsigned int *, const unsigned char *,
                            unsigned int) = &decodeBlock_BC1;
  if (FileBuffer::checkType(fourCC, "DX10"))
  {
    dataOffs = 148;
    buf.setPosition(0x80);
    unsigned int  tmp = buf.readUInt32();
    switch (tmp)
    {
      case 0x47:                        // DXGI_FORMAT_BC1_UNORM
      case 0x48:                        // DXGI_FORMAT_BC1_UNORM_SRGB
        break;
      case 0x4A:                        // DXGI_FORMAT_BC2_UNORM
      case 0x4B:                        // DXGI_FORMAT_BC2_UNORM_SRGB
        haveAlpha = true;
        blockSize = 16;
        decodeFunction = &decodeBlock_BC2;
        break;
      case 0x4D:                        // DXGI_FORMAT_BC3_UNORM
      case 0x4E:                        // DXGI_FORMAT_BC3_UNORM_SRGB
        haveAlpha = true;
        blockSize = 16;
        decodeFunction = &decodeBlock_BC3;
        break;
      case 0x50:                        // DXGI_FORMAT_BC4_UNORM
        decodeFunction = &decodeBlock_BC4;
        break;
      case 0x51:                        // DXGI_FORMAT_BC4_SNORM
        decodeFunction = &decodeBlock_BC4S;
        break;
      case 0x53:                        // DXGI_FORMAT_BC5_UNORM
        blockSize = 16;
        decodeFunction = &decodeBlock_BC5;
        break;
      case 0x54:                        // DXGI_FORMAT_BC5_SNORM
        blockSize = 16;
        decodeFunction = &decodeBlock_BC5S;
        break;
      default:
        throw errorMessage("unsupported DXGI_FORMAT: 0x%08X", tmp);
    }
  }
  else
  {
    switch (fourCC)
    {
      case 0x31545844:                  // "DXT1"
        break;
      case 0x32545844:                  // "DXT2"
      case 0x33545844:                  // "DXT3"
        haveAlpha = true;
        blockSize = 16;
        decodeFunction = &decodeBlock_BC2;
        break;
      case 0x34545844:                  // "DXT4"
      case 0x35545844:                  // "DXT5"
        haveAlpha = true;
        blockSize = 16;
        decodeFunction = &decodeBlock_BC3;
        break;
      default:
        throw errorMessage("unsupported DDS fourCC: 0x%08X",
                           FileBuffer::swapUInt32(fourCC));
    }
  }
  size_t  sizeRequired = dataOffs;
  for (int i = 0; i < mipLevelCnt; i++)
  {
    unsigned int  w = xSizeMip0 >> (i + 2);
    unsigned int  h = ySizeMip0 >> (i + 2);
    if (w < 1)
      w = 1;
    if (h < 1)
      h = 1;
    sizeRequired = sizeRequired + (size_t(w) * h * blockSize);
  }
  if (buf.size() < sizeRequired)
    throw errorMessage("DDS file is shorter than expected");
  const unsigned char *srcPtr = buf.getDataPtr() + dataOffs;
  for ( ; mipOffset > 0 && mipLevelCnt > 1; mipOffset--, mipLevelCnt--)
  {
    unsigned int  w = (xSizeMip0 + 3) >> 2;
    unsigned int  h = (ySizeMip0 + 3) >> 2;
    srcPtr = srcPtr + (size_t(w) * h * blockSize);
    xSizeMip0 = (xSizeMip0 + 1) >> 1;
    ySizeMip0 = (ySizeMip0 + 1) >> 1;
  }

  size_t  dataOffsets[20];
  size_t  bufSize = 0;
  for (int i = 0; i < 20; i++)
  {
    unsigned int  w = xSizeMip0 >> i;
    unsigned int  h = ySizeMip0 >> i;
    if (w < 1)
      w = 1;
    if (h < 1)
      h = 1;
    dataOffsets[i] = bufSize;
    bufSize = bufSize + (size_t(w) * h);
  }
  textureDataBuf.resize(bufSize);
  for (int i = 0; i < 20; i++)
  {
    unsigned int  *p = &(textureDataBuf.front()) + dataOffsets[i];
    textureData[i] = p;
    unsigned int  w = xSizeMip0 >> i;
    unsigned int  h = ySizeMip0 >> i;
    if (w < 1)
      w = 1;
    if (h < 1)
      h = 1;
    if (i < mipLevelCnt)
    {
      if (w < 4 || h < 4)
      {
        unsigned int  tmpBuf[16];
        for (unsigned int y = 0; y < h; y = y + 4)
        {
          for (unsigned int x = 0; x < w; x = x + 4)
          {
            srcPtr = srcPtr + decodeFunction(tmpBuf, srcPtr, 4);
            for (unsigned int j = 0; j < 16; j++)
            {
              if ((y + (j >> 2)) < h && (x + (j & 3)) < w)
                p[(y + (j >> 2)) * w + (x + (j & 3))] = tmpBuf[j];
            }
          }
        }
      }
      else
      {
        for (unsigned int y = 0; y < h; y = y + 4)
        {
          for (unsigned int x = 0; x < w; x = x + 4)
            srcPtr = srcPtr + decodeFunction(p + (y * w + x), srcPtr, w);
        }
      }
    }
    else
    {
      for (unsigned int y = 0; y < h; y++)
      {
        for (unsigned int x = 0; x < w; x++)
        {
          unsigned int  c0 = getPixelN(x << 1, y << 1, i - 1);
          unsigned int  c1 = getPixelN((x << 1) + 1, y << 1, i - 1);
          unsigned int  c2 = getPixelN(x << 1, (y << 1) + 1, i - 1);
          unsigned int  c3 = getPixelN((x << 1) + 1, (y << 1) + 1, i - 1);
          c0 = blendRGBA32(c0, c1, 128);
          c2 = blendRGBA32(c2, c3, 128);
          p[y * w + x] = blendRGBA32(c0, c2, 128);
        }
      }
    }
  }
  for ( ; mipOffset > 0; mipOffset--)
  {
    xSizeMip0 = (xSizeMip0 + 1) >> 1;
    ySizeMip0 = (ySizeMip0 + 1) >> 1;
    for (int i = 0; (i + 1) < 20; i++)
      dataOffsets[i] = dataOffsets[i + 1];
  }
}

DDSTexture::DDSTexture(const char *fileName, int mipOffset)
{
  FileBuffer  tmpBuf(fileName);
  loadTexture(tmpBuf, mipOffset);
}

DDSTexture::DDSTexture(const unsigned char *buf, size_t bufSize, int mipOffset)
{
  FileBuffer  tmpBuf(buf, bufSize);
  loadTexture(tmpBuf, mipOffset);
}

DDSTexture::DDSTexture(FileBuffer& buf, int mipOffset)
{
  loadTexture(buf, mipOffset);
}

DDSTexture::~DDSTexture()
{
}

unsigned int DDSTexture::getPixelB(float x, float y, int mipLevel) const
{
  int     x0 = int(x * 256.0f + (x < 0.0f ? -0.5f : 0.5f));
  int     y0 = int(y * 256.0f + (y < 0.0f ? -0.5f : 0.5f));
  int     xf = x0 & 0xFF;
  int     yf = y0 & 0xFF;
  x0 = (x0 & 0x7FFFFF00) >> 8;
  y0 = (y0 & 0x7FFFFF00) >> 8;
  unsigned int  c0 = blendRGBA32(getPixelN(x0, y0, mipLevel),
                                 getPixelN(x0 + 1, y0, mipLevel), xf);
  unsigned int  c1 = blendRGBA32(getPixelN(x0, y0 + 1, mipLevel),
                                 getPixelN(x0 + 1, y0 + 1, mipLevel), xf);
  return blendRGBA32(c0, c1, yf);
}

unsigned int DDSTexture::getPixelT(float x, float y, float mipLevel) const
{
  int     x0 = int(x * 256.0f + (x < 0.0f ? -0.5f : 0.5f));
  int     y0 = int(y * 256.0f + (y < 0.0f ? -0.5f : 0.5f));
  int     m0 = int(mipLevel * 256.0f + (mipLevel < 0.0f ? -0.5f : 0.5f));
  int     x0m1 = (x0 & 0x7FFFFFFF) >> 1;
  int     y0m1 = (y0 & 0x7FFFFFFF) >> 1;
  int     xf = x0 & 0xFF;
  int     yf = y0 & 0xFF;
  int     mf = m0 & 0xFF;
  x0 = (x0 & 0x7FFFFF00) >> 8;
  y0 = (y0 & 0x7FFFFF00) >> 8;
  m0 = (m0 & 0x7FFFFF00) >> 8;
  int     m1 = m0 + 1;
  int     xfm1 = x0m1 & 0xFF;
  int     yfm1 = y0m1 & 0xFF;
  x0m1 = (x0m1 & 0x7FFFFF00) >> 8;
  y0m1 = (y0m1 & 0x7FFFFF00) >> 8;
  unsigned int  c0 = blendRGBA32(getPixelN(x0, y0, m0),
                                 getPixelN(x0 + 1, y0, m0), xf);
  unsigned int  c1 = blendRGBA32(getPixelN(x0, y0 + 1, m0),
                                 getPixelN(x0 + 1, y0 + 1, m0), xf);
  unsigned int  c = blendRGBA32(c0, c1, yf);
  c0 = blendRGBA32(getPixelN(x0m1, y0m1, m1),
                   getPixelN(x0m1 + 1, y0m1, m1), xfm1);
  c1 = blendRGBA32(getPixelN(x0m1, y0m1 + 1, m1),
                   getPixelN(x0m1 + 1, y0m1 + 1, m1), xfm1);
  return blendRGBA32(c, blendRGBA32(c0, c1, yfm1), mf);
}

