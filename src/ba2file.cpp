
#include "common.hpp"
#include "ba2file.hpp"

BA2File::BA2File(const char *fileName)
  : FileBuffer(fileName),
    archiveType(-1)
{
  {
    unsigned int  hdr1 = readUInt32();
    unsigned int  hdr2 = readUInt32();
    unsigned int  hdr3 = readUInt32();
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
    if (archiveType < 0)
      throw errorMessage("unsupported archive file format");
  }
  if (archiveType <= 1)
  {
    size_t  fileCnt = readUInt32();
    size_t  nameOffs = readUInt64();
    if (fileCnt > (size() / 40) || nameOffs > size())
      throw errorMessage("invalid BA2 file header");
    fileDecls.resize(fileCnt);
    if (archiveType == 0)               // BA2 general
    {
      for (size_t i = 0; i < fileCnt; i++)
      {
        (void) readUInt32();            // unknown
        (void) readUInt32();            // extension
        (void) readUInt32();            // unknown
        (void) readUInt32();            // flags
        fileDecls[i].offset = readUInt64();
        fileDecls[i].packedSize = readUInt32();
        fileDecls[i].unpackedSize = readUInt32();
        (void) readUInt32();            // 0xBAADF00D
      }
    }
    else                                // BA2 textures
    {
      for (size_t i = 0; i < fileCnt; i++)
      {
        (void) readUInt32();            // unknown
        (void) readUInt32();            // extension ("dds\0")
        (void) readUInt32();            // unknown
        (void) readUInt8();             // unknown
        fileDecls[i].offset = getPosition();
        size_t  chunkCnt = readUInt8();
        (void) readUInt16();            // chunk header size
        (void) readUInt16();            // texture width
        (void) readUInt16();            // texture height
        (void) readUInt8();             // number of mipmaps
        (void) readUInt8();             // format
        (void) readUInt16();            // 0x0800
        fileDecls[i].packedSize = 0;
        fileDecls[i].unpackedSize = 148;
        for (size_t j = 0; j < chunkCnt; j++)
        {
          (void) readUInt64();          // file offset of chunk data
          fileDecls[i].packedSize = fileDecls[i].packedSize + readUInt32();
          fileDecls[i].unpackedSize = fileDecls[i].unpackedSize + readUInt32();
          (void) readUInt16();          // start mipmap
          (void) readUInt16();          // end mipmap
          (void) readUInt32();          // 0xBAADF00D
        }
      }
    }
    setPosition(nameOffs);
    for (size_t i = 0; i < fileCnt; i++)
    {
      fileDecls[i].fileName.clear();
      size_t  nameLen = readUInt16();
      while (nameLen--)
        fileDecls[i].fileName += fixNameCharacter(readUInt8());
    }
  }
  else                                  // BSA
  {
    std::string folderName;
    unsigned int  flags = readUInt32();
    if ((flags & ~0x01BCU) != 0x0003)
      throw errorMessage("unsupported archive file format");
    size_t  folderCnt = readUInt32();
    fileDecls.resize(readUInt32());     // total number of files
    (void) readUInt32();                // total length of all folder names
    (void) readUInt32();                // total length of all file names
    (void) readUInt16();                // file flags
    (void) readUInt16();                // padding
    std::vector< unsigned int > folderFileCnts(folderCnt);
    for (size_t i = 0; i < folderCnt; i++)
    {
      setPosition(i * (archiveType < 105 ? 16U : 24U) + 36);
      (void) readUInt64();              // hash
      folderFileCnts[i] = readUInt32();
    }
    setPosition(folderCnt * (archiveType < 105 ? 16U : 24U) + 36);
    size_t  n = 0;
    for (size_t i = 0; i < folderCnt; i++)
    {
      folderName.clear();
      for (size_t j = readUInt8(); j-- > 0; )
      {
        unsigned char c = readUInt8();
        if (folderName.empty() && (c == '.' || c == '/' || c == '\\'))
          continue;
        if (c)
          folderName += fixNameCharacter(c);
      }
      if (folderName.length() > 0 && folderName[folderName.length() - 1] != '/')
        folderName += '/';
      for (size_t j = folderFileCnts[i]; j-- > 0; n++)
      {
        fileDecls[n].fileName = folderName;
        (void) readUInt64();            // hash
        fileDecls[n].unpackedSize = readUInt32();
        fileDecls[n].offset = readUInt32();
        if ((flags ^ (fileDecls[n].unpackedSize >> 28)) & 0x04)
        {
          fileDecls[n].packedSize = fileDecls[n].unpackedSize & 0x3FFFFFFF;
          fileDecls[n].unpackedSize = 0;
        }
        else
        {
          fileDecls[n].packedSize = 0;
          fileDecls[n].unpackedSize = fileDecls[n].unpackedSize & 0x3FFFFFFF;
        }
      }
    }
    if (n != fileDecls.size())
      throw errorMessage("invalid file count in BSA archive");
    for (size_t i = 0; i < n; i++)
    {
      unsigned char c;
      while ((c = readUInt8()) != '\0')
        fileDecls[i].fileName += fixNameCharacter(c);
    }
    for (size_t i = 0; i < n; i++)
    {
      setPosition(size_t(fileDecls[i].offset));
      if (flags & 0x0100)
      {
        size_t  offs = readUInt8();
        offs = offs + getPosition();
        setPosition(offs);
      }
      if (fileDecls[i].packedSize)
      {
        fileDecls[i].packedSize = fileDecls[i].packedSize - 4;
        fileDecls[i].unpackedSize = readUInt32();
      }
      fileDecls[i].offset = getPosition();
    }
  }
  for (size_t i = 0; i < fileDecls.size(); i++)
  {
    if (fileMap.find(fileDecls[i].fileName) == fileMap.end())
    {
      fileMap.insert(std::pair< std::string, const FileDeclaration * >(
                         fileDecls[i].fileName, &(fileDecls.front()) + i));
    }
    else
    {
      fileMap[fileDecls[i].fileName] = &(fileDecls.front()) + i;
    }
  }
}

BA2File::~BA2File()
{
}

void BA2File::getFileList(std::vector< std::string >& fileList) const
{
  fileList.clear();
  for (std::map< std::string, const FileDeclaration * >::const_iterator i =
           fileMap.begin(); i != fileMap.end(); i++)
  {
    fileList.push_back(i->first);
  }
}

long BA2File::getFileSize(const char *fileName) const
{
  std::string s(fileName ? fileName : "");
  std::map< std::string, const FileDeclaration * >::const_iterator  i =
      fileMap.find(s);
  if (i == fileMap.end())
    return -1L;
  return i->second->unpackedSize;
}

void BA2File::extractFile(std::vector< unsigned char >& buf,
                          const char *fileName)
{
  buf.clear();
  std::string s(fileName ? fileName : "");
  std::map< std::string, const FileDeclaration * >::const_iterator  i =
    fileMap.find(s);
  if (i == fileMap.end())
    throw errorMessage("file not found in archive");
  if (i->second->unpackedSize == 0)
    return;
  buf.resize(i->second->unpackedSize, 0);
  if (archiveType != 1)
  {
    size_t  offs = size_t(i->second->offset);
    const unsigned char *p = getDataPtr() + offs;
    if (i->second->packedSize)
    {
      if (zlibDecompressor.decompressData(&(buf.front()), buf.size(),
                                          p, i->second->packedSize)
          != buf.size())
      {
        throw errorMessage("invalid or corrupt ZLib compressed data");
      }
    }
    else
    {
      std::memcpy(&(buf.front()), p, buf.size());
    }
    return;
  }
  // BA2 texture
  size_t  offs = size_t(i->second->offset);
  const unsigned char *p = getDataPtr() + offs;
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
  offs = offs + 11;
  size_t  writeOffs = 148;
  for ( ; chunkCnt-- > 0; offs = offs + 24)
  {
    setPosition(offs);
    size_t  chunkOffset = readUInt64();
    size_t  chunkSizePacked = readUInt32();
    size_t  chunkSizeUnpacked = readUInt32();
    (void) readUInt16();
    if (!chunkSizePacked)
    {
      if (chunkOffset >= size() || (chunkOffset + chunkSizeUnpacked) > size())
      {
        throw errorMessage("invalid texture chunk offset or size");
      }
      std::memcpy(&(buf.front()) + writeOffs,
                  getDataPtr() + chunkOffset, chunkSizeUnpacked);
    }
    else
    {
      if (chunkOffset >= size() || (chunkOffset + chunkSizePacked) > size())
      {
        throw errorMessage("invalid texture chunk offset or size");
      }
      if (zlibDecompressor.decompressData(&(buf.front()) + writeOffs,
                                          chunkSizeUnpacked,
                                          getDataPtr() + chunkOffset,
                                          chunkSizePacked)
          != chunkSizeUnpacked)
      {
        throw errorMessage("invalid or corrupt ZLib compressed data");
      }
    }
    writeOffs = writeOffs + chunkSizeUnpacked;
  }
}

