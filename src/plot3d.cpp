
#include "common.hpp"
#include "plot3d.hpp"

const float Plot3D_TriShape::defaultLightingPolynomial[6] =
{
  0.672235f, 0.997428f, 0.009355f, -0.771812f, 0.108711f, 0.369682f
};

inline void Plot3D_TriShape::Plot3DTS_Water::drawPixel(int x, int y,
                                                       const ColorV2& z)
{
  if (x < 0 || x >= width || y < 0 || y >= height || z.v0 < 0.0f)
    return;
  size_t  offs = size_t(y) * size_t(width) + size_t(x);
  if (outBufZ[offs] <= z.v0)
    return;
  outBufZ[offs] = z.v0;
  unsigned int  lightLevel = (unsigned int) int(z.v1 * 0.25f + 1.375f);
  outBufRGBW[offs] = (outBufRGBW[offs] & 0x00FFFFFFU) | (lightLevel << 24);
}

inline void Plot3D_TriShape::Plot3DTS_TextureN::drawPixel(int x, int y,
                                                          const ColorV2& z)
{
  if (x < 0 || x >= width || y < 0 || y >= height || z.v0 < 0.0f)
    return;
  size_t  offs = size_t(y) * size_t(width) + size_t(x);
  if (outBufZ[offs] <= z.v0)
    return;
  unsigned int  c = textureD->getPixelN(0, 0, 15);
  if (!(c = Plot3D_TriShape::multiplyWithLight(c, alphaThreshold, int(z.v1))))
    return;
  outBufZ[offs] = z.v0;
  outBufRGBW[offs] = c;
}

inline void Plot3D_TriShape::Plot3DTS_TextureB::drawPixel(int x, int y,
                                                          const ColorV4& z)
{
  if (x < 0 || x >= width || y < 0 || y >= height || z.v0 < 0.0f)
    return;
  size_t  offs = size_t(y) * size_t(width) + size_t(x);
  if (outBufZ[offs] <= z.v0)
    return;
  unsigned int  c = textureD->getPixelB(z.v2, z.v3, int(mipLevel));
  if (!(c = Plot3D_TriShape::multiplyWithLight(c, alphaThreshold, int(z.v1))))
    return;
  outBufZ[offs] = z.v0;
  outBufRGBW[offs] = c;
}

inline void Plot3D_TriShape::Plot3DTS_TextureT::drawPixel(int x, int y,
                                                          const ColorV4& z)
{
  if (x < 0 || x >= width || y < 0 || y >= height || z.v0 < 0.0f)
    return;
  size_t  offs = size_t(y) * size_t(width) + size_t(x);
  if (outBufZ[offs] <= z.v0)
    return;
  unsigned int  c = textureD->getPixelT(z.v2, z.v3, mipLevel);
  if (!(c = Plot3D_TriShape::multiplyWithLight(c, alphaThreshold, int(z.v1))))
    return;
  outBufZ[offs] = z.v0;
  outBufRGBW[offs] = c;
}

unsigned int Plot3D_TriShape::Plot3DTS_NormalsT::environmentMap(
    unsigned int c, float normalX, float normalY, int l) const
{
  float   u = (normalX + 0.5f) * float(textureE->getWidth());
  float   v = (normalY + 0.5f) * float(textureE->getHeight());
  unsigned int  tmp = textureE->getPixelB(u, v, 0);
  tmp = Plot3D_TriShape::multiplyWithLight(tmp, 0U, l);
  unsigned int  rb = (c & 0x00FF00FFU) + (tmp & 0x00FF00FFU);
  unsigned int  g = (c & 0x0000FF00U) + (tmp & 0x0000FF00U);
  tmp = (((rb & 0x01000100U) | (g & 0x00010000U)) >> 8) * 0xFFU;
  return (0xFF000000U | (rb & 0x00FF00FFU) | (g & 0x0000FF00U) | tmp);
}

void Plot3D_TriShape::Plot3DTS_NormalsT::drawPixel(int x, int y,
                                                   const ColorV6& z)
{
  if (x < 0 || x >= width || y < 0 || y >= height || z.v0 < 0.0f)
    return;
  size_t  offs = size_t(y) * size_t(width) + size_t(x);
  if (outBufZ[offs] <= z.v0)
    return;
  unsigned int  c = textureD->getPixelT(z.v2, z.v3, mipLevel);
  if (c < alphaThreshold)
    return;
  unsigned int  n =
      textureN->getPixelT(z.v2 * textureScaleN, z.v3 * textureScaleN, mipLevel);
  float   tmpX = float(int(n & 0xFF)) * (1.0f / 127.5f) - 1.0f;
  float   tmpY = float(int((n >> 8) & 0xFF)) * (1.0f / 127.5f) - 1.0f;
  float   tmpZ = (tmpX * tmpX) + (tmpY * tmpY);
  if (tmpZ >= 1.0f)
  {
    tmpZ = (3.0f - tmpZ) * 0.5f;        // approximates 1.0 / sqrt(tmpZ)
    tmpX = tmpX * tmpZ;
    tmpY = tmpY * tmpZ;
    tmpZ = 0.0f;
  }
  else
  {
    tmpZ = float(std::sqrt(1.0001f - tmpZ));
  }
  float   ry_s = -tmpX;
  float   ry_c = float(std::sqrt(1.0001f - (ry_s * ry_s)));
  float   rx_s = tmpY * (1.0f / ry_c);
  float   rx_c = tmpZ * (1.0f / ry_c);
  float   normalX = (z.v4 * ry_c) - (z.v1 * ry_s);
  float   normalY = (z.v4 * ry_s * rx_s) + (z.v5 * rx_c) + (z.v1 * tmpY);
  float   normalZ = (z.v4 * ry_s * rx_c) - (z.v5 * rx_s) + (z.v1 * tmpZ);
  int     l = int(Plot3D_TriShape::calculateLighting(
                      normalX, normalY, normalZ, lightX, lightY, lightZ,
                      lightingPolynomial));
  outBufZ[offs] = z.v0;
  c = Plot3D_TriShape::multiplyWithLight(c, 0U, l);
  if (textureE)
    c = environmentMap(c, normalX, normalY, 172);
  outBufRGBW[offs] = c;
}

bool Plot3D_TriShape::transformVertexData(
    const NIFFile::NIFVertexTransform& modelTransform,
    const NIFFile::NIFVertexTransform& viewTransform,
    float& lightX, float& lightY, float& lightZ)
{
  if (vertexBuf.size() < vertexCnt)
    vertexBuf.resize(vertexCnt);
  NIFFile::NIFVertexTransform vt(vertexTransform);
  vt *= modelTransform;
  vt *= viewTransform;
  NIFFile::NIFBounds  b;
  for (size_t i = 0; i < vertexCnt; i++)
  {
    NIFFile::NIFVertex& v = vertexBuf[i];
    v = vertexData[i];
    vt.transformXYZ(v.x, v.y, v.z);
    b += v;
  }
  if (b.xMin >= (float(width) - 0.5f) || b.xMax < -0.5f ||
      b.yMin >= (float(height) - 0.5f) || b.yMax < -0.5f ||
      b.zMin >= 16777216.0f || b.zMax < 0.0f)
  {
    return false;
  }
  for (size_t i = 0; i < vertexCnt; i++)
  {
    NIFFile::NIFVertex& v = vertexBuf[i];
    float   x = v.normalX;
    float   y = v.normalY;
    float   z = v.normalZ;
    v.normalX = (x * vt.rotateXX) + (y * vt.rotateXY) + (z * vt.rotateXZ);
    v.normalY = (x * vt.rotateYX) + (y * vt.rotateYY) + (z * vt.rotateYZ);
    v.normalZ = (x * vt.rotateZX) + (y * vt.rotateZY) + (z * vt.rotateZZ);
  }
  const NIFFile::NIFVertexTransform&  t = viewTransform;
  float   x = lightX;
  float   y = lightY;
  float   z = lightZ;
  lightX = (x * t.rotateXX) + (y * t.rotateXY) + (z * t.rotateXZ);
  lightY = (x * t.rotateYX) + (y * t.rotateYY) + (z * t.rotateYZ);
  lightZ = (x * t.rotateZX) + (y * t.rotateZY) + (z * t.rotateZZ);
  return true;
}

Plot3D_TriShape::Plot3D_TriShape(
    unsigned int *outBufRGBW, float *outBufZ, int imageWidth, int imageHeight,
    const NIFFile::NIFTriShape& t)
  : NIFFile::NIFTriShape(t),
    bufRGBW(outBufRGBW),
    bufZ(outBufZ),
    width(imageWidth),
    height(imageHeight),
    vertexBuf(vertexCnt)
{
  setLightingFunction(defaultLightingPolynomial);
}

Plot3D_TriShape::~Plot3D_TriShape()
{
}

void Plot3D_TriShape::setLightingFunction(const float *a)
{
  for (int i = 0; i < 6; i++)
    lightingPolynomial[i] = a[i] * 256.0f;
  lightingPolynomial[0] = lightingPolynomial[0] + 0.5f;
}

void Plot3D_TriShape::getLightingFunction(float *a) const
{
  for (int i = 0; i < 6; i++)
    a[i] = lightingPolynomial[i] * (1.0f / 256.0f);
  a[0] = a[0] - (1.0f / 512.0f);
}

void Plot3D_TriShape::getDefaultLightingFunction(float *a)
{
  for (int i = 0; i < 6; i++)
    a[i] = defaultLightingPolynomial[i];
}

float Plot3D_TriShape::calculateLighting(
    float normalX, float normalY, float normalZ,
    float lightX, float lightY, float lightZ, const float *a)
{
  float   tmp = (normalX * normalX) + (normalY * normalY) + (normalZ * normalZ);
  if (tmp > 0.0f)
  {
    if (tmp >= 0.96875f && tmp <= 1.03125f)
      tmp = (3.0f - tmp) * 0.5f;
    else
      tmp = 1.0f / float(std::sqrt(tmp));
    normalX *= tmp;
    normalY *= tmp;
    normalZ *= tmp;
  }
  float   x = (normalX * lightX) + (normalY * lightY) + (normalZ * lightZ);
  return (((((x * a[5] + a[4]) * x + a[3]) * x + a[2]) * x + a[1]) * x + a[0]);
}

void Plot3D_TriShape::drawTriShape(
    const NIFFile::NIFVertexTransform& modelTransform,
    const NIFFile::NIFVertexTransform& viewTransform,
    float lightX, float lightY, float lightZ,
    const DDSTexture *textureD, const DDSTexture *textureN,
    const DDSTexture *textureE)
{
  if (!transformVertexData(modelTransform, viewTransform,
                           lightX, lightY, lightZ))
  {
    return;
  }
  Plot3DTS_Base plot3d;
  plot3d.outBufRGBW = bufRGBW;
  plot3d.outBufZ = bufZ;
  plot3d.width = width;
  plot3d.height = height;
  plot3d.mipLevel = 0.0f;
  plot3d.alphaThreshold = (unsigned int) alphaThreshold << 24;
  plot3d.textureD = textureD;
  plot3d.textureN = (DDSTexture *) 0;
  plot3d.textureScaleN = 1.0f;
  plot3d.lightX = lightX;
  plot3d.lightY = lightY;
  plot3d.lightZ = lightZ;
  plot3d.lightingPolynomial = lightingPolynomial;
  plot3d.textureE = (DDSTexture *) 0;
  if (textureD && textureN &&
      ((textureD->getWidth() * textureN->getHeight())
       == (textureD->getHeight() * textureN->getWidth())))
  {
    plot3d.textureN = textureN;
    plot3d.textureScaleN =
        float(textureN->getWidth()) / float(textureD->getWidth());
    plot3d.textureE = textureE;
  }
  for (size_t i = 0; i < triangleCnt; i++)
  {
    if (triangleData[i].v0 >= vertexCnt || triangleData[i].v1 >= vertexCnt ||
        triangleData[i].v2 >= vertexCnt)
    {
      continue;
    }
    const NIFFile::NIFVertex& v0 = vertexBuf[triangleData[i].v0];
    const NIFFile::NIFVertex& v1 = vertexBuf[triangleData[i].v1];
    const NIFFile::NIFVertex& v2 = vertexBuf[triangleData[i].v2];
    if (v0.normalZ > 0.0f && v1.normalZ > 0.0f && v2.normalZ > 0.0f &&
        !alphaThreshold && !isWater)
    {
      continue;
    }
    int     x0 = roundFloat(v0.x);
    int     y0 = roundFloat(v0.y);
    int     x1 = roundFloat(v1.x);
    int     y1 = roundFloat(v1.y);
    int     x2 = roundFloat(v2.x);
    int     y2 = roundFloat(v2.y);
    if ((x0 < 0 && x1 < 0 && x2 < 0) ||
        (x0 >= width && x1 >= width && x2 >= width) ||
        (y0 < 0 && y1 < 0 && y2 < 0) ||
        (y0 >= height && y1 >= height && y2 >= height) ||
        (v0.z < 0.0f && v1.z < 0.0f && v2.z < 0.0f))
    {
      continue;
    }
    ColorV6 z0, z1, z2;
    z0.v0 = v0.z;
    z1.v0 = v1.z;
    z2.v0 = v2.z;
    z0.v1 = calculateLighting(v0.normalX, v0.normalY, v0.normalZ,
                              lightX, lightY, lightZ, lightingPolynomial);
    z1.v1 = calculateLighting(v1.normalX, v1.normalY, v1.normalZ,
                              lightX, lightY, lightZ, lightingPolynomial);
    z2.v1 = calculateLighting(v2.normalX, v2.normalY, v2.normalZ,
                              lightX, lightY, lightZ, lightingPolynomial);
    if (isWater)
    {
      reinterpret_cast< Plot3D< Plot3DTS_Water, ColorV2 > * >(
          &plot3d)->drawTriangle(x0, y0, z0, x1, y1, z1, x2, y2, z2);
    }
    else if (textureD)
    {
      z0.v2 = v0.getU() * textureScaleU + textureOffsetU;
      z0.v3 = v0.getV() * textureScaleV + textureOffsetV;
      z1.v2 = v1.getU() * textureScaleU + textureOffsetU;
      z1.v3 = v1.getV() * textureScaleV + textureOffsetV;
      z2.v2 = v2.getU() * textureScaleU + textureOffsetU;
      z2.v3 = v2.getV() * textureScaleV + textureOffsetV;
      float   xyArea = float(std::fabs((v1.x - v0.x) * (v1.y - v0.y)))
                       + float(std::fabs((v2.x - v0.x) * (v2.y - v0.y)))
                       + float(std::fabs((v2.x - v1.x) * (v2.y - v1.y)));
      float   uvArea = float(std::fabs((z1.v2 - z0.v2) * (z1.v3 - z0.v3)))
                       + float(std::fabs((z2.v2 - z0.v2) * (z2.v3 - z0.v3)))
                       + float(std::fabs((z2.v2 - z1.v2) * (z2.v3 - z1.v3)));
      float   txtW = float(textureD->getWidth());
      float   txtH = float(textureD->getHeight());
      uvArea *= (txtW * txtH);
      float   maxScale = (txtW < txtH ? txtW : txtH);
      if (!((xyArea * (maxScale * maxScale)) > uvArea))
      {
        // fill with solid color
        reinterpret_cast< Plot3D< Plot3DTS_TextureN, ColorV2 > * >(
            &plot3d)->drawTriangle(x0, y0, z0, x1, y1, z1, x2, y2, z2);
      }
      else
      {
        float   mipLevel = 0.0f;
        if (uvArea > xyArea)
          mipLevel = float(std::log(uvArea / xyArea)) * 0.7213475f;
        int     mipLevel_i = int(mipLevel);
        mipLevel = mipLevel - float(mipLevel_i);
        if (mipLevel < 0.0625f)
        {
          mipLevel = 0.0f;
        }
        else if (mipLevel >= 0.9375f)
        {
          mipLevel = 0.0f;
          mipLevel_i++;
        }
        float   txtScale = float(65536 >> mipLevel_i) * (1.0f / 65536.0f);
        txtW *= txtScale;
        txtH *= txtScale;
        z0.v2 *= txtW;
        z0.v3 *= txtH;
        z1.v2 *= txtW;
        z1.v3 *= txtH;
        z2.v2 *= txtW;
        z2.v3 *= txtH;
        plot3d.mipLevel = float(mipLevel_i) + mipLevel;
        if (textureN)
        {
          z0.v4 = v0.normalX;
          z0.v5 = v0.normalY;
          z0.v1 = v0.normalZ;
          z1.v4 = v1.normalX;
          z1.v5 = v1.normalY;
          z1.v1 = v1.normalZ;
          z2.v4 = v2.normalX;
          z2.v5 = v2.normalY;
          z2.v1 = v2.normalZ;
          reinterpret_cast< Plot3D< Plot3DTS_NormalsT, ColorV6 > * >(
              &plot3d)->drawTriangle(x0, y0, z0, x1, y1, z1, x2, y2, z2);
        }
        else if (mipLevel == 0.0f)
        {
          reinterpret_cast< Plot3D< Plot3DTS_TextureB, ColorV4 > * >(
              &plot3d)->drawTriangle(x0, y0, z0, x1, y1, z1, x2, y2, z2);
        }
        else
        {
          reinterpret_cast< Plot3D< Plot3DTS_TextureT, ColorV4 > * >(
              &plot3d)->drawTriangle(x0, y0, z0, x1, y1, z1, x2, y2, z2);
        }
      }
    }
  }
}

