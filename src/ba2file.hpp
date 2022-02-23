
#ifndef BA2FILE_HPP_INCLUDED
#define BA2FILE_HPP_INCLUDED

#include "common.hpp"
#include "zlib.hpp"
#include "filebuf.hpp"

class BA2File
{
 protected:
  struct FileDeclaration
  {
    const unsigned char *fileData;
    unsigned int  packedSize;
    unsigned int  unpackedSize;
    // 0: BA2 general, 1: BA2 textures, 103-105: BSA
    int           archiveType;
    unsigned int  archiveFile;
  };
  std::vector< FileDeclaration >  fileDecls;
  std::map< std::string, size_t > fileMap;
  ZLibDecompressor  zlibDecompressor;
  std::vector< FileBuffer * > archiveFiles;
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
  size_t allocateFileDecls(size_t fileCnt);
  void addPackedFile(const std::string& fileName, size_t n);
  void loadBA2General(FileBuffer& buf, FileDeclaration& fileDecl);
  void loadBA2Textures(FileBuffer& buf, FileDeclaration& fileDecl);
  void loadBSAFile(FileBuffer& buf, FileDeclaration& fileDecl);
  void loadArchiveFile(const char *fileName);
 public:
  BA2File(const char *pathName);
  virtual ~BA2File();
  void getFileList(std::vector< std::string >& fileList) const;
  long getFileSize(const char *fileName) const;
  void extractFile(std::vector< unsigned char >& buf, const char *fileName);
};

#endif

