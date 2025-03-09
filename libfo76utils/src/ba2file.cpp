
#include "common.hpp"
#include "zlib.hpp"
#include "ba2file.hpp"

#include <new>

#include <sys/types.h>
#include <sys/stat.h>
#if defined(_WIN32) || defined(_WIN64)
#  include <io.h>
#else
#  include <dirent.h>
#endif

inline std::uint64_t BA2File::hashFunction(const std::string_view& s)
{
  size_t  l = s.length();
  std::uint64_t h = hashFunctionUInt32(s.data(), l);
  h = h | (std::uint64_t(l) << 32);
  return h;
}

BA2File::FileInfo * BA2File::addPackedFile(const std::string_view& fileName)
{
  if (fileFilterFunction)
  {
    if (!fileFilterFunction(fileFilterFunctionData, fileName))
      return nullptr;
  }
  std::uint64_t h = hashFunction(fileName);
  size_t  m = fileMapHashMask;
  size_t  n;
  for (n = size_t(h & m); fileMap[n]; n = (n + 1) & m)
  {
    if (fileMap[n]->hashValue == h && fileMap[n]->fileName == fileName)
      return nullptr;
  }
  FileInfo  *fd = fileInfoBufs.allocateObject< FileInfo >();
  fd->fileData = nullptr;
  fd->hashValue = h;
  size_t  nameLen = fileName.length();
  char    *s = fileNameBufs.allocateObjects< char >(nameLen + 1);
  if (nameLen) [[likely]]
    std::memcpy(s, fileName.data(), nameLen);
  (void) new(&(fd->fileName)) std::string_view(s, nameLen);
  fileMap[n] = fd;
  fileMapFileCnt++;
  if ((fileMapFileCnt * 3UL) > (m << 1)) [[unlikely]]
    allocateFileMap();                  // keep load factor below 2/3
  return fd;
}

void BA2File::allocateFileMap()
{
  size_t  m = (fileMapHashMask << 1) | 0x0FFF;
  void    *p = std::calloc(m + 1, sizeof(FileInfo *));
  if (!p)
    throw std::bad_alloc();
  FileInfo  **q = reinterpret_cast< FileInfo ** >(p);
#if !(defined(__i386__) || defined(__x86_64__) || defined(__x86_64))
  for (size_t i = 0; i <= m; i++)
    q[i] = nullptr;
#endif
  size_t  n = fileMapHashMask + size_t(bool(fileMapHashMask));
  for (size_t i = 0; i < n; i++)
  {
    if (!fileMap[i])
      continue;
    size_t  j = size_t(fileMap[i]->hashValue & m);
    while (q[j])
      j = (j + 1) & m;
    q[j] = fileMap[i];
  }
  std::free(fileMap);
  fileMap = q;
  fileMapHashMask = m;
}

bool BA2File::loadBA2General(
    FileBuffer& buf, size_t archiveFile, size_t hdrSize)
{
  size_t  fileCnt = buf.readUInt32();
  size_t  nameOffs = buf.readUInt64();
  if (nameOffs > buf.size() || (fileCnt * 36ULL + hdrSize) > nameOffs)
    errorMessage("invalid BA2 file header");
  std::vector< FileInfo * > fileList(fileCnt);
  buf.setPosition(nameOffs);
  std::string fileName;
  for (size_t i = 0; i < fileCnt; i++)
  {
    size_t  nameLen = buf.readUInt16();
    if ((buf.getPosition() + nameLen) > buf.size())
      errorMessage("end of input file");
    fileName.resize(nameLen);
    for (size_t j = 0; j < nameLen; j++)
      fileName[j] = fixNameCharacter(buf.readUInt8Fast());
    fileList[i] = addPackedFile(fileName);
  }
  bool    haveFiles = false;
  for (size_t i = 0; i < fileCnt; i++)
  {
    if (!fileList[i])
      continue;
    FileInfo& fd = *(fileList[i]);
    const unsigned char *p = buf.data() + (i * 36UL + hdrSize);
    //  0 uint32_t  CRC32 of base name without extension
    //  4 uint32_t  extension
    //  8 uint32_t  CRC32 of directory name
    // 12 uint32_t  flags
    // 16 uint64_t  data offset
    // 24 uint32_t  packed size
    // 28 uint32_t  unpacked size
    // 32 uint32_t  0xBAADF00D
    fd.fileData = buf.data() + FileBuffer::readUInt64Fast(p + 16);
    fd.packedSize = FileBuffer::readUInt32Fast(p + 24);
    fd.unpackedSize = FileBuffer::readUInt32Fast(p + 28);
    fd.archiveType = 0;
    fd.archiveFile = (unsigned int) archiveFile;
    haveFiles = true;
  }
  return haveFiles;
}

bool BA2File::loadBA2Textures(
    FileBuffer& buf, size_t archiveFile, size_t hdrSize)
{
  size_t  fileCnt = buf.readUInt32();
  size_t  nameOffs = buf.readUInt64();
  if (nameOffs > buf.size() || (fileCnt * 48ULL + hdrSize) > nameOffs)
    errorMessage("invalid BA2 file header");
  std::vector< FileInfo * > fileList(fileCnt);
  buf.setPosition(nameOffs);
  std::string fileName;
  for (size_t i = 0; i < fileCnt; i++)
  {
    size_t  nameLen = buf.readUInt16();
    if ((buf.getPosition() + nameLen) > buf.size())
      errorMessage("end of input file");
    fileName.resize(nameLen);
    for (size_t j = 0; j < nameLen; j++)
      fileName[j] = fixNameCharacter(buf.readUInt8Fast());
    fileList[i] = addPackedFile(fileName);
  }
  buf.setPosition(hdrSize);
  bool    haveFiles = false;
  for (size_t i = 0; i < fileCnt; i++)
  {
    if ((buf.getPosition() + 24UL) > buf.size())
      errorMessage("end of input file");
    const unsigned char *p = buf.getReadPtr();
    //  0 uint32_t  CRC32 of base name without extension
    //  4 uint32_t  extension ("dds\0")
    //  8 uint32_t  CRC32 of directory name
    // 12 uint8_t   unknown, usually 0
    // 13 uint8_t   number of chunks (FileInfo::fileData points to this)
    // 14 uint16_t  chunk header size (24)
    // 16 uint16_t  texture height
    // 18 uint16_t  texture width
    // 20 uint8_t   number of mipmaps
    // 21 uint8_t   DXGI format code
    // 22 uint16_t  flags, usually 0x0800, bit 0 is set for cube maps
    const unsigned char *fileData = p + 13;
    size_t  chunkCnt = fileData[0];
    if ((buf.getPosition() + std::uint64_t((chunkCnt + 1) * 24)) > buf.size())
      errorMessage("end of input file");
    buf.setPosition(buf.getPosition() + ((chunkCnt + 1) * 24));
    if (!fileList[i])
      continue;
    FileInfo& fd = *(fileList[i]);
    unsigned int  packedSize = 0;
    unsigned int  unpackedSize = 148;
    for (size_t j = 0; j < chunkCnt; j++)
    {
      p = p + 24;
      //  0 uint64_t  file offset of chunk data
      //  8 uint32_t  packed size
      // 12 uint32_t  unpacked size
      // 16 uint16_t  start mipmap
      // 18 uint16_t  end mipmap
      // 20 uint32_t  0xBAADF00D
      packedSize = packedSize + FileBuffer::readUInt32Fast(p + 8);
      unpackedSize = unpackedSize + FileBuffer::readUInt32Fast(p + 12);
    }
    fd.fileData = fileData;
    fd.packedSize = packedSize;
    fd.unpackedSize = unpackedSize;
    fd.archiveType = (hdrSize != 36 ? 1 : 2);
    fd.archiveFile = (unsigned int) archiveFile;
    haveFiles = true;
  }
  return haveFiles;
}

bool BA2File::loadBSAFile(FileBuffer& buf, size_t archiveFile, int archiveType)
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
  std::vector< unsigned long long > fileList(fileCnt, 0ULL);
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
      fileList[n] = tmp ^ ((flags & 0x04U) << 28);
    }
  }
  if (n != fileCnt)
    errorMessage("invalid file count in BSA archive");
  n = 0;
  std::string fileName;
  bool    haveFiles = false;
  for (size_t i = 0; i < folderCnt; i++)
  {
    for (size_t j = 0; j < folderFileCnts[i]; j++, n++)
    {
      fileName = folderNames[i];
      unsigned char c;
      while ((c = buf.readUInt8()) != '\0')
        fileName += fixNameCharacter(c);
      FileInfo  *fd = addPackedFile(fileName);
      if (fd)
      {
        fd->fileData = buf.data() + size_t(fileList[n] >> 32);
        fd->packedSize = 0;
        fd->unpackedSize = (unsigned int) (fileList[n] & 0x7FFFFFFFU);
        fd->archiveType = archiveType | int((flags & 0x0100)
                          | (fd->unpackedSize & 0x40000000));
        fd->archiveFile = archiveFile;
        if (fd->unpackedSize & 0x40000000)
        {
          fd->packedSize = fd->unpackedSize & 0x3FFFFFFFU;
          fd->unpackedSize = 0;
        }
        haveFiles = true;
      }
    }
  }
  return haveFiles;
}

bool BA2File::loadTES3Archive(FileBuffer& buf, size_t archiveFile)
{
  buf.setPosition(4);
  std::uint64_t fileDataOffs = buf.readUInt32();        // hash table offset
  size_t  fileCnt = buf.readUInt32();   // total number of files
  fileDataOffs = fileDataOffs + (std::uint64_t(fileCnt) << 3) + 12UL;
  // dataSize + (dataOffset << 32), name offset
  std::vector< std::pair< std::uint64_t, std::uint32_t > >  fileList;
  for (size_t i = 0; i < fileCnt; i++)
    fileList.emplace_back(buf.readUInt64(), 0U);
  for (size_t i = 0; i < fileCnt; i++)
    fileList[i].second = buf.readUInt32();
  size_t  nameTableOffs = buf.getPosition();
  std::string fileName;
  bool    haveFiles = false;
  for (size_t i = 0; i < fileCnt; i++)
  {
    fileName.clear();
    size_t  nameOffs = fileList[i].second;
    if ((nameTableOffs + std::uint64_t(nameOffs)) >= buf.size())
      errorMessage("invalid file name offset in Morrowind BSA file");
    buf.setPosition(nameTableOffs + nameOffs);
    unsigned char c;
    while ((c = buf.readUInt8()) != '\0')
      fileName += fixNameCharacter(c);
    FileInfo  *fd = addPackedFile(fileName);
    if (fd)
    {
      std::uint64_t dataOffs = fileDataOffs + (fileList[i].first >> 32);
      std::uint32_t dataSize = std::uint32_t(fileList[i].first & 0xFFFFFFFFU);
      if ((dataOffs + dataSize) > buf.size())
        errorMessage("invalid file data offset in Morrowind BSA file");
      fd->fileData = buf.data() + dataOffs;
      fd->packedSize = 0;
      fd->unpackedSize = dataSize;
      fd->archiveType = 64;
      fd->archiveFile = archiveFile;
      haveFiles = true;
    }
  }
  return haveFiles;
}

void BA2File::loadFile(const char *fileName, size_t nameLen, size_t prefixLen,
                       size_t fileSize)
{
  std::string fileName2;
  fileName2.reserve(nameLen);
  for (size_t i = 0; i < nameLen; i++)
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
  if (prefixLen > 0)
    fileName2.erase(0, prefixLen);
  while (fileName2.starts_with("./"))
    fileName2.erase(0, 2);
  while (fileName2.starts_with("../"))
    fileName2.erase(0, 3);
  if (fileName2.empty())
    return;
  FileInfo  *fd = addPackedFile(fileName2);
  if (!fd)
    return;
  unsigned char *fsPath =
      fileNameBufs.allocateObjects< unsigned char >(nameLen + 1);
  std::memcpy(fsPath, fileName, nameLen + 1);
  fd->fileData = fsPath;
  fd->packedSize = (unsigned int) nameLen;
  fd->unpackedSize = (unsigned int) fileSize;
  fd->archiveType = -0x80000000;
  fd->archiveFile = 0xFFFFFFFFU;
}

void BA2File::loadFile(
    void *bufPtr, unsigned char * (*allocFunc)(void *bufPtr, size_t nBytes),
    const FileInfo& fd) const
{
  const char  *fileName = reinterpret_cast< const char * >(fd.fileData);
  FileBuffer  f(fileName);
  if (f.size() != fd.unpackedSize)
  {
    throw FO76UtilsError("BA2File: unexpected change to size of loose file %s",
                         fileName);
  }
  unsigned char *buf = allocFunc(bufPtr, fd.unpackedSize);
  if (fd.unpackedSize) [[likely]]
    std::memcpy(buf, f.data(), fd.unpackedSize);
}

bool BA2File::checkDataDirName(const char *s, size_t len)
{
  static const char *dataDirNames[14] =
  {
    "geometries", "icons", "interface", "materials", "meshes",
    "particles", "planetdata", "scripts", "shadersfx", "sound",
    "strings", "terrain", "textures", "vis"
  };
  size_t  n0 = 0;
  size_t  n2 = sizeof(dataDirNames) / sizeof(char *);
  while (n2 > n0)
  {
    size_t  n1 = n0 + ((n2 - n0) >> 1);
    const char  *t = dataDirNames[n1];
    int     d = 0;
    for (size_t i = 0; i < len && !d; i++)
      d = int((unsigned char) s[i] | 0x20) - int((unsigned char) t[i]);
    if (!d || n1 == n0)
      return !d;
    if (d < 0)
      n2 = n1;
    else
      n0 = n1;
  }
  return false;
}

size_t BA2File::findPrefixLen(const char *pathName)
{
  if (!pathName) [[unlikely]]
    return 0;
  size_t  n = 0;
  while (pathName[n])
    n++;
  while (n > 0)
  {
    size_t  n2 = n;
#if defined(_WIN32) || defined(_WIN64)
    while (n2 > 0 && pathName[n2 - 1] != '/' && pathName[n2 - 1] != '\\' &&
           !(n2 == 2 && pathName[n2 - 1] == ':'))
#else
    while (n2 > 0 && pathName[n2 - 1] != '/')
#endif
    {
      n2--;
    }
    if (n > n2 && checkDataDirName(pathName + n2, n - n2))
      return n2;
    n = n2 - size_t(n2 > 0);
  }
  return 0;
}

struct ArchiveDirListItem
{
  //   -4: game archive
  //   -3: DLC or update archive
  //   -2: mod archive
  //   -1: sub-directory
  // >= 0: size of loose file
  std::int64_t  fileSize;
  std::string   baseName;
  inline bool operator<(const ArchiveDirListItem& r) const
  {
    std::int64_t  size1 = std::min< std::int64_t >(fileSize, -1);
    std::int64_t  size2 = std::min< std::int64_t >(r.fileSize, -1);
    return (size1 < size2 || (size1 == size2 && baseName < r.baseName));
  }
};

void BA2File::loadArchivesFromDir(const char *pathName, size_t prefixLen)
{
  std::string dirName(pathName);
#if defined(_WIN32) || defined(_WIN64)
  dirName += ((dirName.back() != '/' && dirName.back() != '\\' &&
               !(dirName.length() == 2 && dirName[1] == ':')) ? "\\*" : "*");
  __finddata64_t  e;
  std::intptr_t   d = _findfirst64(dirName.c_str(), &e);
  if (d < 0)
    errorMessage("error opening archive directory");
  dirName.resize(dirName.length() - 1);
#else
  if (dirName.back() != '/')
    dirName += '/';
  DIR     *d = opendir(pathName);
  if (!d)
    errorMessage("error opening archive directory");
#endif
  if (!prefixLen)
    prefixLen = dirName.length();

  try
  {
    std::set< ArchiveDirListItem >  fileList;
    ArchiveDirListItem  f;
    std::string baseNameL;
    std::string fullName(dirName);

#if defined(_WIN32) || defined(_WIN64)
    do
    {
      f.baseName = e.name;
#else
    struct dirent *e;
    while (bool(e = readdir(d)))
    {
      f.baseName = e->d_name;
#endif
      if (f.baseName.empty() || f.baseName == "." || f.baseName == "..")
        continue;
      {
#if defined(_WIN32) || defined(_WIN64)
        f.fileSize = std::int64_t(e.size);
        bool    isDir = bool(e.attrib & _A_SUBDIR);
#else
        fullName.resize(dirName.length());
        fullName += f.baseName;
        struct stat st;
        if (stat(fullName.c_str(), &st) != 0)
          continue;
        f.fileSize = std::int64_t(st.st_size);
        bool    isDir = ((st.st_mode & S_IFMT) == S_IFDIR);
#endif
        if (isDir)
        {
          if (prefixLen < dirName.length() ||
              checkDataDirName(f.baseName.c_str(), f.baseName.length()))
          {
            f.fileSize = -1;
            fileList.insert(f);
          }
          continue;
        }
      }
      if (f.fileSize < 1)               // ignore empty files
        continue;
      size_t  n = f.baseName.rfind('.');
      if (n == std::string::npos || (f.baseName.length() - n) > 10)
        continue;
      std::uint64_t fileType = 0U;
      for (unsigned char b = 48; ++n < f.baseName.length(); b = b - 6)
      {
        unsigned char c = (unsigned char) f.baseName[n];
        if (c < 0x20 || c > 0x7F)
          c = 0xFF;
        c = (c & 0x1F) | ((c & 0x40) >> 1);
        fileType = fileType | (std::uint64_t(c) << b);
      }
      switch (fileType)
      {
        case 0x22852000000000ULL:       // "ba2"
        case 0x22CE1000000000ULL:       // "bsa"
          baseNameL = f.baseName;
          for (char& c : baseNameL)
          {
            if (c >= 'A' && c <= 'Z')
              c = c + ('a' - 'A');
          }
          f.fileSize = -2;
          if (baseNameL == "morrowind.bsa" ||
              baseNameL.starts_with("oblivion") ||
              baseNameL.starts_with("fallout") ||
              baseNameL.starts_with("skyrim") ||
              baseNameL.starts_with("seventysix") ||
              baseNameL.starts_with("starfield"))
          {
            f.fileSize = (baseNameL.find("update") == std::string::npos &&
                          !baseNameL.ends_with("patch.ba2") ? -4 : -3);
          }
          else if (baseNameL.starts_with("dlc"))
          {
            f.fileSize = -3;
          }
          [[fallthrough]];
        case 0x229E5B40000000ULL:       // "bgem"
        case 0x229F3B40000000ULL:       // "bgsm"
        case 0x22B70000000000ULL:       // "bmp"
        case 0x22D24000000000ULL:       // "btd"
        case 0x22D2F000000000ULL:       // "bto"
        case 0x22D32000000000ULL:       // "btr"
        case 0x23922000000000ULL:       // "cdb"
        case 0x24933000000000ULL:       // "dds"
        case 0x24B33D32A6E9F3ULL:       // "dlstrings"
        case 0x28932000000000ULL:       // "hdr"
        case 0x29B33D32A6E9F3ULL:       // "ilstrings"
        case 0x2B980000000000ULL:       // "kf"
        case 0x2D874000000000ULL:       // "mat"
        case 0x2D973A00000000ULL:       // "mesh"
        case 0x2EA66000000000ULL:       // "nif"
        case 0x33D32A6E9F3000ULL:       // "strings"
        case 0x349E1000000000ULL:       // "tga"
          fileList.insert(f);
          break;
      }
    }
#if defined(_WIN32) || defined(_WIN64)
    while (_findnext64(d, &e) >= 0);
    _findclose(d);
    d = -1;
#else
    closedir(d);
    d = nullptr;
#endif
    for (std::set< ArchiveDirListItem >::const_iterator
             i = fileList.end(); i != fileList.begin(); )
    {
      i--;
      fullName.replace(fullName.begin() + dirName.length(), fullName.end(),
                       i->baseName);
      if (i->fileSize >= 0)
      {
        // create list of loose files without opening them
        loadFile(fullName.c_str(), fullName.length(), prefixLen,
                 size_t(i->fileSize));
      }
      else if (i->fileSize == -1)
      {
        loadArchivesFromDir(fullName.c_str(), prefixLen);
      }
      else
      {
        loadArchiveFile(fullName.c_str(), prefixLen);
      }
    }
  }
  catch (...)
  {
#if defined(_WIN32) || defined(_WIN64)
    if (d >= 0)
      _findclose(d);
#else
    if (d)
      closedir(d);
#endif
    throw;
  }
}

void BA2File::loadArchiveFile(const char *fileName, size_t prefixLen)
{
  if (!fileName || *fileName == '\0') [[unlikely]]
  {
    std::string dataPath;
    if (!FileBuffer::getDefaultDataPath(dataPath))
      errorMessage("empty input file name");
    loadArchivesFromDir(dataPath.c_str(), findPrefixLen(dataPath.c_str()));
    return;
  }
  size_t  nameLen = std::strlen(fileName);
  size_t  fileSize;
  {
    if (!prefixLen) [[unlikely]]
      prefixLen = findPrefixLen(fileName);
#if defined(_WIN32) || defined(_WIN64)
    char    c = fileName[nameLen - 1];
    if (c == '/' || c == '\\')
    {
      loadArchivesFromDir(fileName, prefixLen);
      return;
    }
    struct __stat64 st;
    if (_stat64(fileName, &st) != 0)
#else
    struct stat st;
    if (stat(fileName, &st) != 0)
#endif
    {
      throw FO76UtilsError("error opening archive file or directory '%s'",
                           fileName);
    }
#if defined(_WIN32) || defined(_WIN64)
    if ((st.st_mode & _S_IFMT) == _S_IFDIR)
#else
    if ((st.st_mode & S_IFMT) == S_IFDIR)
#endif
    {
      loadArchivesFromDir(fileName, prefixLen);
      return;
    }
    fileSize = size_t(std::max< std::int64_t >(std::int64_t(st.st_size), 0));
  }

  std::uint32_t ext = 0;
  if (nameLen >= 4)
    ext = FileBuffer::readUInt32Fast(fileName + (nameLen - 4)) | 0x20202000U;
  if (!(fileSize >= 12 && (ext == 0x3261622E || ext == 0x6173622E)))
  {                                     // not ".ba2" or ".bsa"
    loadFile(fileName, nameLen, prefixLen, fileSize);
    return;
  }

  FileBuffer  *bufp = new FileBuffer(fileName);
  try
  {
    FileBuffer&   buf = *bufp;
    size_t  archiveFile = archiveFiles.size();
    // loose file: -1, Morrowind BSA: 128, Oblivion+ BSA: 103 to 105,
    // BA2: header size (+ 1 if DX10)
    int           archiveType = -1;
    if (buf.size() >= 12)
    {
      unsigned int  hdr1 = buf.readUInt32Fast();
      unsigned int  hdr2 = buf.readUInt32Fast();
      unsigned int  hdr3 = buf.readUInt32Fast();
      if (hdr1 == 0x58445442 && hdr2 <= 15U && ((hdr2 = (1U << hdr2)) & 0x018E))
      {                                 // "BTDX", version 1, 2, 3, 7 or 8
        if (hdr3 == 0x4C524E47)                         // "GNRL"
          archiveType = (!(hdr2 & 0x0C) ? 24 : 32);
        else if (hdr3 == 0x30315844)                    // "DX10"
          archiveType = (!(hdr2 & 0x0C) ? 25 : (!(hdr2 & 8) ? 33 : 37));
      }
      else if (hdr1 == 0x00415342 && hdr3 == 36)        // "BSA\0", header size
      {
        if (hdr2 >= 103 && hdr2 <= 105)
          archiveType = int(hdr2);
      }
      else if (hdr1 == 0x00000100 && ext == 0x6173622E) // ".bsa"
      {
        archiveType = 128;
      }
    }
    bool    haveFiles;
    switch (archiveType & 0xC1)
    {
      case 0:
        haveFiles = loadBA2General(buf, archiveFile, size_t(archiveType));
        break;
      case 1:
        haveFiles = loadBA2Textures(buf, archiveFile,
                                    size_t(archiveType & 0x3E));
        break;
      case 0x40:
      case 0x41:
        haveFiles = loadBSAFile(buf, archiveFile, archiveType);
        break;
      case 0x80:
        haveFiles = loadTES3Archive(buf, archiveFile);
        break;
      default:
        delete bufp;
        loadFile(fileName, nameLen, prefixLen, fileSize);
        return;
    }
    if (!haveFiles)
    {
      delete bufp;
      return;
    }
    archiveFiles.push_back(bufp);
  }
  catch (...)
  {
    size_t  m = fileMapHashMask;
    size_t  n = archiveFiles.size();
    for (size_t i = 0; i <= m; i++)
    {
      if (fileMap[i] && fileMap[i]->archiveFile == n)
      {
        fileMap[i] = nullptr;
        fileMapFileCnt--;
      }
    }
    delete bufp;
    throw;
  }
}

unsigned int BA2File::getBSAUnpackedSize(const unsigned char*& dataPtr,
                                         const FileInfo& fd) const
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

[[noreturn]] void BA2File::findFileError(const std::string_view& fileName)
{
  std::string s(fileName);
  throw FO76UtilsError("file %s not found in archive", s.c_str());
}

void BA2File::clear()
{
  for (size_t i = 0; i < archiveFiles.size(); i++)
    delete archiveFiles[i];
  std::free(fileMap);
}

BA2File::BA2File()
  : fileMap(nullptr),
    fileMapHashMask(0),
    fileMapFileCnt(0),
    fileFilterFunction(nullptr),
    fileFilterFunctionData(nullptr)
{
  allocateFileMap();
}

BA2File::BA2File(const char *pathName,
                 bool (*fileFilterFunc)(void *p, const std::string_view& s),
                 void *fileFilterFuncData)
  : fileMap(nullptr),
    fileMapHashMask(0),
    fileMapFileCnt(0),
    fileFilterFunction(fileFilterFunc),
    fileFilterFunctionData(fileFilterFuncData)
{
  allocateFileMap();
  try
  {
    loadArchiveFile(pathName, 0);
  }
  catch (...)
  {
    clear();
    throw;
  }
}

void BA2File::loadArchivePath(
    const char *pathName,
    bool (*fileFilterFunc)(void *p, const std::string_view& s),
    void *fileFilterFuncData)
{
  fileFilterFunction = fileFilterFunc;
  fileFilterFunctionData = fileFilterFuncData;
  loadArchiveFile(pathName, 0);
}

BA2File::~BA2File()
{
  clear();
}

void BA2File::getFileList(
    std::vector< std::string_view >& fileList, bool disableSorting,
    bool (*fileFilterFunc)(void *p, const std::string_view& s),
    void *fileFilterFuncData) const
{
  fileList.clear();
  size_t  m = fileMapHashMask;
  if (!fileFilterFunc)
  {
    size_t  n = fileMapFileCnt;
    fileList.resize(n);
    size_t  j = 0;
    for (size_t i = 0; j < n && i <= m; i++)
    {
      if (!fileMap[i])
        continue;
      fileList[j] = fileMap[i]->fileName;
      j++;
    }
  }
  else
  {
    for (size_t i = 0; i <= m; i++)
    {
      if (!fileMap[i])
        continue;
      const std::string_view& s = fileMap[i]->fileName;
      if (fileFilterFunc(fileFilterFuncData, s))
        fileList.push_back(s);
    }
  }
  if (fileList.size() > 1 && !disableSorting)
    std::sort(fileList.begin(), fileList.end());
}

bool BA2File::scanFileList(
    bool (*fileScanFunc)(void *p, const FileInfo& fd),
    void *fileScanFuncData) const
{
  size_t  m = fileMapHashMask;
  for (size_t i = 0; i <= m; i++)
  {
    if (fileMap[i] && fileScanFunc(fileScanFuncData, *(fileMap[i])))
      return true;
  }
  return false;
}

const BA2File::FileInfo * BA2File::findFile(
    const std::string_view& fileName) const
{
  std::uint64_t h = hashFunction(fileName);
  size_t  m = fileMapHashMask;
  for (size_t n = size_t(h & m); fileMap[n]; n = (n + 1) & m)
  {
    if (fileMap[n]->hashValue == h && fileMap[n]->fileName == fileName)
      return fileMap[n];
  }
  return nullptr;
}

std::int64_t BA2File::getFileSize(
    const std::string_view& fileName, bool packedSize) const
{
  const FileInfo  *fd = findFile(fileName);
  if (!fd)
    return -1;
  if (packedSize && fd->packedSize)
  {
    if (fd->archiveType < 0) [[unlikely]]
      return std::int64_t(fd->unpackedSize);
    return std::int64_t(fd->packedSize);
  }
  if (fd->archiveType & 0x40000000)     // compressed BSA
  {
    const unsigned char *p = fd->fileData;
    return std::int64_t(getBSAUnpackedSize(p, *fd));
  }
  return std::int64_t(fd->unpackedSize);
}

int BA2File::extractBA2Texture(
    void *bufPtr, unsigned char * (*allocFunc)(void *bufPtr, size_t nBytes),
    const FileInfo& fd, int mipOffset) const
{
  const unsigned char *p = fd.fileData;
  size_t  chunkCnt = p[0];
  unsigned int  height = FileBuffer::readUInt16Fast(p + 3);
  unsigned int  width = FileBuffer::readUInt16Fast(p + 5);
  int     mipCnt = p[7];
  unsigned char dxgiFormat = p[8];
  bool    isCubeMap = bool(p[9] & 1);
  p = p + 11;
  // skip chunks if mipOffset > 0
  for ( ; chunkCnt > 1; p = p + 24, chunkCnt--)
  {
    int     chunkMipCnt = FileBuffer::readUInt16Fast(p + 16);
    chunkMipCnt = int(FileBuffer::readUInt16Fast(p + 18)) + 1 - chunkMipCnt;
    if (chunkMipCnt <= 0 || chunkMipCnt > mipOffset || chunkMipCnt >= mipCnt)
      break;
    mipOffset -= chunkMipCnt;
    mipCnt -= chunkMipCnt;
    width = std::max< unsigned int >(width >> std::uint8_t(chunkMipCnt), 1U);
    height = std::max< unsigned int >(height >> std::uint8_t(chunkMipCnt), 1U);
  }
  // calculate uncompressed size
  size_t  unpackedSize = 148;
  for (size_t i = 0; i < chunkCnt; i++)
    unpackedSize += size_t(FileBuffer::readUInt32Fast(p + (i * 24 + 12)));
  unsigned char *buf = allocFunc(bufPtr, unpackedSize);

  // write DDS header
  if (!FileBuffer::writeDDSHeader(buf, dxgiFormat,
                                  int(width), int(height), mipCnt, isCubeMap))
  {
    throw FO76UtilsError("unsupported DXGI_FORMAT 0x%02X",
                         (unsigned int) dxgiFormat);
  }
  buf = buf + 148;

  const unsigned char *fileBuf = archiveFiles[fd.archiveFile]->data();
  for ( ; chunkCnt-- > 0; p = p + 24)
  {
    std::uint64_t chunkOffset = FileBuffer::readUInt64Fast(p);
    std::uint32_t chunkSizePacked = FileBuffer::readUInt32Fast(p + 8);
    std::uint32_t chunkSizeUnpacked = FileBuffer::readUInt32Fast(p + 12);
    extractBlock(buf, chunkSizeUnpacked,
                 fd, fileBuf + chunkOffset, chunkSizePacked);
    buf = buf + chunkSizeUnpacked;
  }

  // return the remaining number of mip levels to be skipped
  return mipOffset;
}

void BA2File::extractBlock(
    unsigned char *buf, size_t unpackedSize,
    const FileInfo& fd, const unsigned char *p, size_t packedSize) const
{
  const FileBuffer& fileBuf = *(archiveFiles[fd.archiveFile]);
  std::uint64_t offs = std::uint64_t(p - fileBuf.data());
  if (!packedSize)
  {
    if (offs >= fileBuf.size() || (offs + unpackedSize) > fileBuf.size())
      errorMessage("invalid packed data offset or size");
    std::memcpy(buf, p, unpackedSize);
  }
  else
  {
    if (offs >= fileBuf.size() || (offs + packedSize) > fileBuf.size())
      errorMessage("invalid packed data offset or size");
    size_t  n;
    if (fd.archiveType == 2)
      n = ZLibDecompressor::decompressLZ4Raw(buf, unpackedSize, p, packedSize);
    else
      n = ZLibDecompressor::decompressData(buf, unpackedSize, p, packedSize);
    if (n != unpackedSize)
      errorMessage("invalid or corrupt compressed data in archive");
  }
}

void BA2File::UCharArray::reserve(size_t nBytes)
{
  if (nBytes > capacity)
  {
    void    *tmp = std::realloc(data, nBytes);
    if (!tmp)
      throw std::bad_alloc();
    data = reinterpret_cast< unsigned char * >(tmp);
    capacity = nBytes;
  }
}

unsigned char * BA2File::UCharArray::allocFunc(void *bufPtr, size_t nBytes)
{
  UCharArray  *p = reinterpret_cast< UCharArray * >(bufPtr);
  if (nBytes > p->capacity)
  {
    void    *tmp = std::malloc(nBytes);
    if (!tmp)
      throw std::bad_alloc();
    std::free(p->data);
    p->data = reinterpret_cast< unsigned char * >(tmp);
    p->capacity = nBytes;
  }
  p->size = nBytes;
  return p->data;
}

void BA2File::extractFile(UCharArray& buf,
                          const std::string_view& fileName) const
{
  buf.clear();
  const FileInfo  *fdPtr = findFile(fileName);
  if (!fdPtr) [[unlikely]]
    findFileError(fileName);
  extractFile(&buf, &UCharArray::allocFunc, *fdPtr);
}

int BA2File::extractTexture(
    UCharArray& buf, const std::string_view& fileName, int mipOffset) const
{
  buf.clear();
  const FileInfo  *fdPtr = findFile(fileName);
  if (!fdPtr) [[unlikely]]
    findFileError(fileName);
  const FileInfo& fd = *fdPtr;
  if ((fd.archiveType - 1) & ~1)
  {
    // not a BA2 texture archive
    extractFile(&buf, &UCharArray::allocFunc, fd);
    return mipOffset;
  }
  return extractBA2Texture(&buf, &UCharArray::allocFunc, fd, mipOffset);
}

size_t BA2File::extractFile(
    const unsigned char*& fileData, UCharArray& buf,
    const std::string_view& fileName) const
{
  fileData = nullptr;
  buf.clear();
  const FileInfo  *fdPtr = findFile(fileName);
  if (!fdPtr) [[unlikely]]
    findFileError(fileName);
  const FileInfo& fd = *fdPtr;
  unsigned int  packedSize = fd.packedSize;
  int     archiveType = fd.archiveType;
  if (packedSize || archiveType == 1 || archiveType == 2)
  {
    extractFile(&buf, &UCharArray::allocFunc, fd);
    size_t  unpackedSize = buf.size;
    if (unpackedSize) [[likely]]
      fileData = buf.data;
    return unpackedSize;
  }

  size_t  unpackedSize = fd.unpackedSize;
  if (unpackedSize)
  {
    const unsigned char *p = fd.fileData;
    const FileBuffer& fileBuf = *(archiveFiles[fd.archiveFile]);
    size_t  offs = size_t(p - fileBuf.data());
    if (offs >= fileBuf.size() || (offs + unpackedSize) > fileBuf.size())
      errorMessage("invalid packed data offset or size");
    fileData = p;
  }
  return unpackedSize;
}

void BA2File::extractFile(
    void *bufPtr, unsigned char * (*allocFunc)(void *bufPtr, size_t nBytes),
    const FileInfo& fd) const
{
  int     archiveType = fd.archiveType;
  if (archiveType < 0) [[unlikely]]
  {
    loadFile(bufPtr, allocFunc, fd);
    return;
  }
  if (!((archiveType - 1) & ~1))
  {
    (void) extractBA2Texture(bufPtr, allocFunc, fd);
    return;
  }

  const unsigned char *p = fd.fileData;
  unsigned int  packedSize = fd.packedSize;
  unsigned int  unpackedSize = fd.unpackedSize;
  if (archiveType & 0x40000100)         // BSA with compression or full names
  {
    unpackedSize = getBSAUnpackedSize(p, fd);
    if (packedSize) [[likely]]
      packedSize = packedSize - (unsigned int) (p - fd.fileData);
  }

  unsigned char *buf = allocFunc(bufPtr, unpackedSize);
  if (unpackedSize) [[likely]]
    extractBlock(buf, unpackedSize, fd, p, packedSize);
}

