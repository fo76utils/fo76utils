
#include "common.hpp"
#include "bgsmfile.hpp"
#include "fp32vec4.hpp"

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
  alphaFlags = std::uint16_t(bool(buf.readUInt8Fast()));
  // source blend mode
  alphaFlags = alphaFlags | std::uint16_t((buf.readUInt32Fast() & 15) << 1);
  // destination blend mode
  alphaFlags = alphaFlags | std::uint16_t((buf.readUInt32Fast() & 15) << 5);
  alphaThreshold = buf.readUInt8Fast();
  if (buf.readUInt8Fast())
    alphaFlags = alphaFlags | 0x1200;   // thresholding enabled, greater mode
  // Z buffer write, Z buffer test, screen space reflections
  (void) buf.readUInt32Fast();
  // decal
  flags = flags | ((unsigned char) bool(buf.readUInt8Fast()) << 3);
  // two sided
  flags = flags | ((unsigned char) bool(buf.readUInt8Fast()) << 4);
  unsigned long long  texturePathMap = 0ULL;
  float   envScale = 0.0f;
  if (version == 2)                     // Fallout 4
  {
    if (buf[57])
    {
      buf.setPosition(58);
      envScale = buf.readFloat();
    }
    // gradient map disabled / enabled
    texturePathMap = (!buf[62] ? 0xFFFFFFFE7E244610ULL : 0xFFFFFFFE7E243610ULL);
    flags = flags | ((unsigned char) bool(buf[buf.size() - 29]) << 5);  // tree
    flags = flags | ((unsigned char) bool(buf[buf.size() - 45]) << 7);  // glow
    texturePaths.resize(9);
    buf.setPosition(63);
  }
  else                                  // Fallout 76
  {
    envScale = 1.0f;
    texturePathMap = (!buf[58] ? 0xFFFFFFEE98724E10ULL : 0xFFFFFFEE98723E10ULL);
    flags = flags | ((unsigned char) bool(buf[buf.size() - 10]) << 5);
    flags = flags | ((unsigned char) bool(buf[buf.size() - 24]) << 7);
    texturePaths.resize(10);
    buf.setPosition(60);
  }
  for ( ; (texturePathMap & 15U) != 15U; texturePathMap = texturePathMap >> 4)
  {
    size_t  n = size_t(texturePathMap & 15U);
    size_t  len = buf.readUInt32();
    if (n >= texturePaths.size())
    {
      buf.setPosition(buf.getPosition() + len);
      continue;
    }
    buf.readPath(texturePaths[n], len, "textures/", ".dds");
    if (!texturePaths[n].empty())
      texturePathMask = texturePathMask | std::uint16_t(1U << (unsigned int) n);
  }
  buf.setPosition(buf.getPosition() + (version == 2 ? 15 : 24));
  bool    specularEnabled = bool(buf.readUInt8());
  FloatVector4  specColor(buf.readFloatVector4());
  float   specScale = specColor[3] * 128.0f;
  tmp = roundFloat(envScale * specScale);
  envMapScale = (unsigned char) (tmp > 0 ? (tmp < 255 ? tmp : 255) : 0);
  specularColor = std::uint32_t(specColor * FloatVector4(255.0f)) | 0xFF000000U;
  if (specularEnabled)
  {
    tmp = roundFloat(specScale);
    specularScale = (unsigned char) (tmp > 0 ? (tmp < 255 ? tmp : 255) : 0);
  }
  tmp = roundFloat(buf.readFloat() * 255.0f);
  specularSmoothness = (unsigned char) (tmp > 0 ? (tmp < 255 ? tmp : 255) : 0);
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
  envMapScale = 0;
  specularColor = 0xFFFFFFFFU;
  specularScale = 0;
  specularSmoothness = 128;
  alphaFlags = 0x00EC;
  alphaThreshold = 0;
  alpha = 128;
  texturePathMask = 0;
  offsetU = 0.0f;
  offsetV = 0.0f;
  scaleU = 1.0f;
  scaleV = 1.0f;
}

