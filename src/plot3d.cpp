
#include "common.hpp"
#include "plot3d.hpp"

inline unsigned int Plot3D_TriShape::Plot3DTS_Base::multiplyWithLight(
    unsigned int c, float lightLevel)
{
  if (!(c & 0x80000000U))       // alpha threshold
    return 0U;
  unsigned int  lightLevel_i = (unsigned int) int(lightLevel * 256.0f + 0.5f);
  unsigned long long  tmp =
      (unsigned long long) (c & 0x000000FFU)
      | ((unsigned long long) (c & 0x0000FF00U) << 12)
      | ((unsigned long long) (c & 0x00FF0000U) << 24);
  tmp = (tmp * lightLevel_i) + 0x0000800008000080ULL;
  unsigned long long  tmp2 = ((tmp >> 8) | (tmp >> 9)) & 0x0001000010000100ULL;
  tmp = tmp | (tmp2 * 0xFFU);
  return ((unsigned int) ((tmp >> 8) & 0x000000FFU)
          | (unsigned int) ((tmp >> 20) & 0x0000FF00U)
          | (unsigned int) ((tmp >> 32) & 0x00FF0000U) | 0xFF000000U);
}

inline void Plot3D_TriShape::Plot3DTS_Water::drawPixel(int x, int y,
                                                       const ColorZUVL& z)
{
  if (x < 0 || x >= width || y < 0 || y >= height || z.z < 0.0f)
    return;
  size_t  offs = size_t(y) * size_t(width) + size_t(x);
  if (outBufZ[offs] <= z.z)
    return;
  outBufZ[offs] = z.z;
  unsigned int  lightLevel = (unsigned int) int(z.l * 64.0f + 1.5f);
  outBufRGBW[offs] = (outBufRGBW[offs] & 0x00FFFFFFU) | (lightLevel << 24);
}

inline void Plot3D_TriShape::Plot3DTS_TextureN::drawPixel(int x, int y,
                                                          const ColorZUVL& z)
{
  if (x < 0 || x >= width || y < 0 || y >= height || z.z < 0.0f)
    return;
  size_t  offs = size_t(y) * size_t(width) + size_t(x);
  if (outBufZ[offs] <= z.z)
    return;
  unsigned int  c = textureD->getPixelN(0, 0, 15);
  if (!(c = multiplyWithLight(c, z.l)))
    return;
  outBufZ[offs] = z.z;
  outBufRGBW[offs] = c;
}

inline void Plot3D_TriShape::Plot3DTS_TextureB::drawPixel(int x, int y,
                                                          const ColorZUVL& z)
{
  if (x < 0 || x >= width || y < 0 || y >= height || z.z < 0.0f)
    return;
  size_t  offs = size_t(y) * size_t(width) + size_t(x);
  if (outBufZ[offs] <= z.z)
    return;
  unsigned int  c = textureD->getPixelB(z.u, z.v, int(mipLevel));
  if (!(c = multiplyWithLight(c, z.l)))
    return;
  outBufZ[offs] = z.z;
  outBufRGBW[offs] = c;
}

inline void Plot3D_TriShape::Plot3DTS_TextureT::drawPixel(int x, int y,
                                                          const ColorZUVL& z)
{
  if (x < 0 || x >= width || y < 0 || y >= height || z.z < 0.0f)
    return;
  size_t  offs = size_t(y) * size_t(width) + size_t(x);
  if (outBufZ[offs] <= z.z)
    return;
  unsigned int  c = textureD->getPixelT(z.u, z.v, mipLevel);
  if (!(c = multiplyWithLight(c, z.l)))
    return;
  outBufZ[offs] = z.z;
  outBufRGBW[offs] = c;
}

inline void Plot3D_TriShape::Plot3DTS_NormalsT::drawPixel(int x, int y,
                                                          const ColorZUVL& z)
{
  if (x < 0 || x >= width || y < 0 || y >= height || z.z < 0.0f)
    return;
  size_t  offs = size_t(y) * size_t(width) + size_t(x);
  if (outBufZ[offs] <= z.z)
    return;
  unsigned int  c = textureD->getPixelT(z.u, z.v, mipLevel);
  // TODO: implement normal map
#if 0
  (void) textureN->getPixelT(z.u, z.v, mipLevel);
#endif
  if (!(c = multiplyWithLight(c, z.l)))
    return;
  outBufZ[offs] = z.z;
  outBufRGBW[offs] = c;
}

Plot3D_TriShape::Plot3D_TriShape(
    unsigned int *outBufRGBW, float *outBufZ, int imageWidth, int imageHeight,
    const NIFFile::NIFTriShape& t,
    const NIFFile::NIFVertexTransform& viewTransform)
  : bufRGBW(outBufRGBW),
    bufZ(outBufZ),
    width(imageWidth),
    height(imageHeight),
    vertexBuf(t.vertexCnt)
{
  vertexCnt = t.vertexCnt;
  triangleCnt = 0;
  vertexData = &(vertexBuf.front());
  vertexTransform = t.vertexTransform;
  vertexTransform *= viewTransform;
  isWater = t.isWater;
  texturePathCnt = t.texturePathCnt;
  texturePaths = t.texturePaths;
  materialPath = t.materialPath;
  textureOffsetU = t.textureOffsetU;
  textureOffsetV = t.textureOffsetV;
  textureScaleU = t.textureScaleU;
  textureScaleV = t.textureScaleV;
  name = t.name;
  float   xMin_f = float(0x40000000);
  float   yMin_f = xMin_f;
  float   zMin_f = xMin_f;
  float   xMax_f = -xMin_f;
  float   yMax_f = xMax_f;
  float   zMax_f = xMax_f;
  for (size_t i = 0; i < vertexCnt; i++)
  {
    NIFFile::NIFVertex& v = vertexBuf[i];
    v = t.vertexData[i];
    vertexTransform.transformXYZ(v.x, v.y, v.z);
    xMin_f = (v.x < xMin_f ? v.x : xMin_f);
    yMin_f = (v.y < yMin_f ? v.y : yMin_f);
    zMin_f = (v.z < zMin_f ? v.z : zMin_f);
    xMax_f = (v.x > xMax_f ? v.x : xMax_f);
    yMax_f = (v.y > yMax_f ? v.y : yMax_f);
    zMax_f = (v.z > zMax_f ? v.z : zMax_f);
  }
  xMin = roundFloat(xMin_f);
  yMin = roundFloat(yMin_f);
  zMin = roundFloat(zMin_f);
  xMax = roundFloat(xMax_f);
  yMax = roundFloat(yMax_f);
  zMax = roundFloat(zMax_f);
  if (!(xMin >= width || xMax < 0 || yMin >= height || yMax < 0 ||
        zMin >= 16777216 || zMax < 0))
  {
    const NIFFile::NIFVertexTransform&  vt = t.vertexTransform;
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
    if (t.triangleCnt > 0)
    {
      triangleBuf.reserve(t.triangleCnt);
      for (size_t i = 0; i < t.triangleCnt; i++)
        triangleBuf.push_back(t.triangleData[i]);
      triangleCnt = t.triangleCnt;
      triangleData = &(triangleBuf.front());
    }
  }
}

Plot3D_TriShape::~Plot3D_TriShape()
{
}

float Plot3D_TriShape::calculateLighting(
    float normalX, float normalY, float normalZ,
    float lightX, float lightY, float lightZ) const
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
  x = (x > -1.0f ? (x < 1.0f ? (x + 1.0f) : 2.0f) : 0.0f);
  return ((((x * -0.161345f + 0.811628f) * x - 1.530962f) * x + 1.679668f) * x
          + 0.2f);
}

void Plot3D_TriShape::drawTriShape(
    float lightX, float lightY, float lightZ,
    const DDSTexture *textureD, const DDSTexture *textureN)
{
  if (textureD && textureN &&
      !(textureN->getWidth() == textureD->getWidth() &&
        textureN->getHeight() == textureD->getHeight()))
  {
    textureN = (DDSTexture *) 0;
  }
  Plot3DTS_Base plot3d;
  plot3d.outBufRGBW = bufRGBW;
  plot3d.outBufZ = bufZ;
  plot3d.width = width;
  plot3d.height = height;
  plot3d.mipLevel = 0.0f;
  plot3d.normalX = 0.0f;
  plot3d.normalY = 0.0f;
  plot3d.normalZ = 1.0f;
  plot3d.textureD = textureD;
  plot3d.textureN = textureN;
  for (size_t i = 0; i < triangleCnt; i++)
  {
    if (triangleData[i].v0 >= vertexCnt || triangleData[i].v1 >= vertexCnt ||
        triangleData[i].v2 >= vertexCnt)
    {
      continue;
    }
    const NIFFile::NIFVertex& v0 = vertexData[triangleData[i].v0];
    const NIFFile::NIFVertex& v1 = vertexData[triangleData[i].v1];
    const NIFFile::NIFVertex& v2 = vertexData[triangleData[i].v2];
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
    ColorZUVL z0, z1, z2;
    z0.z = v0.z;
    z0.u = v0.getU() * textureScaleU + textureOffsetU;
    z0.v = v0.getV() * textureScaleV + textureOffsetV;
    z0.l = calculateLighting(v0.normalX, v0.normalY, v0.normalZ,
                             lightX, lightY, lightZ);
    z1.z = v1.z;
    z1.u = v1.getU() * textureScaleU + textureOffsetU;
    z1.v = v1.getV() * textureScaleV + textureOffsetV;
    z1.l = calculateLighting(v1.normalX, v1.normalY, v1.normalZ,
                             lightX, lightY, lightZ);
    z2.z = v2.z;
    z2.u = v2.getU() * textureScaleU + textureOffsetU;
    z2.v = v2.getV() * textureScaleV + textureOffsetV;
    z2.l = calculateLighting(v2.normalX, v2.normalY, v2.normalZ,
                             lightX, lightY, lightZ);
    if (isWater)
    {
      reinterpret_cast< Plot3D< Plot3DTS_Water, ColorZUVL > * >(
          &plot3d)->drawTriangle(x0, y0, z0, x1, y1, z1, x2, y2, z2);
    }
    else if (textureD)
    {
      float   xyArea = float(std::fabs((v1.x - v0.x) * (v1.y - v0.y)))
                       + float(std::fabs((v2.x - v0.x) * (v2.y - v0.y)))
                       + float(std::fabs((v2.x - v1.x) * (v2.y - v1.y)));
      float   uvArea = float(std::fabs((z1.u - z0.u) * (z1.v - z0.v)))
                       + float(std::fabs((z2.u - z0.u) * (z2.v - z0.v)))
                       + float(std::fabs((z2.u - z1.u) * (z2.v - z1.v)));
      float   txtW = float(textureD->getWidth());
      float   txtH = float(textureD->getHeight());
      uvArea *= (txtW * txtH);
      float   maxScale = (txtW < txtH ? txtW : txtH);
      if (!((xyArea * (maxScale * maxScale)) > uvArea))
      {
        // fill with solid color
        reinterpret_cast< Plot3D< Plot3DTS_TextureN, ColorZUVL > * >(
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
        z0.u *= txtW;
        z0.v *= txtH;
        z1.u *= txtW;
        z1.v *= txtH;
        z2.u *= txtW;
        z2.v *= txtH;
        plot3d.mipLevel = float(mipLevel_i) + mipLevel;
        if (textureN)
        {
          reinterpret_cast< Plot3D< Plot3DTS_NormalsT, ColorZUVL > * >(
              &plot3d)->drawTriangle(x0, y0, z0, x1, y1, z1, x2, y2, z2);
        }
        else if (mipLevel == 0.0f)
        {
          reinterpret_cast< Plot3D< Plot3DTS_TextureB, ColorZUVL > * >(
              &plot3d)->drawTriangle(x0, y0, z0, x1, y1, z1, x2, y2, z2);
        }
        else
        {
          reinterpret_cast< Plot3D< Plot3DTS_TextureT, ColorZUVL > * >(
              &plot3d)->drawTriangle(x0, y0, z0, x1, y1, z1, x2, y2, z2);
        }
      }
    }
  }
}

