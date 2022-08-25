
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
  0.67223534f, 0.98737308f, 0.04499248f, -0.71293136f, 0.06848673f, 0.31271107f
};

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
  NIFFile::NIFVertexTransform mt(vertexTransform);
  mt *= modelTransform;
  NIFFile::NIFVertexTransform vt(viewTransform);
  if (flags & 0x08)                     // decal
    vt.offsZ = vt.offsZ - 0.0625f;
  NIFFile::NIFVertexTransform xt(mt);
  xt *= vt;
  {
    NIFFile::NIFBounds  b;
    for (size_t i = 0; i < vertexCnt; i++)
    {
      Vertex  v(vertexData[i]);
      v.xyz = xt.transformXYZ(v.xyz);
      b += v.xyz;
      vertexBuf.push_back(v);
    }
    if (b.xMin() >= (float(width) - 0.5f) || b.xMax() < -0.5f ||
        b.yMin() >= (float(height) - 0.5f) || b.yMax() < -0.5f ||
        b.zMin() >= 16777216.0f || b.zMax() < 0.0f)
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
      v.xyz[0] = x;
    if (float(std::fabs(v.xyz[1] - y)) < VERTEX_XY_SNAP)
      v.xyz[1] = y;
#endif
    if (flags & 0x20)                   // tree: ignore vertex alpha
      v.vertexColor[3] = 1.0f;
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
      tmp.normalize3Fast();
      v.bitangent = vt.rotateXYZ(tmp);
      tmp = FloatVector4(0.0f, v.normal[2], -(v.normal[1]), 0.0f);
      tmp.normalize3Fast();
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
    v.bitangent[3] = txtU;
    v.tangent[3] = txtV;
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
  float   d2 = d * d;
  FloatVector4  tmp(d * d2, d2, d, 1.0f);
  float   y = tmp.dotProduct(lightingPolynomial3_0)
              + (tmp * FloatVector4(d2)).dotProduct2(lightingPolynomial5_4);
  return (y > 0.0f ? (y < 4.0f ? y : 4.0f) : 0.0f);
}

inline FloatVector4 Plot3D_TriShape::normalMap(Vertex& v, FloatVector4 n) const
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
  tmpB.normalize3Fast();
  tmpB *= x;
  FloatVector4  tmpT(v.tangent);
  tmpT.normalize3Fast();
  tmpT *= y;
  FloatVector4  tmpN(v.normal);
  tmpN.normalize3Fast();
  tmpN *= z;
  tmpN += tmpB;
  tmpN += tmpT;
  // normalize and store result
  tmpN.normalize(invNormals);
  v.normal = tmpN;
  return tmpN;
}

inline FloatVector4 Plot3D_TriShape::calculateReflection(
    const Vertex& v, int x, int y) const
{
  // view vector
  FloatVector4  viewVector(float(x), float(y), 0.0f, 0.0f);
  viewVector += envMapOffs;
  // reflect (I - 2.0 * dot(N, I) * N)
  viewVector -= (FloatVector4(2.0f * viewVector.dotProduct3(v.normal))
                 * v.normal);
  return viewVector.normalize();
}

inline FloatVector4 Plot3D_TriShape::environmentMap(
    FloatVector4 reflectedView, float smoothness) const
{
  // inverse rotation by view matrix
  FloatVector4  v((FloatVector4(reflectedView[0]) * viewTransformInvX)
                  + (FloatVector4(reflectedView[1]) * viewTransformInvY)
                  + (FloatVector4(reflectedView[2]) * viewTransformInvZ));
  float   r = 1.0f - smoothness;        // roughness
  float   m = (r > 0.0f ? r : 0.0f) * float(textureE->getMaxMipLevel());
  FloatVector4  e(textureE->cubeMap(v[0], v[1], v[2], m));
  e *= (1.0f / 255.0f);
  return e;
}

inline float Plot3D_TriShape::specularPhong(
    FloatVector4 reflectedView, float smoothness,
    float nDotL, bool isNormalized) const
{
  if (!(specularLevel > 0.0f && nDotL > -0.125f))
    return 0.0f;
  float   s = reflectedView.dotProduct3(lightVector);
  if (!(s > -0.5f))
    return 0.0f;
  s = (s < 1.0f ? s : 1.0f);
  float   g = FloatVector4::exp2Fast(9.0f * smoothness);
  // approximates g * log(s) / 2, more accurately if s is closer to 1
  s = g * (s - 1.0f) / (s + 1.0f);
  if (!(s > -7.5f))
    return 0.0f;
  s = FloatVector4::exp2Fast(s * 2.8853901f);   // 2 / log(2)
  if (BRANCH_EXPECT((nDotL < 0.0f), false))
    s = s * (nDotL * 8.0f + 1.0f);
  if (isNormalized)
    s = s * FloatVector4::squareRootFast(g + 0.4375f) * 0.20822077f;
  return s;
}

inline float Plot3D_TriShape::specularGGX(
    FloatVector4 reflectedView, float smoothness,
    float nDotL, float nDotV) const
{
  if (!(specularLevel > 0.0f && nDotL > 0.0f))
    return 0.0f;
  // h2 = 2.0 * squared dot product with halfway vector
  float   h2 = reflectedView.dotProduct3(lightVector) + 1.0f;
  float   r = 0.96875f - (smoothness * 0.9375f);
  float   a = r * r;
  float   a2 = a * a;
  float   d = h2 * (a2 - 1.0f) + 2.0f;
  float   s = a2 / (d * d);
  float   k = (r + 1.0f) * (r + 1.0f) * 0.125f;
  s *= (nDotL / (nDotL * (1.0f - k) + k));
#if 1
  s *= (nDotV / (nDotV * (1.0f - k) + k));
#else
  (void) nDotV;
#endif
  return s;
}

inline float Plot3D_TriShape::fresnelGamma2(float nDotV)
{
  FloatVector4  a3_0(-18.90442774f, 13.42739638f, -5.32806707f, 1.00000000f);
  FloatVector4  a5_4(-4.10377539f, 13.90887383f, 0.0f, 0.0f);
  float   d2 = nDotV * nDotV;
  FloatVector4  tmp(nDotV * d2, d2, nDotV, 1.0f);
  return (tmp.dotProduct(a3_0) + (tmp * FloatVector4(d2)).dotProduct2(a5_4));
}

void Plot3D_TriShape::drawPixel_Water(
    Plot3D_TriShape& p, int x, int y, float txtU, float txtV, Vertex& z)
{
  (void) txtU;
  (void) txtV;
  size_t  offs = size_t(y) * size_t(p.width) + size_t(x);
  // first pass: only write depth and alpha
  float   d = float(int(~(p.bufRGBA[offs] >> 24) & 0xFFU));
  d = (d * d) + ((p.bufZ[offs] - z.xyz[2]) * (16.0f / p.viewScale));
  d = FloatVector4::squareRootFast(d < 65025.0f ? d : 65025.0f);
  p.bufZ[offs] = z.xyz[2] * 1.00000025f + 0.00000025f;
  // alpha channel = 255 - sqrt(waterDepth*16)
  p.bufRGBA[offs] = (p.bufRGBA[offs] & 0x00FFFFFFU)
                    | ((unsigned int) (255 - roundFloat(d)) << 24);
}

void Plot3D_TriShape::drawPixel_Water_2(
    Plot3D_TriShape& p, int x, int y, float txtU, float txtV, Vertex& z)
{
  size_t  offs = size_t(y) * size_t(p.width) + size_t(x);
  p.bufZ[offs] = z.xyz[2];
  if (BRANCH_EXPECT(p.textureN, true))
    (void) p.normalMap(z, p.textureN->getPixelT(txtU, txtV, p.mipLevel));
  else
    z.normal.normalize(p.invNormals);
  FloatVector4  c(p.bufRGBA[offs]);
  FloatVector4  n(z.normal);
  FloatVector4  reflectedView(p.calculateReflection(z, x, y));
  FloatVector4  e(0.0f);
  float   f = 0.0f;
  float   nDotL = n.dotProduct3(p.lightVector);
  float   a = 255.0f - c[3];
  a = (a * a) * p.specularColorFloat[3];
  c *= (1.0f / 255.0f);
  if (a > -3.0f && a < 0.0f)
    a = FloatVector4::exp2Fast(a - 1.0f);
  else
    a = (a < -2.0f ? 0.0625f : 0.5f);
  if (p.textureE)
  {
    e = p.environmentMap(reflectedView, 1.0f);
    float   nDotV = float(std::fabs(z.normal[2]));
    nDotV = (nDotV < 1.0f ? nDotV : 1.0f);
    // f0 = pow(0.0203, 2.0 / 2.2)
    f = fresnelGamma2(nDotV) * (1.0f - 0.029f) + 0.029f;
    f = FloatVector4::squareRootFast(f);
    float   specular = p.specularPhong(reflectedView, 0.875f, nDotL, true);
    e = (e + FloatVector4(specular)) * FloatVector4(f * p.reflectionLevel);
  }
  float   l = p.getLightLevel(nDotL);
  c = (c * FloatVector4(a))
      + (p.specularColorFloat * FloatVector4(l * (1.0f - a)));
  c = c * FloatVector4(1.0f - (f * 0.5f)) + e;
  p.bufRGBA[offs] = (unsigned int) (c * FloatVector4(255.0f)) | 0xFF000000U;
}

void Plot3D_TriShape::drawPixel_Water_2G(
    Plot3D_TriShape& p, int x, int y, float txtU, float txtV, Vertex& z)
{
  size_t  offs = size_t(y) * size_t(p.width) + size_t(x);
  p.bufZ[offs] = z.xyz[2];
  if (BRANCH_EXPECT(p.textureN, true))
    (void) p.normalMap(z, p.textureN->getPixelT(txtU, txtV, p.mipLevel));
  else
    z.normal.normalize(p.invNormals);
  FloatVector4  c(p.bufRGBA[offs]);
  FloatVector4  n(z.normal);
  FloatVector4  reflectedView(p.calculateReflection(z, x, y));
  FloatVector4  e(0.0f);
  float   f = 0.0f;
  float   nDotL = n.dotProduct3(p.lightVector);
  float   a = 255.0f - c[3];
  a = (a * a) * p.specularColorFloat[3];
  c *= (1.0f / 255.0f);
  if (a > -3.0f && a < 0.0f)
    a = FloatVector4::exp2Fast(a - 1.0f);
  else
    a = (a < -2.0f ? 0.0625f : 0.5f);
  if (p.textureE)
  {
    e = p.environmentMap(reflectedView, 1.0f);
    float   nDotV = float(std::fabs(z.normal[2]));
    nDotV = (nDotV < 1.0f ? nDotV : 1.0f);
    // f0 = pow(0.0203, 2.0 / 2.2)
    f = fresnelGamma2(nDotV) * (1.0f - 0.029f) + 0.029f;
    float   specular = p.specularGGX(reflectedView, 0.875f, nDotL, nDotV);
    e = (e * e + FloatVector4(specular)) * FloatVector4(f * p.reflectionLevel);
  }
  float   l = p.getLightLevel(nDotL);
  c = (c * c * FloatVector4(a))
      + (p.specularColorFloat * FloatVector4(l * l * (1.0f - a)));
  c = (c * FloatVector4(1.0f - f) + e).squareRootFast();
  p.bufRGBA[offs] = (unsigned int) (c * FloatVector4(255.0f)) | 0xFF000000U;
}

inline FloatVector4 Plot3D_TriShape::getDiffuseColor(
    float txtU, float txtV, const Vertex& z, bool& alphaFlag,
    bool enableGamma) const
{
  FloatVector4  c(textureD->getPixelT(txtU, txtV, mipLevel));
  FloatVector4  vColor(z.vertexColor);
  float   a = c[3] * vColor[3];
  alphaFlag = !(a < alphaThresholdFloat);
  if (BRANCH_EXPECT(!alphaFlag, false))
    return c;
  if (BRANCH_EXPECT(textureG, false))
  {
    float   u = c[1] * (1.0f / 255.0f);
    float   v = float(int(gradientMapV) - 255) * (1.0f / 255.0f) + vColor[0];
    c = textureG->getPixelBM(u, v, 0);
    if (enableGamma)
    {
      c *= c;
      c *= (1.0f / 255.0f);
    }
  }
  else
  {
    if (enableGamma)
    {
      c *= c;
      c *= (1.0f / 255.0f);
    }
    c *= vColor;
  }
  c[3] = a;
  return c;
}

inline FloatVector4 Plot3D_TriShape::glowMap(
    FloatVector4 c, float txtU, float txtV) const
{
  if (BRANCH_EXPECT(!textureGlow, true))
    return c;
  FloatVector4  g(textureGlow->getPixelT(txtU, txtV, mipLevel + mipOffsetG));
  g *= getLightLevel(1.0f);
  if (gammaCorrectEnabled)
  {
    g *= g;
    g *= (1.0f / 255.0f);
  }
  return (c + g);
}

void Plot3D_TriShape::drawPixel_Debug(
    Plot3D_TriShape& p, int x, int y, float txtU, float txtV, Vertex& z)
{
  size_t  offs = size_t(y) * size_t(p.width) + size_t(x);
  FloatVector4  c(0.0f);
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
    p.bufRGBA[offs] = tmp;
    return;
  }
  FloatVector4  n(127.5f, 127.5f, 0.0f, 0.0f);
  if (p.textureN)
    n = p.textureN->getPixelT(txtU, txtV, p.mipLevel + p.mipOffsetN);
  (void) p.normalMap(z, n);
  if (p.debugMode == 3)
  {
    c = z.normal;
    c *= FloatVector4(127.5f, -127.5f, -127.5f, 0.0f);
    c += FloatVector4(127.5f, 127.5f, 127.5f, 0.0f);
  }
  else
  {
    c *= p.getLightLevel(z.normal.dotProduct3(p.lightVector));
  }
  p.bufRGBA[offs] = (unsigned int) c | 0xFF000000U;
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
  z.normal.normalize(p.invNormals);
  c *= p.getLightLevel(z.normal.dotProduct3(p.lightVector));
  c = p.glowMap(c, txtU, txtV);
  p.bufRGBA[offs] = (unsigned int) c | 0xFF000000U;
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
  c *= p.getLightLevel(p.normalMap(z, n).dotProduct3(p.lightVector));
  c = p.glowMap(c, txtU, txtV);
  p.bufRGBA[offs] = (unsigned int) c | 0xFF000000U;
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
  float   nDotL = p.normalMap(z, n).dotProduct3(p.lightVector);
  c = p.glowMap(c * FloatVector4(p.getLightLevel(nDotL)), txtU, txtV);
  float   smoothness = float(int(p.specularSmoothness)) * (1.0f / 255.0f);
  FloatVector4  reflectedView(p.calculateReflection(z, x, y));
  FloatVector4  e(p.environmentMap(reflectedView, smoothness));
  float   specular = p.specularPhong(reflectedView, smoothness, nDotL, false);
  float   specLevel = n[3];
  e *= (p.reflectionLevel * specLevel);
  c += e;
  c += (p.specularColorFloat
        * FloatVector4(specular * p.specularLevel * specLevel));
  p.bufRGBA[offs] = (unsigned int) c | 0xFF000000U;
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
  float   nDotL = p.normalMap(z, n).dotProduct3(p.lightVector);
  c = p.glowMap(c * FloatVector4(p.getLightLevel(nDotL)), txtU, txtV);
  FloatVector4  m(p.textureS->getPixelT(txtU, txtV, p.mipLevel + p.mipOffsetS));
  float   smoothness = float(int(p.specularSmoothness)) * (1.0f / 255.0f);
  FloatVector4  reflectedView(p.calculateReflection(z, x, y));
  FloatVector4  e(p.environmentMap(reflectedView, smoothness));
  float   specular = p.specularPhong(reflectedView, smoothness, nDotL, false);
  float   specLevel = m[0];
  e *= (p.reflectionLevel * specLevel);
  c += e;
  c += (p.specularColorFloat
        * FloatVector4(specular * p.specularLevel * specLevel));
  p.bufRGBA[offs] = (unsigned int) c | 0xFF000000U;
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
  float   nDotL = p.normalMap(z, n).dotProduct3(p.lightVector);
  c = p.glowMap(c * FloatVector4(p.getLightLevel(nDotL)), txtU, txtV);
  float   smoothness =
      float(int(p.specularSmoothness)) * n[3] * (1.0f / (255.0f * 255.0f));
  float   specLevel = n[2];
  FloatVector4  reflectedView(p.calculateReflection(z, x, y));
  FloatVector4  e(p.environmentMap(reflectedView, smoothness));
  float   specular = p.specularPhong(reflectedView, smoothness, nDotL, true);
  e *= (p.reflectionLevel * specLevel);
  c += e;
  c += (p.specularColorFloat
        * FloatVector4(specular * p.specularLevel * specLevel));
  p.bufRGBA[offs] = (unsigned int) c | 0xFF000000U;
}

void Plot3D_TriShape::drawPixel_NormalRefl(
    Plot3D_TriShape& p, int x, int y, float txtU, float txtV, Vertex& z)
{
  size_t  offs = size_t(y) * size_t(p.width) + size_t(x);
  bool    alphaFlag = false;
  FloatVector4  c(p.getDiffuseColor(txtU, txtV, z, alphaFlag, true));
  if (BRANCH_EXPECT(!alphaFlag, false))
    return;
  p.bufZ[offs] = z.xyz[2];
  FloatVector4  n(p.textureN->getPixelT_2(txtU, txtV, p.mipLevel + p.mipOffsetN,
                                          p.textureS));
  FloatVector4  r(p.textureR->getPixelT(txtU, txtV, p.mipLevel + p.mipOffsetR));
  r *= (1.0f / 255.0f);
  r *= r;
  r.maxValues(FloatVector4(0.025f));
  float   nDotL = p.normalMap(z, n).dotProduct3(p.lightVector);
  float   l = p.getLightLevel(nDotL);
  c *= (l * l);
  float   smoothness = n[2];
  float   specLevel = n[3];
  if (BRANCH_EXPECT(!p.textureS, false))
  {
    smoothness = 127.5f;
    specLevel = 255.0f;
  }
  smoothness = smoothness * float(int(p.specularSmoothness))
               * (1.0f / (255.0f * 255.0f));
  FloatVector4  reflectedView(p.calculateReflection(z, x, y));
  FloatVector4  e(p.environmentMap(reflectedView, smoothness));
  float   nDotV = float(std::fabs(z.normal[2]));
  nDotV = (nDotV < 1.0f ? nDotV : 1.0f);
  float   specular = p.specularGGX(reflectedView, smoothness, nDotL, nDotV);
  e = (e * e * FloatVector4(p.reflectionLevel))
      + (p.specularColorFloat * FloatVector4(specular * p.specularLevel));
  FloatVector4  f(r + ((FloatVector4(smoothness).maxValues(r) - r)
                       * FloatVector4(fresnelGamma2(nDotV))));
  e *= specLevel;
  c = p.glowMap(c + ((e - c) * f), txtU, txtV);
  c *= 255.0f;
  c.squareRootFast();
  p.bufRGBA[offs] = (unsigned int) c | 0xFF000000U;
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
    int x, int y, Vertex& v,
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
  v.tangent =
      interpolateVectors(v0.tangent, v1.tangent, v2.tangent, w0, w1, w2);
  v.normal = interpolateVectors(v0.normal, v1.normal, v2.normal, w0, w1, w2);
  v.vertexColor =
      interpolateVectors(v0.vertexColor, v1.vertexColor, v2.vertexColor,
                         w0, w1, w2);
  drawPixelFunction(*this, x, y, v.bitangent[3], v.tangent[3], v);
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
          v.xyz[2] = (v0.xyz[2] * w0) + (v1.xyz[2] * float(w1));
          drawPixel(x, y, v, v0, v1, v0, w0, float(w1), 0.0f);
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
          v.xyz[2] = (v0.xyz[2] * w0) + (v1.xyz[2] * float(w1));
          drawPixel(x, y, v, v0, v1, v0, w0, float(w1), 0.0f);
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
    v.xyz[2] = v0.xyz[2];
    drawPixel(x, y, v, v0, v0, v0, 1.0f, 0.0f, 0.0f);
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
    if (BRANCH_EXPECT(textureD, true))
    {
      float   txtU1d = v1->bitangent[3] - v0->bitangent[3];
      float   txtU2d = v2->bitangent[3] - v0->bitangent[3];
      float   txtV1d = v1->tangent[3] - v0->tangent[3];
      float   txtV2d = v2->tangent[3] - v0->tangent[3];
      float   uvArea2 = float(std::fabs((txtU1d * txtV2d) - (txtU2d * txtV1d)));
      if (xyArea2 > uvArea2)
      {
        uvArea2 *= (float(textureD->getWidth()) * float(textureD->getHeight()));
        mipLevel = -6.0f;
        if (BRANCH_EXPECT(((uvArea2 * 4096.0f) > xyArea2), true))
        {
          // calculate base 4 logarithm of texel area / pixel area
          mipLevel = FloatVector4::log2Fast(float(std::fabs(r2xArea * uvArea2)))
                     * 0.5f;
          int     mipLevel_i = roundFloat(mipLevel);
          if (float(std::fabs(mipLevel - float(mipLevel_i))) < 0.0625f)
            mipLevel = float(mipLevel_i);
        }
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
          v.xyz[2] = z;
          drawPixel(x, y, v, *v0, *v1, *v2, float(w0), float(w1), float(w2));
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
          v.xyz[2] = z;
          drawPixel(x, y, v, *v0, *v1, *v2, float(w0), float(w1), float(w2));
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
    unsigned int *outBufRGBA, float *outBufZ, int imageWidth, int imageHeight,
    bool enableGamma)
  : bufRGBA(outBufRGBA),
    bufZ(outBufZ),
    width(imageWidth),
    height(imageHeight),
    textureD((DDSTexture *) 0),
    textureG((DDSTexture *) 0),
    textureN((DDSTexture *) 0),
    textureE((DDSTexture *) 0),
    textureS((DDSTexture *) 0),
    textureR((DDSTexture *) 0),
    textureGlow((DDSTexture *) 0),
    mipOffsetN(0.0f),
    mipOffsetS(0.0f),
    mipOffsetR(0.0f),
    mipOffsetG(0.0f),
    mipLevel(15.0f),
    alphaThresholdFloat(0.0f),
    invNormals(false),
    gammaCorrectEnabled(enableGamma),
    viewScale(1.0f),
    lightVector(0.0f, 0.0f, 1.0f, 0.0f),
    lightingPolynomial3_0(0.0f),
    lightingPolynomial5_4(0.0f),
    envMapOffs(float(imageWidth) * -0.5f, float(imageHeight) * -0.5f,
               float(imageHeight), 0.0f),
    viewTransformInvX(1.0f, 0.0f, 0.0f, 0.0f),
    viewTransformInvY(0.0f, 1.0f, 0.0f, 0.0f),
    viewTransformInvZ(0.0f, 0.0f, 1.0f, 0.0f),
    specularColorFloat(1.0f),
    reflectionLevel(0.0f),
    specularLevel(0.0f),
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
  lightingPolynomial3_0 = FloatVector4(a[3], a[2], a[1], a[0]);
  lightingPolynomial5_4 = FloatVector4(a[5], a[4], 0.0f, 0.0f);
}

void Plot3D_TriShape::getLightingFunction(float *a) const
{
  a[0] = lightingPolynomial3_0[3];
  a[1] = lightingPolynomial3_0[2];
  a[2] = lightingPolynomial3_0[1];
  a[3] = lightingPolynomial3_0[0];
  a[4] = lightingPolynomial5_4[1];
  a[5] = lightingPolynomial5_4[0];
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
  int     m = FloatVector4::log2Int(a1) - FloatVector4::log2Int(a2);
  if (m < 0)
  {
    m = (1 - m) >> 1;
    return float(m < 6 ? -m : -6);
  }
  m = (m + 1) >> 1;
  return float(m < 2 ? m : 2);
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
  textureGlow = (DDSTexture *) 0;
  mipOffsetN = 0.0f;
  mipOffsetS = 0.0f;
  mipOffsetR = 0.0f;
  mipOffsetG = 0.0f;
  mipLevel = 15.0f;
  alphaThresholdFloat = float(int(alphaThreshold)) * 0.999999f;
  viewScale = viewTransform.scale;
  viewTransformInvX =
      FloatVector4(viewTransform.rotateXX, viewTransform.rotateXY,
                   viewTransform.rotateXZ, 0.0f);
  viewTransformInvY =
      FloatVector4(viewTransform.rotateYX, viewTransform.rotateYY,
                   viewTransform.rotateYZ, 0.0f);
  viewTransformInvZ =
      FloatVector4(viewTransform.rotateZX, viewTransform.rotateZY,
                   viewTransform.rotateZZ, 0.0f);
  specularColorFloat = FloatVector4(specularColor);
  specularColorFloat *= (1.0f / 255.0f);
  reflectionLevel = 0.0f;
  specularLevel = 0.0f;
  if (textureCnt >= 1)
    textureD = textures[0];
  if (textureCnt >= 4)
    textureG = textures[3];
  if (textureCnt >= 3 && BRANCH_EXPECT(textures[2], false) && (flags & 0x80))
  {
    textureGlow = textures[2];
    mipOffsetG = calculateMipOffset(textureGlow, textureD);
  }
  if (textureCnt >= 2)
    textureN = textures[1];
  if (isWater)
  {
    drawPixelFunction = &drawPixel_Water;
  }
  else if (!textureN)
  {
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
      reflectionLevel = getLightLevel(1.0f);
      textureE = textures[4];
      if (textureCnt >= 10 && textures[8])      // Fallout 76
      {
        textureR = textures[8];
        mipOffsetR = calculateMipOffset(textureR, textureD);
        textureS = textures[9];
        drawPixelFunction = &drawPixel_NormalRefl;
        reflectionLevel = reflectionLevel * reflectionLevel;
      }
      else if (textureCnt >= 7 && textures[6])  // Fallout 4
      {
        textureS = textures[6];
        drawPixelFunction = &drawPixel_NormalEnvS;
        reflectionLevel = reflectionLevel * 0.625f;
      }
      else if (textureCnt >= 6 && textures[5])  // Skyrim with environment mask
      {
        textureS = textures[5];
        mipOffsetS = calculateMipOffset(textureS, textureD);
        drawPixelFunction = &drawPixel_NormalEnvM;
        reflectionLevel = reflectionLevel * 0.625f;
      }
      else
      {
        drawPixelFunction = &drawPixel_NormalEnv;
        reflectionLevel = reflectionLevel * 0.625f;
      }
      reflectionLevel = reflectionLevel * (1.0f / 128.0f);
      specularLevel = reflectionLevel * float(int(specularScale));
      reflectionLevel = reflectionLevel * float(int(envMapScale));
    }
  }
  if (BRANCH_EXPECT(debugMode, false))
    drawPixelFunction = &drawPixel_Debug;
  drawTriangles();
}

void Plot3D_TriShape::drawWater(
    const NIFFile::NIFVertexTransform& modelTransform,
    const NIFFile::NIFVertexTransform& viewTransform,
    float lightX, float lightY, float lightZ,
    const DDSTexture * const *textures, size_t textureCnt,
    unsigned int waterColor, float envMapLevel)
{
  if (!(flags & 0x02) || debugMode)
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
  textureGlow = (DDSTexture *) 0;
  mipOffsetN = 0.0f;
  mipOffsetS = 0.0f;
  mipOffsetR = 0.0f;
  mipOffsetG = 0.0f;
  mipLevel = 15.0f;
  alphaThresholdFloat = float(int(alphaThreshold)) * 0.999999f;
  viewScale = viewTransform.scale;
  viewTransformInvX =
      FloatVector4(viewTransform.rotateXX, viewTransform.rotateXY,
                   viewTransform.rotateXZ, 0.0f);
  viewTransformInvY =
      FloatVector4(viewTransform.rotateYX, viewTransform.rotateYY,
                   viewTransform.rotateYZ, 0.0f);
  viewTransformInvZ =
      FloatVector4(viewTransform.rotateZX, viewTransform.rotateZY,
                   viewTransform.rotateZZ, 0.0f);
  specularColorFloat = FloatVector4(waterColor);
  float   a = 256.0f - specularColorFloat[3];
  specularColorFloat *= (1.0f / 255.0f);
  if (gammaCorrectEnabled)
    specularColorFloat *= specularColorFloat;
  specularColorFloat[3] = -1.5f / (a * a);      // = -3.0 / (maxDepth * 16.0)
  reflectionLevel = 0.0f;
  specularLevel = 0.0f;
  if (textureCnt >= 2 && textures[1])
    textureD = textures[1];
  else if (textureCnt >= 1)
    textureD = textures[0];
  textureN = textureD;
  if (!gammaCorrectEnabled)
    drawPixelFunction = &drawPixel_Water_2;
  else
    drawPixelFunction = &drawPixel_Water_2G;
  if (textureCnt >= 5 && textures[4])
  {
    textureE = textures[4];
    reflectionLevel = envMapLevel * getLightLevel(1.0f);
    if (gammaCorrectEnabled)
      reflectionLevel = reflectionLevel * reflectionLevel;
    specularLevel = reflectionLevel;
  }
  drawTriangles();
}

