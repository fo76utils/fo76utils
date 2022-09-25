
#ifndef BGSMFILE_HPP_INCLUDED
#define BGSMFILE_HPP_INCLUDED

#include "common.hpp"
#include "filebuf.hpp"
#include "ba2file.hpp"

struct BGSMFile
{
  unsigned char version;                // 2: Fallout 4, 20: Fallout 76
  unsigned char gradientMapV;           // 255 = 1.0
  //   1: tile U
  //   2: tile V
  //   4: is effect
  //   8: is decal
  //  16: two sided
  //  32: tree
  // 128: glow map
  std::uint16_t flags;
  // bit 0 = blending enabled
  // bits 1-4 = source blend mode
  // bits 5-8 = destination blend mode
  // bit 9 = threshold enabled
  // bits 10-12 = threshold mode (always 4 = greater)
  std::uint16_t alphaFlags;
  unsigned char alphaThreshold;
  unsigned char alpha;                  // 128 = 1.0
  std::uint32_t specularColor;          // alpha channel = specular scale * 128
  unsigned char specularSmoothness;     // 255 = 1.0, glossiness from 2 to 1024
  unsigned char envMapScale;            // 128 = 1.0
  std::uint16_t texturePathMask;
  float   offsetU;
  float   offsetV;
  float   scaleU;
  float   scaleV;
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
  void clear();
};

#endif

