
#ifndef BA2FILE_HPP_INCLUDED
#define BA2FILE_HPP_INCLUDED

#include "common.hpp"
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
  std::vector< FileBuffer * > archiveFiles;
  const std::vector< std::string >  *includePatternsPtr;
  const std::vector< std::string >  *excludePatternsPtr;
  const std::set< std::string > *fileNamesPtr;
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
  bool addPackedFile(const std::string& fileName, size_t n);
  void loadBA2General(FileBuffer& buf, FileDeclaration& fileDecl);
  void loadBA2Textures(FileBuffer& buf, FileDeclaration& fileDecl);
  void loadBSAFile(FileBuffer& buf, FileDeclaration& fileDecl);
  void loadArchivesFromDir(const char *pathName);
  void loadArchiveFile(const char *fileName);
 public:
  BA2File(const char *pathName,
          const std::vector< std::string > *includePatterns = 0,
          const std::vector< std::string > *excludePatterns = 0,
          const std::set< std::string > *fileNames = 0);
  BA2File(const std::vector< std::string >& pathNames,
          const std::vector< std::string > *includePatterns = 0,
          const std::vector< std::string > *excludePatterns = 0,
          const std::set< std::string > *fileNames = 0);
  virtual ~BA2File();
  void getFileList(std::vector< std::string >& fileList) const;
  long getFileSize(const std::string& fileName) const;
 protected:
  int extractBA2Texture(std::vector< unsigned char >& buf,
                        const FileDeclaration& fileDecl,
                        int mipOffset = 0) const;
  void extractBlock(std::vector< unsigned char >& buf,
                    unsigned int unpackedSize,
                    const FileDeclaration& fileDecl,
                    const unsigned char *p, unsigned int packedSize) const;
 public:
  void extractFile(std::vector< unsigned char >& buf,
                   const std::string& fileName) const;
  // returns the remaining number of mip levels to be skipped
  int extractTexture(std::vector< unsigned char >& buf,
                     const std::string& fileName, int mipOffset = 0) const;
};

#endif

