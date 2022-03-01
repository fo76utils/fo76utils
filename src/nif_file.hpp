
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
    float   u;
    float   v;
    unsigned short  textureID;
    unsigned char   normalX;
    unsigned char   normalY;
    unsigned char   normalZ;
    unsigned char   tangentX;
    unsigned char   tangentY;
    unsigned char   tangentZ;
    unsigned int    vertexColor;
    static inline float normalToFloat(unsigned char n)
    {
      return ((float(int(n)) - 127.5f) * (2.0f / 255.0f));
    }
  };
  struct NIFTriangle
  {
    unsigned short  v0;
    unsigned short  v1;
    unsigned short  v2;
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
  struct NIFVertexTransform
  {
    float   offsX, offsY, offsZ;
    float   rotateXX, rotateXY, rotateXZ;
    float   rotateYX, rotateYY, rotateYZ;
    float   rotateZX, rotateZY, rotateZZ;
    float   scale;
    void readFromBuffer(FileBuffer& buf);
    void transformVertex(NIFVertex& v);
    void debugPrint();
  };
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
    float readFloat16();
    NIFBlkBSTriShape(NIFFile& nifFile,
                     const unsigned char *blockBuf, size_t blockBufSize);
    virtual ~NIFBlkBSTriShape();
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
 public:
  NIFFile(const char *fileName);
  NIFFile(const unsigned char *buf, size_t bufSize);
  NIFFile(FileBuffer& buf);
  virtual ~NIFFile();
};

#endif

