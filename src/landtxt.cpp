
#include "common.hpp"
#include "landtxt.hpp"

const unsigned short LandscapeTexture::fo76VClrTable[32] =
{
  // round(2048.0 * pow(x / 15.5, 1.0 / 2.2))
     0,  589,  807,  971, 1106, 1225, 1330, 1427,
  1516, 1600, 1678, 1752, 1823, 1891, 1955, 2018,
  2078, 2136, 2192, 2247, 2300, 2351, 2401, 2450,
  2498, 2545, 2591, 2636, 2680, 2723, 2765, 2806
};

inline void LandscapeTexture::rotateNormalFO76(unsigned int& n)
{
  int     tmpX = int(n & 0xFFU) * 46341;        // 65536 * sqrt(0.5)
  int     tmpY = int((n >> 16) & 0xFFU) * 46341;
  int     normalX = (tmpX + tmpY) - 3428347;    // 8388608 - (127.5 * 46341 * 2)
  int     normalY = (tmpX - tmpY) + 8388608;
  normalX = (normalX > 0 ? (normalX < 0x00FF0000 ? (normalX >> 16) : 255) : 0);
  normalY = (normalY > 0 ? (normalY < 0x00FF0000 ? (normalY >> 16) : 255) : 0);
  n = (unsigned int) normalX | ((unsigned int) normalY << 16);
}

inline void LandscapeTexture::rotateNormalTES4(unsigned int& n)
{
  n = n ^ 0x00FF0000U;
}

inline unsigned long long LandscapeTexture::getFO76VertexColor(
    size_t offs) const
{
  unsigned int  c = FileBuffer::readUInt16Fast(vclrData16 + (offs << 1));
  unsigned int  r = fo76VClrTable[(c >> 10) & 0x1F];
  unsigned int  b = fo76VClrTable[c & 0x1F];
  return ((unsigned long long) (r | (b << 20))
          | ((unsigned long long) fo76VClrTable[(c >> 5) & 0x1F] << 40));
}

inline unsigned long long LandscapeTexture::getTES4VertexColor(
    size_t offs) const
{
  unsigned int  b = vclrData24[offs * 3];
  unsigned int  g = vclrData24[offs * 3 + 1];
  unsigned int  r = vclrData24[offs * 3 + 2];
  return (((unsigned long long) (r | (b << 20))
           | ((unsigned long long) g << 40)) * 13U);
}

inline void LandscapeTexture::getFO76VertexColor(
    unsigned int& r, unsigned int& g, unsigned int& b,
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
  unsigned long long  c =
      blendRBG64(blendRBG64(getFO76VertexColor(offs0),
                            getFO76VertexColor(offs1), xf),
                 blendRBG64(getFO76VertexColor(offs2),
                            getFO76VertexColor(offs3), xf), yf);
  r = (unsigned int) (c & 0x0FFFU);
  g = (unsigned int) ((c >> 40) & 0x0FFFU);
  b = (unsigned int) ((c >> 20) & 0x0FFFU);
}

inline void LandscapeTexture::getTES4VertexColor(
    unsigned int& r, unsigned int& g, unsigned int& b,
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
  unsigned long long  c =
      blendRBG64(blendRBG64(getTES4VertexColor(offs0),
                            getTES4VertexColor(offs1), xf),
                 blendRBG64(getTES4VertexColor(offs2),
                            getTES4VertexColor(offs3), xf), yf);
  r = ((unsigned int) (c & 0x0FFFU) * 5061U + 4096U) >> 13;
  g = ((unsigned int) ((c >> 40) & 0x0FFFU) * 5061U + 4096U) >> 13;
  b = ((unsigned int) ((c >> 20) & 0x0FFFU) * 5061U + 4096U) >> 13;
}

unsigned long long LandscapeTexture::renderPixelFO76I_NoNormals(
    const LandscapeTexture& p, unsigned int& n,
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
  unsigned long long  c = p.defaultColor;
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
  c = rgb24ToRBG64((unsigned int) c) << 4;
  return c;
}

unsigned long long LandscapeTexture::renderPixelFO76I(
    const LandscapeTexture& p, unsigned int& n,
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
  unsigned long long  c = p.defaultColor;
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
      unsigned int  nTmp =
          p.landTexturesN[t]->getPixelN(txtX - txtY, txtX + txtY,
                                        int(p.mipLevel));
      n = (nTmp & 0x00FFU) | ((nTmp & 0xFF00U) << 8);
    }
    break;
  }
  c = rgb24ToRBG64((unsigned int) c) << 4;
  return c;
}

unsigned long long LandscapeTexture::renderPixelFO76F(
    const LandscapeTexture& p, unsigned int& n,
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
  unsigned long long  c = p.defaultColor;
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
        p.landTextures[t]->getPixelT(float(txtX - txtY) * p.txtScale,
                                     float(txtX + txtY) * p.txtScale,
                                     p.mipLevel);
    if (cTmp < ((aTmp * 0x4900U + 0x00800000U) & 0xFF000000U))
      continue;
    c = cTmp;
    if (p.landTexturesN && p.landTexturesN[t])
    {
      unsigned int  nTmp =
          p.landTexturesN[t]->getPixelT(float(txtX - txtY) * p.txtScale,
                                        float(txtX + txtY) * p.txtScale,
                                        p.mipLevel);
      n = (nTmp & 0x00FFU) | ((nTmp & 0xFF00U) << 8);
    }
    break;
  }
  c = rgb24ToRBG64((unsigned int) c) << 4;
  return c;
}

unsigned long long LandscapeTexture::renderPixelFO76I_GCVR(
    const LandscapeTexture& p, unsigned int& n,
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
  unsigned long long  c = p.defaultColor;
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
    if (p.landTexturesN && p.landTexturesN[t])
    {
      unsigned int  nTmp =
          p.landTexturesN[t]->getPixelN(txtX - txtY, txtX + txtY,
                                        int(p.mipLevel));
      n = (nTmp & 0x00FFU) | ((nTmp & 0xFF00U) << 8);
    }
    break;
  }
  c = rgb24ToRBG64((unsigned int) c) << 4;
  p0 = p.txtSetData + ((size_t(p0 - p.txtSetData) & ~7UL) | 8UL);
  a = p.gcvrData[offs];
  for ( ; a; a = a >> 1, p0++)
  {
    if (!(a & 1))
      continue;
    unsigned char t = *p0;
    if (t >= p.landTextureCnt || !p.landTextures[t])
      continue;
    unsigned int  cTmp =
        p.landTextures[t]->getPixelN(txtX - txtY, txtX + txtY, int(p.mipLevel));
    c = blendRBG64(c, rgb24ToRBG64(cTmp) << 4, cTmp >> 25);
    if (p.landTexturesN && p.landTexturesN[t])
    {
      unsigned int  nTmp =
          p.landTexturesN[t]->getPixelN(txtX - txtY, txtX + txtY,
                                        int(p.mipLevel));
      nTmp = (nTmp & 0x00FFU) | ((nTmp & 0xFF00U) << 8);
      n = blendNormals(n, nTmp, cTmp >> 25);
    }
  }
  return c;
}

unsigned long long LandscapeTexture::renderPixelFO76F_GCVR(
    const LandscapeTexture& p, unsigned int& n,
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
  unsigned long long  c = p.defaultColor;
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
        p.landTextures[t]->getPixelT(float(txtX - txtY) * p.txtScale,
                                     float(txtX + txtY) * p.txtScale,
                                     p.mipLevel);
    if (cTmp < ((aTmp * 0x4900U + 0x00800000U) & 0xFF000000U))
      continue;
    c = cTmp;
    if (p.landTexturesN && p.landTexturesN[t])
    {
      unsigned int  nTmp =
          p.landTexturesN[t]->getPixelT(float(txtX - txtY) * p.txtScale,
                                        float(txtX + txtY) * p.txtScale,
                                        p.mipLevel);
      n = (nTmp & 0x00FFU) | ((nTmp & 0xFF00U) << 8);
    }
    break;
  }
  c = rgb24ToRBG64((unsigned int) c) << 4;
  p0 = p.txtSetData + ((size_t(p0 - p.txtSetData) & ~7UL) | 8UL);
  a = p.gcvrData[offs];
  for ( ; a; a = a >> 1, p0++)
  {
    if (!(a & 1))
      continue;
    unsigned char t = *p0;
    if (t >= p.landTextureCnt || !p.landTextures[t])
      continue;
    unsigned int  cTmp =
        p.landTextures[t]->getPixelT(float(txtX - txtY) * p.txtScale,
                                     float(txtX + txtY) * p.txtScale,
                                     p.mipLevel);
    c = blendRBG64(c, rgb24ToRBG64(cTmp) << 4, cTmp >> 25);
    if (p.landTexturesN && p.landTexturesN[t])
    {
      unsigned int  nTmp =
          p.landTexturesN[t]->getPixelT(float(txtX - txtY) * p.txtScale,
                                        float(txtX + txtY) * p.txtScale,
                                        p.mipLevel);
      nTmp = (nTmp & 0x00FFU) | ((nTmp & 0xFF00U) << 8);
      n = blendNormals(n, nTmp, cTmp >> 25);
    }
  }
  return c;
}

unsigned long long LandscapeTexture::renderPixelTES4I_NoNormals(
    const LandscapeTexture& p, unsigned int& n,
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
  unsigned long long  c = rgb24ToRBG64(p.defaultColor) << 4;
  unsigned char prvTexture = 0xFF;
  for ( ; a; a = a >> 4, p0++)
  {
    unsigned int  aTmp = (unsigned int) (a & 0x0FU);
    if (!aTmp)
      continue;
    unsigned char t = *p0;
    if (t >= p.landTextureCnt || t == prvTexture || !p.landTextures[t])
      continue;
    unsigned long long  cTmp =
        rgb24ToRBG64(p.landTextures[t]->getPixelN(txtX, txtY,
                                                  int(p.mipLevel))) << 4;
    prvTexture = (aTmp == 15 ? t : (unsigned char) 0xFF);
    if (aTmp != 15)
      cTmp = blendRBG64(c, cTmp, (aTmp * 273U + 8U) >> 4);
    c = cTmp;
  }
  return c;
}

unsigned long long LandscapeTexture::renderPixelTES4I(
    const LandscapeTexture& p, unsigned int& n,
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
  unsigned long long  c = rgb24ToRBG64(p.defaultColor) << 4;
  unsigned char prvTexture = 0xFF;
  for ( ; a; a = a >> 4, p0++)
  {
    unsigned int  aTmp = (unsigned int) (a & 0x0FU);
    if (!aTmp)
      continue;
    unsigned char t = *p0;
    if (t >= p.landTextureCnt || t == prvTexture || !p.landTextures[t])
      continue;
    unsigned long long  cTmp =
        rgb24ToRBG64(p.landTextures[t]->getPixelN(txtX, txtY,
                                                  int(p.mipLevel))) << 4;
    if (p.landTexturesN[t])
    {
      unsigned int  nTmp =
          p.landTexturesN[t]->getPixelN(txtX, txtY, int(p.mipLevel));
      nTmp = (nTmp & 0x00FFU) | ((nTmp & 0xFF00U) << 8);
      if (aTmp == 15)
        n = nTmp;
      else
        n = blendNormals(n, nTmp, (aTmp * 273U + 8U) >> 4);
    }
    prvTexture = (aTmp == 15 ? t : (unsigned char) 0xFF);
    if (aTmp != 15)
      cTmp = blendRBG64(c, cTmp, (aTmp * 273U + 8U) >> 4);
    c = cTmp;
  }
  return c;
}

unsigned long long LandscapeTexture::renderPixelTES4F(
    const LandscapeTexture& p, unsigned int& n,
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
  unsigned long long  c = rgb24ToRBG64(p.defaultColor) << 4;
  unsigned char prvTexture = 0xFF;
  for ( ; a; a = a >> 4, p0++)
  {
    unsigned int  aTmp = (unsigned int) (a & 0x0FU);
    if (!aTmp)
      continue;
    unsigned char t = *p0;
    if (t >= p.landTextureCnt || t == prvTexture || !p.landTextures[t])
      continue;
    unsigned long long  cTmp =
        rgb24ToRBG64(p.landTextures[t]->getPixelT(float(txtX) * p.txtScale,
                                                  float(txtY) * p.txtScale,
                                                  p.mipLevel)) << 4;
    if (p.landTexturesN && p.landTexturesN[t])
    {
      unsigned int  nTmp =
          p.landTexturesN[t]->getPixelT(float(txtX) * p.txtScale,
                                        float(txtY) * p.txtScale, p.mipLevel);
      nTmp = (nTmp & 0x00FFU) | ((nTmp & 0xFF00U) << 8);
      if (aTmp == 15)
        n = nTmp;
      else
        n = blendNormals(n, nTmp, (aTmp * 273U + 8U) >> 4);
    }
    prvTexture = (aTmp == 15 ? t : (unsigned char) 0xFF);
    if (aTmp != 15)
      cTmp = blendRBG64(c, cTmp, (aTmp * 273U + 8U) >> 4);
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
    rgbScale(256U),
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
    rgbScale(256U),
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
  int     tmp = roundFloat(n * 256.0f);
  rgbScale = (unsigned int) (tmp > 128 ? (tmp < 2048 ? tmp : 2048) : 128);
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
  for (int y = y0; y < y1; y++)
  {
    int     yc = (y > 0 ? y : 0) >> renderScale;
    unsigned int  yf = (unsigned int) ((y << (8 - renderScale)) & 0xFF);
    for (int x = x0; x < x1; x++, outBuf = outBuf + 3)
    {
      int     xc = (x > 0 ? x : 0) >> renderScale;
      unsigned int  n = 0x00800080U;
      unsigned long long  c = renderPixel(n, xc, yc, x, y);
      unsigned int  xf = (unsigned int) ((x << (8 - renderScale)) & 0xFF);
      if (xf)
      {
        unsigned int  nTmp = 0x00800080U;
        c = blendRBG64(c, renderPixel(nTmp, xc + 1, yc, x, y), xf);
        n = blendNormals(n, nTmp, xf);
      }
      if (yf)
      {
        unsigned int  n2 = 0x00800080U;
        unsigned long long  c2 = renderPixel(n2, xc, yc + 1, x, y);
        if (xf)
        {
          unsigned int  nTmp = 0x00800080U;
          c2 = blendRBG64(c2, renderPixel(nTmp, xc + 1, yc + 1, x, y), xf);
          n2 = blendNormals(n2, nTmp, xf);
        }
        c = blendRBG64(c, c2, yf);
        n = blendNormals(n, n2, yf);
      }
      unsigned int  r = 2048;
      unsigned int  g = 2048;
      unsigned int  b = 2048;
      if (vclrData16)
        getFO76VertexColor(r, g, b, x, y, renderScale);
      else if (vclrData24)
        getTES4VertexColor(r, g, b, x, y, renderScale);
      r = ((r * rgbScale + 4U) >> 3) * (unsigned int) (c & 0x0FFFU);
      g = ((g * rgbScale + 4U) >> 3) * (unsigned int) ((c >> 40) & 0x0FFFU);
      b = ((b * rgbScale + 4U) >> 3) * (unsigned int) ((c >> 20) & 0x0FFFU);
      outBuf[0] =
          (unsigned char) (b < 0x0FF00000U ? ((b + 0x00080000U) >> 20) : 255U);
      outBuf[1] =
          (unsigned char) (g < 0x0FF00000U ? ((g + 0x00080000U) >> 20) : 255U);
      outBuf[2] =
          (unsigned char) (r < 0x0FF00000U ? ((r + 0x00080000U) >> 20) : 255U);
      if (outBufN)
      {
        if (isFO76)
          rotateNormalFO76(n);
        else
          rotateNormalTES4(n);
        outBufN[0] = (unsigned char) (n & 0xFFU);               // X
        outBufN[1] = (unsigned char) ((n >> 16) & 0xFFU);       // Y
        outBufN = outBufN + 2;
      }
    }
  }
}

