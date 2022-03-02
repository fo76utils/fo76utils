
#include "common.hpp"
#include "nif_file.hpp"

#define ENABLE_DEBUG    1

#ifdef ENABLE_DEBUG
#  include "ba2file.hpp"
#endif

NIFFile::NIFVertexTransform::NIFVertexTransform()
  : offsX(0.0f), offsY(0.0f), offsZ(0.0f),
    rotateXX(1.0f), rotateXY(0.0f), rotateXZ(0.0f),
    rotateYX(0.0f), rotateYY(1.0f), rotateYZ(0.0f),
    rotateZX(0.0f), rotateZY(0.0f), rotateZZ(1.0f),
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
  float   r_xx = rotateXX;
  float   r_xy = rotateXY;
  float   r_xz = rotateXZ;
  float   r_yx = rotateYX;
  float   r_yy = rotateYY;
  float   r_yz = rotateYZ;
  float   r_zx = rotateZX;
  float   r_zy = rotateZY;
  float   r_zz = rotateZZ;
  rotateXX = (r_xx * r.rotateXX) + (r_yx * r.rotateXY) + (r_zx * r.rotateXZ);
  rotateXY = (r_xy * r.rotateXX) + (r_yy * r.rotateXY) + (r_zy * r.rotateXZ);
  rotateXZ = (r_xz * r.rotateXX) + (r_yz * r.rotateXY) + (r_zz * r.rotateXZ);
  rotateYX = (r_xx * r.rotateYX) + (r_yx * r.rotateYY) + (r_zx * r.rotateYZ);
  rotateYY = (r_xy * r.rotateYX) + (r_yy * r.rotateYY) + (r_zy * r.rotateYZ);
  rotateYZ = (r_xz * r.rotateYX) + (r_yz * r.rotateYY) + (r_zz * r.rotateYZ);
  rotateZX = (r_xx * r.rotateZX) + (r_yx * r.rotateZY) + (r_zx * r.rotateZZ);
  rotateZY = (r_xy * r.rotateZX) + (r_yy * r.rotateZY) + (r_zy * r.rotateZZ);
  rotateZZ = (r_xz * r.rotateZX) + (r_yz * r.rotateZY) + (r_zz * r.rotateZZ);
  scale = scale * r.scale;
  return (*this);
}

void NIFFile::NIFVertexTransform::transformXYZ(
    float& x, float& y, float& z) const
{
  float   tmpX = (x * rotateXX) + (y * rotateXY) + (z * rotateXZ);
  float   tmpY = (x * rotateYX) + (y * rotateYY) + (z * rotateYZ);
  float   tmpZ = (x * rotateZX) + (y * rotateZY) + (z * rotateZZ);
  x = tmpX * scale + offsX;
  y = tmpY * scale + offsY;
  z = tmpZ * scale + offsZ;
}

void NIFFile::NIFVertexTransform::transformVertex(NIFVertex& v) const
{
  float   x = v.x;
  float   y = v.y;
  float   z = v.z;
  v.x = ((x * rotateXX) + (y * rotateXY) + (z * rotateXZ)) * scale + offsX;
  v.y = ((x * rotateYX) + (y * rotateYY) + (z * rotateYZ)) * scale + offsY;
  v.z = ((x * rotateZX) + (y * rotateZY) + (z * rotateZZ)) * scale + offsZ;
  x = v.normalX;
  y = v.normalY;
  z = v.normalZ;
  v.normalX = (x * rotateXX) + (y * rotateXY) + (z * rotateXZ);
  v.normalY = (x * rotateYX) + (y * rotateYY) + (z * rotateYZ);
  v.normalZ = (x * rotateZX) + (y * rotateZY) + (z * rotateZZ);
}

void NIFFile::NIFVertexTransform::debugPrint() const
{
#ifdef ENABLE_DEBUG
  std::printf("  NIFVertexTransform:\n");
  std::printf("    Scale: %f\n", scale);
  std::printf("    Rotation matrix: %8f, %8f, %8f\n",
              rotateXX, rotateXY, rotateXZ);
  std::printf("                     %8f, %8f, %8f\n",
              rotateYX, rotateYY, rotateYZ);
  std::printf("                     %8f, %8f, %8f\n",
              rotateZX, rotateZY, rotateZZ);
  std::printf("    Offset: %f, %f, %f\n", offsX, offsY, offsZ);
#endif
}

NIFFile::NIFBlock::NIFBlock(NIFFile& nifFile, int blockType,
                            const unsigned char *blockBuf, size_t blockBufSize)
  : FileBuffer(blockBuf, blockBufSize),
    f(nifFile),
    type(blockType),
    nameID(-1)
{
  if (blockType != NIFFile::BlkTypeBSLightingShaderProperty)
  {
    int     n = readInt32();
    if (n >= 0 && n < int(nifFile.stringTable.size()))
      nameID = n;
  }
}

NIFFile::NIFBlock::~NIFBlock()
{
}

NIFFile::NIFBlkNiNode::NIFBlkNiNode(NIFFile& nifFile,
                                    const unsigned char *blockBuf,
                                    size_t blockBufSize)
  : NIFBlock(nifFile, NIFFile::BlkTypeNiNode, blockBuf, blockBufSize)
{
  unsigned int  n = readUInt32();
  while (n--)
    extraData.push_back(readUInt32());
  controller = readInt32();
  flags = readUInt32();
  vertexTransform.readFromBuffer(*this);
  collisionObject = readInt32();
  n = readUInt32();
  while (n--)
    children.push_back(readUInt32());
#ifdef ENABLE_DEBUG
  std::printf("NiNode:\n");
  if (nameID >= 0)
    std::printf("  Name: %s\n", f.stringTable[nameID].c_str());
  if (extraData.size() > 0)
  {
    std::printf("  Extra data:\n");
    for (size_t i = 0; i < extraData.size(); i++)
      std::printf("    %3u\n", extraData[i]);
  }
  if (controller >= 0)
    std::printf("  Controller: %3d\n", controller);
  std::printf("  Flags: 0x%08X\n", flags);
  vertexTransform.debugPrint();
  if (collisionObject >= 0)
    std::printf("  Collision object: %3d\n", collisionObject);
  if (children.size() > 0)
  {
    std::printf("  Children:\n");
    for (size_t i = 0; i < children.size(); i++)
      std::printf("    %3u\n", children[i]);
  }
#endif
}

NIFFile::NIFBlkNiNode::~NIFBlkNiNode()
{
}

NIFFile::NIFBlkBSTriShape::NIFBlkBSTriShape(NIFFile& nifFile,
                                            const unsigned char *blockBuf,
                                            size_t blockBufSize)
  : NIFBlock(nifFile, NIFFile::BlkTypeBSTriShape, blockBuf, blockBufSize)
{
  unsigned int  n = readUInt32();
  while (n--)
    extraData.push_back(readUInt32());
  controller = readInt32();
  flags = readUInt32();
  vertexTransform.readFromBuffer(*this);
  collisionObject = readInt32();
  boundCenterX = readFloat();
  boundCenterY = readFloat();
  boundCenterZ = readFloat();
  boundRadius = readFloat();
  if (nifFile.bsVersion < 0x90)
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
    boundMinX = readFloat();
    boundMinY = readFloat();
    boundMinZ = readFloat();
    boundMaxX = readFloat();
    boundMaxY = readFloat();
    boundMaxZ = readFloat();
  }
  skinID = readInt32();
  shaderProperty = readInt32();
  alphaProperty = readInt32();
  vertexFmtDesc = readUInt64();
  size_t  triangleCnt;
  if (nifFile.bsVersion < 0x80)
    triangleCnt = readUInt16();
  else
    triangleCnt = readUInt32();
  size_t  vertexCnt = readUInt16();
  size_t  vertexSize = size_t(vertexFmtDesc & 0x0FU) << 2;
  size_t  dataSize = readUInt32();
  if (vertexSize < 4 || (getPosition() + dataSize) > size() ||
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
      throw errorMessage("invalid vertex or triangle data size in NIF file");
    }
  }
  int     xyzOffs = -1;
  int     uvOffs = -1;
  int     normalsOffs = -1;
  int     vclrOffs = -1;
  bool    useFloat16 =
      (nifFile.bsVersion >= 0x80 && !(vertexFmtDesc & (1ULL << 54)));
  if (vertexFmtDesc & (1ULL << 44))
    xyzOffs = int((vertexFmtDesc >> 4) & 0x0FU) << 2;
  if (vertexFmtDesc & (1ULL << 45))
    uvOffs = int((vertexFmtDesc >> 8) & 0x0FU) << 2;
  if (vertexFmtDesc & (1ULL << 47))
    normalsOffs = int((vertexFmtDesc >> 16) & 0x0FU) << 2;
  if (vertexFmtDesc & (1ULL << 49))
    vclrOffs = int((vertexFmtDesc >> 24) & 0x0FU) << 2;
  if ((xyzOffs >= 0 && (xyzOffs + (useFloat16 ? 6 : 12)) > int(vertexSize)) ||
      (uvOffs >= 0 && (uvOffs + 4) > int(vertexSize)) ||
      (normalsOffs >= 0 && (normalsOffs + 3) > int(vertexSize)) ||
      (vclrOffs >= 0 && (vclrOffs + 4) > int(vertexSize)))
  {
    throw errorMessage("invalid vertex format in NIF file");
  }
  vertexData.resize(vertexCnt);
  triangleData.resize(triangleCnt);
  for (size_t i = 0; i < vertexCnt; i++)
  {
    NIFVertex&  v = vertexData[i];
    size_t  offs = getPosition();
    v.x = 0.0f;
    v.y = 0.0f;
    v.z = 0.0f;
    v.normalX = 0.0f;
    v.normalY = 0.0f;
    v.normalZ = 1.0f;
    v.u = 0;
    v.v = 0;
    v.vertexColor = 0xFFFFFFFFU;
    if (xyzOffs >= 0)
    {
      setPosition(offs + size_t(xyzOffs));
      if (!useFloat16)
      {
        v.x = readFloat();
        v.y = readFloat();
        v.z = readFloat();
      }
      else
      {
        v.x = NIFFile::NIFVertex::convertFloat16(readUInt16Fast());
        v.y = NIFFile::NIFVertex::convertFloat16(readUInt16Fast());
        v.z = NIFFile::NIFVertex::convertFloat16(readUInt16Fast());
      }
    }
    if (uvOffs >= 0)
    {
      setPosition(offs + size_t(uvOffs));
      v.u = readUInt16Fast();
      v.v = readUInt16Fast();
    }
    if (normalsOffs >= 0)
    {
      setPosition(offs + size_t(normalsOffs));
      v.normalX = (float(int(readUInt8Fast())) - 127.5f) * (1.0f / 127.5f);
      v.normalY = (float(int(readUInt8Fast())) - 127.5f) * (1.0f / 127.5f);
      v.normalZ = (float(int(readUInt8Fast())) - 127.5f) * (1.0f / 127.5f);
    }
    if (vclrOffs >= 0)
    {
      setPosition(offs + size_t(vclrOffs));
      v.vertexColor = readUInt32Fast();
    }
    setPosition(offs + vertexSize);
  }
  for (size_t i = 0; i < triangleCnt; i++)
  {
    triangleData[i].v0 = readUInt16Fast();
    triangleData[i].v1 = readUInt16Fast();
    triangleData[i].v2 = readUInt16Fast();
    if (triangleData[i].v0 >= vertexCnt || triangleData[i].v1 >= vertexCnt ||
        triangleData[i].v2 >= vertexCnt)
    {
      throw errorMessage("invalid triangle data in NIF file");
    }
  }
#ifdef ENABLE_DEBUG
  std::printf("BSTriShape:\n");
  if (nameID >= 0)
    std::printf("  Name: %s\n", f.stringTable[nameID].c_str());
  if (extraData.size() > 0)
  {
    std::printf("  Extra data:\n");
    for (size_t i = 0; i < extraData.size(); i++)
      std::printf("    %3u\n", extraData[i]);
  }
  if (controller >= 0)
    std::printf("  Controller: %3d\n", controller);
  std::printf("  Flags: 0x%08X\n", flags);
  vertexTransform.debugPrint();
  if (collisionObject >= 0)
    std::printf("  Collision object: %3d\n", collisionObject);
  std::printf("  Bounding sphere: (%f, %f, %f), %f\n",
              boundCenterX, boundCenterY, boundCenterZ, boundRadius);
  if (skinID >= 0)
    std::printf("  Skin: %3d\n", skinID);
  if (shaderProperty >= 0)
    std::printf("  Shader property: %3d\n", shaderProperty);
  if (alphaProperty >= 0)
    std::printf("  Alpha property: %3d\n", alphaProperty);
  std::printf("  Vertex format: 0x%016llX\n", vertexFmtDesc);
  std::printf("  Vertex list:\n");
  for (size_t i = 0; i < vertexCnt; i++)
  {
    std::printf("    %4d: X: %f, Y: %f, Z: %f, U: %f, V: %f\n",
                int(i), vertexData[i].x, vertexData[i].y, vertexData[i].z,
                vertexData[i].getU(), vertexData[i].getV());
  }
  std::printf("  Triangle list:\n");
  for (size_t i = 0; i < triangleCnt; i++)
  {
    std::printf("    %4d: %4u, %4u, %4u\n",
                int(i),
                triangleData[i].v0, triangleData[i].v1, triangleData[i].v2);
  }
#endif
}

NIFFile::NIFBlkBSTriShape::~NIFBlkBSTriShape()
{
}

NIFFile::NIFBlkBSLightingShaderProperty::NIFBlkBSLightingShaderProperty(
    NIFFile& nifFile, size_t blockNum,
    const unsigned char *blockBuf, size_t blockBufSize)
  : NIFBlock(nifFile, NIFFile::BlkTypeBSLightingShaderProperty,
             blockBuf, blockBufSize)
{
  if (nifFile.bsVersion < 0x90)
    type = readUInt32();
  else
    type = 0;
  int     n = readInt32();
  if (n >= 0 && n < int(nifFile.stringTable.size()))
  {
    nameID = n;
    if (nifFile.bsVersion >= 0x80 && !nifFile.stringTable[n].empty())
    {
      materialName = nifFile.stringTable[n];
      if (std::strncmp(materialName.c_str(), "materials/", 10) != 0)
        materialName.insert(0, "materials/");
    }
  }
  if (nifFile.bsVersion < 0x90)
  {
    for (unsigned int i = readUInt32(); i--; )
      extraData.push_back(readUInt32());
    controller = readInt32();
    flags = readUInt64();
    offsetU = readFloat();
    offsetV = readFloat();
    scaleU = readFloat();
    scaleV = readFloat();
    textureSet = readInt32();
  }
  else
  {
    controller = -1;
    flags = 0ULL;
    offsetU = 0.0f;
    offsetV = 0.0f;
    scaleU = 1.0f;
    scaleV = 1.0f;
    textureSet = -1;
    if ((blockNum + 1) < nifFile.blockTypes.size() &&
        nifFile.blockTypes[blockNum + 1] == NIFFile::BlkTypeBSShaderTextureSet)
    {
      textureSet = int(blockNum + 1);
    }
  }
#ifdef ENABLE_DEBUG
  std::printf("BSLightingShaderProperty:\n");
  if (nameID >= 0)
    std::printf("  Name: %s\n", f.stringTable[nameID].c_str());
  std::printf("  Material: %s\n", materialName.c_str());
  if (extraData.size() > 0)
  {
    std::printf("  Extra data:\n");
    for (size_t i = 0; i < extraData.size(); i++)
      std::printf("    %3u\n", extraData[i]);
  }
  if (controller >= 0)
    std::printf("  Controller: %3d\n", controller);
  std::printf("  Flags: 0x%016llX\n", flags);
  std::printf("  UV offset: %f, %f\n", offsetU, offsetV);
  std::printf("  UV scale: %f, %f\n", scaleU, scaleV);
  std::printf("  Texture set: %3d\n", textureSet);
#endif
}

NIFFile::NIFBlkBSLightingShaderProperty::~NIFBlkBSLightingShaderProperty()
{
}

NIFFile::NIFBlkBSShaderTextureSet::NIFBlkBSShaderTextureSet(
    NIFFile& nifFile, const unsigned char *blockBuf, size_t blockBufSize)
  : NIFBlock(nifFile, NIFFile::BlkTypeBSShaderTextureSet,
             blockBuf, blockBufSize)
{
  texturePaths.resize(f.bsVersion < 0x80 ? 9 : (f.bsVersion < 0x90 ? 10 : 13));
  for (size_t i = 0; i < texturePaths.size(); i++)
  {
    NIFFile::readString(texturePaths[i], *this, 4);
    if (texturePaths[i].length() > 0 &&
        std::strncmp(texturePaths[i].c_str(), "textures/", 9) != 0)
    {
      texturePaths[i].insert(0, "textures/");
    }
  }
#ifdef ENABLE_DEBUG
  std::printf("BSShaderTextureSet:\n");
  if (nameID >= 0)
    std::printf("  Name: %s\n", f.stringTable[nameID].c_str());
  for (size_t i = 0; i < texturePaths.size(); i++)
    std::printf("  Texture %2d: %s\n", int(i), texturePaths[i].c_str());
#endif
}

NIFFile::NIFBlkBSShaderTextureSet::~NIFBlkBSShaderTextureSet()
{
}

void NIFFile::readString(std::string& s,
                         FileBuffer& buf, size_t stringLengthSize)
{
  s.clear();
  size_t  n = ~(size_t(0));
  if (stringLengthSize == 1)
    n = buf.readUInt8();
  else if (stringLengthSize == 2)
    n = buf.readUInt16();
  else if (stringLengthSize)
    n = buf.readUInt32();
  bool    isPath = false;
  while (n--)
  {
    char    c = char(buf.readUInt8());
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

void NIFFile::loadNIFFile()
{
  if (fileBufSize < 57 ||
      std::memcmp(fileBuf, "Gamebryo File Format, Version ", 30) != 0)
  {
    throw errorMessage("invalid NIF file header");
  }
  filePos = 40;
  if (readUInt64() != 0x0000000C01140200ULL)    // 20.2.0.x, 1, 12
    throw errorMessage("unsupported NIF file version or endianness");
  blockCnt = readUInt32Fast();
  bsVersion = readUInt32Fast();
  readString(authorName, *this, 1);
  if (bsVersion >= 0x90)
    (void) readUInt32();
  readString(processScriptName, *this, 1);
  readString(exportScriptName, *this, 1);
  std::string tmp;
  if (bsVersion >= 0x80 && bsVersion < 0x90)
    readString(tmp, *this, 1);
  for (size_t n = readUInt16(); n--; )
  {
    readString(tmp, *this, 4);
    blockTypeStrings.push_back(tmp);
  }
  for (size_t i = 0; i < blockCnt; i++)
  {
    unsigned int  blockType = readUInt16();
    if (blockType >= blockTypeStrings.size())
      throw errorMessage("invalid block type in NIF file");
    blockTypes.push_back(int(blockType));
  }
  for (size_t i = 0; i < blockCnt; i++)
  {
    unsigned int  blockSize = readUInt32();
    blockSizes.push_back(blockSize);
  }
  size_t  stringCnt = readUInt32();
  (void) readUInt32();  // ignore maximum string length
  for (size_t i = 0; i < stringCnt; i++)
  {
    readString(tmp, *this, 4);
    stringTable.push_back(tmp);
  }
  if (readUInt32() != 0)
    throw errorMessage("NIFFile: number of groups > 0 is not supported");
  for (size_t i = 0; i < blockCnt; i++)
  {
    if ((filePos + blockSizes[i]) > fileBufSize)
      throw errorMessage("invalid block size in NIF file");
    blockOffsets.push_back(filePos);
    filePos = filePos + blockSizes[i];
  }
#ifdef ENABLE_DEBUG
  std::printf("BS version: 0x%08X\n", bsVersion);
  std::printf("Author name: %s\n", authorName.c_str());
  std::printf("Process script: %s\n", processScriptName.c_str());
  std::printf("Export script: %s\n", exportScriptName.c_str());

  for (size_t i = 0; i < stringTable.size(); i++)
    std::printf("String %3d: %s\n", int(i), stringTable[i].c_str());

  std::printf("Block count: %u\n", blockCnt);
  for (size_t i = 0; i < blockCnt; i++)
  {
    std::printf("  Block %3d: offset = 0x%08X, size = %7u, type = %d (%s)\n",
                int(i), (unsigned int) blockOffsets[i], blockSizes[i],
                blockTypes[i], blockTypeStrings[blockTypes[i]].c_str());
  }
#endif
  for (size_t i = 0; i < blockCnt; i++)
  {
    const std::string&  blockTypeName = blockTypeStrings[blockTypes[i]];
    if (blockTypeName == "NiNode")
      blockTypes[i] = BlkTypeNiNode;
    else if (blockTypeName == "BSFadeNode")
      blockTypes[i] = BlkTypeBSFadeNode;
    else if (blockTypeName == "BSMultiBoundNode")
      blockTypes[i] = BlkTypeBSMultiBoundNode;
    else if (blockTypeName == "BSTriShape")
      blockTypes[i] = BlkTypeBSTriShape;
    else if (blockTypeName == "BSEffectShaderProperty")
      blockTypes[i] = BlkTypeBSEffectShaderProperty;
    else if (blockTypeName == "NiAlphaProperty")
      blockTypes[i] = BlkTypeNiAlphaProperty;
    else if (blockTypeName == "BSLightingShaderProperty")
      blockTypes[i] = BlkTypeBSLightingShaderProperty;
    else if (blockTypeName == "BSShaderTextureSet")
      blockTypes[i] = BlkTypeBSShaderTextureSet;
    else if (blockTypeName == "BSWaterShaderProperty")
      blockTypes[i] = BlkTypeBSWaterShaderProperty;
  }

  blocks.resize(blockCnt, (NIFBlock *) 0);
  try
  {
    for (size_t i = 0; i < blockCnt; i++)
    {
      const unsigned char *blockDataPtr = fileBuf + blockOffsets[i];
      size_t  blockSize = blockSizes[i];
      switch (blockTypes[i])
      {
        case BlkTypeNiNode:
        case BlkTypeBSFadeNode:
        case BlkTypeBSMultiBoundNode:
          blocks[i] = new NIFBlkNiNode(*this, blockDataPtr, blockSize);
          break;
        case BlkTypeBSTriShape:
          blocks[i] = new NIFBlkBSTriShape(*this, blockDataPtr, blockSize);
          break;
        case BlkTypeBSLightingShaderProperty:
          blocks[i] = new NIFBlkBSLightingShaderProperty(*this, i,
                                                         blockDataPtr,
                                                         blockSize);
          break;
        case BlkTypeBSShaderTextureSet:
          blocks[i] = new NIFBlkBSShaderTextureSet(*this,
                                                   blockDataPtr, blockSize);
          break;
      }
    }
  }
  catch (...)
  {
    for (size_t i = 0; i < blocks.size(); i++)
    {
      if (blocks[i])
        delete blocks[i];
    }
    throw;
  }
}

void NIFFile::getMesh(std::vector< NIFTriShape >& v, unsigned int blockNum,
                      std::vector< unsigned int >& parentBlocks) const
{
  if (blockNum >= blockTypes.size())
    return;
  if (blockTypes[blockNum] == BlkTypeNiNode ||
      blockTypes[blockNum] == BlkTypeBSFadeNode ||
      blockTypes[blockNum] == BlkTypeBSMultiBoundNode)
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
      if (!loopFound)
        getMesh(v, n, parentBlocks);
    }
    parentBlocks.resize(parentBlocks.size() - 1);
    return;
  }

  if (blockTypes[blockNum] != BlkTypeBSTriShape)
    return;
  const NIFBlkBSTriShape& b = *((const NIFBlkBSTriShape *) blocks[blockNum]);
  if (b.vertexData.size() < 1 || b.triangleData.size() < 1)
    return;
  NIFTriShape t;
  t.vertexCnt = b.vertexData.size();
  t.triangleCnt = b.triangleData.size();
  t.vertexData = &(b.vertexData.front());
  t.triangleData = &(b.triangleData.front());
  t.vertexTransform = b.vertexTransform;
  t.isWater = false;
  t.texturePathCnt = 0;
  t.texturePaths = (std::string *) 0;
  t.materialPath = (std::string *) 0;
  t.textureOffsetU = 0.0f;
  t.textureOffsetV = 0.0f;
  t.textureScaleU = 1.0f;
  t.textureScaleV = 1.0f;
  t.name = "";
  if (b.nameID >= 0)
    t.name = stringTable[b.nameID].c_str();
  for (size_t i = parentBlocks.size(); i-- > 0; )
  {
    t.vertexTransform *=
        ((const NIFBlkNiNode *) blocks[parentBlocks[i]])->vertexTransform;
  }
  size_t  n = size_t(b.shaderProperty);
  if (b.shaderProperty >= 0 && n < blockTypes.size())
  {
    if (blockTypes[n] == BlkTypeBSWaterShaderProperty)
    {
      t.isWater = true;
    }
    else if (blockTypes[n] == BlkTypeBSLightingShaderProperty)
    {
      const NIFBlkBSLightingShaderProperty& lsBlock =
          *((const NIFBlkBSLightingShaderProperty *) blocks[n]);
      if (!lsBlock.materialName.empty())
        t.materialPath = &(lsBlock.materialName);
      t.textureOffsetU = lsBlock.offsetU;
      t.textureOffsetV = lsBlock.offsetV;
      t.textureScaleU = lsBlock.scaleU;
      t.textureScaleV = lsBlock.scaleV;
      if (lsBlock.textureSet >= 0 &&
          size_t(lsBlock.textureSet) < blockTypes.size())
      {
        n = size_t(lsBlock.textureSet);
      }
    }
    if (blockTypes[n] == BlkTypeBSShaderTextureSet)
    {
      const NIFBlkBSShaderTextureSet& tsBlock =
          *((const NIFBlkBSShaderTextureSet *) blocks[n]);
      t.texturePathCnt = (unsigned char) tsBlock.texturePaths.size();
      t.texturePaths = &(tsBlock.texturePaths.front());
    }
  }
  v.push_back(t);
}

NIFFile::NIFFile(const char *fileName)
  : FileBuffer(fileName)
{
  loadNIFFile();
}

NIFFile::NIFFile(const unsigned char *buf, size_t bufSize)
  : FileBuffer(buf, bufSize)
{
  loadNIFFile();
}

NIFFile::NIFFile(FileBuffer& buf)
  : FileBuffer(buf.getDataPtr(), buf.size())
{
  loadNIFFile();
}

NIFFile::~NIFFile()
{
  for (size_t i = 0; i < blocks.size(); i++)
  {
    if (blocks[i])
      delete blocks[i];
  }
}

void NIFFile::getMesh(std::vector< NIFTriShape >& v) const
{
  v.clear();
  std::vector< unsigned int > parentBlocks;
  getMesh(v, 0U, parentBlocks);
}

#ifdef ENABLE_DEBUG
int main(int argc, char **argv)
{
  if (argc < 3)
  {
    std::fprintf(stderr, "Usage: %s ARCHIVEPATH PATTERN...\n", argv[0]);
    return 1;
  }
  try
  {
    std::vector< std::string >  fileNames;
    for (int i = 2; i < argc; i++)
      fileNames.push_back(argv[i]);
    BA2File ba2File(argv[1], &fileNames);
    ba2File.getFileList(fileNames);
    for (size_t i = 0; i < fileNames.size(); i++)
    {
      if (fileNames[i].length() < 5 ||
          std::strcmp(fileNames[i].c_str() + fileNames[i].length() - 4, ".nif")
          != 0)
      {
        continue;
      }
      std::printf("==== %s ====\n", fileNames[i].c_str());
      std::vector< unsigned char >  fileBuf;
      ba2File.extractFile(fileBuf, fileNames[i]);
      NIFFile nifFile(&(fileBuf.front()), fileBuf.size());
      std::vector< NIFFile::NIFTriShape > meshData;
      nifFile.getMesh(meshData);
      for (size_t j = 0; j < meshData.size(); j++)
      {
        std::printf("TriShape %3d (%s):\n", int(j), meshData[j].name);
        std::printf("  Vertex count: %d\n", int(meshData[j].vertexCnt));
        std::printf("  Triangle count: %d\n", int(meshData[j].triangleCnt));
        std::printf("  Is water: %d\n", int(meshData[j].isWater));
        meshData[j].vertexTransform.debugPrint();
        if (meshData[j].materialPath)
          std::printf("  Material: %s\n", meshData[j].materialPath->c_str());
        std::printf("  Texture UV offset, scale: (%f, %f), (%f, %f)\n",
                    meshData[j].textureOffsetU, meshData[j].textureOffsetV,
                    meshData[j].textureScaleU, meshData[j].textureScaleV);
        for (size_t k = 0; k < meshData[j].texturePathCnt; k++)
        {
          std::printf("  Texture %2d: %s\n",
                      int(k), meshData[j].texturePaths[k].c_str());
        }
        std::printf("  Vertex list:\n");
        for (size_t k = 0; k < meshData[j].vertexCnt; k++)
        {
          NIFFile::NIFVertex  v = meshData[j].vertexData[k];
          meshData[j].vertexTransform.transformVertex(v);
          std::printf("    %4d: XYZ: (%f, %f, %f), normals: (%f, %f, %f), "
                      "UV: (%f, %f), color: 0x%08X\n",
                      int(k), v.x, v.y, v.z, v.normalX, v.normalY, v.normalZ,
                      v.getU(), v.getV(), v.vertexColor);
        }
        std::printf("  Triangle list:\n");
        for (size_t k = 0; k < meshData[j].triangleCnt; k++)
        {
          std::printf("    %4d: %4u, %4u, %4u\n",
                      int(k),
                      meshData[j].triangleData[k].v0,
                      meshData[j].triangleData[k].v1,
                      meshData[j].triangleData[k].v2);
        }
      }
    }
  }
  catch (std::exception& e)
  {
    std::fprintf(stderr, "%s: %s\n", argv[0], e.what());
    return 1;
  }
  return 0;
}
#endif

