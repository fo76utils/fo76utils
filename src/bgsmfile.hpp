
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
  // texturePaths[0] = diffuse
  // texturePaths[1] = normal
  // texturePaths[2] = Fallout 4 _s.dds
  // texturePaths[4] = environment map
  // texturePaths[6] = Fallout 76 _r.dds
  // texturePaths[7] = Fallout 76 _l.dds
  std::vector< std::string >  texturePaths;
  BGSMFile();
  BGSMFile(const char *fileName);
  BGSMFile(const unsigned char *buf, size_t bufSize);
  BGSMFile(FileBuffer& buf);
  BGSMFile(const BA2File& ba2File, const std::string& fileName);
  void loadBGSMFile(FileBuffer& buf);
  void loadBGSMFile(const BA2File& ba2File, const std::string& fileName);
  void clear();
};

#endif

