
#include "common.hpp"
#include "ddstxt.hpp"

static inline unsigned long long blendToRBGA64Bilinear(
    unsigned int c0, unsigned int c1, unsigned int c2, unsigned int c3,
    int xf, int yf)
{
  unsigned int  f3 = ((unsigned int) xf * (unsigned int) yf + 128U) >> 8;
  unsigned int  f1 = (unsigned int) xf - f3;
  unsigned int  f2 = (unsigned int) yf - f3;
  unsigned int  f0 = 256U - ((unsigned int) xf + ((unsigned int) yf - f3));
  unsigned long long  c =
      (rgba32ToRBGA64(c0) * f0) + (rgba32ToRBGA64(c1) * f1)
      + (rgba32ToRBGA64(c2) * f2) + (rgba32ToRBGA64(c3) * f3);
  return (((c + 0x0080008000800080ULL) >> 8) & 0x00FF00FF00FF00FFULL);
}

static const float mipScaleTable[8] =
{
  0.5f, 0.25f, 0.125f, 0.0625f, 0.03125f, 0.015625f, 0.0078125f, 0.00390625f
};

static inline void fixNegativeMipLevel(float& x, float& y, int& m)
{
  if (BRANCH_EXPECT((m < 0), false))
  {
    float   tmp = mipScaleTable[~m & 7];
    x *= tmp;
    y *= tmp;
    m = 0;
  }
}

static inline void convertTexCoord(int& x0, int& y0, float& xf, float& yf,
                                   float x, float y)
{
  x0 = int(float(std::floor(x)));
  y0 = int(float(std::floor(y)));
  xf = x - float(x0);
  yf = y - float(y0);
}

static inline FloatVector4 blendFP32Vec4Bilinear(
    unsigned int c0, unsigned int c1, unsigned int c2, unsigned int c3,
    float xf, float yf)
{
  float   f3 = xf * yf;
  float   f2 = yf - f3;
  float   f1 = xf - f3;
  float   f0 = 1.0f - (xf + f2);
  FloatVector4  c0_f(c0);
  FloatVector4  c1_f(c1);
  FloatVector4  c2_f(c2);
  FloatVector4  c3_f(c3);
  c0_f *= f0;
  c1_f *= f1;
  c2_f *= f2;
  c3_f *= f3;
  c0_f += c1_f;
  c0_f += c2_f;
  c0_f += c3_f;
  return c0_f;
}

static inline unsigned long long decodeBC3Alpha(unsigned int *a,
                                                const unsigned char *src,
                                                bool isSigned = false)
{
  a[0] = src[0] ^ (!isSigned ? 0U : 0x80U);
  a[1] = src[1] ^ (!isSigned ? 0U : 0x80U);
  if (a[0] > a[1])
  {
    int     n = int((a[0] << 18) + 0x00020000U);
    int     d = (int(a[1]) - int(a[0])) * 37449;
    for (unsigned int i = 2; i < 8; i++)
    {
      n = n + d;
      a[i] = (unsigned int) n >> 18;
    }
  }
  else
  {
    int     n = int((a[0] << 20) + 0x00080000U);
    int     d = (int(a[1]) - int(a[0])) * 209715;
    for (unsigned int i = 2; i < 6; i++)
    {
      n = n + d;
      a[i] = (unsigned int) n >> 20;
    }
    a[6] = 0;
    a[7] = 255;
  }
  unsigned long long  ba = FileBuffer::readUInt16Fast(src + 2);
  ba = ba | ((unsigned long long) FileBuffer::readUInt32Fast(src + 4) << 16);
  return ba;
}

static inline unsigned int decodeBC1Colors(unsigned int *c,
                                           const unsigned char *src,
                                           unsigned int a = 0U)
{
  unsigned int  c0 = FileBuffer::readUInt16Fast(src);
  unsigned int  c1 = FileBuffer::readUInt16Fast(src + 2);
  unsigned int  rb0 =
      ((((c0 & 0x001F) << 16) | ((c0 & 0xF800) >> 11)) * 2106U) + 0x007F007FU;
  unsigned int  g0 = ((c0 & 0x07E0) * (1036U << 3)) + 0x00008500U;
  c[0] = (((rb0 & 0xFF00FF00U) | (g0 & 0x00FF0000U)) >> 8) | a;
  unsigned int  rb1 =
      ((((c1 & 0x001F) << 16) | ((c1 & 0xF800) >> 11)) * 2106U) + 0x007F007FU;
  unsigned int  g1 = ((c1 & 0x07E0) * (1036U << 3)) + 0x00008500U;
  c[1] = (((rb1 & 0xFF00FF00U) | (g1 & 0x00FF0000U)) >> 8) | a;
  if (BRANCH_EXPECT(c0 > c1, true))
  {
    unsigned long long  c0l = rgba32ToRBGA64(c[0]);
    unsigned long long  c1l = rgba32ToRBGA64(c[1]);
    unsigned long long  tmp0 = (c0l + c0l + c1l) * 85U;
    unsigned long long  tmp1 = (c0l + c1l + c1l) * 85U;
    tmp0 = tmp0 + ((tmp0 & 0xFF00FF00FF00FF00ULL) >> 8) + 0x0080008000800080ULL;
    tmp1 = tmp1 + ((tmp1 & 0xFF00FF00FF00FF00ULL) >> 8) + 0x0080008000800080ULL;
    c[2] = rbga64ToRGBA32(tmp0 >> 8);
    c[3] = rbga64ToRGBA32(tmp1 >> 8);
  }
  else
  {
    c[2] = ((c[0] >> 1) & 0x7F7F7F7FU) + ((c[1] >> 1) & 0x7F7F7F7FU)
           + ((c[0] | c[1]) & 0x01010101U);
    c[3] = c[2] & ~a;
  }
  unsigned int  bc = FileBuffer::readUInt32Fast(src + 4);
  return bc;
}

SYSV_ABI size_t DDSTexture::decodeBlock_BC1(
    unsigned int *dst, const unsigned char *src, unsigned int w)
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

SYSV_ABI size_t DDSTexture::decodeBlock_BC2(
    unsigned int *dst, const unsigned char *src, unsigned int w)
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

SYSV_ABI size_t DDSTexture::decodeBlock_BC3(
    unsigned int *dst, const unsigned char *src, unsigned int w)
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

SYSV_ABI size_t DDSTexture::decodeBlock_BC4(
    unsigned int *dst, const unsigned char *src, unsigned int w)
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

SYSV_ABI size_t DDSTexture::decodeBlock_BC4S(
    unsigned int *dst, const unsigned char *src, unsigned int w)
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

SYSV_ABI size_t DDSTexture::decodeBlock_BC5(
    unsigned int *dst, const unsigned char *src, unsigned int w)
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

SYSV_ABI size_t DDSTexture::decodeBlock_BC5S(
    unsigned int *dst, const unsigned char *src, unsigned int w)
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

static inline unsigned int bgraToRGBA(unsigned int c)
{
  return ((c & 0xFF00FF00U)
          | ((c << 16) & 0x00FF0000U) | ((c >> 16) & 0x000000FFU));
}

SYSV_ABI size_t DDSTexture::decodeLine_RGB(
    unsigned int *dst, const unsigned char *src, unsigned int w)
{
  unsigned int  x = 0;
  for ( ; (x + 1U) < w; x++, dst++, src = src + 3)
    *dst = 0xFF000000U | FileBuffer::readUInt32Fast(src);
  if (BRANCH_EXPECT((x < w), true))
  {
    unsigned int  r = src[0];
    unsigned int  g = src[1];
    unsigned int  b = src[2];
    *dst = 0xFF000000U | r | (g << 8) | (b << 16);
  }
  return (size_t(w) * 3);
}

SYSV_ABI size_t DDSTexture::decodeLine_BGR(
    unsigned int *dst, const unsigned char *src, unsigned int w)
{
  unsigned int  x = 0;
  for ( ; (x + 1U) < w; x++, dst++, src = src + 3)
    *dst = bgraToRGBA(0xFF000000U | FileBuffer::readUInt32Fast(src));
  if (BRANCH_EXPECT((x < w), true))
  {
    unsigned int  b = src[0];
    unsigned int  g = src[1];
    unsigned int  r = src[2];
    *dst = 0xFF000000U | r | (g << 8) | (b << 16);
  }
  return (size_t(w) * 3);
}

SYSV_ABI size_t DDSTexture::decodeLine_RGB32(
    unsigned int *dst, const unsigned char *src, unsigned int w)
{
  for (unsigned int x = 0; x < w; x++, dst++, src = src + 4)
    *dst = 0xFF000000U | FileBuffer::readUInt32Fast(src);
  return (size_t(w) << 2);
}

SYSV_ABI size_t DDSTexture::decodeLine_BGR32(
    unsigned int *dst, const unsigned char *src, unsigned int w)
{
  for (unsigned int x = 0; x < w; x++, dst++, src = src + 4)
    *dst = bgraToRGBA(0xFF000000U | FileBuffer::readUInt32Fast(src));
  return (size_t(w) << 2);
}

SYSV_ABI size_t DDSTexture::decodeLine_RGBA(
    unsigned int *dst, const unsigned char *src, unsigned int w)
{
  for (unsigned int x = 0; x < w; x++, dst++, src = src + 4)
    *dst = FileBuffer::readUInt32Fast(src);
  return (size_t(w) << 2);
}

SYSV_ABI size_t DDSTexture::decodeLine_BGRA(
    unsigned int *dst, const unsigned char *src, unsigned int w)
{
  for (unsigned int x = 0; x < w; x++, dst++, src = src + 4)
    *dst = bgraToRGBA(FileBuffer::readUInt32Fast(src));
  return (size_t(w) << 2);
}

SYSV_ABI size_t DDSTexture::decodeLine_R8G8(
    unsigned int *dst, const unsigned char *src, unsigned int w)
{
  unsigned int  x = 0;
  for ( ; (x + 4U) <= w; x = x + 4U, dst = dst + 4, src = src + 8)
  {
    dst[0] = 0xFF000000U | FileBuffer::readUInt16Fast(src);
    dst[1] = 0xFF000000U | FileBuffer::readUInt16Fast(src + 2);
    dst[2] = 0xFF000000U | FileBuffer::readUInt16Fast(src + 4);
    dst[3] = 0xFF000000U | FileBuffer::readUInt16Fast(src + 6);
  }
  for ( ; x < w; x++, dst++, src = src + 2)
    *dst = 0xFF000000U | FileBuffer::readUInt16Fast(src);
  return (size_t(w) << 1);
}

void DDSTexture::loadTextureData(
    const unsigned char *srcPtr, int n, size_t blockSize,
    SYSV_ABI size_t (*decodeFunction)(unsigned int *,
                                      const unsigned char *, unsigned int))
{
  size_t  dataOffs = size_t(n) * textureDataSize;
  for (int i = 0; i < 20; i++)
  {
    unsigned int  *p = textureData[i] + dataOffs;
    unsigned int  w = xSizeMip0 >> i;
    unsigned int  h = ySizeMip0 >> i;
    if (w < 1)
      w = 1;
    if (h < 1)
      h = 1;
    if (i < mipLevelCnt)
    {
      if (blockSize > 16)
      {
        // uncompressed format
        for (unsigned int y = 0; y < h; y++)
          srcPtr = srcPtr + decodeFunction(p + (y * w), srcPtr, w);
      }
      else if (w < 4 || h < 4)
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
          p[y * w + x] =
              rbga64ToRGBA32(blendToRBGA64Bilinear(
                                 getPixelN(x << 1, y << 1, i - 1),
                                 getPixelN((x << 1) + 1, y << 1, i - 1),
                                 getPixelN(x << 1, (y << 1) + 1, i - 1),
                                 getPixelN((x << 1) + 1, (y << 1) + 1, i - 1),
                                 128, 128));
        }
      }
    }
  }
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
  isCubeMap = false;
  textureCnt = 1;
  ySizeMip0 = buf.readUInt32();
  xSizeMip0 = buf.readUInt32();
  if (ySizeMip0 < 1 || ySizeMip0 > 32768 ||
      xSizeMip0 < 1 || xSizeMip0 > 32768 ||
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
  unsigned int  dataOffs = 128;
  size_t  blockSize = 8;
  SYSV_ABI size_t (*decodeFunction)(unsigned int *, const unsigned char *,
                                    unsigned int) = &decodeBlock_BC1;
  unsigned int  formatFlags = buf.readUInt32();
  unsigned int  fourCC = buf.readUInt32();
  if (!(formatFlags & 0x04))            // DDPF_FOURCC
  {
    if (!(formatFlags & 0x40))          // DDPF_RGB
      throw errorMessage("unsupported texture file format");
    blockSize = buf.readUInt32();
    if (blockSize != 16 && blockSize != 24 && blockSize != 32)
      throw errorMessage("unsupported texture file format");
    blockSize = blockSize << 1;
    unsigned long long  rgMask = buf.readUInt64();
    unsigned long long  baMask = buf.readUInt64();
    if (blockSize < 64 || !(formatFlags & 0x03))
      baMask = baMask & ~0xFF00000000000000ULL;
    else
      haveAlpha = true;
    if (rgMask == 0x0000FF00000000FFULL && baMask == 0x0000000000FF0000ULL)
      decodeFunction = (blockSize < 64 ? &decodeLine_RGB : &decodeLine_RGB32);
    else if (rgMask == 0x0000FF0000FF0000ULL && baMask == 0x00000000000000FFULL)
      decodeFunction = (blockSize < 64 ? &decodeLine_BGR : &decodeLine_BGR32);
    else if (rgMask == 0x0000FF00000000FFULL && baMask == 0xFF00000000FF0000ULL)
      decodeFunction = &decodeLine_RGBA;
    else if (rgMask == 0x0000FF0000FF0000ULL && baMask == 0xFF000000000000FFULL)
      decodeFunction = &decodeLine_BGRA;
    else if (rgMask == 0x0000FF00000000FFULL && !baMask)
      decodeFunction = &decodeLine_R8G8;
    else
      throw errorMessage("unsupported texture file format");
  }
  else
  {
    if (FileBuffer::checkType(fourCC, "DX10"))
    {
      dataOffs = 148;
      buf.setPosition(0x80);
      unsigned int  tmp = buf.readUInt32();
      switch (tmp)
      {
        case 0x1C:                      // DXGI_FORMAT_R8G8B8A8_UNORM
        case 0x1D:                      // DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
          haveAlpha = true;
          blockSize = 64;
          decodeFunction = &decodeLine_RGBA;
          break;
        case 0x47:                      // DXGI_FORMAT_BC1_UNORM
        case 0x48:                      // DXGI_FORMAT_BC1_UNORM_SRGB
          break;
        case 0x4A:                      // DXGI_FORMAT_BC2_UNORM
        case 0x4B:                      // DXGI_FORMAT_BC2_UNORM_SRGB
          haveAlpha = true;
          blockSize = 16;
          decodeFunction = &decodeBlock_BC2;
          break;
        case 0x4D:                      // DXGI_FORMAT_BC3_UNORM
        case 0x4E:                      // DXGI_FORMAT_BC3_UNORM_SRGB
          haveAlpha = true;
          blockSize = 16;
          decodeFunction = &decodeBlock_BC3;
          break;
        case 0x50:                      // DXGI_FORMAT_BC4_UNORM
          decodeFunction = &decodeBlock_BC4;
          break;
        case 0x51:                      // DXGI_FORMAT_BC4_SNORM
          decodeFunction = &decodeBlock_BC4S;
          break;
        case 0x53:                      // DXGI_FORMAT_BC5_UNORM
          blockSize = 16;
          decodeFunction = &decodeBlock_BC5;
          break;
        case 0x54:                      // DXGI_FORMAT_BC5_SNORM
          blockSize = 16;
          decodeFunction = &decodeBlock_BC5S;
          break;
        case 0x57:                      // DXGI_FORMAT_B8G8R8A8_UNORM
          haveAlpha = true;
          blockSize = 64;
          decodeFunction = &decodeLine_BGRA;
          break;
        default:
          throw errorMessage("unsupported DXGI_FORMAT: 0x%08X", tmp);
      }
    }
    else
    {
      switch (fourCC)
      {
        case 0x31545844:                // "DXT1"
          break;
        case 0x32545844:                // "DXT2"
        case 0x33545844:                // "DXT3"
          haveAlpha = true;
          blockSize = 16;
          decodeFunction = &decodeBlock_BC2;
          break;
        case 0x34545844:                // "DXT4"
        case 0x35545844:                // "DXT5"
          haveAlpha = true;
          blockSize = 16;
          decodeFunction = &decodeBlock_BC3;
          break;
        default:
          throw errorMessage("unsupported DDS fourCC: 0x%08X",
                             FileBuffer::swapUInt32(fourCC));
      }
    }
  }
  size_t  sizeRequired = 0;
  if (blockSize > 16)
  {
    for (int i = 0; i < mipLevelCnt; i++)
    {
      unsigned int  w = xSizeMip0 >> i;
      unsigned int  h = ySizeMip0 >> i;
      if (w < 1)
        w = 1;
      if (h < 1)
        h = 1;
      sizeRequired = sizeRequired + (size_t(w) * h * (blockSize >> 4));
    }
  }
  else
  {
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
  }
  if (buf.size() > (dataOffs + sizeRequired))
  {
    size_t  n = (buf.size() - dataOffs) / sizeRequired;
    if (n > 255)
      n = 255;
    isCubeMap = (n == 6);
    textureCnt = (unsigned short) n;
  }
  else if (buf.size() < (dataOffs + sizeRequired))
  {
    throw errorMessage("DDS file is shorter than expected");
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
  textureDataSize = bufSize;
  textureDataBuf = new unsigned int[bufSize * size_t(textureCnt)];
  for (int i = 0; i < 20; i++)
    textureData[i] = textureDataBuf + dataOffsets[i];
  const unsigned char *srcPtr = buf.getDataPtr() + dataOffs;
  for ( ; mipOffset > 0 && mipLevelCnt > 1; mipOffset--, mipLevelCnt--)
  {
    unsigned int  w = xSizeMip0;
    unsigned int  h = ySizeMip0;
    if (blockSize > 16)
      srcPtr = srcPtr + (size_t(w) * h * (blockSize >> 4));
    else
      srcPtr = srcPtr + (size_t((w + 3) >> 2) * ((h + 3) >> 2) * blockSize);
    xSizeMip0 = (xSizeMip0 + 1) >> 1;
    ySizeMip0 = (ySizeMip0 + 1) >> 1;
  }
  for (size_t i = 0; i < textureCnt; i++)
  {
    loadTextureData(srcPtr + (i * sizeRequired), int(i),
                    blockSize, decodeFunction);
  }
  for ( ; mipOffset > 0; mipOffset--)
  {
    xSizeMip0 = (xSizeMip0 + 1) >> 1;
    ySizeMip0 = (ySizeMip0 + 1) >> 1;
    for (int i = 0; (i + 1) < 20; i++)
      textureData[i] = textureData[i + 1];
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
  if (textureDataBuf)
    delete[] textureDataBuf;
}

SYSV_ABI FloatVector4 DDSTexture::getPixelB(
    float x, float y, int mipLevel) const
{
  fixNegativeMipLevel(x, y, mipLevel);
  int     x0, y0;
  float   xf, yf;
  convertTexCoord(x0, y0, xf, yf, x, y);
  return blendFP32Vec4Bilinear(getPixelN(x0, y0, mipLevel),
                               getPixelN(x0 + 1, y0, mipLevel),
                               getPixelN(x0, y0 + 1, mipLevel),
                               getPixelN(x0 + 1, y0 + 1, mipLevel), xf, yf);
}

SYSV_ABI FloatVector4 DDSTexture::getPixelT(
    float x, float y, float mipLevel) const
{
  int     m0 = int(float(std::floor(mipLevel)));
  float   mf = mipLevel - float(m0);
  fixNegativeMipLevel(x, y, m0);
  if (BRANCH_EXPECT(!(((xSizeMip0 - 1) | (ySizeMip0 - 1)) >> m0), false))
    return FloatVector4(textureData[m0][0]);
  int     x0, y0;
  float   xf, yf;
  convertTexCoord(x0, y0, xf, yf, x, y);
  FloatVector4  c0(blendFP32Vec4Bilinear(
                       getPixelN(x0, y0, m0),
                       getPixelN(x0 + 1, y0, m0),
                       getPixelN(x0, y0 + 1, m0),
                       getPixelN(x0 + 1, y0 + 1, m0), xf, yf));
  if (BRANCH_EXPECT((mf >= (1.0f / 512.0f) && mf < (511.0f / 512.0f)), true))
  {
    convertTexCoord(x0, y0, xf, yf, x * 0.5f, y * 0.5f);
    FloatVector4  c1(blendFP32Vec4Bilinear(
                         getPixelN(x0, y0, m0 + 1),
                         getPixelN(x0 + 1, y0, m0 + 1),
                         getPixelN(x0, y0 + 1, m0 + 1),
                         getPixelN(x0 + 1, y0 + 1, m0 + 1), xf, yf));
    c0 *= (1.0f - mf);
    c1 *= mf;
    c0 += c1;
  }
  return c0;
}

SYSV_ABI FloatVector4 DDSTexture::getPixelBM(
    float x, float y, int mipLevel) const
{
  fixNegativeMipLevel(x, y, mipLevel);
  int     x0, y0;
  float   xf, yf;
  convertTexCoord(x0, y0, xf, yf, x, y);
  return blendFP32Vec4Bilinear(getPixelM(x0, y0, mipLevel),
                               getPixelM(x0 + 1, y0, mipLevel),
                               getPixelM(x0, y0 + 1, mipLevel),
                               getPixelM(x0 + 1, y0 + 1, mipLevel), xf, yf);
}

SYSV_ABI FloatVector4 DDSTexture::getPixelTM(
    float x, float y, float mipLevel) const
{
  int     m0 = int(float(std::floor(mipLevel)));
  float   mf = mipLevel - float(m0);
  fixNegativeMipLevel(x, y, m0);
  if (BRANCH_EXPECT(!(((xSizeMip0 - 1) | (ySizeMip0 - 1)) >> m0), false))
    return FloatVector4(textureData[m0][0]);
  int     x0, y0;
  float   xf, yf;
  convertTexCoord(x0, y0, xf, yf, x, y);
  FloatVector4  c0(blendFP32Vec4Bilinear(
                       getPixelM(x0, y0, m0),
                       getPixelM(x0 + 1, y0, m0),
                       getPixelM(x0, y0 + 1, m0),
                       getPixelM(x0 + 1, y0 + 1, m0), xf, yf));
  if (BRANCH_EXPECT((mf >= (1.0f / 512.0f) && mf < (511.0f / 512.0f)), true))
  {
    convertTexCoord(x0, y0, xf, yf, x * 0.5f, y * 0.5f);
    FloatVector4  c1(blendFP32Vec4Bilinear(
                         getPixelM(x0, y0, m0 + 1),
                         getPixelM(x0 + 1, y0, m0 + 1),
                         getPixelM(x0, y0 + 1, m0 + 1),
                         getPixelM(x0 + 1, y0 + 1, m0 + 1), xf, yf));
    c0 *= (1.0f - mf);
    c1 *= mf;
    c0 += c1;
  }
  return c0;
}

SYSV_ABI FloatVector4 DDSTexture::getPixelBC(
    float x, float y, int mipLevel) const
{
  fixNegativeMipLevel(x, y, mipLevel);
  int     x0, y0;
  float   xf, yf;
  convertTexCoord(x0, y0, xf, yf, x, y);
  return blendFP32Vec4Bilinear(getPixelC(x0, y0, mipLevel),
                               getPixelC(x0 + 1, y0, mipLevel),
                               getPixelC(x0, y0 + 1, mipLevel),
                               getPixelC(x0 + 1, y0 + 1, mipLevel), xf, yf);
}

SYSV_ABI FloatVector4 DDSTexture::getPixelTC(
    float x, float y, float mipLevel) const
{
  int     m0 = int(float(std::floor(mipLevel)));
  float   mf = mipLevel - float(m0);
  fixNegativeMipLevel(x, y, m0);
  if (BRANCH_EXPECT(!(((xSizeMip0 - 1) | (ySizeMip0 - 1)) >> m0), false))
    return FloatVector4(textureData[m0][0]);
  int     x0, y0;
  float   xf, yf;
  convertTexCoord(x0, y0, xf, yf, x, y);
  FloatVector4  c0(blendFP32Vec4Bilinear(
                       getPixelC(x0, y0, m0),
                       getPixelC(x0 + 1, y0, m0),
                       getPixelC(x0, y0 + 1, m0),
                       getPixelC(x0 + 1, y0 + 1, m0), xf, yf));
  if (BRANCH_EXPECT((mf >= (1.0f / 512.0f) && mf < (511.0f / 512.0f)), true))
  {
    convertTexCoord(x0, y0, xf, yf, x * 0.5f, y * 0.5f);
    FloatVector4  c1(blendFP32Vec4Bilinear(
                         getPixelC(x0, y0, m0 + 1),
                         getPixelC(x0 + 1, y0, m0 + 1),
                         getPixelC(x0, y0 + 1, m0 + 1),
                         getPixelC(x0 + 1, y0 + 1, m0 + 1), xf, yf));
    c0 *= (1.0f - mf);
    c1 *= mf;
    c0 += c1;
  }
  return c0;
}

SYSV_ABI FloatVector4 DDSTexture::cubeMap(
    float x, float y, float z, float mipLevel) const
{
  float   xm = float(std::fabs(x));
  float   ym = float(std::fabs(y));
  float   zm = float(std::fabs(z));
  size_t  n = 0;
  if (xm >= ym && xm >= zm)             // right (0), left (1)
  {
    float   tmp = 1.0f / xm;
    if (x < 0.0f)
    {
      z = -z;
      n++;
    }
    x = z * tmp;
    y = y * tmp;
  }
  else if (ym >= xm && ym >= zm)        // front (2), back (3)
  {
    float   tmp = 1.0f / ym;
    n = 2;
    if (y < 0.0f)
    {
      z = -z;
      n++;
    }
    x = x * tmp;
    y = z * tmp;
  }
  else                                  // bottom (4), top (5)
  {
    float   tmp = 1.0f / zm;
    n = 4;
    if (z >= 0.0f)
    {
      x = -x;
      n++;
    }
    x = x * tmp;
    y = y * tmp;
  }
  if (BRANCH_EXPECT(!isCubeMap, false))
    n = 0;
  else
    n *= textureDataSize;
  mipLevel = (mipLevel > 0.0f ? mipLevel : 0.0f);
  int     m0 = int(float(std::floor(mipLevel)));
  float   mf = mipLevel - float(m0);
  int     w = int((xSizeMip0 - 1U) >> (unsigned char) m0);
  int     h = int((ySizeMip0 - 1U) >> (unsigned char) m0);
  float   txtX = (x + 1.0f) * (float(w) * 0.5f);
  float   txtY = (1.0f - y) * (float(h) * 0.5f);
  int     x0, y0;
  float   xf, yf;
  convertTexCoord(x0, y0, xf, yf, txtX, txtY);
  const unsigned int  *p = textureData[m0] + n;
  FloatVector4  c0(blendFP32Vec4Bilinear(
                       p[(y0 & h) * (w + 1) + (x0 & w)],
                       p[(y0 & h) * (w + 1) + ((x0 + 1) & w)],
                       p[((y0 + 1) & h) * (w + 1) + (x0 & w)],
                       p[((y0 + 1) & h) * (w + 1) + ((x0 + 1) & w)], xf, yf));
  if (BRANCH_EXPECT((mf >= (1.0f / 512.0f) && mf < (511.0f / 512.0f)), true))
  {
    w = w >> 1;
    h = h >> 1;
    txtX = (x + 1.0f) * (float(w) * 0.5f);
    txtY = (1.0f - y) * (float(h) * 0.5f);
    convertTexCoord(x0, y0, xf, yf, txtX, txtY);
    p = textureData[m0 + 1] + n;
    FloatVector4  c1(blendFP32Vec4Bilinear(
                         p[(y0 & h) * (w + 1) + (x0 & w)],
                         p[(y0 & h) * (w + 1) + ((x0 + 1) & w)],
                         p[((y0 + 1) & h) * (w + 1) + (x0 & w)],
                         p[((y0 + 1) & h) * (w + 1) + ((x0 + 1) & w)], xf, yf));
    c0 *= (1.0f - mf);
    c1 *= mf;
    c0 += c1;
  }
  return c0;
}

struct Downsample2xTable
{
  //   1   0  -3   0  10  16  10   0  -3   0   1
  //   0   0   0   0   0   0   0   0   0   0   0
  //  -3   0   9   0 -30 -48 -30   0   9   0  -3
  //   0   0   0   0   0   0   0   0   0   0   0
  //  10   0 -30   0 100 160 100   0 -30   0  10
  //  16   0 -48   0 160 256 160   0 -48   0  16
  //  10   0 -30   0 100 160 100   0 -30   0  10
  //   0   0   0   0   0   0   0   0   0   0   0
  //  -3   0   9   0 -30 -48 -30   0   9   0  -3
  //   0   0   0   0   0   0   0   0   0   0   0
  //   1   0  -3   0  10  16  10   0  -3   0   1
  // Y offset, (X offset, multiplier) * 4
  static const int  filterTable[27];
  static inline void getPixelFast(FloatVector4& c, const unsigned int *buf,
                                  int w, int x, int y)
  {
    FloatVector4  tmp(*(buf + (y * w + x)));
    tmp *= tmp;
    c += tmp;
  }
  static inline void getPixelFast(FloatVector4& c, const unsigned int *buf,
                                  int w, int x, int y, int m)
  {
    FloatVector4  tmp(*(buf + (y * w + x)));
    tmp *= tmp;
    tmp *= float(m);
    c += tmp;
  }
  static inline void getPixel(FloatVector4& c, const unsigned int *buf,
                              int w, int h, int x, int y)
  {
    x = (x > 0 ? (x < (w - 1) ? x : (w - 1)) : 0);
    y = (y > 0 ? (y < (h - 1) ? y : (h - 1)) : 0);
    FloatVector4  tmp(buf[size_t(y) * size_t(w) + size_t(x)]);
    tmp *= tmp;
    c += tmp;
  }
  static inline void getPixel(FloatVector4& c, const unsigned int *buf,
                              int w, int h, int x, int y, int m)
  {
    x = (x > 0 ? (x < (w - 1) ? x : (w - 1)) : 0);
    y = (y > 0 ? (y < (h - 1) ? y : (h - 1)) : 0);
    FloatVector4  tmp(buf[size_t(y) * size_t(w) + size_t(x)]);
    tmp *= tmp;
    tmp *= float(m);
    c += tmp;
  }
};

const int Downsample2xTable::filterTable[27] =
{
  1,  0, 160,  1, 100,  3, -30,  5,  10,
  3,  0, -48,  1, -30,  3,   9,  5,  -3,
  5,  0,  16,  1,  10,  3,  -3,  5,   1
};

SYSV_ABI unsigned int downsample2xFilter(
    const unsigned int *buf, int imageWidth, int imageHeight, int x, int y)
{
  static Downsample2xTable  t;
  const int   *p = t.filterTable;
  FloatVector4  c(0U);
  if (BRANCH_EXPECT((x >= 5 && x < (imageWidth - 5) &&
                     y >= 5 && y < (imageHeight - 5)), true))
  {
    buf = buf + (size_t(y) * size_t(imageWidth) + size_t(x));
    t.getPixelFast(c, buf, imageWidth, 0, 0, 256);
    for (int i = 0; i < 3; i++)
    {
      int     yOffs = *(p++);
      for (int j = 0; j < 4; j++, p = p + 2)
      {
        FloatVector4  tmp(0U);
        int     xOffs = p[0];
        t.getPixelFast(tmp, buf, imageWidth, xOffs, yOffs);
        t.getPixelFast(tmp, buf, imageWidth, yOffs, -xOffs);
        t.getPixelFast(tmp, buf, imageWidth, -xOffs, -yOffs);
        t.getPixelFast(tmp, buf, imageWidth, -yOffs, xOffs);
        tmp *= float(p[1]);
        c += tmp;
      }
    }
  }
  else
  {
    t.getPixel(c, buf, imageWidth, imageHeight, x, y, 256);
    for (int i = 0; i < 3; i++)
    {
      int     yOffs = *(p++);
      for (int j = 0; j < 4; j++, p = p + 2)
      {
        FloatVector4  tmp(0U);
        int     xOffs = p[0];
        t.getPixel(tmp, buf, imageWidth, imageHeight, x + xOffs, y + yOffs);
        t.getPixel(tmp, buf, imageWidth, imageHeight, x + yOffs, y - xOffs);
        t.getPixel(tmp, buf, imageWidth, imageHeight, x - xOffs, y - yOffs);
        t.getPixel(tmp, buf, imageWidth, imageHeight, x - yOffs, y + xOffs);
        tmp *= float(p[1]);
        c += tmp;
      }
    }
  }
  c *= (1.0f / 1024.0f);
  c.squareRoot();
  return (unsigned int) c;
}

