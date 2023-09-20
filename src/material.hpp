
#ifndef MATERIAL_HPP_INCLUDED
#define MATERIAL_HPP_INCLUDED

#include "common.hpp"
#include "fp32vec4.hpp"
#include "ba2file.hpp"
#include "ddstxt.hpp"

// CE2Material (.mat file), object type 1
//   |
//   +--(component type 0x0067)-- CE2Material::Blender, object type 2
//   |      |
//   |      +--(component type 0x006B)-- CE2Material::UVStream, object type 6
//   +--(component type 0x0068)-- CE2Material::Layer, object type 3
//   |      |
//   |      +--(component type 0x006B)-- CE2Material::UVStream, object type 6
//   |      |
//   |      +--(component type 0x0069)-- CE2Material::Material, object type 4
//   |             |
//   |         (component type 0x006A)-- CE2Material::TextureSet, object type 5
//   +--(component type 0x006C)-- LOD material, object type 1

struct CE2MaterialObject
{
  // 0: CE2MaterialObject
  // 1: CE2Material
  // 2: CE2Material::Blender
  // 3: CE2Material::Layer
  // 4: CE2Material::Material
  // 5: CE2Material::TextureSet
  // 6: CE2Material::UVStream
  // For uninitialized objects, ~(base object ID) * 256 is added to type.
  std::int32_t  type;
  // extension (0x0074616D for "mat\0")
  std::uint32_t e;
  // b0 to b31 = base name hash (lower case, no extension)
  // b32 to b63 = directory name hash (lower case with '\\', no trailing '\\')
  std::uint64_t h;
  const std::string *name;
  const CE2MaterialObject *parent;
};

struct CE2Material : public CE2MaterialObject   // object type 1
{
  struct UVStream : public CE2MaterialObject    // object type 6
  {
    // U scale, V scale, U offset, V offset
    FloatVector4  scaleAndOffset;
    // 0 = "Wrap", 1 = "Clamp", 2 = "Mirror", 3 = "Border"
    unsigned char textureAddressMode;
  };
  struct TextureSet : public CE2MaterialObject  // object type 5
  {
    enum
    {
      maxTexturePaths = 21
    };
    std::uint32_t texturePathMask;
    float   floatParam;
    // texturePaths[0] =  albedo (_color.dds)
    // texturePaths[1] =  normal map (_normal.dds)
    // texturePaths[2] =  alpha (_opacity.dds)
    // texturePaths[3] =  roughness (_rough.dds)
    // texturePaths[4] =  metalness (_metal.dds)
    // texturePaths[5] =  ambient occlusion (_ao.dds)
    // texturePaths[6] =  height map (_height.dds)
    // texturePaths[7] =  glow map (_emissive.dds)
    // texturePaths[8] =  translucency (_transmissive.dds)
    // texturePaths[9] =  _curvature.dds
    // texturePaths[10] = _mask.dds
    const std::string *texturePaths[maxTexturePaths];
    // texture replacements are colors in R8G8B8A8 format
    std::uint32_t textureReplacements[maxTexturePaths];
  };
  struct Material : public CE2MaterialObject    // object type 4
  {
    FloatVector4  color;
    // MaterialOverrideColorTypeComponent, 0 = "Multiply", 1 = "Lerp"
    unsigned char colorMode;
    const TextureSet  *textureSet;
  };
  struct Layer : public CE2MaterialObject       // object type 3
  {
    const Material  *material;
    const UVStream  *uvStream;
  };
  struct Blender : public CE2MaterialObject     // object type 2
  {
    enum
    {
      maxFloatParams = 5,
      maxBoolParams = 8
    };
    const UVStream  *uvStream;
    const std::string *texturePath;
    std::uint32_t textureReplacement;
    // parameters set via component types 0x0098 and 0x009A
    float   floatParams[maxFloatParams];
    bool    boolParams[maxBoolParams];
  };
  enum
  {
    textureNumColor = 0,
    textureNumNormal = 1,
    textureNumOpacity = 2,
    textureNumRough = 3,
    textureNumMetal = 4,
    textureNumAO = 5,
    textureNumHeight = 6,
    textureNumEmissive = 7,
    textureNumTransmissive = 8,
    textureNumCurvature = 9,
    textureNumMask = 10
  };
  enum
  {
    // flag values can be combined (bitwise OR) with NIFFile::NIFTriShape::flags
    Flag_IsEffect = 0x00000004,
    Flag_IsDecal = 0x00000008,
    Flag_TwoSided = 0x00000010,
    Flag_IsVegetation = 0x00000020,
    Flag_Glow = 0x00000080,
    Flag_AlphaBlending = 0x00001000,
    Flag_IsWater = 0x00004000,
    Flag_AlphaTesting = 0x00010000,
    Flag_IsTerrain = 0x00020000
  };
  enum
  {
    maxLayers = 6,
    maxBlenders = 5,
    maxLODMaterials = 3
  };
  std::uint32_t flags;
  float   alphaThreshold;
  // 0 = "Lerp", 1 = "Additive", 2 = "Subtractive", 3 = "Multiplicative"
  unsigned char opacityMode1;
  unsigned char opacityMode2;
  std::uint32_t layerMask;
  const Layer   *layers[maxLayers];
  const Blender *blenders[maxBlenders];
  const CE2Material *lodMaterials[maxLODMaterials];
  FloatVector4  emissiveColor;          // R, G, B, overall scale
  inline void setFlags(std::uint32_t m, bool n)
  {
    flags = (flags & ~m) | ((0U - std::uint32_t(n)) & m);
  }
};

// Component types:
//   0x0061: "BSComponentDB::CTName"
//   0x0062: "BSBind::DirectoryComponent"
//   0x0063: "BSBind::ControllerComponent"
//   0x0064: "BSComponentDB2::OuterEdge"
//   0x0067: "BSMaterial::BlenderID"
//   0x0068: "BSMaterial::LayerID"
//   0x0069: "BSMaterial::MaterialID"
//   0x006A: "BSMaterial::TextureSetID"
//   0x006B: "BSMaterial::UVStreamID"
//   0x006C: "BSMaterial::LODMaterialID"
//   0x006D: "BSMaterial::TextureResolutionSetting"
//   0x006E: "BSMaterial::TextureReplacement"
//   0x006F: "BSMaterial::MaterialOverrideColorTypeComponent"
//   0x0070: "BSMaterial::FlipbookComponent"
//   0x0071: "BSMaterial::BlendModeComponent"
//   0x0072: "BSMaterial::ColorChannelTypeComponent"
//   0x0073: "BSMaterial::ShaderRouteComponent"
//   0x0074: "BSMaterial::DetailBlenderSettingsComponent"
//   0x0075: "BSMaterial::AlphaSettingsComponent"
//   0x0076: "BSMaterial::DecalSettingsComponent"
//   0x0077: "BSMaterial::CollisionComponent"
//   0x0078: "BSMaterial::EmissiveSettingsComponent"
//   0x0079: "BSMaterial::TranslucencySettingsComponent"
//   0x007A: "BSMaterial::ShaderModelComponent"
//   0x007B: "BSMaterial::FlowSettingsComponent"
//   0x007C: "BSMaterial::EffectSettingsComponent"
//   0x007D: "BSMaterial::OpacityComponent"
//   0x007E: "BSMaterial::LayeredEmissivityComponent"
//   0x007F: "BSMaterial::HairSettingsComponent"
//   0x0080: "BSMaterial::MouthSettingsComponent"
//   0x0081: "BSMaterial::WaterSettingsComponent"
//   0x0082: "BSMaterial::WaterFoamSettingsComponent"
//   0x0083: "BSMaterial::WaterGrimeSettingsComponent"
//   0x0084: "BSMaterial::ColorRemapSettingsComponent"
//   0x0085: "BSMaterial::StarmapBodyEffectComponent"
//   0x0086: "BSMaterial::TerrainSettingsComponent"
//   0x0087: "BSMaterial::EyeSettingsComponent"
//   0x0088: "BSMaterial::DistortionComponent"
//   0x0089: "BSMaterial::VegetationSettingsComponent"
//   0x008A: "BSMaterial::TerrainTintSettingsComponent"
//   0x008C: "BSMaterial::GlobalLayerDataComponent"
//   0x008E: "BSMaterial::LevelOfDetailSettings"
//   0x008F: "BSMaterial::LayeredEdgeFalloffComponent"
//   0x0090: "BSMaterial::TextureSetKindComponent"
//   0x0091: "BSMaterial::Color"
//   0x0093: "BSMaterial::Scale"
//   0x0094: "BSMaterial::Offset"
//   0x0095: "BSMaterial::Channel"
//   0x0096: "BSMaterial::TextureFile"
//   0x0097: "BSMaterial::MRTextureFile"
//   0x0098: "BSMaterial::MaterialParamFloat"
//   0x0099: "BSMaterial::BlendParamFloat"
//   0x009A: "BSMaterial::ParamBool"
//   0x009C: "BSMaterial::TextureAddressModeComponent"
//   0x009D: "BSMaterial::UVStreamParamBool"

class CE2MaterialDB
{
 protected:
  enum
  {
    objectNameHashMask = 0x0007FFFF,
    stringBufShift = 16,
    stringBufMask = 0xFFFF,
    stringHashMask = 0x001FFFFF
  };
  static const std::uint32_t  crc32Table[256];
  static const char *stringTable[451];
  // objectNameHashMask + 1 elements
  std::vector< CE2MaterialObject * >  objectNameMap;
  std::vector< std::vector< unsigned char > > objectBuffers;
  // STRT chunk offset | (stringTable index << 32)
  std::vector< std::uint64_t >  stringMap;
  // stringHashMask + 1 elements
  std::vector< std::uint32_t >  storedStringParams;
  // stringBuffers[0][0] = ""
  std::vector< std::vector< std::string > > stringBuffers;
  // returns extension of fileName (0x0074616D for ".mat")
  static std::uint32_t calculateHash(std::uint64_t& h,
                                     const std::string& fileName);
  static int findString(const char *s);
  int findString(unsigned int strtOffs) const;
  inline const CE2MaterialObject *findObject(
      const std::vector< CE2MaterialObject * >& t, unsigned int objectID) const;
  void initializeObject(CE2MaterialObject *o,
                        const std::vector< CE2MaterialObject * >& objectTable);
  CE2MaterialObject *allocateObject(
      std::vector< CE2MaterialObject * >& objectTable,
      std::uint32_t objectID, std::uint32_t baseObjID);
  // type = 0: general string (stored without conversion)
  // type = 1: DDS file name (prefix = "textures/", suffix = ".dds")
  const std::string *readStringParam(std::string& stringBuf, FileBuffer& buf,
                                     size_t len, int type);
  // returns the number of components, the position of buf is set to the
  // beginning of the component data
  size_t readTables(const unsigned char*& componentInfoPtr,
                    std::vector< CE2MaterialObject * >& objectTable,
                    FileBuffer& buf);
  // should be called after readTables()
  void readComponents(
      FileBuffer& buf,
      const unsigned char *componentInfoPtr, size_t componentCnt,
      const std::vector< CE2MaterialObject * >& objectTable);
 public:
  CE2MaterialDB();
  CE2MaterialDB(const BA2File& ba2File, const char *fileName = (char *) 0);
  virtual ~CE2MaterialDB();
  void loadCDBFile(FileBuffer& buf);
  void loadCDBFile(const unsigned char *buf, size_t bufSize);
  void loadCDBFile(const BA2File& ba2File, const char *fileName = (char *) 0);
  const CE2Material *findMaterial(const std::string& fileName) const;
#if 0
  static void printTables(const BA2File& ba2File,
                          const char *fileName = (char *) 0);
#endif
};

#endif

