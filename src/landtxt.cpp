
#include "common.hpp"
#include "landtxt.hpp"

inline FloatVector4 LandscapeTexture::rotateNormalFO76(FloatVector4 n)
{
  return FloatVector4((n[1] + n[0]) * 0.70710678f,
                      (n[1] - n[0]) * 0.70710678f, n[2], n[3]);
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

inline FloatVector4 LandscapeTexture::colorToNormal(FloatVector4 c)
{
  float   x = c[0] - 127.5f;
  float   y = c[1] - 127.5f;
  float   z = (127.5f * 127.5f) - ((x * x) + (y * y));
  return FloatVector4(x, y, FloatVector4::squareRootFast(z), 0.0f);
}

inline FloatVector4 LandscapeTexture::normalToColor(FloatVector4 n)
{
  FloatVector4  tmp(n);
  tmp.normalize3Fast();
  tmp += 1.0f;
  tmp *= 127.5f;
  return tmp;
}

inline unsigned int LandscapeTexture::getFO76VertexColor(size_t offs) const
{
  unsigned int  c = FileBuffer::readUInt16Fast(vclrData16 + (offs << 1));
  c = ((c >> 10) & 0x1FU) | ((c << 3) & 0x1F00U) | ((c & 0x1FU) << 16);
  return c;
}

inline unsigned int LandscapeTexture::getTES4VertexColor(size_t offs) const
{
  unsigned int  b = vclrData24[offs * 3];
  unsigned int  g = vclrData24[offs * 3 + 1];
  unsigned int  r = vclrData24[offs * 3 + 2];
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
  c0 *= (1.0f / 15.5f);
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
    const LandscapeTexture& p, FloatVector4& n,
    int x, int y, int txtX, int txtY)
{
  (void) n;
  x = (x < p.width ? x : (p.width - 1));
  y = (y < p.height ? y : (p.height - 1));
  size_t  offs = size_t(y) * size_t(p.width) + size_t(x);
  const unsigned char *p0 = p.txtSetData;
  p0 = p0 + (((size_t(y) >> p.txtSetMip) * (size_t(p.width) >> p.txtSetMip)
              + (size_t(x) >> p.txtSetMip)) << 4);
  unsigned int  a = FileBuffer::readUInt16Fast(p.ltexData16 + (offs << 1));
  a = (a << 3) | 7U;
  unsigned int  c = p.defaultColor;
  p0 = p0 + 6;
  for ( ; a; a = a << 3)
  {
    p0--;
    if (a < 0x8000U)
      continue;
    unsigned int  aTmp = ~a & 0x00038000U;
    a = a & 0x7FFFU;
    unsigned char t = *p0;
    if (t >= p.landTextureCnt || !p.landTextures[t])
      continue;
    unsigned int  cTmp =
        p.landTextures[t]->getPixelN(txtX - txtY, txtX + txtY, int(p.mipLevel));
    if (cTmp < ((aTmp * 0x4900U + 0x00800000U) & 0xFF000000U))
      continue;
    c = cTmp;
    break;
  }
  return FloatVector4(c);
}

FloatVector4 LandscapeTexture::renderPixelFO76I(
    const LandscapeTexture& p, FloatVector4& n,
    int x, int y, int txtX, int txtY)
{
  x = (x < p.width ? x : (p.width - 1));
  y = (y < p.height ? y : (p.height - 1));
  size_t  offs = size_t(y) * size_t(p.width) + size_t(x);
  const unsigned char *p0 = p.txtSetData;
  p0 = p0 + (((size_t(y) >> p.txtSetMip) * (size_t(p.width) >> p.txtSetMip)
              + (size_t(x) >> p.txtSetMip)) << 4);
  unsigned int  a = FileBuffer::readUInt16Fast(p.ltexData16 + (offs << 1));
  a = (a << 3) | 7U;
  unsigned int  c = p.defaultColor;
  p0 = p0 + 6;
  for ( ; a; a = a << 3)
  {
    p0--;
    if (a < 0x8000U)
      continue;
    unsigned int  aTmp = ~a & 0x00038000U;
    a = a & 0x7FFFU;
    unsigned char t = *p0;
    if (t >= p.landTextureCnt || !p.landTextures[t])
      continue;
    unsigned int  cTmp =
        p.landTextures[t]->getPixelN(txtX - txtY, txtX + txtY, int(p.mipLevel));
    if (cTmp < ((aTmp * 0x4900U + 0x00800000U) & 0xFF000000U))
      continue;
    c = cTmp;
    if (p.landTexturesN[t])
    {
      n = colorToNormal(FloatVector4(p.landTexturesN[t]->getPixelN(
                                         txtX - txtY, txtX + txtY,
                                         int(p.mipLevel))));
    }
    break;
  }
  return FloatVector4(c);
}

FloatVector4 LandscapeTexture::renderPixelFO76F(
    const LandscapeTexture& p, FloatVector4& n,
    int x, int y, int txtX, int txtY)
{
  x = (x < p.width ? x : (p.width - 1));
  y = (y < p.height ? y : (p.height - 1));
  size_t  offs = size_t(y) * size_t(p.width) + size_t(x);
  const unsigned char *p0 = p.txtSetData;
  p0 = p0 + (((size_t(y) >> p.txtSetMip) * (size_t(p.width) >> p.txtSetMip)
              + (size_t(x) >> p.txtSetMip)) << 4);
  unsigned int  a = FileBuffer::readUInt16Fast(p.ltexData16 + (offs << 1));
  a = (a << 3) | 7U;
  FloatVector4  c(p.defaultColor);
  p0 = p0 + 6;
  for ( ; a; a = a << 3)
  {
    p0--;
    if (a < 0x8000U)
      continue;
    unsigned int  aTmp = ~a & 0x00038000U;
    a = a & 0x7FFFU;
    unsigned char t = *p0;
    if (t >= p.landTextureCnt || !p.landTextures[t])
      continue;
    FloatVector4  cTmp(p.landTextures[t]->getPixelT_N(
                           float(txtX - txtY) * p.txtScale,
                           float(txtX + txtY) * p.txtScale, p.mipLevel));
    if (cTmp[3] < (float(int(aTmp)) * (255.5f / float(0x00038000))))
      continue;
    c = cTmp;
    if (p.landTexturesN && p.landTexturesN[t])
    {
      n = colorToNormal(p.landTexturesN[t]->getPixelT_N(
                            float(txtX - txtY) * p.txtScale,
                            float(txtX + txtY) * p.txtScale, p.mipLevel));
    }
    break;
  }
  return c;
}

FloatVector4 LandscapeTexture::renderPixelFO76I_GCVR(
    const LandscapeTexture& p, FloatVector4& n,
    int x, int y, int txtX, int txtY)
{
  x = (x < p.width ? x : (p.width - 1));
  y = (y < p.height ? y : (p.height - 1));
  size_t  offs = size_t(y) * size_t(p.width) + size_t(x);
  const unsigned char *p0 = p.txtSetData;
  p0 = p0 + (((size_t(y) >> p.txtSetMip) * (size_t(p.width) >> p.txtSetMip)
              + (size_t(x) >> p.txtSetMip)) << 4);
  unsigned int  a = FileBuffer::readUInt16Fast(p.ltexData16 + (offs << 1));
  a = (a << 3) | 7U;
  FloatVector4  c(p.defaultColor);
  p0 = p0 + 6;
  for ( ; a; a = a << 3)
  {
    p0--;
    if (a < 0x8000U)
      continue;
    unsigned int  aTmp = ~a & 0x00038000U;
    a = a & 0x7FFFU;
    unsigned char t = *p0;
    if (t >= p.landTextureCnt || !p.landTextures[t])
      continue;
    unsigned int  cTmp =
        p.landTextures[t]->getPixelN(txtX - txtY, txtX + txtY, int(p.mipLevel));
    if (cTmp < ((aTmp * 0x4900U + 0x00800000U) & 0xFF000000U))
      continue;
    c = FloatVector4(cTmp);
    if (p.landTexturesN && p.landTexturesN[t])
    {
      n = colorToNormal(FloatVector4(p.landTexturesN[t]->getPixelN(
                                         txtX - txtY, txtX + txtY,
                                         int(p.mipLevel))));
    }
    break;
  }
  p0 = p.txtSetData + ((size_t(p0 - p.txtSetData) & ~7UL) | 8UL);
  a = p.gcvrData[offs];
  for ( ; a; a = a >> 1, p0++)
  {
    if (!(a & 1))
      continue;
    unsigned char t = *p0;
    if (t >= p.landTextureCnt || !p.landTextures[t])
      continue;
    FloatVector4  cTmp(p.landTextures[t]->getPixelN(txtX - txtY, txtX + txtY,
                                                    int(p.mipLevel)));
    float   aTmp = cTmp[3] * (1.0f / 512.0f);
    c = blendColors(c, cTmp, aTmp);
    if (p.landTexturesN && p.landTexturesN[t])
    {
      FloatVector4  nTmp(colorToNormal(
                             FloatVector4(p.landTexturesN[t]->getPixelN(
                                              txtX - txtY, txtX + txtY,
                                              int(p.mipLevel)))));
      n = blendColors(n, nTmp, aTmp);
    }
  }
  return c;
}

FloatVector4 LandscapeTexture::renderPixelFO76F_GCVR(
    const LandscapeTexture& p, FloatVector4& n,
    int x, int y, int txtX, int txtY)
{
  x = (x < p.width ? x : (p.width - 1));
  y = (y < p.height ? y : (p.height - 1));
  size_t  offs = size_t(y) * size_t(p.width) + size_t(x);
  const unsigned char *p0 = p.txtSetData;
  p0 = p0 + (((size_t(y) >> p.txtSetMip) * (size_t(p.width) >> p.txtSetMip)
              + (size_t(x) >> p.txtSetMip)) << 4);
  unsigned int  a = FileBuffer::readUInt16Fast(p.ltexData16 + (offs << 1));
  a = (a << 3) | 7U;
  FloatVector4  c(p.defaultColor);
  p0 = p0 + 6;
  for ( ; a; a = a << 3)
  {
    p0--;
    if (a < 0x8000U)
      continue;
    unsigned int  aTmp = ~a & 0x00038000U;
    a = a & 0x7FFFU;
    unsigned char t = *p0;
    if (t >= p.landTextureCnt || !p.landTextures[t])
      continue;
    FloatVector4  cTmp(p.landTextures[t]->getPixelT_N(
                           float(txtX - txtY) * p.txtScale,
                           float(txtX + txtY) * p.txtScale, p.mipLevel));
    if (cTmp[3] < (float(int(aTmp)) * (255.5f / float(0x00038000))))
      continue;
    c = cTmp;
    if (p.landTexturesN && p.landTexturesN[t])
    {
      n = colorToNormal(p.landTexturesN[t]->getPixelT_N(
                            float(txtX - txtY) * p.txtScale,
                            float(txtX + txtY) * p.txtScale, p.mipLevel));
    }
    break;
  }
  p0 = p.txtSetData + ((size_t(p0 - p.txtSetData) & ~7UL) | 8UL);
  a = p.gcvrData[offs];
  for ( ; a; a = a >> 1, p0++)
  {
    if (!(a & 1))
      continue;
    unsigned char t = *p0;
    if (t >= p.landTextureCnt || !p.landTextures[t])
      continue;
    FloatVector4  cTmp(p.landTextures[t]->getPixelT_N(
                           float(txtX - txtY) * p.txtScale,
                           float(txtX + txtY) * p.txtScale, p.mipLevel));
    float   aTmp = cTmp[3] * (1.0f / 512.0f);
    c = blendColors(c, cTmp, aTmp);
    if (p.landTexturesN && p.landTexturesN[t])
    {
      FloatVector4  nTmp(colorToNormal(p.landTexturesN[t]->getPixelT_N(
                                           float(txtX - txtY) * p.txtScale,
                                           float(txtX + txtY) * p.txtScale,
                                           p.mipLevel)));
      n = blendColors(n, nTmp, aTmp);
    }
  }
  return c;
}

FloatVector4 LandscapeTexture::renderPixelTES4I_NoNormals(
    const LandscapeTexture& p, FloatVector4& n,
    int x, int y, int txtX, int txtY)
{
  (void) n;
  x = (x < p.width ? x : (p.width - 1));
  y = (y < p.height ? y : (p.height - 1));
  size_t  offs = size_t(y) * size_t(p.width) + size_t(x);
  const unsigned char *p0 = p.txtSetData;
  p0 = p0 + (((size_t(y) >> p.txtSetMip) * (size_t(p.width) >> p.txtSetMip)
              + (size_t(x) >> p.txtSetMip)) << 4);
  unsigned long long  a =
      FileBuffer::readUInt32Fast(p.ltexData32 + (offs << 2));
  a = (a << 4) | 15ULL;
  FloatVector4  c(p.defaultColor);
  unsigned char prvTexture = 0xFF;
  for ( ; a; a = a >> 4, p0++)
  {
    unsigned int  aTmp = (unsigned int) (a & 0x0FU);
    if (!aTmp)
      continue;
    unsigned char t = *p0;
    if (t >= p.landTextureCnt || t == prvTexture || !p.landTextures[t])
      continue;
    FloatVector4  cTmp(p.landTextures[t]->getPixelN(txtX, txtY,
                                                    int(p.mipLevel)));
    prvTexture = (aTmp == 15 ? t : (unsigned char) 0xFF);
    if (aTmp != 15)
      cTmp = blendColors(c, cTmp, float(int(aTmp)) * (1.0f / 15.0f));
    c = cTmp;
  }
  return c;
}

FloatVector4 LandscapeTexture::renderPixelTES4I(
    const LandscapeTexture& p, FloatVector4& n,
    int x, int y, int txtX, int txtY)
{
  x = (x < p.width ? x : (p.width - 1));
  y = (y < p.height ? y : (p.height - 1));
  size_t  offs = size_t(y) * size_t(p.width) + size_t(x);
  const unsigned char *p0 = p.txtSetData;
  p0 = p0 + (((size_t(y) >> p.txtSetMip) * (size_t(p.width) >> p.txtSetMip)
              + (size_t(x) >> p.txtSetMip)) << 4);
  unsigned long long  a =
      FileBuffer::readUInt32Fast(p.ltexData32 + (offs << 2));
  a = (a << 4) | 15ULL;
  FloatVector4  c(p.defaultColor);
  unsigned char prvTexture = 0xFF;
  for ( ; a; a = a >> 4, p0++)
  {
    unsigned int  aTmp = (unsigned int) (a & 0x0FU);
    if (!aTmp)
      continue;
    unsigned char t = *p0;
    if (t >= p.landTextureCnt || t == prvTexture || !p.landTextures[t])
      continue;
    FloatVector4  cTmp(p.landTextures[t]->getPixelN(txtX, txtY,
                                                    int(p.mipLevel)));
    float   aTmp_f = float(int(aTmp)) * (1.0f / 15.0f);
    if (p.landTexturesN[t])
    {
      FloatVector4  nTmp(colorToNormal(
                             FloatVector4(p.landTexturesN[t]->getPixelN(
                                              txtX, txtY, int(p.mipLevel)))));
      if (aTmp != 15)
        nTmp = blendColors(n, nTmp, aTmp_f);
      n = nTmp;
    }
    prvTexture = (aTmp == 15 ? t : (unsigned char) 0xFF);
    if (aTmp != 15)
      cTmp = blendColors(c, cTmp, aTmp_f);
    c = cTmp;
  }
  return c;
}

FloatVector4 LandscapeTexture::renderPixelTES4F(
    const LandscapeTexture& p, FloatVector4& n,
    int x, int y, int txtX, int txtY)
{
  x = (x < p.width ? x : (p.width - 1));
  y = (y < p.height ? y : (p.height - 1));
  size_t  offs = size_t(y) * size_t(p.width) + size_t(x);
  const unsigned char *p0 = p.txtSetData;
  p0 = p0 + (((size_t(y) >> p.txtSetMip) * (size_t(p.width) >> p.txtSetMip)
              + (size_t(x) >> p.txtSetMip)) << 4);
  unsigned long long  a =
      FileBuffer::readUInt32Fast(p.ltexData32 + (offs << 2));
  a = (a << 4) | 15ULL;
  FloatVector4  c(p.defaultColor);
  unsigned char prvTexture = 0xFF;
  for ( ; a; a = a >> 4, p0++)
  {
    unsigned int  aTmp = (unsigned int) (a & 0x0FU);
    if (!aTmp)
      continue;
    unsigned char t = *p0;
    if (t >= p.landTextureCnt || t == prvTexture || !p.landTextures[t])
      continue;
    FloatVector4  cTmp(p.landTextures[t]->getPixelT_N(float(txtX) * p.txtScale,
                                                      float(txtY) * p.txtScale,
                                                      p.mipLevel));
    float   aTmp_f = float(int(aTmp)) * (1.0f / 15.0f);
    if (p.landTexturesN && p.landTexturesN[t])
    {
      FloatVector4  nTmp(colorToNormal(p.landTexturesN[t]->getPixelT_N(
                                           float(txtX) * p.txtScale,
                                           float(txtY) * p.txtScale,
                                           p.mipLevel)));
      if (aTmp != 15)
        nTmp = blendColors(n, nTmp, aTmp_f);
      n = nTmp;
    }
    prvTexture = (aTmp == 15 ? t : (unsigned char) 0xFF);
    if (aTmp != 15)
      cTmp = blendColors(c, cTmp, aTmp_f);
    c = cTmp;
  }
  return c;
}

void LandscapeTexture::setRenderPixelFunction()
{
  if (isFO76)
  {
    if (!gcvrData)
    {
      if (integerMip)
      {
        if (!landTexturesN)
          renderPixelFunction = &renderPixelFO76I_NoNormals;
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
      if (!landTexturesN)
        renderPixelFunction = &renderPixelTES4I_NoNormals;
      else
        renderPixelFunction = &renderPixelTES4I;
    }
    else
    {
      renderPixelFunction = &renderPixelTES4F;
    }
  }
}

LandscapeTexture::LandscapeTexture(
    const unsigned char *txtSetPtr, const unsigned char *ltex32Ptr,
    const unsigned char *vclr24Ptr, const unsigned char *ltex16Ptr,
    const unsigned char *vclr16Ptr, const unsigned char *gcvrPtr,
    int vertexCntX, int vertexCntY, int cellResolution,
    const DDSTexture * const *landTxts, size_t landTxtCnt,
    const DDSTexture * const *landTxtsN)
  : txtSetData(txtSetPtr),
    ltexData32(ltex32Ptr),
    vclrData24(vclr24Ptr),
    ltexData16(ltex16Ptr),
    vclrData16(vclr16Ptr),
    gcvrData(gcvrPtr),
    renderPixelFunction(&renderPixelFO76I_NoNormals),
    landTextures(landTxts),
    landTexturesN(landTxtsN),
    landTextureCnt(landTxtCnt),
    mipLevel(0.0f),
    rgbScale(1.0f / 255.0f),
    width(vertexCntX),
    height(vertexCntY),
    txtScale(1.0f),
    integerMip(true),
    isFO76(ltex16Ptr != (unsigned char *) 0),
    txtSetMip(0),
    fo76VClrMip(0),
    defaultColor(0x003F3F3FU)
{
  while ((2 << txtSetMip) < cellResolution)
    txtSetMip++;
  while ((32 << fo76VClrMip) < cellResolution)
    fo76VClrMip++;
  setRenderPixelFunction();
}

LandscapeTexture::LandscapeTexture(
    const LandscapeData& landData,
    const DDSTexture * const *landTxts, size_t landTxtCnt,
    const DDSTexture * const *landTxtsN)
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
    landTextures(landTxts),
    landTexturesN(landTxtsN),
    landTextureCnt(landTxtCnt),
    mipLevel(0.0f),
    rgbScale(1.0f / 255.0f),
    width(landData.getImageWidth()),
    height(landData.getImageHeight()),
    txtScale(1.0f),
    integerMip(true),
    isFO76(ltexData16 != (unsigned char *) 0),
    txtSetMip(0),
    fo76VClrMip(0),
    defaultColor(0x003F3F3FU)
{
  int     cellResolution = landData.getCellResolution();
  while ((2 << txtSetMip) < cellResolution)
    txtSetMip++;
  while ((32 << fo76VClrMip) < cellResolution)
    fo76VClrMip++;
  setRenderPixelFunction();
}

void LandscapeTexture::setMipLevel(float n)
{
  mipLevel = n;
  integerMip = (float(int(mipLevel)) == mipLevel);
  txtScale = 1.0f;
  if (!integerMip)
    txtScale = float(std::pow(2.0, mipLevel - float(int(mipLevel))));
  setRenderPixelFunction();
}

void LandscapeTexture::setRGBScale(float n)
{
  rgbScale = (n > 0.5f ? (n < 8.0f ? n : 8.0f) : 0.5f) / 255.0f;
}

void LandscapeTexture::setDefaultColor(unsigned int c)
{
  defaultColor = ((c >> 16) & 0x00FFU) | (c & 0xFF00U) | ((c & 0x00FFU) << 16);
}

void LandscapeTexture::renderTexture(unsigned char *outBuf, int renderScale,
                                     int x0, int y0, int x1, int y1,
                                     unsigned char *outBufN) const
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
  int     m = (1 << renderScale) - 1;
  float   renderScale_f = 1.0f / float(1 << renderScale);
  for (int y = y0; y < y1; y++)
  {
    int     yc = (y > 0 ? y : 0) >> renderScale;
    int     yf = y & m;
    for (int x = x0; x < x1; x++, outBuf = outBuf + 3)
    {
      int     xc = (x > 0 ? x : 0) >> renderScale;
      FloatVector4  n(0.0f);
      FloatVector4  c(renderPixel(n, xc, yc, x, y));
      int     xf = x & m;
      if (xf)
      {
        FloatVector4  nTmp(0.0f);
        FloatVector4  cTmp(renderPixel(nTmp, xc + 1, yc, x, y));
        n = blendColors(n, nTmp, float(xf) * renderScale_f);
        c = blendColors(c, cTmp, float(xf) * renderScale_f);
      }
      if (yf)
      {
        FloatVector4  n2(0.0f);
        FloatVector4  c2(renderPixel(n2, xc, yc + 1, x, y));
        if (xf)
        {
          FloatVector4  nTmp(0.0f);
          FloatVector4  cTmp(renderPixel(nTmp, xc + 1, yc + 1, x, y));
          n2 = blendColors(n2, nTmp, float(xf) * renderScale_f);
          c2 = blendColors(c2, cTmp, float(xf) * renderScale_f);
        }
        n = blendColors(n, n2, float(yf) * renderScale_f);
        c = blendColors(c, c2, float(yf) * renderScale_f);
      }
      FloatVector4  vColor(255.0f);
      if (vclrData16)
        vColor = getFO76VertexColor(x, y, renderScale);
      else if (vclrData24)
        vColor = getTES4VertexColor(x, y, renderScale);
      c *= vColor;
      c *= rgbScale;
      unsigned int  cTmp = (unsigned int) c;
      outBuf[0] = (unsigned char) ((cTmp >> 16) & 0xFFU);       // B
      outBuf[1] = (unsigned char) ((cTmp >> 8) & 0xFFU);        // G
      outBuf[2] = (unsigned char) (cTmp & 0xFFU);               // R
      if (outBufN)
      {
        if (isFO76)
          n = rotateNormalFO76(n);
        unsigned int  nTmp = (unsigned int) normalToColor(n);
        outBufN[0] = (unsigned char) (nTmp & 0xFFU);            // X
        outBufN[1] = (unsigned char) ((nTmp >> 8) & 0xFFU);     // Y
        outBufN = outBufN + 2;
      }
    }
  }
}

