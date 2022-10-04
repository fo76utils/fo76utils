
#ifndef BGSMFILE_HPP_INCLUDED
#define BGSMFILE_HPP_INCLUDED

#include "common.hpp"
#include "filebuf.hpp"
#include "ba2file.hpp"

struct BGSMFile
{
  unsigned char version;                // 2: Fallout 4, 20: Fallout 76
  unsigned char gradientMapV;           // 255 = 1.0
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
    // TriShape flags
    Flag_TSAlphaBlending = 0x1000,
    Flag_TSVertexColors = 0x2000,
    Flag_TSWater = 0x4000,
    Flag_TSHidden = 0x8000
  };
  std::uint16_t flags;
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
  unsigned char alphaThreshold;
  unsigned char alpha;                  // 128 = 1.0
  std::uint32_t specularColor;          // alpha channel = specular scale * 128
  unsigned char specularSmoothness;     // 255 = 1.0, glossiness from 2 to 1024
  unsigned char envMapScale;            // 128 = 1.0
  std::uint16_t texturePathMask;        // bit N = 1 if texture path N is valid
  float   textureOffsetU;
  float   textureOffsetV;
  float   textureScaleU;
  float   textureScaleV;
  // for shader materials: emissive color (alpha channel = emissive scale * 128)
  // for effect materials: base color and scale * 128
  // for water: deep color, alpha = 256 - sqrt(maxDepth * 8)
  std::uint32_t emissiveColor;
  inline void clear();
  BGSMFile();
  // texturePaths[0] = diffuse
  // texturePaths[1] = normal
  // texturePaths[2] = glow
  // texturePaths[3] = gradient map
  // texturePaths[4] = environment map
  // texturePaths[5] = unused / Skyrim _em.dds
  // texturePaths[6] = Fallout 4 _s.dds
  // texturePaths[7] = wrinkles
  // texturePaths[8] = Fallout 76 _r.dds
  // texturePaths[9] = Fallout 76 _l.dds
  BGSMFile(std::vector< std::string >& texturePaths, const char *fileName);
  BGSMFile(std::vector< std::string >& texturePaths,
           const unsigned char *buf, size_t bufSize);
  BGSMFile(std::vector< std::string >& texturePaths, FileBuffer& buf);
  BGSMFile(std::vector< std::string >& texturePaths,
           const BA2File& ba2File, const std::string& fileName);
  void readTexturePaths(std::vector< std::string >& texturePaths,
                        FileBuffer& buf, unsigned long long texturePathMap);
  void loadBGEMFile(std::vector< std::string >& texturePaths, FileBuffer& buf);
  void loadBGSMFile(std::vector< std::string >& texturePaths, FileBuffer& buf);
  void loadBGSMFile(std::vector< std::string >& texturePaths,
                    const BA2File& ba2File, const std::string& fileName);
  inline bool isAlphaBlending() const
  {
    return ((alphaFlags & 0x001F) == 0x000D);
  }
  inline bool isAlphaTesting() const
  {
    return (!(~alphaFlags & 0x1200));
  }
  inline float getAlphaThreshold() const
  {
    if (BRANCH_LIKELY(!(isAlphaBlending() || isAlphaTesting())))
      return 0.0f;
    if (BRANCH_UNLIKELY(!alpha))
      return 256.0f;
    if (!isAlphaTesting())
      return 0.0f;
    if (BRANCH_UNLIKELY(alphaFlags & 0x0400))
      return (!(alphaFlags & 0x0800) ? 0.0f : 256.0f);
    float   t = float(int(alphaThreshold)) * (128.0f / float(int(alpha)));
    return (t + (!(alphaFlags & 0x0800) ? (1.0f / 512.0f) : (-1.0f / 512.0f)));
  }
};

#endif

