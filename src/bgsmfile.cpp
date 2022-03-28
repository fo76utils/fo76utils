
#include "common.hpp"
#include "bgsmfile.hpp"

BGSMFile::BGSMFile()
{
  clear();
}

BGSMFile::BGSMFile(std::vector< std::string >& texturePaths,
                   const char *fileName)
{
  FileBuffer  buf(fileName);
  loadBGSMFile(texturePaths, buf);
}

BGSMFile::BGSMFile(std::vector< std::string >& texturePaths,
                   const unsigned char *buf, size_t bufSize)
{
  FileBuffer  tmpBuf(buf, bufSize);
  loadBGSMFile(texturePaths, tmpBuf);
}

BGSMFile::BGSMFile(std::vector< std::string >& texturePaths, FileBuffer& buf)
{
  loadBGSMFile(texturePaths, buf);
}

BGSMFile::BGSMFile(std::vector< std::string >& texturePaths,
                   const BA2File& ba2File, const std::string& fileName)
{
  loadBGSMFile(texturePaths, ba2File, fileName);
}

void BGSMFile::loadBGSMFile(std::vector< std::string >& texturePaths,
                            FileBuffer& buf)
{
  clear();
  for (size_t i = 0; i < texturePaths.size(); i++)
    texturePaths[i].clear();
  buf.setPosition(0);
  if (buf.size() < 68)
    throw errorMessage("BGSM file is shorter than expected");
  if (buf.readUInt32Fast() != 0x4D534742)       // "BGSM"
    throw errorMessage("invalid BGSM file header");
  version = buf.readUInt32Fast();
  if (version != 2 && version != 20)
    throw errorMessage("unsupported BGSM file version");
  flags = buf.readUInt32Fast();
  offsetU = buf.readFloat();
  offsetV = buf.readFloat();
  scaleU = buf.readFloat();
  scaleV = buf.readFloat();
  alpha = buf.readFloat();
  alphaBlendMode = (unsigned short) buf.readUInt8Fast() << 8;
  alphaBlendMode |= (unsigned short) ((buf.readUInt32Fast() & 0x0FU) << 4);
  alphaBlendMode |= (unsigned short) (buf.readUInt32Fast() & 0x0FU);
  alphaThreshold = buf.readUInt8Fast();
  alphaThresholdEnabled = bool(buf.readUInt8Fast());
  if (version == 2)
  {
    buf.setPosition(63);                // Fallout 4
    texturePaths.resize(9);
  }
  else
  {
    buf.setPosition(60);                // Fallout 76
    texturePaths.resize(10);
  }
  for (size_t i = 0; i < texturePaths.size(); i++)
  {
    static const unsigned char  texturePathMap[20] =
    {
      0x00, 0x01, 0x06, 0x03, 0x04, 0x02, 0x88, 0x07, 0x88, 0x88,       // FO4
      0x00, 0x01, 0x86, 0x03, 0x02, 0x07, 0x08, 0x09, 0x86, 0x86        // FO76
    };
    size_t  len = buf.readUInt32();
    size_t  n = texturePathMap[i + (version == 2 ? 0 : 10)];
    bool    unusedTexture = bool(n & 0x80);
    n = n & 0x0F;
    buf.readPath(texturePaths[n], len, "textures/", ".dds");
    if (unusedTexture)
      texturePaths[n].clear();
  }
}

void BGSMFile::loadBGSMFile(std::vector< std::string >& texturePaths,
                            const BA2File& ba2File, const std::string& fileName)
{
  std::vector< unsigned char >  tmpBuf;
  ba2File.extractFile(tmpBuf, fileName);
  FileBuffer  buf(&(tmpBuf.front()), tmpBuf.size());
  loadBGSMFile(texturePaths, buf);
}

void BGSMFile::clear()
{
  version = 0;
  flags = 0;
  offsetU = 0.0f;
  offsetV = 0.0f;
  scaleU = 1.0f;
  scaleV = 1.0f;
  alpha = 1.0f;
  alphaBlendMode = 0x0067;
  alphaThreshold = 128;
  alphaThresholdEnabled = false;
}

