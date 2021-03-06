
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
  int     tmp = uint32ToSigned(buf.readUInt32Fast());
  if (tmp != 2 && tmp != 20)
    throw errorMessage("unsupported BGSM file version");
  version = (unsigned char) tmp;
  flags = (unsigned char) (buf.readUInt32Fast() & 3U);
  offsetU = buf.readFloat();
  offsetV = buf.readFloat();
  scaleU = buf.readFloat();
  scaleV = buf.readFloat();
  tmp = roundFloat(buf.readFloat() * 128.0f);
  alpha = (unsigned char) (tmp > 0 ? (tmp < 255 ? tmp : 255) : 0);
  // blending enabled
  alphaFlags = (unsigned short) bool(buf.readUInt8Fast());
  // source blend mode
  alphaFlags = alphaFlags | (unsigned short) ((buf.readUInt32Fast() & 15) << 1);
  // destination blend mode
  alphaFlags = alphaFlags | (unsigned short) ((buf.readUInt32Fast() & 15) << 5);
  alphaThreshold = buf.readUInt8Fast();
  if (buf.readUInt8Fast())
    alphaFlags = alphaFlags | 0x1200;   // thresholding enabled, greater mode
  // Z buffer write, Z buffer test, screen space reflections
  (void) buf.readUInt32Fast();
  // decal
  flags = flags | ((unsigned char) bool(buf.readUInt8Fast()) << 2);
  // two sided
  flags = flags | ((unsigned char) bool(buf.readUInt8Fast()) << 3);
  unsigned long long  texturePathMap = 0ULL;
  buf.setPosition(58);
  if (version == 2)                     // Fallout 4
  {
    tmp = roundFloat(buf.readFloat() * 128.0f);
    envMapScale = (unsigned char) (tmp > 0 ? (tmp < 255 ? tmp : 255) : 0);
    // gradient map disabled / enabled
    texturePathMap = (!buf.readUInt8Fast() ?
                      0xFFFFFFFE7E244610ULL : 0xFFFFFFFE7E243610ULL);
    flags = flags | ((unsigned char) bool(buf[buf.size() - 29]) << 4);  // tree
    texturePaths.resize(9);
  }
  else                                  // Fallout 76
  {
    texturePathMap = (!(buf.readUInt16Fast() & 0xFF) ?
                      0xFFFFFFEE98724E10ULL : 0xFFFFFFEE98723E10ULL);
    flags = flags | ((unsigned char) bool(buf[buf.size() - 10]) << 4);
    texturePaths.resize(10);
  }
  for ( ; (texturePathMap & 15U) != 15U; texturePathMap = texturePathMap >> 4)
  {
    size_t  n = size_t(texturePathMap & 15U);
    size_t  len = buf.readUInt32();
    if (n >= texturePaths.size())
      buf.setPosition(buf.getPosition() + len);
    else
      buf.readPath(texturePaths[n], len, "textures/", ".dds");
  }
  buf.setPosition(buf.getPosition() + (version == 2 ? 15 : 24));
  if (buf.readUInt8() != 0)             // specular enabled
  {
    tmp = roundFloat(buf.readFloat() * 255.0f);
    tmp = (tmp > 0 ? (tmp < 255 ? tmp : 255) : 0);
    specularColor = (unsigned int) tmp | 0xFF000000U;   // R
    tmp = roundFloat(buf.readFloat() * 255.0f);
    tmp = (tmp > 0 ? (tmp < 255 ? tmp : 255) : 0);
    specularColor |= ((unsigned int) tmp << 8);         // G
    tmp = roundFloat(buf.readFloat() * 255.0f);
    tmp = (tmp > 0 ? (tmp < 255 ? tmp : 255) : 0);
    specularColor |= ((unsigned int) tmp << 16);        // B
    tmp = roundFloat(buf.readFloat() * 128.0f);
    specularScale = (unsigned char) (tmp > 0 ? (tmp < 255 ? tmp : 255) : 0);
    tmp = roundFloat(buf.readFloat() * 255.0f);
    specularSmoothness =
        (unsigned char) (tmp > 0 ? (tmp < 255 ? tmp : 255) : 0);
  }
  if (texturePaths[4].empty() && !texturePaths[(version == 2 ? 6 : 8)].empty())
  {
    if (specularScale)
    {
      tmp = (int(envMapScale) * int(specularScale) + 64) >> 7;
      envMapScale = (unsigned char) (tmp < 255 ? tmp : 255);
    }
    else if (version != 2 || buf[57] == 0)
    {
      envMapScale = 0;
    }
    if (!envMapScale)
      texturePaths[6].clear();
  }
  if (!texturePaths[3].empty())
  {
    buf.setPosition(buf.size() - (version == 2 ? 5 : 6));
    gradientMapV =
        (unsigned char) (roundFloat(buf.readFloat() * 255.0f) & 0xFF);
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
  gradientMapV = 128;
  envMapScale = 128;
  specularColor = 0xFFFFFFFFU;
  specularScale = 0;
  specularSmoothness = 128;
  alphaFlags = 0x00EC;
  alphaThreshold = 0;
  alpha = 128;
  offsetU = 0.0f;
  offsetV = 0.0f;
  scaleU = 1.0f;
  scaleV = 1.0f;
}

