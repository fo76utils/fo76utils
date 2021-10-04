
#ifndef STRINGDB_HPP_INCLUDED
#define STRINGDB_HPP_INCLUDED

#include "common.hpp"
#include "filebuf.hpp"

class StringDB : public FileBuffer
{
 protected:
  std::map< unsigned int, std::string > strings;
 public:
  StringDB();
  virtual ~StringDB();
  void clear();
  bool loadFile(const char *fileName, const char *stringsPrefix);
  bool findString(std::string& s, unsigned int id) const;
  std::string operator[](size_t id) const;
};

#endif

