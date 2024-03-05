
#include "common.hpp"
#include "fp32vec4.hpp"
#include "nif_file.hpp"
#include "ba2file.hpp"
#include "material.hpp"
#include "meshfile.hpp"

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
  : m(nullptr),
    ts(nullptr),
    flags(0U),
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
    bTmp += ((rotateX * vertexData[i].xyz[0])
             + (rotateY * vertexData[i].xyz[1])
             + (rotateZ * vertexData[i].xyz[2]) + offsXYZ);
  }
  b = bTmp;
}

void NIFFile::NIFTriShape::setMaterial(const CE2Material *material)
{
  m = material;
  // keep TriShape flags
  flags = flags & (Flag_TSHidden | Flag_TSVertexColors | Flag_TSOrdered
                   | Flag_TSMarker);
  if (m)
  {
    // add material specific flags
    flags = flags | m->flags;
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
    if (childBlock >= f.blockCnt)
      errorMessage("invalid child block number in NIF node");
    children.push_back(childBlock);
  }
}

NIFFile::NIFBlkNiNode::~NIFBlkNiNode()
{
}

NIFFile::NIFBlkBSTriShape::NIFBlkBSTriShape(NIFFile& f, int l)
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
  if (f.bsVersion >= 0x90)
  {
    boundMinX = f.readFloat();
    boundMinY = f.readFloat();
    boundMinZ = f.readFloat();
    boundMaxX = f.readFloat();
    boundMaxY = f.readFloat();
    boundMaxZ = f.readFloat();
    if (f.bsVersion >= 0xA0)
    {
      float   tmpX = boundMinX;
      float   tmpY = boundMinY;
      float   tmpZ = boundMinZ;
      boundMinX = tmpX - boundMaxX;
      boundMinY = tmpY - boundMaxY;
      boundMinZ = tmpZ - boundMaxZ;
      boundMaxX = tmpX + boundMaxX;
      boundMaxY = tmpY + boundMaxY;
      boundMaxZ = tmpZ + boundMaxZ;
    }
  }
  else
  {
    boundMinX = 0.0f;
    boundMinY = 0.0f;
    boundMinZ = 0.0f;
    boundMaxX = 0.0f;
    boundMaxY = 0.0f;
    boundMaxZ = 0.0f;
  }
  skinID = f.readBlockID();
  shaderProperty = f.readBlockID();
  alphaProperty = f.readBlockID();
  if (f.bsVersion >= 0xA0)
  {
    // Starfield BSGeometry block
    vertexFmtDesc = 0x0003B00000000000ULL;
    if (f.readUInt8() != 0x01)
      errorMessage("invalid or unsupported BSGeometry data in NIF file");
    unsigned int  triangleCntx3, vertexCnt;
    do
    {
      triangleCntx3 = f.readUInt32();
      vertexCnt = f.readUInt32();
      if ((triangleCntx3 % 3U) != 0U || vertexCnt > 0x00010000U)
        errorMessage("invalid vertex or triangle data size in NIF file");
      if (f.readUInt32() != 0x40U)
        errorMessage("invalid or unsupported BSGeometry data in NIF file");
      unsigned int  meshPathLen = f.readUInt32();
      f.readPath(meshFileName, meshPathLen, "geometries/", ".mesh");
      if (f.getPosition() >= f.size() || f.readUInt8Fast() == 0x00)
        break;
    }
    while (--l >= 0);
    f.ba2File.extractFile(f.meshBuf, meshFileName);
    FileBuffer  tmpBuf(f.meshBuf.data(), f.meshBuf.size());
    readStarfieldMeshFile(vertexData, triangleData, tmpBuf);
    if (vertexData.size() != vertexCnt ||
        triangleData.size() != (triangleCntx3 / 3U))
    {
      errorMessage("invalid vertex or triangle data size in NIF file");
    }
    return;
  }
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
  int     bitangentOffs = -1;
  int     vclrOffs = -1;
  bool    useFloat16 = (f.bsVersion >= 0x80 && !(vertexFmtDesc & (1ULL << 54)));
  if (vertexFmtDesc & (1ULL << 44))
    xyzOffs = int((vertexFmtDesc >> 4) & 0x0FU) << 2;
  if (vertexFmtDesc & (1ULL << 45))
    uvOffs = int((vertexFmtDesc >> 8) & 0x0FU) << 2;
  if (vertexFmtDesc & (1ULL << 47))
    normalOffs = int((vertexFmtDesc >> 16) & 0x0FU) << 2;
  if (vertexFmtDesc & (1ULL << 48))
    bitangentOffs = int((vertexFmtDesc >> 20) & 0x0FU) << 2;
  if (vertexFmtDesc & (1ULL << 49))
    vclrOffs = int((vertexFmtDesc >> 24) & 0x0FU) << 2;
  if ((xyzOffs >= 0 && (xyzOffs + (useFloat16 ? 8 : 16)) > int(vertexSize)) ||
      (uvOffs >= 0 && (uvOffs + 4) > int(vertexSize)) ||
      (normalOffs >= 0 && (normalOffs + 4) > int(vertexSize)) ||
      (bitangentOffs >= 0 && (bitangentOffs + 4) > int(vertexSize)) ||
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
    FloatVector4  tangent(1.0f, 0.0f, 0.0f, 0.0f);
    if (xyzOffs >= 0)
    {
      f.setPosition(offs + size_t(xyzOffs));
      v.xyz = (!useFloat16 ? f.readFloatVector4() : f.readFloat16Vector4());
      tangent[0] = v.xyz[3];
      v.xyz[3] = 1.0f;
    }
    if (uvOffs >= 0)
    {
      f.setPosition(offs + size_t(uvOffs));
      v.texCoord = FloatVector4::convertFloat16(f.readUInt32Fast(), true);
    }
    if (normalOffs >= 0)
    {
      f.setPosition(offs + size_t(normalOffs));
      FloatVector4  normal(f.readUInt32Fast());
      normal = normal * (1.0f / 127.5f) - 1.0f;
      tangent[1] = normal[3];
      v.normal = normal.convertToX10Y10Z10();
    }
    if (bitangentOffs >= 0)
    {
      f.setPosition(offs + size_t(bitangentOffs));
      FloatVector4  bitangent(f.readUInt32Fast());
      bitangent = bitangent * (1.0f / 127.5f) - 1.0f;
      tangent[2] = bitangent[3];
      v.bitangent = bitangent.convertToX10Y10Z10();
    }
    if (vclrOffs >= 0)
    {
      f.setPosition(offs + size_t(vclrOffs));
      v.vertexColor = f.readUInt32Fast();
    }
    v.tangent = tangent.convertToX10Y10Z10();
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

NIFFile::NIFBlkBSLightingShaderProperty::NIFBlkBSLightingShaderProperty(
    NIFFile& f, CE2MaterialDB *materials)
  : NIFBlock(NIFFile::BlkTypeBSLightingShaderProperty),
    shaderType(0U),
    controller(-1),
    material(nullptr)
{
  if (f.bsVersion < 0x90)
    return;
  int     n = f.readInt32();
  if (std::uintptr_t(n) < std::uintptr_t(f.stringTable.size()))
  {
    nameID = n;
    if (!f.stringTable[n].empty())
    {
      // set material path
      size_t  i;
      if (f.stringTable[n].starts_with("materials/"))
        f.stringBuf = f.stringTable[n];
      else if ((i = f.stringTable[n].find("/materials/")) != std::string::npos)
        f.stringBuf.assign(f.stringTable[n], i + 1);
      else
        (f.stringBuf = "materials/").append(f.stringTable[n]);
      if (f.bsVersion >= 0xAC && !f.stringBuf.ends_with(".mat"))
      {
        if (f.stringBuf.ends_with(".bgsm") || f.stringBuf.ends_with(".bgem"))
          f.stringBuf.resize(f.stringBuf.length() - 5);
        f.stringBuf += ".mat";
      }
      materialPath = f.stringBuf;
    }
  }
  for (unsigned int i = f.readUInt32(); i--; )
    extraData.push_back(f.readUInt32());
  controller = f.readBlockID();
  if (materials && !materialPath.empty())
  {
    material = materials->loadMaterial(materialPath);
    if (material && (material->flags & CE2Material::Flag_IsEffect))
      shaderType--;
  }
}

NIFFile::NIFBlkBSLightingShaderProperty::~NIFBlkBSLightingShaderProperty()
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

void NIFFile::loadNIFFile(CE2MaterialDB *materials, int l)
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
  if (bsVersion >= 0xAC) [[likely]]
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
  blockOffsets = new size_t[blockCnt + 1];
  blockOffsets[0] = 0;
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

  blocks = new NIFBlock*[blockCnt];
  for (size_t i = 0; i < blockCnt; i++)
    blocks[i] = nullptr;
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
          blocks[i] = new NIFBlkBSTriShape(*this, l);
          break;
        case BlkTypeBSLightingShaderProperty:
          blocks[i] = new NIFBlkBSLightingShaderProperty(*this, materials);
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
    for (size_t i = 0; i < blockCnt; i++)
    {
      if (blocks[i])
        delete blocks[i];
    }
    throw;
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
      if (n >= blockCnt)
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
        v[i].flags = v[i].flags | NIFTriShape::Flag_TSOrdered;
    }
    return;
  }

  const NIFBlkBSTriShape& b = *((const NIFBlkBSTriShape *) blocks[blockNum]);
  if (b.vertexData.size() < 1 || b.triangleData.size() < 1)
    return;
  NIFTriShape t;
  t.ts = &b;
  t.vertexTransform = b.vertexTransform;
  // hidden, has vertex colors
  t.flags = std::uint32_t(((b.flags & 0x01) << 15)
                          | ((b.vertexFmtDesc >> 36) & 0x2000));
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
        t.flags = t.flags | NIFTriShape::Flag_TSMarker;
        break;
      }
      s++;
    }
    while (--n);
  }
  t.vertexCnt = (unsigned int) b.vertexData.size();
  t.triangleCnt = (unsigned int) b.triangleData.size();
  t.vertexData = b.vertexData.data();
  t.triangleData = b.triangleData.data();
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
      t.setMaterial(lsBlock.material);
    }
    else if (baseBlockType == BlkTypeBSWaterShaderProperty)
    {
      t.flags = t.flags
                | NIFTriShape::Flag_TSWater | NIFTriShape::Flag_TSAlphaBlending;
    }
    else
    {
      // unknown shader type
      t.flags = t.flags | NIFTriShape::Flag_TSHidden;
      return;
    }
  }
  else
  {
    t.flags = t.flags | NIFTriShape::Flag_TSHidden;
  }
  v.push_back(t);
}

NIFFile::NIFFile(const char *fileName, const BA2File& archiveFiles,
                 CE2MaterialDB *materials, int l)
  : FileBuffer(fileName),
    blockOffsets(nullptr),
    blocks(nullptr),
    ba2File(archiveFiles)
{
  try
  {
    loadNIFFile(materials, l);
  }
  catch (...)
  {
    if (blocks)
      delete[] blocks;
    if (blockOffsets)
      delete[] blockOffsets;
    throw;
  }
}

NIFFile::NIFFile(const unsigned char *buf, size_t bufSize,
                 const BA2File& archiveFiles,
                 CE2MaterialDB *materials, int l)
  : FileBuffer(buf, bufSize),
    blockOffsets(nullptr),
    blocks(nullptr),
    ba2File(archiveFiles)
{
  try
  {
    loadNIFFile(materials, l);
  }
  catch (...)
  {
    if (blocks)
      delete[] blocks;
    if (blockOffsets)
      delete[] blockOffsets;
    throw;
  }
}

NIFFile::NIFFile(FileBuffer& buf, const BA2File& archiveFiles,
                 CE2MaterialDB *materials, int l)
  : FileBuffer(buf.data(), buf.size()),
    blockOffsets(nullptr),
    blocks(nullptr),
    ba2File(archiveFiles)
{
  try
  {
    loadNIFFile(materials, l);
  }
  catch (...)
  {
    if (blocks)
      delete[] blocks;
    if (blockOffsets)
      delete[] blockOffsets;
    throw;
  }
}

NIFFile::~NIFFile()
{
  for (size_t i = 0; i < blockCnt; i++)
  {
    if (blocks[i])
      delete blocks[i];
  }
  delete[] blocks;
  delete[] blockOffsets;
}

void NIFFile::getMesh(std::vector< NIFTriShape >& v, unsigned int rootNode,
                      unsigned int switchActive, bool noRootNodeTransform) const
{
  v.clear();
  if (rootNode < blockCnt &&
      (blocks[rootNode]->isNode() || blocks[rootNode]->isTriShape()))
  {
    std::vector< unsigned int > parentBlocks;
    getMesh(v, rootNode, parentBlocks, switchActive, noRootNodeTransform);
  }
}

