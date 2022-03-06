
#ifndef NIF_FILE_HPP_INCLUDED
#define NIF_FILE_HPP_INCLUDED

#include "common.hpp"
#include "filebuf.hpp"

class NIFFile : public FileBuffer
{
 public:
  struct NIFVertex
  {
    float   x;
    float   y;
    float   z;
    float   normalX;
    float   normalY;
    float   normalZ;
    unsigned short  u;
    unsigned short  v;
    unsigned int    vertexColor;
    static inline float convertFloat16(unsigned short n)
    {
      unsigned char e = (unsigned char) ((n >> 10) & 0x1F);
      if (!e)
        return 0.0f;
      long long m = (long long) ((n & 0x03FF) | 0x0400) << e;
      return (float(!(n & 0x8000) ? m : -m) * (1.0f / 33554432.0f));
    }
    NIFVertex()
      : x(0.0f), y(0.0f), z(0.0f),
        normalX(0.0f), normalY(0.0f), normalZ(1.0f),
        u(0), v(0), vertexColor(0xFFFFFFFFU)
    {
    }
    inline float getU() const
    {
      return convertFloat16(u);
    }
    inline float getV() const
    {
      return convertFloat16(v);
    }
  };
  struct NIFTriangle
  {
    unsigned short  v0;
    unsigned short  v1;
    unsigned short  v2;
  };
  struct NIFVertexTransform
  {
    float   offsX, offsY, offsZ;
    float   rotateXX, rotateXY, rotateXZ;
    float   rotateYX, rotateYY, rotateYZ;
    float   rotateZX, rotateZY, rotateZZ;
    float   scale;
    NIFVertexTransform();
    NIFVertexTransform(float xyzScale, float rx, float ry, float rz,
                       float offsetX, float offsetY, float offsetZ);
    void readFromBuffer(FileBuffer& buf);
    NIFVertexTransform& operator*=(const NIFVertexTransform& r);
    void transformXYZ(float& x, float& y, float& z) const;
    void transformVertex(NIFVertex& v) const;
  };
  struct NIFTriShape
  {
    size_t  vertexCnt;
    size_t  triangleCnt;
    const NIFVertex     *vertexData;
    const NIFTriangle   *triangleData;
    NIFVertexTransform  vertexTransform;
    bool    isWater;
    // 9 for Skyrim, 10 for Fallout 4, 13 for Fallout 76
    unsigned char texturePathCnt;
    // texturePaths[0] = diffuse texture
    // texturePaths[1] = normal map
    const std::string   *texturePaths;
    // BGSM file name for Fallout 4 and 76
    const std::string   *materialPath;
    float   textureOffsetU;
    float   textureOffsetV;
    float   textureScaleU;
    float   textureScaleV;
    const char  *name;
    NIFTriShape()
      : vertexCnt(0), triangleCnt(0),
        vertexData((NIFVertex *) 0), triangleData((NIFTriangle *) 0),
        isWater(false), texturePathCnt(0), texturePaths((std::string *) 0),
        materialPath((std::string *) 0),
        textureOffsetU(0.0f), textureOffsetV(0.0f),
        textureScaleU(1.0f), textureScaleV(1.0f), name("")
    {
    }
  };
  enum
  {
    // the complete list of block types is in nifblock.cpp
    BlkTypeUnknown = 0,
    BlkTypeNiNode = 88,
    BlkTypeBSFadeNode = 17,
    BlkTypeBSMultiBoundNode = 31,
    BlkTypeBSLeafAnimNode = 23,
    BlkTypeNiSwitchNode = 130,
    BlkTypeBSTriShape = 55,
    BlkTypeBSMeshLODTriShape = 28,
    BlkTypeBSEffectShaderProperty = 13,
    BlkTypeNiAlphaProperty = 60,
    BlkTypeBSLightingShaderProperty = 24,
    BlkTypeBSShaderTextureSet = 47,
    BlkTypeBSWaterShaderProperty = 57
  };
  struct NIFBlock
  {
    int       type;
    int       nameID;
    NIFBlock(int blockType);
    virtual ~NIFBlock();
    inline bool isNode() const;
    inline bool isTriShape() const;
  };
  struct NIFBlkNiNode : public NIFBlock
  {
    std::vector< unsigned int > extraData;
    int     controller;
    unsigned int  flags;
    NIFVertexTransform  vertexTransform;
    int     collisionObject;
    std::vector< unsigned int > children;
    NIFBlkNiNode(NIFFile& f);
    virtual ~NIFBlkNiNode();
  };
  struct NIFBlkBSTriShape : public NIFBlock
  {
    std::vector< unsigned int > extraData;
    int     controller;
    unsigned int  flags;
    NIFVertexTransform  vertexTransform;
    int     collisionObject;
    float   boundCenterX, boundCenterY, boundCenterZ, boundRadius;
    // bound min/max for Fallout 76 only
    float   boundMinX, boundMinY, boundMinZ;
    float   boundMaxX, boundMaxY, boundMaxZ;
    int     skinID;
    int     shaderProperty;
    int     alphaProperty;
    // bits  0 -  3: vertex data size / 4
    // bits  4 -  7: offset of vertex coordinates (X, Y, Z) / 4
    // bits  8 - 11: offset of texture coordinates (U, V) / 4
    // bits 12 - 15: offset of texture coordinates 2 / 4
    // bits 16 - 19: offset of vertex normals / 4
    // bits 20 - 23: offset of vertex tangents / 4
    // bits 24 - 27: offset of vertex color / 4
    // bits 44 - 53: set if attribute N - 44 is present
    // bit  54:      set if X, Y, Z are 32-bit floats (always for Skyrim)
    unsigned long long  vertexFmtDesc;
    std::vector< NIFVertex >    vertexData;
    std::vector< NIFTriangle >  triangleData;
    NIFBlkBSTriShape(NIFFile& f);
    virtual ~NIFBlkBSTriShape();
  };
  struct NIFBlkBSLightingShaderProperty : public NIFBlock
  {
    unsigned int  shaderType;
    std::string   materialName;
    std::vector< unsigned int > extraData;
    int     controller;
    unsigned long long  flags;
    float   offsetU;
    float   offsetV;
    float   scaleU;
    float   scaleV;
    int     textureSet;
    NIFBlkBSLightingShaderProperty(NIFFile& f, size_t nxtBlk, int nxtBlkType);
    virtual ~NIFBlkBSLightingShaderProperty();
  };
  struct NIFBlkBSShaderTextureSet : public NIFBlock
  {
    std::vector< std::string >  texturePaths;
    NIFBlkBSShaderTextureSet(NIFFile& f);
    virtual ~NIFBlkBSShaderTextureSet();
  };
 protected:
  static const char *blockTypeStrings[168];
  static const unsigned char blockTypeBaseTable[168];
  static int stringToBlockType(const char *s);
  static inline bool isNodeBlock(int blockType);
  static inline bool isTriShapeBlock(int blockType);
  // file version:
  //      11: Oblivion
  //      34: Fallout 3
  //     100: Skyrim
  //     130: Fallout 4
  //     155: Fallout 76
  unsigned int  bsVersion;
  unsigned int  blockCnt;
  std::string   authorName;
  std::string   processScriptName;
  std::string   exportScriptName;
  std::vector< size_t >       blockOffsets;
  std::vector< std::string >  stringTable;
  std::vector< NIFBlock * >   blocks;
  void readString(std::string& s, size_t stringLengthSize);
  inline int readBlockID()
  {
    int     n = readInt32();
    return (n >= 0 && size_t(n) < blocks.size() ? n : -1);
  }
  void loadNIFFile();
  void getMesh(std::vector< NIFTriShape >& v, unsigned int blockNum,
               std::vector< unsigned int >& parentBlocks,
               unsigned int switchActive) const;
 public:
  NIFFile(const char *fileName);
  NIFFile(const unsigned char *buf, size_t bufSize);
  NIFFile(FileBuffer& buf);
  virtual ~NIFFile();
  inline unsigned int getVersion() const;
  inline const std::string& getAuthorName() const;
  inline const std::string& getProcessScriptName() const;
  inline const std::string& getExportScriptName() const;
  inline size_t getBlockCount() const;
  inline int getBlockType(size_t n) const;
  inline int getBaseBlockType(size_t n) const;
  inline const char *getBlockTypeAsString(size_t n) const;
  inline const char *getBlockName(size_t n) const;
  inline size_t getBlockOffset(size_t n) const;
  inline size_t getBlockSize(size_t n) const;
  inline const std::vector< unsigned int > *getNodeChildren(size_t n) const;
  // these functions return NULL if the block is not of the requested type
  inline const NIFBlkNiNode *getNode(size_t n) const;
  inline const NIFBlkBSTriShape *getTriShape(size_t n) const;
  inline const NIFBlkBSLightingShaderProperty *
      getLightingShaderProperty(size_t n) const;
  inline const NIFBlkBSShaderTextureSet *getShaderTextureSet(size_t n) const;
  void getMesh(std::vector< NIFTriShape >& v, unsigned int rootNode = 0U,
               unsigned int switchActive = 0U) const;
};

inline bool NIFFile::isNodeBlock(int blockType)
{
  return (blockTypeBaseTable[blockType] == (unsigned char) BlkTypeNiNode);
}

inline bool NIFFile::isTriShapeBlock(int blockType)
{
  return (blockTypeBaseTable[blockType] == (unsigned char) BlkTypeBSTriShape);
}

inline bool NIFFile::NIFBlock::isNode() const
{
  return NIFFile::isNodeBlock(type);
}

inline bool NIFFile::NIFBlock::isTriShape() const
{
  return NIFFile::isTriShapeBlock(type);
}

inline unsigned int NIFFile::getVersion() const
{
  return bsVersion;
}

inline const std::string& NIFFile::getAuthorName() const
{
  return authorName;
}

inline const std::string& NIFFile::getProcessScriptName() const
{
  return processScriptName;
}

inline const std::string& NIFFile::getExportScriptName() const
{
  return exportScriptName;
}

inline size_t NIFFile::getBlockCount() const
{
  return blockCnt;
}

inline int NIFFile::getBlockType(size_t n) const
{
  return blocks[n]->type;
}

inline int NIFFile::getBaseBlockType(size_t n) const
{
  return int(blockTypeBaseTable[blocks[n]->type]);
}

inline const char * NIFFile::getBlockTypeAsString(size_t n) const
{
  return blockTypeStrings[blocks[n]->type];
}

inline const char * NIFFile::getBlockName(size_t n) const
{
  if (blocks[n]->nameID >= 0)
    return stringTable[blocks[n]->nameID].c_str();
  return "";
}

inline size_t NIFFile::getBlockOffset(size_t n) const
{
  return blockOffsets[n];
}

inline size_t NIFFile::getBlockSize(size_t n) const
{
  return (blockOffsets[n + 1] - blockOffsets[n]);
}

inline const std::vector< unsigned int > *
    NIFFile::getNodeChildren(size_t n) const
{
  if (!blocks[n]->isNode())
    return (std::vector< unsigned int > *) 0;
  return &(((const NIFBlkNiNode *) blocks[n])->children);
}

inline const NIFFile::NIFBlkNiNode * NIFFile::getNode(size_t n) const
{
  if (!blocks[n]->isNode())
    return (NIFBlkNiNode *) 0;
  return ((const NIFBlkNiNode *) blocks[n]);
}

inline const NIFFile::NIFBlkBSTriShape * NIFFile::getTriShape(size_t n) const
{
  if (!blocks[n]->isTriShape())
    return (NIFBlkBSTriShape *) 0;
  return ((const NIFBlkBSTriShape *) blocks[n]);
}

inline const NIFFile::NIFBlkBSLightingShaderProperty *
    NIFFile::getLightingShaderProperty(size_t n) const
{
  if (getBaseBlockType(n) != BlkTypeBSLightingShaderProperty)
    return (NIFBlkBSLightingShaderProperty *) 0;
  return ((const NIFBlkBSLightingShaderProperty *) blocks[n]);
}

inline const NIFFile::NIFBlkBSShaderTextureSet *
    NIFFile::getShaderTextureSet(size_t n) const
{
  if (getBaseBlockType(n) != BlkTypeBSShaderTextureSet)
    return (NIFBlkBSShaderTextureSet *) 0;
  return ((const NIFBlkBSShaderTextureSet *) blocks[n]);
}

#endif

