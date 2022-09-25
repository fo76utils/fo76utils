
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

void BGSMFile::readTexturePaths(
    std::vector< std::string >& texturePaths,
    FileBuffer& buf, unsigned long long texturePathMap)
{
  size_t  texturePathCnt = texturePaths.size();
  for ( ; texturePathMap; texturePathMap = texturePathMap >> 4)
  {
    size_t  n = size_t(texturePathMap & 15U);
    if ((buf.getPosition() + 4) > buf.size())
      throw errorMessage("end of input file");
    size_t  len = buf.readUInt32Fast();
    if (n >= texturePathCnt)
    {
      buf.setPosition(buf.getPosition() + len);
      continue;
    }
    buf.readPath(texturePaths[n], len, "textures/", ".dds");
    if (!texturePaths[n].empty())
      texturePathMask = texturePathMask | std::uint16_t(1U << (unsigned int) n);
  }
}

void BGSMFile::loadBGEMFile(std::vector< std::string >& texturePaths,
                            FileBuffer& buf)
{
  clear();
  for (size_t i = 0; i < texturePaths.size(); i++)
    texturePaths[i].clear();
  buf.setPosition(0);
  if (buf.size() < 68)
    throw errorMessage("material file is shorter than expected");
  if (buf.readUInt32Fast() != 0x4D454742)       // "BGEM"
    throw errorMessage("invalid material file header");
  int     tmp = uint32ToSigned(buf.readUInt32Fast());
  if (tmp != 2 && tmp != 20)
    throw errorMessage("unsupported BGEM file version");
  version = (unsigned char) tmp;
  gradientMapV = 255;
  flags = std::uint16_t((buf.readUInt32Fast() & 3U) | 4U);      // is effect
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
  flags = flags | (std::uint16_t(bool(buf.readUInt8Fast())) << 3);
  // two sided
  flags = flags | (std::uint16_t(bool(buf.readUInt8Fast())) << 4);
  unsigned int  texturePathMap = 0U;
  float   envScale = 0.0f;
  if (version == 2)                     // Fallout 4
  {
    if (buf[57])
    {
      buf.setPosition(58);
      envScale = buf.readFloat();
    }
    // gradient map disabled / enabled
    texturePathMap = (!buf[62] ? 0x000514F0U : 0x00051430U);
    texturePaths.resize(6);
    buf.setPosition(63);
  }
  else                                  // Fallout 76
  {
    texturePathMap = (!buf[58] ? 0xF98514F0U : 0xF9851430U);
    texturePaths.resize(10);
    buf.setPosition(60);
  }
  readTexturePaths(texturePaths, buf, texturePathMap);
  if (version != 2)
  {
    if (buf.readUInt8())
    {
      envScale = buf.readFloat();
      if (!(texturePathMask & 0x0200))
      {
        texturePaths[9] = "textures/shared/default_noise_100_l.dds";
        texturePathMask = texturePathMask | 0x0200;
      }
    }
    if (buf[buf.size() - 2] && (texturePathMask & 0x0004))
      flags = flags | 0x80;             // enable glow map
    if (buf[buf.size() - 1] || (texturePathMask & 0x0200))
      specularColor = 0x80FFFFFFU;      // enable specular
    specularSmoothness = 255;
  }
  tmp = roundFloat(envScale * 128.0f);
  envMapScale = (unsigned char) (tmp > 0 ? (tmp < 255 ? tmp : 255) : 0);
}

void BGSMFile::loadBGSMFile(std::vector< std::string >& texturePaths,
                            FileBuffer& buf)
{
  if (buf.size() >= 4 && buf[2] == 0x45)
  {
    loadBGEMFile(texturePaths, buf);
    return;
  }
  clear();
  for (size_t i = 0; i < texturePaths.size(); i++)
    texturePaths[i].clear();
  buf.setPosition(0);
  if (buf.size() < 68)
    throw errorMessage("material file is shorter than expected");
  if (buf.readUInt32Fast() != 0x4D534742)       // "BGSM"
    throw errorMessage("invalid material file header");
  int     tmp = uint32ToSigned(buf.readUInt32Fast());
  if (tmp != 2 && tmp != 20)
    throw errorMessage("unsupported BGSM file version");
  version = (unsigned char) tmp;
  flags = std::uint16_t(buf.readUInt32Fast() & 3U);
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
  flags = flags | (std::uint16_t(bool(buf.readUInt8Fast())) << 3);
  // two sided
  flags = flags | (std::uint16_t(bool(buf.readUInt8Fast())) << 4);
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
    texturePathMap = (!buf[62] ? 0x0000000F7F244610ULL : 0x0000000F7F243610ULL);
    flags = flags | (std::uint16_t(bool(buf[buf.size() - 29])) << 5);   // tree
    flags = flags | (std::uint16_t(bool(buf[buf.size() - 45])) << 7);   // glow
    texturePaths.resize(9);
    buf.setPosition(63);
  }
  else                                  // Fallout 76
  {
    envScale = 1.0f;
    texturePathMap = (!buf[58] ? 0x000000FF9872FF10ULL : 0x000000FF98723F10ULL);
    flags = flags | (std::uint16_t(bool(buf[buf.size() - 10])) << 5);
    flags = flags | (std::uint16_t(bool(buf[buf.size() - 24])) << 7);
    texturePaths.resize(10);
    buf.setPosition(60);
  }
  readTexturePaths(texturePaths, buf, texturePathMap);
  buf.setPosition(buf.getPosition() + (version == 2 ? 15 : 24));
  bool    specularEnabled = bool(buf.readUInt8());
  FloatVector4  specColor(buf.readFloatVector4());
  tmp = roundFloat(envScale * specColor[3] * 128.0f);
  envMapScale = (unsigned char) (tmp > 0 ? (tmp < 255 ? tmp : 255) : 0);
  specColor[3] = specColor[3] * (specularEnabled ? (128.0f / 255.0f) : 0.0f);
  specularColor = std::uint32_t(specColor * FloatVector4(255.0f));
  tmp = roundFloat(buf.readFloat() * 255.0f);
  specularSmoothness = (unsigned char) (tmp > 0 ? (tmp < 255 ? tmp : 255) : 0);
  if (version == 2)
  {
    buf.setPosition(buf.getPosition() + 28);
    size_t  len = buf.readUInt32();             // root material
    buf.setPosition(buf.getPosition() + len);
    buf.setPosition(buf.getPosition() + 1);
    if (buf.getPosition() < buf.size())         // emit enabled
      flags = flags & ~(std::uint16_t(buf.readUInt8Fast() == 0) << 7);
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
  gradientMapV = 128;
  flags = 0;
  alphaFlags = 0x00EC;
  alphaThreshold = 0;
  alpha = 128;
  specularColor = 0x00FFFFFFU;
  specularSmoothness = 128;
  envMapScale = 0;
  texturePathMask = 0;
  offsetU = 0.0f;
  offsetV = 0.0f;
  scaleU = 1.0f;
  scaleV = 1.0f;
}

