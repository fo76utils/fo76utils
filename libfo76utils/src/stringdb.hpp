
#ifndef STRINGDB_HPP_INCLUDED
#define STRINGDB_HPP_INCLUDED

#include "common.hpp"
#include "filebuf.hpp"
#include "ba2file.hpp"

class StringDB : public FileBuffer
{
 protected:
  std::map< unsigned int, std::string > strings;
  static bool archiveFilterFunction(void *p, const std::string_view& s);
 public:
  StringDB();
  virtual ~StringDB();
  void clear();
  bool loadFile(const BA2File& ba2File, const char *stringsPrefix);
  bool loadFile(const char *archivePath, const char *stringsPrefix);
  bool findString(std::string& s, unsigned int id) const;
  std::string operator[](size_t id) const;
};

#endif

