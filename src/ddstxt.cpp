
#include "common.hpp"
#include "ddstxt.hpp"

#include <new>

extern "C" bool detexDecompressBlockBPTC_FLOAT(
    const std::uint8_t *bitstring, std::uint32_t mode_mask,
    std::uint32_t flags, std::uint8_t *pixel_buffer);           // BC6U
extern "C" bool detexDecompressBlockBPTC_SIGNED_FLOAT(
    const std::uint8_t *bitstring, std::uint32_t mode_mask,
    std::uint32_t flags, std::uint8_t *pixel_buffer);           // BC6S
extern "C" bool detexDecompressBlockBPTC(
    const std::uint8_t *bitstring, std::uint32_t mode_mask,
    std::uint32_t flags, std::uint8_t *pixel_buffer);           // BC7

const DDSTexture::DXGIFormatInfo DDSTexture::dxgiFormatInfoTable[31] =
{
  {                             //  0: DXGI_FORMAT_UNKNOWN = 0x00
    (size_t (*)(std::uint32_t *, const unsigned char *, unsigned int)) 0,
    "UNKNOWN", false, false, 0, 0
  },
  {                             //  1: DXGI_FORMAT_R16G16B16A16_FLOAT = 0x0A
    &decodeLine_RGBA64F, "R16G16B16A16_FLOAT", false, true, 4, 8
  },
  {                             //  2: DXGI_FORMAT_R8G8B8A8_UNORM = 0x1C
    &decodeLine_RGBA, "R8G8B8A8_UNORM", false, false, 4, 4
  },
  {                             //  3: DXGI_FORMAT_R8G8B8A8_UNORM_SRGB = 0x1D
    &decodeLine_RGBA, "R8G8B8A8_UNORM_SRGB", false, true, 4, 4
  },
  {                             //  4: DXGI_FORMAT_R8G8_UNORM = 0x31
    &decodeLine_R8G8, "R8G8_UNORM", false, false, 2, 2
  },
  {                             //  5: DXGI_FORMAT_R8_UNORM = 0x3D
    &decodeLine_R8, "R8_UNORM", false, false, 1, 1
  },
  {                             //  6: DXGI_FORMAT_R8_UINT = 0x3E
    &decodeLine_R8, "R8_UINT", false, false, 1, 1
  },
  {                             //  7: DXGI_FORMAT_BC1_UNORM = 0x47
    &decodeBlock_BC1, "BC1_UNORM", true, false, 4, 8
  },
  {                             //  8: DXGI_FORMAT_BC1_UNORM_SRGB = 0x48
    &decodeBlock_BC1, "BC1_UNORM_SRGB", true, true, 4, 8
  },
  {                             //  9: DXGI_FORMAT_BC2_UNORM = 0x4A
    &decodeBlock_BC2, "BC2_UNORM", true, false, 4, 16
  },
  {                             // 10: DXGI_FORMAT_BC2_UNORM_SRGB = 0x4B
    &decodeBlock_BC2, "BC2_UNORM_SRGB", true, true, 4, 16
  },
  {                             // 11: DXGI_FORMAT_BC3_UNORM = 0x4D
    &decodeBlock_BC3, "BC3_UNORM", true, false, 4, 16
  },
  {                             // 12: DXGI_FORMAT_BC3_UNORM_SRGB = 0x4E
    &decodeBlock_BC3, "BC3_UNORM_SRGB", true, true, 4, 16
  },
  {                             // 13: DXGI_FORMAT_BC4_UNORM = 0x50
    &decodeBlock_BC4, "BC4_UNORM", true, false, 1, 8
  },
  {                             // 14: DXGI_FORMAT_BC4_SNORM = 0x51
    &decodeBlock_BC4S, "BC4_SNORM", true, false, 1, 8
  },
  {                             // 15: DXGI_FORMAT_BC5_UNORM = 0x53
    &decodeBlock_BC5, "BC5_UNORM", true, false, 2, 16
  },
  {                             // 16: DXGI_FORMAT_BC5_SNORM = 0x54
    &decodeBlock_BC5S, "BC5_SNORM", true, false, 2, 16
  },
  {                             // 17: DXGI_FORMAT_B8G8R8A8_UNORM = 0x57
    &decodeLine_BGRA, "B8G8R8A8_UNORM", false, false, 4, 4
  },
  {                             // 18: DXGI_FORMAT_B8G8R8X8_UNORM = 0x58
    &decodeLine_BGR32, "B8G8R8X8_UNORM", false, false, 3, 4
  },
  {                             // 19: DXGI_FORMAT_B8G8R8A8_UNORM_SRGB = 0x5B
    &decodeLine_BGRA, "B8G8R8A8_UNORM_SRGB", false, true, 4, 4
  },
  {                             // 20: DXGI_FORMAT_B8G8R8X8_UNORM_SRGB = 0x5D
    &decodeLine_BGR32, "B8G8R8X8_UNORM_SRGB", false, true, 3, 4
  },
  {                             // 21: DXGI_FORMAT_BC6H_UF16 = 0x5F
    &decodeBlock_BC6U, "BC6H_UF16", true, true, 4, 16
  },
  {                             // 22: DXGI_FORMAT_BC6H_SF16 = 0x60
    &decodeBlock_BC6S, "BC6H_SF16", true, false, 4, 16
  },
  {                             // 23: DXGI_FORMAT_BC7_UNORM = 0x62
    &decodeBlock_BC7, "BC7_UNORM", true, false, 4, 16
  },
  {                             // 24: DXGI_FORMAT_BC7_UNORM_SRGB = 0x63
    &decodeBlock_BC7, "BC7_UNORM_SRGB", true, true, 4, 16
  },
  // non-standard formats
  {                             // 25: FORMAT_R8G8B8X8_UNORM = 0x7A
    &decodeLine_RGB32, "R8G8B8X8_UNORM", false, false, 3, 4
  },
  {                             // 26: FORMAT_R8G8B8X8_UNORM_SRGB = 0x7B
    &decodeLine_RGB32, "R8G8B8X8_UNORM_SRGB", false, true, 3, 4
  },
  {                             // 27: FORMAT_B8G8R8_UNORM = 0x7C
    &decodeLine_BGR, "B8G8R8_UNORM", false, false, 3, 3
  },
  {                             // 28: FORMAT_B8G8R8_UNORM_SRGB = 0x7D
    &decodeLine_BGR, "B8G8R8_UNORM_SRGB", false, true, 3, 3
  },
  {                             // 29: FORMAT_R8G8B8_UNORM = 0x7E
    &decodeLine_RGB, "R8G8B8_UNORM", false, false, 3, 3
  },
  {                             // 30: FORMAT_R8G8B8_UNORM_SRGB = 0x7F
    &decodeLine_RGB, "R8G8B8_UNORM_SRGB", false, true, 3, 3
  }
};

const unsigned char DDSTexture::dxgiFormatMap[128] =
{
   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  1,  0,   0,  0,  0,  0,    // 0x00
   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   2,  3,  0,  0,    // 0x10
   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,    // 0x20
   0,  4,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  5,  6,  0,    // 0x30
   0,  0,  0,  0,   0,  0,  0,  7,   8,  0,  9, 10,   0, 11, 12,  0,    // 0x40
  13, 14,  0, 15,  16,  0,  0, 17,  18,  0,  0, 19,   0, 20,  0, 21,    // 0x50
  22,  0, 23, 24,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,    // 0x60
   0,  0,  0,  0,   0,  0,  0,  0,   0,  0, 25, 26,  27, 28, 29, 30     // 0x70
};

static inline std::uint64_t decodeBC3Alpha(std::uint64_t& a,
                                           const unsigned char *src,
                                           bool isSigned = false)
{
  std::uint64_t ba = FileBuffer::readUInt64Fast(src);
  if (isSigned)
    ba = ba ^ 0x8080U;
  unsigned int  a0 = (unsigned int) (ba & 0xFFU);
  unsigned int  a1 = (unsigned int) ((ba >> 8) & 0xFFU);
  FloatVector4  tmp((std::uint32_t) ba);
  FloatVector4  a0_f(tmp[0], tmp[0], tmp[0], tmp[0]);
  FloatVector4  a1_f(tmp[1], tmp[1], tmp[1], tmp[1]);
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
  return (ba >> 16);
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
    c[2] = std::uint32_t((c0_f + c0_f + c1_f) * (1.0f / 3.0f));
    c[3] = std::uint32_t((c0_f + c1_f + c1_f) * (1.0f / 3.0f));
  }
  else
  {
    c[2] = std::uint32_t((c0_f + c1_f) * 0.5f);
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

size_t DDSTexture::decodeBlock_BC6U(
    std::uint32_t *dst, const unsigned char *src, unsigned int w)
{
  std::uint8_t  tmp[128];
  (void) detexDecompressBlockBPTC_FLOAT(
             reinterpret_cast< const std::uint8_t * >(src),
             0xFFFFFFFFU, 0U, tmp);
  for (unsigned int i = 0; i < 16; i++)
  {
    std::uint64_t b = FileBuffer::readUInt64Fast(&(tmp[i << 3]));
    FloatVector4  c(FloatVector4::convertFloat16(b));
    c.maxValues(FloatVector4(0.0f));
    float   a = c[3] * 255.0f;
    c = c / (c + 4.0f);
    c.srgbCompress();
    c[3] = a;
    dst[(i >> 2) * w + (i & 3)] = std::uint32_t(c);
  }
  return 16;
}

size_t DDSTexture::decodeBlock_BC6S(
    std::uint32_t *dst, const unsigned char *src, unsigned int w)
{
  std::uint8_t  tmp[128];
  (void) detexDecompressBlockBPTC_SIGNED_FLOAT(
             reinterpret_cast< const std::uint8_t * >(src),
             0xFFFFFFFFU, 0U, tmp);
  for (unsigned int i = 0; i < 16; i++)
  {
    std::uint64_t b = FileBuffer::readUInt64Fast(&(tmp[i << 3]));
    FloatVector4  c(FloatVector4::convertFloat16(b));
    c.maxValues(FloatVector4(-1.0f));
    c.minValues(FloatVector4(1.0f));
    dst[(i >> 2) * w + (i & 3)] = std::uint32_t(c * 127.5f + 127.5f);
  }
  return 16;
}

size_t DDSTexture::decodeBlock_BC7(
    std::uint32_t *dst, const unsigned char *src, unsigned int w)
{
  std::uint8_t  tmp[64];
  (void) detexDecompressBlockBPTC(reinterpret_cast< const std::uint8_t * >(src),
                                  0xFFFFFFFFU, 0U, tmp);
  for (unsigned int i = 0; i < 16; i++)
    dst[(i >> 2) * w + (i & 3)] = FileBuffer::readUInt32Fast(&(tmp[i << 2]));
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

size_t DDSTexture::decodeLine_R8(
    std::uint32_t *dst, const unsigned char *src, unsigned int w)
{
  unsigned int  x = 0;
  for ( ; (x + 4U) <= w; x = x + 4U, dst = dst + 4, src = src + 4)
  {
    dst[0] = 0xFF000000U | (std::uint32_t(src[0]) * 0x00010101U);
    dst[1] = 0xFF000000U | (std::uint32_t(src[1]) * 0x00010101U);
    dst[2] = 0xFF000000U | (std::uint32_t(src[2]) * 0x00010101U);
    dst[3] = 0xFF000000U | (std::uint32_t(src[3]) * 0x00010101U);
  }
  for ( ; x < w; x++, dst++, src++)
    *dst = 0xFF000000U | (std::uint32_t(*src) * 0x00010101U);
  return size_t(w);
}

size_t DDSTexture::decodeLine_RGBA64F(
    std::uint32_t *dst, const unsigned char *src, unsigned int w)
{
  for (unsigned int x = 0; x < w; x++, dst++, src = src + 8)
  {
    std::uint64_t b = FileBuffer::readUInt64Fast(src);
    FloatVector4  c(FloatVector4::convertFloat16(b));
    c.maxValues(FloatVector4(0.0f));
    float   a = c[3] * 255.0f;
    c *= (float(int(*dst)) * (1.0f / 16777216.0f));
    c.srgbCompress();
    c[3] = a;
    *dst = std::uint32_t(c);
  }
  return (size_t(w) << 3);
}

void DDSTexture::loadTextureData(
    const unsigned char *srcPtr, int n, bool isCompressed,
    size_t (*decodeFunction)(std::uint32_t *,
                             const unsigned char *, unsigned int))
{
  size_t  dataOffs = size_t(n) * size_t(textureDataSize);
  for (int i = 0; i < 19; i++)
  {
    std::uint32_t *p = textureData[i] + dataOffs;
    unsigned int  w = (xMaskMip0 >> (unsigned char) i) + 1U;
    unsigned int  h = (yMaskMip0 >> (unsigned char) i) + 1U;
    if (i <= int(maxMipLevel))
    {
      if (!isCompressed)
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
          textureData[i - 1] + (size_t(textureDataSize) * size_t(n));
      unsigned int  xMask = xMaskMip0 >> (unsigned char) (i - 1);
      unsigned int  yMask = yMaskMip0 >> (unsigned char) (i - 1);
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
          p[y * w + x] = std::uint32_t(c * 0.25f);
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
          p[y * w + x] = std::uint32_t((c0 * 0.23031584f) + (c1 * 0.12479824f)
                                       + (c2 * 0.06762280f));
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
    errorMessage("unsupported texture file format");
  if (buf.readUInt32() != 0x7C)         // size of DDS_HEADER
    errorMessage("unsupported texture file format");
  unsigned int  flags = buf.readUInt32();
  if ((flags & 0x1006) != 0x1006)       // height, width, pixel format required
    errorMessage("unsupported texture file format");
  maxMipLevel = 0U;
  isSRGB = false;
  channelCnt = 0;
  maxTextureNum = 0;
  dxgiFormat = 0;
  yMaskMip0 = buf.readUInt32() - 1U;
  xMaskMip0 = buf.readUInt32() - 1U;
  // width and height must be power of two and in the range 1 to 32768
  if (((xMaskMip0 | yMaskMip0) & ~0x7FFFU) ||
      ((xMaskMip0 & (xMaskMip0 + 1U)) | (yMaskMip0 & (yMaskMip0 + 1U))) != 0U)
  {
    errorMessage("invalid or unsupported texture dimensions");
  }
  buf.setPosition(buf.getPosition() + 8);       // dwPitchOrLinearSize, dwDepth
  if (flags & 0x00020000)               // DDSD_MIPMAPCOUNT
  {
    int     mipLevelCnt = buf.readInt32();
    if (mipLevelCnt < 1 || mipLevelCnt > 16)
      errorMessage("invalid mipmap count");
    maxMipLevel = std::uint32_t(mipLevelCnt - 1);
  }
  buf.setPosition(0x4C);                // ddspf
  if (buf.readUInt32() != 0x20)         // size of DDS_PIXELFORMAT
    errorMessage("unsupported texture file format");
  unsigned int  dataOffs = 128;
  unsigned int  formatFlags = buf.readUInt32();
  unsigned int  fourCC = buf.readUInt32();
  if (formatFlags & 0x04)               // DDPF_FOURCC
  {
    if (FileBuffer::checkType(fourCC, "DX10"))
    {
      dataOffs = 148;
      buf.setPosition(0x80);
      unsigned int  tmp = buf.readUInt32();
      if (tmp && (tmp > 0x79U || !dxgiFormatMap[tmp]))
        throw FO76UtilsError("unsupported DXGI_FORMAT: 0x%08X", tmp);
      dxgiFormat = (unsigned char) tmp;
    }
    else
    {
      switch (fourCC)
      {
        case 0x53344342:                // "BC4S"
          dxgiFormat = 0x51;
          break;
        case 0x55344342:                // "BC4U"
          dxgiFormat = 0x50;
          break;
        case 0x53354342:                // "BC5S"
          dxgiFormat = 0x54;
          break;
        case 0x55354342:                // "BC5U"
          dxgiFormat = 0x53;
          break;
        case 0x31545844:                // "DXT1"
          dxgiFormat = 0x48;
          break;
        case 0x32545844:                // "DXT2"
        case 0x33545844:                // "DXT3"
          dxgiFormat = 0x4B;
          break;
        case 0x34545844:                // "DXT4"
        case 0x35545844:                // "DXT5"
          dxgiFormat = 0x4E;
          break;
        default:
          throw FO76UtilsError("unsupported DDS fourCC: 0x%08X",
                               FileBuffer::swapUInt32(fourCC));
      }
    }
  }
  if (!dxgiFormat)
  {
    buf.setPosition(0x58);
    if (!(formatFlags & 0x40))          // DDPF_RGB
      errorMessage("unsupported texture file format");
    unsigned int  bitsPerPixel = buf.readUInt32();
    if ((bitsPerPixel - 8U) & ~24U)     // must be 8, 16, 24 or 32
      errorMessage("unsupported texture file format");
    unsigned long long  rgMask = buf.readUInt64();
    unsigned long long  baMask = buf.readUInt64();
    if (bitsPerPixel < 32U || !(formatFlags & 0x03))
      baMask = baMask & ~0xFF00000000000000ULL;
    if (rgMask == 0x0000FF00000000FFULL && baMask == 0x0000000000FF0000ULL)
      dxgiFormat = (bitsPerPixel < 32U ? 0x7F : 0x7B);
    else if (rgMask == 0x0000FF0000FF0000ULL && baMask == 0x00000000000000FFULL)
      dxgiFormat = (bitsPerPixel < 32U ? 0x7D : 0x5D);
    else if (rgMask == 0x0000FF00000000FFULL && baMask == 0xFF00000000FF0000ULL)
      dxgiFormat = 0x1D;
    else if (rgMask == 0x0000FF0000FF0000ULL && baMask == 0xFF000000000000FFULL)
      dxgiFormat = 0x5B;
    else if (rgMask == 0x0000FF00000000FFULL && !baMask)
      dxgiFormat = 0x31;
    else if (rgMask == 0x00000000000000FFULL && !baMask)
      dxgiFormat = 0x3D;
    else
      errorMessage("unsupported texture file format");
  }
  const DXGIFormatInfo& dxgiFmtInfo =
      dxgiFormatInfoTable[dxgiFormatMap[dxgiFormat]];
  size_t  (*decodeFunction)(std::uint32_t *, const unsigned char *,
                            unsigned int) = dxgiFmtInfo.decodeFunction;
  bool    isCompressed = dxgiFmtInfo.isCompressed;
  isSRGB = dxgiFmtInfo.isSRGB;
  channelCnt = dxgiFmtInfo.channelCnt;
  size_t  blockSize = dxgiFmtInfo.blockSize;
  size_t  sizeRequired = 0;
  if (!isCompressed)
  {
    for (unsigned int i = 0; i <= maxMipLevel; i++)
    {
      unsigned int  w = (xMaskMip0 >> i) + 1U;
      unsigned int  h = (yMaskMip0 >> i) + 1U;
      sizeRequired = sizeRequired + (size_t(w) * h * blockSize);
    }
  }
  else
  {
    for (int i = 0; i <= int(maxMipLevel); i++)
    {
      unsigned int  w = (xMaskMip0 >> (i + 2)) + 1U;
      unsigned int  h = (yMaskMip0 >> (i + 2)) + 1U;
      sizeRequired = sizeRequired + (size_t(w) * h * blockSize);
    }
  }
  if (buf.size() > (dataOffs + sizeRequired))
  {
    size_t  n = (buf.size() - dataOffs) / sizeRequired;
    if (n > 256)
      n = 256;
    maxTextureNum = (unsigned char) (n - 1);
  }
  else if (buf.size() < (dataOffs + sizeRequired))
  {
    errorMessage("DDS file is shorter than expected");
  }
  const unsigned char *srcPtr = buf.data() + dataOffs;
  for ( ; mipOffset > 0 && maxMipLevel; mipOffset--, maxMipLevel--)
  {
    unsigned int  w = xMaskMip0 + 1U;
    unsigned int  h = yMaskMip0 + 1U;
    if (!isCompressed)
      srcPtr = srcPtr + (size_t(w) * h * blockSize);
    else
      srcPtr = srcPtr + (size_t((w + 3) >> 2) * ((h + 3) >> 2) * blockSize);
    xMaskMip0 = xMaskMip0 >> 1;
    yMaskMip0 = yMaskMip0 >> 1;
  }
  size_t  dataOffsets[19];
  size_t  bufSize = 0;
  unsigned int  xMask = (xMaskMip0 << 1) | 1U;
  unsigned int  yMask = (yMaskMip0 << 1) | 1U;
  for (unsigned int i = 0; i < 19; i++)
  {
    if (!(xMask | yMask))
    {
      dataOffsets[i] = dataOffsets[i - 1];
      continue;
    }
    xMask = xMask >> 1;
    yMask = yMask >> 1;
    dataOffsets[i] = bufSize;
    bufSize = bufSize + (size_t(xMask + 1U) * (yMask + 1U));
  }
  textureDataSize = std::uint32_t(bufSize);
  size_t  totalDataSize =
      bufSize * (size_t(maxTextureNum) + 1) * sizeof(std::uint32_t);
  std::uint32_t *textureDataBuf =
      reinterpret_cast< std::uint32_t * >(std::malloc(totalDataSize));
  if (!textureDataBuf)
    throw std::bad_alloc();
  for (unsigned int i = 0; i < 19; i++)
    textureData[i] = textureDataBuf + dataOffsets[i];
  if (BRANCH_UNLIKELY(dxgiFormat == 0x0A))
  {                                     // DXGI_FORMAT_R16G16B16A16_FLOAT
    std::uint32_t scale = 16777216U;
    if (maxTextureNum == 5)
    {
      // normalize FP16 format cube maps
      size_t  srcDataSize = sizeRequired * 6;
      FloatVector4  avgLevel(0.0f);
      for (size_t i = 0; i < srcDataSize; i = i + 8)
      {
        FloatVector4  tmp(FloatVector4::convertFloat16(
                              FileBuffer::readUInt64Fast(srcPtr + i), true));
        avgLevel += (tmp * tmp);
      }
      avgLevel /= float(int(srcDataSize >> 3));
      float   tmp = float(std::sqrt(avgLevel[0] + avgLevel[1] + avgLevel[2]));
      tmp = 16777216.0f / std::min(std::max(tmp * 1.125f, 1.0f), 65536.0f);
      scale = std::uint32_t(roundFloat(tmp));
    }
    memsetUInt32(textureDataBuf, scale, totalDataSize / sizeof(std::uint32_t));
  }
  for (size_t i = 0; i <= maxTextureNum; i++)
  {
    loadTextureData(srcPtr + (i * sizeRequired), int(i),
                    isCompressed, decodeFunction);
  }
  if (mipOffset > 0 && dataOffsets[1])
  {
    mipOffset = (mipOffset < 18 ? mipOffset : 18);
    size_t  offs = dataOffsets[mipOffset];
    xMaskMip0 = xMaskMip0 >> (unsigned char) mipOffset;
    yMaskMip0 = yMaskMip0 >> (unsigned char) mipOffset;
    totalDataSize = totalDataSize - (offs * sizeof(std::uint32_t));
    std::memmove(textureData[0], textureData[mipOffset], totalDataSize);
    textureDataBuf = reinterpret_cast< std::uint32_t * >(
                         std::realloc(textureDataBuf, totalDataSize));
    if (BRANCH_UNLIKELY(!textureDataBuf))
      throw std::bad_alloc();
    for (int i = 0; i < 19; i++)
    {
      int     j = ((i + mipOffset) < 18 ? (i + mipOffset) : 18);
      textureData[i] = textureDataBuf + (dataOffsets[j] - offs);
    }
  }
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

DDSTexture::DDSTexture(std::uint32_t c, bool srgbColor)
  : xMaskMip0(0U),
    yMaskMip0(0U),
    maxMipLevel(0U),
    textureDataSize(0U),
    textureColor(c),
    isSRGB(srgbColor),
    channelCnt(4),
    maxTextureNum(0),
    dxgiFormat(0)
{
#if ENABLE_X86_64_AVX
  std::uintptr_t  tmp1 =
      std::uintptr_t(reinterpret_cast< unsigned char * >(&textureColor));
  const YMM_UInt64  tmp2 = { tmp1, tmp1, tmp1, tmp1 };
  // sizeof(textureData) == sizeof(std::uint32_t *) * 19
  __asm__ ("vmovdqu %t1, %0" : "=m" (textureData[0]) : "x" (tmp2));
  __asm__ ("vmovdqu %t1, %0" : "=m" (textureData[4]) : "x" (tmp2));
  __asm__ ("vmovdqu %t1, %0" : "=m" (textureData[8]) : "x" (tmp2));
  __asm__ ("vmovdqu %t1, %0" : "=m" (textureData[12]) : "x" (tmp2));
  __asm__ ("vmovdqu %x1, %0" : "=m" (textureData[16]) : "x" (tmp2));
  __asm__ ("vmovq %x1, %0" : "=m" (textureData[18]) : "x" (tmp2));
#else
  for (size_t i = 0; i < (sizeof(textureData) / sizeof(std::uint32_t *)); i++)
    textureData[i] = &textureColor;
#endif
}

DDSTexture::~DDSTexture()
{
  if (textureDataSize)
    std::free(textureData[0]);
}

FloatVector4 DDSTexture::getPixelB(float x, float y, int mipLevel) const
{
  return getPixelB_Inline(x, y, mipLevel);
}

FloatVector4 DDSTexture::getPixelT(float x, float y, float mipLevel) const
{
  return getPixelT_Inline(x, y, mipLevel);
}

FloatVector4 DDSTexture::getPixelT_2(float x, float y, float mipLevel,
                                     const DDSTexture& t) const
{
  mipLevel = std::max(mipLevel, 0.0f);
  int     m0 = int(mipLevel);
  const std::uint32_t * const *t1 = &(textureData[m0]);
  const std::uint32_t * const *t2 = &(t.textureData[m0]);
  unsigned int  xMask = xMaskMip0;
  unsigned int  yMask = yMaskMip0;
  unsigned int  xMask2 = t.xMaskMip0;
  unsigned int  yMask2 = t.yMaskMip0;
  if (BRANCH_UNLIKELY(!(xMask2 == xMask && yMask2 == yMask)))
  {
    if (xMask2 == (xMask >> 1) && yMask2 == (yMask >> 1) && m0 > 0)
    {
      t2--;
    }
    else if ((xMask2 >> 1) == xMask && (yMask2 >> 1) == yMask)
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
  if (BRANCH_UNLIKELY(!convertTexCoord(x0, y0, xf, yf, xMask, yMask, x, y, m0)))
    return FloatVector4(t1[0], t2[0]);
  FloatVector4  c0(getPixelB_2(t1[0], t2[0], x0, y0, xf, yf, xMask, yMask));
  float   mf = float(m0);
  if (BRANCH_LIKELY(mf != mipLevel))
  {
    mf = mipLevel - mf;
    xf = (xf + float(x0 & 1)) * 0.5f;
    yf = (yf + float(y0 & 1)) * 0.5f;
    x0 = int((unsigned int) x0 >> 1);
    y0 = int((unsigned int) y0 >> 1);
    FloatVector4  c1(getPixelB_2(t1[1], t2[1], x0, y0, xf, yf,
                                 xMask >> 1, yMask >> 1));
    c0 = (c0 * (1.0f - mf)) + (c1 * mf);
  }
  return c0;
}

FloatVector4 DDSTexture::getPixelT_N(float x, float y, float mipLevel) const
{
  mipLevel = std::max(mipLevel, 0.0f);
  int     m0 = int(mipLevel);
  unsigned int  xMask = xMaskMip0 >> (unsigned char) m0;
  unsigned int  yMask = yMaskMip0 >> (unsigned char) m0;
  if (BRANCH_UNLIKELY(!(xMask | yMask)))
    return FloatVector4(textureData[m0]);
  float   xf = float(std::floor(x));
  float   yf = float(std::floor(y));
  int     x0 = int(xf);
  int     y0 = int(yf);
  xf = x - xf;
  yf = y - yf;
  FloatVector4  c0(getPixelB(textureData[m0], x0, y0, xf, yf, xMask, yMask));
  float   mf = float(m0);
  if (BRANCH_LIKELY(mf != mipLevel))
  {
    mf = mipLevel - mf;
    xf = (xf + float(x0 & 1)) * 0.5f;
    yf = (yf + float(y0 & 1)) * 0.5f;
    x0 = int((unsigned int) x0 >> 1);
    y0 = int((unsigned int) y0 >> 1);
    FloatVector4  c1(getPixelB(textureData[m0 + 1], x0, y0, xf, yf,
                               xMask >> 1, yMask >> 1));
    c0 = (c0 * (1.0f - mf)) + (c1 * mf);
  }
  return c0;
}

FloatVector4 DDSTexture::getPixelBM(float x, float y, int mipLevel) const
{
  return getPixelBM_Inline(x, y, mipLevel);
}

FloatVector4 DDSTexture::getPixelTM(float x, float y, float mipLevel) const
{
  mipLevel = std::max(mipLevel, 0.0f);
  int     m0 = int(mipLevel);
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
  float   mf = float(m0);
  if (BRANCH_LIKELY(mf != mipLevel))
  {
    mf = mipLevel - mf;
    (void) convertTexCoord(x0, y0, xf, yf, xMask, yMask, x, y, m0 + 1, true);
    FloatVector4  c1(getPixelB(textureData[m0 + 1], x0, y0, xf, yf,
                               xMask, yMask));
    c0 = (c0 * (1.0f - mf)) + (c1 * mf);
  }
  return c0;
}

FloatVector4 DDSTexture::getPixelBC(float x, float y, int mipLevel) const
{
  return getPixelBC_Inline(x, y, mipLevel);
}

FloatVector4 DDSTexture::getPixelTC(float x, float y, float mipLevel) const
{
  mipLevel = std::max(mipLevel, 0.0f);
  x = std::min(std::max(x, 0.0f), 1.0f);
  y = std::min(std::max(y, 0.0f), 1.0f);
  int     m0 = int(mipLevel);
  int     x0, y0;
  float   xf, yf;
  unsigned int  xMask, yMask;
  if (BRANCH_UNLIKELY(!convertTexCoord(x0, y0, xf, yf, xMask, yMask, x, y, m0,
                                       true)))
  {
    return FloatVector4(textureData[m0]);
  }
  FloatVector4  c0(getPixelB(textureData[m0], x0, y0, xf, yf, xMask, yMask));
  float   mf = float(m0);
  if (BRANCH_LIKELY(mf != mipLevel))
  {
    mf = mipLevel - mf;
    (void) convertTexCoord(x0, y0, xf, yf, xMask, yMask, x, y, m0 + 1, true);
    FloatVector4  c1(getPixelB(textureData[m0 + 1], x0, y0, xf, yf,
                               xMask, yMask));
    c0 = (c0 * (1.0f - mf)) + (c1 * mf);
  }
  return c0;
}

FloatVector4 DDSTexture::cubeMap(float x, float y, float z,
                                 float mipLevel) const
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
      n = 1;
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
      n = 3;
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
      n = 5;
    }
    x = x * tmp;
    y = y * tmp;
  }
  n = std::min(n, size_t(maxTextureNum)) * textureDataSize;
  mipLevel = std::max(mipLevel, 0.0f);
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
    c0 = (c0 * (1.0f - mf)) + (c1 * mf);
  }
  return c0;
}

