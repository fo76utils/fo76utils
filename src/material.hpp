
#ifndef MATERIAL_HPP_INCLUDED
#define MATERIAL_HPP_INCLUDED

#include "common.hpp"
#include "fp32vec4.hpp"
#include "ba2file.hpp"
#include "ddstxt.hpp"

class CE2Material
{

};

class CE2MaterialDB
{
 protected:
  static const std::uint32_t  crc32Table[256];
  static const char *stringTable[451];
  // STRT chunk offset | (stringTable index << 32)
  std::vector< std::uint64_t >  stringMap;
  static unsigned int hashFunction(const void *p, size_t n);
  static int findString(const std::string& s);
  int findString(unsigned int strtOffs) const;
 public:
  CE2MaterialDB(const BA2File& ba2File, const char *fileName = (char *) 0);
  virtual ~CE2MaterialDB();
#if 0
  static void printTables(const BA2File& ba2File,
                          const char *fileName = (char *) 0);
#endif
};

#endif

