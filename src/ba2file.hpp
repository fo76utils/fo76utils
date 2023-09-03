
#ifndef BA2FILE_HPP_INCLUDED
#define BA2FILE_HPP_INCLUDED

#include "common.hpp"
#include "filebuf.hpp"

class BA2File
{
 public:
  struct FileDeclaration
  {
    const unsigned char *fileData;
    unsigned int  packedSize;           // 0 if the file is uncompressed
    unsigned int  unpackedSize;         // not valid for compressed BSA files
    // 0: BA2 general, 1: BA2 textures, 2: BA2 textures in raw LZ4 format,
    // >= 103: BSA (version + flags, 0x40000000: compressed, 0x0100: full name)
    int           archiveType;
    unsigned int  archiveFile;
    std::uint32_t hashValue;            // 32-bit hash value
    std::int32_t  prv;                  // index of previous file with same hash
    std::string   fileName;
    inline FileDeclaration(const std::string& fName,
                           std::uint32_t h, std::int32_t p);
    inline bool compare(const std::string& fName, std::uint32_t h) const;
  };
 protected:
  enum
  {
    fileDeclBufShift = 12,
    fileDeclBufMask = 0x0FFF,
    nameHashMask = 0x000FFFFF
  };
  std::vector< std::int32_t >   fileMap;        // nameHashMask + 1 elements
  std::vector< std::vector< FileDeclaration > > fileDeclBufs;
  std::vector< FileBuffer * >   archiveFiles;
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
  inline FileDeclaration& getFileDecl(std::uint32_t n)
  {
    return fileDeclBufs[n >> fileDeclBufShift][n & fileDeclBufMask];
  }
  inline const FileDeclaration& getFileDecl(std::uint32_t n) const
  {
    return fileDeclBufs[n >> fileDeclBufShift][n & fileDeclBufMask];
  }
  static inline std::uint32_t hashFunction(const std::string& s);
  FileDeclaration *addPackedFile(const std::string& fileName);
  void loadBA2General(FileBuffer& buf, size_t archiveFile, size_t hdrSize);
  void loadBA2Textures(FileBuffer& buf, size_t archiveFile, size_t hdrSize);
  void loadBSAFile(FileBuffer& buf, size_t archiveFile, int archiveType);
  void loadFile(FileBuffer& buf, size_t archiveFile, const char *fileName);
  void loadArchivesFromDir(const char *pathName);
  void loadArchiveFile(const char *fileName);
  unsigned int getBSAUnpackedSize(const unsigned char*& dataPtr,
                                  const FileDeclaration& fd) const;
 public:
  BA2File(const char *pathName,
          const std::vector< std::string > *includePatterns = 0,
          const std::vector< std::string > *excludePatterns = 0,
          const std::set< std::string > *fileNames = 0);
  BA2File(const std::vector< std::string >& pathNames,
          const std::vector< std::string > *includePatterns = 0,
          const std::vector< std::string > *excludePatterns = 0,
          const std::set< std::string > *fileNames = 0);
  // includePatterns, excludePatterns, and fileNames are tab separated lists
  BA2File(const char *pathName, const char *includePatterns,
          const char *excludePatterns = 0, const char *fileNames = 0);
  virtual ~BA2File();
  void getFileList(std::vector< std::string >& fileList,
                   bool disableSorting = false) const;
  // returns pointer to file information, or NULL if the file is not found
  const FileDeclaration *findFile(const std::string& fileName) const;
  // returns -1 if the file is not found
  long getFileSize(const std::string& fileName, bool packedSize = false) const;
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

