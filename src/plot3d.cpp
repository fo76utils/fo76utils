
#include "common.hpp"
#include "plot3d.hpp"
#include "fp32vec4.hpp"

#include <algorithm>

// vertex coordinates closer than this to exact integers are rounded
#ifndef VERTEX_XY_SNAP
#  define VERTEX_XY_SNAP  0.03125f
#endif

const float Plot3D_TriShape::defaultLightingPolynomial[6] =
{
  0.672235f, 0.997428f, 0.009355f, -0.771812f, 0.108711f, 0.369682f
};

Plot3D_TriShape::VertexTransform::VertexTransform(
    const NIFFile::NIFVertexTransform& vt)
  : rotateX(vt.rotateXX, vt.rotateXY, vt.rotateXZ, vt.offsX),
    rotateY(vt.rotateYX, vt.rotateYY, vt.rotateYZ, vt.offsY),
    rotateZ(vt.rotateZX, vt.rotateZY, vt.rotateZZ, vt.offsZ),
    scale(vt.scale)
{
}

FloatVector4 Plot3D_TriShape::VertexTransform::transformXYZ(
    FloatVector4 v) const
{
  FloatVector4  tmp(v);
  tmp *= scale;
  tmp.setElement(3, 1.0f);
  return FloatVector4(tmp.dotProduct(rotateX), tmp.dotProduct(rotateY),
                      tmp.dotProduct(rotateZ), 0.0f);
}

FloatVector4 Plot3D_TriShape::VertexTransform::rotateXYZ(FloatVector4 v) const
{
  FloatVector4  tmp(v);
  tmp.setElement(3, 0.0f);
  return FloatVector4(tmp.dotProduct(rotateX), tmp.dotProduct(rotateY),
                      tmp.dotProduct(rotateZ), 0.0f);
}

Plot3D_TriShape::VertexTransform& Plot3D_TriShape::VertexTransform::operator*=(
    const VertexTransform& r)
{
  FloatVector4  offsXYZ(rotateX[3], rotateY[3], rotateZ[3], 0.0f);
  offsXYZ = r.transformXYZ(offsXYZ);
  FloatVector4  tmpX(rotateX[0], rotateY[0], rotateZ[0], 0.0f);
  FloatVector4  tmpY(rotateX[1], rotateY[1], rotateZ[1], 0.0f);
  FloatVector4  tmpZ(rotateX[2], rotateY[2], rotateZ[2], 0.0f);
  rotateX = FloatVector4(tmpX.dotProduct(r.rotateX), tmpY.dotProduct(r.rotateX),
                         tmpZ.dotProduct(r.rotateX), offsXYZ[0]);
  rotateY = FloatVector4(tmpX.dotProduct(r.rotateY), tmpY.dotProduct(r.rotateY),
                         tmpZ.dotProduct(r.rotateY), offsXYZ[1]);
  rotateZ = FloatVector4(tmpX.dotProduct(r.rotateZ), tmpY.dotProduct(r.rotateZ),
                         tmpZ.dotProduct(r.rotateZ), offsXYZ[2]);
  scale = scale * r.scale;
  return (*this);
}

size_t Plot3D_TriShape::transformVertexData(
    const NIFFile::NIFVertexTransform& modelTransform,
    const NIFFile::NIFVertexTransform& viewTransform)
{
  vertexBuf.clear();
  if (vertexBuf.capacity() < vertexCnt)
    vertexBuf.reserve(vertexCnt);
  triangleBuf.clear();
  if (triangleBuf.capacity() < triangleCnt)
    triangleBuf.reserve(triangleCnt);
  VertexTransform mt(vertexTransform);
  mt *= VertexTransform(modelTransform);
  VertexTransform vt(viewTransform);
  if (flags & 0x08)                     // decal
    vt.rotateZ.setElement(3, vt.rotateZ[3] - 0.0625f);
  VertexTransform xt(mt);
  xt *= vt;
  {
    const float   tmp = float(0x40000000);
    FloatVector4  boundsMin(tmp, tmp, tmp, tmp);
    FloatVector4  boundsMax(-tmp, -tmp, -tmp, -tmp);
    for (size_t i = 0; i < vertexCnt; i++)
    {
      Vertex  v(vertexData[i]);
      v.xyz = xt.transformXYZ(v.xyz);
      boundsMin.minValues(v.xyz);
      boundsMax.maxValues(v.xyz);
      vertexBuf.push_back(v);
    }
    if (boundsMin[0] >= (float(width) - 0.5f) || boundsMax[0] < -0.5f ||
        boundsMin[1] >= (float(height) - 0.5f) || boundsMax[1] < -0.5f ||
        boundsMin[2] >= 16777216.0f || boundsMax[2] < 0.0f)
    {
      return 0;
    }
  }
  lightVector = vt.rotateXYZ(lightVector);
  lightVector.normalize();
  for (size_t i = 0; i < vertexCnt; i++)
  {
    Vertex& v = vertexBuf[i];
#ifdef VERTEX_XY_SNAP
    float   x = float(roundFloat(v.xyz[0]));
    float   y = float(roundFloat(v.xyz[1]));
    if (float(std::fabs(v.xyz[0] - x)) < VERTEX_XY_SNAP)
      v.xyz.setElement(0, x);
    if (float(std::fabs(v.xyz[1] - y)) < VERTEX_XY_SNAP)
      v.xyz.setElement(1, y);
#endif
    if (flags & 0x20)                   // tree: ignore vertex alpha
      v.vertexColor.setElement(3, 1.0f);
    float   txtU = v.bitangent[3];
    float   txtV = v.tangent[3];
    if (BRANCH_EXPECT((flags & 0x02), false))   // water
    {
      FloatVector4  tmp(vertexData[i].x, vertexData[i].y, vertexData[i].z,
                        0.0f);
      tmp = mt.transformXYZ(tmp);
      const float waterUVScale = 2.0f / 4096.0f;
      txtU = tmp[0] * waterUVScale;
      txtV = tmp[1] * waterUVScale;
      v.normal = FloatVector4(vertexData[i].normal);
      v.normal *= (1.0f / 127.5f);
      v.normal -= 1.0f;
      v.normal = mt.rotateXYZ(v.normal);
      tmp = FloatVector4(v.normal[2], 0.0f, -(v.normal[0]), 0.0f);
      tmp.normalizeFast();
      v.bitangent = vt.rotateXYZ(tmp);
      tmp = FloatVector4(0.0f, v.normal[2], -(v.normal[1]), 0.0f);
      tmp.normalizeFast();
      v.tangent = vt.rotateXYZ(tmp);
      v.normal = vt.rotateXYZ(v.normal);
    }
    else
    {
      txtU = txtU * textureScaleU + textureOffsetU;
      txtV = txtV * textureScaleV + textureOffsetV;
      v.bitangent = xt.rotateXYZ(v.bitangent);
      v.tangent = xt.rotateXYZ(v.tangent);
      v.normal = xt.rotateXYZ(v.normal);
    }
    v.bitangent.setElement(3, txtU);
    v.tangent.setElement(3, txtV);
  }
  for (size_t i = 0; i < triangleCnt; i++)
  {
    unsigned int  n0 = triangleData[i].v0;
    unsigned int  n1 = triangleData[i].v1;
    unsigned int  n2 = triangleData[i].v2;
    if (n0 >= vertexCnt || n1 >= vertexCnt || n2 >= vertexCnt)
      continue;
    Vertex& v0 = vertexBuf[n0];
    Vertex& v1 = vertexBuf[n1];
    Vertex& v2 = vertexBuf[n2];
    float   x0 = v0.xyz[0];
    float   y0 = v0.xyz[1];
    float   x1 = v1.xyz[0];
    float   y1 = v1.xyz[1];
    float   x2 = v2.xyz[0];
    float   y2 = v2.xyz[1];
    if (!(flags & 0x10))
    {
      if (((x1 - x0) * (y2 - y0)) > ((x2 - x0) * (y1 - y0)))
        continue;               // cull if vertices are not in CCW order
    }
    FloatVector4  boundsMin(v0.xyz);
    FloatVector4  boundsMax(v0.xyz);
    boundsMin.minValues(v1.xyz);
    boundsMax.maxValues(v1.xyz);
    boundsMin.minValues(v2.xyz);
    boundsMax.maxValues(v2.xyz);
    if (!(boundsMin[0] < float(width) && boundsMin[1] < float(height) &&
          boundsMax[0] >= 0.0f && boundsMax[1] >= 0.0f && boundsMax[2] >= 0.0f))
    {
      continue;                 // out of bounds
    }
#ifdef VERTEX_XY_SNAP
    int     xMin = int(boundsMin[0]);
    int     yMin = int(boundsMin[1]);
    if ((xMin == int(boundsMax[0]) && boundsMin[0] != float(xMin)) ||
        (yMin == int(boundsMax[1]) && boundsMin[1] != float(yMin)))
    {
      continue;                 // no sampling point within the triangle bounds
    }
#endif
    Triangle  tmp;
    tmp.z = v0.xyz[2] + v1.xyz[2] + v2.xyz[2];
    tmp.n = (unsigned int) i;
    triangleBuf.push_back(tmp);
  }
  std::stable_sort(triangleBuf.begin(), triangleBuf.end());
  return triangleBuf.size();
}

inline float Plot3D_TriShape::getLightLevel(float d) const
{
  FloatVector4  tmp(lightingPolynomial[2], lightingPolynomial[3],
                    lightingPolynomial[4], lightingPolynomial[5]);
  float   d2 = d * d;
  FloatVector4  tmp2(d2, d2, d2, d2);
  tmp2 *= FloatVector4(1.0f, d, d2, d2 * d);
  float   y = d * lightingPolynomial[1] + lightingPolynomial[0];
  y += tmp2.dotProduct(tmp);
  return (y > 0.0f ? (y < 4.0f ? y : 4.0f) : 0.0f);
}

inline float Plot3D_TriShape::normalMap(Vertex& v, FloatVector4 n) const
{
  float   x = n[0] * (1.0f / 127.5f) - 1.0f;
  float   y = n[1] * (1.0f / 127.5f) - 1.0f;
  // calculate Z normal
  float   z = 1.0f - ((x * x) + (y * y));
  if (BRANCH_EXPECT((z > 0.0f), true))
    z = float(std::sqrt(z));
  else
    z = 0.0f;
  FloatVector4  tmpB(v.bitangent);
  tmpB.normalizeFast();
  tmpB *= x;
  FloatVector4  tmpT(v.tangent);
  tmpT.normalizeFast();
  tmpT *= y;
  FloatVector4  tmpN(v.normal);
  tmpN.normalizeFast();
  tmpN *= z;
  tmpN += tmpB;
  tmpN += tmpT;
  // normalize and store result
  tmpN.normalize(invNormals);
  v.normal = tmpN;
  return getLightLevel(tmpN.dotProduct(lightVector));
}

inline FloatVector4 Plot3D_TriShape::environmentMap(
    const Vertex& v, int x, int y, float smoothness, float *specPtr) const
{
  float   r = 1.0f - smoothness;        // roughness
  r = (r > 0.0f ? r : 0.0f);
  float   m = r * float(textureE->getMaxMipLevel());
  // view vector
  FloatVector4  viewVector(float(x), float(y), 0.0f, 0.0f);
  viewVector += envMapOffs;
  // reflect
  FloatVector4  tmp2(v.normal);
  float   tmp = viewVector.dotProduct(tmp2) * 2.0f;
  tmp2 *= tmp;
  viewVector -= tmp2;
  viewVector.normalize();
  if (specPtr && specularLevel > 0.0f)
  {
    *specPtr = 0.0f;
    // calculate specular reflection
    float   s = viewVector.dotProduct(lightVector);
    if (s > 0.0f && v.normal.dotProduct(lightVector) > 0.0f)
    {
      if (s >= 1.0f)
        s = 1.0f;
      else
        s = float(std::pow(s, float(std::pow(2.0f, 9.0f * (1.0f - r)))));
      *specPtr = s;
    }
  }
  // inverse rotation by view matrix
  float   tmpX = viewVector.dotProduct(viewTransformInvX);
  float   tmpY = viewVector.dotProduct(viewTransformInvY);
  float   tmpZ = viewVector.dotProduct(viewTransformInvZ);
  FloatVector4  e(textureE->cubeMap(tmpX, tmpY, tmpZ, m));
  e *= (1.0f / 255.0f);
  return e;
}

void Plot3D_TriShape::drawPixel_Water(
    Plot3D_TriShape& p, int x, int y, float txtU, float txtV, Vertex& z)
{
  size_t  offs = size_t(y) * size_t(p.width) + size_t(x);
  p.bufZ[offs] = z.xyz[2];
  unsigned int  c = p.bufRGBW[offs];
  if (BRANCH_EXPECT(!((c + 0x01000000U) & 0xFE000000U), true))
  {
    // convert from R8G8B8 to R5G6B5 with dithering
    c = c & 0x00FEFEFEU;
    c = c + ((unsigned int) ((x & 1) | (((x ^ y) & 1) << 1)) * 0x00020102U);
    c = c - ((c >> 7) & 0x00020202U);
    c = ((c >> 3) & 0x001FU) | ((c >> 5) & 0x07E0U) | ((c >> 8) & 0xF800U);
  }
  c = c & 0xFFFFU;
  if (BRANCH_EXPECT(p.textureN, true))
    (void) p.normalMap(z, p.textureN->getPixelT(txtU, txtV, p.mipLevel));
  else
    z.normal.normalize(p.invNormals);
  c = c | ((unsigned int) (roundFloat(z.normal[0] * 126.0f) + 128) << 16);
  c = c | ((unsigned int) (roundFloat(z.normal[1] * 126.0f) + 128) << 24);
  p.bufRGBW[offs] = c;
}

inline FloatVector4 Plot3D_TriShape::getDiffuseColor(
    float txtU, float txtV, const Vertex& z, bool& alphaFlag) const
{
  FloatVector4  c(textureD->getPixelT(txtU, txtV, mipLevel));
  FloatVector4  vColor(z.vertexColor);
  float   a = c[3] * vColor[3];
  alphaFlag = !(a < alphaThresholdFloat);
  if (BRANCH_EXPECT(!alphaFlag, false))
    return c;
  if (BRANCH_EXPECT(textureG, false))
  {
    float   u = c[1] * float(textureG->getWidth() - 1) * (1.0f / 255.0f);
    float   v = (float(int(gradientMapV) - 255) * (1.0f / 255.0f) + vColor[0])
                * float(textureG->getHeight() - 1);
    c = textureG->getPixelB(u, v, 0);
  }
  else
  {
    if (BRANCH_EXPECT(gammaCorrectEnabled, true))
      vColor.squareRoot();
    c *= vColor;
  }
  c.setElement(3, a);
  return c;
}

void Plot3D_TriShape::drawPixel_Debug(
    Plot3D_TriShape& p, int x, int y, float txtU, float txtV, Vertex& z)
{
  size_t  offs = size_t(y) * size_t(p.width) + size_t(x);
  FloatVector4  c(0.0f, 0.0f, 0.0f, 0.0f);
  if (BRANCH_EXPECT(p.textureD, true))
  {
    bool    alphaFlag = false;
    c = p.getDiffuseColor(txtU, txtV, z, alphaFlag);
    if (BRANCH_EXPECT(!alphaFlag, false))
      return;
    if (p.debugMode == 5)
      c = FloatVector4(0xFFB8B8B8U);    // full scale with default polynomial
  }
  p.bufZ[offs] = z.xyz[2];
  if (!(p.debugMode == 0 || p.debugMode == 3 || p.debugMode == 5))
  {
    unsigned int  tmp = 0xFF000000U;
    if (p.debugMode & 0x80000000U)
      tmp = tmp | p.debugMode;
    else if (p.debugMode == 2)
      tmp = tmp | (unsigned int) roundFloat(z.xyz[2] * 16.0f);
    else
      tmp = tmp | (unsigned int) c;
    if (p.debugMode != 4)
      tmp = (tmp & 0xFF00FF00U) | ((tmp & 0xFFU) << 16) | ((tmp >> 16) & 0xFFU);
    p.bufRGBW[offs] = tmp;
    return;
  }
  FloatVector4  n(127.5f, 127.5f, 0.0f, 0.0f);
  if (p.textureN)
    n = p.textureN->getPixelT(txtU, txtV, p.mipLevel + p.mipOffsetN);
  c *= p.normalMap(z, n);
  if (p.debugMode == 3)
  {
    c = z.normal;
    c *= FloatVector4(127.5f, -127.5f, -127.5f, 0.0f);
    c += FloatVector4(127.5f, 127.5f, 127.5f, 0.0f);
  }
  p.bufRGBW[offs] = (unsigned int) c | 0xFF000000U;
}

float Plot3D_TriShape::calculateLighting(FloatVector4 normal) const
{
  FloatVector4  tmp(normal);
  tmp.normalize(invNormals);
  return getLightLevel(tmp.dotProduct(lightVector));
}

void Plot3D_TriShape::drawPixel_Diffuse(
    Plot3D_TriShape& p, int x, int y, float txtU, float txtV, Vertex& z)
{
  size_t  offs = size_t(y) * size_t(p.width) + size_t(x);
  bool    alphaFlag = false;
  FloatVector4  c(p.getDiffuseColor(txtU, txtV, z, alphaFlag));
  if (BRANCH_EXPECT(!alphaFlag, false))
    return;
  p.bufZ[offs] = z.xyz[2];
  c *= p.calculateLighting(z.normal);
  p.bufRGBW[offs] = (unsigned int) c | 0xFF000000U;
}

void Plot3D_TriShape::drawPixel_Normal(
    Plot3D_TriShape& p, int x, int y, float txtU, float txtV, Vertex& z)
{
  size_t  offs = size_t(y) * size_t(p.width) + size_t(x);
  bool    alphaFlag = false;
  FloatVector4  c(p.getDiffuseColor(txtU, txtV, z, alphaFlag));
  if (BRANCH_EXPECT(!alphaFlag, false))
    return;
  p.bufZ[offs] = z.xyz[2];
  FloatVector4  n(p.textureN->getPixelT(txtU, txtV, p.mipLevel + p.mipOffsetN));
  c *= p.normalMap(z, n);
  p.bufRGBW[offs] = (unsigned int) c | 0xFF000000U;
}

void Plot3D_TriShape::drawPixel_NormalEnv(
    Plot3D_TriShape& p, int x, int y, float txtU, float txtV, Vertex& z)
{
  size_t  offs = size_t(y) * size_t(p.width) + size_t(x);
  bool    alphaFlag = false;
  FloatVector4  c(p.getDiffuseColor(txtU, txtV, z, alphaFlag));
  if (BRANCH_EXPECT(!alphaFlag, false))
    return;
  p.bufZ[offs] = z.xyz[2];
  FloatVector4  n(p.textureN->getPixelT(txtU, txtV, p.mipLevel + p.mipOffsetN));
  c *= p.normalMap(z, n);
  float   smoothness = float(int(p.specularSmoothness)) * (1.0f / 255.0f);
  float   specular = 0.0f;
  FloatVector4  e(p.environmentMap(z, x, y, smoothness, &specular));
  float   specLevel = n[3];
  e *= (p.reflectionLevel * specLevel);
  c += e;
  specular *= (p.specularLevel * specLevel);
  if (specular > 0.0f)
  {
    FloatVector4  tmp(p.specularColorFloat);
    tmp *= specular;
    c += tmp;
  }
  p.bufRGBW[offs] = (unsigned int) c | 0xFF000000U;
}

void Plot3D_TriShape::drawPixel_NormalEnvM(
    Plot3D_TriShape& p, int x, int y, float txtU, float txtV, Vertex& z)
{
  size_t  offs = size_t(y) * size_t(p.width) + size_t(x);
  bool    alphaFlag = false;
  FloatVector4  c(p.getDiffuseColor(txtU, txtV, z, alphaFlag));
  if (BRANCH_EXPECT(!alphaFlag, false))
    return;
  p.bufZ[offs] = z.xyz[2];
  FloatVector4  n(p.textureN->getPixelT(txtU, txtV, p.mipLevel + p.mipOffsetN));
  c *= p.normalMap(z, n);
  FloatVector4  m(p.textureS->getPixelT(txtU, txtV, p.mipLevel + p.mipOffsetS));
  float   smoothness = float(int(p.specularSmoothness)) * (1.0f / 255.0f);
  float   specular = 0.0f;
  FloatVector4  e(p.environmentMap(z, x, y, smoothness, &specular));
  float   specLevel = m[0];
  e *= (p.reflectionLevel * specLevel);
  c += e;
  specular *= (p.specularLevel * specLevel);
  if (specular > 0.0f)
  {
    FloatVector4  tmp(p.specularColorFloat);
    tmp *= specular;
    c += tmp;
  }
  p.bufRGBW[offs] = (unsigned int) c | 0xFF000000U;
}

void Plot3D_TriShape::drawPixel_NormalEnvS(
    Plot3D_TriShape& p, int x, int y, float txtU, float txtV, Vertex& z)
{
  size_t  offs = size_t(y) * size_t(p.width) + size_t(x);
  bool    alphaFlag = false;
  FloatVector4  c(p.getDiffuseColor(txtU, txtV, z, alphaFlag));
  if (BRANCH_EXPECT(!alphaFlag, false))
    return;
  p.bufZ[offs] = z.xyz[2];
  FloatVector4  n(p.textureN->getPixelT_2(txtU, txtV, p.mipLevel + p.mipOffsetN,
                                          p.textureS));
  c *= p.normalMap(z, n);
  float   smoothness =
      float(int(p.specularSmoothness)) * n[3] * (1.0f / (255.0f * 255.0f));
  float   specLevel = n[2];
  float   specular = 0.0f;
  FloatVector4  e(p.environmentMap(z, x, y, smoothness, &specular));
  e *= (p.reflectionLevel * specLevel);
  c += e;
  specular *= (p.specularLevel * specLevel);
  if (specular > 0.0f)
  {
    FloatVector4  tmp(p.specularColorFloat);
    tmp *= (specular * (smoothness * 2.0f));
    c += tmp;
  }
  p.bufRGBW[offs] = (unsigned int) c | 0xFF000000U;
}

void Plot3D_TriShape::drawPixel_NormalRefl(
    Plot3D_TriShape& p, int x, int y, float txtU, float txtV, Vertex& z)
{
  size_t  offs = size_t(y) * size_t(p.width) + size_t(x);
  bool    alphaFlag = false;
  FloatVector4  c(p.getDiffuseColor(txtU, txtV, z, alphaFlag));
  if (BRANCH_EXPECT(!alphaFlag, false))
    return;
  p.bufZ[offs] = z.xyz[2];
  FloatVector4  n(p.textureN->getPixelT_2(txtU, txtV, p.mipLevel + p.mipOffsetN,
                                          p.textureS));
  FloatVector4  r(p.textureR->getPixelT(txtU, txtV, p.mipLevel + p.mipOffsetR));
  r *= (1.0f / 255.0f);
  FloatVector4  tmp(r[1], r[1], r[1], r[1]);
  tmp.maxValues(r);
  tmp.maxValues(FloatVector4(r[2], r[2], r[2], r[2]));
  float   m = 1.0f - tmp[0];
  c *= (p.normalMap(z, n) * m);
  float   smoothness = n[2];
  float   specLevel = n[3];
  if (BRANCH_EXPECT(!p.textureS, false))
  {
    smoothness = 127.5f;
    specLevel = 255.0f;
  }
  smoothness = smoothness * float(int(p.specularSmoothness))
               * (1.0f / (255.0f * 255.0f));
  float   specular = 0.0f;
  FloatVector4  e(p.environmentMap(z, x, y, smoothness, &specular));
  e *= (p.reflectionLevel * specLevel);
  e *= r;
  c += e;
  specular *= (p.specularLevel * specLevel);
  if (specular > 0.0f)
  {
    specular *= (smoothness * smoothness * (m + 1.0f));
    tmp = p.specularColorFloat;
    r *= (1.0f - m);
    tmp *= m;
    tmp += r;
    tmp *= specular;
    c += tmp;
  }
  p.bufRGBW[offs] = (unsigned int) c | 0xFF000000U;
}

inline FloatVector4 Plot3D_TriShape::interpolateVectors(
    FloatVector4 v0, FloatVector4 v1, FloatVector4 v2,
    float w0, float w1, float w2)
{
  FloatVector4  tmp0(v0);
  FloatVector4  tmp1(v1);
  FloatVector4  tmp2(v2);
  tmp0 *= w0;
  tmp1 *= w1;
  tmp2 *= w2;
  tmp0 += tmp1;
  tmp0 += tmp2;
  return tmp0;
}

inline void Plot3D_TriShape::drawPixel(
    int x, int y, float txtScaleU, float txtScaleV, Vertex& v,
    const Vertex& v0, const Vertex& v1, const Vertex& v2,
    float w0, float w1, float w2)
{
  if (x < 0 || x >= width || v.xyz[2] < 0.0f ||
      bufZ[size_t(y) * size_t(width) + size_t(x)] <= v.xyz[2])
  {
    return;
  }
  v.bitangent =
      interpolateVectors(v0.bitangent, v1.bitangent, v2.bitangent, w0, w1, w2);
  float   txtU = v.bitangent[3] * txtScaleU;
  v.bitangent.setElement(3, 0.0f);
  v.tangent =
      interpolateVectors(v0.tangent, v1.tangent, v2.tangent, w0, w1, w2);
  float   txtV = v.tangent[3] * txtScaleV;
  v.tangent.setElement(3, 0.0f);
  v.normal = interpolateVectors(v0.normal, v1.normal, v2.normal, w0, w1, w2);
  v.vertexColor =
      interpolateVectors(v0.vertexColor, v1.vertexColor, v2.vertexColor,
                         w0, w1, w2);
  drawPixelFunction(*this, x, y, txtU, txtV, v);
}

void Plot3D_TriShape::drawLine(Vertex& v, const Vertex& v0, const Vertex& v1)
{
  int     x = roundFloat(v0.xyz[0]);
  int     y = roundFloat(v0.xyz[1]);
  if (float(std::fabs(v1.xyz[1] - v0.xyz[1])) >= (1.0f / 1024.0f))
  {
    double  dy1 = 1.0 / (double(v1.xyz[1]) - double(v0.xyz[1]));
    int     y1 = roundFloat(v1.xyz[1]);
    while (true)
    {
      double  w1 = (double(y) - double(v0.xyz[1])) * dy1;
      if (w1 > -0.000001 && w1 < 1.000001)
      {
        float   xf = (v1.xyz[0] - v0.xyz[0]) * float(w1) + v0.xyz[0];
        x = roundFloat(xf);
        if (float(std::fabs(xf - float(x))) < (1.0f / 1024.0f) &&
            y >= 0 && y < height)
        {
          float   w0 = float(1.0 - w1);
          v.xyz.setElement(2, (v0.xyz[2] * w0) + (v1.xyz[2] * float(w1)));
          drawPixel(x, y, 0.0f, 0.0f, v, v0, v1, v0, w0, float(w1), 0.0f);
        }
      }
      if (y == y1)
        break;
      y = y + (y < y1 ? 1 : -1);
    }
    return;
  }
  if (y < 0 || y >= height)
    return;
  if (float(std::fabs(v1.xyz[0] - v0.xyz[0])) >= (1.0f / 1024.0f))
  {
    double  dx1 = 1.0 / (double(v1.xyz[0]) - double(v0.xyz[0]));
    int     x1 = roundFloat(v1.xyz[0]);
    while (true)
    {
      double  w1 = (double(x) - double(v0.xyz[0])) * dx1;
      if (w1 > -0.000001 && w1 < 1.000001)
      {
        float   yf = (v1.xyz[1] - v0.xyz[1]) * float(w1) + v0.xyz[1];
        if (float(std::fabs(yf - float(y))) < (1.0f / 1024.0f))
        {
          float   w0 = float(1.0 - w1);
          v.xyz.setElement(2, (v0.xyz[2] * w0) + (v1.xyz[2] * float(w1)));
          drawPixel(x, y, 0.0f, 0.0f, v, v0, v1, v0, w0, float(w1), 0.0f);
        }
      }
      if (x == x1)
        break;
      x = x + (x < x1 ? 1 : -1);
    }
  }
  else if (float(std::fabs(v0.xyz[0] - float(x))) < (1.0f / 1024.0f) &&
           float(std::fabs(v0.xyz[1] - float(y))) < (1.0f / 1024.0f))
  {
    v.xyz.setElement(2, v0.xyz[2]);
    drawPixel(x, y, 0.0f, 0.0f, v, v0, v0, v0, 1.0f, 0.0f, 0.0f);
  }
}

void Plot3D_TriShape::drawTriangles()
{
  Vertex  v;
  for (size_t n = 0; n < triangleBuf.size(); n++)
  {
    mipLevel = 15.0f;
    const Vertex  *v0, *v1, *v2;
    {
      const Triangle& t = triangleBuf[n];
      v0 = &(vertexBuf.front()) + triangleData[t.n].v0;
      v1 = &(vertexBuf.front()) + triangleData[t.n].v1;
      v2 = &(vertexBuf.front()) + triangleData[t.n].v2;
    }
    float   xyArea2 = float(((double(v1->xyz[0]) - double(v0->xyz[0]))
                             * (double(v2->xyz[1]) - double(v0->xyz[1])))
                            - ((double(v2->xyz[0]) - double(v0->xyz[0]))
                               * (double(v1->xyz[1]) - double(v0->xyz[1]))));
    invNormals = (xyArea2 >= 0.0f);
    xyArea2 = float(std::fabs(xyArea2));
    if (xyArea2 < (1.0f / 1048576.0f))  // if area < 2^-21 square pixels
    {
      drawLine(v, *v0, *v1);
      drawLine(v, *v1, *v2);
      drawLine(v, *v2, *v0);
      continue;
    }
    // sort vertices by Y coordinate
    if (v0->xyz[1] > v1->xyz[1])
    {
      const Vertex  *tmp = v0;
      v0 = v1;
      v1 = tmp;
    }
    if (v1->xyz[1] > v2->xyz[1])
    {
      const Vertex  *tmp;
      if (v0->xyz[1] > v2->xyz[1])
      {
        tmp = v0;
        v0 = v2;
        v2 = tmp;
      }
      tmp = v1;
      v1 = v2;
      v2 = tmp;
    }
    double  x0 = double(v0->xyz[0]);
    double  y0 = double(v0->xyz[1]);
    double  x1 = double(v1->xyz[0]);
    double  y1 = double(v1->xyz[1]);
    double  x2 = double(v2->xyz[0]);
    double  y2 = double(v2->xyz[1]);
    double  r2xArea = 1.0 / (((x1 - x0) * (y2 - y0)) - ((x2 - x0) * (y1 - y0)));
    float   txtScaleU = 0.0f;
    float   txtScaleV = 0.0f;
    if (BRANCH_EXPECT(textureD, true))
    {
      float   txtU1d = v1->bitangent[3] - v0->bitangent[3];
      float   txtU2d = v2->bitangent[3] - v0->bitangent[3];
      float   txtV1d = v1->tangent[3] - v0->tangent[3];
      float   txtV2d = v2->tangent[3] - v0->tangent[3];
      float   uvArea2 = float(std::fabs((txtU1d * txtV2d) - (txtU2d * txtV1d)));
      if (xyArea2 > uvArea2)
      {
        float   txtW = float(textureD->getWidth());
        float   txtH = float(textureD->getHeight());
        uvArea2 *= (txtW * txtH);
        int     mipLevel_i = mipLevelMin;
        mipLevel = float(mipLevel_i);
        if ((uvArea2 * float(1 << ((-mipLevel_i) << 1))) > xyArea2)
        {
          // calculate base 4 logarithm of texel area / pixel area
          mipLevel = float(std::log2(float(std::fabs(r2xArea * uvArea2))))
                     * 0.5f;
          mipLevel_i = roundFloat(mipLevel);
          if (float(std::fabs(mipLevel - float(mipLevel_i))) < 0.0625f)
            mipLevel = float(mipLevel_i);
          mipLevel_i = int(float(std::floor(mipLevel)));
        }
        float   txtScale =
            float(262144 >> (mipLevel_i + 2)) * (1.0f / 65536.0f);
        txtScaleU = txtW * txtScale;
        txtScaleV = txtH * txtScale;
      }
    }
    double  dy2 = 1.0 / (y2 - y0);
    double  a1 = (y2 - y0) * r2xArea;
    double  b1 = -((x2 - x0) * a1);
    double  a2 = (y0 - y1) * r2xArea;
    double  b2 = 1.0 - ((x2 - x0) * a2);
    int     y = int(y0 + 0.9999995);
    int     yMax = int(y2 + 0.0000005);
    if (BRANCH_EXPECT((y > (height - 1) || yMax < 0), false))
      continue;
    y = (y > 0 ? y : 0);
    yMax = (yMax < (height - 1) ? yMax : (height - 1));
    if (a1 < 0.0)                       // negative X direction
    {
      do
      {
        double  yf = (double(y) - y0) * dy2;
        int     x = roundFloat(float((x2 - x0) * yf + x0));
        double  w1 = ((double(x) - x0) * a1) + (yf * b1);
        if (!(w1 >= -0.000001))
        {
          w1 -= a1;
          x--;
        }
        double  w2 = ((double(x) - x0) * a2) + (yf * b2);
        double  w0 = 1.0 - (w1 + w2);
        while (!(!(w0 >= -0.000001) | !(w2 >= -0.000001)))
        {
          float   z = (v0->xyz[2] * float(w0)) + (v1->xyz[2] * float(w1))
                      + (v2->xyz[2] * float(w2));
          v.xyz.setElement(2, z);
          drawPixel(x, y, txtScaleU, txtScaleV,
                    v, *v0, *v1, *v2, float(w0), float(w1), float(w2));
          w1 -= a1;
          w2 -= a2;
          w0 = 1.0 - (w1 + w2);
          x--;
        }
      }
      while (++y <= yMax);
    }
    else                                // positive X direction
    {
      do
      {
        double  yf = (double(y) - y0) * dy2;
        int     x = roundFloat(float((x2 - x0) * yf + x0));
        double  w1 = ((double(x) - x0) * a1) + (yf * b1);
        if (!(w1 >= -0.000001))
        {
          w1 += a1;
          x++;
        }
        double  w2 = ((double(x) - x0) * a2) + (yf * b2);
        double  w0 = 1.0 - (w1 + w2);
        while (!(!(w0 >= -0.000001) | !(w2 >= -0.000001)))
        {
          float   z = (v0->xyz[2] * float(w0)) + (v1->xyz[2] * float(w1))
                      + (v2->xyz[2] * float(w2));
          v.xyz.setElement(2, z);
          drawPixel(x, y, txtScaleU, txtScaleV,
                    v, *v0, *v1, *v2, float(w0), float(w1), float(w2));
          w1 += a1;
          w2 += a2;
          w0 = 1.0 - (w1 + w2);
          x++;
        }
      }
      while (++y <= yMax);
    }
  }
}

Plot3D_TriShape::Plot3D_TriShape(
    unsigned int *outBufRGBW, float *outBufZ, int imageWidth, int imageHeight,
    bool enableGamma)
  : bufRGBW(outBufRGBW),
    bufZ(outBufZ),
    width(imageWidth),
    height(imageHeight),
    textureD((DDSTexture *) 0),
    textureG((DDSTexture *) 0),
    textureN((DDSTexture *) 0),
    textureE((DDSTexture *) 0),
    textureS((DDSTexture *) 0),
    textureR((DDSTexture *) 0),
    mipOffsetN(1.0f),
    mipOffsetS(1.0f),
    mipOffsetR(1.0f),
    mipLevel(15.0f),
    alphaThresholdFloat(0.0f),
    mipLevelMin(0),
    lightVector(0.0f, 0.0f, 1.0f, 0.0f),
    envMapOffs(float(imageWidth) * -0.5f, float(imageHeight) * -0.5f,
               float(imageHeight), 0.0f),
    viewTransformInvX(1.0f, 0.0f, 0.0f, 0.0f),
    viewTransformInvY(0.0f, 1.0f, 0.0f, 0.0f),
    viewTransformInvZ(0.0f, 0.0f, 1.0f, 0.0f),
    specularColorFloat(1.0f, 1.0f, 1.0f, 1.0f),
    reflectionLevel(0.0f),
    specularLevel(0.0f),
    invNormals(false),
    gammaCorrectEnabled(enableGamma),
    debugMode(0U),
    drawPixelFunction(&drawPixel_Water)
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
  envMapScale = t.envMapScale;
  alphaThreshold = t.alphaThreshold;
  specularColor = t.specularColor;
  specularScale = t.specularScale;
  specularSmoothness = t.specularSmoothness;
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
    lightingPolynomial[i] = a[i];
}

void Plot3D_TriShape::getLightingFunction(float *a) const
{
  for (int i = 0; i < 6; i++)
    a[i] = lightingPolynomial[i];
}

void Plot3D_TriShape::getDefaultLightingFunction(float *a)
{
  for (int i = 0; i < 6; i++)
    a[i] = defaultLightingPolynomial[i];
}

static float calculateMipOffset(const DDSTexture *t1, const DDSTexture *t2)
{
  int     a1 = t1->getWidth() * t1->getHeight();
  int     a2 = t2->getWidth() * t2->getHeight();
  if (a1 == a2)
    return 0.0f;
  if (a1 < a2)
    return (a1 <= (a2 >> 3) ? -2.0f : (a1 <= (a2 >> 1) ? -1.0f : 0.0f));
  return (a2 <= (a1 >> 3) ? 2.0f : (a2 <= (a1 >> 1) ? 1.0f : 0.0f));
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
  lightVector = FloatVector4(lightX, lightY, lightZ, 0.0f);
  size_t  triangleCntRendered =
      transformVertexData(modelTransform, viewTransform);
  if (!triangleCntRendered)
    return;
  textureD = (DDSTexture *) 0;
  textureG = (DDSTexture *) 0;
  textureN = (DDSTexture *) 0;
  textureE = (DDSTexture *) 0;
  textureS = (DDSTexture *) 0;
  textureR = (DDSTexture *) 0;
  mipOffsetN = 0.0f;
  mipOffsetS = 0.0f;
  mipOffsetR = 0.0f;
  mipLevel = 15.0f;
  alphaThresholdFloat = float(int(alphaThreshold)) * 0.999999f;
  viewTransformInvX =
      FloatVector4(viewTransform.rotateXX, viewTransform.rotateYX,
                   viewTransform.rotateZX, 0.0f);
  viewTransformInvY =
      FloatVector4(viewTransform.rotateXY, viewTransform.rotateYY,
                   viewTransform.rotateZY, 0.0f);
  viewTransformInvZ =
      FloatVector4(viewTransform.rotateXZ, viewTransform.rotateYZ,
                   viewTransform.rotateZZ, 0.0f);
  specularColorFloat = FloatVector4(specularColor);
  specularColorFloat *= (1.0f / 255.0f);
  reflectionLevel = 0.0f;
  specularLevel = 0.0f;
  if (textureCnt >= 1)
    textureD = textures[0];
  if (textureCnt >= 4)
    textureG = textures[3];
  if (textureCnt >= 2)
    textureN = textures[1];
  if (isWater)
  {
    if (!textureN)
      textureN = textureD;
    else
      textureD = textureN;
    drawPixelFunction = &drawPixel_Water;
  }
  else if (!(textureN && (textureD->getWidth() * textureN->getHeight())
                         == (textureD->getHeight() * textureN->getWidth())))
  {
    textureN = (DDSTexture *) 0;
    drawPixelFunction = &drawPixel_Diffuse;
  }
  else
  {
    mipOffsetN = calculateMipOffset(textureN, textureD);
    if (!(textureCnt >= 5 && textures[4]))
    {
      drawPixelFunction = &drawPixel_Normal;
    }
    else
    {
      textureE = textures[4];
      if (textureCnt >= 10 && textures[8])      // Fallout 76
      {
        textureR = textures[8];
        mipOffsetR = calculateMipOffset(textureR, textureD);
        textureS = textures[9];
        drawPixelFunction = &drawPixel_NormalRefl;
      }
      else if (textureCnt >= 7 && textures[6])  // Fallout 4
      {
        textureS = textures[6];
        drawPixelFunction = &drawPixel_NormalEnvS;
      }
      else if (textureCnt >= 6 && textures[5])  // Skyrim with environment mask
      {
        textureS = textures[5];
        mipOffsetS = calculateMipOffset(textureS, textureD);
        drawPixelFunction = &drawPixel_NormalEnvM;
      }
      else
      {
        drawPixelFunction = &drawPixel_NormalEnv;
      }
      reflectionLevel = getLightLevel(1.0f) * (0.7217095f / 128.0f);
      specularLevel = reflectionLevel * float(int(specularScale));
      reflectionLevel = reflectionLevel * float(int(envMapScale));
    }
  }
  float   tmp = 0.0f;
  tmp = (mipOffsetN > tmp ? mipOffsetN : tmp);
  tmp = (mipOffsetS > tmp ? mipOffsetS : tmp);
  tmp = (mipOffsetR > tmp ? mipOffsetR : tmp);
  mipLevelMin = roundFloat(-tmp);
  if (BRANCH_EXPECT(debugMode, false))
    drawPixelFunction = &drawPixel_Debug;
  drawTriangles();
}

void Plot3D_TriShape::renderWater(
    const NIFFile::NIFVertexTransform& viewTransform,
    unsigned int waterColor, float lightX, float lightY, float lightZ,
    const DDSTexture *envMap, float envMapLevel)
{
  viewTransform.rotateXYZ(lightX, lightY, lightZ);
  textureE = envMap;
  envMapLevel = envMapLevel * lightingPolynomial[0] * 255.0f;
  lightVector = FloatVector4(lightX, lightY, lightZ, 0.0f);
  lightVector.normalize();
  viewTransformInvX =
      FloatVector4(viewTransform.rotateXX, viewTransform.rotateYX,
                   viewTransform.rotateZX, 0.0f);
  viewTransformInvY =
      FloatVector4(viewTransform.rotateXY, viewTransform.rotateYY,
                   viewTransform.rotateZY, 0.0f);
  viewTransformInvZ =
      FloatVector4(viewTransform.rotateXZ, viewTransform.rotateYZ,
                   viewTransform.rotateZZ, 0.0f);
  specularColorFloat = FloatVector4(1.0f, 1.0f, 1.0f, 1.0f);
  specularLevel = 1.0f;
  invNormals = false;
  specularColor = 0xFFFFFFFFU;
  specularScale = 128;
  specularSmoothness = 255;
  Vertex  z;
  FloatVector4  waterColorFloat(waterColor);
  float   waterAlpha = waterColorFloat[3] * (1.0f / 256.0f);
  waterColorFloat *= waterAlpha;
  waterAlpha = 1.0f - waterAlpha;
  unsigned int  *p = bufRGBW;
  for (int y = 0; y < height; y++)
  {
    for (int x = 0; x < width; x++, p++)
    {
      unsigned int  c = *p;
      if (!((c + 0x01000000U) & 0xFE000000U))
        continue;
      z.normal = FloatVector4((c >> 16) & 0xFFFFU);
      z.normal += FloatVector4(-128.0f, -128.0f, 0.0f, 0.0f);
      z.normal *= (1.0f / 126.0f);
      float   normalXY2 = z.normal.dotProduct(z.normal);
      float   normalZSqr = 1.0f - normalXY2;
      if (BRANCH_EXPECT((normalZSqr > 0.0f), true))
      {
        z.normal.setElement(2, -(float(std::sqrt(normalZSqr))));
      }
      else
      {
        z.normal.normalize();
        normalXY2 = 1.0f;
      }
      FloatVector4  v(((c & 0x001FU) << 3) | ((c & 0x07E0U) << 5)
                      | ((c & 0xF800U) << 8));
      FloatVector4  tmp(waterColorFloat);
      tmp *= calculateLighting(z.normal);
      v *= waterAlpha;
      v += tmp;
      if (envMap)
      {
        float   s = 0.0f;
        FloatVector4  e(environmentMap(z, x, y, 1.0f, &s));
        e += FloatVector4(s, s, s, s);
        e *= (envMapLevel * (normalXY2 * 0.75f + 0.25f));
        v += e;
      }
      *p = (unsigned int) v | 0xFF000000U;
    }
  }
}

