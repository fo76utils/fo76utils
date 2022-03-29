
#include "common.hpp"
#include "plot3d.hpp"

const float Plot3D_TriShape::defaultLightingPolynomial[6] =
{
  0.672235f, 0.997428f, 0.009355f, -0.771812f, 0.108711f, 0.369682f
};

Plot3D_TriShape::Plot3DTS_Base::Plot3DTS_Base(
    Plot3D_TriShape& p, float light_X, float light_Y, float light_Z,
    const DDSTexture * const *textures, size_t textureCnt)
{
  outBufRGBW = p.bufRGBW;
  outBufZ = p.bufZ;
  width = p.width;
  height = p.height;
  mipLevel = 0.0f;
  alphaThreshold = (unsigned int) p.alphaThreshold << 24;
  textureD = (DDSTexture *) 0;
  textureN = (DDSTexture *) 0;
  textureScaleN = 1.0f;
  lightX = light_X;
  lightY = light_Y;
  lightZ = light_Z;
  lightingPolynomial = p.lightingPolynomial;
  textureE = (DDSTexture *) 0;
  reflectionLevel = 0;
  textureScaleR = 1.0f;
  textureR = (DDSTexture *) 0;
  if (textureCnt >= 1)
    textureD = textures[0];
  if (!(textureD && textureCnt >= 2 && textures[1] &&
        ((textureD->getWidth() * textures[1]->getHeight())
         == (textureD->getHeight() * textures[1]->getWidth()))))
  {
    return;
  }
  textureN = textures[1];
  textureScaleN = float(textureN->getWidth()) / float(textureD->getWidth());
  if (!(textureCnt >= 5 && textures[4]))
    return;
  textureE = textures[4];
  if (!(textureCnt >= 9 && textures[8]))
  {
    reflectionLevel = int(Plot3D_TriShape::calculateLighting(
                              0.0f, 0.0f, -1.0f, 0.707107f, 0.707107f, 0.0f,
                              lightingPolynomial));
    return;
  }
  textureR = textures[8];
  reflectionLevel = int(Plot3D_TriShape::calculateLighting(
                            0.0f, 0.0f, -1.0f, 0.684653f, 0.684653f, -0.25f,
                            lightingPolynomial));
  textureScaleR = float(textureR->getWidth()) / float(textureD->getWidth());
}

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

inline int Plot3D_TriShape::Plot3DTS_NormalsT::normalMap(
    float& normalX, float& normalY, float& normalZ, unsigned int n) const
{
  float   tmpX = 1.0f - (float(int(n & 0xFF)) * (1.0f / 127.5f));
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
  // convert normal map data to rotation matrix, and apply to input normal
  float   ry_c = float(std::sqrt(1.0001f - (tmpX * tmpX)));
  float   rx_s = tmpY * (1.0f / ry_c);
  float   rx_c = tmpZ * (1.0f / ry_c);
  tmpZ = (normalZ * tmpZ) - (normalY * rx_s) - (normalX * tmpX * rx_c);
  tmpY = (normalZ * tmpY) + (normalY * rx_c) - (normalX * tmpX * rx_s);
  tmpX = (normalZ * tmpX) + (normalX * ry_c);
  // normalize
  float   tmp = (tmpX * tmpX) + (tmpY * tmpY) + (tmpZ * tmpZ);
  if (tmp > 0.0f)
  {
    if (tmp >= 0.96875f && tmp <= 1.03125f)
      tmp = (3.0f - tmp) * 0.5f;
    else
      tmp = 1.0f / float(std::sqrt(tmp));
    tmpX *= tmp;
    tmpY *= tmp;
    tmpZ *= tmp;
  }
  // calculate lighting
  const float   *a = lightingPolynomial;
  float   d = (tmpX * lightX) + (tmpY * lightY) + (tmpZ * lightZ);
  normalX = tmpX;
  normalY = tmpY;
  normalZ = tmpZ;
  return int(((((d * a[5] + a[4]) * d + a[3]) * d + a[2]) * d + a[1]) * d
             + a[0]);
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
  outBufZ[offs] = z.v0;
  unsigned int  n =
      textureN->getPixelT(z.v2 * textureScaleN, z.v3 * textureScaleN, mipLevel);
  float   normalX = z.v4;
  float   normalY = z.v5;
  float   normalZ = z.v1;
  int     l = normalMap(normalX, normalY, normalZ, n);
  outBufRGBW[offs] = Plot3D_TriShape::multiplyWithLight(c, 0U, l);
}

inline unsigned int Plot3D_TriShape::Plot3DTS_NormalsEnvT::environmentMap(
    unsigned int c, float normalX, float normalY, float normalZ,
    int x, int y) const
{
  float   u = float((height >> 1) - y) * (0.25f / float(height)) + 0.5f;
  float   v = float(x - (width >> 1)) * (0.25f / float(height)) + 0.5f;
  if (normalZ < -0.5f)
  {
    u = u + (normalY * (0.25f / normalZ));
    v = v - (normalX * (0.25f / normalZ));
  }
  else
  {
    u = u - (normalY * 0.5f);
    v = v + (normalX * 0.5f);
  }
  unsigned int  tmp = textureE->getPixelB(u * float(textureE->getWidth()),
                                          v * float(textureE->getHeight()), 0);
  tmp = Plot3D_TriShape::multiplyWithLight(tmp, 0U, reflectionLevel);
  unsigned int  rb = (c & 0x00FF00FFU) + (tmp & 0x00FF00FFU);
  unsigned int  g = (c & 0x0000FF00U) + (tmp & 0x0000FF00U);
  tmp = (((rb & 0x01000100U) | (g & 0x00010000U)) >> 8) * 0xFFU;
  return (0xFF000000U | (rb & 0x00FF00FFU) | (g & 0x0000FF00U) | tmp);
}

void Plot3D_TriShape::Plot3DTS_NormalsEnvT::drawPixel(int x, int y,
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
  outBufZ[offs] = z.v0;
  unsigned int  n =
      textureN->getPixelT(z.v2 * textureScaleN, z.v3 * textureScaleN, mipLevel);
  float   normalX = z.v4;
  float   normalY = z.v5;
  float   normalZ = z.v1;
  int     l = normalMap(normalX, normalY, normalZ, n);
  c = Plot3D_TriShape::multiplyWithLight(c, 0U, l);
  outBufRGBW[offs] = environmentMap(c, normalX, normalY, normalZ, x, y);
}

inline unsigned int Plot3D_TriShape::Plot3DTS_NormalsReflT::reflectionMap(
    unsigned int c, float normalX, float normalY, float normalZ,
    int x, int y, const ColorV6& z) const
{
  unsigned int  tmp =
      textureR->getPixelT(z.v2 * textureScaleR, z.v3 * textureScaleR, mipLevel);
  if (!(tmp & 0x00FFFFFFU))
    return c;
  float   u = float((height >> 1) - y) * (0.25f / float(height)) + 0.5f;
  float   v = float(x - (width >> 1)) * (0.25f / float(height)) + 0.5f;
  if (normalZ < -0.5f)
  {
    u = u + (normalY * (0.25f / normalZ));
    v = v - (normalX * (0.25f / normalZ));
  }
  else
  {
    u = u - (normalY * 0.5f);
    v = v + (normalX * 0.5f);
  }
  unsigned int  n = 0xFFC0C0C0U;
  if (BRANCH_EXPECT(textureE, true))
  {
    n = textureE->getPixelB(u * float(textureE->getWidth()),
                            v * float(textureE->getHeight()), 0);
  }
  unsigned int  r = ((tmp & 0xFFU) * (n & 0xFFU) + 0x80U) >> 8;
  unsigned int  g = ((tmp & 0xFF00U) * (n & 0xFF00U) + 0x00800000U) >> 24;
  unsigned int  b = (((tmp >> 16) & 0xFFU) * ((n >> 16) & 0xFFU) + 0x80U) >> 8;
  tmp = Plot3D_TriShape::multiplyWithLight(r | (g << 8) | (b << 16), 0U,
                                           reflectionLevel);
  unsigned int  rb = (c & 0x00FF00FFU) + (tmp & 0x00FF00FFU);
  g = (c & 0x0000FF00U) + (tmp & 0x0000FF00U);
  tmp = (((rb & 0x01000100U) | (g & 0x00010000U)) >> 8) * 0xFFU;
  return (0xFF000000U | (rb & 0x00FF00FFU) | (g & 0x0000FF00U) | tmp);
}

void Plot3D_TriShape::Plot3DTS_NormalsReflT::drawPixel(int x, int y,
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
  outBufZ[offs] = z.v0;
  unsigned int  n =
      textureN->getPixelT(z.v2 * textureScaleN, z.v3 * textureScaleN, mipLevel);
  float   normalX = z.v4;
  float   normalY = z.v5;
  float   normalZ = z.v1;
  int     l = normalMap(normalX, normalY, normalZ, n);
  c = Plot3D_TriShape::multiplyWithLight(c, 0U, l);
  outBufRGBW[offs] = reflectionMap(c, normalX, normalY, normalZ, x, y, z);
}

void Plot3D_TriShape::sortTriangles(size_t n0, size_t n2)
{
  if ((n2 - n0) < 2)
    return;
  size_t  n1 = (n0 + n2) >> 1;
  if ((n1 - n0) > 1)
    sortTriangles(n0, n1);
  if ((n2 - n1) > 1)
    sortTriangles(n1, n2);
  size_t  i = n0;
  size_t  j = n1;
  size_t  k = triangleCnt;
  for ( ; i < n1 && j < n2; k++)
  {
    const NIFFile::NIFTriangle& t1 = triangleData[triangleBuf[i]];
    const NIFFile::NIFTriangle& t2 = triangleData[triangleBuf[j]];
    float   z1 = vertexBuf[t1.v0].z + vertexBuf[t1.v1].z + vertexBuf[t1.v2].z;
    float   z2 = vertexBuf[t2.v0].z + vertexBuf[t2.v1].z + vertexBuf[t2.v2].z;
    if (z2 < z1)
    {
      triangleBuf[k] = triangleBuf[j];
      j++;
    }
    else
    {
      triangleBuf[k] = triangleBuf[i];
      i++;
    }
  }
  for ( ; i < n1; i++, k++)
    triangleBuf[k] = triangleBuf[i];
  for ( ; j < n2; j++, k++)
    triangleBuf[k] = triangleBuf[j];
  for (i = n0; i < n2; i++)
    triangleBuf[i] = triangleBuf[triangleCnt + (i - n0)];
}

size_t Plot3D_TriShape::transformVertexData(
    const NIFFile::NIFVertexTransform& modelTransform,
    const NIFFile::NIFVertexTransform& viewTransform,
    float& lightX, float& lightY, float& lightZ)
{
  if (vertexBuf.size() < vertexCnt)
    vertexBuf.resize(vertexCnt);
  if (triangleBuf.size() < (triangleCnt << 1))
    triangleBuf.resize(triangleCnt << 1);
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
    return 0;
  }
  {
    const NIFFile::NIFVertexTransform&  t = viewTransform;
    float   x = lightX;
    float   y = lightY;
    float   z = lightZ;
    lightX = (x * t.rotateXX) + (y * t.rotateXY) + (z * t.rotateXZ);
    lightY = (x * t.rotateYX) + (y * t.rotateYY) + (z * t.rotateYZ);
    lightZ = (x * t.rotateZX) + (y * t.rotateZY) + (z * t.rotateZZ);
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
    float   l = calculateLighting(v.normalX, v.normalY, v.normalZ,
                                  lightX, lightY, lightZ, lightingPolynomial);
    v.vertexColor = (unsigned int) roundFloat(l * 1048576.0f);
  }
  size_t  n = 0;
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
        !alphaThreshold && !(flags & 0x0002))
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
    triangleBuf[n] = (unsigned int) i;
    n++;
  }
  sortTriangles(0, n);
  return n;
}

Plot3D_TriShape::Plot3D_TriShape(
    unsigned int *outBufRGBW, float *outBufZ, int imageWidth, int imageHeight)
  : bufRGBW(outBufRGBW),
    bufZ(outBufZ),
    width(imageWidth),
    height(imageHeight)
{
  setLightingFunction(defaultLightingPolynomial);
}

Plot3D_TriShape::~Plot3D_TriShape()
{
}

Plot3D_TriShape& Plot3D_TriShape::operator=(const NIFFile::NIFTriShape& t)
{
  vertexCnt = t.vertexCnt;
  triangleCnt = t.triangleCnt;
  vertexData = t.vertexData;
  triangleData = t.triangleData;
  vertexTransform = t.vertexTransform;
  flags = t.flags;
  alphaThreshold = t.alphaThreshold;
  texturePathCnt = t.texturePathCnt;
  texturePaths = t.texturePaths;
  materialPath = t.materialPath;
  textureOffsetU = t.textureOffsetU;
  textureOffsetV = t.textureOffsetV;
  textureScaleU = t.textureScaleU;
  textureScaleV = t.textureScaleV;
  name = t.name;
  return (*this);
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
    const DDSTexture * const *textures, size_t textureCnt)
{
  if (flags & 0x0002)
    textureCnt = 0;                     // water
  else if (!(textureCnt >= 1 && textures[0]))
    return;
  size_t  triangleCntRendered =
      transformVertexData(modelTransform, viewTransform,
                          lightX, lightY, lightZ);
  if (!triangleCntRendered)
    return;
  Plot3DTS_Base plot3d(*this, lightX, lightY, lightZ, textures, textureCnt);
  const DDSTexture  *textureD = plot3d.textureD;
  const DDSTexture  *textureN = plot3d.textureN;
  const DDSTexture  *textureE = plot3d.textureE;
  const DDSTexture  *textureR = plot3d.textureR;
  for (size_t i = 0; i < triangleCntRendered; i++)
  {
    const NIFFile::NIFVertex& v0 = vertexBuf[triangleData[triangleBuf[i]].v0];
    const NIFFile::NIFVertex& v1 = vertexBuf[triangleData[triangleBuf[i]].v1];
    const NIFFile::NIFVertex& v2 = vertexBuf[triangleData[triangleBuf[i]].v2];
    int     x0 = roundFloat(v0.x);
    int     y0 = roundFloat(v0.y);
    int     x1 = roundFloat(v1.x);
    int     y1 = roundFloat(v1.y);
    int     x2 = roundFloat(v2.x);
    int     y2 = roundFloat(v2.y);
    ColorV6 z0, z1, z2;
    z0.v0 = v0.z;
    z0.v1 = float(int(v0.vertexColor)) * (1.0f / 1048576.0f);
    z1.v0 = v1.z;
    z1.v1 = float(int(v1.vertexColor)) * (1.0f / 1048576.0f);
    z2.v0 = v2.z;
    z2.v1 = float(int(v2.vertexColor)) * (1.0f / 1048576.0f);
    if (!textureD)
    {
      reinterpret_cast< Plot3D< Plot3DTS_Water, ColorV2 > * >(
          &plot3d)->drawTriangle(x0, y0, z0, x1, y1, z1, x2, y2, z2);
    }
    else
    {
      z0.v2 = v0.getU() * textureScaleU + textureOffsetU;
      z0.v3 = v0.getV() * textureScaleV + textureOffsetV;
      z1.v2 = v1.getU() * textureScaleU + textureOffsetU;
      z1.v3 = v1.getV() * textureScaleV + textureOffsetV;
      z2.v2 = v2.getU() * textureScaleU + textureOffsetU;
      z2.v3 = v2.getV() * textureScaleV + textureOffsetV;
      float   xyArea2 = float(std::fabs(((v1.x - v0.x) * (v2.y - v0.y))
                                        - ((v2.x - v0.x) * (v1.y - v0.y))));
      float   uvArea2 = float(std::fabs(((z1.v2 - z0.v2) * (z2.v3 - z0.v3))
                                        - ((z2.v2 - z0.v2) * (z1.v3 - z0.v3))));
      float   txtW = float(textureD->getWidth());
      float   txtH = float(textureD->getHeight());
      uvArea2 *= (txtW * txtH);
      float   maxScale = (txtW < txtH ? txtW : txtH);
      float   mipLevel = 15.0f;
      bool    integerMipLevel = true;
      if ((xyArea2 * (maxScale * maxScale)) > uvArea2)
      {
        mipLevel = 0.0f;
        if (uvArea2 > xyArea2)
          mipLevel = float(std::log(uvArea2 / xyArea2)) * 0.7213475f;
        int     mipLevel_i = int(mipLevel);
        mipLevel = mipLevel - float(mipLevel_i);
        integerMipLevel = (mipLevel < 0.0625f || mipLevel >= 0.9375f);
        if (integerMipLevel)
        {
          mipLevel_i += int(mipLevel >= 0.5f);
          mipLevel = 0.0f;
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
        mipLevel += float(mipLevel_i);
      }
      plot3d.mipLevel = mipLevel;
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
        if (!textureE)
        {
          reinterpret_cast< Plot3D< Plot3DTS_NormalsT, ColorV6 > * >(
              &plot3d)->drawTriangle(x0, y0, z0, x1, y1, z1, x2, y2, z2);
        }
        else if (textureR)
        {
          reinterpret_cast< Plot3D< Plot3DTS_NormalsReflT, ColorV6 > * >(
              &plot3d)->drawTriangle(x0, y0, z0, x1, y1, z1, x2, y2, z2);
        }
        else
        {
          reinterpret_cast< Plot3D< Plot3DTS_NormalsEnvT, ColorV6 > * >(
              &plot3d)->drawTriangle(x0, y0, z0, x1, y1, z1, x2, y2, z2);
        }
      }
      else if (integerMipLevel)
      {
        if (mipLevel >= 15.0f)
        {
          // fill with solid color
          reinterpret_cast< Plot3D< Plot3DTS_TextureN, ColorV2 > * >(
              &plot3d)->drawTriangle(x0, y0, z0, x1, y1, z1, x2, y2, z2);
        }
        else
        {
          reinterpret_cast< Plot3D< Plot3DTS_TextureB, ColorV4 > * >(
              &plot3d)->drawTriangle(x0, y0, z0, x1, y1, z1, x2, y2, z2);
        }
      }
      else
      {
        reinterpret_cast< Plot3D< Plot3DTS_TextureT, ColorV4 > * >(
            &plot3d)->drawTriangle(x0, y0, z0, x1, y1, z1, x2, y2, z2);
      }
    }
  }
}

