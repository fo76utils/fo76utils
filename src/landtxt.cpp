
#include "common.hpp"
#include "landtxt.hpp"

static const DDSTexture16 defaultLandTxt_N(
    FloatVector4(0.0f, 0.0f, 1.0f, 1.0f));
static const DDSTexture16 defaultLandTxt_R(
    FloatVector4(0.749f, 0.749f, 0.749f, 1.0f));
static const DDSTexture16 defaultLandTxt_M(
    FloatVector4(0.0f, 0.0f, 0.0f, 1.0f));
static const DDSTexture16 defaultLandTxt_AO(FloatVector4(1.0f));

inline FloatVector4 LandscapeTexture::rotateNormalSF(const FloatVector4& n)
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
  n2[0] *= f;
  n2[1] *= f;
  n[0] += n2[0];
  n[1] += n2[1];
}

inline FloatVector4 LandscapeTexture::getNormal(
    size_t t, int x, int y, int m) const
{
  FloatVector4  tmp(FloatVector4::convertFloat16(
                        landTextures[t][1]->getPixelN(x, y, m)));
  tmp[2] = FloatVector4::squareRootFast(1.0f - tmp.dotProduct2(tmp));
  return tmp;
}

inline void LandscapeTexture::getPBRMapsI(
    FloatVector4 *n, size_t t, int x, int y, int m) const
{
  n[0] = getNormal(t, x, y, m);
  std::uint64_t tmp = landTextures[t][3]->getPixelN(x, y, m) & 0xFFFFU;
  tmp = tmp | ((landTextures[t][4]->getPixelN(x, y, m) & 0xFFFFU) << 16);
  tmp = tmp | ((landTextures[t][5]->getPixelN(x, y, m) & 0xFFFFU) << 32);
  n[1] = FloatVector4::convertFloat16(tmp);
}

inline void LandscapeTexture::getPBRMapsF(
    FloatVector4 *n, size_t t, float x, float y, float m) const
{
  FloatVector4  tmp(landTextures[t][1]->getPixelT(x, y, m));
  tmp[2] = FloatVector4::squareRootFast(1.0f - tmp.dotProduct2(tmp));
  n[0] = tmp;
  FloatVector4  tmpR(landTextures[t][3]->getPixelT(x, y, m));
  FloatVector4  tmpM(landTextures[t][4]->getPixelT(x, y, m));
  FloatVector4  tmpAO(landTextures[t][5]->getPixelT(x, y, m));
  n[1] = FloatVector4(tmpR[0], tmpM[0], tmpAO[0], 0.0f);
}

inline std::uint32_t LandscapeTexture::normalToColor(FloatVector4 n)
{
  n.normalize3Fast();
  n += 1.0f;
  n *= 127.5f;
  return std::uint32_t(n);
}

static const std::uint16_t  alphaThresholdTable[8] =
{
  0x0000, 0x3092, 0x3492, 0x36DB, 0x3892, 0x39B7, 0x3ADB, 0x3C00
};

FloatVector4 LandscapeTexture::renderPixelSF_I_NoNormals(
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
  std::uint32_t a =
      (std::uint32_t(std::int16_t(FileBuffer::readUInt16Fast(
                                      p.ltexData16 + (offs << 1))))
       & 0x0003FFFFU) | 0x001C0000U;
  std::uint64_t c = p.defaultColor;
  for ( ; a; a = a >> 3, p0++)
  {
    if (!(a & 7U))
      continue;
    std::uint32_t aTmp = (a & 7U) ^ 7U;
    unsigned char t = *p0;
    if (t >= p.landTextureCnt || !p.landTextures[t][0])
      continue;
    std::uint64_t cTmp =
        p.landTextures[t][0]->getPixelN(txtX - txtY, txtX + txtY,
                                        int(p.mipLevel));
    if (aTmp && p.landTextures[t][2])
    {
      if ((p.landTextures[t][2]->getPixelN(txtX - txtY, txtX + txtY,
                                           int(p.mipLevel)) & 0xFFFFU)
          < alphaThresholdTable[aTmp])
      {
        continue;
      }
    }
    c = cTmp;
    break;
  }
  return FloatVector4::convertFloat16(c);
}

FloatVector4 LandscapeTexture::renderPixelSF_I_NoPBR(
    const LandscapeTexture& p, FloatVector4 *n,
    int x, int y, int txtX, int txtY)
{
  x = (x < p.width ? x : (p.width - 1));
  y = (y < p.height ? y : (p.height - 1));
  size_t  offs = size_t(y) * size_t(p.width) + size_t(x);
  const unsigned char *p0 = p.txtSetData;
  p0 = p0 + (((size_t(y) >> p.txtSetMip) * (size_t(p.width) >> p.txtSetMip)
              + (size_t(x) >> p.txtSetMip)) << 4);
  std::uint32_t a =
      (std::uint32_t(std::int16_t(FileBuffer::readUInt16Fast(
                                      p.ltexData16 + (offs << 1))))
       & 0x0003FFFFU) | 0x001C0000U;
  std::uint64_t c = p.defaultColor;
  for ( ; a; a = a >> 3, p0++)
  {
    if (!(a & 7U))
      continue;
    std::uint32_t aTmp = (a & 7U) ^ 7U;
    unsigned char t = *p0;
    if (t >= p.landTextureCnt || !p.landTextures[t][0])
      continue;
    std::uint64_t cTmp =
        p.landTextures[t][0]->getPixelN(txtX - txtY, txtX + txtY,
                                        int(p.mipLevel));
    if (aTmp && p.landTextures[t][2])
    {
      if ((p.landTextures[t][2]->getPixelN(txtX - txtY, txtX + txtY,
                                           int(p.mipLevel)) & 0xFFFFU)
          < alphaThresholdTable[aTmp])
      {
        continue;
      }
    }
    c = cTmp;
    n[0] = p.getNormal(t, txtX - txtY, txtX + txtY, int(p.mipLevel));
    break;
  }
  return FloatVector4::convertFloat16(c);
}

FloatVector4 LandscapeTexture::renderPixelSF_I(
    const LandscapeTexture& p, FloatVector4 *n,
    int x, int y, int txtX, int txtY)
{
  x = (x < p.width ? x : (p.width - 1));
  y = (y < p.height ? y : (p.height - 1));
  size_t  offs = size_t(y) * size_t(p.width) + size_t(x);
  const unsigned char *p0 = p.txtSetData;
  p0 = p0 + (((size_t(y) >> p.txtSetMip) * (size_t(p.width) >> p.txtSetMip)
              + (size_t(x) >> p.txtSetMip)) << 4);
  std::uint32_t a =
      (std::uint32_t(std::int16_t(FileBuffer::readUInt16Fast(
                                      p.ltexData16 + (offs << 1))))
       & 0x0003FFFFU) | 0x001C0000U;
  std::uint64_t c = p.defaultColor;
  for ( ; a; a = a >> 3, p0++)
  {
    if (!(a & 7U))
      continue;
    std::uint32_t aTmp = (a & 7U) ^ 7U;
    unsigned char t = *p0;
    if (t >= p.landTextureCnt || !p.landTextures[t][0])
      continue;
    std::uint64_t cTmp =
        p.landTextures[t][0]->getPixelN(txtX - txtY, txtX + txtY,
                                        int(p.mipLevel));
    if (aTmp && p.landTextures[t][2])
    {
      if ((p.landTextures[t][2]->getPixelN(txtX - txtY, txtX + txtY,
                                           int(p.mipLevel)) & 0xFFFFU)
          < alphaThresholdTable[aTmp])
      {
        continue;
      }
    }
    c = cTmp;
    p.getPBRMapsI(n, t, txtX - txtY, txtX + txtY, int(p.mipLevel));
    break;
  }
  return FloatVector4::convertFloat16(c);
}

FloatVector4 LandscapeTexture::renderPixelSF_F(
    const LandscapeTexture& p, FloatVector4 *n,
    int x, int y, int txtX, int txtY)
{
  x = (x < p.width ? x : (p.width - 1));
  y = (y < p.height ? y : (p.height - 1));
  size_t  offs = size_t(y) * size_t(p.width) + size_t(x);
  const unsigned char *p0 = p.txtSetData;
  p0 = p0 + (((size_t(y) >> p.txtSetMip) * (size_t(p.width) >> p.txtSetMip)
              + (size_t(x) >> p.txtSetMip)) << 4);
  std::uint32_t a =
      (std::uint32_t(std::int16_t(FileBuffer::readUInt16Fast(
                                      p.ltexData16 + (offs << 1))))
       & 0x0003FFFFU) | 0x001C0000U;
  FloatVector4  c(FloatVector4::convertFloat16(p.defaultColor));
  for ( ; a; a = a >> 3, p0++)
  {
    if (!(a & 7U))
      continue;
    std::uint32_t aTmp = (a & 7U) ^ 7U;
    unsigned char t = *p0;
    if (t >= p.landTextureCnt || !p.landTextures[t][0])
      continue;
    FloatVector4  txtCoord(float(txtX - txtY), float(txtX + txtY), 0.0f, 0.0f);
    txtCoord *= p.txtScale;
    float   w = float(p.landTextures[t][0]->getWidth());
    float   h = float(p.landTextures[t][0]->getHeight());
    txtCoord /= FloatVector4(w, h, w, w);
    FloatVector4  cTmp(p.landTextures[t][0]->getPixelT(
                           txtCoord[0], txtCoord[1], p.mipLevel));
    if (aTmp && p.landTextures[t][2])
    {
      if (p.landTextures[t][2]->getPixelT(
              txtCoord[0], txtCoord[1], p.mipLevel)[0]
          < (float(int(aTmp)) * (1.0f / 7.0f)))
      {
        continue;
      }
    }
    c = cTmp;
    p.getPBRMapsF(n, t, txtCoord[0], txtCoord[1], p.mipLevel);
    break;
  }
  return c;
}

void LandscapeTexture::setRenderPixelFunction(
    bool haveNormalMaps, bool havePBRMaps)
{
  if (integerMip)
  {
    if (!haveNormalMaps)
      renderPixelFunction = &renderPixelSF_I_NoNormals;
    else if (!havePBRMaps)
      renderPixelFunction = &renderPixelSF_I_NoPBR;
    else
      renderPixelFunction = &renderPixelSF_I;
  }
  else
  {
    renderPixelFunction = &renderPixelSF_F;
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
    for (size_t j = 0; j < 8; j++)
    {
      if (landTxts[i][j] || !(0x003FU & (1U << (unsigned char) j)))
        landTextures[i][j] = landTxts[i][j];
      else if (j == 1)
        landTextures[i][j] = &defaultLandTxt_N;
      else if (j == 3)
        landTextures[i][j] = &defaultLandTxt_R;
      else if (j == 4)
        landTextures[i][j] = &defaultLandTxt_M;
      else if (j == 5)
        landTextures[i][j] = &defaultLandTxt_AO;
    }
  }
}

LandscapeTexture::LandscapeTexture(
    const unsigned char *txtSetPtr, const unsigned char *ltex16Ptr,
    int vertexCntX, int vertexCntY, int cellResolution,
    const LandscapeTextureSet *landTxts, size_t landTxtCnt)
  : txtSetData(txtSetPtr),
    ltexData16(ltex16Ptr),
    renderPixelFunction(&renderPixelSF_I_NoNormals),
    landTextures((LandscapeTextureSet *) 0),
    landTextureCnt(landTxtCnt),
    mipLevel(0.0f),
    rgbScale(1.0f),
    width(vertexCntX),
    height(vertexCntY),
    txtScale(1.0f),
    integerMip(true),
    txtSetMip(0),
    defaultColor(0x00002A662A662A66ULL)         // 0.05, 0.05, 0.05, 0.0
{
  copyTextureSet(landTxts, landTxtCnt);
  while ((2 << txtSetMip) < cellResolution)
    txtSetMip++;
}

LandscapeTexture::LandscapeTexture(
    const LandscapeData& landData,
    const LandscapeTextureSet *landTxts, size_t landTxtCnt)
  : txtSetData(landData.getCellTextureSets()),
    ltexData16(reinterpret_cast< const unsigned char * >(
                   landData.getLandTexture16())),
    renderPixelFunction(&renderPixelSF_I_NoNormals),
    landTextures((LandscapeTextureSet *) 0),
    landTextureCnt(landTxtCnt),
    mipLevel(0.0f),
    rgbScale(1.0f),
    width(landData.getImageWidth()),
    height(landData.getImageHeight()),
    txtScale(1.0f),
    integerMip(true),
    txtSetMip(0),
    defaultColor(0x00002A662A662A66ULL)
{
  copyTextureSet(landTxts, landTxtCnt);
  int     cellResolution = landData.getCellResolution();
  while ((2 << txtSetMip) < cellResolution)
    txtSetMip++;
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
  txtScale = float(std::pow(2.0f, mipLevel));
}

void LandscapeTexture::setRGBScale(float n)
{
  rgbScale = (n > 0.5f ? (n < 8.0f ? n : 8.0f) : 0.5f);
}

void LandscapeTexture::setDefaultColor(std::uint32_t c)
{
  FloatVector4  tmp(c);
  tmp.shuffleValues(0xC6).srgbExpand().clearV3();
  defaultColor = tmp.convertToFloat16();
}

void LandscapeTexture::renderTexture_NoNormals(
    unsigned char **outBufs, int renderScale, int x0, int y0, int x1, int y1)
{
  setRenderPixelFunction(false, false);
  FloatVector4  rgbScale_v(rgbScale);
  int     m = (1 << renderScale) - 1;
  float   renderScale_f = 1.0f / float(1 << renderScale);
  FloatVector4  n[2];
  n[0] = FloatVector4(0.0f);
  n[1] = FloatVector4(0.0f);
  unsigned char *outBuf = outBufs[0];
  for (int y = y0; y < y1; y++)
  {
    int     yc = (y > 0 ? y : 0) >> renderScale;
    int     yf = y & m;
    for (int x = x0; x < x1; x++, outBuf = outBuf + 4)
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
      std::uint32_t cTmp = std::uint32_t(c.srgbCompress() * rgbScale_v);
      FileBuffer::writeUInt32Fast(outBuf, cTmp | 0xFF000000U);
    }
  }
}

void LandscapeTexture::renderTexture_NoPBR(
    unsigned char **outBufs, int renderScale, int x0, int y0, int x1, int y1)
{
  setRenderPixelFunction(true, false);
  FloatVector4  rgbScale_v(rgbScale);
  int     m = (1 << renderScale) - 1;
  float   renderScale_f = 1.0f / float(1 << renderScale);
  FloatVector4  nDefault(0.0f, 0.0f, 127.5f, 0.0f);
  FloatVector4  n[2], n2[2], nTmp[2];
  n[1] = FloatVector4(0.0f);
  n2[1] = FloatVector4(0.0f);
  nTmp[1] = FloatVector4(0.0f);
  unsigned char *outBuf = outBufs[0];
  unsigned char *outBufN = outBufs[1];
  for (int y = y0; y < y1; y++)
  {
    int     yc = (y > 0 ? y : 0) >> renderScale;
    int     yf = y & m;
    for (int x = x0; x < x1; x++, outBuf = outBuf + 4)
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
      std::uint32_t cTmp = std::uint32_t(c.srgbCompress() * rgbScale_v);
      FileBuffer::writeUInt32Fast(outBuf, cTmp | 0xFF000000U);
      std::uint32_t n0 = normalToColor(rotateNormalSF(n[0])) ^ 0x8080U;
      FileBuffer::writeUInt16Fast(outBufN, std::uint16_t(n0 & 0xFFFFU));
      outBufN = outBufN + 2;
    }
  }
}

void LandscapeTexture::renderTexture(
    unsigned char **outBufs, int renderScale, int x0, int y0, int x1, int y1)
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
  if (!outBufs[1])
  {
    renderTexture_NoNormals(outBufs, renderScale, x0, y0, x1, y1);
    return;
  }
  if (!(outBufs[3] || outBufs[4] || outBufs[5]))
  {
    renderTexture_NoPBR(outBufs, renderScale, x0, y0, x1, y1);
    return;
  }
  setRenderPixelFunction(true, true);
  FloatVector4  rgbScale_v(rgbScale);
  int     m = (1 << renderScale) - 1;
  float   renderScale_f = 1.0f / float(1 << renderScale);
  FloatVector4  nDefault(0.0f, 0.0f, 1.0f, 1.0f);
  FloatVector4  pbrDefault(0.749f, 0.0f, 1.0f, 0.0f);   // rough, metal, ao
  unsigned char *outBuf = outBufs[0];
  unsigned char *outBufN = outBufs[1];
  unsigned char *outBufR = outBufs[3];
  unsigned char *outBufM = outBufs[4];
  unsigned char *outBufAO = outBufs[5];
  for (int y = y0; y < y1; y++)
  {
    int     yc = (y > 0 ? y : 0) >> renderScale;
    int     yf = y & m;
    for (int x = x0; x < x1; x++, outBuf = outBuf + 4)
    {
      int     xc = (x > 0 ? x : 0) >> renderScale;
      FloatVector4  n[2];
      n[0] = nDefault;
      n[1] = pbrDefault;
      FloatVector4  c(renderPixel(n, xc, yc, x, y));
      int     xf = x & m;
      if (xf)
      {
        FloatVector4  nTmp[2];
        nTmp[0] = nDefault;
        nTmp[1] = pbrDefault;
        FloatVector4  cTmp(renderPixel(nTmp, xc + 1, yc, x, y));
        blendNormals(n, nTmp, float(xf) * renderScale_f);
        c = blendColors(c, cTmp, float(xf) * renderScale_f);
      }
      if (yf)
      {
        FloatVector4  n2[2];
        n2[0] = nDefault;
        n2[1] = pbrDefault;
        FloatVector4  c2(renderPixel(n2, xc, yc + 1, x, y));
        if (xf)
        {
          FloatVector4  nTmp[2];
          nTmp[0] = nDefault;
          nTmp[1] = pbrDefault;
          FloatVector4  cTmp(renderPixel(nTmp, xc + 1, yc + 1, x, y));
          blendNormals(n2, nTmp, float(xf) * renderScale_f);
          c2 = blendColors(c2, cTmp, float(xf) * renderScale_f);
        }
        blendNormals(n, n2, float(yf) * renderScale_f);
        c = blendColors(c, c2, float(yf) * renderScale_f);
      }
      std::uint32_t cTmp = std::uint32_t(c.srgbCompress() * rgbScale_v);
      FileBuffer::writeUInt32Fast(outBuf, cTmp | 0xFF000000U);
      std::uint32_t n0 = normalToColor(rotateNormalSF(n[0])) ^ 0x8080U;
      FileBuffer::writeUInt16Fast(outBufN, std::uint16_t(n0 & 0xFFFFU));
      outBufN = outBufN + 2;
      std::uint32_t n1 = std::uint32_t(n[1]);
      if (outBufR)
        *(outBufR++) = (unsigned char) (n1 & 0xFFU);
      if (outBufM)
        *(outBufM++) = (unsigned char) ((n1 >> 8) & 0xFFU);
      if (outBufAO)
        *(outBufAO++) = (unsigned char) ((n1 >> 16) & 0xFFU);
    }
  }
}

