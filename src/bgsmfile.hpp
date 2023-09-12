
#ifndef BGSMFILE_HPP_INCLUDED
#define BGSMFILE_HPP_INCLUDED

#include "common.hpp"
#include "filebuf.hpp"
#include "ba2file.hpp"

#include <atomic>

struct BGSMFile
{
  enum
  {
    textureNumDiffuse = 0,
    textureNumNormal = 1,
    textureNumGlow = 2,
    textureNumGradient = 3,     // grayscale to color map
    textureNumEnvMap = 4,
    textureNumEnvMask = 5,      // Skyrim/Fallout 4 environment map mask
    textureNumSpecMap = 6,      // Fallout 4 specular (R) and smoothness (G) map
    textureNumWrinkles = 7,
    textureNumReflMap = 8,      // Fallout 76 reflectance map
    textureNumLighting = 9,     // Fallout 76 smoothness (R) and AO (G) map
    texturePathCnt = 10
  };
  class TextureSet
  {
   protected:
    struct TextureSetData
    {
      std::string texturePaths[texturePathCnt];
      std::string materialPath;
      mutable std::atomic< size_t > refCnt;
      inline TextureSetData();
      TextureSetData(const TextureSetData& r);
    };
    mutable TextureSetData  *dataPtr;
    void copyTextureSetData();
    inline void copyTexturePaths();
   public:
    inline TextureSet();
    inline TextureSet(const TextureSet& r);
    inline ~TextureSet();
    TextureSet& operator=(const TextureSet& r);
    void setMaterialPath(const std::string& s);
    void setTexturePath(size_t n, const char *s);
    std::uint32_t readTexturePaths(FileBuffer& f,
                                   unsigned long long texturePathMap);
    inline operator bool() const;
    inline const std::string& materialPath() const;
    inline const std::string& operator[](size_t n) const;
  };
  enum
  {
    Flag_TileU = 0x0001,
    Flag_TileV = 0x0002,
    Flag_IsEffect = 0x0004,
    Flag_IsDecal = 0x0008,
    Flag_TwoSided = 0x0010,
    Flag_IsTree = 0x0020,
    Flag_GrayscaleToAlpha = 0x0040,
    Flag_Glow = 0x0080,
    Flag_NoZWrite = 0x0100,
    Flag_FalloffEnabled = 0x0200,
    Flag_EffectLighting = 0x0400,
    // TriShape flags
    Flag_TSOrdered = 0x0800,    // for BSOrderedNode children, except the first
    Flag_TSAlphaBlending = 0x1000,
    Flag_TSVertexColors = 0x2000,
    Flag_TSWater = 0x4000,
    Flag_TSHidden = 0x8000
  };
  std::uint32_t flags;
  // 0: material data from NIF file, 2: Fallout 4, 20: Fallout 76
  unsigned char version;
  unsigned char alphaThreshold;
  // bit 0 = blending enabled
  // bits 1-4 = source blend mode
  //      0: 1.0
  //      1: 0.0
  //      2: source color
  //      3: 1.0 - source color
  //      4: destination color
  //      5: 1.0 - destination color
  //      6: source alpha
  //      7: 1.0 - source alpha
  //      8: destination alpha
  //      9: 1.0 - destination alpha
  //     10: source alpha saturate
  // bits 5-8 = destination blend mode
  // bit 9 = threshold enabled
  // bits 10-12 = threshold mode (always 4 = greater for material files)
  //      0: always
  //      1: less
  //      2: equal
  //      3: less or equal
  //      4: greater
  //      5: not equal
  //      6: greater or equal
  //      7: never
  std::uint16_t alphaFlags;
  float   alpha;
  float   alphaThresholdFloat;          // calculated from the other variables
  union
  {
    // struct s = shader material,
    // valid if (flags & (Flag_IsEffect | Flag_TSWater)) == 0
    struct
    {
      float   gradientMapV;             // grayscale to color map V offset
      float   unused;                   // padding
      float   envMapScale;              // envMapScale and specularSmoothness
      float   specularSmoothness;       // are shared between all material types
      FloatVector4  specularColor;      // specularColor[3] = specular scale
      FloatVector4  emissiveColor;      // emissiveColor[3] = emissive scale
    }
    s;
    // struct e = effect material, valid if (flags & Flag_IsEffect) != 0
    struct
    {
      float   baseColorScale;
      float   lightingInfluence;
      float   envMapScale;
      float   specularSmoothness;
      FloatVector4  falloffParams;
      FloatVector4  baseColor;
    }
    e;
    // struct w = water material, valid if (flags & Flag_TSWater) != 0
    struct
    {
      float   maxDepth;
      float   unused;
      float   envMapScale;
      float   specularSmoothness;
      // for Fallout 76, shallowColor is the opacity separately for each channel
      FloatVector4  shallowColor;       // shallowColor[3] = alpha at 0 depth
      FloatVector4  deepColor;          // deepColor[3] = alpha at maximum depth
    }
    w;
  };
  float   textureOffsetU;
  float   textureOffsetV;
  float   textureScaleU;
  float   textureScaleV;
  // 11: Oblivion, 34: Fallout 3, 100: Skyrim, 130: Fallout 4, 155: Fallout 76,
  // 172: Starfield
  std::uint32_t nifVersion;
  std::uint32_t texturePathMask;        // bit N = 1 if texture path N is valid
  TextureSet    texturePaths;
  inline void clear();
  BGSMFile();
  BGSMFile(const char *fileName);
  BGSMFile(const unsigned char *buf, size_t bufSize);
  BGSMFile(FileBuffer& buf);
  BGSMFile(const BA2File& ba2File, const std::string& fileName);
 protected:
  void loadBGEMFile(FileBuffer& buf);
 public:
  void loadBGSMFile(FileBuffer& buf);
  void loadBGSMFile(const BA2File& ba2File, const std::string& fileName);
  void updateAlphaProperties();
  // alpha channel of c = 256 - sqrt(maxDepth * 512)
  void setWaterColor(std::uint32_t c, float reflectionLevel);
};

inline BGSMFile::TextureSet::TextureSetData::TextureSetData()
  : refCnt(0)
{
}

inline void BGSMFile::TextureSet::copyTexturePaths()
{
  if (dataPtr->refCnt > 0)
    copyTextureSetData();
}

inline BGSMFile::TextureSet::TextureSet()
  : dataPtr((TextureSetData *) 0)
{
}

inline BGSMFile::TextureSet::TextureSet(const TextureSet& r)
{
  if (r.dataPtr)
    r.dataPtr->refCnt++;
  dataPtr = r.dataPtr;
}

inline BGSMFile::TextureSet::~TextureSet()
{
  if (dataPtr && dataPtr->refCnt-- < 1)
    delete dataPtr;
}

inline BGSMFile::TextureSet::operator bool() const
{
  return bool(dataPtr);
}

inline const std::string& BGSMFile::TextureSet::materialPath() const
{
  return dataPtr->materialPath;
}

inline const std::string& BGSMFile::TextureSet::operator[](size_t n) const
{
  return dataPtr->texturePaths[n];
}

#endif

