
#include "common.hpp"
#include "nif_file.hpp"

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

void NIFFile::NIFVertexTransform::transformVertex(NIFVertex& v)
{
  float   x = (v.x * rotateXX) + (v.y * rotateXY) + (v.z * rotateXZ);
  float   y = (v.x * rotateYX) + (v.y * rotateYY) + (v.z * rotateYZ);
  float   z = (v.x * rotateZX) + (v.y * rotateZY) + (v.z * rotateZZ);
  v.x = x * scale + offsX;
  v.y = y * scale + offsY;
  v.z = z * scale + offsZ;
}

NIFFile::NIFBlock::~NIFBlock()
{
}

NIFFile::NIFBlkNiNode::NIFBlkNiNode(FileBuffer& buf, unsigned int bsVersion)
{
  (void) bsVersion;
  type = NIFFile::BlkTypeNiNode;
  nameID = buf.readInt32();
  unsigned int  n = buf.readUInt32();
  while (n--)
    extraData.push_back(buf.readUInt32());
  controller = buf.readInt32();
  flags = buf.readUInt32();
  vertexTransform.readFromBuffer(buf);
  collisionObject = buf.readInt32();
  n = buf.readUInt32();
  while (n--)
    children.push_back(buf.readUInt32());
#if 1
  std::printf("NiNode:\n");
  if (nameID >= 0)
    std::printf("  Name: %3d\n", nameID);
  if (extraData.size() > 0)
  {
    std::printf("  Extra data:\n");
    for (size_t i = 0; i < extraData.size(); i++)
      std::printf("    %3u\n", extraData[i]);
  }
  if (controller >= 0)
    std::printf("  Controller: %3d\n", controller);
  std::printf("  Flags: 0x%08X\n", flags);
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

float NIFFile::NIFBlkBSTriShape::readFloat16(FileBuffer& buf)
{
  unsigned int  tmp = buf.readUInt16Fast();
  int     e = int((tmp >> 10) & 0x1F);
  if (e == 0x00)
    return 0.0f;
  double  m = double(int((tmp & 0x03FF) | 0x0400));
  m = std::ldexp(m, e - 25);
  if (tmp & 0x8000U)
    m = -m;
  return float(m);
}

NIFFile::NIFBlkBSTriShape::NIFBlkBSTriShape(FileBuffer& buf,
                                            unsigned int bsVersion)
{
  type = NIFFile::BlkTypeBSTriShape;
  nameID = buf.readInt32();
  unsigned int  n = buf.readUInt32();
  while (n--)
    extraData.push_back(buf.readUInt32());
  controller = buf.readInt32();
  flags = buf.readUInt32();
  vertexTransform.readFromBuffer(buf);
  collisionObject = buf.readInt32();
  boundCenterX = buf.readFloat();
  boundCenterY = buf.readFloat();
  boundCenterZ = buf.readFloat();
  boundRadius = buf.readFloat();
  if (bsVersion < 0x90)
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
    boundMinX = buf.readFloat();
    boundMinY = buf.readFloat();
    boundMinZ = buf.readFloat();
    boundMaxX = buf.readFloat();
    boundMaxY = buf.readFloat();
    boundMaxZ = buf.readFloat();
  }
  skinID = buf.readInt32();
  shaderProperty = buf.readInt32();
  alphaProperty = buf.readInt32();
  vertexFmtDesc = buf.readUInt64();
  size_t  triangleCnt;
  if (bsVersion < 0x80)
    triangleCnt = buf.readUInt16();
  else
    triangleCnt = buf.readUInt32();
  size_t  vertexCnt = buf.readUInt16();
  size_t  vertexSize = size_t(vertexFmtDesc & 0x0FU) << 2;
  size_t  dataSize = buf.readUInt32();
  if (vertexSize < 4 || (buf.getPosition() + dataSize) > buf.size() ||
      ((vertexCnt * vertexSize) + (triangleCnt * 6UL)) > dataSize)
  {
    throw errorMessage("invalid vertex or triangle data size in NIF file");
  }
  int     xyzOffs = -1;
  int     uvOffs = -1;
  int     normalsOffs = -1;
  int     tangentsOffs = -1;
  int     vclrOffs = -1;
  bool    useFloat16 = (bsVersion >= 0x80 && !(vertexFmtDesc & (1ULL << 54)));
  if (vertexFmtDesc & (1ULL << 44))
    xyzOffs = int((vertexFmtDesc >> 4) & 0x0FU) << 2;
  if (vertexFmtDesc & (1ULL << 45))
    uvOffs = int((vertexFmtDesc >> 8) & 0x0FU) << 2;
  if (vertexFmtDesc & (1ULL << 47))
    normalsOffs = int((vertexFmtDesc >> 16) & 0x0FU) << 2;
  if (vertexFmtDesc & (1ULL << 48))
    tangentsOffs = int((vertexFmtDesc >> 20) & 0x0FU) << 2;
  if (vertexFmtDesc & (1ULL << 49))
    vclrOffs = int((vertexFmtDesc >> 24) & 0x0FU) << 2;
  if ((xyzOffs >= 0 && (xyzOffs + (useFloat16 ? 6 : 12)) > int(vertexSize)) ||
      (uvOffs >= 0 && (uvOffs + 4) > int(vertexSize)) ||
      (normalsOffs >= 0 && (normalsOffs + 3) > int(vertexSize)) ||
      (tangentsOffs >= 0 && (tangentsOffs + 3) > int(vertexSize)) ||
      (vclrOffs >= 0 && (vclrOffs + 4) > int(vertexSize)))
  {
    throw errorMessage("invalid vertex format in NIF file");
  }
  vertexData.resize(vertexCnt);
  triangleData.resize(triangleCnt);
  for (size_t i = 0; i < vertexCnt; i++)
  {
    NIFVertex&  v = vertexData[i];
    size_t  offs = buf.getPosition();
    v.x = 0.0f;
    v.y = 0.0f;
    v.z = 0.0f;
    v.u = 0.0f;
    v.v = 0.0f;
    v.textureID = (unsigned short) shaderProperty;
    v.normalX = 0x80;
    v.normalY = 0x80;
    v.normalZ = 0x80;
    v.tangentX = 0x80;
    v.tangentY = 0x80;
    v.tangentZ = 0x80;
    v.vertexColor = 0xFFFFFFFFU;
    if (xyzOffs >= 0)
    {
      buf.setPosition(offs + size_t(xyzOffs));
      if (!useFloat16)
      {
        v.x = buf.readFloat();
        v.y = buf.readFloat();
        v.z = buf.readFloat();
      }
      else
      {
        v.x = readFloat16(buf);
        v.y = readFloat16(buf);
        v.z = readFloat16(buf);
      }
    }
    if (uvOffs >= 0)
    {
      buf.setPosition(offs + size_t(uvOffs));
      v.u = readFloat16(buf);
      v.v = readFloat16(buf);
    }
    if (normalsOffs >= 0)
    {
      buf.setPosition(offs + size_t(normalsOffs));
      v.normalX = buf.readUInt8Fast();
      v.normalY = buf.readUInt8Fast();
      v.normalZ = buf.readUInt8Fast();
    }
    if (tangentsOffs >= 0)
    {
      buf.setPosition(offs + size_t(tangentsOffs));
      v.tangentX = buf.readUInt8Fast();
      v.tangentY = buf.readUInt8Fast();
      v.tangentZ = buf.readUInt8Fast();
    }
    if (vclrOffs >= 0)
    {
      buf.setPosition(offs + size_t(vclrOffs));
      v.vertexColor = buf.readUInt32Fast();
    }
    vertexTransform.transformVertex(v);
    buf.setPosition(offs + vertexSize);
  }
  for (size_t i = 0; i < triangleCnt; i++)
  {
    triangleData[i].v0 = buf.readUInt16Fast();
    triangleData[i].v1 = buf.readUInt16Fast();
    triangleData[i].v2 = buf.readUInt16Fast();
  }
#if 1
  std::printf("BSTriShape:\n");
  if (nameID >= 0)
    std::printf("  Name: %3d\n", nameID);
  if (extraData.size() > 0)
  {
    std::printf("  Extra data:\n");
    for (size_t i = 0; i < extraData.size(); i++)
      std::printf("    %3u\n", extraData[i]);
  }
  if (controller >= 0)
    std::printf("  Controller: %3d\n", controller);
  std::printf("  Flags: 0x%08X\n", flags);
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
                vertexData[i].u, vertexData[i].v);
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

void NIFFile::readString(std::string& s, size_t stringLengthSize)
{
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

void NIFFile::loadNIFFile()
{
  if (fileBufSize < 57 ||
      std::memcmp(fileBuf, "Gamebryo File Format, Version ", 30) != 0)
  {
    throw errorMessage("invalid NIF file header");
  }
  filePos = 39;
  if (readUInt32Fast() != 0x14020007)
    throw errorMessage("unsupported NIF file version");
  if (readUInt8Fast() != 0x01)
    throw errorMessage("unsupported NIF file endianness");
  if (readUInt32Fast() != 0x0000000C)
    throw errorMessage("unsupported NIF file version");
  blockCnt = readUInt32Fast();
  bsVersion = readUInt32Fast();
  readString(authorName, 1);
  if (bsVersion >= 0x90)
    (void) readUInt32();
  readString(processScriptName, 1);
  readString(exportScriptName, 1);
  std::string tmp;
  if (bsVersion >= 0x80 && bsVersion < 0x90)
    readString(tmp, 1);
  for (size_t n = readUInt16(); n--; )
  {
    readString(tmp, 4);
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
    readString(tmp, 4);
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
#if 1
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
  }

  blocks.resize(blockCnt, (NIFBlock *) 0);
  try
  {
    for (size_t i = 0; i < blockCnt; i++)
    {
      FileBuffer  tmpBuf(fileBuf + blockOffsets[i], blockSizes[i]);
      if (blockTypes[i] == BlkTypeNiNode)
        blocks[i] = new NIFBlkNiNode(tmpBuf, bsVersion);
      else if (blockTypes[i] == BlkTypeBSTriShape)
        blocks[i] = new NIFBlkBSTriShape(tmpBuf, bsVersion);
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

