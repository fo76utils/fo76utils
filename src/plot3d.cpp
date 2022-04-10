
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
  textureG = (DDSTexture *) 0;
  textureN = (DDSTexture *) 0;
  textureScaleN = 1.0f;
  lightX = light_X;
  lightY = light_Y;
  lightZ = light_Z;
  lightingPolynomial = p.lightingPolynomial;
  textureE = (DDSTexture *) 0;
  reflectionLevel = 0;
  gradientMapV = p.gradientMapV;
  textureScaleR = 1.0f;
  textureR = (DDSTexture *) 0;
  if (textureCnt >= 1)
    textureD = textures[0];
  if (textureCnt >= 4)
    textureG = textures[3];
  if (!(textureD && textureCnt >= 2 && textures[1] &&
        ((textureD->getWidth() * textures[1]->getHeight())
         == (textureD->getHeight() * textures[1]->getWidth()))))
  {
    if ((p.flags & 0x02) && textureCnt >= 2 && textures[1])
      textureD = textures[1];
    else
      return;
  }
  textureN = textures[1];
  textureScaleN = float(textureN->getWidth()) / float(textureD->getWidth());
  if (!(textureCnt >= 5 && textures[4]))
    return;
  textureE = textures[4];
  if (!(textureCnt >= 9 && textures[8]))
  {
    reflectionLevel = short(Plot3D_TriShape::calculateLighting(
                                0.0f, 0.0f, -1.0f, 0.707107f, 0.707107f, 0.0f,
                                lightingPolynomial));
    return;
  }
  textureR = textures[8];
  reflectionLevel = short(Plot3D_TriShape::calculateLighting(
                              0.0f, 0.0f, -1.0f, 0.684653f, 0.684653f, -0.25f,
                              lightingPolynomial));
  textureScaleR = float(textureR->getWidth()) / float(textureD->getWidth());
}

unsigned int Plot3D_TriShape::Plot3DTS_Base::gradientMap(unsigned int c) const
{
  unsigned int  u = (c & 0xFFU) + ((c >> 7) & 0x01FEU) + ((c >> 16) & 0xFFU);
  u = ((u + 2U) * (unsigned int) (textureG->getWidth() - 1)) >> 2;
  unsigned int  v =
      ((255U - gradientMapV) * (unsigned int) textureG->getHeight()) >> 8;
  unsigned int  c0 = textureG->getPixelN(int(u >> 8), int(v), 0);
  unsigned int  c1 = textureG->getPixelN(int(u >> 8) + 1, int(v), 0);
  return blendRGBA32(c0, c1, int(u & 0xFFU));
}

inline int Plot3D_TriShape::Plot3DTS_Base::normalMap(
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

inline unsigned int Plot3D_TriShape::Plot3DTS_Base::environmentMap(
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

void Plot3D_TriShape::Plot3DTS_Water::drawPixel(int x, int y, const ColorV6& z)
{
  if (x < 0 || x >= width || y < 0 || y >= height || z.v0 < 0.0f)
    return;
  size_t  offs = size_t(y) * size_t(width) + size_t(x);
  if (outBufZ[offs] <= z.v0)
    return;
  outBufZ[offs] = z.v0;
  unsigned int  c = outBufRGBW[offs];
  if (BRANCH_EXPECT(!((c + 0x01000000U) & 0xFE000000U), true))
  {
    // convert from R8G8B8 to R5G6B5
    c = c & 0x00FEFEFEU;
    c = c + ((unsigned int) ((x & 1) | (((x ^ y) & 1) << 1)) * 0x00020102U);
    c = c - ((c >> 7) & 0x00020202U);
    c = ((c >> 3) & 0x001FU) | ((c >> 5) & 0x07E0U) | ((c >> 8) & 0xF800U);
  }
  c = c & 0xFFFFU;
  unsigned int  n = 0xFFFF8080U;
  if (textureN)
    n = textureN->getPixelT(z.v2, z.v3, mipLevel);
  float   normalX = z.v4;
  float   normalY = z.v5;
  float   normalZ = z.v1;
  (void) normalMap(normalX, normalY, normalZ, n);
  c = c | ((unsigned int) (roundFloat(normalX * 126.0f) + 128) << 16);
  c = c | ((unsigned int) (roundFloat(normalY * 126.0f) + 128) << 24);
  outBufRGBW[offs] = c;
}

inline void Plot3D_TriShape::Plot3DTS_TextureT::drawPixel(int x, int y,
                                                          const ColorV6& z)
{
  if (x < 0 || x >= width || y < 0 || y >= height || z.v0 < 0.0f)
    return;
  size_t  offs = size_t(y) * size_t(width) + size_t(x);
  if (outBufZ[offs] <= z.v0)
    return;
  unsigned int  c = textureD->getPixelT(z.v2, z.v3, mipLevel);
  if (BRANCH_EXPECT(textureG, false))
    c = gradientMap(c);
  if (c < alphaThreshold)
    return;
  outBufZ[offs] = z.v0;
  int     l = int(Plot3D_TriShape::calculateLighting(
                      z.v4, z.v5, z.v1, lightX, lightY, lightZ,
                      lightingPolynomial));
  outBufRGBW[offs] = Plot3D_TriShape::multiplyWithLight(c, 0U, l);
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
  if (BRANCH_EXPECT(textureG, false))
    c = gradientMap(c);
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

void Plot3D_TriShape::Plot3DTS_NormalsEnvT::drawPixel(int x, int y,
                                                      const ColorV6& z)
{
  if (x < 0 || x >= width || y < 0 || y >= height || z.v0 < 0.0f)
    return;
  size_t  offs = size_t(y) * size_t(width) + size_t(x);
  if (outBufZ[offs] <= z.v0)
    return;
  unsigned int  c = textureD->getPixelT(z.v2, z.v3, mipLevel);
  if (BRANCH_EXPECT(textureG, false))
    c = gradientMap(c);
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
  if (BRANCH_EXPECT(textureG, false))
    c = gradientMap(c);
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

void Plot3D_TriShape::calculateWaterUV(
    const NIFFile::NIFVertexTransform& modelTransform)
{
  textureOffsetU = 0.0f;
  textureOffsetV = 0.0f;
  textureScaleU = 1.0f;
  textureScaleV = 1.0f;
  float   u0 = 0.0f;
  float   v0 = 0.0f;
  for (size_t i = 0; i < vertexCnt; i++)
  {
    NIFFile::NIFVertex  v(vertexData[i]);
    modelTransform.transformXYZ(v.x, v.y, v.z);
    float   tmpU = v.x * (2.0f / 4096.0f);
    float   tmpV = v.y * (2.0f / 4096.0f);
    if (!i)
    {
      u0 = float(roundFloat(tmpU));
      v0 = float(roundFloat(tmpV));
    }
    vertexBuf[i].u = convertToFloat16(tmpU - u0);
    vertexBuf[i].v = convertToFloat16(tmpV - v0);
  }
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
  if (flags & 0x08)                     // decal
    vt.offsZ = vt.offsZ - 0.5f;
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
  viewTransform.rotateXYZ(lightX, lightY, lightZ);
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
  if (flags & 0x02)
    calculateWaterUV(modelTransform);
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
    if (!(flags & 0x20))
    {
      if ((!(flags & 0x10) &&
           ((v1.x - v0.x) * (v2.y - v0.y)) > ((v2.x - v0.x) * (v1.y - v0.y))) ||
          ((flags & 0x10) &&
           ((v1.x - v0.x) * (v2.y - v0.y)) < ((v2.x - v0.x) * (v1.y - v0.y))))
      {
        continue;
      }
    }
    else if (!(flags & 0x10) &&
             v0.normalZ > 0.0f && v1.normalZ > 0.0f && v2.normalZ > 0.0f)
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
  gradientMapV = t.gradientMapV;
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
    if (tmp >= 0.875f && tmp <= 1.125f)
      tmp = (tmp * 0.378732f - 1.260466f) * tmp + 1.881728f;
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
  bool    isWater = bool(flags & 0x02);
  if (!((textureCnt >= 1 && textures[0]) || isWater))
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
  plot3d.mipLevel = 15.0f;
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
    z0.v1 = v0.normalZ;
    z0.v2 = v0.getU() * textureScaleU + textureOffsetU;
    z0.v3 = v0.getV() * textureScaleV + textureOffsetV;
    z0.v4 = v0.normalX;
    z0.v5 = v0.normalY;
    z1.v0 = v1.z;
    z1.v1 = v1.normalZ;
    z1.v2 = v1.getU() * textureScaleU + textureOffsetU;
    z1.v3 = v1.getV() * textureScaleV + textureOffsetV;
    z1.v4 = v1.normalX;
    z1.v5 = v1.normalY;
    z2.v0 = v2.z;
    z2.v1 = v2.normalZ;
    z2.v2 = v2.getU() * textureScaleU + textureOffsetU;
    z2.v3 = v2.getV() * textureScaleV + textureOffsetV;
    z2.v4 = v2.normalX;
    z2.v5 = v2.normalY;
    if (BRANCH_EXPECT(textureD, true))
    {
      float   xyArea2 = float(std::fabs(((v1.x - v0.x) * (v2.y - v0.y))
                                        - ((v2.x - v0.x) * (v1.y - v0.y))));
      float   uvArea2 = float(std::fabs(((z1.v2 - z0.v2) * (z2.v3 - z0.v3))
                                        - ((z2.v2 - z0.v2) * (z1.v3 - z0.v3))));
      float   mipLevel = 15.0f;
      bool    integerMipLevel = true;
      if (xyArea2 > uvArea2)
      {
        float   txtW = float(textureD->getWidth());
        float   txtH = float(textureD->getHeight());
        uvArea2 *= (txtW * txtH);
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
    }
    if (BRANCH_EXPECT(isWater, false))
    {
      reinterpret_cast< Plot3D< Plot3DTS_Water, ColorV6 > * >(
          &plot3d)->drawTriangle(x0, y0, z0, x1, y1, z1, x2, y2, z2);
    }
    else if (!textureN)
    {
      reinterpret_cast< Plot3D< Plot3DTS_TextureT, ColorV6 > * >(
          &plot3d)->drawTriangle(x0, y0, z0, x1, y1, z1, x2, y2, z2);
    }
    else if (!textureE)
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
}

void Plot3D_TriShape::renderWater(
    const NIFFile::NIFVertexTransform& viewTransform,
    unsigned int waterColor, float lightX, float lightY, float lightZ,
    const DDSTexture *envMap, float reflectionLevel)
{
  viewTransform.rotateXYZ(lightX, lightY, lightZ);
  Plot3DTS_Base plot3d(*this, lightX, lightY, lightZ, (DDSTexture **) 0, 0);
  plot3d.textureE = envMap;
  plot3d.reflectionLevel =
      short(roundFloat(lightingPolynomial[0] * reflectionLevel));
  unsigned int  *p = bufRGBW;
  for (int y = 0; y < height; y++)
  {
    for (int x = 0; x < width; x++, p++)
    {
      unsigned int  c = *p;
      if (!((c + 0x01000000U) & 0xFE000000U))
        continue;
      float   normalX = float(int((c >> 16) & 0xFFU) - 128) * (1.0f / 126.0f);
      float   normalY = float(int((c >> 24) & 0xFFU) - 128) * (1.0f / 126.0f);
      float   normalZ = (normalX * normalX) + (normalY * normalY);
      if (normalZ >= 1.0f)
      {
        normalZ = (3.0f - normalZ) * 0.5f;  // approximates 1.0 / sqrt(normalZ)
        normalX = normalX * normalZ;
        normalY = normalY * normalZ;
        normalZ = 0.0f;
      }
      else
      {
        normalZ = -(float(std::sqrt(1.0001f - normalZ)));
      }
      int     l = int(calculateLighting(
                          normalX, normalY, normalZ,
                          lightX, lightY, lightZ, lightingPolynomial));
      unsigned int  tmp = multiplyWithLight(waterColor, 0U, l);
      c = ((c & 0x001FU) << 3) | ((c & 0x07E0U) << 5) | ((c & 0xF800U) << 8);
      c = blendRGBA32(c, tmp, int(waterColor >> 24));
      if (envMap)
        c = plot3d.environmentMap(c, normalX, normalY, normalZ, x, y);
      *p = c;
    }
  }
}

