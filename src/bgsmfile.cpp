
#include "common.hpp"
#include "bgsmfile.hpp"
#include "fp32vec4.hpp"

inline void BGSMFile::clear()
{
  version = 0;
  gradientMapV = 128;
  flags = 0;
  alphaFlags = 0x00EC;
  alphaThreshold = 0;
  alpha = 128;
  specularColor = 0x00FFFFFFU;
  specularSmoothness = 128;
  envMapScale = 128;
  texturePathMask = 0;
  textureOffsetU = 0.0f;
  textureOffsetV = 0.0f;
  textureScaleU = 1.0f;
  textureScaleV = 1.0f;
  emissiveColor = 0x80FFFFFFU;
}

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
      errorMessage("end of input file");
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
    errorMessage("material file is shorter than expected");
  if (buf.readUInt32Fast() != 0x4D454742)       // "BGEM"
    errorMessage("invalid material file header");
  if (buf.readUInt32Fast() > 0xFFU || (buf[4] != 2 && buf[4] != 20))
    errorMessage("unsupported BGEM file version");
  version = buf[4];
  gradientMapV = 255;
  flags = std::uint16_t((buf.readUInt32Fast() & 3U) | 4U);      // is effect
  textureOffsetU = buf.readFloat();
  textureOffsetV = buf.readFloat();
  textureScaleU = buf.readFloat();
  textureScaleV = buf.readFloat();
  alpha = floatToUInt8Clamped(buf.readFloat(), 128.0f);
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
    else
    {
      buf.setPosition(buf.getPosition() + 4);
    }
    if (buf[buf.size() - 2] && (texturePathMask & 0x0004))
      flags = flags | Flag_Glow;        // enable glow map
    if (buf[buf.size() - 1] || (texturePathMask & 0x0200))
      specularColor = 0x80FFFFFFU;      // enable specular
    specularSmoothness = 255;
  }
  if (!(texturePathMask & 0x0310))
    specularSmoothness = 0;
  envMapScale = floatToUInt8Clamped(envScale, 128.0f);
  buf.setPosition(buf.getPosition() + 6);
  // grayscale to alpha mapping
  if ((texturePathMask & 0x0008) && buf.getPosition() < buf.size())
    flags = flags | (std::uint16_t(bool(buf[buf.getPosition() - 2])) << 6);
  emissiveColor = std::uint32_t(buf.readFloatVector4()
                                * FloatVector4(255.0f, 255.0f, 255.0f, 128.0f));
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
    errorMessage("material file is shorter than expected");
  if (buf.readUInt32Fast() != 0x4D534742)       // "BGSM"
    errorMessage("invalid material file header");
  if (buf.readUInt32Fast() > 0xFFU || (buf[4] != 2 && buf[4] != 20))
    errorMessage("unsupported BGSM file version");
  version = buf[4];
  flags = std::uint16_t(buf.readUInt32Fast() & 3U);
  textureOffsetU = buf.readFloat();
  textureOffsetV = buf.readFloat();
  textureScaleU = buf.readFloat();
  textureScaleV = buf.readFloat();
  alpha = floatToUInt8Clamped(buf.readFloat(), 128.0f);
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
  envMapScale = floatToUInt8Clamped(envScale, specColor[3] * 128.0f);
  specColor[3] = specColor[3] * (specularEnabled ? (128.0f / 255.0f) : 0.0f);
  specularColor = std::uint32_t(specColor * 255.0f);
  specularSmoothness = floatToUInt8Clamped(buf.readFloat(), 255.0f);
  buf.setPosition(buf.getPosition() + (version == 2 ? 28 : 30));
  size_t  len = buf.readUInt32();                       // root material,
  buf.setPosition(buf.getPosition() + (len + 1));       // anisotropic lighting
  if ((buf.getPosition() + 17) <= buf.size())
  {
    if (buf.readUInt8Fast())                    // emit enabled
    {
      emissiveColor =
          std::uint32_t(buf.readFloatVector4()
                        * FloatVector4(255.0f, 255.0f, 255.0f, 128.0f));
    }
    else if (version != 2 && (texturePathMask & 0x0004))
    {
      emissiveColor =
          (std::uint32_t(floatToUInt8Clamped(buf.readFloat(), 128.0f)) << 24)
          | 0x00FFFFFFU;
    }
    else
    {
      flags = flags & ~(std::uint16_t(Flag_Glow));
      emissiveColor = 0U;
    }
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

