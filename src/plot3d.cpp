
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

#include "frtable.cpp"

FloatVector4 Plot3D_TriShape::colorToSRGB(FloatVector4 c)
{
  FloatVector4  tmp(c);
  tmp.squareRoot();
  tmp = (tmp * 7.17074622f) + 9.51952724f;
  tmp.squareRoot();
  return (tmp - 3.08537311f).maxValues(FloatVector4(0.0f));
}

Plot3D_TriShape::Triangle::Triangle(size_t i, float z)
{
#if defined(__i386__) || defined(__x86_64__) || defined(__x86_64)
  union
  {
    float   z_f;
    std::int32_t  z_i;
  }
  tmp;
  tmp.z_f = z;
  if (tmp.z_i < 0)
    tmp.z_i = std::int32_t(0x80000000U) - tmp.z_i;
  n = std::int64_t(i) | (std::int64_t(tmp.z_i) << 32);
#else
  n = std::int64_t(i) | (std::int64_t(roundFloat(z * 32.0f)) << 32);
#endif
}

size_t Plot3D_TriShape::transformVertexData(
    const NIFFile::NIFVertexTransform& modelTransform)
{
  vertexBuf.resize(vertexCnt);
  triangleBuf.clear();
  if (triangleBuf.capacity() < triangleCnt)
    triangleBuf.reserve(triangleCnt);
  NIFFile::NIFVertexTransform mt(vertexTransform);
  mt *= modelTransform;
  NIFFile::NIFVertexTransform vt(viewTransform);
  if (m.flags & BGSMFile::Flag_IsDecal) // decal
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
      vertexBuf[i].xyz = ((rX * vertexData[i].x) + (rY * vertexData[i].y)
                          + (rZ * vertexData[i].z)) * scale + offsXYZ;
      b += vertexBuf[i].xyz;
    }
    if (b.xMin() >= (float(width) - 0.5f) || b.xMax() < -0.5f ||
        b.yMin() >= (float(height) - 0.5f) || b.yMax() < -0.5f ||
        b.zMin() >= 16777216.0f || b.zMax() < 0.0f)
    {
      return 0;
    }
  }
  for (size_t i = 0; i < vertexCnt; i++)
  {
    const NIFFile::NIFVertex& r = vertexData[i];
    Vertex& v = vertexBuf[i];
#ifdef VERTEX_XY_SNAP
    FloatVector4  xyzRounded(v.xyz);
    xyzRounded.roundValues();
    FloatVector4  xyzDiffSqr = v.xyz - xyzRounded;
    xyzDiffSqr *= xyzDiffSqr;
    if (xyzDiffSqr[0] < (VERTEX_XY_SNAP * VERTEX_XY_SNAP))
      v.xyz[0] = xyzRounded[0];
    if (xyzDiffSqr[1] < (VERTEX_XY_SNAP * VERTEX_XY_SNAP))
      v.xyz[1] = xyzRounded[1];
#endif
    FloatVector4  normal(r.getNormal());
    float   txtU, txtV;
    if (BRANCH_UNLIKELY(m.flags & BGSMFile::Flag_TSWater))      // water
    {
      FloatVector4  tmp(r.x, r.y, r.z, 0.0f);
      tmp = mt.transformXYZ(tmp);
      tmp *= (float(int(waterUVScale)) * (1.0f / 65536.0f));
      txtU = tmp[0];
      txtV = tmp[1];
      normal = mt.rotateXYZ(normal);
      tmp = FloatVector4(normal[2], 0.0f, -(normal[0]), 0.0f);
      tmp.normalize3Fast();
      v.bitangent = vt.rotateXYZ(tmp);
      v.bitangent[3] = txtU;
      tmp = FloatVector4(0.0f, normal[2], -(normal[1]), 0.0f);
      tmp.normalize3Fast();
      v.tangent = vt.rotateXYZ(tmp);
      v.tangent[3] = txtV;
      v.normal = vt.rotateXYZ(normal);
    }
    else
    {
      r.getUV(txtU, txtV);
      v.bitangent = xt.rotateXYZ(r.getBitangent());
      v.bitangent[3] = txtU * m.textureScaleU + m.textureOffsetU;
      v.tangent = xt.rotateXYZ(r.getTangent());
      v.tangent[3] = txtV * m.textureScaleV + m.textureOffsetV;
      v.normal = xt.rotateXYZ(normal);
    }
    v.vertexColor = FloatVector4(&(r.vertexColor)) * (1.0f / 255.0f);
    if (m.flags & BGSMFile::Flag_IsTree)
      v.vertexColor[3] = 1.0f;          // tree: ignore vertex alpha
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
    if (!(m.flags & BGSMFile::Flag_TwoSided))
    {
      // cull if vertices are not in CCW order
      FloatVector4  d1(v1.xyz - v0.xyz);
      FloatVector4  d2(v2.xyz - v0.xyz);
      d1 = FloatVector4(d1[1], d1[0], d1[3], d1[2]) * d2;
      if (d1[1] > d1[0])
        continue;
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
    FloatVector4  bndsMinInt(boundsMin);
    FloatVector4  bndsMaxInt(boundsMax);
    bndsMinInt.floorValues();
    bndsMaxInt.floorValues();
    if ((bndsMinInt[0] == bndsMaxInt[0] && boundsMin[0] != bndsMinInt[0]) ||
        (bndsMinInt[1] == bndsMaxInt[1] && boundsMin[1] != bndsMinInt[1]))
    {
      continue;                 // no sampling point within the triangle bounds
    }
#endif
    triangleBuf.emplace(triangleBuf.end(),
                        i, v0.xyz[2] + v1.xyz[2] + v2.xyz[2]);
  }
  if (BRANCH_UNLIKELY(alphaBlendingEnabled()))
    std::sort(triangleBuf.begin(), triangleBuf.end(), Triangle::gt);
  else
    std::sort(triangleBuf.begin(), triangleBuf.end());
  return triangleBuf.size();
}

inline void Plot3D_TriShape::storeNormal(
    FloatVector4 n, const std::uint32_t *cPtr)
{
#if ENABLE_TXST_DECALS
  if (BRANCH_UNLIKELY(bufN))
    bufN[size_t(cPtr - bufRGBA)] = n.normalToUInt32();
#else
  (void) n;
  (void) cPtr;
#endif
}

static inline FloatVector4 alphaBlendFunction(
    FloatVector4 c, const FloatVector4& cSrc, const FloatVector4& cDst,
    float srcAlpha, unsigned char blendMode)
{
  switch (blendMode)
  {
    case 0:
    case 8:
      return c;
    case 2:
    case 4:
      // multiplicative blending hack to fix oil puddles in Fallout 4/76
      return (cDst * ((cSrc * srcAlpha) + (1.0f - srcAlpha)));
    case 3:
      return (c - (c * cSrc));
    case 5:
      return (c - (c * cDst));
    case 6:
      return (c * srcAlpha);
    case 7:
      return (c - (c * srcAlpha));
  }
  return FloatVector4(0.0f);
}

FloatVector4 Plot3D_TriShape::alphaBlend(
    FloatVector4 c, float a, const Fragment& z) const
{
  a = a * m.alpha * (1.0f / 255.0f);
  unsigned char blendMode = (unsigned char) ((m.alphaFlags >> 1) & 0xFFU);
  if (a >= (511.0f / 512.0f) && blendMode == 0xEC)
    return c;
  FloatVector4  c0(FloatVector4::convertRGBA32(*(z.cPtr)));
  if (usingSRGBColorSpace)
    c0 *= (1.0f / 255.0f);
  else
    c0.srgbExpand();
  c0 /= lightColor[3];
  if (BRANCH_LIKELY(blendMode == 0xEC))
    return (c0 + ((c - c0) * a));
  if (blendMode == 0x0C)
    return (c0 + (c * a));
  return (alphaBlendFunction(c, c, c0, a, blendMode & 0x0F)
          + alphaBlendFunction(c0, c, c0, a, blendMode >> 4));
}

inline FloatVector4 Plot3D_TriShape::calculateLighting_FO76(
    FloatVector4 c, FloatVector4 e, FloatVector4 specular,
    float ao, FloatVector4 f0, const Fragment& z,
    float nDotL, float nDotV, float r, int s_i) const
{
  e *= m.s.envMapScale;
  // geometry function
  float   kEnv = r * r * 0.5f;
  FloatVector4  g((ao * nDotV) / (nDotV + kEnv - (nDotV * kEnv)));
  g *= envColor;
  e *= g;
  float   a = c[3];
  c *= ((lightColor * std::max(nDotL, 0.0f)) + (ambientLight * g));
  // Fresnel (coefficients are optimized for glass) with roughness
  const float *p = &(fresnelRoughTable[0]) + (s_i << 2);
  float   f = FloatVector4::polynomial3(p, nDotV);
  c += ((e - c) * (f0 + ((FloatVector4(1.0f) - f0) * (f * f))));
#if 0
  c += (specular * FloatVector4(m.s.specularColor[3]) * lightColor);
#else
  // ignore specular color/scale/smoothness set in material file
  c += (specular * lightColor);
#endif
  if (BRANCH_UNLIKELY(alphaBlendingEnabled()))
    c = alphaBlend(c, a, z);
  if (BRANCH_UNLIKELY(glowEnabled()))
    c += glowMap(z);
  return (c * lightColor[3]).srgbCompress();
}

inline FloatVector4 Plot3D_TriShape::calculateLighting_FO4(
    FloatVector4 c, FloatVector4 e, float specular, float specLevel,
    const Fragment& z, float nDotL, float nDotV, float smoothness) const
{
  e *= (m.s.envMapScale * specLevel);
  // geometry function
  float   r = 1.0f - std::min(smoothness, 0.999f);
  float   kEnv = r * r * 0.5f;
  FloatVector4  g(nDotV / (nDotV + kEnv - (nDotV * kEnv)));
  g *= envColor;
  float   a = c[3];
  c *= ((lightColor * std::max(nDotL, 0.0f)) + (ambientLight * g));
  c += ((e * g) + (m.s.specularColor * lightColor
                   * (specular * specLevel * m.s.specularColor[3])));
  if (BRANCH_UNLIKELY(alphaBlendingEnabled()))
    c = alphaBlend(c, a, z);
  if (BRANCH_UNLIKELY(glowEnabled()))
    c += glowMap(z);
  return (c * (lightColor[3] * 255.0f));
}

inline FloatVector4 Plot3D_TriShape::calculateLighting_TES5(
    FloatVector4 c, FloatVector4 e, float specular, float envLevel,
    float specLevel, const Fragment& z, float nDotL, float nDotV) const
{
  float   g = std::max(nDotL + 0.125f, 0.0f) * (1.0f / 1.125f);
  g = g / (g * 0.875f + 0.125f);
  // geometry function for ambient light only
  FloatVector4  a(nDotV / (nDotV * 0.875f + 0.125f));
  a *= envColor;
  FloatVector4  s(m.s.specularColor * lightColor);
  e *= (a * (m.s.envMapScale * envLevel));
  s *= (specular * specLevel * m.s.specularColor[3] * g);
  FloatVector4  glow(0.0f);
  if (BRANCH_UNLIKELY(glowEnabled()))
    glow = glowMap(z) * c * 2.0f;
  float   alpha = c[3];
  c *= ((lightColor * std::max(nDotL, 0.0f)) + (ambientLight * a));
  c += (e + s);
  if (BRANCH_UNLIKELY(alphaBlendingEnabled()))
    c = alphaBlend(c, alpha, z);
  return ((c + glow) * (lightColor[3] * 255.0f));
}

inline FloatVector4 Plot3D_TriShape::calculateLighting_FO76(
    FloatVector4 c, const Fragment& z) const
{
  float   nDotL = z.normal.dotProduct3(lightVector);
  float   nDotV = float(std::fabs(z.normal[2]));
  float   l = std::max(nDotL, 0.0f);
  // assume roughness = 1.0, f0 = 0.04, specular = 0.25
  FloatVector4  e(ambientLight);
#if 0
  FloatVector4  s(m.s.specularColor[3] * 0.5625f);
#else
  // ignore specular color/scale/smoothness set in material file
  FloatVector4  s(0.5625f);
#endif
  s *= lightColor;
  // geometry function (kEnv = kSpec = 0.5), multiplied with ao = 8.0 / 9.0
  FloatVector4  g(nDotV / (nDotV * 0.5625f + 0.5625f));
  e *= envColor;
  e *= g;
  float   a = c[3];
  c *= (lightColor * l + e);
  e *= m.s.envMapScale;
  s *= (g * (l / (l + 1.0f)));
  if (BRANCH_UNLIKELY(alphaBlendingEnabled()))
    c = alphaBlend(c, a, z);
  FloatVector4  f((nDotV * 0.05959029f - 0.11906419f) * nDotV + 0.10030248f);
  c = c + ((e + s - c) * f);
  if (BRANCH_UNLIKELY(glowEnabled()))
    c += glowMap(z);
  return (c * lightColor[3]).srgbCompress();
}

inline FloatVector4 Plot3D_TriShape::calculateLighting_FO4(
    FloatVector4 c, const Fragment& z) const
{
  float   nDotL = z.normal.dotProduct3(lightVector);
  float   nDotV = float(std::fabs(z.normal[2]));
  // geometry function for ambient light only
  FloatVector4  g(nDotV / (nDotV * 0.875f + 0.125f));
  g *= envColor;
  float   a = c[3];
  c *= ((lightColor * std::max(nDotL, 0.0f)) + (ambientLight * g));
  if (BRANCH_UNLIKELY(alphaBlendingEnabled()))
    c = alphaBlend(c, a, z);
  if (BRANCH_UNLIKELY(glowEnabled()))
    c += glowMap(z);
  return (c * (lightColor[3] * 255.0f));
}

inline FloatVector4 Plot3D_TriShape::calculateLighting_TES5(
    FloatVector4 c, const Fragment& z) const
{
  float   nDotL = z.normal.dotProduct3(lightVector);
  float   nDotV = float(std::fabs(z.normal[2]));
  // geometry function for ambient light only
  FloatVector4  g(nDotV / (nDotV * 0.875f + 0.125f));
  g *= envColor;
  FloatVector4  glow(0.0f);
  if (BRANCH_UNLIKELY(glowEnabled()))
    glow = glowMap(z) * c * 2.0f;
  float   a = c[3];
  c *= ((lightColor * std::max(nDotL, 0.0f)) + (ambientLight * g));
  if (BRANCH_UNLIKELY(alphaBlendingEnabled()))
    c = alphaBlend(c, a, z);
  return ((c + glow) * (lightColor[3] * 255.0f));
}

inline FloatVector4 Plot3D_TriShape::Fragment::normalMap(FloatVector4 n)
{
  n -= 127.5f;
  // calculate Z normal
  float   z = 16256.25f - std::min(n.dotProduct2(n), 16256.25f);    // 127.5^2
  z = float(std::sqrt(z));
  FloatVector4  tmpN(FloatVector4::normalize3x3Fast(
                         bitangent, tangent, normal));
  n *= tmpN;
  tmpN = (bitangent * n[0]) + (tangent * n[1]) + (normal * (z * tmpN[2]));
  tmpN.clearV3();
  // normalize and store result
  tmpN.normalize(invNormals);
  normal = tmpN;
  return tmpN;
}

inline FloatVector4 Plot3D_TriShape::calculateReflection(const Vertex& v) const
{
  FloatVector4  n(v.normal);
  // view vector for specular reflection only
  FloatVector4  viewVector((v.xyz + envMapOffs) * -(n[2]));
  viewVector[2] = envMapOffs[2];
  viewVector.normalize();
#if 0
  vDotL = -(viewVector.dotProduct3(lightVector));
#endif
  // reflect (I - 2.0 * dot(N, I) * N)
  return (viewVector - (n * (2.0f * viewVector.dotProduct3(n))));
}

inline FloatVector4 Plot3D_TriShape::environmentMap(
    FloatVector4 reflectedView, float smoothness, bool isSRGB, bool invZ) const
{
  if (BRANCH_UNLIKELY(!textureE))
    return ambientLight;
  // inverse rotation by view matrix
  FloatVector4  v((viewTransformInvX * reflectedView[0])
                  + (viewTransformInvY * reflectedView[1])
                  + (viewTransformInvZ * reflectedView[2]));
  float   mipLevel = (1.0f - smoothness) * float(textureE->getMaxMipLevel());
  FloatVector4  e(textureE->cubeMap(v[0], v[1], (!invZ ? v[2] : -(v[2])),
                                    mipLevel));
  if (!isSRGB)
    return (e * (1.0f / 255.0f));
  return e.srgbExpand();
}

inline float Plot3D_TriShape::specularPhong(
    FloatVector4 reflectedView, float smoothness, float nDotL, float nDotV,
    const float *fresnelPoly) const
{
  if (!(nDotL > (!fresnelPoly ? -0.125f : 0.0f)))
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
  if (fresnelPoly)
  {
    s = s * (g * 0.154375f + 0.095625f);        // 0.25 at smoothness = 0.0
    // geometry function
    float   k = (2.0f - smoothness) * (2.0f - smoothness) * 0.125f;
    s /= ((nDotL * (1.0f - k) + k) * (nDotV * (1.0f - k) + k));
    s *= (nDotL * nDotV);
    // polynomial not squared here because of gamma compressed color space
    s = s * FloatVector4::polynomial3(fresnelPoly, vDotH);
  }
  return s;
}

inline FloatVector4 Plot3D_TriShape::specularGGX(
    FloatVector4 reflectedView, float roughness, float nDotL, float nDotV,
    const float *fresnelPoly, FloatVector4 f0) const
{
  if (!(nDotL > 0.0f))
    return FloatVector4(0.0f);
  // geometry function
  float   k = (roughness + 1.0f) * (roughness + 1.0f) * 0.125f;
  float   g = (nDotL * (1.0f - k) + k) * (nDotV * (1.0f - k) + k);
  // h2 = 2.0 * squared dot product with halfway vector
  float   h2 = reflectedView.dotProduct3(lightVector) + 1.0f;
  float   a = roughness * roughness;
  float   d = h2 * (a * a - 1.0f) + 2.0f;
  d = (a * a * (nDotL * nDotV)) / (d * d * g);
  FloatVector4  f(FloatVector4::polynomial3(fresnelPoly, vDotH));
  f = f0 + ((FloatVector4(1.0f) - f0) * (f * f));
  return (f * d);
}

inline FloatVector4 Plot3D_TriShape::calculateLighting_Water(
    FloatVector4 c0, float a, Fragment& z, bool isFO76) const
{
  float   nDotL, nDotV;
  FloatVector4  n(z.normalMap(textureN->getPixelT(z.u(), z.v(), z.mipLevel)));
  FloatVector4  reflectedView(calculateReflection(z));
  FloatVector4  e(environmentMap(reflectedView, 1.0f,
                                 isFO76, (!isFO76 && renderMode < 8)));
  nDotL = n.dotProduct3(lightVector);
  nDotV = float(std::fabs(n[2]));
  FloatVector4  specular;
  if (!isFO76)
  {
    specular = FloatVector4(specularPhong(reflectedView, 0.875f, nDotL, nDotV,
                                          fresnelPoly3_Water));
  }
  else
  {
    specular = specularGGX(reflectedView, 0.125f, nDotL, nDotV,
                           fresnelPoly3_Water, FloatVector4(0.0f));
  }
  FloatVector4  c(m.w.deepColor);
  if (!isFO76)
  {
    c += ((m.w.shallowColor - c)
          * std::min(std::max(a * 2.285714f - 0.1428571f, 0.0f), 1.0f));
  }
  c *= ((lightColor * std::max(nDotL, 0.0f)) + (ambientLight * envColor));
  c *= lightColor[3];
  if (!isFO76)
    c += ((c0 * (1.0f / 255.0f) - c) * a);
  else
    c += ((c0.srgbExpand() - c) * a * (FloatVector4(1.0f) - m.w.shallowColor));
  e *= envColor;
  specular *= lightColor;
  e *= (m.w.envMapScale * lightColor[3]);
  specular *= (m.w.envMapScale * lightColor[3]);
  // Fresnel (water)
  float   f = FloatVector4::polynomial3(fresnelPoly3_Water, nDotV);
  if (!isFO76)
  {
    // polynomial not squared here because of gamma compressed color space
    return ((c + ((e - c) * f) + specular) * 255.0f);
  }
  return (c + ((e - c) * (f * f)) + specular).srgbCompress();
}

void Plot3D_TriShape::drawPixel_Water(Plot3D_TriShape& p, Fragment& z)
{
  float   a = *(z.zPtr) - z.xyz[2];     // water depth in pixels
  *(z.zPtr) = z.xyz[2];
  FloatVector4  c0(FloatVector4::convertRGBA32(*(z.cPtr)));
  a = a * p.m.w.unused;         // -3.0 / maximum depth in pixels
  if (a > -3.0f)
    a = FloatVector4::exp2Fast(a - 1.0f);
  else
    a = 0.0625f;
  FloatVector4  c(p.calculateLighting_Water(c0, a, z, false));
  *(z.cPtr) = c.convertToRGBA32(true);
}

void Plot3D_TriShape::drawPixel_Effect(Plot3D_TriShape& p, Fragment& z)
{
  FloatVector4  c;
  if (BRANCH_UNLIKELY(!getDiffuseColor_Effect(p, c, z)))
    return;
  *(z.zPtr) = z.xyz[2];
  float   nDotL = z.normal.dotProduct3(p.lightVector);
  float   nDotV = float(std::fabs(z.normal[2]));
  FloatVector4  reflectedView(p.calculateReflection(z));
  FloatVector4  e(0.0f);
  if (p.m.e.envMapScale > 0.0f)
  {
    e = p.environmentMap(reflectedView, 1.0f, false, (p.renderMode < 8));
    e *= p.m.e.envMapScale;
    if (p.textureS)
    {
      e *= (p.textureS->getPixelT(z.u(), z.v(), z.mipLevel + p.mipOffsetS)[0]
            * (1.0f / 255.0f));
    }
  }
  // geometry function for ambient light only
  FloatVector4  g(nDotV / (nDotV * 0.875f + 0.125f));
  g *= p.envColor;
  e *= g;
  float   a = c[3];
  if (p.m.flags & BGSMFile::Flag_EffectLighting)
  {
    c += ((c * ((p.lightColor * 0.5f) + (p.ambientLight * g)) - c)
          * p.m.e.lightingInfluence);
  }
  else
  {
    c *= ((p.lightColor * std::max(nDotL, 0.0f)) + (p.ambientLight * g));
  }
  c += e;
  if (p.alphaBlendingEnabled())
    c = p.alphaBlend(c, a, z);
  c *= (p.lightColor[3] * 255.0f);
  *(z.cPtr) = c.convertToRGBA32(true);
}

void Plot3D_TriShape::drawPixel_Water_G(Plot3D_TriShape& p, Fragment& z)
{
  float   a = *(z.zPtr) - z.xyz[2];     // water depth in pixels
  *(z.zPtr) = z.xyz[2];
  FloatVector4  c0(FloatVector4::convertRGBA32(*(z.cPtr)));
  a = a * p.m.w.unused;         // -3.0 / maximum depth in pixels
  if (a > -3.0f)
    a = FloatVector4::exp2Fast(a - 0.75f);
  else
    a = 0.07432544f;
  FloatVector4  c(p.calculateLighting_Water(c0, a, z, true));
  *(z.cPtr) = c.convertToRGBA32(true, true);
}

void Plot3D_TriShape::drawPixel_Effect_G(Plot3D_TriShape& p, Fragment& z)
{
  FloatVector4  c;
  if (BRANCH_UNLIKELY(!getDiffuseColor_Effect(p, c, z)))
    return;
  *(z.zPtr) = z.xyz[2];
  float   txtU = z.u();
  float   txtV = z.v();
  float   smoothness = 255.0f;
  float   ao = 1.0f;
  if (p.textureS)
  {
    FloatVector4  tmp(p.textureS->getPixelT(txtU, txtV,
                                            z.mipLevel + p.mipOffsetS));
    smoothness = tmp[0];
    ao = tmp[1] * (1.0f / 255.0f);
  }
  smoothness *= p.m.e.specularSmoothness;
  int     s_i = roundFloat(smoothness) & 0xFF;
  smoothness = smoothness * (1.0f / 255.0f);
  float   roughness = std::max(1.0f - smoothness, 0.03125f);
  float   nDotL = z.normal.dotProduct3(p.lightVector);
  float   nDotV = float(std::fabs(z.normal[2]));
  FloatVector4  reflectedView(p.calculateReflection(z));
  FloatVector4  e(0.0f);
  if (p.m.e.envMapScale > 0.0f)
  {
    e = p.environmentMap(reflectedView, smoothness, true, false);
    e *= p.m.e.envMapScale;
  }
  // geometry function
  float   kEnv = roughness * roughness * 0.5f;
  FloatVector4  g((ao * nDotV) / (nDotV + kEnv - (nDotV * kEnv)));
  g *= p.envColor;
  e *= g;
  float   a = c[3];
  if (p.m.flags & BGSMFile::Flag_EffectLighting)
  {
    c += ((c * ((p.lightColor * 0.5f) + (p.ambientLight * g)) - c)
          * p.m.e.lightingInfluence);
  }
  else
  {
    c *= ((p.lightColor * std::max(nDotL, 0.0f)) + (p.ambientLight * g));
  }
  if (!p.textureS)
  {
    c += e;
    if (p.alphaBlendingEnabled())
      c = p.alphaBlend(c, a, z);
  }
  else
  {
    // enable PBR mode if lighting texture is present
    if (BRANCH_LIKELY(p.alphaBlendingEnabled()))
      c = p.alphaBlend(c, a, z);
    FloatVector4  f0(p.textureR->getPixelT(txtU, txtV,
                                           z.mipLevel + p.mipOffsetR));
    f0.srgbExpand();
    // Fresnel (coefficients are optimized for glass) with roughness
    float   f =
        FloatVector4::polynomial3(&(fresnelRoughTable[0]) + (s_i << 2), nDotV);
    c += ((e - c) * (f0 + ((FloatVector4(1.0f) - f0) * (f * f))));
    if (BRANCH_LIKELY(nDotL > 0.0f))
    {
      c += (p.specularGGX(reflectedView, roughness, nDotL, nDotV,
                          fresnelPoly3N_Glass, f0) * p.lightColor);
    }
  }
  c = (c * p.lightColor[3]).srgbCompress();
  *(z.cPtr) = c.convertToRGBA32(true, true);
}

FloatVector4 Plot3D_TriShape::glowMap(const Fragment& z) const
{
  FloatVector4  c(m.s.emissiveColor);
  c *= m.s.emissiveColor[3];
  if (BRANCH_LIKELY(textureGlow))
  {
    FloatVector4  tmp(textureGlow->getPixelT_Inline(z.u(), z.v(),
                                                    z.mipLevel + mipOffsetG));
    if (usingSRGBColorSpace)
      tmp *= (1.0f / 255.0f);
    else
      tmp.srgbExpand();
    c *= tmp;
  }
  return c;
}

bool Plot3D_TriShape::getDiffuseColor_Effect(
    const Plot3D_TriShape& p, FloatVector4& c, Fragment& z)
{
  if (BRANCH_LIKELY(!p.debugMode))
  {
    if (p.textureN)
    {
      (void) z.normalMap(p.textureN->getPixelT_Inline(
                             z.u(), z.v(), z.mipLevel + p.mipOffsetN));
    }
    else if (z.invNormals)
    {
      z.normal *= -1.0f;
    }
  }
  FloatVector4  tmp(p.textureD->getPixelT_Inline(z.u(), z.v(), z.mipLevel));
  FloatVector4  baseColor(p.m.e.baseColor);
  FloatVector4  vColor(z.vertexColor);
  float   a = (tmp * baseColor)[3];
  float   f = 1.0f;
  if (p.m.flags & BGSMFile::Flag_FalloffEnabled)
  {
    float   d = p.m.e.falloffParams[1] - p.m.e.falloffParams[0];
    if (BRANCH_UNLIKELY(!((d * d) >= (1.0f / float(0x10000000)))))
    {
      f = 0.5f;
    }
    else
    {
      float   nDotV = float(std::fabs(z.normal[2]));
      f = (nDotV - p.m.e.falloffParams[0]) / d;
      f = std::max(std::min(f, 1.0f), 0.0f);
    }
    f = (p.m.e.falloffParams[2] * (1.0f - f)) + (p.m.e.falloffParams[3] * f);
  }
  if (p.textureG)
  {
    tmp *= (1.0f / 255.0f);
    vColor *= f;
    baseColor *= vColor;
    if (p.m.flags & BGSMFile::Flag_GrayscaleToAlpha)
      a = p.textureG->getPixelBC_Inline(tmp[3], baseColor[3], 0)[3];
    else
      a = a * vColor[3];
    if (a < p.m.alphaThresholdFloat)
      return false;
    tmp = p.textureG->getPixelBC_Inline(tmp[1], baseColor[0], 0);
    if (p.usingSRGBColorSpace)
      tmp *= (1.0f / 255.0f);
    else
      tmp.srgbExpand();
  }
  else
  {
    a = a * vColor[3] * f;
    if (a < p.m.alphaThresholdFloat)
      return false;
    baseColor *= p.m.e.baseColorScale;
    tmp *= vColor;
    if (p.usingSRGBColorSpace)
      tmp *= (1.0f / 255.0f);
    else
      tmp.srgbExpand();
    tmp *= baseColor;
  }
  tmp[3] = a;                   // alpha * 255
  c = tmp;
  if (BRANCH_UNLIKELY(p.m.flags & BGSMFile::Flag_NoZWrite))
    z.zPtr = &(z.xyz[2]);
  return true;
}

bool Plot3D_TriShape::getDiffuseColor_sRGB_G(
    const Plot3D_TriShape& p, FloatVector4& c, Fragment& z)
{
  FloatVector4  tmp(p.textureD->getPixelT_Inline(z.u(), z.v(), z.mipLevel));
  FloatVector4  vColor(z.vertexColor);
  float   a = (tmp * vColor)[3];
  if (BRANCH_UNLIKELY(a < p.m.alphaThresholdFloat))
    return false;
  float   u = tmp[1] * (1.0f / 255.0f);
  float   v = p.m.s.gradientMapV * vColor[0];
  tmp = p.textureG->getPixelBC_Inline(u, v, 0);
  tmp *= (1.0f / 255.0f);
  tmp[3] = a;                   // alpha * 255
  c = tmp;
  return true;
}

bool Plot3D_TriShape::getDiffuseColor_sRGB(
    const Plot3D_TriShape& p, FloatVector4& c, Fragment& z)
{
  FloatVector4  tmp(p.textureD->getPixelT_Inline(z.u(), z.v(), z.mipLevel));
  FloatVector4  vColor(z.vertexColor);
  tmp *= vColor;
  float   a = tmp[3];
  if (BRANCH_UNLIKELY(a < p.m.alphaThresholdFloat))
    return false;
  tmp *= (1.0f / 255.0f);
  tmp[3] = a;                   // alpha * 255
  c = tmp;
  return true;
}

bool Plot3D_TriShape::getDiffuseColor_Linear_G(
    const Plot3D_TriShape& p, FloatVector4& c, Fragment& z)
{
  FloatVector4  tmp(p.textureD->getPixelT_Inline(z.u(), z.v(), z.mipLevel));
  FloatVector4  vColor(z.vertexColor);
  float   a = (tmp * vColor)[3];
  if (BRANCH_UNLIKELY(a < p.m.alphaThresholdFloat))
    return false;
  float   u = tmp[1] * (1.0f / 255.0f);
  float   v = p.m.s.gradientMapV * vColor[0];
  tmp = p.textureG->getPixelBC_Inline(u, v, 0).srgbExpand();
  tmp[3] = a;                   // alpha * 255
  c = tmp;
  return true;
}

bool Plot3D_TriShape::getDiffuseColor_Linear(
    const Plot3D_TriShape& p, FloatVector4& c, Fragment& z)
{
  FloatVector4  tmp(p.textureD->getPixelT_Inline(z.u(), z.v(), z.mipLevel));
  tmp *= z.vertexColor;
  float   a = tmp[3];
  if (BRANCH_UNLIKELY(a < p.m.alphaThresholdFloat))
    return false;
  tmp.srgbExpand();
  tmp[3] = a;                   // alpha * 255
  c = tmp;
  return true;
}

void Plot3D_TriShape::drawPixel_Debug(Plot3D_TriShape& p, Fragment& z)
{
  FloatVector4  c;
  if (BRANCH_UNLIKELY(!p.getDiffuseColor(c, z)))
    return;
  if (p.debugMode == 5)
    c = FloatVector4(1.0f);
  *(z.zPtr) = z.xyz[2];
  if (!(p.debugMode == 3 || p.debugMode == 5))
  {
    std::uint32_t tmp = std::uint32_t(p.debugMode);
    if (p.debugMode != 4)
    {
#if USE_PIXELFMT_RGB10A2
      if (!(tmp & 0x80000000U))
      {
        tmp = std::uint32_t(roundFloat(z.xyz[2] * 64.0f));
        tmp = tmp | (tmp < 0x3FFFFFFFU ? 0xC0000000U : 0xFFFFFFFFU);
      }
      else
      {
        tmp = ((tmp & 0x00FFU) << 2) | ((tmp & 0xFF00U) << 4)
              | ((tmp & 0x00FF0000U) << 6) | 0xC0100401U;
      }
#else
      if (!(tmp & 0x80000000U))
      {
        tmp = std::uint32_t(roundFloat(z.xyz[2] * 16.0f));
        tmp = (tmp < 0x00FFFFFFU ? tmp : 0x00FFFFFFU);
      }
      tmp = ((tmp & 0x00FFU) << 16) | ((tmp >> 16) & 0x00FFU)
            | (tmp & 0xFF00U) | 0xFF000000U;
#endif
    }
    else if (p.renderMode >= 12)
    {
      tmp = c.srgbCompress().convertToRGBA32(true, true);
    }
    else
    {
      tmp = (c * 255.0f).convertToRGBA32(true);
    }
    *(z.cPtr) = tmp;
    return;
  }
  FloatVector4  n(p.textureN->getPixelT(z.u(), z.v(),
                                        z.mipLevel + p.mipOffsetN));
  n = z.normalMap(n);
  if (p.debugMode == 3)
  {
    c = n * FloatVector4(127.5f, -127.5f, -127.5f, 0.0f);
    c += 127.5f;
  }
  else
  {
    if (p.renderMode >= 12)
      c = p.calculateLighting_FO76(c, z);
    else if (p.renderMode >= 8)
      c = p.calculateLighting_FO4(c, z);
    else
      c = p.calculateLighting_TES5(c, z);
  }
  *(z.cPtr) = c.convertToRGBA32(true);
}

void Plot3D_TriShape::drawPixel_D_TES5(Plot3D_TriShape& p, Fragment& z)
{
  // diffuse texture only
  FloatVector4  c;
  if (BRANCH_UNLIKELY(!p.getDiffuseColor(c, z)))
    return;
  *(z.zPtr) = z.xyz[2];
  z.normal.normalize(z.invNormals);
  c = p.calculateLighting_TES5(c, z);
  *(z.cPtr) = c.convertToRGBA32(true);
  p.storeNormal(z.normal, z.cPtr);
}

void Plot3D_TriShape::drawPixel_N_TES5(Plot3D_TriShape& p, Fragment& z)
{
  // normal map only
  FloatVector4  c;
  if (BRANCH_UNLIKELY(!p.getDiffuseColor(c, z)))
    return;
  *(z.zPtr) = z.xyz[2];
  FloatVector4  n(p.textureN->getPixelT(z.u(), z.v(),
                                        z.mipLevel + p.mipOffsetN));
  (void) z.normalMap(n);
  c = p.calculateLighting_TES5(c, z);
  *(z.cPtr) = c.convertToRGBA32(true);
  p.storeNormal(z.normal, z.cPtr);
}

void Plot3D_TriShape::drawPixel_TES5(Plot3D_TriShape& p, Fragment& z)
{
  FloatVector4  c;
  if (BRANCH_UNLIKELY(!p.getDiffuseColor(c, z)))
    return;
  *(z.zPtr) = z.xyz[2];
  float   txtU = z.u();
  float   txtV = z.v();
  FloatVector4  n(p.textureN->getPixelT(txtU, txtV, z.mipLevel + p.mipOffsetN));
  float   specLevel = n[3] * (1.0f / 255.0f);
  float   smoothness = std::min(p.m.s.specularSmoothness, 1.0f);
  float   nDotL = z.normalMap(n).dotProduct3(p.lightVector);
  float   nDotV = float(std::fabs(z.normal[2]));
  FloatVector4  reflectedView(p.calculateReflection(z));
  FloatVector4  e(p.environmentMap(reflectedView, smoothness, false, true));
  float   envLevel = specLevel;
  if (p.textureS)
  {
    envLevel = p.textureS->getPixelT(txtU, txtV, z.mipLevel + p.mipOffsetS)[0];
    envLevel *= (1.0f / 255.0f);
  }
  float   specular = p.specularPhong(reflectedView, smoothness, nDotL, nDotV);
  c = p.calculateLighting_TES5(c, e, specular, envLevel,
                               specLevel, z, nDotL, nDotV);
  *(z.cPtr) = c.convertToRGBA32(true);
  p.storeNormal(z.normal, z.cPtr);
}

void Plot3D_TriShape::drawPixel_D_FO4(Plot3D_TriShape& p, Fragment& z)
{
  // diffuse texture only
  FloatVector4  c;
  if (BRANCH_UNLIKELY(!p.getDiffuseColor(c, z)))
    return;
  *(z.zPtr) = z.xyz[2];
  z.normal.normalize(z.invNormals);
  c = p.calculateLighting_FO4(c, z);
  *(z.cPtr) = c.convertToRGBA32(true);
  p.storeNormal(z.normal, z.cPtr);
}

void Plot3D_TriShape::drawPixel_N_FO4(Plot3D_TriShape& p, Fragment& z)
{
  // normal map only
  FloatVector4  c;
  if (BRANCH_UNLIKELY(!p.getDiffuseColor(c, z)))
    return;
  *(z.zPtr) = z.xyz[2];
  FloatVector4  n(p.textureN->getPixelT(z.u(), z.v(),
                                        z.mipLevel + p.mipOffsetN));
  (void) z.normalMap(n);
  c = p.calculateLighting_FO4(c, z);
  *(z.cPtr) = c.convertToRGBA32(true);
  p.storeNormal(z.normal, z.cPtr);
}

void Plot3D_TriShape::drawPixel_FO4(Plot3D_TriShape& p, Fragment& z)
{
  FloatVector4  c;
  if (BRANCH_UNLIKELY(!p.getDiffuseColor(c, z)))
    return;
  *(z.zPtr) = z.xyz[2];
  FloatVector4  n(p.textureN->getPixelT_2(
                      z.u(), z.v(), z.mipLevel + p.mipOffsetN, *(p.textureS)));
  float   specLevel = n[2] * (1.0f / 255.0f);
  float   smoothness =
      std::min(p.m.s.specularSmoothness * n[3] * (1.0f / 255.0f), 1.0f);
  float   nDotL = z.normalMap(n).dotProduct3(p.lightVector);
  float   nDotV = float(std::fabs(z.normal[2]));
  FloatVector4  reflectedView(p.calculateReflection(z));
  FloatVector4  e(p.environmentMap(reflectedView, smoothness, false, false));
  float   specular = p.specularPhong(reflectedView, smoothness, nDotL, nDotV,
                                     fresnelPoly3_FO4);
  c = p.calculateLighting_FO4(c, e, specular, specLevel,
                              z, nDotL, nDotV, smoothness);
  *(z.cPtr) = c.convertToRGBA32(true);
  p.storeNormal(z.normal, z.cPtr);
}

void Plot3D_TriShape::drawPixel_D_FO76(Plot3D_TriShape& p, Fragment& z)
{
  // diffuse texture only
  FloatVector4  c;
  if (BRANCH_UNLIKELY(!p.getDiffuseColor(c, z)))
    return;
  *(z.zPtr) = z.xyz[2];
  z.normal.normalize(z.invNormals);
  c = p.calculateLighting_FO76(c, z);
  *(z.cPtr) = c.convertToRGBA32(true, true);
  p.storeNormal(z.normal, z.cPtr);
}

void Plot3D_TriShape::drawPixel_N_FO76(Plot3D_TriShape& p, Fragment& z)
{
  // normal map only, fixed roughness = 1.0, ao = 8.0 / 9.0, reflectance = 0.04
  FloatVector4  c;
  if (BRANCH_UNLIKELY(!p.getDiffuseColor(c, z)))
    return;
  *(z.zPtr) = z.xyz[2];
  FloatVector4  n(p.textureN->getPixelT(z.u(), z.v(),
                                        z.mipLevel + p.mipOffsetN));
  float   nDotL = z.normalMap(n).dotProduct3(p.lightVector);
  float   nDotV = float(std::fabs(z.normal[2]));
  FloatVector4  reflectedView(p.calculateReflection(z));
  FloatVector4  f0(0.04f);
  FloatVector4  specular(p.specularGGX(reflectedView, 1.0f, nDotL, nDotV,
                                       fresnelPoly3N_Glass, f0));
  c = p.calculateLighting_FO76(c, p.ambientLight, specular, 1.0f / 1.125f, f0,
                               z, nDotL, nDotV, 1.0f, 0);
  *(z.cPtr) = c.convertToRGBA32(true, true);
  p.storeNormal(z.normal, z.cPtr);
}

void Plot3D_TriShape::drawPixel_FO76(Plot3D_TriShape& p, Fragment& z)
{
  FloatVector4  c;
  if (BRANCH_UNLIKELY(!p.getDiffuseColor(c, z)))
    return;
  *(z.zPtr) = z.xyz[2];
  float   txtU = z.u();
  float   txtV = z.v();
  FloatVector4  n, f0;
  n = p.textureN->getPixelT_2(txtU, txtV, z.mipLevel + p.mipOffsetN,
                              *(p.textureS));
  f0 = p.textureR->getPixelT(txtU, txtV, z.mipLevel + p.mipOffsetR);
  f0.srgbExpand();
  f0.maxValues(FloatVector4(0.015625f));
#if 0
  float   smoothness = std::min(p.m.s.specularSmoothness * n[2], 255.0f);
#else
  // ignore specular color/scale/smoothness set in material file
  float   smoothness = n[2];
#endif
  float   ao = n[3] * (1.0f / 255.0f);
  int     s_i = roundFloat(smoothness) & 0xFF;
  smoothness = smoothness * (1.0f / 255.0f);
  float   roughness = std::max(1.0f - smoothness, 0.03125f);
  float   nDotL = z.normalMap(n).dotProduct3(p.lightVector);
  float   nDotV = float(std::fabs(z.normal[2]));
  FloatVector4  reflectedView(p.calculateReflection(z));
  FloatVector4  e(p.environmentMap(reflectedView, smoothness, true, false));
  FloatVector4  specular(p.specularGGX(reflectedView, roughness, nDotL, nDotV,
                                       fresnelPoly3N_Glass, f0));
  c = p.calculateLighting_FO76(c, e, specular, ao, f0,
                               z, nDotL, nDotV, roughness, s_i);
  *(z.cPtr) = c.convertToRGBA32(true, true);
  p.storeNormal(z.normal, z.cPtr);
}

inline void Plot3D_TriShape::drawPixel(
    int x, int y, Fragment& v,
    const Vertex& v0, const Vertex& v1, const Vertex& v2,
    float w0f, float w1f, float w2f)
{
  FloatVector4  w0(w0f);
  FloatVector4  w1(w1f);
  FloatVector4  w2(w2f);
  v.xyz = (v0.xyz * w0) + (v1.xyz * w1) + (v2.xyz * w2);
  if (BRANCH_UNLIKELY(v.xyz[2] < 0.0f))
    return;
  size_t  offs = size_t(y) * size_t(width) + size_t(x);
  if (!(v.xyz[2] < *(bufZ + offs)))
    return;
  v.zPtr = bufZ + offs;
  v.cPtr = bufRGBA + offs;
  v.bitangent = (v0.bitangent * w0) + (v1.bitangent * w1) + (v2.bitangent * w2);
  v.tangent = (v0.tangent * w0) + (v1.tangent * w1) + (v2.tangent * w2);
  v.normal = (v0.normal * w0) + (v1.normal * w1) + (v2.normal * w2);
  v.vertexColor =
      (v0.vertexColor * w0) + (v1.vertexColor * w1) + (v2.vertexColor * w2);
  drawPixelFunction(*this, v);
}

void Plot3D_TriShape::drawLine(Fragment& v, const Vertex& v0, const Vertex& v1)
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
      if (w1 > -0.0000005 && w1 < 1.0000005)
      {
        float   xf = (v1.xyz[0] - v0.xyz[0]) * float(w1) + v0.xyz[0];
        x = roundFloat(xf);
        if (float(std::fabs(xf - float(x))) < (1.0f / 1024.0f) &&
            (unsigned int) y < (unsigned int) height &&
            (unsigned int) x < (unsigned int) width)
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
  if ((unsigned int) y >= (unsigned int) height)
    return;
  if (float(std::fabs(v1.xyz[0] - v0.xyz[0])) >= (1.0f / 1024.0f))
  {
    double  dx1 = 1.0 / (double(v1.xyz[0]) - double(v0.xyz[0]));
    int     x1 = roundFloat(v1.xyz[0]);
    while (true)
    {
      double  w1 = (double(x) - double(v0.xyz[0])) * dx1;
      if (w1 > -0.0000005 && w1 < 1.0000005)
      {
        float   yf = (v1.xyz[1] - v0.xyz[1]) * float(w1) + v0.xyz[1];
        if (float(std::fabs(yf - float(y))) < (1.0f / 1024.0f) &&
            (unsigned int) x < (unsigned int) width)
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
           float(std::fabs(v0.xyz[1] - float(y))) < (1.0f / 1024.0f) &&
           (unsigned int) x < (unsigned int) width)
  {
    drawPixel(x, y, v, v0, v0, v0, 1.0f, 0.0f, 0.0f);
  }
}

void Plot3D_TriShape::drawTriangles()
{
  Fragment  v;
  for (size_t n = 0; n < triangleBuf.size(); n++)
  {
    v.mipLevel = 15.0f;
    const Vertex  *v0, *v1, *v2;
    {
      const NIFFile::NIFTriangle& t = triangleData[size_t(triangleBuf[n])];
      v0 = vertexBuf.data() + t.v0;
      v1 = vertexBuf.data() + t.v1;
      v2 = vertexBuf.data() + t.v2;
    }
    // rotate vertices so that v0 has the lowest Y coordinate
    if (v0->xyz[1] > v1->xyz[1] || v0->xyz[1] > v2->xyz[1])
    {
      if (v1->xyz[1] > v2->xyz[1])      // 2, 0, 1
      {
        const Vertex  *tmp = v2;
        v2 = v1;
        v1 = v0;
        v0 = tmp;
      }
      else                              // 1, 2, 0
      {
        const Vertex  *tmp = v0;
        v0 = v1;
        v1 = v2;
        v2 = tmp;
      }
    }
    double  x0 = double(v0->xyz[0]);
    double  y0 = double(v0->xyz[1]);
    double  x1 = double(v1->xyz[0]);
    double  y1 = double(v1->xyz[1]);
    double  x2 = double(v2->xyz[0]);
    double  y2 = double(v2->xyz[1]);
    double  xyArea2_d = ((x1 - x0) * (y2 - y0)) - ((x2 - x0) * (y1 - y0));
    v.invNormals = (xyArea2_d >= 0.0);
    float   xyArea2 = float(std::fabs(xyArea2_d));
    if (BRANCH_UNLIKELY(xyArea2 < (1.0f / 1048576.0f)))
    {
      // if area < 2^-21 square pixels
      drawLine(v, *v0, *v1);
      drawLine(v, *v1, *v2);
      drawLine(v, *v2, *v0);
      continue;
    }
    double  r2xArea = 1.0 / xyArea2_d;
    if (BRANCH_LIKELY(textureD))
    {
      float   uvArea2 =
          float(std::fabs(((v1->u() - v0->u()) * (v2->v() - v0->v()))
                          - ((v2->u() - v0->u()) * (v1->v() - v0->v()))));
      if (xyArea2 > uvArea2)
      {
        uvArea2 *= (float(textureD->getWidth()) * float(textureD->getHeight()));
        v.mipLevel = -6.0f;
        if (BRANCH_LIKELY((uvArea2 * 4096.0f) > xyArea2))
        {
          // calculate base 4 logarithm of texel area / pixel area
          v.mipLevel =
              FloatVector4::log2Fast(float(std::fabs(r2xArea * uvArea2)))
              * 0.5f;
          int     mipLevel_i = roundFloat(v.mipLevel);
          if (float(std::fabs(v.mipLevel - float(mipLevel_i))) < 0.0625f)
            v.mipLevel = float(mipLevel_i);
        }
      }
    }
    double  a1 = (y2 - y0) * r2xArea;
    double  b1 = (x0 - x2) * r2xArea;
    double  a2 = (y0 - y1) * r2xArea;
    double  b2 = (x1 - x0) * r2xArea;
    int     y = roundDouble(y0 + 0.4999999995);
    int     yMax = roundDouble((y1 > y2 ? y1 : y2) - 0.4999999995);
    y = (y > 0 ? y : 0);
    yMax = (yMax < (height - 1) ? yMax : (height - 1));
    if (y1 > y2)                        // from v0 -> v1 edge to v0 -> v2
    {
      double  c1 = (x1 - x0) / (y1 - y0);
      if (a2 < 0.0)                     // negative X direction
      {
        for ( ; y <= yMax; y++)
        {
          double  yf = double(y) - y0;
          int     x = roundDouble(yf * c1 + x0 - 0.4999999995);
          x = (x < (width - 1) ? x : (width - 1));
          double  w1 = ((double(x) - x0) * a1) + (yf * b1);
          double  w2 = ((double(x) - x0) * a2) + (yf * b2);
          for ( ; BRANCH_LIKELY(x >= 0); x--, w1 -= a1, w2 -= a2)
          {
            if (w1 < -0.0000005 || (w1 + w2) > 1.0000005)
              break;
            drawPixel(x, y, v, *v0, *v1, *v2,
                      float(1.0 - (w1 + w2)), float(w1), float(w2));
          }
        }
      }
      else                              // positive X direction
      {
        for ( ; y <= yMax; y++)
        {
          double  yf = double(y) - y0;
          int     x = roundDouble(yf * c1 + x0 + 0.4999999995);
          x = (x > 0 ? x : 0);
          double  w1 = ((double(x) - x0) * a1) + (yf * b1);
          double  w2 = ((double(x) - x0) * a2) + (yf * b2);
          for ( ; BRANCH_LIKELY(x < width); x++, w1 += a1, w2 += a2)
          {
            if (w1 < -0.0000005 || (w1 + w2) > 1.0000005)
              break;
            drawPixel(x, y, v, *v0, *v1, *v2,
                      float(1.0 - (w1 + w2)), float(w1), float(w2));
          }
        }
      }
    }
    else                                // from v0 -> v2 edge to v0 -> v1
    {
      double  c1 = (x2 - x0) / (y2 - y0);
      if (a1 < 0.0)                     // negative X direction
      {
        for ( ; y <= yMax; y++)
        {
          double  yf = double(y) - y0;
          int     x = roundDouble(yf * c1 + x0 - 0.4999999995);
          x = (x < (width - 1) ? x : (width - 1));
          double  w1 = ((double(x) - x0) * a1) + (yf * b1);
          double  w2 = ((double(x) - x0) * a2) + (yf * b2);
          for ( ; BRANCH_LIKELY(x >= 0); x--, w1 -= a1, w2 -= a2)
          {
            if (w2 < -0.0000005 || (w1 + w2) > 1.0000005)
              break;
            drawPixel(x, y, v, *v0, *v1, *v2,
                      float(1.0 - (w1 + w2)), float(w1), float(w2));
          }
        }
      }
      else                              // positive X direction
      {
        for ( ; y <= yMax; y++)
        {
          double  yf = double(y) - y0;
          int     x = roundDouble(yf * c1 + x0 + 0.4999999995);
          x = (x > 0 ? x : 0);
          double  w1 = ((double(x) - x0) * a1) + (yf * b1);
          double  w2 = ((double(x) - x0) * a2) + (yf * b2);
          for ( ; BRANCH_LIKELY(x < width); x++, w1 += a1, w2 += a2)
          {
            if (w2 < -0.0000005 || (w1 + w2) > 1.0000005)
              break;
            drawPixel(x, y, v, *v0, *v1, *v2,
                      float(1.0 - (w1 + w2)), float(w1), float(w2));
          }
        }
      }
    }
  }
}

Plot3D_TriShape::Plot3D_TriShape(
    std::uint32_t *outBufRGBA, float *outBufZ, int imageWidth, int imageHeight,
    unsigned int mode)
  : bufZ(outBufZ),
    bufRGBA(outBufRGBA),
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
    renderMode((unsigned char) (mode & 15U)),
    usingSRGBColorSpace(renderMode < 12),
    waterUVScale(32),
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
    drawPixelFunction(&drawPixel_Water),
    getDiffuseColorFunc(&getDiffuseColor_sRGB),
    halfwayVector(1.0f, 0.0f, 0.0f, 0.0f),
    vDotL(-1.0f),
    vDotH(0.001f),
    bufN((std::uint32_t *) 0)
{
}

Plot3D_TriShape::~Plot3D_TriShape()
{
}

void Plot3D_TriShape::setViewAndLightVector(
    const NIFFile::NIFVertexTransform& vt,
    float lightX, float lightY, float lightZ)
{
  viewTransform = vt;
  viewTransformInvX =
      FloatVector4(viewTransform.rotateXX, viewTransform.rotateXY,
                   viewTransform.rotateXZ, 0.0f);
  viewTransformInvY =
      FloatVector4(viewTransform.rotateYX, viewTransform.rotateYY,
                   viewTransform.rotateYZ, 0.0f);
  viewTransformInvZ =
      FloatVector4(viewTransform.rotateZX, viewTransform.rotateZY,
                   viewTransform.rotateZZ, 0.0f);
  lightVector = FloatVector4(lightX, lightY, lightZ, 0.0f);
  lightVector = vt.rotateXYZ(lightVector);
  lightVector.normalize();
  halfwayVector = lightVector - FloatVector4(0.0f, 0.0f, 1.0f, 0.0f);
  halfwayVector.normalize();
  if (halfwayVector.dotProduct3(halfwayVector) < 0.5f)
    halfwayVector = FloatVector4(1.0f, 0.0f, 0.0f, 0.0f);
  vDotL = -(lightVector[2]);
  float   tmp = std::min(std::max(vDotL * 0.5f + 0.5f, 0.000001f), 0.999999f);
  vDotH = float(std::sqrt(tmp));
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
  *(static_cast< NIFFile::NIFTriShape * >(this)) = t;
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
    const DDSTexture * const *textures, unsigned int textureMask)
{
  if (BRANCH_UNLIKELY(!((textureMask & 1U)
                        | (m.flags & BGSMFile::Flag_TSWater))))
  {
    return;                     // not water and no diffuse texture
  }
  size_t  triangleCntRendered = transformVertexData(modelTransform);
  if (!triangleCntRendered)
    return;
  if (BRANCH_UNLIKELY(m.flags & BGSMFile::Flag_IsEffect))
  {
    drawEffect(textures, textureMask);
    return;
  }
  if (BRANCH_LIKELY(!(textureMask & 0x0008)))
  {
    getDiffuseColorFunc = (!usingSRGBColorSpace ?
                           &getDiffuseColor_Linear : &getDiffuseColor_sRGB);
  }
  else
  {
    getDiffuseColorFunc = (!usingSRGBColorSpace ?
                           &getDiffuseColor_Linear_G : &getDiffuseColor_sRGB_G);
  }
  if (BRANCH_UNLIKELY(m.flags & BGSMFile::Flag_TSWater))
  {
    drawWater(textures, textureMask, viewTransform.scale);
    return;
  }
  textureD = textures[0];
  int     mode = (!debugMode ? int(renderMode) : 0);
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
      if (BRANCH_UNLIKELY(!(textureMask & 0x0040U)))
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
      if (BRANCH_UNLIKELY(!(textureMask & 0x0100U)))
        textureR = &defaultTexture_R;
      else
        textureR = textures[8];
      mipOffsetR = calculateMipOffset(textureR, textureD);
      if (BRANCH_UNLIKELY(!(textureMask & 0x0200U)))
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
  if (BRANCH_UNLIKELY(!(textureMask & 0x0002U)))
    textureN = &defaultTexture_N;
  else
    textureN = textures[1];
  mipOffsetN = calculateMipOffset(textureN, textureD);
  if (BRANCH_UNLIKELY(textureMask & 0x0004U))
  {
    if (m.flags & BGSMFile::Flag_Glow)
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

void Plot3D_TriShape::drawEffect(
    const DDSTexture * const *textures, unsigned int textureMask)
{
  if (BRANCH_UNLIKELY(!(textureMask & 0x0001)))
    return;
  getDiffuseColorFunc = &getDiffuseColor_Effect;
  textureD = textures[0];
  textureG = (DDSTexture *) 0;
  textureN = (DDSTexture *) 0;
  textureE = (DDSTexture *) 0;
  textureS = (DDSTexture *) 0;
  textureR = (DDSTexture *) 0;
  textureGlow = (DDSTexture *) 0;
  if (BRANCH_UNLIKELY(debugMode))
  {
    drawPixelFunction = &drawPixel_Debug;
    textureN = &defaultTexture_N;
  }
  else if (renderMode < 12)             // Skyrim or Fallout 4
  {
    drawPixelFunction = &drawPixel_Effect;
    if ((renderMode & 3) && (textureMask & 0x0020))
    {
      textureS = textures[5];
      mipOffsetS = calculateMipOffset(textureS, textureD);
    }
  }
  else                                  // Fallout 76
  {
    drawPixelFunction = &drawPixel_Effect_G;
    if ((renderMode & 3) && (textureMask & 0x0300))
    {
      textureS = &defaultTexture_L;
      textureR = &defaultTexture_R;
      if (textureMask & 0x0100)
        textureR = textures[8];
      if (textureMask & 0x0200)
        textureS = textures[9];
      mipOffsetR = calculateMipOffset(textureR, textureD);
      mipOffsetS = calculateMipOffset(textureS, textureD);
    }
  }
  if (textureMask & 0x0002)
    textureN = textures[1];
  if (textureN)
    mipOffsetN = calculateMipOffset(textureN, textureD);
  if (textureMask & 0x0008)
    textureG = textures[3];
  if (textureMask & 0x0010)
    textureE = textures[4];
  drawTriangles();
}

void Plot3D_TriShape::drawWater(
    const DDSTexture * const *textures, unsigned int textureMask,
    float viewScale)
{
  mipOffsetN = 0.0f;
  textureN = &defaultTexture_N;
  if (textureMask & 2U)
    textureN = textures[1];
  else if (textureMask & 1U)
    textureN = textures[0];
  textureD = textureN;
  // -3.0 / maximum depth in pixels
  m.w.unused = -3.0f / (m.w.maxDepth * viewScale);
  drawPixelFunction =
      (usingSRGBColorSpace ? &drawPixel_Water : &drawPixel_Water_G);
  textureE = (DDSTexture *) 0;
  if (textureMask & 16U)
    textureE = textures[4];
  if (BRANCH_UNLIKELY(debugMode))
  {
    m.s.specularColor = FloatVector4(1.0f, 1.0f, 1.0f, m.w.envMapScale);
    drawPixelFunction = &drawPixel_Debug;
  }
  drawTriangles();
}

FloatVector4 Plot3D_TriShape::cubeMapToAmbient(const DDSTexture *e) const
{
  if (!e)
    return FloatVector4(0.05f);
  FloatVector4  s(0.0f);
  int     w = 0;
  int     n = int(e->getTextureCount());
  int     mipLevel = e->getMaxMipLevel();
  int     txtW = e->getWidth() >> mipLevel;
  int     txtH = e->getHeight() >> mipLevel;
  bool    isSRGB = usingSRGBColorSpace;
  for (int i = 0; i < n; i++)
  {
    for (int y = 0; y < txtH; y++)
    {
      if (isSRGB)
      {
        for (int x = 0; x < txtW; x++, w++)
          s += FloatVector4(e->getPixelN(x, y, mipLevel, i));
      }
      else
      {
        for (int x = 0; x < txtW; x++, w++)
          s += FloatVector4(e->getPixelN(x, y, mipLevel, i)).srgbExpand();
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

void Plot3D_TriShape::drawDecal(
    const NIFFile::NIFVertexTransform& modelTransform,
    const DDSTexture * const *textures, unsigned int textureMask,
    const NIFFile::NIFBounds& b, std::uint32_t c)
{
#if ENABLE_TXST_DECALS
  if (BRANCH_UNLIKELY(!(textureMask & 1U)))
    return;                     // no diffuse texture
  if (BRANCH_UNLIKELY(m.flags
                      & (BGSMFile::Flag_IsEffect | BGSMFile::Flag_TSWater)))
  {
    return;
  }
  if (debugMode && !(debugMode == 4U || (debugMode & 0x80000000U)))
    return;

  NIFFile::NIFVertexTransform vt(modelTransform);
  vt *= viewTransform;
  NIFFile::NIFBounds  screenBounds;
  for (int i = 0; i < 8; i++)
  {
    float   x = (!(i & 1) ? b.boundsMin[0] : b.boundsMax[0]);
    float   y = (!(i & 2) ? b.boundsMin[1] : b.boundsMax[1]);
    float   z = (!(i & 4) ? b.boundsMin[2] : b.boundsMax[2]);
    vt.transformXYZ(x, y, z);
    screenBounds += FloatVector4(x, y, z, 0.0f);
  }
  int     x0 = roundFloat(screenBounds.boundsMin[0]);
  int     y0 = roundFloat(screenBounds.boundsMin[1]);
  int     z0 = roundFloat(screenBounds.boundsMin[2]);
  int     x1 = roundFloat(screenBounds.boundsMax[0]);
  int     y1 = roundFloat(screenBounds.boundsMax[1]);
  int     z1 = roundFloat(screenBounds.boundsMax[2]);
  if (x0 >= width || x1 < 0 || y0 >= height || y1 < 0 ||
      z0 >= 16777216 || z1 < 0)
  {
    return;
  }
  x0 = (x0 > 0 ? x0 : 0);
  x1 = (x1 < (width - 1) ? x1 : (width - 1));
  y0 = (y0 > 0 ? y0 : 0);
  y1 = (y1 < (height - 1) ? y1 : (height - 1));

  if (BRANCH_LIKELY(!(textureMask & 0x0008)))
  {
    getDiffuseColorFunc = (!usingSRGBColorSpace ?
                           &getDiffuseColor_Linear : &getDiffuseColor_sRGB);
  }
  else
  {
    getDiffuseColorFunc = (!usingSRGBColorSpace ?
                           &getDiffuseColor_Linear_G : &getDiffuseColor_sRGB_G);
  }
  textureD = textures[0];
  int     mode = (!debugMode ? int(renderMode) : 0);
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
      if (BRANCH_UNLIKELY(!(textureMask & 0x0040U)))
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
      if (BRANCH_UNLIKELY(!(textureMask & 0x0100U)))
        textureR = &defaultTexture_R;
      else
        textureR = textures[8];
      mipOffsetR = calculateMipOffset(textureR, textureD);
      if (BRANCH_UNLIKELY(!(textureMask & 0x0200U)))
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
  if (BRANCH_UNLIKELY(!(textureMask & 0x0002U)))
    textureN = &defaultTexture_N;
  else
    textureN = textures[1];
  mipOffsetN = calculateMipOffset(textureN, textureD);
  if (BRANCH_UNLIKELY(textureMask & 0x0004U))
  {
    if (m.flags & BGSMFile::Flag_Glow)
    {
      textureGlow = textures[2];
      mipOffsetG = calculateMipOffset(textureGlow, textureD);
    }
  }
  if (textureMask & 0x0008U)
    textureG = textures[3];
  if (textureMask & 0x0010U)
    textureE = textures[4];

  NIFFile::NIFVertexTransform viewToModelSpace(vt);
  viewToModelSpace.rotateYX = vt.rotateXY;
  viewToModelSpace.rotateZX = vt.rotateXZ;
  viewToModelSpace.rotateXY = vt.rotateYX;
  viewToModelSpace.rotateZY = vt.rotateYZ;
  viewToModelSpace.rotateXZ = vt.rotateZX;
  viewToModelSpace.rotateYZ = vt.rotateZY;
  viewToModelSpace.scale = 1.0f / viewToModelSpace.scale;
  float   uScale = float(textures[0]->getWidth());
  float   vScale = float(textures[0]->getHeight());
  uScale = (uScale - 1.0f) / (uScale * (b.boundsMax[0] - b.boundsMin[0]));
  vScale = (vScale - 1.0f) / (vScale * (b.boundsMin[2] - b.boundsMax[2]));
  float   uOffset = -(b.boundsMin[0] * uScale);
  float   vOffset = -(b.boundsMax[2] * vScale);

  Fragment  v;
  v.bitangent =
      FloatVector4(vt.rotateXX, vt.rotateYX, vt.rotateZX, vt.rotateXY);
  v.tangent =
      FloatVector4(vt.rotateXZ, vt.rotateYZ, vt.rotateZZ, vt.scale) * -1.0f;
  v.vertexColor = FloatVector4(c);
  if (!usingSRGBColorSpace)
    v.vertexColor *= (1.0f / 255.0f);
  else
    v.vertexColor.srgbExpand();
  v.mipLevel = 0.0f;                    // TODO: calculate mip level
  v.invNormals = false;
  FloatVector4  decalDir(vt.rotateXY, vt.rotateYY, vt.rotateZY, vt.rotateXZ);
  decalDir *= -1.0f;
  for (int y = y0; y <= y1; y++)
  {
    size_t  offs = size_t(y) * size_t(width) + size_t(x0);
    for (int x = x0; x <= x1; x++, offs++)
    {
      v.zPtr = bufZ + offs;
      v.cPtr = bufRGBA + offs;
      v.xyz = FloatVector4(float(x), float(y), *(v.zPtr), 0.0f);
      FloatVector4  modelXYZ(v.xyz);
      modelXYZ -= FloatVector4(viewToModelSpace.offsX, viewToModelSpace.offsY,
                               viewToModelSpace.offsZ, 0.0f);
      modelXYZ = viewToModelSpace.rotateXYZ(modelXYZ);
      modelXYZ *= viewToModelSpace.scale;
      if (modelXYZ[0] < b.boundsMin[0] || modelXYZ[0] > b.boundsMax[0] ||
          modelXYZ[1] < b.boundsMin[1] || modelXYZ[1] > b.boundsMax[1] ||
          modelXYZ[2] < b.boundsMin[2] || modelXYZ[2] > b.boundsMax[2])
      {
        continue;
      }
      if (BRANCH_UNLIKELY(!(bufN && !debugMode)))
      {
        v.normal = decalDir;
      }
      else
      {
        v.normal = FloatVector4::uint32ToNormal(bufN[offs]);
        // cull if surface normal differs from decal direction by > 60 degrees
        if (v.normal.dotProduct3(decalDir) < 0.5f)
          continue;
      }
      v.bitangent[3] = modelXYZ[0] * uScale + uOffset;
      v.tangent[3] = modelXYZ[2] * vScale + vOffset;
      drawPixelFunction(*this, v);
    }
  }
#else
  (void) modelTransform;
  (void) textures;
  (void) textureMask;
  (void) b;
  (void) c;
#endif
}

