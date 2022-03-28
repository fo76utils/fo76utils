
#ifndef BGSMFILE_HPP_INCLUDED
#define BGSMFILE_HPP_INCLUDED

#include "common.hpp"
#include "filebuf.hpp"
#include "ba2file.hpp"

struct BGSMFile
{
  unsigned int  version;                // 2: Fallout 4, 20: Fallout 76
  unsigned int  flags;                  // 1: U, 2: V
  float   offsetU;
  float   offsetV;
  float   scaleU;
  float   scaleV;
  float   alpha;
  unsigned short  alphaBlendMode;       // (mode0 << 8) | (mode1 << 4) | mode2
  unsigned char   alphaThreshold;
  bool    alphaThresholdEnabled;
  BGSMFile();
  // texturePaths[0] = diffuse
  // texturePaths[1] = normal
  // texturePaths[2] = glow
  // texturePaths[4] = environment map
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
  void loadBGSMFile(std::vector< std::string >& texturePaths, FileBuffer& buf);
  void loadBGSMFile(std::vector< std::string >& texturePaths,
                    const BA2File& ba2File, const std::string& fileName);
  void clear();
};

#endif

