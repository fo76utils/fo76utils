
#include "common.hpp"
#include "zlib.hpp"
#include "ba2file.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

inline BA2File::FileDeclaration::FileDeclaration()
  : fileData((unsigned char *) 0),
    packedSize(0),
    unpackedSize(0),
    archiveType(0),
    archiveFile(0)
{
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
  FileDeclaration&  fd = fileMap[fileName];
  return &fd;
}

void BA2File::loadBA2General(FileBuffer& buf, size_t archiveFile)
{
  size_t  fileCnt = buf.readUInt32();
  size_t  nameOffs = buf.readUInt64();
  if (nameOffs > buf.size() || (fileCnt * 36ULL + 24U) > nameOffs)
    throw errorMessage("invalid BA2 file header");
  std::vector< FileDeclaration * >  fileDecls(fileCnt, (FileDeclaration *) 0);
  buf.setPosition(nameOffs);
  std::string fileName;
  for (size_t i = 0; i < fileCnt; i++)
  {
    fileName.clear();
    size_t  nameLen = buf.readUInt16();
    if ((buf.getPosition() + nameLen) > buf.size())
      throw errorMessage("end of input file");
    while (nameLen--)
      fileName += fixNameCharacter(buf.readUInt8Fast());
    fileDecls[i] = addPackedFile(fileName);
  }
  for (size_t i = 0; i < fileCnt; i++)
  {
    if (!fileDecls[i])
      continue;
    FileDeclaration&  fileDecl = *(fileDecls[i]);
    buf.setPosition(i * 36 + 24);
    (void) buf.readUInt32Fast();        // unknown
    (void) buf.readUInt32Fast();        // extension
    (void) buf.readUInt32Fast();        // unknown
    (void) buf.readUInt32Fast();        // flags
    fileDecl.fileData = buf.getDataPtr() + buf.readUInt64();
    fileDecl.packedSize = buf.readUInt32Fast();
    fileDecl.unpackedSize = buf.readUInt32Fast();
    fileDecl.archiveType = 0;
    fileDecl.archiveFile = (unsigned int) archiveFile;
    (void) buf.readUInt32Fast();        // 0xBAADF00D
  }
}

void BA2File::loadBA2Textures(FileBuffer& buf, size_t archiveFile)
{
  size_t  fileCnt = buf.readUInt32();
  size_t  nameOffs = buf.readUInt64();
  if (nameOffs > buf.size() || (fileCnt * 48ULL + 24U) > nameOffs)
    throw errorMessage("invalid BA2 file header");
  std::vector< FileDeclaration * >  fileDecls(fileCnt, (FileDeclaration *) 0);
  buf.setPosition(nameOffs);
  std::string fileName;
  for (size_t i = 0; i < fileCnt; i++)
  {
    fileName.clear();
    size_t  nameLen = buf.readUInt16();
    if ((buf.getPosition() + nameLen) > buf.size())
      throw errorMessage("end of input file");
    while (nameLen--)
      fileName += fixNameCharacter(buf.readUInt8Fast());
    fileDecls[i] = addPackedFile(fileName);
  }
  buf.setPosition(24);
  for (size_t i = 0; i < fileCnt; i++)
  {
    if ((buf.getPosition() + 24) > buf.size())
      throw errorMessage("end of input file");
    (void) buf.readUInt32Fast();        // unknown
    (void) buf.readUInt32Fast();        // extension ("dds\0")
    (void) buf.readUInt32Fast();        // unknown
    (void) buf.readUInt8Fast();         // unknown
    const unsigned char *fileData = buf.getDataPtr() + buf.getPosition();
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
      throw errorMessage("end of input file");
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
      fileDecl.archiveType = 1;
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
    throw errorMessage("unsupported archive file format");
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
    if (folderName.length() > 0 && folderName[folderName.length() - 1] != '/')
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
    throw errorMessage("invalid file count in BSA archive");
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
        fileDecl->fileData = buf.getDataPtr() + size_t(fileDecls[n] >> 32);
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

void BA2File::loadArchivesFromDir(const char *pathName)
{
  DIR     *d = opendir(pathName);
  if (!d)
    throw errorMessage("error opening archive directory");
  try
  {
    std::set< std::string > archiveNames1;
    std::set< std::string > archiveNames2;
    std::string dirName(pathName);
    if (dirName[dirName.length() - 1] != '/' &&
        dirName[dirName.length() - 1] != '\\')
    {
      dirName += '/';
    }
    std::string baseName;
    std::string fullName;
    struct dirent *e;
    while (bool(e = readdir(d)))
    {
      baseName = e->d_name;
      if (baseName.length() < 5)
        continue;
      fullName = dirName;
      fullName += baseName;
      for (size_t i = 0; i < baseName.length(); i++)
      {
        if (baseName[i] >= 'A' && baseName[i] <= 'Z')
          baseName[i] = baseName[i] + ('a' - 'A');
      }
      const char  *s = baseName.c_str() + (baseName.length() - 4);
      if (!(std::strcmp(s, ".ba2") == 0 || std::strcmp(s, ".bsa") == 0))
        continue;
      if ((std::strncmp(baseName.c_str(), "oblivion", 8) == 0 ||
           std::strncmp(baseName.c_str(), "fallout", 7) == 0 ||
           std::strncmp(baseName.c_str(), "skyrim", 6) == 0 ||
           std::strncmp(baseName.c_str(), "seventysix", 10) == 0) &&
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
    if (!fileName || *fileName == '\0')
      throw errorMessage("empty input file name");
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
      throw errorMessage("error opening archive file or directory");
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

  FileBuffer  *bufp = new FileBuffer(fileName);
  try
  {
    FileBuffer&   buf = *bufp;
    int           archiveType = -1;
    unsigned int  hdr1 = buf.readUInt32();
    unsigned int  hdr2 = buf.readUInt32();
    unsigned int  hdr3 = buf.readUInt32();
    if (hdr1 == 0x58445442 && hdr2 == 0x00000001)       // "BTDX", version
    {
      if (hdr3 == 0x4C524E47)                           // "GNRL"
        archiveType = 0;
      else if (hdr3 == 0x30315844)                      // "DX10"
        archiveType = 1;
    }
    else if (hdr1 == 0x00415342 && hdr3 == 0x00000024)  // "BSA\0", header size
    {
      if (hdr2 >= 103 && hdr2 <= 105)
        archiveType = int(hdr2);
    }
    if (archiveType == 0)
      loadBA2General(buf, archiveFiles.size());
    else if (archiveType == 1)
      loadBA2Textures(buf, archiveFiles.size());
    else if (archiveType >= 0)
      loadBSAFile(buf, archiveFiles.size(), archiveType);
    else
      throw errorMessage("unsupported archive file format");
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
  size_t  offs = size_t(dataPtr - buf.getDataPtr());
  if (fd.archiveType & 0x00000100)
    offs = offs + (size_t(buf.readUInt8(offs)) + 1);
  unsigned int  unpackedSize = fd.unpackedSize;
  if (fd.archiveType & 0x40000000)
  {
    unpackedSize = buf.readUInt32(offs);
    offs = offs + 4;
  }
  dataPtr = buf.getDataPtr() + offs;
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

void BA2File::getFileList(std::vector< std::string >& fileList) const
{
  fileList.clear();
  for (std::map< std::string, FileDeclaration >::const_iterator
           i = fileMap.begin(); i != fileMap.end(); i++)
  {
    fileList.push_back(i->first);
  }
}

long BA2File::getFileSize(const std::string& fileName, bool packedSize) const
{
  std::map< std::string, FileDeclaration >::const_iterator  i =
      fileMap.find(fileName);
  if (i == fileMap.end())
    return -1L;
  const FileDeclaration&  fd = i->second;
  if (packedSize && fd.packedSize)
    return long(fd.packedSize);
  if (fd.archiveType & 0x40000000)      // compressed BSA
  {
    const unsigned char *p = fd.fileData;
    return long(getBSAUnpackedSize(p, fd));
  }
  return long(fd.unpackedSize);
}

int BA2File::extractBA2Texture(std::vector< unsigned char >& buf,
                               const FileDeclaration& fileDecl,
                               int mipOffset) const
{
  FileBuffer  fileBuf(archiveFiles[fileDecl.archiveFile]->getDataPtr(),
                      archiveFiles[fileDecl.archiveFile]->size());
  const unsigned char *p = fileDecl.fileData;
  size_t  chunkCnt = p[0];
  unsigned int  width = ((unsigned int) p[6] << 8) | p[5];
  unsigned int  height = ((unsigned int) p[4] << 8) | p[3];
  int     mipCnt = p[7];
  unsigned char dxgiFormat = p[8];
  size_t  offs = size_t(p - fileBuf.getDataPtr()) + 11;
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
                   fileBuf.getDataPtr() + chunkOffset, chunkSizePacked);
    }
  }
  // write DDS header
  unsigned int  pitch = width;
  bool    compressedTexture = true;
  switch (dxgiFormat)
  {
    case 0x3D:                  // DXGI_FORMAT_R8_UNORM
      compressedTexture = false;
      break;
    case 0x1B:                  // DXGI_FORMAT_R8G8B8A8_TYPELESS
    case 0x1C:                  // DXGI_FORMAT_R8G8B8A8_UNORM
    case 0x1D:                  // DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
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
      throw errorMessage("unsupported DXGI_FORMAT 0x%02X",
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
  size_t  offs = size_t(p - fileBuf.getDataPtr());
  if (!packedSize)
  {
    if (offs >= fileBuf.size() || (offs + unpackedSize) > fileBuf.size())
      throw errorMessage("invalid packed data offset or size");
    std::memcpy(&(buf.front()) + n, p, unpackedSize);
  }
  else
  {
    if (offs >= fileBuf.size() || (offs + packedSize) > fileBuf.size())
      throw errorMessage("invalid packed data offset or size");
    if (ZLibDecompressor::decompressData(&(buf.front()) + n, unpackedSize,
                                         p, packedSize) != unpackedSize)
    {
      throw errorMessage("invalid or corrupt ZLib compressed data");
    }
  }
}

void BA2File::extractFile(std::vector< unsigned char >& buf,
                          const std::string& fileName) const
{
  buf.clear();
  std::map< std::string, FileDeclaration >::const_iterator  i =
      fileMap.find(fileName);
  if (i == fileMap.end())
    throw errorMessage("file %s not found in archive", fileName.c_str());
  const FileDeclaration&  fileDecl = i->second;
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

  if (archiveType == 1)
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
  std::map< std::string, FileDeclaration >::const_iterator  i =
      fileMap.find(fileName);
  if (i == fileMap.end())
    throw errorMessage("file %s not found in archive", fileName.c_str());
  const FileDeclaration&  fileDecl = i->second;
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

  if (archiveType == 1)
    return extractBA2Texture(buf, fileDecl, mipOffset);

  extractBlock(buf, unpackedSize, fileDecl, p, packedSize);
  return mipOffset;
}

