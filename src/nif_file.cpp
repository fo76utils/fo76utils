
#include "common.hpp"
#include "nif_file.hpp"
#include "ba2file.hpp"
#include "bgsmfile.hpp"
#include "fp32vec4.hpp"

#include "nifblock.cpp"

NIFFile::NIFVertexTransform::NIFVertexTransform()
  : offsX(0.0f), offsY(0.0f), offsZ(0.0f),
    rotateXX(1.0f), rotateYX(0.0f), rotateZX(0.0f),
    rotateXY(0.0f), rotateYY(1.0f), rotateZY(0.0f),
    rotateXZ(0.0f), rotateYZ(0.0f), rotateZZ(1.0f),
    scale(1.0f)
{
}

NIFFile::NIFVertexTransform::NIFVertexTransform(
    float xyzScale, float rx, float ry, float rz,
    float offsetX, float offsetY, float offsetZ)
{
  offsX = offsetX;
  offsY = offsetY;
  offsZ = offsetZ;
  float   rx_s = float(std::sin(rx));
  float   rx_c = float(std::cos(rx));
  float   ry_s = float(std::sin(ry));
  float   ry_c = float(std::cos(ry));
  float   rz_s = float(std::sin(rz));
  float   rz_c = float(std::cos(rz));
  if (float(std::fabs(rx_s)) < 0.000001f)
    rx_s = 0.0f;
  if (float(std::fabs(rx_c)) < 0.000001f)
    rx_c = 0.0f;
  if (float(std::fabs(ry_s)) < 0.000001f)
    ry_s = 0.0f;
  if (float(std::fabs(ry_c)) < 0.000001f)
    ry_c = 0.0f;
  if (float(std::fabs(rz_s)) < 0.000001f)
    rz_s = 0.0f;
  if (float(std::fabs(rz_c)) < 0.000001f)
    rz_c = 0.0f;
  rotateXX = ry_c * rz_c;
  rotateXY = ry_c * rz_s;
  rotateXZ = -ry_s;
  rotateYX = (rx_s * ry_s * rz_c) - (rx_c * rz_s);
  rotateYY = (rx_s * ry_s * rz_s) + (rx_c * rz_c);
  rotateYZ = rx_s * ry_c;
  rotateZX = (rx_c * ry_s * rz_c) + (rx_s * rz_s);
  rotateZY = (rx_c * ry_s * rz_s) - (rx_s * rz_c);
  rotateZZ = rx_c * ry_c;
  scale = xyzScale;
}

void NIFFile::NIFVertexTransform::readFromBuffer(FileBuffer& buf)
{
  offsX = buf.readFloat();
  offsY = buf.readFloat();
  offsZ = buf.readFloat();
  rotateXX = buf.readFloat();
  rotateXY = buf.readFloat();
  rotateXZ = buf.readFloat();
  rotateYX = buf.readFloat();
  rotateYY = buf.readFloat();
  rotateYZ = buf.readFloat();
  rotateZX = buf.readFloat();
  rotateZY = buf.readFloat();
  rotateZZ = buf.readFloat();
  scale = buf.readFloat();
}

NIFFile::NIFVertexTransform& NIFFile::NIFVertexTransform::operator*=(
    const NIFVertexTransform& r)
{
  r.transformXYZ(offsX, offsY, offsZ);
  FloatVector4  r_x(r.rotateXX, r.rotateYX, r.rotateZX, r.rotateXY);
  FloatVector4  r_y(r.rotateXY, r.rotateYY, r.rotateZY, r.rotateXZ);
  FloatVector4  r_z(r.rotateXZ, r.rotateYZ, r.rotateZZ, r.scale);
  FloatVector4  tmp((r_x * rotateXX) + (r_y * rotateYX) + (r_z * rotateZX));
  rotateXX = tmp[0];
  rotateYX = tmp[1];
  rotateZX = tmp[2];
  tmp = (r_x * rotateXY) + (r_y * rotateYY) + (r_z * rotateZY);
  rotateXY = tmp[0];
  rotateYY = tmp[1];
  rotateZY = tmp[2];
  tmp = (r_x * rotateXZ) + (r_y * rotateYZ) + (r_z * rotateZZ);
  rotateXZ = tmp[0];
  rotateYZ = tmp[1];
  rotateZZ = tmp[2];
  scale = scale * r.scale;
  return (*this);
}

FloatVector4 NIFFile::NIFVertexTransform::rotateXYZ(FloatVector4 v) const
{
  FloatVector4  tmpX(rotateXX, rotateYX, rotateZX, rotateXY);
  tmpX *= v[0];
  FloatVector4  tmpY(rotateXY, rotateYY, rotateZY, rotateXZ);
  tmpY *= v[1];
  FloatVector4  tmpZ(rotateXZ, rotateYZ, rotateZZ, scale);
  tmpZ *= v[2];
  tmpX = tmpX + tmpY + tmpZ;
  return tmpX.clearV3();
}

FloatVector4 NIFFile::NIFVertexTransform::transformXYZ(FloatVector4 v) const
{
  FloatVector4  tmpX(rotateXX, rotateYX, rotateZX, rotateXY);
  tmpX *= v[0];
  FloatVector4  tmpY(rotateXY, rotateYY, rotateZY, rotateXZ);
  tmpY *= v[1];
  FloatVector4  tmpZ(rotateXZ, rotateYZ, rotateZZ, scale);
  tmpZ *= v[2];
  tmpX = tmpX + tmpY + tmpZ;
  tmpX *= scale;
  tmpX += FloatVector4(offsX, offsY, offsZ, rotateXX);
  return tmpX.clearV3();
}

void NIFFile::NIFVertexTransform::rotateXYZ(float& x, float& y, float& z) const
{
  FloatVector4  tmpX(rotateXX, rotateYX, rotateZX, rotateXY);
  tmpX *= x;
  FloatVector4  tmpY(rotateXY, rotateYY, rotateZY, rotateXZ);
  tmpY *= y;
  FloatVector4  tmpZ(rotateXZ, rotateYZ, rotateZZ, scale);
  tmpZ *= z;
  tmpX = tmpX + tmpY + tmpZ;
  x = tmpX[0];
  y = tmpX[1];
  z = tmpX[2];
}

void NIFFile::NIFVertexTransform::transformXYZ(
    float& x, float& y, float& z) const
{
  FloatVector4  tmpX(rotateXX, rotateYX, rotateZX, rotateXY);
  tmpX *= x;
  FloatVector4  tmpY(rotateXY, rotateYY, rotateZY, rotateXZ);
  tmpY *= y;
  FloatVector4  tmpZ(rotateXZ, rotateYZ, rotateZZ, scale);
  tmpZ *= z;
  tmpX = tmpX + tmpY + tmpZ;
  tmpX *= scale;
  tmpX += FloatVector4(offsX, offsY, offsZ, rotateXX);
  x = tmpX[0];
  y = tmpX[1];
  z = tmpX[2];
}

NIFFile::NIFTriShape::NIFTriShape()
  : nameID(-1),
    vertexCnt(0),
    triangleCnt(0),
    vertexData(nullptr),
    triangleData(nullptr)
{
}

void NIFFile::NIFTriShape::calculateBounds(NIFBounds& b,
                                           const NIFVertexTransform *vt) const
{
  NIFVertexTransform  t(vertexTransform);
  if (vt)
    t *= *vt;
  FloatVector4  rotateX(t.rotateXX, t.rotateYX, t.rotateZX, t.rotateXY);
  FloatVector4  rotateY(t.rotateXY, t.rotateYY, t.rotateZY, t.rotateXZ);
  FloatVector4  rotateZ(t.rotateXZ, t.rotateYZ, t.rotateZZ, t.scale);
  FloatVector4  offsXYZ(t.offsX, t.offsY, t.offsZ, t.rotateXX);
  rotateX *= t.scale;
  rotateY *= t.scale;
  rotateZ *= t.scale;
  NIFBounds bTmp(b);
  for (size_t i = 0; i < vertexCnt; i++)
  {
    bTmp += ((rotateX * vertexData[i].x) + (rotateY * vertexData[i].y)
             + (rotateZ * vertexData[i].z) + offsXYZ);
  }
  b = bTmp;
}

void NIFFile::NIFTriShape::setMaterial(
    const BGSMFile& material, std::uint32_t alphaProperties)
{
  // keep TriShape flags and NIF file version
  std::uint32_t tsFlagsMask =
      BGSMFile::Flag_TSMarker | BGSMFile::Flag_TSHidden | BGSMFile::Flag_TSWater
      | BGSMFile::Flag_TSVertexColors | BGSMFile::Flag_TSOrdered;
  std::uint32_t flags = m.flags & tsFlagsMask;
  std::uint32_t nifVersion = m.nifVersion;
  m = material;
  m.flags = (m.flags & ~tsFlagsMask) | flags;
  m.nifVersion = (nifVersion ? nifVersion : m.nifVersion);
  if (alphaProperties) [[unlikely]]
  {
    m.alphaFlags = std::uint16_t(alphaProperties & 0xFFFFU);
    m.alphaThreshold = (unsigned char) ((alphaProperties >> 16) & 0xFFU);
    m.alpha = (unsigned char) ((alphaProperties >> 24) & 0xFFU);
    m.updateAlphaProperties();
  }
  if (m.flags & BGSMFile::Flag_TSWater)
  {
    m.flags =
        m.flags | (BGSMFile::Flag_TwoSided | BGSMFile::Flag_TSAlphaBlending);
  }
}

NIFFile::NIFBlock::NIFBlock(int blockType)
  : type(blockType),
    nameID(-1)
{
}

NIFFile::NIFBlock::~NIFBlock()
{
}

NIFFile::NIFBlkNiNode::NIFBlkNiNode(NIFFile& f)
  : NIFBlock(NIFFile::BlkTypeNiNode)
{
  nameID = f.readInt32();
  if (nameID < 0 || size_t(nameID) >= f.stringTable.size())
    nameID = -1;
  for (unsigned int n = f.readUInt32(); n--; )
    extraData.push_back(f.readUInt32());
  controller = f.readBlockID();
  flags = f.readUInt32();
  vertexTransform.readFromBuffer(f);
  collisionObject = f.readBlockID();
  for (unsigned int n = f.readUInt32(); n--; )
  {
    unsigned int  childBlock = f.readUInt32();
    if (childBlock == 0xFFFFFFFFU)
      continue;
    if (childBlock >= f.blocks.size())
      errorMessage("invalid child block number in NIF node");
    children.push_back(childBlock);
  }
}

NIFFile::NIFBlkNiNode::~NIFBlkNiNode()
{
}

NIFFile::NIFBlkBSTriShape::NIFBlkBSTriShape(NIFFile& f)
  : NIFBlock(NIFFile::BlkTypeBSTriShape)
{
  nameID = f.readInt32();
  if (nameID < 0 || size_t(nameID) >= f.stringTable.size())
    nameID = -1;
  for (unsigned int n = f.readUInt32(); n--; )
    extraData.push_back(f.readUInt32());
  controller = f.readBlockID();
  flags = f.readUInt32();
  vertexTransform.readFromBuffer(f);
  collisionObject = f.readBlockID();
  boundCenterX = f.readFloat();
  boundCenterY = f.readFloat();
  boundCenterZ = f.readFloat();
  boundRadius = f.readFloat();
  if (f.bsVersion < 0x90)
  {
    boundMinX = 0.0f;
    boundMinY = 0.0f;
    boundMinZ = 0.0f;
    boundMaxX = 0.0f;
    boundMaxY = 0.0f;
    boundMaxZ = 0.0f;
  }
  else
  {
    boundMinX = f.readFloat();
    boundMinY = f.readFloat();
    boundMinZ = f.readFloat();
    boundMaxX = f.readFloat();
    boundMaxY = f.readFloat();
    boundMaxZ = f.readFloat();
  }
  skinID = f.readBlockID();
  shaderProperty = f.readBlockID();
  alphaProperty = f.readBlockID();
  vertexFmtDesc = f.readUInt64();
  size_t  triangleCnt;
  if (f.bsVersion < 0x80)
    triangleCnt = f.readUInt16();
  else
    triangleCnt = f.readUInt32();
  size_t  vertexCnt = f.readUInt16();
  size_t  vertexSize = size_t(vertexFmtDesc & 0x0FU) << 2;
  size_t  dataSize = f.readUInt32();
  if (vertexSize < 4 || (f.getPosition() + dataSize) > f.size() ||
      ((vertexCnt * vertexSize) + (triangleCnt * 6UL)) > dataSize)
  {
    if (!vertexSize || !vertexCnt || !triangleCnt)
    {
      vertexFmtDesc = 0ULL;
      triangleCnt = 0;
      vertexCnt = 0;
      vertexSize = 0;
      dataSize = 0;
    }
    else
    {
      errorMessage("invalid vertex or triangle data size in NIF file");
    }
  }
  int     xyzOffs = -1;
  int     uvOffs = -1;
  int     normalOffs = -1;
  int     tangentOffs = -1;
  int     vclrOffs = -1;
  bool    useFloat16 = (f.bsVersion >= 0x80 && !(vertexFmtDesc & (1ULL << 54)));
  if (vertexFmtDesc & (1ULL << 44))
    xyzOffs = int((vertexFmtDesc >> 4) & 0x0FU) << 2;
  if (vertexFmtDesc & (1ULL << 45))
    uvOffs = int((vertexFmtDesc >> 8) & 0x0FU) << 2;
  if (vertexFmtDesc & (1ULL << 47))
    normalOffs = int((vertexFmtDesc >> 16) & 0x0FU) << 2;
  if (vertexFmtDesc & (1ULL << 48))
    tangentOffs = int((vertexFmtDesc >> 20) & 0x0FU) << 2;
  if (vertexFmtDesc & (1ULL << 49))
    vclrOffs = int((vertexFmtDesc >> 24) & 0x0FU) << 2;
  if ((xyzOffs >= 0 && (xyzOffs + (useFloat16 ? 8 : 16)) > int(vertexSize)) ||
      (uvOffs >= 0 && (uvOffs + 4) > int(vertexSize)) ||
      (normalOffs >= 0 && (normalOffs + 4) > int(vertexSize)) ||
      (tangentOffs >= 0 && (tangentOffs + 4) > int(vertexSize)) ||
      (vclrOffs >= 0 && (vclrOffs + 4) > int(vertexSize)))
  {
    errorMessage("invalid vertex format in NIF file");
  }
  vertexData.resize(vertexCnt);
  triangleData.resize(triangleCnt);
  for (size_t i = 0; i < vertexCnt; i++)
  {
    NIFVertex&  v = vertexData[i];
    size_t  offs = f.getPosition();
    int     bitangentX = 255;
    int     bitangentY = 128;
    int     bitangentZ = 128;
    if (xyzOffs >= 0)
    {
      f.setPosition(offs + size_t(xyzOffs));
      FloatVector4  xyz(!useFloat16 ?
                        f.readFloatVector4() : f.readFloat16Vector4());
      v.x = xyz[0];
      v.y = xyz[1];
      v.z = xyz[2];
      bitangentX = floatToUInt8Clamped(xyz[3] + 1.0f, 127.5f);
    }
    if (uvOffs >= 0)
    {
      f.setPosition(offs + size_t(uvOffs));
      v.u = f.readUInt16Fast();
      v.v = f.readUInt16Fast();
    }
    if (normalOffs >= 0)
    {
      f.setPosition(offs + size_t(normalOffs));
      v.normal = f.readUInt32Fast();
      bitangentY = int((v.normal >> 24) & 0xFFU);
      v.normal = v.normal & 0x00FFFFFFU;
    }
    if (tangentOffs >= 0)
    {
      f.setPosition(offs + size_t(tangentOffs));
      v.tangent = f.readUInt32Fast();
      bitangentZ = int((v.tangent >> 24) & 0xFFU);
      v.tangent = v.tangent & 0x00FFFFFFU;
    }
    if (vclrOffs >= 0)
    {
      f.setPosition(offs + size_t(vclrOffs));
      v.vertexColor = f.readUInt32Fast();
    }
    v.bitangent =
        std::uint32_t(bitangentX | (bitangentY << 8) | (bitangentZ << 16));
    f.setPosition(offs + vertexSize);
  }
  for (size_t i = 0; i < triangleCnt; i++)
  {
    triangleData[i].v0 = f.readUInt16Fast();
    triangleData[i].v1 = f.readUInt16Fast();
    triangleData[i].v2 = f.readUInt16Fast();
    if (triangleData[i].v0 >= vertexCnt || triangleData[i].v1 >= vertexCnt ||
        triangleData[i].v2 >= vertexCnt)
    {
      errorMessage("invalid triangle data in NIF file");
    }
  }
}

NIFFile::NIFBlkBSTriShape::~NIFBlkBSTriShape()
{
}

void NIFFile::NIFBlkBSLightingShaderProperty::readEffectShaderProperty(
    NIFFile& f)
{
  material.texturePathMask |= material.texturePaths.readTexturePaths(f, 0U);
  unsigned int  tmp = f.readUInt32();
  material.flags = (tmp & 0x03U) | 0x04U;       // is effect
  material.e.lightingInfluence =
      float(int((tmp >> 8) & 0xFFU)) * (1.0f / 255.0f);
  material.e.falloffParams = f.readFloatVector4();
  material.e.falloffParams.maxValues(FloatVector4(0.0f));
  material.e.falloffParams.minValues(FloatVector4(1.0f));
  if (f.bsVersion >= 0x90)
    f.setPosition(f.getPosition() + 4);
  material.e.baseColor = f.readFloatVector4();
  material.e.baseColor.maxValues(FloatVector4(0.0f));
  material.e.baseColor.minValues(FloatVector4(1.0f));
  material.e.baseColorScale = f.readFloat();
  material.e.baseColorScale =
      std::min(std::max(material.e.baseColorScale, 0.0f), 8.0f);
  f.setPosition(f.getPosition() + 4);
  material.texturePathMask |=
      material.texturePaths.readTexturePaths(
          f, (f.bsVersion < 0x80 ? 0x00000003U : 0x00005143U));
  // grayscale to alpha
  material.flags = material.flags | std::uint32_t((flags << 1) & 0x0040U);
  if (flags & (((f.bsVersion << 1) & 0x0100U) | 0x0040U))
    material.flags = material.flags | BGSMFile::Flag_FalloffEnabled;
  // effect lighting
  material.flags = material.flags | std::uint32_t((flags >> 52) & 0x0400U);
  if (f.bsVersion >= 0x80)
  {
    material.e.envMapScale = f.readFloat();
    material.e.envMapScale =
        std::min(std::max(material.e.envMapScale, 0.0f), 8.0f);
    if (f.bsVersion >= 0x90)
    {
      material.texturePathMask |=
          material.texturePaths.readTexturePaths(f, 0x98U);
    }
  }
  if (!(material.texturePathMask & 0x0310))
  {
    material.e.envMapScale = 0.0f;
    material.e.specularSmoothness = 0.0f;
  }
  else
  {
    material.e.specularSmoothness = 1.0f;
  }
}

void NIFFile::NIFBlkBSLightingShaderProperty::readLightingShaderProperty(
    NIFFile& f)
{
  textureSet = f.readBlockID();
  if (!(shaderType == 0 || shaderType == 1))
    return;                             // not default or environment map
  size_t  sizeRequired = (f.bsVersion < 0x80 ? 48 : 68);
  if (shaderType == 1)
    sizeRequired = sizeRequired + (f.bsVersion < 0x80 ? 12 : 32);
  if ((f.getPosition() + sizeRequired) > f.size())
    return;
  material.s.emissiveColor = f.readFloatVector4();
  material.s.emissiveColor.maxValues(FloatVector4(0.0f));
  material.s.emissiveColor.minValues(FloatVector4(1.0f, 1.0f, 1.0f, 8.0f));
  if (f.bsVersion >= 0x80)
    f.setPosition(f.getPosition() + 4);         // root material
  material.flags = f.readUInt32Fast() & 0x03U;
  material.alpha = f.readFloat();
  material.alpha = std::min(std::max(material.alpha, 0.0f), 8.0f);
  f.setPosition(f.getPosition() + 4);           // refraction strength
  float   s = f.readFloat();                    // glossiness or smoothness
  if (f.bsVersion < 0x80)
    s = (s > 2.0f ? ((float(std::log2(s)) - 1.0f) * (1.0f / 9.0f)) : 0.0f);
  material.s.specularSmoothness = std::min(std::max(s, 0.0f), 8.0f);
  material.s.specularColor = f.readFloatVector4();
  material.s.specularColor.maxValues(FloatVector4(0.0f));
  material.s.specularColor.minValues(FloatVector4(1.0f, 1.0f, 1.0f, 8.0f));
  if (f.bsVersion >= 0x80)
  {
    if (f.bsVersion < 0x90)
      f.setPosition(f.getPosition() + 12);
    else
      flags = flags | 1ULL;
    material.s.gradientMapV = f.readFloat();
    material.s.gradientMapV =
        std::min(std::max(material.s.gradientMapV, 0.0f), 1.0f);
  }
  if (!(flags & 0x80U))                 // environment mapping enabled
  {
    material.s.envMapScale = 0.0f;
  }
  else
  {
    material.s.envMapScale = material.s.specularColor[3];
    if (shaderType == 1)
    {
      f.setPosition(f.getPosition() + (f.bsVersion < 0x80 ? 8 : 28));
      material.s.envMapScale *= f.readFloat();
      material.s.envMapScale =
          std::min(std::max(material.s.envMapScale, 0.0f), 8.0f);
    }
  }
  if (!(flags & 0x01U))                 // specular enabled
    material.s.specularColor[3] = 0.0f;
  else if (f.bsVersion >= 0x90)
    material.s.specularColor[3] = 1.0f;
}

static const std::uint32_t fo76ShaderFlagsTable[] =
{
  0x12C549CFU, 0x0AU,   // face
  0x14C5C2ADU, 0x25U,   // vertex colors
  0x1A5C2577U, 0x04U,   // grayscale to palette color
  0x2B9633EFU, 0x00U,   // PBR (interpreted as specular enabled)
  0x2D45EC6EU, 0x24U,   // two sided
  0x35C8C18BU, 0x10U,   // refraction falloff (interpreted as fire refraction)
  0x4B58B946U, 0x12U,   // hair tint (interpreted as hair)
  0x58727978U, 0x15U,   // skin tint
  0x5D2DABECU, 0x09U,   // cast shadows
  0x5DF93B67U, 0x1BU,   // dynamic decal
  0x67B70934U, 0x1FU,   // Z buffer test
  0x74AAC97EU, 0x0FU,   // refraction
  0x7BE0BF93U, 0x31U,   // weapon blood
  0x802D68A3U, 0x1DU,   // external emittance
  0x86DBD392U, 0x16U,   // own emit
  0x8B0FD1F2U, 0x03U,   // vertex alpha
  0x8F044840U, 0x26U,   // glow map
  0x97E67F9FU, 0x0CU,   // model space normals
  0xAC7B1CAAU, 0x07U,   // environment mapping
  0xACA889F3U, 0x22U,   // LOD objects
  0xACEA54F4U, 0x05U,   // grayscale to palette alpha
  0xB2757B8CU, 0x23U,   // no fade
  0xBCBAC5F3U, 0x20U,   // Z buffer write
  0xBE8ADFF2U, 0x27U,   // transform changed
  0xCD92BF4BU, 0x08U,   // RGB falloff
  0xCF08760AU, 0x3EU,   // effect lighting
  0xD0CE0E30U, 0x1EU,   // soft effect
  0xDCFA8A8BU, 0x3EU,   // no exposure (interpreted as effect lighting)
  0xDF3182B0U, 0x01U,   // skinned
  0xE56D16E0U, 0x1AU,   // decal
  0xED440D9CU, 0x06U    // use falloff
};

static std::uint64_t decodeFO76ShaderFlags(FileBuffer& f)
{
  size_t  n = f.readUInt32Fast();       // flags 1
  n = n + f.readUInt32Fast();           // flags 2
  if ((f.getPosition() + (n << 2)) > f.size())
    errorMessage("end of input file");
  std::uint64_t flags = 0ULL;
  for (size_t i = 0; i < n; i++)
  {
    std::uint32_t tmp = f.readUInt32Fast();
    size_t  n0 = 0;
    size_t  n2 = sizeof(fo76ShaderFlagsTable) / sizeof(std::uint32_t);
    for (size_t j = 0; j < 5; j++)
    {
      size_t  n1 = ((n0 + n2) >> 1) & ~(size_t(1));
      n0 = (tmp < fo76ShaderFlagsTable[n1] ? n0 : n1);
      n2 = (tmp < fo76ShaderFlagsTable[n1] ? n1 : n2);
    }
    if (tmp == fo76ShaderFlagsTable[n0])
      flags = flags | (1ULL << fo76ShaderFlagsTable[n0 + 1U]);
  }
  return flags;
}

NIFFile::NIFBlkBSLightingShaderProperty::NIFBlkBSLightingShaderProperty(
    NIFFile& f, size_t nxtBlk, int nxtBlkType, bool isEffect,
    const BA2File *ba2File)
  : NIFBlock(NIFFile::BlkTypeBSLightingShaderProperty)
{
  shaderType = (!isEffect ? 0U : 0xFFFFFFFFU);
  if (f.bsVersion < 0x90 && !isEffect)
    shaderType = f.readUInt32();
  f.stringBuf.clear();
  int     n = f.readInt32();
  if (std::uintptr_t(n) < std::uintptr_t(f.stringTable.size()))
  {
    nameID = n;
    if (f.bsVersion >= 0x80U && !f.stringTable[n].empty())
    {
      // set material path
      size_t  i;
      if (std::strncmp(f.stringTable[n].c_str(), "materials/", 10) == 0)
        f.stringBuf = f.stringTable[n];
      else if ((i = f.stringTable[n].find("/materials/")) != std::string::npos)
        f.stringBuf.assign(f.stringTable[n], i + 1);
      else
        (f.stringBuf = "materials/").append(f.stringTable[n]);
    }
  }
  for (unsigned int i = f.readUInt32(); i--; )
    extraData.push_back(f.readUInt32());
  controller = f.readBlockID();
  flags = 0ULL;
  textureSet = -1;
  bool    haveMaterial = false;
  if (!f.stringBuf.empty() && ba2File)  // f.stringBuf = material path
  {
    try
    {
      material.loadBGSMFile(*ba2File, f.stringBuf);
      haveMaterial = true;
    }
    catch (FO76UtilsError&)
    {
    }
  }
  material.nifVersion = f.bsVersion;
  material.texturePaths.setMaterialPath(f.stringBuf);
  if (!haveMaterial && (f.getPosition() + 72) <= f.size())
  {
    if (f.bsVersion >= 0x90)
    {
      if (!isEffect)
        shaderType = f.readUInt32Fast();
      flags = decodeFO76ShaderFlags(f);
    }
    else
    {
      flags = f.readUInt64();
    }
    FloatVector4  txtOffsScale(f.readFloatVector4());
    txtOffsScale.maxValues(FloatVector4(-256.0f));
    txtOffsScale.minValues(FloatVector4(256.0f));
    material.textureOffsetU = txtOffsScale[0];
    material.textureOffsetV = txtOffsScale[1];
    material.textureScaleU = txtOffsScale[2];
    material.textureScaleV = txtOffsScale[3];
    if (isEffect)
      readEffectShaderProperty(f);
    else
      readLightingShaderProperty(f);
    // decal
    material.flags = material.flags | std::uint32_t((flags >> 23) & 0x08U);
    // two sided
    material.flags = material.flags | std::uint32_t((flags >> 32) & 0x10U);
    // is tree
    material.flags = material.flags | std::uint32_t((flags >> 56) & 0x20U);
    // glow map
    material.flags = material.flags | std::uint32_t((flags >> 31) & 0x80U);
    // no Z buffer write
    material.flags = material.flags | std::uint32_t(~(flags >> 24) & 0x0100U);
  }
  else if (!isEffect && nxtBlkType == NIFFile::BlkTypeBSShaderTextureSet)
  {
    textureSet = int(nxtBlk);
  }
}

NIFFile::NIFBlkBSLightingShaderProperty::~NIFBlkBSLightingShaderProperty()
{
}

NIFFile::NIFBlkBSShaderTextureSet::NIFBlkBSShaderTextureSet(NIFFile& f)
  : NIFBlock(NIFFile::BlkTypeBSShaderTextureSet)
{
  nameID = f.readInt32();
  if (nameID < 0 || size_t(nameID) >= f.stringTable.size())
    nameID = -1;
  texturePathMask = 0;
  unsigned long long  texturePathMap = 0x000000006F743210ULL;   // Fallout 4
  if (f.bsVersion < 0x80)
    texturePathMap = 0x00000000FF54F210ULL;     // Skyrim
  else if (f.bsVersion >= 0x90)
    texturePathMap = 0x0000098FFF743210ULL;     // Fallout 76
  texturePathMask = texturePaths.readTexturePaths(f, texturePathMap);
}

NIFFile::NIFBlkBSShaderTextureSet::~NIFBlkBSShaderTextureSet()
{
}

NIFFile::NIFBlkNiAlphaProperty::NIFBlkNiAlphaProperty(NIFFile& f)
  : NIFBlock(NIFFile::BlkTypeNiAlphaProperty)
{
  int     n = f.readInt32();
  if (n >= 0 && n < int(f.stringTable.size()))
    nameID = n;
  for (unsigned int i = f.readUInt32(); i--; )
    extraData.push_back(f.readUInt32());
  controller = f.readBlockID();
  flags = f.readUInt16();
  alphaThreshold = f.readUInt8();
}

NIFFile::NIFBlkNiAlphaProperty::~NIFBlkNiAlphaProperty()
{
}

void NIFFile::readString(std::string& s, size_t stringLengthSize)
{
  size_t  n = std::string::npos;
  if (stringLengthSize == 1)
    n = readUInt8();
  else if (stringLengthSize == 2)
    n = readUInt16();
  else if (stringLengthSize)
    n = readUInt32();
  FileBuffer::readString(s, n);
  if (s.empty() || stringLengthSize < 2)
    return;
  for (size_t i = 0; i < s.length(); i++)
  {
    char    c = s[i];
    if (c == '.' || c == '/' || c == '\\')
    {
      // convert path
      for (size_t j = 0; j < s.length(); j++)
      {
        c = s[j];
        if (c >= 'A' && c <= 'Z')
          s[j] = c + ('a' - 'A');
        else if (c == '\\')
          s[j] = '/';
      }
      break;
    }
  }
}

void NIFFile::loadNIFFile(const BA2File *ba2File)
{
  if (fileBufSize < 57 ||
      std::memcmp(fileBuf, "Gamebryo File Format, Version ", 30) != 0)
  {
    errorMessage("invalid NIF file header");
  }
  filePos = 40;
  if (readUInt64() != 0x0000000C01140200ULL)    // 20.2.0.x, 1, 12
    errorMessage("unsupported NIF file version or endianness");
  blockCnt = readUInt32Fast();
  bsVersion = readUInt32Fast();
  headerStrings.resize(3);
  readString(headerStrings[0], 1);              // author name
  if (bsVersion >= 0x84)
    (void) readUInt32();
  readString(headerStrings[1], 1);              // process script name
  if (bsVersion == 0xAC) [[unlikely]]
  {
    headerStrings[2] = "0x0000000000000000";
    size_t  n = readUInt8();
    for (size_t i = 0; i < n; i++)
    {
      unsigned char c1 = readUInt8();
      if (i >= 8)
        continue;
      unsigned char c2 = c1 >> 4;
      c1 = c1 & 0x0F;
      headerStrings[2][17 - (i << 1)] = char(c1 + (c1 < 10 ? 0x30 : 0x37));
      headerStrings[2][16 - (i << 1)] = char(c2 + (c2 < 10 ? 0x30 : 0x37));
    }
  }
  else
  {
    readString(headerStrings[2], 1);            // export script name
  }

  std::vector< unsigned char >  blockTypes;
  if (bsVersion >= 0x80 && bsVersion < 0x84)
    readString(stringBuf, 1);
  {
    std::vector< int >  tmpBlkTypes;
    for (size_t n = readUInt16(); n--; )
    {
      readString(stringBuf, 4);
      tmpBlkTypes.push_back(stringToBlockType(stringBuf.c_str()));
    }
    if ((blockCnt * 6ULL + filePos) > fileBufSize)
      errorMessage("end of input file");
    blockTypes.resize(blockCnt);
    for (size_t i = 0; i < blockCnt; i++)
    {
      unsigned int  blockType = readUInt16Fast();
      if (blockType >= tmpBlkTypes.size())
        errorMessage("invalid block type in NIF file");
      blockTypes[i] = (unsigned char) tmpBlkTypes[blockType];
    }
  }
  blockOffsets.resize(blockCnt + 1, 0);
  for (size_t i = 1; i <= blockCnt; i++)
  {
    unsigned int  blockSize = readUInt32Fast();
    blockOffsets[i] = blockOffsets[i - 1] + blockSize;
  }
  size_t  stringCnt = readUInt32();
  if (stringCnt >= ((size() - getPosition()) >> 2))
    errorMessage("invalid string count in NIF file");
  (void) readUInt32();  // ignore maximum string length
  stringTable.resize(stringCnt);
  for (size_t i = 0; i < stringCnt; i++)
    readString(stringTable[i], 4);
  if (readUInt32() != 0)
    errorMessage("NIFFile: number of groups > 0 is not supported");
  for (size_t i = 0; i <= blockCnt; i++)
  {
    blockOffsets[i] = blockOffsets[i] + filePos;
    if (blockOffsets[i] > fileBufSize)
      errorMessage("invalid block size in NIF file");
  }

  blocks.resize(blockCnt, nullptr);
  const unsigned char *savedFileBuf = fileBuf;
  size_t  savedFileBufSize = fileBufSize;
  try
  {
    for (size_t i = 0; i < blockCnt; i++)
    {
      fileBuf = savedFileBuf + blockOffsets[i];
      fileBufSize = blockOffsets[i + 1] - blockOffsets[i];
      filePos = 0;
      int     blockType = blockTypes[i];
      int     baseBlockType = blockTypeBaseTable[blockType];
      switch (baseBlockType)
      {
        case BlkTypeNiNode:
          blocks[i] = new NIFBlkNiNode(*this);
          break;
        case BlkTypeBSTriShape:
          blocks[i] = new NIFBlkBSTriShape(*this);
          break;
        case BlkTypeBSLightingShaderProperty:
          {
            int     t = BlkTypeUnknown;
            if ((i + 1) < blockCnt)
              t = blockTypeBaseTable[blockTypes[i + 1]];
            blocks[i] = new NIFBlkBSLightingShaderProperty(
                                *this, i + 1, t,
                                (blockType == BlkTypeBSEffectShaderProperty),
                                ba2File);
          }
          break;
        case BlkTypeBSShaderTextureSet:
          blocks[i] = new NIFBlkBSShaderTextureSet(*this);
          break;
        case BlkTypeNiAlphaProperty:
          blocks[i] = new NIFBlkNiAlphaProperty(*this);
          break;
        default:
          blocks[i] = new NIFBlock(blockType);
          break;
      }
      blocks[i]->type = blockType;
    }
  }
  catch (...)
  {
    fileBuf = savedFileBuf;
    fileBufSize = savedFileBufSize;
    filePos = savedFileBufSize;
    for (size_t i = 0; i < blocks.size(); i++)
    {
      if (blocks[i])
        delete blocks[i];
    }
    throw;
  }
  fileBuf = savedFileBuf;
  fileBufSize = savedFileBufSize;
  filePos = savedFileBufSize;
  for (size_t i = 0; i < blockCnt; i++)
  {
    if (blockTypeBaseTable[blockTypes[i]] != BlkTypeBSLightingShaderProperty)
      continue;
    NIFBlkBSLightingShaderProperty& lspBlock =
        *((NIFBlkBSLightingShaderProperty *) blocks[i]);
    if (lspBlock.material.texturePathMask || lspBlock.textureSet < 0 ||
        blockTypeBaseTable[blockTypes[lspBlock.textureSet]]
        != BlkTypeBSShaderTextureSet)
    {
      continue;
    }
    NIFBlkBSShaderTextureSet& tsBlock =
        *((NIFBlkBSShaderTextureSet *) blocks[lspBlock.textureSet]);
    std::uint32_t m = tsBlock.texturePathMask;
    if (!m)
      continue;
    if (bool(lspBlock.material.texturePaths) &&
        !lspBlock.material.texturePaths.materialPath().empty())
    {
      stringBuf = lspBlock.material.texturePaths.materialPath();
      lspBlock.material.texturePaths = tsBlock.texturePaths;
      lspBlock.material.texturePaths.setMaterialPath(stringBuf);
    }
    else
    {
      lspBlock.material.texturePaths = tsBlock.texturePaths;
    }
    lspBlock.material.texturePathMask = m;
  }
}

void NIFFile::getMesh(std::vector< NIFTriShape >& v, unsigned int blockNum,
                      std::vector< unsigned int >& parentBlocks,
                      unsigned int switchActive, bool noRootNodeTransform) const
{
  int     blockType = blocks[blockNum]->type;
  if (isNodeBlock(blockType))
  {
    parentBlocks.push_back(blockNum);
    const std::vector< unsigned int >&  children =
        ((const NIFBlkNiNode *) blocks[blockNum])->children;
    size_t  firstTriShape = v.size();
    for (size_t i = 0; i < children.size(); i++)
    {
      if (blockType == BlkTypeNiSwitchNode && i != switchActive)
        continue;
      unsigned int  n = children[i];
      if (n >= blocks.size())
        continue;
      if (!blocks[n]->isTriShape())
      {
        if (!blocks[n]->isNode())
          continue;
        bool    loopFound = false;
        for (size_t j = 0; j < parentBlocks.size(); j++)
        {
          if (parentBlocks[j] == n)
          {
            loopFound = true;
            break;
          }
        }
        if (loopFound)
          continue;
      }
      getMesh(v, n, parentBlocks, switchActive, noRootNodeTransform);
    }
    parentBlocks.pop_back();
    if (blockType == BlkTypeBSOrderedNode) [[unlikely]]
    {
      for (size_t i = firstTriShape + 1; i < v.size(); i++)
        v[i].m.flags = v[i].m.flags | BGSMFile::Flag_TSOrdered;
    }
    return;
  }

  const NIFBlkBSTriShape& b = *((const NIFBlkBSTriShape *) blocks[blockNum]);
  if (b.vertexData.size() < 1 || b.triangleData.size() < 1)
    return;
  NIFTriShape t;
  t.vertexCnt = (unsigned int) b.vertexData.size();
  t.triangleCnt = (unsigned int) b.triangleData.size();
  t.vertexData = b.vertexData.data();
  t.triangleData = b.triangleData.data();
  t.vertexTransform = b.vertexTransform;
  // hidden, has vertex colors
  t.m.flags = std::uint32_t(((b.flags & 0x01) << 15)
                            | ((b.vertexFmtDesc >> 36) & 0x2000));
  t.nameID = b.nameID;
  if (b.nameID >= 0 && size_t(b.nameID) < stringTable.size() &&
      stringTable[b.nameID].length() >= 12)
  {
    size_t  n = stringTable[b.nameID].length() - 11;
    const char  *s = stringTable[b.nameID].c_str();
    do
    {
      // "EditorMa", "rker"
      if (FileBuffer::readUInt64Fast(s) == 0x614D726F74696445ULL &&
          FileBuffer::readUInt32Fast(s + 8) == 0x72656B72U)
      {
        t.m.flags = t.m.flags | BGSMFile::Flag_TSMarker;
        break;
      }
      s++;
    }
    while (--n);
  }
  for (size_t i = parentBlocks.size(); i-- > size_t(noRootNodeTransform); )
  {
    t.vertexTransform *=
        ((const NIFBlkNiNode *) blocks[parentBlocks[i]])->vertexTransform;
  }
  if (b.shaderProperty >= 0)
  {
    size_t  n = size_t(b.shaderProperty);
    int     baseBlockType = getBaseBlockType(n);
    if (baseBlockType == BlkTypeBSLightingShaderProperty)
    {
      const NIFBlkBSLightingShaderProperty& lsBlock =
          *((const NIFBlkBSLightingShaderProperty *) blocks[n]);
      std::uint32_t a = 0U;
      if (b.alphaProperty >= 0 && !lsBlock.material.version &&
          getBaseBlockType(size_t(b.alphaProperty)) == BlkTypeNiAlphaProperty)
      {
        const NIFBlkNiAlphaProperty&  apBlock =
            *((const NIFBlkNiAlphaProperty *) blocks[b.alphaProperty]);
        a = apBlock.flags;
        a = a | (std::uint32_t(apBlock.alphaThreshold) << 16);
        a = a | (std::uint32_t(lsBlock.material.alpha) << 24);
      }
      t.setMaterial(lsBlock.material, a);
    }
    else if (baseBlockType == BlkTypeBSWaterShaderProperty)
    {
      t.m.nifVersion = bsVersion;
      t.m.setWaterColor(0xD0804000U, 1.0f);
    }
    else
    {
      // unknown shader type
      t.m.flags = t.m.flags | BGSMFile::Flag_TSHidden;
      return;
    }
  }
  else
  {
    t.m.flags = t.m.flags | BGSMFile::Flag_TSHidden;
  }
  v.push_back(t);
}

NIFFile::NIFFile(const char *fileName, const BA2File *ba2File)
  : FileBuffer(fileName)
{
  loadNIFFile(ba2File);
}

NIFFile::NIFFile(const unsigned char *buf, size_t bufSize,
                 const BA2File *ba2File)
  : FileBuffer(buf, bufSize)
{
  loadNIFFile(ba2File);
}

NIFFile::NIFFile(FileBuffer& buf, const BA2File *ba2File)
  : FileBuffer(buf.data(), buf.size())
{
  loadNIFFile(ba2File);
}

NIFFile::~NIFFile()
{
  for (size_t i = 0; i < blocks.size(); i++)
  {
    if (blocks[i])
      delete blocks[i];
  }
}

void NIFFile::getMesh(std::vector< NIFTriShape >& v, unsigned int rootNode,
                      unsigned int switchActive, bool noRootNodeTransform) const
{
  v.clear();
  if (rootNode < blocks.size() &&
      (blocks[rootNode]->isNode() || blocks[rootNode]->isTriShape()))
  {
    std::vector< unsigned int > parentBlocks;
    getMesh(v, rootNode, parentBlocks, switchActive, noRootNodeTransform);
  }
}

