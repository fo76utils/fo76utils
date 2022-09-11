
#include "common.hpp"
#include "plot3d.hpp"
#include "fp32vec4.hpp"

#include <algorithm>

// vertex coordinates closer than this to exact integers are rounded
#ifndef VERTEX_XY_SNAP
#  define VERTEX_XY_SNAP  0.03125f
#endif

static const DDSTexture defaultTexture_N(0xFFFF8080U);
static const DDSTexture defaultTexture_S(0xFF0040C0U);
static const DDSTexture defaultTexture_L(0xFF00F042U);
static const DDSTexture defaultTexture_R(0xFF0A0A0AU);

FloatVector4 Plot3D_TriShape::colorToSRGB(FloatVector4 c)
{
  FloatVector4  tmp(c);
  tmp.squareRoot();
  tmp = (tmp * FloatVector4(7.17074622f)) + FloatVector4(9.51952724f);
  tmp.squareRoot();
  return (tmp - FloatVector4(3.08537311f)).maxValues(FloatVector4(0.0f));
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
  NIFFile::NIFVertexTransform mt(vertexTransform);
  mt *= modelTransform;
  NIFFile::NIFVertexTransform vt(viewTransform);
  if (flags & 0x08)                     // decal
    vt.offsZ = vt.offsZ - 0.0625f;
  NIFFile::NIFVertexTransform xt(mt);
  xt *= vt;
  {
    NIFFile::NIFBounds  b;
    FloatVector4  rX(xt.rotateXX, xt.rotateYX, xt.rotateZX, xt.rotateXY);
    FloatVector4  rY(xt.rotateXY, xt.rotateYY, xt.rotateZY, xt.rotateXZ);
    FloatVector4  rZ(xt.rotateXZ, xt.rotateYZ, xt.rotateZZ, xt.scale);
    FloatVector4  scale(xt.scale);
    FloatVector4  offsXYZ(xt.offsX, xt.offsY, xt.offsZ, 0.0f);
    scale[3] = 0.0f;
    for (size_t i = 0; i < vertexCnt; i++)
    {
      Vertex  v(vertexData[i]);
      v.xyz = ((FloatVector4(v.xyz[0]) * rX) + (FloatVector4(v.xyz[1]) * rY)
               + (FloatVector4(v.xyz[2]) * rZ)) * scale + offsXYZ;
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

inline FloatVector4 Plot3D_TriShape::calculateLighting_FO76(
    FloatVector4 c, FloatVector4 e, float specular, float ao, FloatVector4 f0,
    float txtU, float txtV, float nDotL, float nDotV, float smoothness) const
{
  float   l = (nDotL > 0.0f ? nDotL : 0.0f);
  float   v = float(std::fabs(nDotV));
  FloatVector4  s(specularColorFloat * FloatVector4(specular * specularLevel));
  e *= reflectionLevel;
  s *= lightColor;
  // geometry function
  float   r = 0.96875f - (smoothness * 0.9375f);
  float   kEnv = r * r * 0.5f;
  float   kSpec = (kEnv + r + 0.5f) * 0.25f;    // = (r + 1) * (r + 1) / 8
  FloatVector4  g((ao * v) / (v * (1.0f - kEnv) + kEnv));
  g *= envColor;
  e *= g;
  c *= ((FloatVector4(l) * lightColor) + (ambientLight * g));
  s /= ((l * (1.0f - kSpec) + kSpec) * (v * (1.0f - kSpec) + kSpec));
  s *= (l * v);
  // Fresnel (coefficients are optimized for glass) with roughness
  float   v2 = v * v;
  float   f =
      FloatVector4(v, v2, v * v2, v2 * v2).dotProduct(
          FloatVector4(-2.93238548f, 4.14755923f, -3.28213005f, 1.06695631f))
      + 1.0f;
  FloatVector4  f2(f0 + ((FloatVector4(smoothness).maxValues(f0) - f0)
                         * FloatVector4(f * f)));
  c += (((e + s) - c) * f2);
  if (BRANCH_EXPECT(textureGlow, false))
    c += textureGlow->getPixelT(txtU, txtV, mipLevel + mipOffsetG).srgbExpand();
  c *= FloatVector4(lightColor[3]);
  return c.srgbCompress();
}

inline FloatVector4 Plot3D_TriShape::calculateLighting_FO4(
    FloatVector4 c, FloatVector4 e, float specular, float specLevel,
    float txtU, float txtV, float nDotL, float nDotV, float smoothness) const
{
  float   l = (nDotL > 0.0f ? nDotL : 0.0f);
  float   v = float(std::fabs(nDotV));
  FloatVector4  s(specularColorFloat * lightColor);
  e *= (reflectionLevel * specLevel);
  s *= (specular * specularLevel * specLevel);
  // geometry function
  float   r = 0.96875f - (smoothness * 0.9375f);
  float   kEnv = r * r * 0.5f;
  float   kSpec = (kEnv + r + 0.5f) * 0.25f;    // = (r + 1) * (r + 1) / 8
  FloatVector4  g(v / (v * (1.0f - kEnv) + kEnv));
  g *= envColor;
  c *= ((FloatVector4(l) * lightColor) + (ambientLight * g));
  c += (e * g);
  s /= ((l * (1.0f - kSpec) + kSpec) * (v * (1.0f - kSpec) + kSpec));
  // Fresnel (coefficients are optimized for glass) with roughness
  float   v2 = v * v;
  float   f =
      FloatVector4(v, v2, v * v2, v2 * v2).dotProduct(
          FloatVector4(-2.93238548f, 4.14755923f, -3.28213005f, 1.06695631f))
      + 1.0f;
  float   f0 = 0.25f;
  f = f0 + (((smoothness > f0 ? smoothness : f0) - f0) * f);
  c += (s * FloatVector4(l * v * f));
  c *= 255.0f;
  if (BRANCH_EXPECT(textureGlow, false))
    c += textureGlow->getPixelT(txtU, txtV, mipLevel + mipOffsetG);
  return (c * FloatVector4(lightColor[3]));
}

inline FloatVector4 Plot3D_TriShape::calculateLighting_TES5(
    FloatVector4 c, FloatVector4 e, float specular, float envLevel,
    float specLevel, float txtU, float txtV, float nDotL, float nDotV) const
{
  float   l = (nDotL > 0.0f ? nDotL : 0.0f);
  float   v = float(std::fabs(nDotV));
  float   g = ((nDotL > -0.125f ? nDotL : -0.125f) + 0.125f) * (1.0f / 1.125f);
  g = g / (g * 0.875f + 0.125f);
  // geometry function for ambient light only
  FloatVector4  a(v / (v * 0.875f + 0.125f));
  a *= envColor;
  FloatVector4  s(specularColorFloat * lightColor);
  e *= (FloatVector4(reflectionLevel * envLevel) * a);
  s *= (specular * specularLevel * specLevel * g);
  FloatVector4  glow(0.0f);
  if (BRANCH_EXPECT(textureGlow, false))
  {
    glow = textureGlow->getPixelT(txtU, txtV, mipLevel + mipOffsetG);
    glow *= (c * FloatVector4(4.0f / 255.0f));
  }
  c *= ((FloatVector4(l) * lightColor) + (ambientLight * a));
  return ((c + glow + e + s) * FloatVector4(lightColor[3] * 255.0f));
}

inline FloatVector4 Plot3D_TriShape::calculateLighting_FO76(
    FloatVector4 c, FloatVector4 n, float txtU, float txtV) const
{
  float   nDotL = n.dotProduct3(lightVector);
  float   nDotV = -(n[2]);
  float   l = (nDotL > 0.0f ? nDotL : 0.0f);
  float   v = float(std::fabs(nDotV));
  // assume roughness = 1.0, f0 = 0.04, specular = 0.25
  FloatVector4  e(ambientLight);
  FloatVector4  s(specularColorFloat * FloatVector4(0.53125f * specularLevel));
  s *= lightColor;
  // geometry function (kEnv = kSpec = 0.5), multiplied with ao = 0.94
  FloatVector4  g(v / (v * 0.53125f + 0.53125f));
  e *= envColor;
  e *= g;
  c *= (FloatVector4(l) * lightColor + e);
  e *= reflectionLevel;
  s *= (g * FloatVector4(l / (l + 1.0f)));
  c = c + ((e + s - c) * FloatVector4(0.04f));
  if (BRANCH_EXPECT(textureGlow, false))
    c += textureGlow->getPixelT(txtU, txtV, mipLevel + mipOffsetG).srgbExpand();
  return (c * FloatVector4(lightColor[3])).srgbCompress();
}

inline FloatVector4 Plot3D_TriShape::calculateLighting_FO4(
    FloatVector4 c, FloatVector4 n, float txtU, float txtV) const
{
  float   nDotL = n.dotProduct3(lightVector);
  float   nDotV = -(n[2]);
  float   l = (nDotL > 0.0f ? nDotL : 0.0f);
  float   v = float(std::fabs(nDotV));
  // geometry function for ambient light only
  FloatVector4  g(v / (v * 0.875f + 0.125f));
  g *= envColor;
  c *= ((FloatVector4(l) * lightColor) + (ambientLight * g));
  c *= 255.0f;
  if (BRANCH_EXPECT(textureGlow, false))
    c += textureGlow->getPixelT(txtU, txtV, mipLevel + mipOffsetG);
  return (c * FloatVector4(lightColor[3]));
}

inline FloatVector4 Plot3D_TriShape::calculateLighting_TES5(
    FloatVector4 c, FloatVector4 n, float txtU, float txtV) const
{
  float   nDotL = n.dotProduct3(lightVector);
  float   nDotV = -(n[2]);
  float   l = (nDotL > 0.0f ? nDotL : 0.0f);
  float   v = float(std::fabs(nDotV));
  // geometry function for ambient light only
  FloatVector4  g(v / (v * 0.875f + 0.125f));
  g *= envColor;
  FloatVector4  glow(0.0f);
  if (BRANCH_EXPECT(textureGlow, false))
  {
    glow = textureGlow->getPixelT(txtU, txtV, mipLevel + mipOffsetG);
    glow *= (c * FloatVector4(4.0f / 255.0f));
  }
  c *= ((FloatVector4(l) * lightColor) + (ambientLight * g));
  return ((c + glow) * FloatVector4(lightColor[3] * 255.0f));
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

inline FloatVector4 Plot3D_TriShape::calculateReflection(const Vertex& v) const
{
  // view vector
  FloatVector4  viewVector(v.xyz[0], v.xyz[1], 0.0f, 0.0f);
  viewVector += envMapOffs;
  // reflect (I - 2.0 * dot(N, I) * N)
  viewVector -= (FloatVector4(2.0f * viewVector.dotProduct3(v.normal))
                 * v.normal);
  return viewVector.normalize();
}

inline FloatVector4 Plot3D_TriShape::environmentMap(
    FloatVector4 reflectedView, float smoothness, bool isSRGB, bool invZ) const
{
  if (BRANCH_EXPECT(!textureE, false))
    return ambientLight;
  // inverse rotation by view matrix
  FloatVector4  v((FloatVector4(reflectedView[0]) * viewTransformInvX)
                  + (FloatVector4(reflectedView[1]) * viewTransformInvY)
                  + (FloatVector4(reflectedView[2]) * viewTransformInvZ));
  float   m = (1.0f - smoothness) * float(textureE->getMaxMipLevel());
  FloatVector4  e(textureE->cubeMap(v[0], v[1], (!invZ ? v[2] : -(v[2])), m));
  if (!isSRGB)
    return (e * FloatVector4(1.0f / 255.0f));
  return e.srgbExpand();
}

inline float Plot3D_TriShape::specularPhong(
    FloatVector4 reflectedView, float smoothness, float nDotL,
    bool isNormalized) const
{
  if (!((specularLevel * nDotL) > 0.0f))
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
  if (isNormalized)
    s = s * (g * 0.154375f + 0.095625f);        // 0.25 at smoothness = 0.0
  return s;
}

inline float Plot3D_TriShape::specularGGX(
    FloatVector4 reflectedView, float smoothness, float nDotL) const
{
  if (!((specularLevel * nDotL) > 0.0f))
    return 0.0f;
  // h2 = 2.0 * squared dot product with halfway vector
  float   h2 = reflectedView.dotProduct3(lightVector) + 1.0f;
  float   r = 0.96875f - (smoothness * 0.9375f);
  float   a = r * r;
  float   a2 = a * a;
  float   d = h2 * (a2 - 1.0f) + 2.0f;
  float   s = a2 / (d * d);
  // return D only, F and G are implemented in calculateLighting_FO76()
  return s;
}

inline FloatVector4 Plot3D_TriShape::calculateLighting_Water(
    FloatVector4 c0, FloatVector4 n, FloatVector4 reflectedView, float a,
    bool isFO76) const
{
  FloatVector4  e(environmentMap(reflectedView, 1.0f,
                                 isFO76, (!isFO76 && renderMode < 8)));
  float   nDotL = n.dotProduct3(lightVector);
  float   nDotV = -(n[2]);
  float   specular;
  if (!isFO76)
    specular = specularPhong(reflectedView, 0.875f, nDotL, true);
  else
    specular = specularGGX(reflectedView, 0.875f, nDotL);
  float   l = (nDotL > 0.0f ? nDotL : 0.0f);
  float   v = float(std::fabs(nDotV));
  FloatVector4  c(specularColorFloat);
  if (!isFO76)
    c0 *= (a * (1.0f / 255.0f));
  else
    c0 = c0.srgbExpand() * FloatVector4(a);
  c *= ((FloatVector4(l) * lightColor) + (ambientLight * envColor));
  c *= (lightColor[3] * (1.0f - a));
  c += c0;
  FloatVector4  s(specular * specularLevel);
  e *= reflectionLevel;
  e *= envColor;
  s *= lightColor;
  // geometry function (roughness assumed to be zero, kEnv = 0.0, kSpec = 0.125)
  s *= ((l * v) / ((l * 0.875f + 0.125f) * (v * 0.875f + 0.125f)));
  s += e;
  s *= lightColor[3];
  // Fresnel (water)
  float   v2 = v * v;
  float   f =
      FloatVector4(v, v2, v * v2, v2 * v2).dotProduct(
          FloatVector4(-3.13316934f, 4.57764210f, -3.58729012f, 1.14281736f));
  if (!isFO76)
  {
    f = f * ((1.0f - 0.15299f) * 255.0f);       // 0.0203 sRGB compressed
    return ((s * FloatVector4(f + 255.0f)) - (c * FloatVector4(f)));
  }
  f = f * (f + 2.0f) * (1.0f - 0.0203f);
  return ((s * FloatVector4(f + 1.0f)) - (c * FloatVector4(f))).srgbCompress();
}

void Plot3D_TriShape::drawPixel_Water(
    Plot3D_TriShape& p, size_t offs, Vertex& z)
{
  // first pass: only write depth and alpha
  float   d = float(int(~(p.bufRGBA[offs] >> 24) & 0xFFU));
  d = (d * d) + ((p.bufZ[offs] - z.xyz[2]) * (16.0f / p.viewScale));
  d = FloatVector4::squareRootFast(d < 65025.0f ? d : 65025.0f);
  p.bufZ[offs] = z.xyz[2] * 1.00000025f + 0.00000025f;
  // alpha channel = 255 - sqrt(waterDepth*16)
  p.bufRGBA[offs] = (p.bufRGBA[offs] & 0x00FFFFFFU)
                    | (std::uint32_t(255 - roundFloat(d)) << 24);
}

void Plot3D_TriShape::drawPixel_Water_2(
    Plot3D_TriShape& p, size_t offs, Vertex& z)
{
  p.bufZ[offs] = z.xyz[2];
  float   txtU = z.bitangent[3];
  float   txtV = z.tangent[3];
  FloatVector4  c0(p.bufRGBA[offs]);
  float   a = 255.0f - c0[3];
  a = (a * a) * p.specularColorFloat[3];
  if (a > -3.0f && a < 0.0f)
    a = FloatVector4::exp2Fast(a - 1.0f);
  else
    a = (a < -2.0f ? 0.0625f : 0.5f);
  FloatVector4  n(p.normalMap(
                      z, p.textureN->getPixelT(txtU, txtV, p.mipLevel)));
  FloatVector4  reflectedView(p.calculateReflection(z));
  FloatVector4  c(p.calculateLighting_Water(c0, n, reflectedView, a, false));
  p.bufRGBA[offs] = std::uint32_t(c) | 0xFF000000U;
}

void Plot3D_TriShape::drawPixel_Water_2G(
    Plot3D_TriShape& p, size_t offs, Vertex& z)
{
  p.bufZ[offs] = z.xyz[2];
  float   txtU = z.bitangent[3];
  float   txtV = z.tangent[3];
  FloatVector4  c0(p.bufRGBA[offs]);
  float   a = 255.0f - c0[3];
  a = (a * a) * p.specularColorFloat[3];
  if (a > -3.0f && a < 0.0f)
    a = FloatVector4::exp2Fast(a - 1.0f);
  else
    a = (a < -2.0f ? 0.0625f : 0.5f);
  FloatVector4  n(p.normalMap(
                      z, p.textureN->getPixelT(txtU, txtV, p.mipLevel)));
  FloatVector4  reflectedView(p.calculateReflection(z));
  FloatVector4  c(p.calculateLighting_Water(c0, n, reflectedView, a, true));
  p.bufRGBA[offs] = std::uint32_t(c) | 0xFF000000U;
}

inline FloatVector4 Plot3D_TriShape::getDiffuseColor(
    float txtU, float txtV, const Vertex& z, bool& alphaFlag, bool isFO76) const
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
    if (!isFO76)
      c *= (1.0f / 255.0f);
    else
      c.srgbExpand();
  }
  else
  {
    if (!isFO76)
      c *= (1.0f / 255.0f);
    else
      c.srgbExpand();
    c *= vColor;
  }
#if 0
  c[3] = a * (1.0f / 255.0f);           // currently unused
#endif
  return c;
}

void Plot3D_TriShape::drawPixel_Debug(
    Plot3D_TriShape& p, size_t offs, Vertex& z)
{
  float   txtU = z.bitangent[3];
  float   txtV = z.tangent[3];
  FloatVector4  c(0.0f);
  if (BRANCH_EXPECT(p.textureD, true))
  {
    bool    alphaFlag = false;
    c = p.getDiffuseColor(txtU, txtV, z, alphaFlag, (p.renderMode >= 12));
    if (BRANCH_EXPECT(!alphaFlag, false))
      return;
    if (p.debugMode == 5)
      c = FloatVector4(1.0f);
  }
  p.bufZ[offs] = z.xyz[2];
  if (!(p.debugMode == 0 || p.debugMode == 3 || p.debugMode == 5))
  {
    std::uint32_t tmp = std::uint32_t(p.debugMode);
    if (p.debugMode != 4)
    {
      if (!(tmp & 0x80000000U))
        tmp = std::uint32_t(roundFloat(z.xyz[2] * 16.0f));
      tmp = (tmp & 0xFF00FF00U) | ((tmp & 0xFFU) << 16) | ((tmp >> 16) & 0xFFU);
    }
    else if (p.renderMode >= 12)
    {
      tmp = std::uint32_t(c.srgbCompress());
    }
    else
    {
      tmp = std::uint32_t(c * FloatVector4(255.0f));
    }
    p.bufRGBA[offs] = tmp | 0xFF000000U;
    return;
  }
  FloatVector4  n(p.textureN->getPixelT(txtU, txtV, p.mipLevel + p.mipOffsetN));
  n = p.normalMap(z, n);
  if (p.debugMode == 3)
  {
    c = n * FloatVector4(127.5f, -127.5f, -127.5f, 0.0f);
    c += 127.5f;
  }
  else
  {
    if (p.renderMode >= 12)
      c = p.calculateLighting_FO76(c, n, txtU, txtV);
    else if (p.renderMode >= 8)
      c = p.calculateLighting_FO4(c, n, txtU, txtV);
    else
      c = p.calculateLighting_TES5(c, n, txtU, txtV);
  }
  p.bufRGBA[offs] = std::uint32_t(c) | 0xFF000000U;
}

void Plot3D_TriShape::drawPixel_D_TES5(
    Plot3D_TriShape& p, size_t offs, Vertex& z)
{
  // diffuse texture only
  float   txtU = z.bitangent[3];
  float   txtV = z.tangent[3];
  bool    alphaFlag = false;
  FloatVector4  c(p.getDiffuseColor(txtU, txtV, z, alphaFlag, false));
  if (BRANCH_EXPECT(!alphaFlag, false))
    return;
  p.bufZ[offs] = z.xyz[2];
  z.normal.normalize(p.invNormals);
  c = p.calculateLighting_TES5(c, z.normal, txtU, txtV);
  p.bufRGBA[offs] = std::uint32_t(c) | 0xFF000000U;
}

void Plot3D_TriShape::drawPixel_N_TES5(
    Plot3D_TriShape& p, size_t offs, Vertex& z)
{
  // normal map only
  float   txtU = z.bitangent[3];
  float   txtV = z.tangent[3];
  bool    alphaFlag = false;
  FloatVector4  c(p.getDiffuseColor(txtU, txtV, z, alphaFlag, false));
  if (BRANCH_EXPECT(!alphaFlag, false))
    return;
  p.bufZ[offs] = z.xyz[2];
  FloatVector4  n(p.textureN->getPixelT(txtU, txtV, p.mipLevel + p.mipOffsetN));
  c = p.calculateLighting_TES5(c, p.normalMap(z, n), txtU, txtV);
  p.bufRGBA[offs] = std::uint32_t(c) | 0xFF000000U;
}

void Plot3D_TriShape::drawPixel_TES5(Plot3D_TriShape& p, size_t offs, Vertex& z)
{
  float   txtU = z.bitangent[3];
  float   txtV = z.tangent[3];
  bool    alphaFlag = false;
  FloatVector4  c(p.getDiffuseColor(txtU, txtV, z, alphaFlag, false));
  if (BRANCH_EXPECT(!alphaFlag, false))
    return;
  p.bufZ[offs] = z.xyz[2];
  FloatVector4  n(p.textureN->getPixelT(txtU, txtV, p.mipLevel + p.mipOffsetN));
  float   specLevel = n[3] * (1.0f / 255.0f);
  float   smoothness = float(int(p.specularSmoothness)) * (1.0f / 255.0f);
  float   nDotL = p.normalMap(z, n).dotProduct3(p.lightVector);
  float   nDotV = -(z.normal[2]);
  FloatVector4  reflectedView(p.calculateReflection(z));
  FloatVector4  e(p.environmentMap(reflectedView, smoothness, false, true));
  float   envLevel = specLevel;
  if (p.textureS)
  {
    envLevel = p.textureS->getPixelT(txtU, txtV, p.mipLevel + p.mipOffsetS)[0];
    envLevel *= (1.0f / 255.0f);
  }
  float   specular = p.specularPhong(reflectedView, smoothness, nDotL, false);
  c = p.calculateLighting_TES5(c, e, specular, envLevel,
                               specLevel, txtU, txtV, nDotL, nDotV);
  p.bufRGBA[offs] = std::uint32_t(c) | 0xFF000000U;
}

void Plot3D_TriShape::drawPixel_D_FO4(
    Plot3D_TriShape& p, size_t offs, Vertex& z)
{
  // diffuse texture only
  float   txtU = z.bitangent[3];
  float   txtV = z.tangent[3];
  bool    alphaFlag = false;
  FloatVector4  c(p.getDiffuseColor(txtU, txtV, z, alphaFlag, false));
  if (BRANCH_EXPECT(!alphaFlag, false))
    return;
  p.bufZ[offs] = z.xyz[2];
  z.normal.normalize(p.invNormals);
  c = p.calculateLighting_FO4(c, z.normal, txtU, txtV);
  p.bufRGBA[offs] = std::uint32_t(c) | 0xFF000000U;
}

void Plot3D_TriShape::drawPixel_N_FO4(
    Plot3D_TriShape& p, size_t offs, Vertex& z)
{
  // normal map only
  float   txtU = z.bitangent[3];
  float   txtV = z.tangent[3];
  bool    alphaFlag = false;
  FloatVector4  c(p.getDiffuseColor(txtU, txtV, z, alphaFlag, false));
  if (BRANCH_EXPECT(!alphaFlag, false))
    return;
  p.bufZ[offs] = z.xyz[2];
  FloatVector4  n(p.textureN->getPixelT(txtU, txtV, p.mipLevel + p.mipOffsetN));
  c = p.calculateLighting_FO4(c, p.normalMap(z, n), txtU, txtV);
  p.bufRGBA[offs] = std::uint32_t(c) | 0xFF000000U;
}

void Plot3D_TriShape::drawPixel_FO4(Plot3D_TriShape& p, size_t offs, Vertex& z)
{
  float   txtU = z.bitangent[3];
  float   txtV = z.tangent[3];
  bool    alphaFlag = false;
  FloatVector4  c(p.getDiffuseColor(txtU, txtV, z, alphaFlag, false));
  if (BRANCH_EXPECT(!alphaFlag, false))
    return;
  p.bufZ[offs] = z.xyz[2];
  FloatVector4  n(p.textureN->getPixelT_2(txtU, txtV, p.mipLevel + p.mipOffsetN,
                                          *(p.textureS)));
  float   specLevel = n[2] * (1.0f / 255.0f);
  float   smoothness =
      float(int(p.specularSmoothness)) * n[3] * (1.0f / (255.0f * 255.0f));
  float   nDotL = p.normalMap(z, n).dotProduct3(p.lightVector);
  float   nDotV = -(z.normal[2]);
  FloatVector4  reflectedView(p.calculateReflection(z));
  FloatVector4  e(p.environmentMap(reflectedView, smoothness, false, false));
  float   specular = p.specularPhong(reflectedView, smoothness, nDotL, true);
  c = p.calculateLighting_FO4(c, e, specular, specLevel,
                              txtU, txtV, nDotL, nDotV, smoothness);
  p.bufRGBA[offs] = std::uint32_t(c) | 0xFF000000U;
}

void Plot3D_TriShape::drawPixel_D_FO76(
    Plot3D_TriShape& p, size_t offs, Vertex& z)
{
  // diffuse texture only
  float   txtU = z.bitangent[3];
  float   txtV = z.tangent[3];
  bool    alphaFlag = false;
  FloatVector4  c(p.getDiffuseColor(txtU, txtV, z, alphaFlag, true));
  if (BRANCH_EXPECT(!alphaFlag, false))
    return;
  p.bufZ[offs] = z.xyz[2];
  z.normal.normalize(p.invNormals);
  c = p.calculateLighting_FO76(c, z.normal, txtU, txtV);
  p.bufRGBA[offs] = std::uint32_t(c) | 0xFF000000U;
}

void Plot3D_TriShape::drawPixel_N_FO76(
    Plot3D_TriShape& p, size_t offs, Vertex& z)
{
  // normal map only
  float   txtU = z.bitangent[3];
  float   txtV = z.tangent[3];
  bool    alphaFlag = false;
  FloatVector4  c(p.getDiffuseColor(txtU, txtV, z, alphaFlag, true));
  if (BRANCH_EXPECT(!alphaFlag, false))
    return;
  p.bufZ[offs] = z.xyz[2];
  FloatVector4  n(p.textureN->getPixelT(txtU, txtV, p.mipLevel + p.mipOffsetN));
  c = p.calculateLighting_FO76(c, p.normalMap(z, n), txtU, txtV);
  p.bufRGBA[offs] = std::uint32_t(c) | 0xFF000000U;
}

void Plot3D_TriShape::drawPixel_FO76(Plot3D_TriShape& p, size_t offs, Vertex& z)
{
  float   txtU = z.bitangent[3];
  float   txtV = z.tangent[3];
  bool    alphaFlag = false;
  FloatVector4  c(p.getDiffuseColor(txtU, txtV, z, alphaFlag, true));
  if (BRANCH_EXPECT(!alphaFlag, false))
    return;
  p.bufZ[offs] = z.xyz[2];
  FloatVector4  n(p.textureN->getPixelT_2(txtU, txtV, p.mipLevel + p.mipOffsetN,
                                          *(p.textureS)));
  FloatVector4  r(p.textureR->getPixelT(txtU, txtV, p.mipLevel + p.mipOffsetR));
  r.srgbExpand();
  r.maxValues(FloatVector4(0.015625f));
  float   smoothness =
      float(int(p.specularSmoothness)) * (n[2] * (1.0f / (255.0f * 255.0f)));
  float   ao = n[3] * (1.0f / 255.0f);
  float   nDotL = p.normalMap(z, n).dotProduct3(p.lightVector);
  float   nDotV = -(z.normal[2]);
  FloatVector4  reflectedView(p.calculateReflection(z));
  FloatVector4  e(p.environmentMap(reflectedView, smoothness, true, false));
  float   specular = p.specularGGX(reflectedView, smoothness, nDotL);
  c = p.calculateLighting_FO76(c, e, specular, ao, r,
                               txtU, txtV, nDotL, nDotV, smoothness);
  p.bufRGBA[offs] = std::uint32_t(c) | 0xFF000000U;
}

inline void Plot3D_TriShape::drawPixel(
    int x, int y, Vertex& v,
    const Vertex& v0, const Vertex& v1, const Vertex& v2,
    float w0f, float w1f, float w2f)
{
  if (BRANCH_EXPECT(((unsigned int) x >= (unsigned int) width), false))
    return;
  FloatVector4  w0(w0f);
  FloatVector4  w1(w1f);
  FloatVector4  w2(w2f);
  v.xyz = (v0.xyz * w0) + (v1.xyz * w1) + (v2.xyz * w2);
  if (BRANCH_EXPECT((v.xyz[2] < 0.0f), false))
    return;
  size_t  offs = size_t(y) * size_t(width) + size_t(x);
  if (!(v.xyz[2] < bufZ[offs]))
    return;
  v.bitangent = (v0.bitangent * w0) + (v1.bitangent * w1) + (v2.bitangent * w2);
  v.tangent = (v0.tangent * w0) + (v1.tangent * w1) + (v2.tangent * w2);
  v.normal = (v0.normal * w0) + (v1.normal * w1) + (v2.normal * w2);
  v.vertexColor =
      (v0.vertexColor * w0) + (v1.vertexColor * w1) + (v2.vertexColor * w2);
  drawPixelFunction(*this, offs, v);
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
          drawPixel(x, y, v, v0, v1, v0, float(1.0 - w1), float(w1), 0.0f);
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
          drawPixel(x, y, v, v0, v1, v0, float(1.0 - w1), float(w1), 0.0f);
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
    std::uint32_t *outBufRGBA, float *outBufZ, int imageWidth, int imageHeight,
    unsigned int mode)
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
    renderMode((unsigned char) (mode & 15U)),
    usingSRGBColorSpace(renderMode < 12),
    debugMode(0U),
    lightVector(0.0f, 0.0f, 1.0f, 0.0f),
    lightColor(1.0f),
    ambientLight(renderMode < 12 ? 0.2497363f : 0.05f),
    envColor(1.0f),
    envMapOffs(float(imageWidth) * -0.5f, float(imageHeight) * -0.5f,
               float(imageHeight), 0.0f),
    viewTransformInvX(1.0f, 0.0f, 0.0f, 0.0f),
    viewTransformInvY(0.0f, 1.0f, 0.0f, 0.0f),
    viewTransformInvZ(0.0f, 0.0f, 1.0f, 0.0f),
    specularColorFloat(1.0f),
    reflectionLevel(0.0f),
    specularLevel(0.0f),
    viewScale(1.0f),
    drawPixelFunction(&drawPixel_Water)
{
}

Plot3D_TriShape::~Plot3D_TriShape()
{
}

void Plot3D_TriShape::setLighting(
    FloatVector4 c, FloatVector4 a, FloatVector4 e, float s)
{
  lightColor = c;
  lightColor[3] = s;
  ambientLight = a;
  envColor = e;
  if (usingSRGBColorSpace)
  {
    lightColor = colorToSRGB(lightColor);
    ambientLight = colorToSRGB(ambientLight);
    envColor = colorToSRGB(envColor);
  }
  ambientLight[3] = 0.0f;
  envColor[3] = 0.0f;
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
  texturePathMask = t.texturePathMask;
  texturePaths = t.texturePaths;
  materialPath = t.materialPath;
  textureOffsetU = t.textureOffsetU;
  textureOffsetV = t.textureOffsetV;
  textureScaleU = t.textureScaleU;
  textureScaleV = t.textureScaleV;
  name = t.name;
  return (*this);
}

static float calculateMipOffset(const DDSTexture *t1, const DDSTexture *t2)
{
  int     a1 = t1->getWidth() * t1->getHeight();
  int     a2 = t2->getWidth() * t2->getHeight();
  int     m = FloatVector4::log2Int(a1) - FloatVector4::log2Int(a2);
  if (m < 0)
    return float(-((1 - m) >> 1));
  m = (m + 1) >> 1;
  return float(m < 2 ? m : 2);
}

void Plot3D_TriShape::drawTriShape(
    const NIFFile::NIFVertexTransform& modelTransform,
    const NIFFile::NIFVertexTransform& viewTransform,
    float lightX, float lightY, float lightZ,
    const DDSTexture * const *textures, unsigned int textureMask)
{
  if (!((textureMask & 1U) | (flags & 0x02U)))
    return;
  lightVector = FloatVector4(lightX, lightY, lightZ, 0.0f);
  size_t  triangleCntRendered =
      transformVertexData(modelTransform, viewTransform);
  if (!triangleCntRendered)
    return;
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
  reflectionLevel = float(int(envMapScale)) * (1.0f / 128.0f);
  specularLevel = float(int(specularScale)) * (1.0f / 128.0f);
  if (BRANCH_EXPECT((flags & 0x02), false))
  {
    drawPixelFunction = &drawPixel_Water;
    drawTriangles();
    return;
  }
  int     mode = (!debugMode ? int(renderMode) : 0);
  textureD = textures[0];
  switch (mode)
  {
    case 4:                             // Skyrim
      drawPixelFunction = &drawPixel_D_TES5;
      break;
    case 5:
      drawPixelFunction = &drawPixel_N_TES5;
      break;
    case 6:
    case 7:
      textureS = (DDSTexture *) 0;
      if (textureMask & 0x0020U)
      {
        textureS = textures[5];         // environment mask
        mipOffsetS = calculateMipOffset(textureS, textureD);
      }
      drawPixelFunction = &drawPixel_TES5;
      break;
    case 8:                             // Fallout 4
      drawPixelFunction = &drawPixel_D_FO4;
      break;
    case 9:
      drawPixelFunction = &drawPixel_N_FO4;
      break;
    case 10:
    case 11:
      if (BRANCH_EXPECT(!(textureMask & 0x0040U), false))
        textureS = &defaultTexture_S;
      else
        textureS = textures[6];
      drawPixelFunction = &drawPixel_FO4;
      break;
    case 12:                            // Fallout 76
      drawPixelFunction = &drawPixel_D_FO76;
      break;
    case 13:
      drawPixelFunction = &drawPixel_N_FO76;
      break;
    case 14:
    case 15:
      if (BRANCH_EXPECT(!(textureMask & 0x0100U), false))
        textureR = &defaultTexture_R;
      else
        textureR = textures[8];
      mipOffsetR = calculateMipOffset(textureR, textureD);
      if (BRANCH_EXPECT(!(textureMask & 0x0200U), false))
        textureS = &defaultTexture_L;
      else
        textureS = textures[9];
      drawPixelFunction = &drawPixel_FO76;
      break;
    default:                            // debug
      drawPixelFunction = &drawPixel_Debug;
      break;
  }
  textureG = (DDSTexture *) 0;
  textureE = (DDSTexture *) 0;
  textureGlow = (DDSTexture *) 0;
  if (BRANCH_EXPECT(!(textureMask & 0x0002U), false))
    textureN = &defaultTexture_N;
  else
    textureN = textures[1];
  mipOffsetN = calculateMipOffset(textureN, textureD);
  if (BRANCH_EXPECT((textureMask & 0x0004U), false))
  {
    if (flags & 0x80)
    {
      textureGlow = textures[2];
      mipOffsetG = calculateMipOffset(textureGlow, textureD);
    }
  }
  if (textureMask & 0x0008U)
    textureG = textures[3];
  if (textureMask & 0x0010U)
    textureE = textures[4];
  drawTriangles();
}

void Plot3D_TriShape::drawWater(
    const NIFFile::NIFVertexTransform& modelTransform,
    const NIFFile::NIFVertexTransform& viewTransform,
    float lightX, float lightY, float lightZ,
    const DDSTexture * const *textures, unsigned int textureMask,
    std::uint32_t waterColor, float envMapLevel)
{
  if (BRANCH_EXPECT(!(flags & 0x02), false))
    return;
  lightVector = FloatVector4(lightX, lightY, lightZ, 0.0f);
  size_t  triangleCntRendered =
      transformVertexData(modelTransform, viewTransform);
  if (!triangleCntRendered)
    return;
  mipOffsetN = 0.0f;
  mipLevel = 15.0f;
  alphaThresholdFloat = 0.0f;
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
  if (usingSRGBColorSpace)
    specularColorFloat *= (1.0f / 255.0f);
  else
    specularColorFloat.srgbExpand();
  specularColorFloat[3] = -1.5f / (a * a);      // = -3.0 / (maxDepth * 16.0)
  reflectionLevel = envMapLevel;
  specularLevel = envMapLevel;
  textureN = &defaultTexture_N;
  if (textureMask & 2U)
    textureN = textures[1];
  else if (textureMask & 1U)
    textureN = textures[0];
  textureD = textureN;
  drawPixelFunction =
      (usingSRGBColorSpace ? &drawPixel_Water_2 : &drawPixel_Water_2G);
  if (BRANCH_EXPECT(debugMode, false))
  {
    textureG = (DDSTexture *) 0;
    textureGlow = (DDSTexture *) 0;
    drawPixelFunction = &drawPixel_Debug;
  }
  textureE = (DDSTexture *) 0;
  if (textureMask & 16U)
    textureE = textures[4];
  drawTriangles();
}

FloatVector4 Plot3D_TriShape::cubeMapToAmbient(const DDSTexture *e) const
{
  if (!e)
    return FloatVector4(0.05f);
  FloatVector4  s(0.0f);
  int     w = 0;
  int     n = int(e->getTextureCount());
  int     m = e->getMaxMipLevel();
  int     txtW = e->getWidth() >> m;
  int     txtH = e->getHeight() >> m;
  bool    isSRGB = usingSRGBColorSpace;
  for (int i = 0; i < n; i++)
  {
    for (int y = 0; y < txtH; y++)
    {
      if (isSRGB)
      {
        for (int x = 0; x < txtW; x++, w++)
          s += FloatVector4(e->getPixelN(x, y, m, i));
      }
      else
      {
        for (int x = 0; x < txtW; x++, w++)
          s += FloatVector4(e->getPixelN(x, y, m, i)).srgbExpand();
      }
    }
  }
  if (w < 1)
    return FloatVector4(0.05f);
  s /= w;
  if (isSRGB)
    s.srgbExpand();
  return s;
}

