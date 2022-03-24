
#include "common.hpp"
#include "bgsmfile.hpp"

BGSMFile::BGSMFile()
{
  clear();
}

BGSMFile::BGSMFile(const char *fileName)
{
  FileBuffer  buf(fileName);
  loadBGSMFile(buf);
}

BGSMFile::BGSMFile(const unsigned char *buf, size_t bufSize)
{
  FileBuffer  tmpBuf(buf, bufSize);
  loadBGSMFile(tmpBuf);
}

BGSMFile::BGSMFile(FileBuffer& buf)
{
  loadBGSMFile(buf);
}

BGSMFile::BGSMFile(const BA2File& ba2File, const std::string& fileName)
{
  loadBGSMFile(ba2File, fileName);
}

void BGSMFile::loadBGSMFile(FileBuffer& buf)
{
  clear();
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
    size_t  len = buf.readUInt32();
    buf.readPath(texturePaths[i], len, "textures/", ".dds");
  }
}

void BGSMFile::loadBGSMFile(const BA2File& ba2File, const std::string& fileName)
{
  std::vector< unsigned char >  tmpBuf;
  ba2File.extractFile(tmpBuf, fileName);
  FileBuffer  buf(&(tmpBuf.front()), tmpBuf.size());
  loadBGSMFile(buf);
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
  texturePaths.resize(0);
}

