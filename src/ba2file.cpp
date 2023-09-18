
#include "common.hpp"
#include "zlib.hpp"
#include "ba2file.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

inline BA2File::FileDeclaration::FileDeclaration(
    const std::string& fName, std::uint32_t h, std::int32_t p)
  : fileData((unsigned char *) 0),
    packedSize(0U),
    unpackedSize(0U),
    archiveType(0),
    archiveFile(0U),
    hashValue(h),
    prv(p),
    fileName(fName)
{
}

inline bool BA2File::FileDeclaration::compare(
    const std::string& fName, std::uint32_t h) const
{
  return (hashValue == h && fileName == fName);
}

inline std::uint32_t BA2File::hashFunction(const std::string& s)
{
  std::uint64_t h = 0xFFFFFFFFU;
  size_t  n = s.length();
  const std::uint8_t  *p = reinterpret_cast< const std::uint8_t * >(s.c_str());
  for ( ; n >= 8; n = n - 8, p = p + 8)
    hashFunctionUInt64(h, FileBuffer::readUInt64Fast(p));
  if (n)
  {
    std::uint64_t m = 0U;
    if (n & 1)
      m = p[n & 6];
    if (n & 2)
      m = (m << 16) | FileBuffer::readUInt16Fast(p + (n & 4));
    if (n & 4)
      m = (m << 32) | FileBuffer::readUInt32Fast(p);
    hashFunctionUInt64(h, m);
  }
  return std::uint32_t(h & 0xFFFFFFFFU);
}

BA2File::FileDeclaration * BA2File::addPackedFile(const std::string& fileName)
{
  bool    nameMatches = false;
  if (fileNamesPtr && fileNamesPtr->begin() != fileNamesPtr->end())
  {
    nameMatches = (fileNamesPtr->find(fileName) != fileNamesPtr->end());
  }
  if (includePatternsPtr && includePatternsPtr->size() > 0 && !nameMatches)
  {
    const std::vector< std::string >& includePatterns = *includePatternsPtr;
    for (size_t i = 0; i < includePatterns.size(); i++)
    {
      if (fileName.find(includePatterns[i]) != std::string::npos)
      {
        nameMatches = true;
        break;
      }
    }
  }
  else if (!(fileNamesPtr && fileNamesPtr->begin() != fileNamesPtr->end()))
  {
    nameMatches = true;
  }
  if (!nameMatches)
    return (FileDeclaration *) 0;
  if (excludePatternsPtr && excludePatternsPtr->size() > 0)
  {
    const std::vector< std::string >& excludePatterns = *excludePatternsPtr;
    for (size_t i = 0; i < excludePatterns.size(); i++)
    {
      if (fileName.find(excludePatterns[i]) != std::string::npos)
        return (FileDeclaration *) 0;
    }
  }
  std::uint32_t h = hashFunction(fileName);
  for (std::int32_t n = fileMap[h & nameHashMask]; n >= 0; )
  {
    FileDeclaration&  fd = getFileDecl(std::uint32_t(n));
    if (fd.compare(fileName, h))
      return &fd;
    n = fd.prv;
  }
  if (fileDeclBufs.size() < 1 ||
      fileDeclBufs.back().size() >= (fileDeclBufMask + 1U))
  {
    fileDeclBufs.emplace_back();
    fileDeclBufs.back().reserve(fileDeclBufMask + 1U);
  }
  std::vector< FileDeclaration >& fdBuf = fileDeclBufs.back();
  size_t  n = ((fileDeclBufs.size() - 1) << fileDeclBufShift) | fdBuf.size();
  fdBuf.emplace_back(fileName, h, fileMap[h & nameHashMask]);
  fileMap[h & nameHashMask] = std::int32_t(n);
  return &(fdBuf.back());
}

void BA2File::loadBA2General(
    FileBuffer& buf, size_t archiveFile, size_t hdrSize)
{
  size_t  fileCnt = buf.readUInt32();
  size_t  nameOffs = buf.readUInt64();
  if (nameOffs > buf.size() || (fileCnt * 36ULL + hdrSize) > nameOffs)
    errorMessage("invalid BA2 file header");
  std::vector< FileDeclaration * >  fileDecls(fileCnt, (FileDeclaration *) 0);
  buf.setPosition(nameOffs);
  std::string fileName;
  for (size_t i = 0; i < fileCnt; i++)
  {
    fileName.clear();
    size_t  nameLen = buf.readUInt16();
    if ((buf.getPosition() + nameLen) > buf.size())
      errorMessage("end of input file");
    while (nameLen--)
      fileName += fixNameCharacter(buf.readUInt8Fast());
    fileDecls[i] = addPackedFile(fileName);
  }
  for (size_t i = 0; i < fileCnt; i++)
  {
    if (!fileDecls[i])
      continue;
    FileDeclaration&  fileDecl = *(fileDecls[i]);
    buf.setPosition(i * 36 + hdrSize);
    (void) buf.readUInt32Fast();        // unknown
    (void) buf.readUInt32Fast();        // extension
    (void) buf.readUInt32Fast();        // unknown
    (void) buf.readUInt32Fast();        // flags
    fileDecl.fileData = buf.data() + buf.readUInt64();
    fileDecl.packedSize = buf.readUInt32Fast();
    fileDecl.unpackedSize = buf.readUInt32Fast();
    fileDecl.archiveType = 0;
    fileDecl.archiveFile = (unsigned int) archiveFile;
    (void) buf.readUInt32Fast();        // 0xBAADF00D
  }
}

void BA2File::loadBA2Textures(
    FileBuffer& buf, size_t archiveFile, size_t hdrSize)
{
  size_t  fileCnt = buf.readUInt32();
  size_t  nameOffs = buf.readUInt64();
  if (nameOffs > buf.size() || (fileCnt * 48ULL + hdrSize) > nameOffs)
    errorMessage("invalid BA2 file header");
  std::vector< FileDeclaration * >  fileDecls(fileCnt, (FileDeclaration *) 0);
  buf.setPosition(nameOffs);
  std::string fileName;
  for (size_t i = 0; i < fileCnt; i++)
  {
    fileName.clear();
    size_t  nameLen = buf.readUInt16();
    if ((buf.getPosition() + nameLen) > buf.size())
      errorMessage("end of input file");
    while (nameLen--)
      fileName += fixNameCharacter(buf.readUInt8Fast());
    fileDecls[i] = addPackedFile(fileName);
  }
  buf.setPosition(hdrSize);
  for (size_t i = 0; i < fileCnt; i++)
  {
    if ((buf.getPosition() + 24) > buf.size())
      errorMessage("end of input file");
    (void) buf.readUInt32Fast();        // unknown
    (void) buf.readUInt32Fast();        // extension ("dds\0")
    (void) buf.readUInt32Fast();        // unknown
    (void) buf.readUInt8Fast();         // unknown
    const unsigned char *fileData = buf.data() + buf.getPosition();
    size_t  chunkCnt = buf.readUInt8Fast();
    (void) buf.readUInt16Fast();        // chunk header size
    (void) buf.readUInt16Fast();        // texture width
    (void) buf.readUInt16Fast();        // texture height
    (void) buf.readUInt8Fast();         // number of mipmaps
    (void) buf.readUInt8Fast();         // format
    (void) buf.readUInt16Fast();        // 0x0800
    unsigned int  packedSize = 0;
    unsigned int  unpackedSize = 148;
    if ((buf.getPosition() + (chunkCnt * 24)) > buf.size())
      errorMessage("end of input file");
    for (size_t j = 0; j < chunkCnt; j++)
    {
      (void) buf.readUInt64();          // file offset of chunk data
      packedSize = packedSize + buf.readUInt32Fast();
      unpackedSize = unpackedSize + buf.readUInt32Fast();
      (void) buf.readUInt16Fast();      // start mipmap
      (void) buf.readUInt16Fast();      // end mipmap
      (void) buf.readUInt32Fast();      // 0xBAADF00D
    }
    if (fileDecls[i])
    {
      FileDeclaration&  fileDecl = *(fileDecls[i]);
      fileDecl.fileData = fileData;
      fileDecl.packedSize = packedSize;
      fileDecl.unpackedSize = unpackedSize;
      fileDecl.archiveType = (hdrSize != 36 ? 1 : 2);
      fileDecl.archiveFile = (unsigned int) archiveFile;
    }
  }
}

void BA2File::loadBSAFile(FileBuffer& buf, size_t archiveFile, int archiveType)
{
  unsigned int  flags = buf.readUInt32();
  if (archiveType < 104)
    flags = flags & ~0x0700U;
  if ((flags & ~0x01BCU) != 0x0003)
    errorMessage("unsupported archive file format");
  size_t  folderCnt = buf.readUInt32();
  size_t  fileCnt = buf.readUInt32();   // total number of files
  (void) buf.readUInt32();              // total length of all folder names
  (void) buf.readUInt32();              // total length of all file names
  (void) buf.readUInt16();              // file flags
  (void) buf.readUInt16();              // padding
  // dataSize + (packedFlag << 30) + (dataOffset << 32)
  std::vector< unsigned long long > fileDecls(fileCnt, 0ULL);
  std::vector< unsigned int > folderFileCnts(folderCnt);
  std::vector< std::string >  folderNames(folderCnt);
  for (size_t i = 0; i < folderCnt; i++)
  {
    buf.setPosition(i * (archiveType < 105 ? 16U : 24U) + 36);
    (void) buf.readUInt64();            // hash
    folderFileCnts[i] = buf.readUInt32();
  }
  buf.setPosition(folderCnt * (archiveType < 105 ? 16U : 24U) + 36);
  size_t  n = 0;
  for (size_t i = 0; i < folderCnt; i++)
  {
    std::string folderName;
    for (size_t j = buf.readUInt8(); j-- > 0; )
    {
      unsigned char c = buf.readUInt8();
      if (folderName.empty() && (c == '.' || c == '/' || c == '\\'))
        continue;
      if (c)
        folderName += fixNameCharacter(c);
    }
    if (folderName.length() > 0 && folderName.back() != '/')
      folderName += '/';
    folderNames[i] = folderName;
    for (size_t j = folderFileCnts[i]; j-- > 0; n++)
    {
      (void) buf.readUInt64();          // hash
      unsigned long long  tmp = buf.readUInt64();       // data size, offset
      fileDecls[n] = tmp ^ ((flags & 0x04U) << 28);
    }
  }
  if (n != fileCnt)
    errorMessage("invalid file count in BSA archive");
  n = 0;
  std::string fileName;
  for (size_t i = 0; i < folderCnt; i++)
  {
    for (size_t j = 0; j < folderFileCnts[i]; j++, n++)
    {
      fileName = folderNames[i];
      unsigned char c;
      while ((c = buf.readUInt8()) != '\0')
        fileName += fixNameCharacter(c);
      FileDeclaration *fileDecl = addPackedFile(fileName);
      if (fileDecl)
      {
        fileDecl->fileData = buf.data() + size_t(fileDecls[n] >> 32);
        fileDecl->packedSize = 0;
        fileDecl->unpackedSize = (unsigned int) (fileDecls[n] & 0x7FFFFFFFU);
        fileDecl->archiveType =
            archiveType
            | int((flags & 0x0100) | (fileDecl->unpackedSize & 0x40000000));
        fileDecl->archiveFile = archiveFile;
        if (fileDecl->unpackedSize & 0x40000000)
        {
          fileDecl->packedSize = fileDecl->unpackedSize & 0x3FFFFFFFU;
          fileDecl->unpackedSize = 0;
        }
      }
    }
  }
}

void BA2File::loadFile(FileBuffer& buf, size_t archiveFile,
                       const char *fileName)
{
  std::string fileName2;
  size_t  n = std::strlen(fileName);
  fileName2.reserve(n);
  for (size_t i = 0; i < n; i++)
  {
    char    c = fileName[i];
    if (c >= 'A' && c <= 'Z')
      c = c + ('a' - 'A');
    else if (c == '\\')
      c = '/';
    if (c == '/' && fileName2.length() > 0 && fileName2.back() == '/')
      continue;
    fileName2 += c;
  }
  if ((n = fileName2.rfind("/geometries/")) != std::string::npos ||
      (n = fileName2.rfind("/interface/")) != std::string::npos ||
      (n = fileName2.rfind("/materials/")) != std::string::npos ||
      (n = fileName2.rfind("/meshes/")) != std::string::npos ||
      (n = fileName2.rfind("/sound/")) != std::string::npos ||
      (n = fileName2.rfind("/strings/")) != std::string::npos ||
      (n = fileName2.rfind("/terrain/")) != std::string::npos ||
      (n = fileName2.rfind("/textures/")) != std::string::npos)
  {
    fileName2.erase(0, n + 1);
  }
  while (std::strncmp(fileName2.c_str(), "./", 2) == 0)
    fileName2.erase(0, 2);
  while (std::strncmp(fileName2.c_str(), "../", 3) == 0)
    fileName2.erase(0, 3);
  if (fileName2.empty())
    return;
  FileDeclaration *fileDecl = addPackedFile(fileName2);
  if (!fileDecl)
    return;
  fileDecl->fileData = buf.data();
  fileDecl->packedSize = 0;
  fileDecl->unpackedSize = (unsigned int) buf.size();
  fileDecl->archiveType = 0;
  fileDecl->archiveFile = (unsigned int) archiveFile;
}

void BA2File::loadArchivesFromDir(const char *pathName)
{
  DIR     *d = opendir(pathName);
  if (!d)
    errorMessage("error opening archive directory");
  try
  {
    std::set< std::string > archiveNames1;
    std::set< std::string > archiveNames2;
    std::string dirName(pathName);
    if (dirName.back() != '/' && dirName.back() != '\\')
      dirName += '/';
    std::string baseName;
    std::string fullName;
    struct dirent *e;
    while (bool(e = readdir(d)))
    {
      baseName = e->d_name;
      if (baseName.length() < 1 || baseName == "." || baseName == "..")
        continue;
      fullName = dirName;
      fullName += baseName;
      {
#if defined(_WIN32) || defined(_WIN64)
        struct __stat64 st;
        if (_stat64(fullName.c_str(), &st) == 0 &&
            (st.st_mode & _S_IFMT) == _S_IFDIR)
#else
        struct stat st;
        if (stat(fullName.c_str(), &st) == 0 &&
            (st.st_mode & S_IFMT) == S_IFDIR)
#endif
        {
          archiveNames2.insert(fullName);
          continue;
        }
      }
      for (size_t i = 0; i < baseName.length(); i++)
      {
        if (baseName[i] >= 'A' && baseName[i] <= 'Z')
          baseName[i] = baseName[i] + ('a' - 'A');
      }
      size_t  n = baseName.rfind('.');
      if (n == std::string::npos ||
          ((((baseName.length() - n) - 4) & ~1ULL) &&
           !baseName.ends_with("strings")))
      {
        continue;
      }
      const char  *s = baseName.c_str() + (baseName.length() - 4);
      std::uint32_t fileType = FileBuffer::readUInt32Fast(s);
      switch (fileType)
      {
        case 0x3261622EU:               // ".ba2"
        case 0x6173622EU:               // ".bsa"
        case 0x6474622EU:               // ".btd"
        case 0x6F74622EU:               // ".bto"
        case 0x7274622EU:               // ".btr"
        case 0x7364642EU:               // ".dds"
        case 0x74616D2EU:               // ".mat"
        case 0x66696E2EU:               // ".nif"
        case 0x6D656762U:               // "bgem"
        case 0x6D736762U:               // "bgsm"
        case 0x6873656DU:               // "mesh"
        case 0x73676E69U:               // "ings"
          break;
        default:
          continue;
      }
      if ((baseName.starts_with("oblivion") ||
           baseName.starts_with("fallout") || baseName.starts_with("skyrim") ||
           baseName.starts_with("seventysix") ||
           baseName.starts_with("starfield")) &&
          baseName.find("update") == std::string::npos)
      {
        archiveNames1.insert(fullName);
      }
      else
      {
        archiveNames2.insert(fullName);
      }
    }
    closedir(d);
    d = (DIR *) 0;
    for (std::set< std::string >::iterator i = archiveNames1.begin();
         i != archiveNames1.end(); i++)
    {
      loadArchiveFile(i->c_str());
    }
    for (std::set< std::string >::iterator i = archiveNames2.begin();
         i != archiveNames2.end(); i++)
    {
      loadArchiveFile(i->c_str());
    }
  }
  catch (...)
  {
    if (d)
      closedir(d);
    throw;
  }
}

void BA2File::loadArchiveFile(const char *fileName)
{
  {
    if (BRANCH_UNLIKELY(!fileName || *fileName == '\0'))
    {
      std::string dataPath;
      if (!FileBuffer::getDefaultDataPath(dataPath))
        errorMessage("empty input file name");
      loadArchivesFromDir(dataPath.c_str());
      return;
    }
#if defined(_WIN32) || defined(_WIN64)
    char    c = fileName[std::strlen(fileName) - 1];
    if (c == '/' || c == '\\')
    {
      loadArchivesFromDir(fileName);
      return;
    }
    struct __stat64 st;
    if (_stat64(fileName, &st) != 0)
#else
    struct stat st;
    if (stat(fileName, &st) != 0)
#endif
    {
      errorMessage("error opening archive file or directory");
    }
#if defined(_WIN32) || defined(_WIN64)
    if ((st.st_mode & _S_IFMT) == _S_IFDIR)
#else
    if ((st.st_mode & S_IFMT) == S_IFDIR)
#endif
    {
      loadArchivesFromDir(fileName);
      return;
    }
  }

  if (fileMap.size() < size_t(nameHashMask + 1))
    fileMap.resize(size_t(nameHashMask + 1), std::int32_t(-1));
  FileBuffer  *bufp = new FileBuffer(fileName);
  try
  {
    FileBuffer&   buf = *bufp;
    int           archiveType = -1;
    unsigned int  hdr1 = buf.readUInt32();
    unsigned int  hdr2 = buf.readUInt32();
    unsigned int  hdr3 = buf.readUInt32();
    if (hdr1 == 0x58445442 && hdr2 && hdr2 <= 3U)       // "BTDX", version <= 3
    {
      if (hdr3 == 0x4C524E47)                           // "GNRL"
        archiveType = 0;
      else if (hdr3 == 0x30315844)                      // "DX10"
        archiveType = int(hdr2);
    }
    else if (hdr1 == 0x00415342 && hdr3 == 0x00000024)  // "BSA\0", header size
    {
      if (hdr2 >= 103 && hdr2 <= 105)
        archiveType = int(hdr2);
    }
    switch (archiveType)
    {
      case 0:
        loadBA2General(buf, archiveFiles.size(), (hdr2 == 1U ? 24 : 32));
        break;
      case 1:
        loadBA2Textures(buf, archiveFiles.size(), 24);
        break;
      case 2:
        loadBA2Textures(buf, archiveFiles.size(), 32);
        break;
      case 3:
        loadBA2Textures(buf, archiveFiles.size(), 36);
        break;
      default:
        if (archiveType >= 0)
          loadBSAFile(buf, archiveFiles.size(), archiveType);
        else
          loadFile(buf, archiveFiles.size(), fileName);
        break;
    }
    archiveFiles.push_back(bufp);
  }
  catch (...)
  {
    delete bufp;
    throw;
  }
}

unsigned int BA2File::getBSAUnpackedSize(const unsigned char*& dataPtr,
                                         const FileDeclaration& fd) const
{
  const FileBuffer& buf = *(archiveFiles[fd.archiveFile]);
  size_t  offs = size_t(dataPtr - buf.data());
  if (fd.archiveType & 0x00000100)
    offs = offs + (size_t(buf.readUInt8(offs)) + 1);
  unsigned int  unpackedSize = fd.unpackedSize;
  if (fd.archiveType & 0x40000000)
  {
    unpackedSize = buf.readUInt32(offs);
    offs = offs + 4;
  }
  dataPtr = buf.data() + offs;
  return unpackedSize;
}

BA2File::BA2File(const char *pathName,
                 const std::vector< std::string > *includePatterns,
                 const std::vector< std::string > *excludePatterns,
                 const std::set< std::string > *fileNames)
  : includePatternsPtr(includePatterns),
    excludePatternsPtr(excludePatterns),
    fileNamesPtr(fileNames)
{
  loadArchiveFile(pathName);
}

BA2File::BA2File(const std::vector< std::string >& pathNames,
                 const std::vector< std::string > *includePatterns,
                 const std::vector< std::string > *excludePatterns,
                 const std::set< std::string > *fileNames)
  : includePatternsPtr(includePatterns),
    excludePatternsPtr(excludePatterns),
    fileNamesPtr(fileNames)
{
  for (size_t i = 0; i < pathNames.size(); i++)
    loadArchiveFile(pathNames[i].c_str());
}

BA2File::BA2File(const char *pathName, const char *includePatterns,
                 const char *excludePatterns, const char *fileNames)
  : includePatternsPtr((std::vector< std::string > *) 0),
    excludePatternsPtr((std::vector< std::string > *) 0),
    fileNamesPtr((std::set< std::string > *) 0)
{
  std::vector< std::string >  tmpIncludePatterns;
  std::vector< std::string >  tmpExcludePatterns;
  std::set< std::string > tmpFileNames;
  std::string tmp;
  for (int i = 0; i < 3; i++)
  {
    const char  *s = includePatterns;
    if (i == 1)
      s = excludePatterns;
    else if (i == 2)
      s = fileNames;
    if (!(s && *s))
      continue;
    for ( ; true; s++)
    {
      char    c = *s;
      if (c == '\t' || c == '\0')
      {
        if (!tmp.empty())
        {
          if (i == 0)
            tmpIncludePatterns.push_back(tmp);
          else if (i == 1)
            tmpExcludePatterns.push_back(tmp);
          else
            tmpFileNames.insert(tmp);
          tmp.clear();
        }
        if (!c)
          break;
      }
      else
      {
        tmp += fixNameCharacter((unsigned char) c);
      }
    }
  }
  if (tmpIncludePatterns.size() > 0)
    includePatternsPtr = &tmpIncludePatterns;
  if (tmpExcludePatterns.size() > 0)
    excludePatternsPtr = &tmpExcludePatterns;
  if (tmpFileNames.begin() != tmpFileNames.end())
    fileNamesPtr = &tmpFileNames;
  loadArchiveFile(pathName);
}

BA2File::~BA2File()
{
  for (size_t i = 0; i < archiveFiles.size(); i++)
    delete archiveFiles[i];
}

void BA2File::getFileList(
    std::vector< std::string >& fileList, bool disableSorting) const
{
  fileList.clear();
  for (size_t i = 0; i < fileDeclBufs.size(); i++)
  {
    for (size_t j = 0; j < fileDeclBufs[i].size(); j++)
      fileList.emplace_back(fileDeclBufs[i][j].fileName);
  }
  if (fileList.size() > 1 && !disableSorting)
    std::sort(fileList.begin(), fileList.end());
}

const BA2File::FileDeclaration * BA2File::findFile(
    const std::string& fileName) const
{
  std::uint32_t h = hashFunction(fileName);
  for (std::int32_t n = fileMap[h & nameHashMask]; n >= 0; )
  {
    const FileDeclaration&  fd = getFileDecl(std::uint32_t(n));
    if (fd.compare(fileName, h))
      return &fd;
    n = fd.prv;
  }
  return (FileDeclaration *) 0;
}

long BA2File::getFileSize(const std::string& fileName, bool packedSize) const
{
  const FileDeclaration *fd = findFile(fileName);
  if (!fd)
    return -1L;
  if (packedSize && fd->packedSize)
    return long(fd->packedSize);
  if (fd->archiveType & 0x40000000)     // compressed BSA
  {
    const unsigned char *p = fd->fileData;
    return long(getBSAUnpackedSize(p, *fd));
  }
  return long(fd->unpackedSize);
}

int BA2File::extractBA2Texture(std::vector< unsigned char >& buf,
                               const FileDeclaration& fileDecl,
                               int mipOffset) const
{
  FileBuffer  fileBuf(archiveFiles[fileDecl.archiveFile]->data(),
                      archiveFiles[fileDecl.archiveFile]->size());
  const unsigned char *p = fileDecl.fileData;
  size_t  chunkCnt = p[0];
  unsigned int  width = ((unsigned int) p[6] << 8) | p[5];
  unsigned int  height = ((unsigned int) p[4] << 8) | p[3];
  int     mipCnt = p[7];
  unsigned char dxgiFormat = p[8];
  size_t  offs = size_t(p - fileBuf.data()) + 11;
  buf.resize(148);
  for ( ; chunkCnt-- > 0; offs = offs + 24)
  {
    fileBuf.setPosition(offs);
    size_t  chunkOffset = fileBuf.readUInt64();
    size_t  chunkSizePacked = fileBuf.readUInt32Fast();
    size_t  chunkSizeUnpacked = fileBuf.readUInt32Fast();
    int     chunkMipCnt = fileBuf.readUInt16Fast();
    chunkMipCnt = int(fileBuf.readUInt16Fast()) + 1 - chunkMipCnt;
    if (chunkMipCnt > 0 &&
        (chunkMipCnt < mipOffset || (chunkMipCnt == mipOffset && chunkCnt > 0)))
    {
      do
      {
        mipOffset--;
        width = (width + 1) >> 1;
        height = (height + 1) >> 1;
        mipCnt--;
      }
      while (--chunkMipCnt > 0);
    }
    else
    {
      extractBlock(buf, chunkSizeUnpacked, fileDecl,
                   fileBuf.data() + chunkOffset, chunkSizePacked);
    }
  }
  // write DDS header
  unsigned int  pitch = width;
  bool    compressedTexture = true;
  switch (dxgiFormat)
  {
    case 0x0A:                  // DXGI_FORMAT_R16G16B16A16_FLOAT
    case 0x0B:                  // DXGI_FORMAT_R16G16B16A16_UNORM
      pitch = width << 3;
    case 0x3D:                  // DXGI_FORMAT_R8_UNORM
      compressedTexture = false;
      break;
    case 0x1B:                  // DXGI_FORMAT_R8G8B8A8_TYPELESS
    case 0x1C:                  // DXGI_FORMAT_R8G8B8A8_UNORM
    case 0x1D:                  // DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
    case 0x1F:                  // DXGI_FORMAT_R8G8B8A8_SNORM
    case 0x57:                  // DXGI_FORMAT_B8G8R8A8_UNORM
    case 0x5A:                  // DXGI_FORMAT_B8G8R8A8_TYPELESS
    case 0x5B:                  // DXGI_FORMAT_B8G8R8A8_UNORM_SRGB
      pitch = width << 2;
      compressedTexture = false;
      break;
    case 0x46:                  // DXGI_FORMAT_BC1_TYPELESS
    case 0x47:                  // DXGI_FORMAT_BC1_UNORM
    case 0x48:                  // DXGI_FORMAT_BC1_UNORM_SRGB
    case 0x4F:                  // DXGI_FORMAT_BC4_TYPELESS
    case 0x50:                  // DXGI_FORMAT_BC4_UNORM
    case 0x51:                  // DXGI_FORMAT_BC4_SNORM
      pitch = (((width + 3) >> 2) * ((height + 3) >> 2)) << 3;
      break;
    case 0x49:                  // DXGI_FORMAT_BC2_TYPELESS
    case 0x4A:                  // DXGI_FORMAT_BC2_UNORM
    case 0x4B:                  // DXGI_FORMAT_BC2_UNORM_SRGB
    case 0x4C:                  // DXGI_FORMAT_BC3_TYPELESS
    case 0x4D:                  // DXGI_FORMAT_BC3_UNORM
    case 0x4E:                  // DXGI_FORMAT_BC3_UNORM_SRGB
    case 0x52:                  // DXGI_FORMAT_BC5_TYPELESS
    case 0x53:                  // DXGI_FORMAT_BC5_UNORM
    case 0x54:                  // DXGI_FORMAT_BC5_SNORM
    case 0x5E:                  // DXGI_FORMAT_BC6H_TYPELESS
    case 0x5F:                  // DXGI_FORMAT_BC6H_UF16
    case 0x60:                  // DXGI_FORMAT_BC6H_SF16
    case 0x61:                  // DXGI_FORMAT_BC7_TYPELESS
    case 0x62:                  // DXGI_FORMAT_BC7_UNORM
    case 0x63:                  // DXGI_FORMAT_BC7_UNORM_SRGB
      pitch = (((width + 3) >> 2) * ((height + 3) >> 2)) << 4;
      break;
    default:
      throw FO76UtilsError("unsupported DXGI_FORMAT 0x%02X",
                           (unsigned int) dxgiFormat);
  }
  buf[0] = 0x44;                // 'D'
  buf[1] = 0x44;                // 'D'
  buf[2] = 0x53;                // 'S'
  buf[3] = 0x20;                // ' '
  buf[4] = 124;                 // size of DDS_HEADER
  if (!compressedTexture)
    buf[8] = 0x0F;              // DDSD_PITCH + same flags as below
  else
    buf[8] = 0x07;              // DDSD_CAPS, DDSD_HEIGHT, DDSD_WIDTH
  buf[9] = 0x10;                // DDSD_PIXELFORMAT
  if (!compressedTexture)
    buf[10] = 0x02;             // DDSD_MIPMAPCOUNT
  else
    buf[10] = 0x0A;             // DDSD_MIPMAPCOUNT, DDSD_LINEARSIZE
  buf[12] = (unsigned char) (height & 0xFF);    // height
  buf[13] = (unsigned char) (height >> 8);
  buf[16] = (unsigned char) (width & 0xFF);     // width
  buf[17] = (unsigned char) (width >> 8);
  buf[20] = (unsigned char) (pitch & 0xFF);
  buf[21] = (unsigned char) ((pitch >> 8) & 0xFF);
  buf[22] = (unsigned char) ((pitch >> 16) & 0xFF);
  buf[23] = (unsigned char) ((pitch >> 24) & 0xFF);
  buf[28] = (unsigned char) mipCnt;     // mipmap count
  buf[76] = 32;                 // size of DDS_PIXELFORMAT
  buf[80] = 0x04;               // DDPF_FOURCC
  buf[84] = 0x44;               // 'D'
  buf[85] = 0x58;               // 'X'
  buf[86] = 0x31;               // '1'
  buf[87] = 0x30;               // '0'
  buf[108] = 0x08;              // DDSCAPS_COMPLEX
  buf[109] = 0x10;              // DDSCAPS_TEXTURE
  buf[110] = 0x40;              // DDSCAPS_MIPMAP
  buf[128] = dxgiFormat;        // DXGI_FORMAT
  buf[132] = 3;                 // D3D10_RESOURCE_DIMENSION_TEXTURE2D
  // return the remaining number of mip levels to be skipped
  return mipOffset;
}

void BA2File::extractBlock(
    std::vector< unsigned char >& buf, unsigned int unpackedSize,
    const FileDeclaration& fileDecl,
    const unsigned char *p, unsigned int packedSize) const
{
  size_t  n = buf.size();
  buf.resize(n + unpackedSize);
  const FileBuffer& fileBuf = *(archiveFiles[fileDecl.archiveFile]);
  size_t  offs = size_t(p - fileBuf.data());
  if (!packedSize)
  {
    if (offs >= fileBuf.size() || (offs + unpackedSize) > fileBuf.size())
      errorMessage("invalid packed data offset or size");
    std::memcpy(buf.data() + n, p, unpackedSize);
  }
  else
  {
    if (offs >= fileBuf.size() || (offs + packedSize) > fileBuf.size())
      errorMessage("invalid packed data offset or size");
    if (BRANCH_UNLIKELY(fileDecl.archiveType == 2))
    {
      n = ZLibDecompressor::decompressLZ4Raw(
              buf.data() + n, unpackedSize, p, packedSize);
    }
    else
    {
      n = ZLibDecompressor::decompressData(
              buf.data() + n, unpackedSize, p, packedSize);
    }
    if (n != unpackedSize)
      errorMessage("invalid or corrupt compressed data in archive");
  }
}

void BA2File::extractFile(std::vector< unsigned char >& buf,
                          const std::string& fileName) const
{
  buf.clear();
  const FileDeclaration *fd = findFile(fileName);
  if (!fd)
    throw FO76UtilsError("file %s not found in archive", fileName.c_str());
  const FileDeclaration&  fileDecl = *fd;
  const unsigned char *p = fileDecl.fileData;
  unsigned int  packedSize = fileDecl.packedSize;
  unsigned int  unpackedSize = fileDecl.unpackedSize;
  int     archiveType = fileDecl.archiveType;
  if (archiveType & 0x40000100)         // BSA with compression or full names
  {
    unpackedSize = getBSAUnpackedSize(p, fileDecl);
    packedSize = packedSize - (unsigned int) (p - fileDecl.fileData);
  }
  if (!unpackedSize)
    return;
  buf.reserve(unpackedSize);

  if (archiveType == 1 || archiveType == 2)
  {
    (void) extractBA2Texture(buf, fileDecl);
    return;
  }

  extractBlock(buf, unpackedSize, fileDecl, p, packedSize);
}

int BA2File::extractTexture(std::vector< unsigned char >& buf,
                            const std::string& fileName, int mipOffset) const
{
  buf.clear();
  const FileDeclaration *fd = findFile(fileName);
  if (!fd)
    throw FO76UtilsError("file %s not found in archive", fileName.c_str());
  const FileDeclaration&  fileDecl = *fd;
  const unsigned char *p = fileDecl.fileData;
  unsigned int  packedSize = fileDecl.packedSize;
  unsigned int  unpackedSize = fileDecl.unpackedSize;
  int     archiveType = fileDecl.archiveType;
  if (archiveType & 0x40000100)         // BSA with compression or full names
  {
    unpackedSize = getBSAUnpackedSize(p, fileDecl);
    packedSize = packedSize - (unsigned int) (p - fileDecl.fileData);
  }
  if (!unpackedSize)
    return mipOffset;
  buf.reserve(unpackedSize);

  if (archiveType == 1 || archiveType == 2)
    return extractBA2Texture(buf, fileDecl, mipOffset);

  extractBlock(buf, unpackedSize, fileDecl, p, packedSize);
  return mipOffset;
}

size_t BA2File::extractFile(
    const unsigned char*& fileData, std::vector< unsigned char >& buf,
    const std::string& fileName) const
{
  fileData = (unsigned char *) 0;
  buf.clear();
  const FileDeclaration *fd = findFile(fileName);
  if (!fd)
    throw FO76UtilsError("file %s not found in archive", fileName.c_str());
  const FileDeclaration&  fileDecl = *fd;
  const unsigned char *p = fileDecl.fileData;
  unsigned int  packedSize = fileDecl.packedSize;
  unsigned int  unpackedSize = fileDecl.unpackedSize;
  int     archiveType = fileDecl.archiveType;
  if (archiveType & 0x40000100)         // BSA with compression or full names
  {
    unpackedSize = getBSAUnpackedSize(p, fileDecl);
    packedSize = packedSize - (unsigned int) (p - fileDecl.fileData);
  }
  if (!unpackedSize)
    return 0;
  if (!(packedSize || archiveType == 1 || archiveType == 2))
  {
    const FileBuffer& fileBuf = *(archiveFiles[fileDecl.archiveFile]);
    size_t  offs = size_t(p - fileBuf.data());
    if (offs >= fileBuf.size() || (offs + unpackedSize) > fileBuf.size())
      errorMessage("invalid packed data offset or size");
    fileData = p;
    return unpackedSize;
  }

  buf.reserve(unpackedSize);
  if (archiveType == 1 || archiveType == 2)
    (void) extractBA2Texture(buf, fileDecl);
  else
    extractBlock(buf, unpackedSize, fileDecl, p, packedSize);
  fileData = buf.data();
  return buf.size();
}

