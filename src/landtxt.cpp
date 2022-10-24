
#include "common.hpp"
#include "landtxt.hpp"

static const DDSTexture defaultLandTxt_N(0xFFFF8080U);
static const DDSTexture defaultLandTxt_S(0xFF004020U);
static const DDSTexture defaultLandTxt_R(0xFF0A0A0AU);
static const DDSTexture defaultLandTxt_L(0xFF00E340U);

inline FloatVector4 LandscapeTexture::rotateNormalFO76(const FloatVector4& n)
{
  float   x = n[0];
  float   y = n[1];
  return FloatVector4((y + x) * 0.70710678f, (y - x) * 0.70710678f, n[2], n[3]);
}

inline FloatVector4 LandscapeTexture::blendColors(
    FloatVector4 c0, FloatVector4 c1, float f)
{
  FloatVector4  tmp0(c0);
  FloatVector4  tmp1(c1);
  tmp0 *= (1.0f - f);
  tmp1 *= f;
  tmp0 += tmp1;
  return tmp0;
}

inline void LandscapeTexture::blendNormals(
    FloatVector4 *n, FloatVector4 *n2, float f)
{
  n2[0] -= n[0];
  n2[1] -= n[1];
  n2[2] -= n[2];
  n2[0] *= f;
  n2[1] *= f;
  n2[2] *= f;
  n[0] += n2[0];
  n[1] += n2[1];
  n[2] += n2[2];
}

inline FloatVector4 LandscapeTexture::getNormal(
    size_t t, int x, int y, int m) const
{
  FloatVector4  tmp(&(landTextures[t][1]->getPixelN(x, y, m)));
  tmp -= 127.5f;
  tmp[2] = FloatVector4::squareRootFast(127.5f * 127.5f - tmp.dotProduct2(tmp));
  return tmp;
}

inline void LandscapeTexture::getNormalSpecI(
    FloatVector4 *n, size_t t, int x, int y, int m) const
{
  n[0] = getNormal(t, x, y, m);
  n[1] = FloatVector4(&(landTextures[t][6]->getPixelN(x, y, m)));
  n[2] = FloatVector4(0.0f);
}

inline void LandscapeTexture::getNormalSpecF(
    FloatVector4 *n, size_t t, float x, float y, float m) const
{
  FloatVector4  tmp(landTextures[t][1]->getPixelT_N(x, y, m) - 127.5f);
  tmp[2] = FloatVector4::squareRootFast(127.5f * 127.5f - tmp.dotProduct2(tmp));
  n[0] = tmp;
  n[1] = landTextures[t][6]->getPixelT_N(x, y, m);
  n[2] = FloatVector4(0.0f);
}

inline void LandscapeTexture::getNormalReflLightI(
    FloatVector4 *n, size_t t, int x, int y, int m) const
{
  n[0] = getNormal(t, x, y, m);
  n[1] = FloatVector4(&(landTextures[t][9]->getPixelN(x, y, m)));
  n[2] = FloatVector4(&(landTextures[t][8]->getPixelN(x, y, m)));
}

inline void LandscapeTexture::getNormalReflLightF(
    FloatVector4 *n, size_t t, float x, float y, float m) const
{
  FloatVector4  tmp(landTextures[t][1]->getPixelT_N(x, y, m) - 127.5f);
  tmp[2] = FloatVector4::squareRootFast(127.5f * 127.5f - tmp.dotProduct2(tmp));
  n[0] = tmp;
  n[1] = landTextures[t][9]->getPixelT_N(x, y, m);
  n[2] = landTextures[t][8]->getPixelT_N(x, y, m);
}

inline std::uint32_t LandscapeTexture::normalToColor(FloatVector4 n)
{
  n.normalize3Fast();
  n += 1.0f;
  n *= 127.5f;
  return std::uint32_t(n);
}

inline std::uint32_t LandscapeTexture::getFO76VertexColor(size_t offs) const
{
  std::uint32_t c = FileBuffer::readUInt16Fast(vclrData16 + (offs << 1));
  c = ((c >> 10) & 0x1FU) | ((c << 3) & 0x1F00U) | ((c & 0x1FU) << 16);
  return c;
}

inline std::uint32_t LandscapeTexture::getTES4VertexColor(size_t offs) const
{
  std::uint32_t b = vclrData24[offs * 3];
  std::uint32_t g = vclrData24[offs * 3 + 1];
  std::uint32_t r = vclrData24[offs * 3 + 2];
  return (r | (g << 8) | (b << 16));
}

inline FloatVector4 LandscapeTexture::getFO76VertexColor(
    int x, int y, int renderScale) const
{
  int     m = int(fo76VClrMip) + renderScale;
  y = y - (((1 << fo76VClrMip) - 1) << renderScale);
  y = (y >= 0 ? y : 0);
  x = x << (8 - m);
  y = y << (8 - m);
  int     xf = x & 255;
  int     yf = y & 255;
  x = x >> 8;
  y = y >> 8;
  size_t  w = size_t(width >> fo76VClrMip);
  size_t  h = size_t(height >> fo76VClrMip);
  size_t  offs0 = size_t(y) * w + size_t(x);
  size_t  offs2 = offs0;
  if (y < int(h - 1))
    offs2 += w;
  size_t  offs1 = offs0;
  size_t  offs3 = offs2;
  if (x < int(w - 1))
  {
    offs1++;
    offs3++;
  }
  FloatVector4  c0(getFO76VertexColor(offs0), getFO76VertexColor(offs1),
                   getFO76VertexColor(offs2), getFO76VertexColor(offs3),
                   float(xf) * (1.0f / 256.0f), float(yf) * (1.0f / 256.0f));
  c0 *= (1.0f / 31.0f);
  return c0.srgbCompress();
}

inline FloatVector4 LandscapeTexture::getTES4VertexColor(
    int x, int y, int renderScale) const
{
  y = (y >= 0 ? y : 0);
  x = x << (8 - renderScale);
  y = y << (8 - renderScale);
  int     xf = x & 255;
  int     yf = y & 255;
  x = x >> 8;
  y = y >> 8;
  size_t  w = size_t(width);
  size_t  h = size_t(height);
  size_t  offs0 = size_t(y) * w + size_t(x);
  size_t  offs2 = offs0;
  if (y < int(h - 1))
    offs2 += w;
  size_t  offs1 = offs0;
  size_t  offs3 = offs2;
  if (x < int(w - 1))
  {
    offs1++;
    offs3++;
  }
  return FloatVector4(getTES4VertexColor(offs0), getTES4VertexColor(offs1),
                      getTES4VertexColor(offs2), getTES4VertexColor(offs3),
                      float(xf) * (1.0f / 256.0f), float(yf) * (1.0f / 256.0f));
}

FloatVector4 LandscapeTexture::renderPixelFO76I_NoNormals(
    const LandscapeTexture& p, FloatVector4 *n,
    int x, int y, int txtX, int txtY)
{
  (void) n;
  x = (x < p.width ? x : (p.width - 1));
  y = (y < p.height ? y : (p.height - 1));
  size_t  offs = size_t(y) * size_t(p.width) + size_t(x);
  const unsigned char *p0 = p.txtSetData;
  p0 = p0 + (((size_t(y) >> p.txtSetMip) * (size_t(p.width) >> p.txtSetMip)
              + (size_t(x) >> p.txtSetMip)) << 4);
  std::uint32_t a = FileBuffer::readUInt16Fast(p.ltexData16 + (offs << 1));
  a = (a << 3) | 7U;
  std::uint32_t c = p.defaultColor;
  p0 = p0 + 6;
  for ( ; a; a = a << 3)
  {
    p0--;
    if (a < 0x8000U)
      continue;
    std::uint32_t aTmp = ~a & 0x00038000U;
    a = a & 0x7FFFU;
    unsigned char t = *p0;
    if (t >= p.landTextureCnt || !p.landTextures[t][0])
      continue;
    std::uint32_t cTmp =
        p.landTextures[t][0]->getPixelN(txtX - txtY, txtX + txtY,
                                        int(p.mipLevel));
    if (cTmp < ((aTmp * 0x4900U + 0x00800000U) & 0xFF000000U))
      continue;
    c = cTmp;
    break;
  }
  return FloatVector4(c);
}

FloatVector4 LandscapeTexture::renderPixelFO76I_NoPBR(
    const LandscapeTexture& p, FloatVector4 *n,
    int x, int y, int txtX, int txtY)
{
  x = (x < p.width ? x : (p.width - 1));
  y = (y < p.height ? y : (p.height - 1));
  size_t  offs = size_t(y) * size_t(p.width) + size_t(x);
  const unsigned char *p0 = p.txtSetData;
  p0 = p0 + (((size_t(y) >> p.txtSetMip) * (size_t(p.width) >> p.txtSetMip)
              + (size_t(x) >> p.txtSetMip)) << 4);
  std::uint32_t a = FileBuffer::readUInt16Fast(p.ltexData16 + (offs << 1));
  a = (a << 3) | 7U;
  std::uint32_t c = p.defaultColor;
  p0 = p0 + 6;
  for ( ; a; a = a << 3)
  {
    p0--;
    if (a < 0x8000U)
      continue;
    std::uint32_t aTmp = ~a & 0x00038000U;
    a = a & 0x7FFFU;
    unsigned char t = *p0;
    if (t >= p.landTextureCnt || !p.landTextures[t][0])
      continue;
    std::uint32_t cTmp =
        p.landTextures[t][0]->getPixelN(txtX - txtY, txtX + txtY,
                                        int(p.mipLevel));
    if (cTmp < ((aTmp * 0x4900U + 0x00800000U) & 0xFF000000U))
      continue;
    c = cTmp;
    n[0] = p.getNormal(t, txtX - txtY, txtX + txtY, int(p.mipLevel));
    break;
  }
  return FloatVector4(c);
}

FloatVector4 LandscapeTexture::renderPixelFO76I(
    const LandscapeTexture& p, FloatVector4 *n,
    int x, int y, int txtX, int txtY)
{
  x = (x < p.width ? x : (p.width - 1));
  y = (y < p.height ? y : (p.height - 1));
  size_t  offs = size_t(y) * size_t(p.width) + size_t(x);
  const unsigned char *p0 = p.txtSetData;
  p0 = p0 + (((size_t(y) >> p.txtSetMip) * (size_t(p.width) >> p.txtSetMip)
              + (size_t(x) >> p.txtSetMip)) << 4);
  std::uint32_t a = FileBuffer::readUInt16Fast(p.ltexData16 + (offs << 1));
  a = (a << 3) | 7U;
  std::uint32_t c = p.defaultColor;
  p0 = p0 + 6;
  for ( ; a; a = a << 3)
  {
    p0--;
    if (a < 0x8000U)
      continue;
    std::uint32_t aTmp = ~a & 0x00038000U;
    a = a & 0x7FFFU;
    unsigned char t = *p0;
    if (t >= p.landTextureCnt || !p.landTextures[t][0])
      continue;
    std::uint32_t cTmp =
        p.landTextures[t][0]->getPixelN(txtX - txtY, txtX + txtY,
                                        int(p.mipLevel));
    if (cTmp < ((aTmp * 0x4900U + 0x00800000U) & 0xFF000000U))
      continue;
    c = cTmp;
    p.getNormalReflLightI(n, t, txtX - txtY, txtX + txtY, int(p.mipLevel));
    break;
  }
  return FloatVector4(c);
}

FloatVector4 LandscapeTexture::renderPixelFO76F(
    const LandscapeTexture& p, FloatVector4 *n,
    int x, int y, int txtX, int txtY)
{
  x = (x < p.width ? x : (p.width - 1));
  y = (y < p.height ? y : (p.height - 1));
  size_t  offs = size_t(y) * size_t(p.width) + size_t(x);
  const unsigned char *p0 = p.txtSetData;
  p0 = p0 + (((size_t(y) >> p.txtSetMip) * (size_t(p.width) >> p.txtSetMip)
              + (size_t(x) >> p.txtSetMip)) << 4);
  std::uint32_t a = FileBuffer::readUInt16Fast(p.ltexData16 + (offs << 1));
  a = (a << 3) | 7U;
  FloatVector4  c(p.defaultColor);
  p0 = p0 + 6;
  for ( ; a; a = a << 3)
  {
    p0--;
    if (a < 0x8000U)
      continue;
    std::uint32_t aTmp = ~a & 0x00038000U;
    a = a & 0x7FFFU;
    unsigned char t = *p0;
    if (t >= p.landTextureCnt || !p.landTextures[t][0])
      continue;
    FloatVector4  cTmp(p.landTextures[t][0]->getPixelT_N(
                           float(txtX - txtY) * p.txtScale,
                           float(txtX + txtY) * p.txtScale, p.mipLevel));
    if (cTmp[3] < (float(int(aTmp)) * (255.5f / float(0x00038000))))
      continue;
    c = cTmp;
    p.getNormalReflLightF(n, t, float(txtX - txtY) * p.txtScale,
                          float(txtX + txtY) * p.txtScale, p.mipLevel);
    break;
  }
  return c;
}

FloatVector4 LandscapeTexture::renderPixelFO76I_GCVR(
    const LandscapeTexture& p, FloatVector4 *n,
    int x, int y, int txtX, int txtY)
{
  x = (x < p.width ? x : (p.width - 1));
  y = (y < p.height ? y : (p.height - 1));
  size_t  offs = size_t(y) * size_t(p.width) + size_t(x);
  const unsigned char *p0 = p.txtSetData;
  p0 = p0 + (((size_t(y) >> p.txtSetMip) * (size_t(p.width) >> p.txtSetMip)
              + (size_t(x) >> p.txtSetMip)) << 4);
  std::uint32_t a = FileBuffer::readUInt16Fast(p.ltexData16 + (offs << 1));
  a = (a << 3) | 7U;
  FloatVector4  c(p.defaultColor);
  p0 = p0 + 6;
  for ( ; a; a = a << 3)
  {
    p0--;
    if (a < 0x8000U)
      continue;
    std::uint32_t aTmp = ~a & 0x00038000U;
    a = a & 0x7FFFU;
    unsigned char t = *p0;
    if (t >= p.landTextureCnt || !p.landTextures[t][0])
      continue;
    std::uint32_t cTmp =
        p.landTextures[t][0]->getPixelN(txtX - txtY, txtX + txtY,
                                        int(p.mipLevel));
    if (cTmp < ((aTmp * 0x4900U + 0x00800000U) & 0xFF000000U))
      continue;
    c = FloatVector4(cTmp);
    p.getNormalReflLightI(n, t, txtX - txtY, txtX + txtY, int(p.mipLevel));
    break;
  }
  p0 = p.txtSetData + ((size_t(p0 - p.txtSetData) & ~7UL) | 8UL);
  a = p.gcvrData[offs];
  for ( ; a; a = a >> 1, p0++)
  {
    if (!(a & 1))
      continue;
    unsigned char t = *p0;
    if (t >= p.landTextureCnt || !p.landTextures[t][0])
      continue;
    FloatVector4  cTmp(p.landTextures[t][0]->getPixelN(txtX - txtY, txtX + txtY,
                                                       int(p.mipLevel)));
    float   aTmp = cTmp[3] * (1.0f / 512.0f);
    c = blendColors(c, cTmp, aTmp);
    FloatVector4  nTmp[3];
    p.getNormalReflLightI(nTmp, t, txtX - txtY, txtX + txtY, int(p.mipLevel));
    blendNormals(n, nTmp, aTmp);
  }
  return c;
}

FloatVector4 LandscapeTexture::renderPixelFO76F_GCVR(
    const LandscapeTexture& p, FloatVector4 *n,
    int x, int y, int txtX, int txtY)
{
  x = (x < p.width ? x : (p.width - 1));
  y = (y < p.height ? y : (p.height - 1));
  size_t  offs = size_t(y) * size_t(p.width) + size_t(x);
  const unsigned char *p0 = p.txtSetData;
  p0 = p0 + (((size_t(y) >> p.txtSetMip) * (size_t(p.width) >> p.txtSetMip)
              + (size_t(x) >> p.txtSetMip)) << 4);
  std::uint32_t a = FileBuffer::readUInt16Fast(p.ltexData16 + (offs << 1));
  a = (a << 3) | 7U;
  FloatVector4  c(p.defaultColor);
  p0 = p0 + 6;
  for ( ; a; a = a << 3)
  {
    p0--;
    if (a < 0x8000U)
      continue;
    std::uint32_t aTmp = ~a & 0x00038000U;
    a = a & 0x7FFFU;
    unsigned char t = *p0;
    if (t >= p.landTextureCnt || !p.landTextures[t][0])
      continue;
    FloatVector4  cTmp(p.landTextures[t][0]->getPixelT_N(
                           float(txtX - txtY) * p.txtScale,
                           float(txtX + txtY) * p.txtScale, p.mipLevel));
    if (cTmp[3] < (float(int(aTmp)) * (255.5f / float(0x00038000))))
      continue;
    c = cTmp;
    p.getNormalReflLightF(n, t, float(txtX - txtY) * p.txtScale,
                          float(txtX + txtY) * p.txtScale, p.mipLevel);
    break;
  }
  p0 = p.txtSetData + ((size_t(p0 - p.txtSetData) & ~7UL) | 8UL);
  a = p.gcvrData[offs];
  for ( ; a; a = a >> 1, p0++)
  {
    if (!(a & 1))
      continue;
    unsigned char t = *p0;
    if (t >= p.landTextureCnt || !p.landTextures[t][0])
      continue;
    FloatVector4  cTmp(p.landTextures[t][0]->getPixelT_N(
                           float(txtX - txtY) * p.txtScale,
                           float(txtX + txtY) * p.txtScale, p.mipLevel));
    float   aTmp = cTmp[3] * (1.0f / 512.0f);
    c = blendColors(c, cTmp, aTmp);
    FloatVector4  nTmp[3];
    p.getNormalReflLightF(nTmp, t, float(txtX - txtY) * p.txtScale,
                          float(txtX + txtY) * p.txtScale, p.mipLevel);
    blendNormals(n, nTmp, aTmp);
  }
  return c;
}

FloatVector4 LandscapeTexture::renderPixelTES4I_NoNormals(
    const LandscapeTexture& p, FloatVector4 *n,
    int x, int y, int txtX, int txtY)
{
  (void) n;
  x = (x < p.width ? x : (p.width - 1));
  y = (y < p.height ? y : (p.height - 1));
  size_t  offs = size_t(y) * size_t(p.width) + size_t(x);
  const unsigned char *p0 = p.txtSetData;
  p0 = p0 + (((size_t(y) >> p.txtSetMip) * (size_t(p.width) >> p.txtSetMip)
              + (size_t(x) >> p.txtSetMip)) << 4);
  std::uint64_t a = FileBuffer::readUInt32Fast(p.ltexData32 + (offs << 2));
  a = (a << 4) | 15ULL;
  FloatVector4  c(p.defaultColor);
  unsigned char prvTexture = 0xFF;
  for ( ; a; a = a >> 4, p0++)
  {
    std::uint32_t aTmp = (std::uint32_t) (a & 0x0FU);
    if (!aTmp)
      continue;
    unsigned char t = *p0;
    if (t >= p.landTextureCnt || t == prvTexture || !p.landTextures[t][0])
      continue;
    FloatVector4  cTmp(p.landTextures[t][0]->getPixelN(txtX, txtY,
                                                       int(p.mipLevel)));
    prvTexture = (aTmp == 15 ? t : (unsigned char) 0xFF);
    if (aTmp != 15)
      cTmp = blendColors(c, cTmp, float(int(aTmp)) * (1.0f / 15.0f));
    c = cTmp;
  }
  return c;
}

FloatVector4 LandscapeTexture::renderPixelTES4I_NoPBR(
    const LandscapeTexture& p, FloatVector4 *n,
    int x, int y, int txtX, int txtY)
{
  x = (x < p.width ? x : (p.width - 1));
  y = (y < p.height ? y : (p.height - 1));
  size_t  offs = size_t(y) * size_t(p.width) + size_t(x);
  const unsigned char *p0 = p.txtSetData;
  p0 = p0 + (((size_t(y) >> p.txtSetMip) * (size_t(p.width) >> p.txtSetMip)
              + (size_t(x) >> p.txtSetMip)) << 4);
  std::uint64_t a = FileBuffer::readUInt32Fast(p.ltexData32 + (offs << 2));
  a = (a << 4) | 15ULL;
  FloatVector4  c(p.defaultColor);
  unsigned char prvTexture = 0xFF;
  for ( ; a; a = a >> 4, p0++)
  {
    std::uint32_t aTmp = (std::uint32_t) (a & 0x0FU);
    if (!aTmp)
      continue;
    unsigned char t = *p0;
    if (t >= p.landTextureCnt || t == prvTexture || !p.landTextures[t][0])
      continue;
    FloatVector4  cTmp(p.landTextures[t][0]->getPixelN(txtX, txtY,
                                                       int(p.mipLevel)));
    float   aTmp_f = float(int(aTmp)) * (1.0f / 15.0f);
    FloatVector4  nTmp(p.getNormal(t, txtX, txtY, int(p.mipLevel)));
    n[0] = n[0] + ((nTmp - n[0]) * aTmp_f);
    prvTexture = (aTmp == 15 ? t : (unsigned char) 0xFF);
    if (aTmp != 15)
      cTmp = blendColors(c, cTmp, aTmp_f);
    c = cTmp;
  }
  return c;
}

FloatVector4 LandscapeTexture::renderPixelTES4I(
    const LandscapeTexture& p, FloatVector4 *n,
    int x, int y, int txtX, int txtY)
{
  x = (x < p.width ? x : (p.width - 1));
  y = (y < p.height ? y : (p.height - 1));
  size_t  offs = size_t(y) * size_t(p.width) + size_t(x);
  const unsigned char *p0 = p.txtSetData;
  p0 = p0 + (((size_t(y) >> p.txtSetMip) * (size_t(p.width) >> p.txtSetMip)
              + (size_t(x) >> p.txtSetMip)) << 4);
  std::uint64_t a = FileBuffer::readUInt32Fast(p.ltexData32 + (offs << 2));
  a = (a << 4) | 15ULL;
  FloatVector4  c(p.defaultColor);
  unsigned char prvTexture = 0xFF;
  for ( ; a; a = a >> 4, p0++)
  {
    std::uint32_t aTmp = (std::uint32_t) (a & 0x0FU);
    if (!aTmp)
      continue;
    unsigned char t = *p0;
    if (t >= p.landTextureCnt || t == prvTexture || !p.landTextures[t][0])
      continue;
    FloatVector4  cTmp(p.landTextures[t][0]->getPixelN(txtX, txtY,
                                                       int(p.mipLevel)));
    float   aTmp_f = float(int(aTmp)) * (1.0f / 15.0f);
    FloatVector4  nTmp[3];
    p.getNormalSpecI((aTmp != 15 ? &(nTmp[0]) : n),
                     t, txtX, txtY, int(p.mipLevel));
    prvTexture = t;
    if (aTmp != 15)
    {
      prvTexture = 0xFF;
      blendNormals(n, nTmp, aTmp_f);
      cTmp = blendColors(c, cTmp, aTmp_f);
    }
    c = cTmp;
  }
  return c;
}

FloatVector4 LandscapeTexture::renderPixelTES4F(
    const LandscapeTexture& p, FloatVector4 *n,
    int x, int y, int txtX, int txtY)
{
  x = (x < p.width ? x : (p.width - 1));
  y = (y < p.height ? y : (p.height - 1));
  size_t  offs = size_t(y) * size_t(p.width) + size_t(x);
  const unsigned char *p0 = p.txtSetData;
  p0 = p0 + (((size_t(y) >> p.txtSetMip) * (size_t(p.width) >> p.txtSetMip)
              + (size_t(x) >> p.txtSetMip)) << 4);
  std::uint64_t a = FileBuffer::readUInt32Fast(p.ltexData32 + (offs << 2));
  a = (a << 4) | 15ULL;
  FloatVector4  c(p.defaultColor);
  unsigned char prvTexture = 0xFF;
  for ( ; a; a = a >> 4, p0++)
  {
    std::uint32_t aTmp = (std::uint32_t) (a & 0x0FU);
    if (!aTmp)
      continue;
    unsigned char t = *p0;
    if (t >= p.landTextureCnt || t == prvTexture || !p.landTextures[t][0])
      continue;
    FloatVector4  cTmp(p.landTextures[t][0]->getPixelT_N(
                           float(txtX) * p.txtScale, float(txtY) * p.txtScale,
                           p.mipLevel));
    float   aTmp_f = float(int(aTmp)) * (1.0f / 15.0f);
    FloatVector4  nTmp[3];
    p.getNormalSpecF((aTmp != 15 ? &(nTmp[0]) : n), t, float(txtX) * p.txtScale,
                     float(txtY) * p.txtScale, p.mipLevel);
    prvTexture = t;
    if (aTmp != 15)
    {
      prvTexture = 0xFF;
      blendNormals(n, nTmp, aTmp_f);
      cTmp = blendColors(c, cTmp, aTmp_f);
    }
    c = cTmp;
  }
  return c;
}

void LandscapeTexture::setRenderPixelFunction(
    bool haveNormalMaps, bool havePBRMaps)
{
  if (isFO76)
  {
    if (!gcvrData)
    {
      if (integerMip)
      {
        if (!haveNormalMaps)
          renderPixelFunction = &renderPixelFO76I_NoNormals;
        else if (!havePBRMaps)
          renderPixelFunction = &renderPixelFO76I_NoPBR;
        else
          renderPixelFunction = &renderPixelFO76I;
      }
      else
      {
        renderPixelFunction = &renderPixelFO76F;
      }
    }
    else if (integerMip)
    {
      renderPixelFunction = &renderPixelFO76I_GCVR;
    }
    else
    {
      renderPixelFunction = &renderPixelFO76F_GCVR;
    }
  }
  else
  {
    if (integerMip)
    {
      if (!haveNormalMaps)
        renderPixelFunction = &renderPixelTES4I_NoNormals;
      else if (!havePBRMaps)
        renderPixelFunction = &renderPixelTES4I_NoPBR;
      else
        renderPixelFunction = &renderPixelTES4I;
    }
    else
    {
      renderPixelFunction = &renderPixelTES4F;
    }
  }
}

void LandscapeTexture::copyTextureSet(
    const LandscapeTextureSet *landTxts, size_t landTxtCnt)
{
  if (!landTxtCnt)
    return;
  landTextures = new LandscapeTextureSet[landTxtCnt];
  for (size_t i = 0; i < landTxtCnt; i++)
  {
    for (size_t j = 0; j < 10; j++)
    {
      if (landTxts[i][j] || !(j == 1 || j == 6 || j >= 8))
        landTextures[i][j] = landTxts[i][j];
      else if (j == 1)
        landTextures[i][j] = &defaultLandTxt_N;
      else if (j == 6)
        landTextures[i][j] = &defaultLandTxt_S;
      else if (j == 8)
        landTextures[i][j] = &defaultLandTxt_R;
      else
        landTextures[i][j] = &defaultLandTxt_L;
    }
  }
}

LandscapeTexture::LandscapeTexture(
    const unsigned char *txtSetPtr, const unsigned char *ltex32Ptr,
    const unsigned char *vclr24Ptr, const unsigned char *ltex16Ptr,
    const unsigned char *vclr16Ptr, const unsigned char *gcvrPtr,
    int vertexCntX, int vertexCntY, int cellResolution,
    const LandscapeTextureSet *landTxts, size_t landTxtCnt)
  : txtSetData(txtSetPtr),
    ltexData32(ltex32Ptr),
    vclrData24(vclr24Ptr),
    ltexData16(ltex16Ptr),
    vclrData16(vclr16Ptr),
    gcvrData(gcvrPtr),
    renderPixelFunction(&renderPixelFO76I_NoNormals),
    landTextures((LandscapeTextureSet *) 0),
    landTextureCnt(landTxtCnt),
    mipLevel(0.0f),
    rgbScale(1.0f),
    width(vertexCntX),
    height(vertexCntY),
    txtScale(1.0f),
    integerMip(true),
    isFO76(ltex16Ptr != (unsigned char *) 0),
    txtSetMip(0),
    fo76VClrMip(0),
    defaultColor(0x003F3F3FU)
{
  copyTextureSet(landTxts, landTxtCnt);
  while ((2 << txtSetMip) < cellResolution)
    txtSetMip++;
  while ((32 << fo76VClrMip) < cellResolution)
    fo76VClrMip++;
}

LandscapeTexture::LandscapeTexture(
    const LandscapeData& landData,
    const LandscapeTextureSet *landTxts, size_t landTxtCnt)
  : txtSetData(landData.getCellTextureSets()),
    ltexData32(reinterpret_cast< const unsigned char * >(
                   landData.getLandTexture32())),
    vclrData24(landData.getVertexColor24()),
    ltexData16(reinterpret_cast< const unsigned char * >(
                   landData.getLandTexture16())),
    vclrData16(reinterpret_cast< const unsigned char * >(
                   landData.getVertexColor16())),
    gcvrData(landData.getGroundCover()),
    renderPixelFunction(&renderPixelFO76I_NoNormals),
    landTextures((LandscapeTextureSet *) 0),
    landTextureCnt(landTxtCnt),
    mipLevel(0.0f),
    rgbScale(1.0f),
    width(landData.getImageWidth()),
    height(landData.getImageHeight()),
    txtScale(1.0f),
    integerMip(true),
    isFO76(ltexData16 != (unsigned char *) 0),
    txtSetMip(0),
    fo76VClrMip(0),
    defaultColor(0x003F3F3FU)
{
  copyTextureSet(landTxts, landTxtCnt);
  int     cellResolution = landData.getCellResolution();
  while ((2 << txtSetMip) < cellResolution)
    txtSetMip++;
  while ((32 << fo76VClrMip) < cellResolution)
    fo76VClrMip++;
}

LandscapeTexture::~LandscapeTexture()
{
  if (landTextures)
    delete[] landTextures;
}

void LandscapeTexture::setMipLevel(float n)
{
  mipLevel = n;
  integerMip = (float(int(mipLevel)) == mipLevel);
  txtScale = 1.0f;
  if (!integerMip)
    txtScale = float(std::pow(2.0, mipLevel - float(int(mipLevel))));
}

void LandscapeTexture::setRGBScale(float n)
{
  rgbScale = (n > 0.5f ? (n < 8.0f ? n : 8.0f) : 0.5f);
}

void LandscapeTexture::setDefaultColor(std::uint32_t c)
{
  defaultColor = ((c >> 16) & 0x00FFU) | (c & 0xFF00U) | ((c & 0x00FFU) << 16);
}

void LandscapeTexture::renderTexture_NoNormals(
    unsigned char *outBuf, int renderScale, int x0, int y0, int x1, int y1)
{
  setRenderPixelFunction(false, false);
  FloatVector4  rgbScale_v(
      rgbScale * (vclrData16 ?
                  (1.0f / 187.67568f) : (vclrData24 ? (1.0f / 255.0f) : 1.0f)));
  int     m = (1 << renderScale) - 1;
  float   renderScale_f = 1.0f / float(1 << renderScale);
  FloatVector4  n[3];
  n[0] = FloatVector4(0.0f);
  n[1] = FloatVector4(0.0f);
  n[2] = FloatVector4(0.0f);
  for (int y = y0; y < y1; y++)
  {
    int     yc = (y > 0 ? y : 0) >> renderScale;
    int     yf = y & m;
    for (int x = x0; x < x1; x++, outBuf = outBuf + 3)
    {
      int     xc = (x > 0 ? x : 0) >> renderScale;
      FloatVector4  c(renderPixel(n, xc, yc, x, y));
      int     xf = x & m;
      if (xf)
      {
        FloatVector4  cTmp(renderPixel(n, xc + 1, yc, x, y));
        c = blendColors(c, cTmp, float(xf) * renderScale_f);
      }
      if (yf)
      {
        FloatVector4  c2(renderPixel(n, xc, yc + 1, x, y));
        if (xf)
        {
          FloatVector4  cTmp(renderPixel(n, xc + 1, yc + 1, x, y));
          c2 = blendColors(c2, cTmp, float(xf) * renderScale_f);
        }
        c = blendColors(c, c2, float(yf) * renderScale_f);
      }
      if (vclrData16)
        c *= getFO76VertexColor(x, y, renderScale);
      else if (vclrData24)
        c *= getTES4VertexColor(x, y, renderScale);
      std::uint32_t cTmp = std::uint32_t(c * rgbScale_v);
      outBuf[0] = (unsigned char) ((cTmp >> 16) & 0xFFU);       // B
      outBuf[1] = (unsigned char) ((cTmp >> 8) & 0xFFU);        // G
      outBuf[2] = (unsigned char) (cTmp & 0xFFU);               // R
    }
  }
}

void LandscapeTexture::renderTexture_NoPBR(
    unsigned char *outBuf, int renderScale, int x0, int y0, int x1, int y1,
    unsigned char *outBufN)
{
  setRenderPixelFunction(true, false);
  FloatVector4  rgbScale_v(
      rgbScale * (vclrData16 ?
                  (1.0f / 187.67568f) : (vclrData24 ? (1.0f / 255.0f) : 1.0f)));
  int     m = (1 << renderScale) - 1;
  float   renderScale_f = 1.0f / float(1 << renderScale);
  FloatVector4  nDefault(0.0f, 0.0f, 127.5f, 0.0f);
  FloatVector4  n[3], n2[3], nTmp[3];
  n[1] = FloatVector4(0.0f);
  n[2] = FloatVector4(0.0f);
  n2[1] = FloatVector4(0.0f);
  n2[2] = FloatVector4(0.0f);
  nTmp[1] = FloatVector4(0.0f);
  nTmp[2] = FloatVector4(0.0f);
  for (int y = y0; y < y1; y++)
  {
    int     yc = (y > 0 ? y : 0) >> renderScale;
    int     yf = y & m;
    for (int x = x0; x < x1; x++, outBuf = outBuf + 3)
    {
      int     xc = (x > 0 ? x : 0) >> renderScale;
      n[0] = nDefault;
      FloatVector4  c(renderPixel(n, xc, yc, x, y));
      int     xf = x & m;
      if (xf)
      {
        nTmp[0] = nDefault;
        FloatVector4  cTmp(renderPixel(nTmp, xc + 1, yc, x, y));
        n[0] = n[0] + ((nTmp[0] - n[0]) * (float(xf) * renderScale_f));
        c = blendColors(c, cTmp, float(xf) * renderScale_f);
      }
      if (yf)
      {
        n2[0] = nDefault;
        FloatVector4  c2(renderPixel(n2, xc, yc + 1, x, y));
        if (xf)
        {
          nTmp[0] = nDefault;
          FloatVector4  cTmp(renderPixel(nTmp, xc + 1, yc + 1, x, y));
          n2[0] = n2[0] + ((nTmp[0] - n2[0]) * (float(xf) * renderScale_f));
          c2 = blendColors(c2, cTmp, float(xf) * renderScale_f);
        }
        n[0] = n[0] + ((n2[0] - n[0]) * (float(yf) * renderScale_f));
        c = blendColors(c, c2, float(yf) * renderScale_f);
      }
      if (vclrData16)
        c *= getFO76VertexColor(x, y, renderScale);
      else if (vclrData24)
        c *= getTES4VertexColor(x, y, renderScale);
      std::uint32_t cTmp = std::uint32_t(c * rgbScale_v);
      outBuf[0] = (unsigned char) ((cTmp >> 16) & 0xFFU);       // B
      outBuf[1] = (unsigned char) ((cTmp >> 8) & 0xFFU);        // G
      outBuf[2] = (unsigned char) (cTmp & 0xFFU);               // R
      std::uint32_t n0;
      if (!isFO76)
        n0 = normalToColor(n[0]);
      else
        n0 = normalToColor(rotateNormalFO76(n[0]));
      outBufN[0] = (unsigned char) (n0 & 0xFFU);                // X
      outBufN[1] = (unsigned char) ((n0 >> 8) & 0xFFU);         // Y
      outBufN = outBufN + 2;
    }
  }
}

void LandscapeTexture::renderTexture(
    unsigned char *outBuf, int renderScale, int x0, int y0, int x1, int y1,
    unsigned char *outBufN, unsigned char *outBufS, unsigned char *outBufR)
{
  if (x0 > x1)
  {
    int     tmp = x0;
    x0 = x1;
    x1 = tmp;
  }
  if (y0 > y1)
  {
    int     tmp = y0;
    y0 = y1;
    y1 = tmp;
  }
  x0 = x0 << renderScale;
  y0 = y0 << renderScale;
  x1 = (x1 + 1) << renderScale;
  y1 = (y1 + 1) << renderScale;
  if (!outBufN)
  {
    renderTexture_NoNormals(outBuf, renderScale, x0, y0, x1, y1);
    return;
  }
  if (!outBufS)
  {
    renderTexture_NoPBR(outBuf, renderScale, x0, y0, x1, y1, outBufN);
    return;
  }
  setRenderPixelFunction(true, true);
  FloatVector4  rgbScale_v(
      rgbScale * (vclrData16 ?
                  (1.0f / 187.67568f) : (vclrData24 ? (1.0f / 255.0f) : 1.0f)));
  int     m = (1 << renderScale) - 1;
  float   renderScale_f = 1.0f / float(1 << renderScale);
  FloatVector4  nDefault(0.0f, 0.0f, 127.5f, 0.0f);
  FloatVector4  sDefault(64.0f);
  FloatVector4  rDefault(10.0f);
  for (int y = y0; y < y1; y++)
  {
    int     yc = (y > 0 ? y : 0) >> renderScale;
    int     yf = y & m;
    for (int x = x0; x < x1; x++, outBuf = outBuf + 3)
    {
      int     xc = (x > 0 ? x : 0) >> renderScale;
      FloatVector4  n[3];
      n[0] = nDefault;
      n[1] = sDefault;
      n[2] = rDefault;
      FloatVector4  c(renderPixel(n, xc, yc, x, y));
      int     xf = x & m;
      if (xf)
      {
        FloatVector4  nTmp[3];
        nTmp[0] = nDefault;
        nTmp[1] = sDefault;
        nTmp[2] = rDefault;
        FloatVector4  cTmp(renderPixel(nTmp, xc + 1, yc, x, y));
        blendNormals(n, nTmp, float(xf) * renderScale_f);
        c = blendColors(c, cTmp, float(xf) * renderScale_f);
      }
      if (yf)
      {
        FloatVector4  n2[3];
        n2[0] = nDefault;
        n2[1] = sDefault;
        n2[2] = rDefault;
        FloatVector4  c2(renderPixel(n2, xc, yc + 1, x, y));
        if (xf)
        {
          FloatVector4  nTmp[3];
          nTmp[0] = nDefault;
          nTmp[1] = sDefault;
          nTmp[2] = rDefault;
          FloatVector4  cTmp(renderPixel(nTmp, xc + 1, yc + 1, x, y));
          blendNormals(n2, nTmp, float(xf) * renderScale_f);
          c2 = blendColors(c2, cTmp, float(xf) * renderScale_f);
        }
        blendNormals(n, n2, float(yf) * renderScale_f);
        c = blendColors(c, c2, float(yf) * renderScale_f);
      }
      if (vclrData16)
        c *= getFO76VertexColor(x, y, renderScale);
      else if (vclrData24)
        c *= getTES4VertexColor(x, y, renderScale);
      std::uint32_t cTmp = std::uint32_t(c * rgbScale_v);
      outBuf[0] = (unsigned char) ((cTmp >> 16) & 0xFFU);       // B
      outBuf[1] = (unsigned char) ((cTmp >> 8) & 0xFFU);        // G
      outBuf[2] = (unsigned char) (cTmp & 0xFFU);               // R
      std::uint32_t n0;
      if (!isFO76)
        n0 = normalToColor(n[0]);
      else
        n0 = normalToColor(rotateNormalFO76(n[0]));
      outBufN[0] = (unsigned char) (n0 & 0xFFU);                // X
      outBufN[1] = (unsigned char) ((n0 >> 8) & 0xFFU);         // Y
      outBufN = outBufN + 2;
      std::uint32_t n1 = std::uint32_t(n[1]);
      // specular mask (FO4) or smoothness (FO76)
      outBufS[0] = (unsigned char) (n1 & 0xFFU);
      // smoothness (FO4) or ambient occlusion (FO76)
      outBufS[1] = (unsigned char) ((n1 >> 8) & 0xFFU);
      outBufS = outBufS + 2;
      if (outBufR)
      {
        std::uint32_t n2 = std::uint32_t(n[2]);
        outBufR[0] = (unsigned char) ((n2 >> 16) & 0xFFU);      // B
        outBufR[1] = (unsigned char) ((n2 >> 8) & 0xFFU);       // G
        outBufR[2] = (unsigned char) (n2 & 0xFFU);              // R
        outBufR = outBufR + 3;
      }
    }
  }
}

