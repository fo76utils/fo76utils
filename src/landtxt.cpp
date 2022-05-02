
#include "common.hpp"
#include "landtxt.hpp"

const unsigned char LandscapeTexture::fo76VClrTable[32] =
{
  // round(172.8918 * pow(x / 31.0, 1.0 / 2.2))
    0,  36,  50,  60,  68,  75,  82,  88,
   93,  99, 103, 108, 112, 116, 120, 124,
  128, 132, 135, 138, 142, 145, 148, 151,
  154, 157, 160, 162, 165, 168, 170, 173
};

void LandscapeTexture::getFO76VertexColor(
    unsigned int& r, unsigned int& g, unsigned int& b, int x, int y) const
{
  y = y - ((1 << fo76VClrMip) - 1);
  y = (y >= 0 ? y : 0);
  x = x << (8 - fo76VClrMip);
  y = y << (8 - fo76VClrMip);
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
  unsigned int  c =
      rbga64ToRGBA32(blendRBGA64(blendRBGA64(
                                     getFO76VertexColor(offs0),
                                     getFO76VertexColor(offs1), xf),
                                 blendRBGA64(
                                     getFO76VertexColor(offs2),
                                     getFO76VertexColor(offs3), xf), yf));
  r = (r * (c & 0xFF) + 64) >> 7;
  g = (g * ((c >> 8) & 0xFF) + 64) >> 7;
  b = (b * ((c >> 16) & 0xFF) + 64) >> 7;
}

unsigned long long LandscapeTexture::renderPixelFO76(int x, int y,
                                                     int txtX, int txtY) const
{
  x = (x < width ? x : (width - 1));
  y = (y < height ? y : (height - 1));
  size_t  offs = size_t(y) * size_t(width) + size_t(x);
  const unsigned char *p0 =
      txtSetData + (((size_t(y) >> txtSetMip) * (size_t(width) >> txtSetMip)
                     + (size_t(x) >> txtSetMip)) << 4);
  unsigned int  a = FileBuffer::readUInt16Fast(ltexData16 + (offs << 1));
  a = (a << 3) | 7U;
  unsigned long long  c = defaultColorScaled;
  p0 = p0 + 6;
  if (!integerMip)
  {
    for ( ; a; a = a << 3)
    {
      p0--;
      if (a < 0x8000U)
        continue;
      unsigned int  aTmp = ~a & 0x00038000U;
      a = a & 0x7FFFU;
      unsigned char t = *p0;
      if (t >= landTextureCnt || !landTextures[t])
        continue;
      unsigned int  cTmp =
          landTextures[t]->getPixelT(float(txtX - txtY) * txtScale,
                                     float(txtX + txtY) * txtScale, mipLevel);
      if (cTmp < ((aTmp * 0x4900U + 0x00800000U) & 0xFF000000U))
        continue;
      c = cTmp;
      break;
    }
    c = rgb24ToRGB64(c);
    if (BRANCH_EXPECT(gcvrData, false))
    {
      p0 = txtSetData + ((size_t(p0 - txtSetData) & ~7UL) | 8UL);
      a = gcvrData[offs];
      for ( ; a; a = a >> 1, p0++)
      {
        if (!(a & 1))
          continue;
        unsigned char t = *p0;
        if (t >= landTextureCnt || !landTextures[t])
          continue;
        unsigned int  cTmp =
            landTextures[t]->getPixelT(float(txtX - txtY) * txtScale,
                                       float(txtX + txtY) * txtScale, mipLevel);
        c = blendRGB64(c, rgb24ToRGB64(cTmp), cTmp >> 25);
      }
    }
  }
  else
  {
    for ( ; a; a = a << 3)
    {
      p0--;
      if (a < 0x8000U)
        continue;
      unsigned int  aTmp = ~a & 0x00038000U;
      a = a & 0x7FFFU;
      unsigned char t = *p0;
      if (t >= landTextureCnt || !landTextures[t])
        continue;
      unsigned int  cTmp =
          landTextures[t]->getPixelN(txtX - txtY, txtX + txtY, int(mipLevel));
      if (cTmp < ((aTmp * 0x4900U + 0x00800000U) & 0xFF000000U))
        continue;
      c = cTmp;
      break;
    }
    c = rgb24ToRGB64(c);
    if (BRANCH_EXPECT(gcvrData, false))
    {
      p0 = txtSetData + ((size_t(p0 - txtSetData) & ~7UL) | 8UL);
      a = gcvrData[offs];
      for ( ; a; a = a >> 1, p0++)
      {
        if (!(a & 1))
          continue;
        unsigned char t = *p0;
        if (t >= landTextureCnt || !landTextures[t])
          continue;
        unsigned int  cTmp =
            landTextures[t]->getPixelN(txtX - txtY, txtX + txtY, int(mipLevel));
        c = blendRGB64(c, rgb24ToRGB64(cTmp), cTmp >> 25);
      }
    }
  }
  c = c * rgbScale;
  unsigned int  r = (unsigned int) (c & 0x000FFFFF);
  unsigned int  g = (unsigned int) ((c >> 20) & 0x000FFFFF);
  unsigned int  b = (unsigned int) ((c >> 40) & 0x000FFFFF);
  if (vclrData16)
    getFO76VertexColor(r, g, b, x, y);
  r = (r < 0xFF00 ? ((r + 128) >> 8) : 255U);
  g = (g < 0xFF00 ? ((g + 128) >> 8) : 255U);
  b = (b < 0xFF00 ? ((b + 128) >> 8) : 255U);
  return (((unsigned long long) g << 32) | (r | (b << 16)));
}

unsigned long long LandscapeTexture::renderPixelTES4(int x, int y,
                                                     int txtX, int txtY) const
{
  x = (x < width ? x : (width - 1));
  y = (y < height ? y : (height - 1));
  size_t  offs = size_t(y) * size_t(width) + size_t(x);
  const unsigned char *p0 =
      txtSetData + (((size_t(y) >> txtSetMip) * (size_t(width) >> txtSetMip)
                     + (size_t(x) >> txtSetMip)) << 4);
  unsigned long long  a = FileBuffer::readUInt32Fast(ltexData32 + (offs << 2));
  a = (a << 4) | 15ULL;
  unsigned long long  c = rgb24ToRGB64(defaultColorScaled);
  unsigned char prvTexture = 0xFF;
  if (!integerMip)
  {
    for ( ; a; a = a >> 4, p0++)
    {
      unsigned int  aTmp = (unsigned int) (a & 0x0FU);
      if (!aTmp)
        continue;
      unsigned char t = *p0;
      if (t >= landTextureCnt || t == prvTexture || !landTextures[t])
        continue;
      unsigned long long  cTmp =
          rgb24ToRGB64(landTextures[t]->getPixelT(float(txtX) * txtScale,
                                                  float(txtY) * txtScale,
                                                  mipLevel));
      prvTexture = (aTmp == 15 ? t : (unsigned char) 0xFF);
      if (aTmp != 15)
        cTmp = blendRGB64(c, cTmp, (aTmp * 273U + 8U) >> 4);
      c = cTmp;
    }
  }
  else
  {
    for ( ; a; a = a >> 4, p0++)
    {
      unsigned int  aTmp = (unsigned int) (a & 0x0FU);
      if (!aTmp)
        continue;
      unsigned char t = *p0;
      if (t >= landTextureCnt || t == prvTexture || !landTextures[t])
        continue;
      unsigned long long  cTmp =
          rgb24ToRGB64(landTextures[t]->getPixelN(txtX, txtY, int(mipLevel)));
      prvTexture = (aTmp == 15 ? t : (unsigned char) 0xFF);
      if (aTmp != 15)
        cTmp = blendRGB64(c, cTmp, (aTmp * 273U + 8U) >> 4);
      c = cTmp;
    }
  }
  c = c * rgbScale;
  unsigned int  r = (unsigned int) (c & 0x000FFFFF);
  unsigned int  g = (unsigned int) ((c >> 20) & 0x000FFFFF);
  unsigned int  b = (unsigned int) ((c >> 40) & 0x000FFFFF);
  if (vclrData24)
  {
    b = (b * vclrData24[offs * 3] + 127U) / 255U;
    g = (g * vclrData24[offs * 3 + 1] + 127U) / 255U;
    r = (r * vclrData24[offs * 3 + 2] + 127U) / 255U;
  }
  r = (r < 0xFF00 ? ((r + 128) >> 8) : 255U);
  g = (g < 0xFF00 ? ((g + 128) >> 8) : 255U);
  b = (b < 0xFF00 ? ((b + 128) >> 8) : 255U);
  return (((unsigned long long) g << 32) | (r | (b << 16)));
}

LandscapeTexture::LandscapeTexture(
    const unsigned char *txtSetPtr, const unsigned char *ltex32Ptr,
    const unsigned char *vclr24Ptr, const unsigned char *ltex16Ptr,
    const unsigned char *vclr16Ptr, const unsigned char *gcvrPtr,
    int vertexCntX, int vertexCntY, int cellResolution,
    const DDSTexture * const *landTxts, size_t landTxtCnt)
  : txtSetData(txtSetPtr),
    ltexData32(ltex32Ptr),
    vclrData24(vclr24Ptr),
    ltexData16(ltex16Ptr),
    vclrData16(vclr16Ptr),
    gcvrData(gcvrPtr),
    landTextures(landTxts),
    landTextureCnt(landTxtCnt),
    mipLevel(0.0f),
    rgbScale(256U),
    defaultColorScaled(0x003F3F3FU),
    width(vertexCntX),
    height(vertexCntY),
    txtScale(1.0f),
    integerMip(true),
    isFO76(ltex16Ptr != (unsigned char *) 0),
    txtSetMip(0),
    fo76VClrMip(0),
    defaultColor(defaultColorScaled)
{
  while ((2 << txtSetMip) < cellResolution)
    txtSetMip++;
  while ((32 << fo76VClrMip) < cellResolution)
    fo76VClrMip++;
}

LandscapeTexture::LandscapeTexture(
    const LandscapeData& landData,
    const DDSTexture * const *landTxts, size_t landTxtCnt)
  : txtSetData(landData.getCellTextureSets()),
    ltexData32(reinterpret_cast< const unsigned char * >(
                   landData.getLandTexture32())),
    vclrData24(landData.getVertexColor24()),
    ltexData16(reinterpret_cast< const unsigned char * >(
                   landData.getLandTexture16())),
    vclrData16(reinterpret_cast< const unsigned char * >(
                   landData.getVertexColor16())),
    gcvrData(landData.getGroundCover()),
    landTextures(landTxts),
    landTextureCnt(landTxtCnt),
    mipLevel(0.0f),
    rgbScale(256U),
    defaultColorScaled(0x003F3F3FU),
    width(landData.getImageWidth()),
    height(landData.getImageHeight()),
    txtScale(1.0f),
    integerMip(true),
    isFO76(ltexData16 != (unsigned char *) 0),
    txtSetMip(0),
    fo76VClrMip(0),
    defaultColor(defaultColorScaled)
{
  int     cellResolution = landData.getCellResolution();
  while ((2 << txtSetMip) < cellResolution)
    txtSetMip++;
  while ((32 << fo76VClrMip) < cellResolution)
    fo76VClrMip++;
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
  int     tmp = roundFloat(n * 256.0f);
  rgbScale = (unsigned int) (tmp > 128 ? (tmp < 2048 ? tmp : 2048) : 128);
  setDefaultColor(defaultColor);
}

void LandscapeTexture::setDefaultColor(unsigned int c)
{
  defaultColor = c & 0x00FFFFFFU;
  unsigned int  r = (((c >> 8) & 0xFF00U) + (rgbScale >> 1)) / rgbScale;
  unsigned int  g = ((c & 0xFF00U) + (rgbScale >> 1)) / rgbScale;
  unsigned int  b = (((c << 8) & 0xFF00U) + (rgbScale >> 1)) / rgbScale;
  defaultColorScaled = r | (g << 8) | (b << 16);
}

void LandscapeTexture::renderTexture(unsigned char *outBuf, int renderScale,
                                     int x0, int y0, int x1, int y1) const
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
    int     yf = (y << (8 - renderScale)) & 0xFF;
    if (!yf)
    {
      for (int x = x0; x < x1; x++, outBuf = outBuf + 3)
      {
        int     xc = (x > 0 ? x : 0) >> renderScale;
        unsigned long long  c = renderPixel(xc, yc, x, y);
        int     xf = (x << (8 - renderScale)) & 0xFF;
        if (xf)
          c = blendRBGA64(c, renderPixel(xc + 1, yc, x, y), xf);
        outBuf[0] = (unsigned char) ((c >> 16) & 0xFFU);        // B
        outBuf[1] = (unsigned char) ((c >> 32) & 0xFFU);        // G
        outBuf[2] = (unsigned char) (c & 0xFFU);                // R
      }
    }
    else
    {
      for (int x = x0; x < x1; x++, outBuf = outBuf + 3)
      {
        int     xc = (x > 0 ? x : 0) >> renderScale;
        unsigned long long  c = renderPixel(xc, yc, x, y);
        unsigned long long  c2 = renderPixel(xc, yc + 1, x, y);
        int     xf = (x << (8 - renderScale)) & 0xFF;
        if (xf)
        {
          c = blendRBGA64(c, renderPixel(xc + 1, yc, x, y), xf);
          c2 = blendRBGA64(c2, renderPixel(xc + 1, yc + 1, x, y), xf);
        }
        c = blendRBGA64(c, c2, yf);
        outBuf[0] = (unsigned char) ((c >> 16) & 0xFFU);        // B
        outBuf[1] = (unsigned char) ((c >> 32) & 0xFFU);        // G
        outBuf[2] = (unsigned char) (c & 0xFFU);                // R
      }
    }
  }
}

