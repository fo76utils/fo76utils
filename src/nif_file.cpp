
#include "common.hpp"
#include "nif_file.hpp"
#include "nifblock.cpp"

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
    children.push_back(f.readUInt32());
    if (children[children.size() - 1] >= f.blocks.size())
      throw errorMessage("invalid child block number in NIF node");
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
      throw errorMessage("invalid vertex or triangle data size in NIF file");
    }
  }
  int     xyzOffs = -1;
  int     uvOffs = -1;
  int     normalsOffs = -1;
  int     vclrOffs = -1;
  bool    useFloat16 = (f.bsVersion >= 0x80 && !(vertexFmtDesc & (1ULL << 54)));
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
    size_t  offs = f.getPosition();
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
      f.setPosition(offs + size_t(xyzOffs));
      if (!useFloat16)
      {
        v.x = f.readFloat();
        v.y = f.readFloat();
        v.z = f.readFloat();
      }
      else
      {
        v.x = NIFFile::NIFVertex::convertFloat16(f.readUInt16Fast());
        v.y = NIFFile::NIFVertex::convertFloat16(f.readUInt16Fast());
        v.z = NIFFile::NIFVertex::convertFloat16(f.readUInt16Fast());
      }
    }
    if (uvOffs >= 0)
    {
      f.setPosition(offs + size_t(uvOffs));
      v.u = f.readUInt16Fast();
      v.v = f.readUInt16Fast();
    }
    if (normalsOffs >= 0)
    {
      f.setPosition(offs + size_t(normalsOffs));
      v.normalX = (float(int(f.readUInt8Fast())) - 127.5f) * (1.0f / 127.5f);
      v.normalY = (float(int(f.readUInt8Fast())) - 127.5f) * (1.0f / 127.5f);
      v.normalZ = (float(int(f.readUInt8Fast())) - 127.5f) * (1.0f / 127.5f);
    }
    if (vclrOffs >= 0)
    {
      f.setPosition(offs + size_t(vclrOffs));
      v.vertexColor = f.readUInt32Fast();
    }
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
      throw errorMessage("invalid triangle data in NIF file");
    }
  }
}

NIFFile::NIFBlkBSTriShape::~NIFBlkBSTriShape()
{
}

NIFFile::NIFBlkBSLightingShaderProperty::NIFBlkBSLightingShaderProperty(
    NIFFile& f, size_t nxtBlk, int nxtBlkType)
  : NIFBlock(NIFFile::BlkTypeBSLightingShaderProperty)
{
  if (f.bsVersion < 0x90)
    shaderType = f.readUInt32();
  else
    shaderType = 0;
  int     n = f.readInt32();
  if (n >= 0 && n < int(f.stringTable.size()))
  {
    nameID = n;
    if (f.bsVersion >= 0x80 && !f.stringTable[n].empty())
    {
      materialName = f.stringTable[n];
      if (std::strncmp(materialName.c_str(), "materials/", 10) != 0)
        materialName.insert(0, "materials/");
    }
  }
  if (f.bsVersion < 0x90)
  {
    for (unsigned int i = f.readUInt32(); i--; )
      extraData.push_back(f.readUInt32());
    controller = f.readBlockID();
    flags = f.readUInt64();
    offsetU = f.readFloat();
    offsetV = f.readFloat();
    scaleU = f.readFloat();
    scaleV = f.readFloat();
    textureSet = f.readBlockID();
  }
  else
  {
    controller = -1;
    flags = 0ULL;
    offsetU = 0.0f;
    offsetV = 0.0f;
    scaleU = 1.0f;
    scaleV = 1.0f;
    textureSet =
        (nxtBlkType == NIFFile::BlkTypeBSShaderTextureSet ? int(nxtBlk) : -1);
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
  texturePaths.resize(f.bsVersion < 0x80 ? 9 : (f.bsVersion < 0x90 ? 10 : 13));
  for (size_t i = 0; i < texturePaths.size(); i++)
  {
    f.readString(texturePaths[i], 4);
    if (texturePaths[i].length() > 0 &&
        std::strncmp(texturePaths[i].c_str(), "textures/", 9) != 0)
    {
      texturePaths[i].insert(0, "textures/");
    }
  }
}

NIFFile::NIFBlkBSShaderTextureSet::~NIFBlkBSShaderTextureSet()
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
  filePos = 40;
  if (readUInt64() != 0x0000000C01140200ULL)    // 20.2.0.x, 1, 12
    throw errorMessage("unsupported NIF file version or endianness");
  blockCnt = readUInt32Fast();
  bsVersion = readUInt32Fast();
  readString(authorName, 1);
  if (bsVersion >= 0x90)
    (void) readUInt32();
  readString(processScriptName, 1);
  readString(exportScriptName, 1);
  std::string tmp;
  std::vector< unsigned char >  blockTypes;
  if (bsVersion >= 0x80 && bsVersion < 0x90)
    readString(tmp, 1);
  {
    std::vector< int >  tmpBlkTypes;
    for (size_t n = readUInt16(); n--; )
    {
      readString(tmp, 4);
      tmpBlkTypes.push_back(stringToBlockType(tmp.c_str()));
    }
    if ((blockCnt * 6ULL + filePos) > fileBufSize)
      throw errorMessage("end of input file");
    blockTypes.resize(blockCnt);
    for (size_t i = 0; i < blockCnt; i++)
    {
      unsigned int  blockType = readUInt16Fast();
      if (blockType >= tmpBlkTypes.size())
        throw errorMessage("invalid block type in NIF file");
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
    readString(tmp, 4);
    stringTable.push_back(tmp);
  }
  if (readUInt32() != 0)
    throw errorMessage("NIFFile: number of groups > 0 is not supported");
  for (size_t i = 0; i <= blockCnt; i++)
  {
    blockOffsets[i] = blockOffsets[i] + filePos;
    if (blockOffsets[i] > fileBufSize)
      throw errorMessage("invalid block size in NIF file");
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
      if (isNodeBlock(blockType))
      {
        blocks[i] = new NIFBlkNiNode(*this);
      }
      else if (isTriShapeBlock(blockType))
      {
        blocks[i] = new NIFBlkBSTriShape(*this);
      }
      else if (blockType == BlkTypeBSLightingShaderProperty)
      {
        int     t = BlkTypeUnknown;
        if ((i + 1) < blockCnt)
          t = blockTypes[i + 1];
        blocks[i] = new NIFBlkBSLightingShaderProperty(*this, i + 1, t);
      }
      else if (blockType == BlkTypeBSShaderTextureSet)
      {
        blocks[i] = new NIFBlkBSShaderTextureSet(*this);
      }
      else
      {
        blocks[i] = new NIFBlock(blockType);
      }
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
}

void NIFFile::getMesh(std::vector< NIFTriShape >& v, unsigned int blockNum,
                      std::vector< unsigned int >& parentBlocks) const
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
      if (!loopFound)
        getMesh(v, n, parentBlocks);
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
  if (b.shaderProperty >= 0)
  {
    size_t  n = size_t(b.shaderProperty);
    if (blocks[n]->type == BlkTypeBSWaterShaderProperty)
    {
      t.isWater = true;
    }
    else if (blocks[n]->type == BlkTypeBSLightingShaderProperty)
    {
      const NIFBlkBSLightingShaderProperty& lsBlock =
          *((const NIFBlkBSLightingShaderProperty *) blocks[n]);
      if (!lsBlock.materialName.empty())
        t.materialPath = &(lsBlock.materialName);
      t.textureOffsetU = lsBlock.offsetU;
      t.textureOffsetV = lsBlock.offsetV;
      t.textureScaleU = lsBlock.scaleU;
      t.textureScaleV = lsBlock.scaleV;
      if (lsBlock.textureSet >= 0)
        n = size_t(lsBlock.textureSet);
    }
    if (blocks[n]->type == BlkTypeBSShaderTextureSet)
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

void NIFFile::getMesh(std::vector< NIFTriShape >& v,
                      unsigned int rootNode) const
{
  v.clear();
  std::vector< unsigned int > parentBlocks;
  getMesh(v, rootNode, parentBlocks);
}

