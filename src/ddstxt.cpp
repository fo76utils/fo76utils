
#include "common.hpp"
#include "ddstxt.hpp"

#include <thread>

static inline std::uint64_t decodeBC3Alpha(std::uint64_t& a,
                                           const unsigned char *src,
                                           bool isSigned = false)
{
  unsigned int  a0 = src[0] ^ (!isSigned ? 0U : 0x80U);
  unsigned int  a1 = src[1] ^ (!isSigned ? 0U : 0x80U);
  FloatVector4  a0_f((float) int(a0));
  FloatVector4  a1_f((float) int(a1));
  a1_f -= a0_f;
  if (a0 > a1)
  {
    FloatVector4  m(0.0f, 1.0f, 1.0f / 7.0f, 2.0f / 7.0f);
    a = std::uint32_t(a0_f + (a1_f * m));
    m = FloatVector4(3.0f / 7.0f, 4.0f / 7.0f, 5.0f / 7.0f, 6.0f / 7.0f);
    a = a | (std::uint64_t(std::uint32_t(a0_f + (a1_f * m))) << 32);
  }
  else
  {
    a = a0 | (a1 << 8) | 0xFF00000000000000ULL;
    FloatVector4  m(0.2f, 0.4f, 0.6f, 0.8f);
    a = a | (std::uint64_t(std::uint32_t(a0_f + (a1_f * m))) << 16);
  }
  std::uint64_t ba = FileBuffer::readUInt16Fast(src + 2);
  ba = ba | (std::uint64_t(FileBuffer::readUInt32Fast(src + 4)) << 16);
  return ba;
}

static inline std::uint32_t decodeAlphaChannel(const std::uint64_t& a,
                                               std::uint64_t ba)
{
#if defined(__i386__) || defined(__x86_64__) || defined(__x86_64)
  return reinterpret_cast< const unsigned char * >(&a)[ba & 7];
#else
  return (std::uint32_t(a >> (unsigned char) ((ba & 7) << 3)) & 0xFFU);
#endif
}

static inline std::uint32_t decodeBC1Colors(std::uint32_t *c,
                                            const unsigned char *src,
                                            std::uint32_t a = 0U)
{
  std::uint32_t c0 = FileBuffer::readUInt16Fast(src);
  std::uint32_t c1 = FileBuffer::readUInt16Fast(src + 2);
  FloatVector4  c0_f(((c0 & 0x001FU) << 16) | ((c0 & 0x07E0U) << 3)
                     | ((c0 & 0xF800U) >> 11) | a);
  FloatVector4  c1_f(((c1 & 0x001FU) << 16) | ((c1 & 0x07E0U) << 3)
                     | ((c1 & 0xF800U) >> 11) | a);
  c0_f *= FloatVector4(255.0f / 31.0f, 255.0f / 63.0f, 255.0f / 31.0f, 1.0f);
  c1_f *= FloatVector4(255.0f / 31.0f, 255.0f / 63.0f, 255.0f / 31.0f, 1.0f);
  c[0] = std::uint32_t(c0_f);
  c[1] = std::uint32_t(c1_f);
  if (BRANCH_LIKELY(c0 > c1))
  {
    c[2] = std::uint32_t((c0_f + c0_f + c1_f) * FloatVector4(1.0f / 3.0f));
    c[3] = std::uint32_t((c0_f + c1_f + c1_f) * FloatVector4(1.0f / 3.0f));
  }
  else
  {
    c[2] = std::uint32_t((c0_f + c1_f) * FloatVector4(0.5f));
    c[3] = c[2] & ~a;
  }
  return FileBuffer::readUInt32Fast(src + 4);
}

size_t DDSTexture::decodeBlock_BC1(
    std::uint32_t *dst, const unsigned char *src, unsigned int w)
{
  std::uint32_t c[4];
  std::uint32_t bc = decodeBC1Colors(c, src, 0xFF000000U);
  for (unsigned int i = 0; i < 16; i++)
  {
    dst[(i >> 2) * w + (i & 3)] = c[bc & 3];
    bc = bc >> 2;
  }
  return 8;
}

size_t DDSTexture::decodeBlock_BC2(
    std::uint32_t *dst, const unsigned char *src, unsigned int w)
{
  std::uint32_t c[4];
  std::uint32_t bc = decodeBC1Colors(c, src + 8);
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

size_t DDSTexture::decodeBlock_BC3(
    std::uint32_t *dst, const unsigned char *src, unsigned int w)
{
  std::uint32_t c[4];
  std::uint64_t a;
  std::uint64_t ba = decodeBC3Alpha(a, src);
  std::uint32_t bc = decodeBC1Colors(c, src + 8);
  for (unsigned int i = 0; i < 16; i++)
  {
    dst[(i >> 2) * w + (i & 3)] = c[bc & 3] | (decodeAlphaChannel(a, ba) << 24);
    ba = ba >> 3;
    bc = bc >> 2;
  }
  return 16;
}

size_t DDSTexture::decodeBlock_BC4(
    std::uint32_t *dst, const unsigned char *src, unsigned int w)
{
  std::uint64_t a;
  std::uint64_t ba = decodeBC3Alpha(a, src);
  for (unsigned int i = 0; i < 16; i++)
  {
    dst[(i >> 2) * w + (i & 3)] =
        (decodeAlphaChannel(a, ba) * 0x00010101U) | 0xFF000000U;
    ba = ba >> 3;
  }
  return 8;
}

size_t DDSTexture::decodeBlock_BC4S(
    std::uint32_t *dst, const unsigned char *src, unsigned int w)
{
  std::uint64_t a;
  std::uint64_t ba = decodeBC3Alpha(a, src, true);
  for (unsigned int i = 0; i < 16; i++)
  {
    dst[(i >> 2) * w + (i & 3)] =
        (decodeAlphaChannel(a, ba) * 0x00010101U) | 0xFF000000U;
    ba = ba >> 3;
  }
  return 8;
}

size_t DDSTexture::decodeBlock_BC5(
    std::uint32_t *dst, const unsigned char *src, unsigned int w)
{
  std::uint64_t a1;
  std::uint64_t a2;
  std::uint64_t ba1 = decodeBC3Alpha(a1, src);
  std::uint64_t ba2 = decodeBC3Alpha(a2, src + 8);
  for (unsigned int i = 0; i < 16; i++)
  {
    dst[(i >> 2) * w + (i & 3)] =
        decodeAlphaChannel(a1, ba1) | (decodeAlphaChannel(a2, ba2) << 8)
        | 0xFF000000U;
    ba1 = ba1 >> 3;
    ba2 = ba2 >> 3;
  }
  return 16;
}

size_t DDSTexture::decodeBlock_BC5S(
    std::uint32_t *dst, const unsigned char *src, unsigned int w)
{
  std::uint64_t a1;
  std::uint64_t a2;
  std::uint64_t ba1 = decodeBC3Alpha(a1, src, true);
  std::uint64_t ba2 = decodeBC3Alpha(a2, src + 8, true);
  for (unsigned int i = 0; i < 16; i++)
  {
    dst[(i >> 2) * w + (i & 3)] =
        decodeAlphaChannel(a1, ba1) | (decodeAlphaChannel(a2, ba2) << 8)
        | 0xFF000000U;
    ba1 = ba1 >> 3;
    ba2 = ba2 >> 3;
  }
  return 16;
}

static inline std::uint32_t bgraToRGBA(std::uint32_t c)
{
  return ((c & 0xFF00FF00U)
          | ((c << 16) & 0x00FF0000U) | ((c >> 16) & 0x000000FFU));
}

size_t DDSTexture::decodeLine_RGB(
    std::uint32_t *dst, const unsigned char *src, unsigned int w)
{
  unsigned int  x = 0;
  for ( ; (x + 1U) < w; x++, dst++, src = src + 3)
    *dst = 0xFF000000U | FileBuffer::readUInt32Fast(src);
  if (BRANCH_LIKELY(x < w))
  {
    std::uint32_t r = src[0];
    std::uint32_t g = src[1];
    std::uint32_t b = src[2];
    *dst = 0xFF000000U | r | (g << 8) | (b << 16);
  }
  return (size_t(w) * 3);
}

size_t DDSTexture::decodeLine_BGR(
    std::uint32_t *dst, const unsigned char *src, unsigned int w)
{
  unsigned int  x = 0;
  for ( ; (x + 1U) < w; x++, dst++, src = src + 3)
    *dst = bgraToRGBA(0xFF000000U | FileBuffer::readUInt32Fast(src));
  if (BRANCH_LIKELY(x < w))
  {
    std::uint32_t b = src[0];
    std::uint32_t g = src[1];
    std::uint32_t r = src[2];
    *dst = 0xFF000000U | r | (g << 8) | (b << 16);
  }
  return (size_t(w) * 3);
}

size_t DDSTexture::decodeLine_RGB32(
    std::uint32_t *dst, const unsigned char *src, unsigned int w)
{
  for (unsigned int x = 0; x < w; x++, dst++, src = src + 4)
    *dst = 0xFF000000U | FileBuffer::readUInt32Fast(src);
  return (size_t(w) << 2);
}

size_t DDSTexture::decodeLine_BGR32(
    std::uint32_t *dst, const unsigned char *src, unsigned int w)
{
  for (unsigned int x = 0; x < w; x++, dst++, src = src + 4)
    *dst = bgraToRGBA(0xFF000000U | FileBuffer::readUInt32Fast(src));
  return (size_t(w) << 2);
}

size_t DDSTexture::decodeLine_RGBA(
    std::uint32_t *dst, const unsigned char *src, unsigned int w)
{
  for (unsigned int x = 0; x < w; x++, dst++, src = src + 4)
    *dst = FileBuffer::readUInt32Fast(src);
  return (size_t(w) << 2);
}

size_t DDSTexture::decodeLine_BGRA(
    std::uint32_t *dst, const unsigned char *src, unsigned int w)
{
  for (unsigned int x = 0; x < w; x++, dst++, src = src + 4)
    *dst = bgraToRGBA(FileBuffer::readUInt32Fast(src));
  return (size_t(w) << 2);
}

size_t DDSTexture::decodeLine_R8G8(
    std::uint32_t *dst, const unsigned char *src, unsigned int w)
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
    size_t (*decodeFunction)(std::uint32_t *,
                             const unsigned char *, unsigned int))
{
  size_t  dataOffs = size_t(n) * textureDataSize;
  for (int i = 0; i < 20; i++)
  {
    std::uint32_t *p = textureData[i] + dataOffs;
    unsigned int  w = ((xSizeMip0 - 1U) >> i) + 1U;
    unsigned int  h = ((ySizeMip0 - 1U) >> i) + 1U;
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
        std::uint32_t tmpBuf[16];
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
      // generate missing mipmaps
      const std::uint32_t *p2 =
          textureData[i - 1] + (textureDataSize * size_t(n));
      unsigned int  xMask = (xSizeMip0 - 1U) >> (i - 1);
      unsigned int  yMask = (ySizeMip0 - 1U) >> (i - 1);
      size_t  w2 = size_t(xMask + 1U);
      for (unsigned int y = 0; y < h; y++)
      {
        size_t  offsY2 = size_t((y << 1) & yMask) * w2;
        size_t  offsY2p1 = size_t(((y << 1) + 1U) & yMask) * w2;
#if 1
        for (unsigned int x = 0; x < w; x++)
        {
          size_t  offsX2 = (x << 1) & xMask;
          size_t  offsX2p1 = ((x << 1) + 1U) & xMask;
          FloatVector4  c(p2 + (offsY2 + offsX2));
          c += FloatVector4(p2 + (offsY2 + offsX2p1));
          c += FloatVector4(p2 + (offsY2p1 + offsX2));
          c += FloatVector4(p2 + (offsY2p1 + offsX2p1));
          p[y * w + x] = std::uint32_t(c * FloatVector4(0.25f));
        }
#else
        size_t  offsY2m1 = size_t(((y << 1) - 1U) & yMask) * w2;
        for (unsigned int x = 0; x < w; x++)
        {
          size_t  offsX2 = (x << 1) & xMask;
          size_t  offsX2m1 = ((x << 1) - 1U) & xMask;
          size_t  offsX2p1 = ((x << 1) + 1U) & xMask;
          FloatVector4  c1(p2 + (offsY2 + offsX2m1));
          FloatVector4  c0(p2 + (offsY2 + offsX2));
          c1 += FloatVector4(p2 + (offsY2 + offsX2p1));
          FloatVector4  c2(p2 + (offsY2p1 + offsX2m1));
          c1 += FloatVector4(p2 + (offsY2p1 + offsX2));
          c2 += FloatVector4(p2 + (offsY2p1 + offsX2p1));
          c2 += FloatVector4(p2 + (offsY2m1 + offsX2m1));
          c1 += FloatVector4(p2 + (offsY2m1 + offsX2));
          c2 += FloatVector4(p2 + (offsY2m1 + offsX2p1));
          p[y * w + x] = std::uint32_t((c0 * FloatVector4(0.23031584f))
                                       + (c1 * FloatVector4(0.12479824f))
                                       + (c2 * FloatVector4(0.06762280f)));
        }
#endif
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
  size_t  (*decodeFunction)(std::uint32_t *, const unsigned char *,
                            unsigned int) = &decodeBlock_BC1;
  unsigned int  formatFlags = buf.readUInt32();
  unsigned int  fourCC = buf.readUInt32();
  if (formatFlags & 0x04)               // DDPF_FOURCC
  {
    if (FileBuffer::checkType(fourCC, "DX10"))
    {
      dataOffs = 148;
      buf.setPosition(0x80);
      unsigned int  tmp = buf.readUInt32();
      switch (tmp)
      {
        case 0x00:                      // DXGI_FORMAT_UNKNOWN
          buf.setPosition(0x58);
          formatFlags &= ~0x04U;
          break;
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
        case 0x53344342:                // "BC4S"
          decodeFunction = &decodeBlock_BC4S;
          break;
        case 0x55344342:                // "BC4U"
          decodeFunction = &decodeBlock_BC4;
          break;
        case 0x53354342:                // "BC5S"
          blockSize = 16;
          decodeFunction = &decodeBlock_BC5S;
          break;
        case 0x55354342:                // "BC5U"
          blockSize = 16;
          decodeFunction = &decodeBlock_BC5;
          break;
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
  textureDataBuf = new std::uint32_t[bufSize * size_t(textureCnt)];
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

inline bool DDSTexture::convertTexCoord(
    int& x0, int& y0, float& xf, float& yf,
    unsigned int& xMask, unsigned int& yMask,
    float x, float y, int mipLevel, bool m) const
{
  xMask = (xSizeMip0 - 1U) >> (unsigned char) mipLevel;
  yMask = (ySizeMip0 - 1U) >> (unsigned char) mipLevel;
  xf = x * float(int(xMask + (!m ? 1U : 0U)));
  yf = y * float(int(yMask + (!m ? 1U : 0U)));
  float   xi = float(std::floor(xf));
  float   yi = float(std::floor(yf));
  x0 = int(xi);
  y0 = int(yi);
  xf -= xi;
  yf -= yi;
  return bool(xMask | yMask);
}

inline FloatVector4 DDSTexture::getPixelB(
    const std::uint32_t *p, int x0, int y0,
    float xf, float yf, unsigned int xMask, unsigned int yMask)
{
  unsigned int  w = xMask + 1U;
  unsigned int  x0u = (unsigned int) x0;
  unsigned int  y0u = (unsigned int) y0;
  unsigned int  x1 = (x0u + 1U) & xMask;
  unsigned int  y1 = (y0u + 1U) & yMask;
  x0u = x0u & xMask;
  y0u = y0u & yMask;
  return FloatVector4(p + (y0u * w + x0u), p + (y0u * w + x1),
                      p + (y1 * w + x0u), p + (y1 * w + x1), xf, yf);
}

inline FloatVector4 DDSTexture::getPixelB_2(
    const std::uint32_t *p1, const std::uint32_t *p2, int x0, int y0,
    float xf, float yf, unsigned int xMask, unsigned int yMask)
{
  unsigned int  w = xMask + 1U;
  unsigned int  x0u = (unsigned int) x0;
  unsigned int  y0u = (unsigned int) y0;
  unsigned int  x1 = (x0u + 1U) & xMask;
  unsigned int  y1 = (y0u + 1U) & yMask;
  x0u = x0u & xMask;
  y0u = y0u & yMask;
  return FloatVector4(p1 + (y0u * w + x0u), p2 + (y0u * w + x0u),
                      p1 + (y0u * w + x1), p2 + (y0u * w + x1),
                      p1 + (y1 * w + x0u), p2 + (y1 * w + x0u),
                      p1 + (y1 * w + x1), p2 + (y1 * w + x1), xf, yf);
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

DDSTexture::DDSTexture(std::uint32_t c)
  : xSizeMip0(1U),
    ySizeMip0(1U),
    mipLevelCnt(1),
    haveAlpha(bool(~c & 0xFF000000U)),
    isCubeMap(false),
    textureCnt(1),
    textureDataSize(0),
    textureDataBuf((std::uint32_t *) 0)
{
  unsigned char *p = reinterpret_cast< unsigned char * >(&textureDataSize);
  for (size_t i = 0; i < (sizeof(textureData) / sizeof(std::uint32_t *)); i++)
    textureData[i] = reinterpret_cast< std::uint32_t * >(p);
  *(textureData[0]) = c;
}

DDSTexture::~DDSTexture()
{
  if (textureDataBuf)
    delete[] textureDataBuf;
}

FloatVector4 DDSTexture::getPixelB(float x, float y, int mipLevel) const
{
  mipLevel = (mipLevel > 0 ? mipLevel : 0);
  int     x0, y0;
  float   xf, yf;
  unsigned int  xMask, yMask;
  (void) convertTexCoord(x0, y0, xf, yf, xMask, yMask, x, y, mipLevel);
  return getPixelB(textureData[mipLevel], x0, y0, xf, yf, xMask, yMask);
}

FloatVector4 DDSTexture::getPixelT(float x, float y, float mipLevel) const
{
  mipLevel = (mipLevel > 0.0f ? mipLevel : 0.0f);
  int     m0 = int(mipLevel);
  float   mf = mipLevel - float(m0);
  int     x0, y0;
  float   xf, yf;
  unsigned int  xMask, yMask;
  if (BRANCH_UNLIKELY(!convertTexCoord(x0, y0, xf, yf, xMask, yMask, x, y, m0)))
    return FloatVector4(textureData[m0]);
  FloatVector4  c0(getPixelB(textureData[m0], x0, y0, xf, yf, xMask, yMask));
  if (BRANCH_LIKELY(mf >= (1.0f / 512.0f) && mf < (511.0f / 512.0f)))
  {
    xf = (xf + float(x0 & 1)) * 0.5f;
    yf = (yf + float(y0 & 1)) * 0.5f;
    x0 = int((unsigned int) x0 >> 1);
    y0 = int((unsigned int) y0 >> 1);
    FloatVector4  c1(getPixelB(textureData[m0 + 1], x0, y0, xf, yf,
                               xMask >> 1, yMask >> 1));
    c0 = (c0 * FloatVector4(1.0f - mf)) + (c1 * FloatVector4(mf));
  }
  return c0;
}

FloatVector4 DDSTexture::getPixelT_2(float x, float y, float mipLevel,
                                     const DDSTexture& t) const
{
  mipLevel = (mipLevel > 0.0f ? mipLevel : 0.0f);
  int     m0 = int(mipLevel);
  float   mf = mipLevel - float(m0);
  const std::uint32_t * const *t1 = &(textureData[m0]);
  const std::uint32_t * const *t2 = &(t.textureData[m0]);
  unsigned int  w = xSizeMip0;
  unsigned int  h = ySizeMip0;
  if (BRANCH_UNLIKELY(!(t.xSizeMip0 == w && t.ySizeMip0 == h)))
  {
    if ((t.xSizeMip0 << 1) == w && (t.ySizeMip0 << 1) == h && m0 > 0)
    {
      t2--;
    }
    else if (t.xSizeMip0 == (w << 1) && t.ySizeMip0 == (h << 1))
    {
      t2++;
    }
    else
    {
      FloatVector4  tmp1(getPixelT(x, y, mipLevel));
      FloatVector4  tmp2(t.getPixelT(x, y, mipLevel));
      tmp1[2] = tmp2[0];
      tmp1[3] = tmp2[1];
      return tmp1;
    }
  }
  int     x0, y0;
  float   xf, yf;
  unsigned int  xMask, yMask;
  if (BRANCH_UNLIKELY(!convertTexCoord(x0, y0, xf, yf, xMask, yMask, x, y, m0)))
    return FloatVector4(t1[0], t2[0]);
  FloatVector4  c0(getPixelB_2(t1[0], t2[0], x0, y0, xf, yf, xMask, yMask));
  if (BRANCH_LIKELY(mf >= (1.0f / 512.0f) && mf < (511.0f / 512.0f)))
  {
    xf = (xf + float(x0 & 1)) * 0.5f;
    yf = (yf + float(y0 & 1)) * 0.5f;
    x0 = int((unsigned int) x0 >> 1);
    y0 = int((unsigned int) y0 >> 1);
    FloatVector4  c1(getPixelB_2(t1[1], t2[1], x0, y0, xf, yf,
                                 xMask >> 1, yMask >> 1));
    c0 = (c0 * FloatVector4(1.0f - mf)) + (c1 * FloatVector4(mf));
  }
  return c0;
}

FloatVector4 DDSTexture::getPixelT_N(float x, float y, float mipLevel) const
{
  mipLevel = (mipLevel > 0.0f ? mipLevel : 0.0f);
  int     m0 = int(mipLevel);
  float   mf = mipLevel - float(m0);
  unsigned int  xMask = (xSizeMip0 - 1U) >> (unsigned char) m0;
  unsigned int  yMask = (ySizeMip0 - 1U) >> (unsigned char) m0;
  if (BRANCH_UNLIKELY(!(xMask | yMask)))
    return FloatVector4(textureData[m0]);
  float   xf = float(std::floor(x));
  float   yf = float(std::floor(y));
  int     x0 = int(xf);
  int     y0 = int(yf);
  xf = x - xf;
  yf = y - yf;
  FloatVector4  c0(getPixelB(textureData[m0], x0, y0, xf, yf, xMask, yMask));
  if (BRANCH_LIKELY(mf >= (1.0f / 512.0f) && mf < (511.0f / 512.0f)))
  {
    xf = (xf + float(x0 & 1)) * 0.5f;
    yf = (yf + float(y0 & 1)) * 0.5f;
    x0 = int((unsigned int) x0 >> 1);
    y0 = int((unsigned int) y0 >> 1);
    FloatVector4  c1(getPixelB(textureData[m0 + 1], x0, y0, xf, yf,
                               xMask >> 1, yMask >> 1));
    c0 = (c0 * FloatVector4(1.0f - mf)) + (c1 * FloatVector4(mf));
  }
  return c0;
}

FloatVector4 DDSTexture::getPixelBM(float x, float y, int mipLevel) const
{
  mipLevel = (mipLevel > 0 ? mipLevel : 0);
  int     x0, y0;
  float   xf = float(std::floor(x));
  float   yf = float(std::floor(y));
  x = x - xf;
  y = y - yf;
  x = (!(int(xf) & 1) ? x : (1.0f - x));
  y = (!(int(yf) & 1) ? y : (1.0f - y));
  unsigned int  xMask, yMask;
  (void) convertTexCoord(x0, y0, xf, yf, xMask, yMask, x, y, mipLevel, true);
  return getPixelB(textureData[mipLevel], x0, y0, xf, yf, xMask, yMask);
}

FloatVector4 DDSTexture::getPixelTM(float x, float y, float mipLevel) const
{
  mipLevel = (mipLevel > 0.0f ? mipLevel : 0.0f);
  int     m0 = int(mipLevel);
  float   mf = mipLevel - float(m0);
  int     x0, y0;
  float   xf = float(std::floor(x));
  float   yf = float(std::floor(y));
  x = x - xf;
  y = y - yf;
  x = (!(int(xf) & 1) ? x : (1.0f - x));
  y = (!(int(yf) & 1) ? y : (1.0f - y));
  unsigned int  xMask, yMask;
  if (BRANCH_UNLIKELY(!convertTexCoord(x0, y0, xf, yf, xMask, yMask, x, y, m0,
                                       true)))
  {
    return FloatVector4(textureData[m0]);
  }
  FloatVector4  c0(getPixelB(textureData[m0], x0, y0, xf, yf, xMask, yMask));
  if (BRANCH_LIKELY(mf >= (1.0f / 512.0f) && mf < (511.0f / 512.0f)))
  {
    (void) convertTexCoord(x0, y0, xf, yf, xMask, yMask, x, y, m0 + 1, true);
    FloatVector4  c1(getPixelB(textureData[m0 + 1], x0, y0, xf, yf,
                               xMask, yMask));
    c0 = (c0 * FloatVector4(1.0f - mf)) + (c1 * FloatVector4(mf));
  }
  return c0;
}

FloatVector4 DDSTexture::getPixelBC(float x, float y, int mipLevel) const
{
  mipLevel = (mipLevel > 0 ? mipLevel : 0);
  x = (x > 0.0f ? (x < 1.0f ? x : 1.0f) : 0.0f);
  y = (y > 0.0f ? (y < 1.0f ? y : 1.0f) : 0.0f);
  int     x0, y0;
  float   xf, yf;
  unsigned int  xMask, yMask;
  (void) convertTexCoord(x0, y0, xf, yf, xMask, yMask, x, y, mipLevel, true);
  return getPixelB(textureData[mipLevel], x0, y0, xf, yf, xMask, yMask);
}

FloatVector4 DDSTexture::getPixelTC(float x, float y, float mipLevel) const
{
  mipLevel = (mipLevel > 0.0f ? mipLevel : 0.0f);
  x = (x > 0.0f ? (x < 1.0f ? x : 1.0f) : 0.0f);
  y = (y > 0.0f ? (y < 1.0f ? y : 1.0f) : 0.0f);
  int     m0 = int(mipLevel);
  float   mf = mipLevel - float(m0);
  int     x0, y0;
  float   xf, yf;
  unsigned int  xMask, yMask;
  if (BRANCH_UNLIKELY(!convertTexCoord(x0, y0, xf, yf, xMask, yMask, x, y, m0,
                                       true)))
  {
    return FloatVector4(textureData[m0]);
  }
  FloatVector4  c0(getPixelB(textureData[m0], x0, y0, xf, yf, xMask, yMask));
  if (BRANCH_LIKELY(mf >= (1.0f / 512.0f) && mf < (511.0f / 512.0f)))
  {
    (void) convertTexCoord(x0, y0, xf, yf, xMask, yMask, x, y, m0 + 1, true);
    FloatVector4  c1(getPixelB(textureData[m0 + 1], x0, y0, xf, yf,
                               xMask, yMask));
    c0 = (c0 * FloatVector4(1.0f - mf)) + (c1 * FloatVector4(mf));
  }
  return c0;
}

FloatVector4 DDSTexture::cubeMap(float x, float y, float z,
                                 float mipLevel) const
{
  float   xm = float(std::fabs(x));
  float   ym = float(std::fabs(y));
  float   zm = float(std::fabs(z));
  size_t  n = textureDataSize;
  if (xm >= ym && xm >= zm)             // right (0), left (1)
  {
    float   tmp = 1.0f / xm;
    if (x < 0.0f)
      z = -z;
    else
      n = 0;
    x = z * tmp;
    y = y * tmp;
  }
  else if (ym >= xm && ym >= zm)        // front (2), back (3)
  {
    float   tmp = 1.0f / ym;
    n = n << 1;
    if (y < 0.0f)
    {
      z = -z;
      n += textureDataSize;
    }
    x = x * tmp;
    y = z * tmp;
  }
  else                                  // bottom (4), top (5)
  {
    float   tmp = 1.0f / zm;
    n = n << 2;
    if (z >= 0.0f)
    {
      x = -x;
      n += textureDataSize;
    }
    x = x * tmp;
    y = y * tmp;
  }
  n = (BRANCH_LIKELY(isCubeMap) ? n : 0U);
  mipLevel = (mipLevel > 0.0f ? mipLevel : 0.0f);
  int     m0 = int(mipLevel);
  float   mf = mipLevel - float(m0);
  x = (x + 1.0f) * 0.5f;
  y = (1.0f - y) * 0.5f;
  int     x0, y0;
  float   xf, yf;
  unsigned int  xMask, yMask;
  const std::uint32_t *p = textureData[m0] + n;
  if (BRANCH_UNLIKELY(!convertTexCoord(x0, y0, xf, yf, xMask, yMask, x, y, m0,
                                       true)))
  {
    return FloatVector4(p);
  }
  FloatVector4  c0(getPixelB(p, x0, y0, xf, yf, xMask, yMask));
  if (BRANCH_LIKELY(mf >= (1.0f / 512.0f) && mf < (511.0f / 512.0f)))
  {
    (void) convertTexCoord(x0, y0, xf, yf, xMask, yMask, x, y, m0 + 1, true);
    p = textureData[m0 + 1] + n;
    FloatVector4  c1(getPixelB(p, x0, y0, xf, yf, xMask, yMask));
    c0 = (c0 * FloatVector4(1.0f - mf)) + (c1 * FloatVector4(mf));
  }
  return c0;
}

//  0.003  0.000 -0.009  0.000  0.033  0.054  0.033  0.000 -0.009  0.000  0.003
//  0.000  0.000  0.000  0.000  0.000  0.000  0.000  0.000  0.000  0.000  0.000
// -0.009  0.000  0.030  0.000 -0.108 -0.174 -0.108  0.000  0.030  0.000 -0.009
//  0.000  0.000  0.000  0.000  0.000  0.000  0.000  0.000  0.000  0.000  0.000
//  0.033  0.000 -0.108  0.000  0.385  0.620  0.385  0.000 -0.108  0.000  0.033
//  0.054  0.000 -0.174  0.000  0.620  1.000  0.620  0.000 -0.174  0.000  0.054
//  0.033  0.000 -0.108  0.000  0.385  0.620  0.385  0.000 -0.108  0.000  0.033
//  0.000  0.000  0.000  0.000  0.000  0.000  0.000  0.000  0.000  0.000  0.000
// -0.009  0.000  0.030  0.000 -0.108 -0.174 -0.108  0.000  0.030  0.000 -0.009
//  0.000  0.000  0.000  0.000  0.000  0.000  0.000  0.000  0.000  0.000  0.000
//  0.003  0.000 -0.009  0.000  0.033  0.054  0.033  0.000 -0.009  0.000  0.003

struct Downsample2xTable
{
  static const float  filterTable[12];
  static inline void getPixelFast(FloatVector4& c, const std::uint32_t *buf,
                                  int w, int x, int y)
  {
    FloatVector4  tmp(buf + (y * w + x));
    tmp *= tmp;
    c += tmp;
  }
  static inline void getPixel(FloatVector4& c, const std::uint32_t *buf,
                              int w, int h, int x, int y)
  {
    x = (x > 0 ? (x < (w - 1) ? x : (w - 1)) : 0);
    y = (y > 0 ? (y < (h - 1) ? y : (h - 1)) : 0);
    FloatVector4  tmp(buf + (size_t(y) * size_t(w) + size_t(x)));
    tmp *= tmp;
    c += tmp;
  }
};

const float Downsample2xTable::filterTable[12] =
{
  // X + 0, 1, 3, 5
   0.62039170f,  0.38488586f, -0.10814215f,  0.03345214f,       // Y + 1
  -0.17431270f, -0.10814215f,  0.03038492f, -0.00939912f,       // Y + 3
   0.05392100f,  0.03345214f, -0.00939912f,  0.00290747f        // Y + 5
};

std::uint32_t downsample2xFilter(const std::uint32_t *buf,
                                 int imageWidth, int imageHeight, int x, int y)
{
  static const Downsample2xTable  t;
  const float *p = t.filterTable;
  FloatVector4  c(0.0f);
  if (BRANCH_LIKELY(x >= 5 && x < (imageWidth - 5) &&
                    y >= 5 && y < (imageHeight - 5)))
  {
    buf = buf + (size_t(y) * size_t(imageWidth) + size_t(x));
    t.getPixelFast(c, buf, imageWidth, 0, 0);
    for (int i = 0; i < 3; i++, p = p + 4)
    {
      int     yOffs = (i << 1) + 1;
      for (int j = 0; j < 4; j++)
      {
        FloatVector4  tmp(0.0f);
        int     xOffs = (j << 1) - int(bool(j));
        t.getPixelFast(tmp, buf, imageWidth, xOffs, yOffs);
        t.getPixelFast(tmp, buf, imageWidth, yOffs, -xOffs);
        t.getPixelFast(tmp, buf, imageWidth, -xOffs, -yOffs);
        t.getPixelFast(tmp, buf, imageWidth, -yOffs, xOffs);
        tmp *= p[j];
        c += tmp;
      }
    }
  }
  else
  {
    t.getPixel(c, buf, imageWidth, imageHeight, x, y);
    for (int i = 0; i < 3; i++, p = p + 4)
    {
      int     yOffs = (i << 1) + 1;
      for (int j = 0; j < 4; j++)
      {
        FloatVector4  tmp(0.0f);
        int     xOffs = (j << 1) - int(bool(j));
        t.getPixel(tmp, buf, imageWidth, imageHeight, x + xOffs, y + yOffs);
        t.getPixel(tmp, buf, imageWidth, imageHeight, x + yOffs, y - xOffs);
        t.getPixel(tmp, buf, imageWidth, imageHeight, x - xOffs, y - yOffs);
        t.getPixel(tmp, buf, imageWidth, imageHeight, x - yOffs, y + xOffs);
        tmp *= p[j];
        c += tmp;
      }
    }
  }
  c *= 0.25f;
  return std::uint32_t(c.squareRootFast());
}

static void downsample2xThread(std::uint32_t *outBuf,
                               const std::uint32_t *inBuf,
                               int w, int h, int y0, int y1, int pitch)
{
  std::uint32_t *p = outBuf + ((size_t(y0) >> 1) * size_t(pitch));
  for (int y = y0; y < y1; y = y + 2, p = p + (pitch - (w >> 1)))
  {
    for (int x = 0; x < w; x = x + 2, p++)
      *p = downsample2xFilter(inBuf, w, h, x, y);
  }
}

void downsample2xFilter(std::uint32_t *outBuf, const std::uint32_t *inBuf,
                        int imageWidth, int imageHeight, int pitch)
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
                                   y0, y1, pitch);
    }
    catch (...)
    {
      threads[i] = (std::thread *) 0;
      downsample2xThread(outBuf, inBuf, imageWidth, imageHeight, y0, y1, pitch);
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

