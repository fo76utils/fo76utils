
#ifndef BGSMFILE_HPP_INCLUDED
#define BGSMFILE_HPP_INCLUDED

#include "common.hpp"
#include "filebuf.hpp"
#include "ba2file.hpp"

struct BGSMFile
{
  unsigned char version;                // 2: Fallout 4, 20: Fallout 76
  // 1: tile U, 2: tile V, 4: is decal, 8: two sided, 16: tree
  unsigned char flags;
  unsigned char gradientMapV;           // 255 = 1.0
  unsigned char envMapScale;            // 128 = 1.0
  // bit 0 = blending enabled
  // bits 1-4 = source blend mode
  // bits 5-8 = destination blend mode
  // bit 9 = threshold enabled
  // bits 10-12 = threshold mode (always 4 = greater)
  unsigned short  alphaFlags;
  unsigned char alphaThreshold;
  unsigned char alpha;                  // 128 = 1.0
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
  // texturePaths[6] = Fallout 4 _s.dds, or environment mask (_em.dds)
  // texturePaths[7] = wrinkles
  // texturePaths[8] = Fallout 76 _r.dds
  // texturePaths[9] = Fallout 76 _l.dds
  BGSMFile(std::vector< std::string >& texturePaths, const char *fileName);
  BGSMFile(std::vector< std::string >& texturePaths,
           const unsigned char *buf, size_t bufSize);
  BGSMFile(std::vector< std::string >& texturePaths, FileBuffer& buf);
  BGSMFile(std::vector< std::string >& texturePaths,
           const BA2File& ba2File, const std::string& fileName);
  void loadBGSMFile(std::vector< std::string >& texturePaths, FileBuffer& buf);
  void loadBGSMFile(std::vector< std::string >& texturePaths,
                    const BA2File& ba2File, const std::string& fileName);
  void clear();
  static inline unsigned char calculateAlphaThreshold(
      unsigned short alphaFlags, unsigned char alphaThreshold,
      unsigned char alpha)
  {
    if (!(alphaFlags & 0x0201))
      return 0;
    if (!alpha)
      return 255;
    if (!(alphaFlags & 0x0200))
      alphaThreshold = 127;             // default to emulate blending
    if (alpha == 128)
      return (unsigned char) (int(alphaThreshold) + int(alphaThreshold < 255));
    int     tmp =
        roundFloat(float(int(alphaThreshold) + 1) * 128.0f / float(int(alpha)));
    return (unsigned char) (tmp < 255 ? tmp : 255);
  }
  inline unsigned char calculateAlphaThreshold() const
  {
    return calculateAlphaThreshold(alphaFlags, alphaThreshold, alpha);
  }
};

#endif

