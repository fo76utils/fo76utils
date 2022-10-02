
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

NIFFile::NIFTextureSet::NIFTextureSet(size_t n)
{
  texturePaths = new const std::string*[n + 1];
  for (size_t i = 0; i <= n; i++)
    texturePaths[i] = (std::string *) 0;
  texturePaths = texturePaths + 1;
}

NIFFile::NIFTextureSet::~NIFTextureSet()
{
  texturePaths = texturePaths - 1;
  delete[] texturePaths;
}

NIFFile::NIFTriShape::NIFTriShape()
  : vertexCnt(0),
    triangleCnt(0),
    vertexData((NIFVertex *) 0),
    triangleData((NIFTriangle *) 0),
    texturePaths((std::string **) 0),
    name("")
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
    const BGSMFile& material, const std::string * const *materialTexturePaths,
    std::uint32_t alphaProperties)
{
  texturePaths = materialTexturePaths;
  // keep TriShape flags
  unsigned int  flags = (m.flags & 0xE000U) | (material.flags & 0x0FFFU);
  m = material;
  if (BRANCH_UNLIKELY(alphaProperties))
  {
    m.alphaFlags = std::uint16_t(alphaProperties & 0xFFFFU);
    m.alphaThreshold = (unsigned char) ((alphaProperties >> 16) & 0xFFU);
    m.alpha = (unsigned char) ((alphaProperties >> 24) & 0xFFU);
  }
  if (flags & BGSMFile::Flag_TSWater)
    flags = flags | (BGSMFile::Flag_TwoSided | BGSMFile::Flag_TSAlphaBlending);
  if (m.isAlphaBlending() && m.alpha)
    flags = flags | BGSMFile::Flag_TSAlphaBlending;
  m.flags = std::uint16_t(flags);
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
  unsigned int  texturePathMap =
      (f.bsVersion < 0x80 ? 0x00000030U : 0x00051430U);
  for ( ; texturePathMap; texturePathMap = texturePathMap >> 4)
  {
    size_t  i = texturePathMap & 15U;
    size_t  len = f.readUInt32();
    f.readPath(f.stringBuf, len, "textures/");
    if (!f.stringBuf.empty())
    {
      material.texturePathMask =
          material.texturePathMask | std::uint16_t(1U << (unsigned int) i);
      texturePaths[i] = f.storeString(f.stringBuf);
    }
    if (!i)
    {
      material.flags = std::uint16_t((f.readUInt32() & 0x03) | 0x04);
      f.setPosition(f.getPosition() + 16);
      material.emissiveColor = std::uint32_t(f.readFloatVector4() * 255.0f);
      // base color scale
      material.emissiveColor =
          (material.emissiveColor & 0x00FFFFFFU)
          | (std::uint32_t(floatToUInt8Clamped(f.readFloat(), 128.0f)) << 24);
      f.setPosition(f.getPosition() + 4);
    }
  }
  // grayscale to alpha
  material.flags = material.flags | std::uint16_t((flags << 1) & 0x40U);
  if (f.bsVersion >= 0x80)
    material.envMapScale = floatToUInt8Clamped(f.readFloat(), 128.0f);
  if (!(material.texturePathMask & 0x0010))
  {
    material.specularSmoothness = 0;
    material.envMapScale = 0;
  }
  else
  {
    material.specularSmoothness = 255;
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
  material.emissiveColor =
      std::uint32_t(f.readFloatVector4()
                    * FloatVector4(255.0f, 255.0f, 255.0f, 128.0f));
  if (f.bsVersion >= 0x80)
    f.setPosition(f.getPosition() + 4);         // root material
  material.flags = std::uint16_t(f.readUInt32Fast() & 0x03);
  material.alpha = floatToUInt8Clamped(f.readFloat(), 128.0f);
  f.setPosition(f.getPosition() + 4);           // refraction strength
  float   s = f.readFloat();                    // glossiness or smoothness
  if (f.bsVersion < 0x80)
    s = (s > 2.0f ? ((float(std::log2(s)) - 1.0f) * (1.0f / 9.0f)) : 0.0f);
  material.specularSmoothness = floatToUInt8Clamped(s, 255.0f);
  FloatVector4  specColor(f.readFloatVector4());
  s = specColor[3] * 128.0f;
  specColor[3] = specColor[3] * (128.0f / 255.0f);
  material.specularColor = std::uint32_t(specColor * 255.0f);
  if (f.bsVersion >= 0x80)
  {
    f.setPosition(f.getPosition() + 12);
    material.gradientMapV =
        (unsigned char) (roundFloat(f.readFloat() * 255.0f) & 0xFF);
  }
  if (shaderType == 1)
  {
    f.setPosition(f.getPosition() + (f.bsVersion < 0x80 ? 8 : 28));
    // s = specular scale * 128
    material.envMapScale = floatToUInt8Clamped(f.readFloat(), s);
  }
}

NIFFile::NIFBlkBSLightingShaderProperty::NIFBlkBSLightingShaderProperty(
    NIFFile& f, size_t nxtBlk, int nxtBlkType, bool isEffect,
    const BA2File *ba2File)
  : NIFBlock(NIFFile::BlkTypeBSLightingShaderProperty),
    texturePaths(f.bsVersion < 0x90 ? (!isEffect ? 8 : 6) : 10)
{
  shaderType = (!isEffect ? 0U : 0xFFFFFFFFU);
  if (f.bsVersion < 0x90 && !isEffect)
    shaderType = f.readUInt32();
  int     n = f.readInt32();
  if (n >= 0 && n < int(f.stringTable.size()))
  {
    nameID = n;
    if (f.bsVersion >= 0x80 && !f.stringTable[n]->empty())
    {
      // set material path
      texturePaths[-1] = f.stringTable[n];
      if (std::strncmp(materialName()->c_str(), "materials/", 10) != 0)
      {
        f.stringBuf = *(materialName());
        size_t  i = f.stringBuf.find("/materials/");
        if (i != std::string::npos)
          f.stringBuf.erase(0, i + 1);
        else
          f.stringBuf.insert(0, "materials/");
        texturePaths[-1] = f.storeString(f.stringBuf);
      }
    }
  }
  for (unsigned int i = f.readUInt32(); i--; )
    extraData.push_back(f.readUInt32());
  controller = f.readBlockID();
  flags = 0ULL;
  textureSet = -1;
  if (f.bsVersion < 0x90)
  {
    flags = f.readUInt64();
    material.textureOffsetU = f.readFloat();
    material.textureOffsetV = f.readFloat();
    material.textureScaleU = f.readFloat();
    material.textureScaleV = f.readFloat();
    if (isEffect)
      readEffectShaderProperty(f);
    else
      readLightingShaderProperty(f);
    // two sided
    material.flags = material.flags | std::uint16_t((flags >> 32) & 0x10U);
    // glow map
    material.flags = material.flags | std::uint16_t((flags >> 31) & 0x80U);
  }
  else if (!isEffect && nxtBlkType == NIFFile::BlkTypeBSShaderTextureSet)
  {
    textureSet = int(nxtBlk);
  }
  if (materialName() && ba2File)
  {
    BGSMFile  bgsmFile;
    try
    {
      bgsmFile.loadBGSMFile(f.bgsmTexturePaths, *ba2File, *(materialName()));
      if (isEffect)
        bgsmFile.flags = bgsmFile.flags | BGSMFile::Flag_IsEffect;
      if (bgsmFile.isAlphaBlending() && bgsmFile.alpha)
        bgsmFile.flags = bgsmFile.flags | BGSMFile::Flag_TSAlphaBlending;
      material = bgsmFile;
      n = (f.bsVersion < 0x90 ? (!isEffect ? 8 : 6) : 10);
      unsigned int  m =
          bgsmFile.texturePathMask & ((1U << (unsigned int) n) - 1U);
      material.texturePathMask = std::uint16_t(m);
      for (int i = 0; m; i++, m = m >> 1)
        texturePaths[i] = f.storeString(f.bgsmTexturePaths[i]);
    }
    catch (FO76UtilsError&)
    {
    }
  }
}

NIFFile::NIFBlkBSLightingShaderProperty::~NIFBlkBSLightingShaderProperty()
{
}

NIFFile::NIFBlkBSShaderTextureSet::NIFBlkBSShaderTextureSet(NIFFile& f)
  : NIFBlock(NIFFile::BlkTypeBSShaderTextureSet),
    texturePaths(f.bsVersion < 0x90 ? 8 : 10)
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
  for ( ; texturePathMap; texturePathMap = texturePathMap >> 4)
  {
    size_t  n = f.readUInt32();
    size_t  j = size_t(texturePathMap & 15U);
    if (j == 15)
    {
      f.setPosition(f.getPosition() + n);
      continue;
    }
    f.readPath(f.stringBuf, n, "textures/");
    if (!f.stringBuf.empty())
    {
      texturePathMask = texturePathMask | std::uint16_t(1U << (unsigned int) j);
      texturePaths[j] = f.storeString(f.stringBuf);
    }
  }
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

void NIFFile::readString(size_t stringLengthSize)
{
  std::string&  s = stringBuf;
  s.clear();
  size_t  n = ~(size_t(0));
  if (stringLengthSize == 1)
    n = readUInt8();
  else if (stringLengthSize == 2)
    n = readUInt16();
  else if (stringLengthSize)
    n = readUInt32();
  bool    isPath = false;
  while (n--)
  {
    char    c = char(readUInt8());
    if (!c)
    {
      if (stringLengthSize)
        continue;
      break;
    }
    if ((unsigned char) c < 0x20)
      c = ' ';
    if (c == '.' || c == '/' || c == '\\')
      isPath = true;
    s += c;
  }
  if (isPath && stringLengthSize > 1)
  {
    for (size_t i = 0; i < s.length(); i++)
    {
      char    c = s[i];
      if (c >= 'A' && c <= 'Z')
        s[i] = c + ('a' - 'A');
      else if (c == '\\')
        s[i] = '/';
    }
  }
}

const std::string * NIFFile::storeString(std::string& s)
{
  std::set< std::string >::const_iterator i = stringSet.insert(s).first;
  return &(*i);
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
  readString(1);
  authorName = storeString(stringBuf);
  if (bsVersion >= 0x84)
    (void) readUInt32();
  readString(1);
  processScriptName = storeString(stringBuf);
  readString(1);
  exportScriptName = storeString(stringBuf);

  std::vector< unsigned char >  blockTypes;
  if (bsVersion >= 0x80 && bsVersion < 0x84)
    readString(1);
  {
    std::vector< int >  tmpBlkTypes;
    for (size_t n = readUInt16(); n--; )
    {
      readString(4);
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
  (void) readUInt32();  // ignore maximum string length
  for (size_t i = 0; i < stringCnt; i++)
  {
    readString(4);
    stringTable.push_back(storeString(stringBuf));
  }
  if (readUInt32() != 0)
    errorMessage("NIFFile: number of groups > 0 is not supported");
  for (size_t i = 0; i <= blockCnt; i++)
  {
    blockOffsets[i] = blockOffsets[i] + filePos;
    if (blockOffsets[i] > fileBufSize)
      errorMessage("invalid block size in NIF file");
  }

  blocks.resize(blockCnt, (NIFBlock *) 0);
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
    if (!lspBlock.material.texturePathMask && lspBlock.textureSet >= 0)
    {
      if (blockTypeBaseTable[blockTypes[lspBlock.textureSet]]
          == BlkTypeBSShaderTextureSet)
      {
        NIFBlkBSShaderTextureSet& tsBlock =
            *((NIFBlkBSShaderTextureSet *) blocks[lspBlock.textureSet]);
        int     n = 10;
        if (bsVersion < 0x90)
          n = (!(lspBlock.material.flags & BGSMFile::Flag_IsEffect) ? 8 : 6);
        unsigned int  m =
            tsBlock.texturePathMask & ((1U << (unsigned int) n) - 1U);
        lspBlock.material.texturePathMask = std::uint16_t(m);
        for (int j = 0; m; j++, m = m >> 1)
          lspBlock.texturePaths[j] = tsBlock.texturePaths[j];
      }
    }
  }
}

void NIFFile::getMesh(std::vector< NIFTriShape >& v, unsigned int blockNum,
                      std::vector< unsigned int >& parentBlocks,
                      unsigned int switchActive, bool noRootNodeTransform) const
{
  if (blockNum >= blocks.size())
    return;
  if (blocks[blockNum]->isNode())
  {
    parentBlocks.push_back(blockNum);
    const NIFBlkNiNode& b = *((const NIFBlkNiNode *) blocks[blockNum]);
    for (size_t i = 0; i < b.children.size(); i++)
    {
      unsigned int  n = b.children[i];
      bool    loopFound = false;
      for (size_t j = 0; j < parentBlocks.size(); j++)
      {
        if (parentBlocks[j] == n)
        {
          loopFound = true;
          break;
        }
      }
      if (!(loopFound || (b.type == BlkTypeNiSwitchNode && i != switchActive)))
        getMesh(v, n, parentBlocks, switchActive, noRootNodeTransform);
    }
    parentBlocks.resize(parentBlocks.size() - 1);
    return;
  }

  if (!blocks[blockNum]->isTriShape())
    return;
  const NIFBlkBSTriShape& b = *((const NIFBlkBSTriShape *) blocks[blockNum]);
  if (b.vertexData.size() < 1 || b.triangleData.size() < 1)
    return;
  NIFTriShape t;
  t.vertexCnt = (unsigned int) b.vertexData.size();
  t.triangleCnt = (unsigned int) b.triangleData.size();
  t.vertexData = &(b.vertexData.front());
  t.triangleData = &(b.triangleData.front());
  t.vertexTransform = b.vertexTransform;
  // hidden, has vertex colors
  t.m.flags = std::uint16_t(((b.flags & 0x01) << 15)
                            | ((b.vertexFmtDesc >> 36) & 0x2000));
  if (b.nameID >= 0)
    t.name = stringTable[b.nameID]->c_str();
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
      t.setMaterial(lsBlock.material, lsBlock.texturePaths.texturePaths, a);
    }
    else if (baseBlockType == BlkTypeBSWaterShaderProperty)
    {
      t.m.flags =
          t.m.flags | (BGSMFile::Flag_TSWater | BGSMFile::Flag_TSAlphaBlending
                       | BGSMFile::Flag_TwoSided);
      t.m.specularColor = 0x80FFFFFFU;
      t.m.specularSmoothness = 255;
      t.m.emissiveColor = 0xD0804000U;
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
  : FileBuffer(buf.getDataPtr(), buf.size())
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
  std::vector< unsigned int > parentBlocks;
  getMesh(v, rootNode, parentBlocks, switchActive, noRootNodeTransform);
}

