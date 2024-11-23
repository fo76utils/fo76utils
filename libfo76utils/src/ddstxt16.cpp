
#include "common.hpp"
#include "ddstxt16.hpp"

#include <new>

const DDSTexture16::DXGIFormatInfo DDSTexture16::dxgiFormatInfoTable[36] =
{
  {                             //  0: DXGI_FORMAT_UNKNOWN = 0x00
    (size_t (*)(std::uint64_t *, const unsigned char *, unsigned int)) 0,
    "UNKNOWN", false, false, 0, 0
  },
  {                             //  1: DXGI_FORMAT_R16G16B16A16_FLOAT = 0x0A
    &decodeLine_RGBA64F, "R16G16B16A16_FLOAT", false, false, 4, 8
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
    &decodeBlock_BC6U, "BC6H_UF16", true, false, 4, 16
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
  },
  // end of non-standard formats
  {                             // 31: DXGI_FORMAT_R9G9B9E5_SHAREDEXP = 0x43
    &decodeLine_RGB9E5, "R9G9B9E5_SHAREDEXP", false, false, 3, 4
  },
  {                             // 32: DXGI_FORMAT_R10G10B10A2_UNORM = 0x18
    &decodeLine_R10G10B10A2, "R10G10B10A2_UNORM", false, false, 4, 4
  },
  {                             // 33: DXGI_FORMAT_R8G8_SNORM = 0x33
    &decodeLine_R8G8S, "R8G8_SNORM", false, false, 2, 2
  },
  {                             // 34: DXGI_FORMAT_R8G8B8A8_SNORM = 0x1F
    &decodeLine_RGBA32S, "R8G8B8A8_SNORM", false, false, 4, 4
  },
  {                             // 35: DXGI_FORMAT_R16G16B16A16_UNORM = 0x0B
    &decodeLine_RGBA64, "R16G16B16A16_UNORM", false, false, 4, 8
  }
};

const unsigned char DDSTexture16::dxgiFormatMap[128] =
{
   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  1, 35,   0,  0,  0,  0,    // 0x00
   0,  0,  0,  0,   0,  0,  0,  0,  32,  0,  0,  0,   2,  3,  0, 34,    // 0x10
   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,    // 0x20
   0,  4,  0, 33,   0,  0,  0,  0,   0,  0,  0,  0,   0,  5,  6,  0,    // 0x30
   0,  0,  0, 31,   0,  0,  0,  7,   8,  0,  9, 10,   0, 11, 12,  0,    // 0x40
  13, 14,  0, 15,  16,  0,  0, 17,  18,  0,  0, 19,   0, 20,  0, 21,    // 0x50
  22,  0, 23, 24,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,    // 0x60
   0,  0,  0,  0,   0,  0,  0,  0,   0,  0, 25, 26,  27, 28, 29, 30     // 0x70
};

static inline std::uint64_t decodeBC3Alpha(float *a,
                                           const unsigned char *src,
                                           bool isSigned = false)
{
  std::uint64_t ba = FileBuffer::readUInt64Fast(src);
  if (isSigned)
    ba = ba ^ 0x8080U;
  unsigned int  a0 = (unsigned int) (ba & 0xFFU);
  unsigned int  a1 = (unsigned int) ((ba >> 8) & 0xFFU);
  FloatVector4  tmp((std::uint32_t) ba);
  tmp *= (1.0f / 255.0f);
  FloatVector8  a0_f(tmp[0]);
  FloatVector8  a1_f(tmp[1]);
  a1_f -= a0_f;
  if (a0 > a1)
  {
    FloatVector8  m(0.0f, 1.0f, 1.0f / 7.0f, 2.0f / 7.0f,
                    3.0f / 7.0f, 4.0f / 7.0f, 5.0f / 7.0f, 6.0f / 7.0f);
    (a0_f + (a1_f * m)).convertToFloats(a);
  }
  else
  {
    a0_f[6] = 0.0f;
    a0_f[7] = 1.0f;
    FloatVector8  m(0.0f, 1.0f, 0.2f, 0.4f, 0.6f, 0.8f, 0.0f, 0.0f);
    (a0_f + (a1_f * m)).convertToFloats(a);
  }
  return (ba >> 16);
}

static inline std::uint32_t decodeBC1Colors(FloatVector4 *c,
                                            const unsigned char *src)
{
  std::uint32_t c0 = FileBuffer::readUInt16Fast(src);
  std::uint32_t c1 = FileBuffer::readUInt16Fast(src + 2);
  std::uint32_t b0 = ((c0 & 0x001FU) << 16) | ((c0 & 0x07E0U) << 3)
                     | ((c0 & 0xF800U) >> 11) | 0xFF000000U;
  std::uint32_t b1 = ((c1 & 0x001FU) << 16) | ((c1 & 0x07E0U) << 3)
                     | ((c1 & 0xF800U) >> 11) | 0xFF000000U;
  FloatVector4  c0_f(b0);
  FloatVector4  c1_f(b1);
  c0_f *= FloatVector4(1.0f / 31.0f, 1.0f / 63.0f, 1.0f / 31.0f, 1.0f / 255.0f);
  c1_f *= FloatVector4(1.0f / 31.0f, 1.0f / 63.0f, 1.0f / 31.0f, 1.0f / 255.0f);
  c[0] = c0_f;
  c[1] = c1_f;
  if (c0 > c1) [[likely]]
  {
    c[2] = (c0_f + c0_f + c1_f) * (1.0f / 3.0f);
    c[3] = (c0_f + c1_f + c1_f) * (1.0f / 3.0f);
  }
  else
  {
    c[2] = (c0_f + c1_f) * 0.5f;
    c[3] = c[2];
    c[3].clearV3();
  }
  return FileBuffer::readUInt32Fast(src + 4);
}

size_t DDSTexture16::decodeBlock_BC1(
    std::uint64_t *dst, const unsigned char *src, unsigned int w)
{
  FloatVector4  c[4];
  std::uint32_t bc = decodeBC1Colors(c, src);
  for (unsigned int i = 0; i < 16; i++)
  {
    dst[(i >> 2) * w + (i & 3)] = c[bc & 3].convertToFloat16();
    bc = bc >> 2;
  }
  return 8;
}

size_t DDSTexture16::decodeBlock_BC2(
    std::uint64_t *dst, const unsigned char *src, unsigned int w)
{
  FloatVector4  c[4];
  std::uint32_t bc = decodeBC1Colors(c, src + 8);
  for (unsigned int i = 0; i < 4; i++)
  {
    std::uint32_t ba = FileBuffer::readUInt16Fast(src + (i << 1));
    ba = (ba & 0x0F0FU) | ((ba & 0xF0F0U) << 12);
    FloatVector4  a(ba);
    a *= (1.0f / 15.0f);
    FloatVector4  tmp(c[bc & 3]);
    tmp[3] = a[0];
    dst[i * w] = tmp.convertToFloat16();
    bc = bc >> 2;
    tmp = c[bc & 3];
    tmp[3] = a[2];
    dst[i * w + 1] = tmp.convertToFloat16();
    bc = bc >> 2;
    tmp = c[bc & 3];
    tmp[3] = a[1];
    dst[i * w + 2] = tmp.convertToFloat16();
    bc = bc >> 2;
    tmp = c[bc & 3];
    tmp[3] = a[3];
    dst[i * w + 3] = tmp.convertToFloat16();
    bc = bc >> 2;
  }
  return 16;
}

size_t DDSTexture16::decodeBlock_BC3(
    std::uint64_t *dst, const unsigned char *src, unsigned int w)
{
  FloatVector4  c[4];
  float         a[8];
  std::uint64_t ba = decodeBC3Alpha(a, src);
  std::uint32_t bc = decodeBC1Colors(c, src + 8);
  for (unsigned int i = 0; i < 16; i++, ba = ba >> 3, bc = bc >> 2)
  {
    FloatVector4  tmp(c[bc & 3]);
    tmp[3] = a[ba & 7];
    dst[(i >> 2) * w + (i & 3)] = tmp.convertToFloat16();
  }
  return 16;
}

size_t DDSTexture16::decodeBlock_BC4(
    std::uint64_t *dst, const unsigned char *src, unsigned int w)
{
  float         a[8];
  std::uint64_t ba = decodeBC3Alpha(a, src);
  for (unsigned int i = 0; i < 16; i++, ba = ba >> 3)
  {
    FloatVector4  tmp(a[ba & 7]);
    tmp[3] = 1.0f;
    dst[(i >> 2) * w + (i & 3)] = tmp.convertToFloat16();
  }
  return 8;
}

size_t DDSTexture16::decodeBlock_BC4S(
    std::uint64_t *dst, const unsigned char *src, unsigned int w)
{
  float         a[8];
  std::uint64_t ba = decodeBC3Alpha(a, src, true);
  for (unsigned int i = 0; i < 16; i++, ba = ba >> 3)
  {
    float   tmp1 = a[ba & 7];
    FloatVector4  tmp2(tmp1 + tmp1 - 1.0f);
    tmp2[3] = 1.0f;
    dst[(i >> 2) * w + (i & 3)] = tmp2.convertToFloat16();
  }
  return 8;
}

size_t DDSTexture16::decodeBlock_BC5(
    std::uint64_t *dst, const unsigned char *src, unsigned int w)
{
  float         a1[8];
  float         a2[8];
  std::uint64_t ba1 = decodeBC3Alpha(a1, src);
  std::uint64_t ba2 = decodeBC3Alpha(a2, src + 8);
  for (unsigned int i = 0; i < 16; i++, ba1 = ba1 >> 3, ba2 = ba2 >> 3)
  {
    FloatVector4  tmp(a1[ba1 & 7], a2[ba2 & 7], 0.0f, 1.0f);
    dst[(i >> 2) * w + (i & 3)] = tmp.convertToFloat16();
  }
  return 16;
}

size_t DDSTexture16::decodeBlock_BC5S(
    std::uint64_t *dst, const unsigned char *src, unsigned int w)
{
  float         a1[8];
  float         a2[8];
  std::uint64_t ba1 = decodeBC3Alpha(a1, src, true);
  std::uint64_t ba2 = decodeBC3Alpha(a2, src + 8, true);
  for (unsigned int i = 0; i < 16; i++, ba1 = ba1 >> 3, ba2 = ba2 >> 3)
  {
    FloatVector4  tmp(a1[ba1 & 7], a2[ba2 & 7], 0.0f, 1.0f);
    tmp = tmp + tmp - 1.0f;
    dst[(i >> 2) * w + (i & 3)] = tmp.convertToFloat16();
  }
  return 16;
}

size_t DDSTexture16::decodeBlock_BC6U(
    std::uint64_t *dst, const unsigned char *src, unsigned int w)
{
  std::uint64_t tmp[16];
  (void) detexDecompressBlockBPTC_FLOAT(
             reinterpret_cast< const std::uint8_t * >(src), 0xFFFFFFFFU, 0U,
             reinterpret_cast< std::uint8_t * >(&(tmp[0])));
  for (unsigned int i = 0; i < 16; i++)
  {
    FloatVector4  c(FloatVector4::convertFloat16(tmp[i]));
    c.maxValues(FloatVector4(0.0f)).minValues(FloatVector4(65504.0f));
    dst[(i >> 2) * w + (i & 3)] = c.convertToFloat16();
  }
  return 16;
}

size_t DDSTexture16::decodeBlock_BC6S(
    std::uint64_t *dst, const unsigned char *src, unsigned int w)
{
  std::uint64_t tmp[16];
  (void) detexDecompressBlockBPTC_SIGNED_FLOAT(
             reinterpret_cast< const std::uint8_t * >(src), 0xFFFFFFFFU, 0U,
             reinterpret_cast< std::uint8_t * >(&(tmp[0])));
  for (unsigned int i = 0; i < 16; i++)
  {
    FloatVector4  c(FloatVector4::convertFloat16(tmp[i]));
    c.maxValues(FloatVector4(-65504.0f)).minValues(FloatVector4(65504.0f));
    dst[(i >> 2) * w + (i & 3)] = c.convertToFloat16();
  }
  return 16;
}

size_t DDSTexture16::decodeBlock_BC7(
    std::uint64_t *dst, const unsigned char *src, unsigned int w)
{
  std::uint32_t tmp[16];
  (void) detexDecompressBlockBPTC(
             reinterpret_cast< const std::uint8_t * >(src), 0xFFFFFFFFU, 0U,
             reinterpret_cast< std::uint8_t * >(&(tmp[0])));
  for (unsigned int i = 0; i < 16; i++)
  {
    dst[(i >> 2) * w + (i & 3)] =
        (FloatVector4(&(tmp[i])) * (1.0f / 255.0f)).convertToFloat16();
  }
  return 16;
}

static inline FloatVector4 bgraToRGBA(FloatVector4 c)
{
  return FloatVector4(c[2], c[1], c[0], c[3]);
}

size_t DDSTexture16::decodeLine_RGB(
    std::uint64_t *dst, const unsigned char *src, unsigned int w)
{
  unsigned int  x = 0;
  for ( ; (x + 1U) < w; x++, dst++, src = src + 3)
  {
    std::uint32_t b = FileBuffer::readUInt32Fast(src) | 0xFF000000U;
    *dst = (FloatVector4(b) * (1.0f / 255.0f)).convertToFloat16();
  }
  if (x < w) [[likely]]
  {
    std::uint32_t r = src[0];
    std::uint32_t g = src[1];
    std::uint32_t b = src[2];
    b = r | (g << 8) | (b << 16) | 0xFF000000U;
    *dst = (FloatVector4(b) * (1.0f / 255.0f)).convertToFloat16();
  }
  return (size_t(w) * 3);
}

size_t DDSTexture16::decodeLine_BGR(
    std::uint64_t *dst, const unsigned char *src, unsigned int w)
{
  unsigned int  x = 0;
  for ( ; (x + 1U) < w; x++, dst++, src = src + 3)
  {
    std::uint32_t b = FileBuffer::readUInt32Fast(src) | 0xFF000000U;
    *dst = (bgraToRGBA(FloatVector4(b)) * (1.0f / 255.0f)).convertToFloat16();
  }
  if (x < w) [[likely]]
  {
    std::uint32_t b = src[0];
    std::uint32_t g = src[1];
    std::uint32_t r = src[2];
    b = r | (g << 8) | (b << 16) | 0xFF000000U;
    *dst = (FloatVector4(b) * (1.0f / 255.0f)).convertToFloat16();
  }
  return (size_t(w) * 3);
}

size_t DDSTexture16::decodeLine_RGB32(
    std::uint64_t *dst, const unsigned char *src, unsigned int w)
{
  for (unsigned int x = 0; x < w; x++, dst++, src = src + 4)
  {
    std::uint32_t b = FileBuffer::readUInt32Fast(src) | 0xFF000000U;
    *dst = (FloatVector4(b) * (1.0f / 255.0f)).convertToFloat16();
  }
  return (size_t(w) << 2);
}

size_t DDSTexture16::decodeLine_BGR32(
    std::uint64_t *dst, const unsigned char *src, unsigned int w)
{
  for (unsigned int x = 0; x < w; x++, dst++, src = src + 4)
  {
    std::uint32_t b = FileBuffer::readUInt32Fast(src) | 0xFF000000U;
    *dst = (bgraToRGBA(FloatVector4(b)) * (1.0f / 255.0f)).convertToFloat16();
  }
  return (size_t(w) << 2);
}

size_t DDSTexture16::decodeLine_RGBA(
    std::uint64_t *dst, const unsigned char *src, unsigned int w)
{
  for (unsigned int x = 0; x < w; x++, dst++, src = src + 4)
  {
    std::uint32_t b = FileBuffer::readUInt32Fast(src);
    *dst = (FloatVector4(b) * (1.0f / 255.0f)).convertToFloat16();
  }
  return (size_t(w) << 2);
}

size_t DDSTexture16::decodeLine_BGRA(
    std::uint64_t *dst, const unsigned char *src, unsigned int w)
{
  for (unsigned int x = 0; x < w; x++, dst++, src = src + 4)
  {
    std::uint32_t b = FileBuffer::readUInt32Fast(src);
    *dst = (bgraToRGBA(FloatVector4(b)) * (1.0f / 255.0f)).convertToFloat16();
  }
  return (size_t(w) << 2);
}

size_t DDSTexture16::decodeLine_R8G8(
    std::uint64_t *dst, const unsigned char *src, unsigned int w)
{
  unsigned int  x = 0;
  for ( ; (x + 2U) <= w; x = x + 2U, dst = dst + 2, src = src + 4)
  {
    FloatVector4  c(FileBuffer::readUInt32Fast(src));
    c *= (1.0f / 255.0f);
    dst[0] = FloatVector4(c[0], c[1], 0.0f, 1.0f).convertToFloat16();
    dst[1] = FloatVector4(c[2], c[3], 0.0f, 1.0f).convertToFloat16();
  }
  if (x < w)
  {
    std::uint32_t b = 0xFF000000U | FileBuffer::readUInt16Fast(src);
    *dst = (FloatVector4(b) * (1.0f / 255.0f)).convertToFloat16();
  }
  return (size_t(w) << 1);
}

size_t DDSTexture16::decodeLine_R8(
    std::uint64_t *dst, const unsigned char *src, unsigned int w)
{
  unsigned int  x = 0;
  for ( ; (x + 4U) <= w; x = x + 4U, dst = dst + 4, src = src + 4)
  {
    FloatVector4  c(FileBuffer::readUInt32Fast(src));
    c *= (1.0f / 255.0f);
    FloatVector4  c0(c[0]);
    FloatVector4  c1(c[1]);
    FloatVector4  c2(c[2]);
    FloatVector4  c3(c[3]);
    c0[3] = 1.0f;
    c1[3] = 1.0f;
    c2[3] = 1.0f;
    c3[3] = 1.0f;
    dst[0] = c0.convertToFloat16();
    dst[1] = c1.convertToFloat16();
    dst[2] = c2.convertToFloat16();
    dst[3] = c3.convertToFloat16();
  }
  for ( ; x < w; x++, dst++, src++)
  {
    std::uint32_t b = (std::uint32_t(*src) * 0x00010101U) | 0xFF000000U;
    *dst = (FloatVector4(b) * (1.0f / 255.0f)).convertToFloat16();
  }
  return size_t(w);
}

size_t DDSTexture16::decodeLine_RGBA64F(
    std::uint64_t *dst, const unsigned char *src, unsigned int w)
{
  for (unsigned int x = 0; x < w; x++, dst++, src = src + 8)
  {
    std::uint64_t b = FileBuffer::readUInt64Fast(src);
    FloatVector4  c(FloatVector4::convertFloat16(b));
    c.maxValues(FloatVector4(-65504.0f)).minValues(FloatVector4(65504.0f));
    *dst = c.convertToFloat16();
  }
  return (size_t(w) << 3);
}

size_t DDSTexture16::decodeLine_RGB9E5(
    std::uint64_t *dst, const unsigned char *src, unsigned int w)
{
  for (unsigned int x = 0; x < w; x++, dst++, src = src + 4)
  {
    std::uint32_t b = FileBuffer::readUInt32Fast(src);
    *dst = FloatVector4::convertR9G9B9E5(b).convertToFloat16();
  }
  return (size_t(w) << 2);
}

size_t DDSTexture16::decodeLine_R10G10B10A2(
    std::uint64_t *dst, const unsigned char *src, unsigned int w)
{
  for (unsigned int x = 0; x < w; x++, dst++, src = src + 4)
  {
    std::uint32_t b = FileBuffer::readUInt32Fast(src);
    FloatVector4  c(FloatVector4::convertA2R10G10B10(b));
    *dst = (c * (1.0f / 255.0f)).shuffleValues(0xC6).convertToFloat16();
  }
  return (size_t(w) << 2);
}

size_t DDSTexture16::decodeLine_R8G8S(
    std::uint64_t *dst, const unsigned char *src, unsigned int w)
{
  unsigned int  x = 0;
  for ( ; (x + 2U) <= w; x = x + 2U, dst = dst + 2, src = src + 4)
  {
    std::uint32_t b = FileBuffer::readUInt32Fast(src) ^ 0x80808080U;
    FloatVector4  c(b);
    c = c * (1.0f / 127.5f) - 1.0f;
    dst[0] = FloatVector4(c[0], c[1], 0.0f, 1.0f).convertToFloat16();
    dst[1] = FloatVector4(c[2], c[3], 0.0f, 1.0f).convertToFloat16();
  }
  if (x < w)
  {
    std::uint32_t b = 0xFF808080U ^ FileBuffer::readUInt16Fast(src);
    *dst = (FloatVector4(b) * (1.0f / 127.5f) - 1.0f).convertToFloat16();
  }
  return (size_t(w) << 1);
}

size_t DDSTexture16::decodeLine_RGBA32S(
    std::uint64_t *dst, const unsigned char *src, unsigned int w)
{
  for (unsigned int x = 0; x < w; x++, dst++, src = src + 4)
  {
    std::uint32_t b = FileBuffer::readUInt32Fast(src) ^ 0x80808080U;
    *dst = (FloatVector4(b) * (1.0f / 127.5f) - 1.0f).convertToFloat16();
  }
  return (size_t(w) << 2);
}

size_t DDSTexture16::decodeLine_RGBA64(
    std::uint64_t *dst, const unsigned char *src, unsigned int w)
{
  for (unsigned int x = 0; x < w; x++, dst++, src = src + 8)
  {
    std::uint64_t b = FileBuffer::readUInt64Fast(src) ^ 0x8000800080008000ULL;
    FloatVector4  c(FloatVector4::convertInt16(b));
    *dst = ((c + 32768.0f) * float(1.0 / 65535.0)).convertToFloat16();
  }
  return (size_t(w) << 3);
}

static void srgbExpandBlock(void *buf, int w, int h, int pitch)
{
#if ENABLE_X86_64_SIMD >= 3
  if (!(w & 1)) [[likely]]
  {
    std::uint16_t *p = reinterpret_cast< std::uint16_t * >(buf);
    for ( ; h > 0; h--, p = p + (pitch << 2))
    {
      for (int x = 0; (x + 2) <= w; x = x + 2)
      {
        FloatVector8  c(p + (x << 2), false);
        DDSTexture16::srgbExpand(c).convertToFloat16(p + (x << 2));
      }
    }
    return;
  }
#endif
  std::uint64_t *p = reinterpret_cast< std::uint64_t * >(buf);
  for ( ; h > 0; h--, p = p + pitch)
  {
    for (int x = 0; x < w; x++)
    {
      FloatVector4  c(FloatVector4::convertFloat16(p[x]));
      p[x] = DDSTexture16::srgbExpand(c).convertToFloat16();
    }
  }
}

void DDSTexture16::loadTextureData(
    const unsigned char *srcPtr, int n, const DXGIFormatInfo& formatInfo,
    bool noSRGBExpand)
{
  size_t  (*decodeFunction)(std::uint64_t *,
                            const unsigned char *, unsigned int) =
      formatInfo.decodeFunction;
  bool    isCompressed = formatInfo.isCompressed;
  bool    isSRGB = false;
  if (!noSRGBExpand)
    isSRGB = formatInfo.isSRGB;
  size_t  dataOffs = size_t(n) * size_t(textureDataSize);
  for (int i = 0; i < 19; i++)
  {
    std::uint64_t *p = textureData[i] + dataOffs;
    unsigned int  w = (xMaskMip0 >> (unsigned char) i) + 1U;
    unsigned int  h = (yMaskMip0 >> (unsigned char) i) + 1U;
    if (i <= int(maxMipLevel))
    {
      if (!isCompressed)
      {
        // uncompressed format
        for (unsigned int y = 0; y < h; y++)
        {
          srcPtr = srcPtr + decodeFunction(p + (y * w), srcPtr, w);
          if (isSRGB)
            srgbExpandBlock(p + (y * w), int(w), 1, int(w));
        }
      }
      else if (w < 4 || h < 4)
      {
        std::uint64_t tmpBuf[16];
        for (unsigned int y = 0; y < h; y = y + 4)
        {
          for (unsigned int x = 0; x < w; x = x + 4)
          {
            srcPtr = srcPtr + decodeFunction(tmpBuf, srcPtr, 4);
            if (isSRGB)
              srgbExpandBlock(tmpBuf, 4, 4, 4);
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
          {
            srcPtr = srcPtr + decodeFunction(p + (y * w + x), srcPtr, w);
            if (isSRGB)
              srgbExpandBlock(p + (y * w + x), 4, 4, int(w));
          }
        }
      }
    }
    else
    {
      // generate missing mipmaps
      const std::uint64_t *p2 =
          textureData[i - 1] + (size_t(textureDataSize) * size_t(n));
      unsigned int  xMask = xMaskMip0 >> (unsigned char) (i - 1);
      unsigned int  yMask = yMaskMip0 >> (unsigned char) (i - 1);
      size_t  w2 = size_t(xMask + 1U);
      for (unsigned int y = 0; y < h; y++)
      {
        const std::uint64_t *p2y2 = p2 + (size_t(y << 1) * w2);
        const std::uint64_t *p2y2p1 = (yMask >= h ? p2y2 + w2 : p2y2);
        for (unsigned int x = 0; x < w; x++)
        {
          size_t  offsX2 = x << 1;
          size_t  offsX2p1 = offsX2 + size_t(w2 > w);
          FloatVector4  c(FloatVector4::convertFloat16(p2y2[offsX2]));
          c += FloatVector4::convertFloat16(p2y2[offsX2p1]);
          c += FloatVector4::convertFloat16(p2y2p1[offsX2]);
          c += FloatVector4::convertFloat16(p2y2p1[offsX2p1]);
          p[y * w + x] = (c * 0.25f).convertToFloat16();
        }
      }
    }
  }
}

void DDSTexture16::loadTexture(FileBuffer& buf, int mipOffset,
                               bool noSRGBExpand)
{
  buf.setPosition(0);
  if (buf.size() < 128)
    errorMessage("unsupported texture file format");
  const unsigned char *hdrData = buf.data();
  // "DDS ", size of DDS_HEADER (124)
  if (FileBuffer::readUInt64Fast(hdrData) != 0x0000007C20534444ULL)
    errorMessage("unsupported texture file format");
  unsigned int  flags = FileBuffer::readUInt32Fast(hdrData + 8);
  if ((flags & 0x1006) != 0x1006)       // height, width, pixel format required
    errorMessage("unsupported texture file format");
  maxMipLevel = 0U;
  channelCntFlags = 0;
  maxTextureNum = 0;
  dxgiFormat = 0;
  yMaskMip0 = FileBuffer::readUInt32Fast(hdrData + 12) - 1U;
  xMaskMip0 = FileBuffer::readUInt32Fast(hdrData + 16) - 1U;
  // width and height must be power of two and in the range 1 to 32768
  if (((xMaskMip0 | yMaskMip0) & ~0x7FFFU) ||
      ((xMaskMip0 & (xMaskMip0 + 1U)) | (yMaskMip0 & (yMaskMip0 + 1U))) != 0U)
  {
    errorMessage("invalid or unsupported texture dimensions");
  }
  // dwPitchOrLinearSize and dwDepth are ignored
  if (flags & 0x00020000)               // DDSD_MIPMAPCOUNT
  {
    maxMipLevel = FileBuffer::readUInt32Fast(hdrData + 28) - 1U;
    if (maxMipLevel & ~15U)
      errorMessage("invalid mipmap count");
  }
  // check size of DDS_PIXELFORMAT (must be 32)
  if (FileBuffer::readUInt32Fast(hdrData + 76) != 0x20)
    errorMessage("unsupported texture file format");
  unsigned int  dataOffs = 128;
  unsigned int  formatFlags = FileBuffer::readUInt32Fast(hdrData + 80);
  unsigned int  fourCC = FileBuffer::readUInt32Fast(hdrData + 84);
  if (formatFlags & 0x04)               // DDPF_FOURCC
  {
    if (FileBuffer::checkType(fourCC, "DX10"))
    {
      dataOffs = 148;
      if (buf.size() < 148)
        errorMessage("DDS file is shorter than expected");
      unsigned int  tmp = FileBuffer::readUInt32Fast(hdrData + 128);
      if (tmp && (tmp > 0x79U || !dxgiFormatMap[tmp]))
        throw FO76UtilsError("unsupported DXGI_FORMAT: 0x%08X", tmp);
      dxgiFormat = (unsigned char) tmp;
    }
    else
    {
      switch (fourCC)
      {
        case 0x00000024:
          dxgiFormat = 0x0B;            // DXGI_FORMAT_R16G16B16A16_UNORM
          break;
        case 0x00000071:
          dxgiFormat = 0x0A;            // DXGI_FORMAT_R16G16B16A16_FLOAT
          break;
        case 0x53344342:                // "BC4S"
          dxgiFormat = 0x51;
          break;
        case 0x55344342:                // "BC4U"
          dxgiFormat = 0x50;
          break;
        case 0x53354342:                // "BC5S"
          dxgiFormat = 0x54;
          break;
        case 0x32495441:                // "ATI2"
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
    if (!(formatFlags & 0x40))          // DDPF_RGB
    {
      if (!(formatFlags & 0x00080000))
        errorMessage("unsupported texture file format");
      formatFlags = formatFlags | 0x41;
    }
    unsigned int  bitsPerPixel = FileBuffer::readUInt32Fast(hdrData + 88);
    if ((bitsPerPixel - 8U) & ~24U)     // must be 8, 16, 24 or 32
      errorMessage("unsupported texture file format");
    unsigned long long  rgMask = FileBuffer::readUInt64Fast(hdrData + 92);
    unsigned long long  baMask = FileBuffer::readUInt64Fast(hdrData + 100);
    if (bitsPerPixel < 32U || !(formatFlags & 0x03))
      baMask = baMask & ~0xFF00000000000000ULL;
    if (rgMask == 0x0000FF00000000FFULL && baMask == 0x0000000000FF0000ULL)
      dxgiFormat = (bitsPerPixel < 32U ? 0x7F : 0x7B);
    else if (rgMask == 0x0000FF0000FF0000ULL && baMask == 0x00000000000000FFULL)
      dxgiFormat = (bitsPerPixel < 32U ? 0x7D : 0x5D);
    else if (rgMask == 0x0000FF00000000FFULL && baMask == 0xFF00000000FF0000ULL)
      dxgiFormat = (!(formatFlags & 0x00080000) ? 0x1D : 0x1F);
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
  bool    isCompressed = dxgiFmtInfo.isCompressed;
  channelCntFlags = dxgiFmtInfo.channelCnt;
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
    if (maxTextureNum >= 5 && xMaskMip0 == yMaskMip0)
      channelCntFlags = channelCntFlags | 0x80;
  }
  else if (buf.size() < (dataOffs + sizeRequired))
  {
    errorMessage("DDS file is shorter than expected");
  }
  const unsigned char *srcPtr = buf.data() + dataOffs;
  if (mipOffset < 0)
    maxMipLevel = 0;
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
      bufSize * (size_t(maxTextureNum) + 1) * sizeof(std::uint64_t);
  std::uint64_t *textureDataBuf =
      reinterpret_cast< std::uint64_t * >(std::malloc(totalDataSize));
  if (!textureDataBuf)
    throw std::bad_alloc();
  for (unsigned int i = 0; i < 19; i++)
    textureData[i] = textureDataBuf + dataOffsets[i];
  for (size_t i = 0; i <= maxTextureNum; i++)
  {
    loadTextureData(srcPtr + (i * sizeRequired), int(i),
                    dxgiFmtInfo, noSRGBExpand);
  }
  if (mipOffset > 0 && dataOffsets[1])
  {
    mipOffset = (mipOffset < 18 ? mipOffset : 18);
    size_t  offs = dataOffsets[mipOffset];
    xMaskMip0 = xMaskMip0 >> (unsigned char) mipOffset;
    yMaskMip0 = yMaskMip0 >> (unsigned char) mipOffset;
    totalDataSize = totalDataSize - (offs * sizeof(std::uint64_t));
    std::memmove(textureData[0], textureData[mipOffset], totalDataSize);
    textureDataBuf = reinterpret_cast< std::uint64_t * >(
                         std::realloc(textureDataBuf, totalDataSize));
    if (!textureDataBuf) [[unlikely]]
      throw std::bad_alloc();
    for (int i = 0; i < 19; i++)
    {
      int     j = ((i + mipOffset) < 18 ? (i + mipOffset) : 18);
      textureData[i] = textureDataBuf + (dataOffsets[j] - offs);
    }
  }
}

DDSTexture16::DDSTexture16(const char *fileName, int mipOffset,
                           bool noSRGBExpand)
{
  FileBuffer  tmpBuf(fileName);
  loadTexture(tmpBuf, mipOffset, noSRGBExpand);
}

DDSTexture16::DDSTexture16(const unsigned char *buf, size_t bufSize,
                           int mipOffset, bool noSRGBExpand)
{
  FileBuffer  tmpBuf(buf, bufSize);
  loadTexture(tmpBuf, mipOffset, noSRGBExpand);
}

DDSTexture16::DDSTexture16(FileBuffer& buf, int mipOffset, bool noSRGBExpand)
{
  loadTexture(buf, mipOffset, noSRGBExpand);
}

DDSTexture16::DDSTexture16(FloatVector4 c, bool srgbColor)
  : xMaskMip0(0U),
    yMaskMip0(0U),
    maxMipLevel(0),
    channelCntFlags(4),
    maxTextureNum(0),
    dxgiFormat(0),
    textureDataSize(0U)
{
  if (srgbColor)
    c = srgbExpand(c);
  textureColor = c.convertToFloat16();
#if ENABLE_X86_64_SIMD >= 2
  std::uintptr_t  tmp1 =
      std::uintptr_t(reinterpret_cast< unsigned char * >(&textureColor));
  const YMM_UInt64  tmp2 = { tmp1, tmp1, tmp1, tmp1 };
  // sizeof(textureData) == sizeof(std::uint64_t *) * 19
  __asm__ ("vmovdqu %t1, %0" : "=m" (textureData[0]) : "x" (tmp2));
  __asm__ ("vmovdqu %t1, %0" : "=m" (textureData[4]) : "x" (tmp2));
  __asm__ ("vmovdqu %t1, %0" : "=m" (textureData[8]) : "x" (tmp2));
  __asm__ ("vmovdqu %t1, %0" : "=m" (textureData[12]) : "x" (tmp2));
  __asm__ ("vmovdqu %x1, %0" : "=m" (textureData[16]) : "x" (tmp2));
  __asm__ ("vmovq %x1, %0" : "=m" (textureData[18]) : "x" (tmp2));
#else
  for (size_t i = 0; i < (sizeof(textureData) / sizeof(std::uint64_t *)); i++)
    textureData[i] = &textureColor;
#endif
}

DDSTexture16::~DDSTexture16()
{
  if (textureDataSize)
    std::free(textureData[0]);
}

FloatVector4 DDSTexture16::getPixelB(float x, float y, int mipLevel) const
{
  return getPixelB_Inline(x, y, mipLevel);
}

FloatVector4 DDSTexture16::getPixelT(float x, float y, float mipLevel) const
{
  return getPixelT_Inline(x, y, mipLevel);
}

FloatVector4 DDSTexture16::getPixelBM(float x, float y, int mipLevel) const
{
  return getPixelBM_Inline(x, y, mipLevel);
}

FloatVector4 DDSTexture16::getPixelTM(float x, float y, float mipLevel) const
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
  if (!convertTexCoord(x0, y0, xf, yf, xMask, yMask, x, y, m0)) [[unlikely]]
    return FloatVector4::convertFloat16(*(textureData[m0]));
  FloatVector4  c0(getPixelB_Clamp(textureData[m0], x0, y0, xf, yf,
                                   xMask, yMask));
  float   mf = float(m0);
  if (mf != mipLevel) [[likely]]
  {
    mf = mipLevel - mf;
    getNextMipTexCoord(x0, y0, xf, yf);
    FloatVector4  c1(getPixelB_Clamp(textureData[m0 + 1], x0, y0, xf, yf,
                                     xMask >> 1, yMask >> 1));
    c0 = (c0 * (1.0f - mf)) + (c1 * mf);
  }
  return c0;
}

FloatVector4 DDSTexture16::getPixelBC(float x, float y, int mipLevel) const
{
  return getPixelBC_Inline(x, y, mipLevel);
}

FloatVector4 DDSTexture16::getPixelTC(float x, float y, float mipLevel) const
{
  mipLevel = std::max(mipLevel, 0.0f);
  int     m0 = int(mipLevel);
  int     x0, y0;
  float   xf, yf;
  unsigned int  xMask, yMask;
  if (!convertTexCoord(x0, y0, xf, yf, xMask, yMask, x, y, m0)) [[unlikely]]
    return FloatVector4::convertFloat16(*(textureData[m0]));
  FloatVector4  c0(getPixelB_Clamp(textureData[m0], x0, y0, xf, yf,
                                   xMask, yMask));
  float   mf = float(m0);
  if (mf != mipLevel) [[likely]]
  {
    mf = mipLevel - mf;
    getNextMipTexCoord(x0, y0, xf, yf);
    FloatVector4  c1(getPixelB_Clamp(textureData[m0 + 1], x0, y0, xf, yf,
                                     xMask >> 1, yMask >> 1));
    c0 = (c0 * (1.0f - mf)) + (c1 * mf);
  }
  return c0;
}

const unsigned char DDSTexture16::cubeWrapTable[24] =
{
  // value = new_face + mirror_U * 0x20 + mirror_V * 0x40 + swap_UV * 0x80
  // +U   -U     +V    -V
  0x05, 0x04,  0xC3, 0xA2,      // face 0 (+X)
  0x04, 0x05,  0xA3, 0xC2,      // face 1 (-X)
  0xC0, 0xA1,  0x04, 0x65,      // face 2 (+Y)
  0xA0, 0xC1,  0x65, 0x04,      // face 3 (-Y)
  0x00, 0x01,  0x03, 0x02,      // face 4 (+Z)
  0x01, 0x00,  0x63, 0x62       // face 5 (-Z)
};

inline bool DDSTexture16::convertTexCoord_Cube(
    int& x0, int& y0, float& xf, float& yf,
    unsigned int& xMask, float x, float y, int mipLevel) const
{
  xMask = xMaskMip0 >> (unsigned char) mipLevel;
  xf = x * float(int(xMask + 1U)) - 0.5f;
  yf = y * float(int(xMask + 1U)) - 0.5f;
  float   xi = float(std::floor(xf));
  float   yi = float(std::floor(yf));
  x0 = int(xi);
  y0 = int(yi);
  xf -= xi;
  yf -= yi;
  return bool(xMask);
}

inline void DDSTexture16::getPixel_CubeWrap(
    FloatVector4& c, float& scale, float weight,
    const std::uint64_t *p, int x, int y, int n, size_t faceDataSize,
    unsigned int xMask)
{
  if (!wrapCubeMapCoord(x, y, n, int(xMask))) [[unlikely]]
  {
    scale = 1.0f / (1.0f - weight);
    return;
  }
  c += (FloatVector4::convertFloat16(
            p[(size_t(n) * faceDataSize)
              + ((unsigned int) y * (xMask + 1U) + (unsigned int) x)])
        * weight);
}

FloatVector4 DDSTexture16::getPixelB_CubeWrap(
    const std::uint64_t *p, int x0, int y0, int n, size_t faceDataSize,
    float xf, float yf, unsigned int xMask)
{
  FloatVector4  c(0.0f);
  float   scale = 1.0f;
  float   weight3 = xf * yf;
  float   weight2 = yf - weight3;
  float   weight1 = xf - weight3;
  float   weight0 = (1.0f - xf) - weight2;
  int     x1 = x0 + 1;
  int     y1 = y0 + 1;
  getPixel_CubeWrap(c, scale, weight0, p, x0, y0, n, faceDataSize, xMask);
  getPixel_CubeWrap(c, scale, weight1, p, x1, y0, n, faceDataSize, xMask);
  getPixel_CubeWrap(c, scale, weight2, p, x0, y1, n, faceDataSize, xMask);
  getPixel_CubeWrap(c, scale, weight3, p, x1, y1, n, faceDataSize, xMask);
  return (c * scale);
}

inline FloatVector4 DDSTexture16::getPixelB_Cube(
    const std::uint64_t *p, int x0, int y0, int n, size_t faceDataSize,
    float xf, float yf, unsigned int xMask)
{
  unsigned int  x0u = std::uint32_t(std::int32_t(x0));
  unsigned int  y0u = std::uint32_t(std::int32_t(y0));
  if (std::max< unsigned int >(x0u, y0u) >= xMask) [[unlikely]]
    return getPixelB_CubeWrap(p, x0, y0, n, faceDataSize, xf, yf, xMask);
  unsigned int  w = xMask + 1U;
  p = p + (size_t(n) * faceDataSize) + (y0u * w + x0u);
  return getPixelBFloat16(p, p + w, xf, yf);
}

FloatVector4 DDSTexture16::cubeMap(float x, float y, float z,
                                   float mipLevel) const
{
  float   xm = float(std::fabs(x));
  float   ym = float(std::fabs(y));
  float   zm = float(std::fabs(z));
  float   tmp = std::max(xm, std::max(ym, zm));
  float   d = 0.5f / tmp;
  mipLevel = std::max(mipLevel, 0.0f);
  int     m0 = int(mipLevel);
  float   mf = float(m0);
  size_t  n = 0;
  if (!(xm < tmp))                      // +X (0), -X (1)
  {
    if (x < 0.0f)
    {
      z = -z;
      n = 1;
    }
    x = 0.5f - z * d;
    y = 0.5f - y * d;
  }
  else if (!(ym < tmp))                 // +Y (2), -Y (3)
  {
    n = 2;
    if (y < 0.0f)
    {
      z = -z;
      n = 3;
    }
    x = x * d + 0.5f;
    y = z * d + 0.5f;
  }
  else                                  // +Z (4), -Z (5)
  {
    n = 4;
    if (z < 0.0f)
    {
      x = -x;
      n = 5;
    }
    x = x * d + 0.5f;
    y = 0.5f - y * d;
  }
  if (!(channelCntFlags & 0x80)) [[unlikely]]
    return getPixelTC(x, y, mipLevel);
  int     x0, y0;
  float   xf, yf;
  unsigned int  xMask;
  if (!convertTexCoord_Cube(x0, y0, xf, yf, xMask, x, y, m0)) [[unlikely]]
    mipLevel = mf;
  FloatVector4  c0(getPixelB_Cube(textureData[m0], x0, y0, n,
                                  textureDataSize, xf, yf, xMask));
  if (mf != mipLevel) [[likely]]
  {
    mf = mipLevel - mf;
    getNextMipTexCoord(x0, y0, xf, yf);
    FloatVector4  c1(getPixelB_Cube(textureData[m0 + 1], x0, y0, n,
                                    textureDataSize, xf, yf, xMask >> 1));
    c0 = (c0 * (1.0f - mf)) + (c1 * mf);
  }
  return c0;
}

FloatVector4 DDSTexture16::cubeMapImportanceSample(
    FloatVector4 t, FloatVector4 b, FloatVector4 n,
    const FloatVector4 *sampleBuf, size_t sampleCnt) const
{
  if (!(channelCntFlags & 0x80)) [[unlikely]]
    return cubeMap(n[0], n[1], n[2], 0.0f);
  FloatVector4  c(0.0f);
  do
  {
    FloatVector4  v = *(sampleBuf++);
    FloatVector4  xyz = (t * v[0]) + (b * v[1]) + (n * v[2]);
    float   mipLevel = v[3];
    float   x = xyz[0];
    float   y = xyz[1];
    float   z = xyz[2];
    float   xm = float(std::fabs(x));
    float   ym = float(std::fabs(y));
    float   zm = float(std::fabs(z));
    float   tmp = std::max(xm, std::max(ym, zm));
    float   d = 0.5f / tmp;
    int     m0 = int(mipLevel);
    float   mf = float(m0);
    size_t  i = 0;
    if (!(xm < tmp))                    // +X (0), -X (1)
    {
      if (x < 0.0f)
      {
        z = -z;
        i = 1;
      }
      x = 0.5f - z * d;
      y = 0.5f - y * d;
    }
    else if (!(ym < tmp))               // +Y (2), -Y (3)
    {
      i = 2;
      if (y < 0.0f)
      {
        z = -z;
        i = 3;
      }
      x = x * d + 0.5f;
      y = z * d + 0.5f;
    }
    else                                // +Z (4), -Z (5)
    {
      i = 4;
      if (z < 0.0f)
      {
        x = -x;
        i = 5;
      }
      x = x * d + 0.5f;
      y = 0.5f - y * d;
    }
    int     x0, y0;
    float   xf, yf;
    unsigned int  xMask;
    if (!convertTexCoord_Cube(x0, y0, xf, yf, xMask, x, y, m0)) [[unlikely]]
      mipLevel = mf;
    FloatVector4  c0(getPixelB_Cube(textureData[m0], x0, y0, i,
                                    textureDataSize, xf, yf, xMask));
    if (mf != mipLevel) [[likely]]
    {
      mf = mipLevel - mf;
      getNextMipTexCoord(x0, y0, xf, yf);
      FloatVector4  c1(getPixelB_Cube(textureData[m0 + 1], x0, y0, i,
                                      textureDataSize, xf, yf, xMask >> 1));
      c0 = (c0 * (1.0f - mf)) + (c1 * mf);
    }
    c += (c0 * v[2]);
  }
  while (--sampleCnt);
  return c;
}

