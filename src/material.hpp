
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
  std::string name;
};

struct CE2Material : public CE2MaterialObject
{
  struct Blender : public CE2MaterialObject
  {
    std::uint32_t uvStream;
    std::uint32_t texturePathMask;
    std::string texturePaths[12];
  };
  struct Layer : public CE2MaterialObject
  {
    std::uint32_t material;
    std::uint32_t uvStream;
  };
  struct Material : public CE2MaterialObject
  {
    std::uint32_t textureSet;
  };
  struct TextureSet : public CE2MaterialObject
  {
    std::uint32_t texturePathMask;
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
    std::string texturePaths[12];
  };
  struct UVStream : public CE2MaterialObject
  {
  };
  std::uint32_t layerMask;
  std::uint32_t layers[10];
  std::uint32_t blenders[9];
  std::uint32_t lodMaterials[3];
};

// Component types:
//   0x0061: "BSComponentDB::CTName"
//   0x0062: "BSBind::DirectoryComponent"
//   0x0063: "BSBind::ControllerComponent"
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
 public:
  struct MaterialNameHash
  {
    // b0 to b31 = base name hash (lower case, no extension)
    // b32 to b63 = directory name hash (lower case with '\\', no trailing '\\')
    std::uint64_t h;
    // extension (0x0074616D for "mat\0")
    std::uint32_t e;
    std::uint32_t objectID;
    inline bool operator<(const MaterialNameHash& r) const
    {
      return (h < r.h || (h == r.h && e < r.e));
    }
  };
 protected:
  struct MaterialDBObject
  {
    // object data
    CE2MaterialObject *p;
    // type of object referenced by p:
    //   0: none (NULL)
    //   1: CE2Material
    //   2: CE2Material::Blender
    //   3: CE2Material::Layer
    //   4: CE2Material::Material
    //   5: CE2Material::TextureSet
    //   6: CE2Material::UVStream
    int     type;
    // 0 = none
    unsigned int  parent;
    inline MaterialDBObject()
      : p((CE2MaterialObject *) 0),
        type(0),
        parent(0U)
    {
    }
    ~MaterialDBObject();
  };
  static const std::uint32_t  crc32Table[256];
  static const char *stringTable[451];
  std::vector< MaterialNameHash > objectMap;
  std::vector< MaterialDBObject > objectList;
  // STRT chunk offset | (stringTable index << 32)
  std::vector< std::uint64_t >  stringMap;
 public:
  static void calculateHash(MaterialNameHash& h, const std::string& fileName);
 protected:
  static int findString(const std::string& s);
  int findString(unsigned int strtOffs) const;
  void allocateObject(std::uint32_t objectID, int type);
 public:
  CE2MaterialDB(const BA2File& ba2File, const char *fileName = (char *) 0);
  virtual ~CE2MaterialDB();
  const CE2Material *findMaterial(const std::string& fileName) const;
  inline const CE2Material::Blender *getBlender(std::uint32_t n) const
  {
    if (n >= objectList.size() || objectList[n].type != 2)
      return (CE2Material::Blender *) 0;
    return static_cast< const CE2Material::Blender * >(objectList[n].p);
  }
  inline const CE2Material::Layer *getLayer(std::uint32_t n) const
  {
    if (n >= objectList.size() || objectList[n].type != 3)
      return (CE2Material::Layer *) 0;
    return static_cast< const CE2Material::Layer * >(objectList[n].p);
  }
  inline const CE2Material::Material *getMaterial(std::uint32_t n) const
  {
    if (n >= objectList.size() || objectList[n].type != 4)
      return (CE2Material::Material *) 0;
    return static_cast< const CE2Material::Material * >(objectList[n].p);
  }
  inline const CE2Material::TextureSet *getTextureSet(std::uint32_t n) const
  {
    if (n >= objectList.size() || objectList[n].type != 5)
      return (CE2Material::TextureSet *) 0;
    return static_cast< const CE2Material::TextureSet * >(objectList[n].p);
  }
  inline const CE2Material::UVStream *getUVStream(std::uint32_t n) const
  {
    if (n >= objectList.size() || objectList[n].type != 6)
      return (CE2Material::UVStream *) 0;
    return static_cast< const CE2Material::UVStream * >(objectList[n].p);
  }
#if 0
  static void printTables(const BA2File& ba2File,
                          const char *fileName = (char *) 0);
#endif
};

#endif

