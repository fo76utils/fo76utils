
#ifndef BA2FILE_HPP_INCLUDED
#define BA2FILE_HPP_INCLUDED

#include "common.hpp"
#include "zlib.hpp"
#include "filebuf.hpp"

class BA2File : public FileBuffer
{
 protected:
  struct FileDeclaration
  {
    std::string   fileName;
    unsigned long long  offset;
    unsigned int  packedSize;
    unsigned int  unpackedSize;
  };
  std::vector< FileDeclaration >  fileDecls;
  std::map< std::string, const FileDeclaration * >  fileMap;
  ZLibDecompressor  zlibDecompressor;
  // 0: BA2 general, 1: BA2 textures, 103-105: BSA
  int     archiveType;
  static inline char fixNameCharacter(unsigned char c)
  {
    if (c >= 'A' && c <= 'Z')
      return (char(c) + ('a' - 'A'));
    else if (c < 0x20 || c >= 0x7F || c == ':')
      return '_';
    else if (c == '\\')
      return '/';
    return char(c);
  }
 public:
  BA2File(const char *fileName);
  virtual ~BA2File();
  void getFileList(std::vector< std::string >& fileList) const;
  long getFileSize(const char *fileName) const;
  void extractFile(std::vector< unsigned char >& buf, const char *fileName);
};

#endif

