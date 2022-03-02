
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
    void debugPrint() const;
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
  };
 protected:
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
  enum
  {
    BlkTypeNiNode = -1,
    BlkTypeBSFadeNode = -2,
    BlkTypeBSMultiBoundNode = -3,
    BlkTypeBSTriShape = -4,
    BlkTypeBSEffectShaderProperty = -5,
    BlkTypeNiAlphaProperty = -6,
    BlkTypeBSLightingShaderProperty = -7,
    BlkTypeBSShaderTextureSet = -8,
    BlkTypeBSWaterShaderProperty = -9
  };
  std::vector< std::string >  blockTypeStrings;
  std::vector< int >          blockTypes;
  std::vector< unsigned int > blockSizes;
  std::vector< size_t >       blockOffsets;
  std::vector< std::string >  stringTable;
  struct NIFBlock : public FileBuffer
  {
    NIFFile&  f;
    int       type;
    int       nameID;
    NIFBlock(NIFFile& nifFile, int blockType,
             const unsigned char *blockBuf, size_t blockBufSize);
    virtual ~NIFBlock();
  };
  struct NIFBlkNiNode : public NIFBlock
  {
    std::vector< unsigned int > extraData;
    int     controller;
    unsigned int  flags;
    NIFVertexTransform  vertexTransform;
    int     collisionObject;
    std::vector< unsigned int > children;
    NIFBlkNiNode(NIFFile& nifFile,
                 const unsigned char *blockBuf, size_t blockBufSize);
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
    NIFBlkBSTriShape(NIFFile& nifFile,
                     const unsigned char *blockBuf, size_t blockBufSize);
    virtual ~NIFBlkBSTriShape();
  };
  struct NIFBlkBSLightingShaderProperty : public NIFBlock
  {
    unsigned int  type;
    std::string   materialName;
    std::vector< unsigned int > extraData;
    int     controller;
    unsigned long long  flags;
    float   offsetU;
    float   offsetV;
    float   scaleU;
    float   scaleV;
    int     textureSet;
    NIFBlkBSLightingShaderProperty(NIFFile& nifFile, size_t blockNum,
                                   const unsigned char *blockBuf,
                                   size_t blockBufSize);
    virtual ~NIFBlkBSLightingShaderProperty();
  };
  struct NIFBlkBSShaderTextureSet : public NIFBlock
  {
    std::vector< std::string >  texturePaths;
    NIFBlkBSShaderTextureSet(NIFFile& nifFile,
                             const unsigned char *blockBuf,
                             size_t blockBufSize);
    virtual ~NIFBlkBSShaderTextureSet();
  };
  std::vector< NIFBlock * > blocks;
  static void readString(std::string& s,
                         FileBuffer& buf, size_t stringLengthSize);
  void loadNIFFile();
  void getMesh(std::vector< NIFTriShape >& v, unsigned int blockNum,
               std::vector< unsigned int >& parentBlocks) const;
 public:
  NIFFile(const char *fileName);
  NIFFile(const unsigned char *buf, size_t bufSize);
  NIFFile(FileBuffer& buf);
  virtual ~NIFFile();
  inline unsigned int getVersion() const
  {
    return bsVersion;
  }
  inline const std::string& getAuthorName() const
  {
    return authorName;
  }
  void getMesh(std::vector< NIFTriShape >& v) const;
};

#endif

