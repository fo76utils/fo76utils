
#ifndef NIF_FILE_HPP_INCLUDED
#define NIF_FILE_HPP_INCLUDED

#include "common.hpp"
#include "filebuf.hpp"
#include "ba2file.hpp"
#include "fp32vec4.hpp"
#include "bgsmfile.hpp"

class NIFFile : public FileBuffer
{
 public:
  struct NIFVertex
  {
    float   x;
    float   y;
    float   z;
    std::uint32_t bitangent;            // 0x00ZZYYXX
    std::uint32_t tangent;
    std::uint32_t normal;
    std::uint16_t u;                    // FP16 format
    std::uint16_t v;
    std::uint32_t vertexColor;          // 0xAABBGGRR
    NIFVertex()
      : x(0.0f), y(0.0f), z(0.0f),
        bitangent(0x008080FFU), tangent(0x0080FF80U), normal(0x00FF8080U),
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
    inline void getUV(float& u_f, float& v_f) const
    {
      FloatVector4  tmp(FloatVector4::convertFloat16(
                            std::uint32_t(u) | (std::uint32_t(v) << 16)));
      u_f = tmp[0];
      v_f = tmp[1];
    }
    inline FloatVector4 getBitangent() const
    {
      FloatVector4  tmp(&bitangent);
      tmp *= (1.0f / 127.5f);
      tmp -= 1.0f;
      return tmp;
    }
    inline FloatVector4 getTangent() const
    {
      FloatVector4  tmp(&tangent);
      tmp *= (1.0f / 127.5f);
      tmp -= 1.0f;
      return tmp;
    }
    inline FloatVector4 getNormal() const
    {
      FloatVector4  tmp(&normal);
      tmp *= (1.0f / 127.5f);
      tmp -= 1.0f;
      return tmp;
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
    float   rotateXX, rotateYX, rotateZX;
    float   rotateXY, rotateYY, rotateZY;
    float   rotateXZ, rotateYZ, rotateZZ;
    float   scale;
    NIFVertexTransform();
    NIFVertexTransform(float xyzScale, float rx, float ry, float rz,
                       float offsetX, float offsetY, float offsetZ);
    void readFromBuffer(FileBuffer& buf);
    NIFVertexTransform& operator*=(const NIFVertexTransform& r);
    FloatVector4 rotateXYZ(FloatVector4 v) const;
    FloatVector4 transformXYZ(FloatVector4 v) const;
    void rotateXYZ(float& x, float& y, float& z) const;
    void transformXYZ(float& x, float& y, float& z) const;
  };
  struct NIFBounds
  {
    FloatVector4  boundsMin;
    FloatVector4  boundsMax;
    NIFBounds()
      : boundsMin(1073741824.0f, 1073741824.0f, 1073741824.0f, 1073741824.0f),
        boundsMax(-1073741824.0f, -1073741824.0f, -1073741824.0f,
                  -1073741824.0f)
    {
    }
    NIFBounds(FloatVector4 v)
      : boundsMin(v),
        boundsMax(v)
    {
    }
    inline NIFBounds& operator+=(const NIFVertex& v)
    {
      FloatVector4  tmp(v.x, v.y, v.z, 0.0f);
      boundsMin.minValues(tmp);
      boundsMax.maxValues(tmp);
      return (*this);
    }
    inline NIFBounds& operator+=(FloatVector4 v)
    {
      boundsMin.minValues(v);
      boundsMax.maxValues(v);
      return (*this);
    }
    inline float xMin() const
    {
      return boundsMin[0];
    }
    inline float yMin() const
    {
      return boundsMin[1];
    }
    inline float zMin() const
    {
      return boundsMin[2];
    }
    inline float xMax() const
    {
      return boundsMax[0];
    }
    inline float yMax() const
    {
      return boundsMax[1];
    }
    inline float zMax() const
    {
      return boundsMax[2];
    }
  };
  struct NIFTextureSet
  {
    // texturePaths[-1] = material path
    const std::string **texturePaths;
    NIFTextureSet(size_t n);
    ~NIFTextureSet();
    inline const std::string*& operator[](long n)
    {
      return *(texturePaths + n);
    }
    inline const std::string* const& operator[](long n) const
    {
      return *(texturePaths + n);
    }
  };
  struct NIFTriShape
  {
    unsigned int  vertexCnt;
    unsigned int  triangleCnt;
    const NIFVertex     *vertexData;
    const NIFTriangle   *triangleData;
    // texturePaths[-1] = material path
    // texturePaths[0] = diffuse texture
    // texturePaths[1] = normal map
    // texturePaths[2] = glow texture
    // texturePaths[3] = gradient map
    // texturePaths[4] = environment map
    // texturePaths[5] = Skyrim environment mask (_em)
    // texturePaths[6] = Fallout 4 specular map
    // texturePaths[7] = wrinkles
    // texturePaths[8] = Fallout 76 _r
    // texturePaths[9] = Fallout 76 _l
    const std::string * const * texturePaths;
    NIFVertexTransform  vertexTransform;
    BGSMFile    m;
    const char  *name;
    NIFTriShape();
    void calculateBounds(NIFBounds& b, const NIFVertexTransform *vt = 0) const;
    // materialTexturePaths[-1] = material path
    // alphaProperties = flags | (threshold << 16) | (alpha << 24)
    void setMaterial(const BGSMFile& material,
                     const std::string * const *materialTexturePaths,
                     std::uint32_t alphaProperties = 0U);
    inline bool haveMaterialPath() const
    {
      return (texturePaths && *(texturePaths - 1));
    }
    inline const std::string& materialPath() const
    {
      return *(*(texturePaths - 1));
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
    // bits  4 -  7: offset of vertex coordinates (X, Y, Z, bitangent X) / 4
    // bits  8 - 11: offset of texture coordinates (U, V) / 4
    // bits 12 - 15: offset of texture coordinates 2 / 4
    // bits 16 - 19: offset of vertex normal (X, Y, Z, bitangent Y) / 4
    // bits 20 - 23: offset of vertex tangent (X, Y, Z, bitangent Z) / 4
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
    // 0xFFFFFFFF: effect shader
    // 0: default
    // 1: environment map
    // 2: glow
    unsigned int  shaderType;
    std::vector< unsigned int > extraData;
    int     controller;
    int     textureSet;
    unsigned long long  flags;
    BGSMFile  material; // (material.flags & 4) != 0 if this is an effect shader
    NIFTextureSet texturePaths;
    void readEffectShaderProperty(NIFFile& f);
    void readLightingShaderProperty(NIFFile& f);
    NIFBlkBSLightingShaderProperty(NIFFile& f, size_t nxtBlk, int nxtBlkType,
                                   bool isEffect, const BA2File *ba2File);
    virtual ~NIFBlkBSLightingShaderProperty();
    inline const std::string *materialName() const
    {
      return texturePaths[-1];
    }
  };
  struct NIFBlkBSShaderTextureSet : public NIFBlock
  {
    NIFTextureSet texturePaths;
    std::uint16_t texturePathMask;
    NIFBlkBSShaderTextureSet(NIFFile& f);
    virtual ~NIFBlkBSShaderTextureSet();
  };
  struct NIFBlkNiAlphaProperty : public NIFBlock
  {
    std::vector< unsigned int > extraData;
    int     controller;
    std::uint16_t flags;
    unsigned char alphaThreshold;
    NIFBlkNiAlphaProperty(NIFFile& f);
    virtual ~NIFBlkNiAlphaProperty();
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
  const std::string *authorName;
  const std::string *processScriptName;
  const std::string *exportScriptName;
  std::vector< size_t >     blockOffsets;
  std::vector< NIFBlock * > blocks;
  std::vector< const std::string * >  stringTable;
  std::set< std::string >   stringSet;
  std::string   stringBuf;
  std::vector< std::string >  bgsmTexturePaths;
  void readString(size_t stringLengthSize);
  const std::string *storeString(std::string& s);
  inline int readBlockID()
  {
    int     n = readInt32();
    return (n >= 0 && size_t(n) < blocks.size() ? n : -1);
  }
  void loadNIFFile(const BA2File *ba2File);
  void getMesh(std::vector< NIFTriShape >& v, unsigned int blockNum,
               std::vector< unsigned int >& parentBlocks,
               unsigned int switchActive, bool noRootNodeTransform) const;
 public:
  NIFFile(const char *fileName, const BA2File *ba2File = (BA2File *) 0);
  NIFFile(const unsigned char *buf, size_t bufSize,
          const BA2File *ba2File = (BA2File *) 0);
  NIFFile(FileBuffer& buf, const BA2File *ba2File = (BA2File *) 0);
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
  inline const NIFBlkNiAlphaProperty *getAlphaProperty(size_t n) const;
  void getMesh(std::vector< NIFTriShape >& v, unsigned int rootNode = 0U,
               unsigned int switchActive = 0U,
               bool noRootNodeTransform = false) const;
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
  return *authorName;
}

inline const std::string& NIFFile::getProcessScriptName() const
{
  return *processScriptName;
}

inline const std::string& NIFFile::getExportScriptName() const
{
  return *exportScriptName;
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
    return stringTable[blocks[n]->nameID]->c_str();
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

inline const NIFFile::NIFBlkNiAlphaProperty *
    NIFFile::getAlphaProperty(size_t n) const
{
  if (getBaseBlockType(n) != BlkTypeNiAlphaProperty)
    return (NIFBlkNiAlphaProperty *) 0;
  return ((const NIFBlkNiAlphaProperty *) blocks[n]);
}

#endif

