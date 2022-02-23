
#include "common.hpp"
#include "ba2file.hpp"

size_t BA2File::allocateFileDecls(size_t fileCnt)
{
  size_t  n0 = fileDecls.size();
  size_t  n = n0 + fileCnt;
  if (fileDecls.capacity() < n)
    fileDecls.reserve(((n + (n >> 2)) | 1023) + 1);
  return n0;
}

void BA2File::addPackedFile(const std::string& fileName, size_t n)
{
  std::map< std::string, size_t >::iterator i = fileMap.find(fileName);
  if (i != fileMap.end())
    i->second = n;
  else
    fileMap.insert(std::pair< std::string, size_t >(fileName, n));
}

void BA2File::loadBA2General(FileBuffer& buf, FileDeclaration& fileDecl)
{
  size_t  fileCnt = buf.readUInt32();
  size_t  nameOffs = buf.readUInt64();
  if (fileCnt > (buf.size() / 40) || nameOffs > buf.size())
    throw errorMessage("invalid BA2 file header");
  size_t  n0 = allocateFileDecls(fileCnt);
  for (size_t i = 0; i < fileCnt; i++)
  {
    (void) buf.readUInt32();            // unknown
    (void) buf.readUInt32();            // extension
    (void) buf.readUInt32();            // unknown
    (void) buf.readUInt32();            // flags
    fileDecl.fileData = buf.getDataPtr() + buf.readUInt64();
    fileDecl.packedSize = buf.readUInt32();
    fileDecl.unpackedSize = buf.readUInt32();
    (void) buf.readUInt32();            // 0xBAADF00D
    fileDecls.push_back(fileDecl);
  }
  buf.setPosition(nameOffs);
  std::string fileName;
  for (size_t i = 0; i < fileCnt; i++)
  {
    fileName.clear();
    size_t  nameLen = buf.readUInt16();
    while (nameLen--)
      fileName += fixNameCharacter(buf.readUInt8());
    addPackedFile(fileName, n0 + i);
  }
}

void BA2File::loadBA2Textures(FileBuffer& buf, FileDeclaration& fileDecl)
{
  size_t  fileCnt = buf.readUInt32();
  size_t  nameOffs = buf.readUInt64();
  if (fileCnt > (buf.size() / 40) || nameOffs > buf.size())
    throw errorMessage("invalid BA2 file header");
  size_t  n0 = allocateFileDecls(fileCnt);
  for (size_t i = 0; i < fileCnt; i++)
  {
    (void) buf.readUInt32();            // unknown
    (void) buf.readUInt32();            // extension ("dds\0")
    (void) buf.readUInt32();            // unknown
    (void) buf.readUInt8();             // unknown
    fileDecl.fileData = buf.getDataPtr() + buf.getPosition();
    size_t  chunkCnt = buf.readUInt8();
    (void) buf.readUInt16();            // chunk header size
    (void) buf.readUInt16();            // texture width
    (void) buf.readUInt16();            // texture height
    (void) buf.readUInt8();             // number of mipmaps
    (void) buf.readUInt8();             // format
    (void) buf.readUInt16();            // 0x0800
    fileDecl.packedSize = 0;
    fileDecl.unpackedSize = 148;
    for (size_t j = 0; j < chunkCnt; j++)
    {
      (void) buf.readUInt64();          // file offset of chunk data
      fileDecl.packedSize = fileDecl.packedSize + buf.readUInt32();
      fileDecl.unpackedSize = fileDecl.unpackedSize + buf.readUInt32();
      (void) buf.readUInt16();          // start mipmap
      (void) buf.readUInt16();          // end mipmap
      (void) buf.readUInt32();          // 0xBAADF00D
    }
    fileDecls.push_back(fileDecl);
  }
  buf.setPosition(nameOffs);
  std::string fileName;
  for (size_t i = 0; i < fileCnt; i++)
  {
    fileName.clear();
    size_t  nameLen = buf.readUInt16();
    while (nameLen--)
      fileName += fixNameCharacter(buf.readUInt8());
    addPackedFile(fileName, n0 + i);
  }
}

void BA2File::loadBSAFile(FileBuffer& buf, FileDeclaration& fileDecl)
{
  unsigned int  flags = buf.readUInt32();
  if (fileDecl.archiveType < 104)
    flags = flags & ~0x0700U;
  if ((flags & ~0x01BCU) != 0x0003)
    throw errorMessage("unsupported archive file format");
  size_t  folderCnt = buf.readUInt32();
  size_t  fileCnt = buf.readUInt32();   // total number of files
  size_t  n0 = allocateFileDecls(fileCnt);
  (void) buf.readUInt32();              // total length of all folder names
  (void) buf.readUInt32();              // total length of all file names
  (void) buf.readUInt16();              // file flags
  (void) buf.readUInt16();              // padding
  std::vector< unsigned int > folderFileCnts(folderCnt);
  std::vector< std::string >  folderNames(folderCnt);
  for (size_t i = 0; i < folderCnt; i++)
  {
    buf.setPosition(i * (fileDecl.archiveType < 105 ? 16U : 24U) + 36);
    (void) buf.readUInt64();            // hash
    folderFileCnts[i] = buf.readUInt32();
  }
  buf.setPosition(folderCnt * (fileDecl.archiveType < 105 ? 16U : 24U) + 36);
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
      fileDecl.unpackedSize = buf.readUInt32();
      fileDecl.fileData = buf.getDataPtr() + buf.readUInt32();
      if ((flags ^ (fileDecl.unpackedSize >> 28)) & 0x04)
      {
        fileDecl.packedSize = fileDecl.unpackedSize & 0x3FFFFFFF;
        fileDecl.unpackedSize = 0;
      }
      else
      {
        fileDecl.packedSize = 0;
        fileDecl.unpackedSize = fileDecl.unpackedSize & 0x3FFFFFFF;
      }
      fileDecls.push_back(fileDecl);
    }
  }
  if (n != fileCnt)
    throw errorMessage("invalid file count in BSA archive");
  n = n0;
  std::string fileName;
  for (size_t i = 0; i < folderCnt; i++)
  {
    for (size_t j = 0; j < folderFileCnts[i]; j++, n++)
    {
      fileName = folderNames[i];
      unsigned char c;
      while ((c = buf.readUInt8()) != '\0')
        fileName += fixNameCharacter(c);
      addPackedFile(fileName, n);
    }
  }
  for (size_t i = n0; i < n; i++)
  {
    buf.setPosition(size_t(fileDecls[i].fileData - buf.getDataPtr()));
    if (flags & 0x0100)
    {
      size_t  offs = buf.readUInt8();
      offs = offs + buf.getPosition();
      buf.setPosition(offs);
    }
    if (fileDecls[i].packedSize)
    {
      fileDecls[i].packedSize = fileDecls[i].packedSize - 4;
      fileDecls[i].unpackedSize = buf.readUInt32();
    }
    fileDecls[i].fileData = buf.getDataPtr() + buf.getPosition();
  }
}

void BA2File::loadArchiveFile(const char *fileName)
{
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
    FileDeclaration fileDecl;
    fileDecl.archiveType = archiveType;
    fileDecl.archiveFile = (unsigned int) archiveFiles.size();
    if (archiveType == 0)
      loadBA2General(buf, fileDecl);
    else if (archiveType == 1)
      loadBA2Textures(buf, fileDecl);
    else if (archiveType >= 0)
      loadBSAFile(buf, fileDecl);
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

BA2File::BA2File(const char *pathName)
{
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
  for (std::map< std::string, size_t >::const_iterator i = fileMap.begin();
       i != fileMap.end(); i++)
  {
    fileList.push_back(i->first);
  }
}

long BA2File::getFileSize(const char *fileName) const
{
  std::string s(fileName ? fileName : "");
  std::map< std::string, size_t >::const_iterator i = fileMap.find(s);
  if (i == fileMap.end())
    return -1L;
  return fileDecls[i->second].unpackedSize;
}

void BA2File::extractFile(std::vector< unsigned char >& buf,
                          const char *fileName)
{
  buf.clear();
  std::string s(fileName ? fileName : "");
  std::map< std::string, size_t >::const_iterator i = fileMap.find(s);
  if (i == fileMap.end())
    throw errorMessage("file not found in archive");
  const FileDeclaration&  fileDecl = *(&(fileDecls.front()) + i->second);
  if (fileDecl.unpackedSize == 0)
    return;
  FileBuffer& fileBuf = *(archiveFiles[fileDecl.archiveFile]);
  buf.resize(fileDecl.unpackedSize, 0);
  if (fileDecl.archiveType != 1)
  {
    const unsigned char *p = fileDecl.fileData;
    if (fileDecl.packedSize)
    {
      if ((p + fileDecl.packedSize) > (fileBuf.getDataPtr() + fileBuf.size()))
        throw errorMessage("invalid packed data offset or size");
      if (zlibDecompressor.decompressData(&(buf.front()), buf.size(),
                                          p, fileDecl.packedSize)
          != buf.size())
      {
        throw errorMessage("invalid or corrupt ZLib compressed data");
      }
    }
    else
    {
      if ((p + fileDecl.unpackedSize) > (fileBuf.getDataPtr() + fileBuf.size()))
        throw errorMessage("invalid packed data offset or size");
      std::memcpy(&(buf.front()), p, buf.size());
    }
    return;
  }

  // BA2 texture
  const unsigned char *p = fileDecl.fileData;
  size_t  chunkCnt = p[0];
  unsigned int  width = ((unsigned int) p[6] << 8) | p[5];
  unsigned int  height = ((unsigned int) p[4] << 8) | p[3];
  unsigned int  pitch = width;
  bool    compressedTexture = true;
  switch (p[8])
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
      throw errorMessage("unsupported DXGI_FORMAT 0x%02X", (unsigned int) p[8]);
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
  buf[12] = p[3];               // height
  buf[13] = p[4];
  buf[16] = p[5];               // width
  buf[17] = p[6];
  buf[20] = (unsigned char) (pitch & 0xFF);
  buf[21] = (unsigned char) ((pitch >> 8) & 0xFF);
  buf[22] = (unsigned char) ((pitch >> 16) & 0xFF);
  buf[23] = (unsigned char) ((pitch >> 24) & 0xFF);
  buf[28] = p[7];               // mipmap count
  buf[76] = 32;                 // size of DDS_PIXELFORMAT
  buf[80] = 0x04;               // DDPF_FOURCC
  buf[84] = 0x44;               // 'D'
  buf[85] = 0x58;               // 'X'
  buf[86] = 0x31;               // '1'
  buf[87] = 0x30;               // '0'
  buf[108] = 0x08;              // DDSCAPS_COMPLEX
  buf[109] = 0x10;              // DDSCAPS_TEXTURE
  buf[110] = 0x40;              // DDSCAPS_MIPMAP
  buf[128] = p[8];              // DXGI_FORMAT
  buf[132] = 3;                 // D3D10_RESOURCE_DIMENSION_TEXTURE2D
  size_t  offs = size_t(p - fileBuf.getDataPtr()) + 11;
  size_t  writeOffs = 148;
  for ( ; chunkCnt-- > 0; offs = offs + 24)
  {
    fileBuf.setPosition(offs);
    size_t  chunkOffset = fileBuf.readUInt64();
    size_t  chunkSizePacked = fileBuf.readUInt32();
    size_t  chunkSizeUnpacked = fileBuf.readUInt32();
    (void) fileBuf.readUInt16();
    if (!chunkSizePacked)
    {
      if (chunkOffset >= fileBuf.size() ||
          (chunkOffset + chunkSizeUnpacked) > fileBuf.size())
      {
        throw errorMessage("invalid texture chunk offset or size");
      }
      std::memcpy(&(buf.front()) + writeOffs,
                  fileBuf.getDataPtr() + chunkOffset, chunkSizeUnpacked);
    }
    else
    {
      if (chunkOffset >= fileBuf.size() ||
          (chunkOffset + chunkSizePacked) > fileBuf.size())
      {
        throw errorMessage("invalid texture chunk offset or size");
      }
      if (zlibDecompressor.decompressData(&(buf.front()) + writeOffs,
                                          chunkSizeUnpacked,
                                          fileBuf.getDataPtr() + chunkOffset,
                                          chunkSizePacked)
          != chunkSizeUnpacked)
      {
        throw errorMessage("invalid or corrupt ZLib compressed data");
      }
    }
    writeOffs = writeOffs + chunkSizeUnpacked;
  }
}

