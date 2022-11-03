
#include "common.hpp"
#include "bgsmfile.hpp"
#include "fp32vec4.hpp"

BGSMFile::TextureSet::TextureSetData::TextureSetData(const TextureSetData& r)
  : texturePaths(r.texturePaths),
    materialPath(r.materialPath),
    refCnt(0)
{
}

void BGSMFile::TextureSet::copyTextureSetData()
{
  TextureSetData  *tmp = new TextureSetData(*dataPtr);
  if (BRANCH_UNLIKELY(dataPtr->refCnt-- < 1))
    delete dataPtr;
  dataPtr = tmp;
}

BGSMFile::TextureSet& BGSMFile::TextureSet::operator=(const TextureSet& r)
{
  if (dataPtr)
  {
    if (dataPtr == r.dataPtr)
      return *this;
    if (dataPtr->refCnt-- < 1)
      delete dataPtr;
  }
  if (r.dataPtr)
    r.dataPtr->refCnt++;
  dataPtr = r.dataPtr;
  return *this;
}

void BGSMFile::TextureSet::setMaterialPath(const std::string& s)
{
  if (!dataPtr)
  {
    dataPtr = new TextureSetData;
  }
  else
  {
    if (s == dataPtr->materialPath)
      return;
    copyTexturePaths();
  }
  dataPtr->materialPath = s;
}

void BGSMFile::TextureSet::setTexturePath(size_t n, const char *s)
{
  if (BRANCH_UNLIKELY(n >= texturePathCnt))
    return;
  if (!s)
    s = "";
  if (!dataPtr)
  {
    dataPtr = new TextureSetData;
  }
  else
  {
    if (dataPtr->texturePaths[n] == s)
      return;
    copyTexturePaths();
  }
  dataPtr->texturePaths[n] = s;
}

std::uint32_t BGSMFile::TextureSet::readTexturePaths(
    FileBuffer& f, unsigned long long texturePathMap)
{
  if (!dataPtr)
    dataPtr = new TextureSetData;
  std::string s;
  unsigned int  m = 0U;
  do
  {
    size_t  n = size_t(texturePathMap & 15U);
    if ((f.getPosition() + 4) > f.size())
      errorMessage("end of input file");
    size_t  len = f.readUInt32Fast();
    if (n >= texturePathCnt)
    {
      f.setPosition(f.getPosition() + len);
      continue;
    }
    f.readPath(s, len, "textures/", ".dds");
    if (!s.empty())
    {
      m = m | (1U << (unsigned int) n);
      if (s != dataPtr->texturePaths[n])
      {
        copyTexturePaths();
        dataPtr->texturePaths[n] = s;
      }
    }
  }
  while ((texturePathMap = (texturePathMap >> 4)) != 0ULL);
  return std::uint32_t(m);
}

inline void BGSMFile::clear()
{
  flags = 0U;
  version = 0;
  alphaThreshold = 0;
  alphaFlags = 0x00EC;
  alpha = 1.0f;
  alphaThresholdFloat = 0.0f;
  s.gradientMapV = 0.5f;
  s.unused = 0.0f;
  s.envMapScale = 1.0f;
  s.specularSmoothness = 0.5f;
  s.specularColor = FloatVector4(1.0f, 1.0f, 1.0f, 0.0f);
  s.emissiveColor = FloatVector4(1.0f, 1.0f, 1.0f, 1.0f);
  textureOffsetU = 0.0f;
  textureOffsetV = 0.0f;
  textureScaleU = 1.0f;
  textureScaleV = 1.0f;
  texturePathMask = 0U;
}

BGSMFile::BGSMFile()
{
  nifVersion = 0U;
  clear();
}

BGSMFile::BGSMFile(const char *fileName)
{
  nifVersion = 0U;
  FileBuffer  buf(fileName);
  loadBGSMFile(buf);
}

BGSMFile::BGSMFile(const unsigned char *buf, size_t bufSize)
{
  nifVersion = 0U;
  FileBuffer  tmpBuf(buf, bufSize);
  loadBGSMFile(tmpBuf);
}

BGSMFile::BGSMFile(FileBuffer& buf)
{
  nifVersion = 0U;
  loadBGSMFile(buf);
}

BGSMFile::BGSMFile(const BA2File& ba2File, const std::string& fileName)
{
  nifVersion = 0U;
  loadBGSMFile(ba2File, fileName);
}

static inline float readFloatClamped(
    FileBuffer& buf, float minVal, float maxVal)
{
  float   tmp = buf.readFloat();
  return std::min(std::max(tmp, minVal), maxVal);
}

static inline FloatVector4 readFP32Vec4Clamped(
    FileBuffer& buf, FloatVector4 minVal, FloatVector4 maxVal)
{
  FloatVector4  tmp = buf.readFloatVector4();
  return tmp.maxValues(minVal).minValues(maxVal);
}

void BGSMFile::loadBGEMFile(FileBuffer& buf)
{
  // Z buffer write, Z buffer test, screen space reflections
  if (!(buf.readUInt32Fast() & 0xFFU))
    flags = flags | Flag_NoZWrite;
  // decal
  flags = flags | (std::uint32_t(bool(buf.readUInt8Fast())) << 3);
  // two sided
  flags = flags | (std::uint32_t(bool(buf.readUInt8Fast())) << 4);
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
    buf.setPosition(63);
  }
  else                                  // Fallout 76
  {
    texturePathMap = (!buf[58] ? 0xF98514F0U : 0xF9851430U);
    buf.setPosition(60);
  }
  texturePathMask = texturePaths.readTexturePaths(buf, texturePathMap);
  if (version != 2)
  {
    if (buf.readUInt8())
    {
      envScale = buf.readFloat();
      if (!(texturePathMask & 0x0200))
      {
        texturePaths.setTexturePath(
            9, "textures/shared/default_noise_100_l.dds");
        texturePathMask = texturePathMask | 0x0200;
      }
    }
    else
    {
      buf.setPosition(buf.getPosition() + 4);
    }
    if (buf[buf.size() - 2] && (texturePathMask & 0x0004))
      flags = flags | Flag_Glow;        // enable glow map
#if 0
    if (buf.back())
      flags = flags | Flag_EffectSpecular;
#endif
    e.specularSmoothness = 1.0f;
  }
  if (!(texturePathMask & 0x0310))
    e.specularSmoothness = 0.0f;
  e.envMapScale =
      (envScale > 0.0f ? (envScale < 8.0f ? envScale : 8.0f) : 0.0f);
  buf.setPosition(buf.getPosition() + 6);
  if (buf.getPosition() < buf.size())
  {
    if (buf[buf.getPosition() - 5])
      flags = flags | Flag_EffectLighting;
    if (buf[buf.getPosition() - 4] | buf[buf.getPosition() - 3])
      flags = flags | Flag_FalloffEnabled;
    // grayscale to alpha mapping
    if ((texturePathMask & 0x0008) && buf[buf.getPosition() - 2])
      flags = flags | Flag_GrayscaleToAlpha;
  }
  e.baseColor = readFP32Vec4Clamped(buf, FloatVector4(0.0f),
                                    FloatVector4(1.0f, 1.0f, 1.0f, 8.0f));
  e.baseColorScale = e.baseColor[3];
  e.baseColor[3] = 1.0f;
  e.falloffParams =
      readFP32Vec4Clamped(buf, FloatVector4(0.0f), FloatVector4(1.0f));
  e.lightingInfluence = readFloatClamped(buf, 0.0f, 1.0f);
  updateAlphaProperties();
}

void BGSMFile::loadBGSMFile(FileBuffer& buf)
{
  clear();
  buf.setPosition(0);
  if (buf.size() < 68)
    errorMessage("material file is shorter than expected");
  {
    std::uint32_t tmp = buf.readUInt32Fast();
    if (tmp != 0x4D534742 && tmp != 0x4D454742)         // "BGSM", "BGEM"
      errorMessage("invalid material file header");
  }
  if (buf.readUInt32Fast() > 0xFFU || (buf[4] != 2 && buf[4] != 20))
    errorMessage("unsupported material file version");
  version = buf[4];
  if (!nifVersion)
    nifVersion = (version == 2 ? 130U : 155U);
  flags = (buf.readUInt32Fast() & 3U) | (buf[2] & 4U);  // tile U,V, is effect
  {
    FloatVector4  txtOffsScale(readFP32Vec4Clamped(buf, FloatVector4(-256.0f),
                                                   FloatVector4(256.0f)));
    textureOffsetU = txtOffsScale[0];
    textureOffsetV = txtOffsScale[1];
    textureScaleU = txtOffsScale[2];
    textureScaleV = txtOffsScale[3];
  }
  alpha = readFloatClamped(buf, 0.0f, 8.0f);
  // blending enabled
  alphaFlags = std::uint16_t(bool(buf.readUInt8Fast()));
  // source blend mode
  alphaFlags = alphaFlags | std::uint16_t((buf.readUInt32Fast() & 15) << 1);
  // destination blend mode
  alphaFlags = alphaFlags | std::uint16_t((buf.readUInt32Fast() & 15) << 5);
  alphaThreshold = buf.readUInt8Fast();
  if (buf.readUInt8Fast())
    alphaFlags = alphaFlags | 0x1200;   // thresholding enabled, greater mode
  if (flags & Flag_IsEffect)
  {
    loadBGEMFile(buf);
    return;
  }
  // Z buffer write, Z buffer test, screen space reflections
  (void) buf.readUInt32Fast();
  // decal
  flags = flags | (std::uint32_t(bool(buf.readUInt8Fast())) << 3);
  // two sided
  flags = flags | (std::uint32_t(bool(buf.readUInt8Fast())) << 4);
  unsigned long long  texturePathMap = 0ULL;
  float   envScale = 0.0f;
  if (version == 2)                     // Fallout 4
  {
    if (buf[57])
    {
      buf.setPosition(58);
      envScale = readFloatClamped(buf, 0.0f, 8.0f);
    }
    // gradient map disabled / enabled
    texturePathMap = (!buf[62] ? 0x0000000F7F244610ULL : 0x0000000F7F243610ULL);
    flags = flags | (std::uint32_t(bool(buf[buf.size() - 29])) << 5);   // tree
    flags = flags | (std::uint32_t(bool(buf[buf.size() - 45])) << 7);   // glow
    buf.setPosition(63);
  }
  else                                  // Fallout 76
  {
    envScale = 1.0f;
    texturePathMap = (!buf[58] ? 0x000000FF9872FF10ULL : 0x000000FF98723F10ULL);
    flags = flags | (std::uint32_t(bool(buf[buf.size() - 10])) << 5);
    flags = flags | (std::uint32_t(bool(buf[buf.size() - 24])) << 7);
    buf.setPosition(60);
  }
  texturePathMask = texturePaths.readTexturePaths(buf, texturePathMap);
  buf.setPosition(buf.getPosition() + (version == 2 ? 15 : 24));
  bool    specularEnabled = bool(buf.readUInt8());
  s.specularColor = readFP32Vec4Clamped(buf, FloatVector4(0.0f),
                                        FloatVector4(1.0f, 1.0f, 1.0f, 8.0f));
  s.envMapScale = std::min(envScale * s.specularColor[3], 8.0f);
  if (!specularEnabled)
    s.specularColor[3] = 0.0f;
  s.specularSmoothness = readFloatClamped(buf, 0.0f, 1.0f);
  buf.setPosition(buf.getPosition() + (version == 2 ? 28 : 30));
  size_t  len = buf.readUInt32();                       // root material,
  buf.setPosition(buf.getPosition() + (len + 1));       // anisotropic lighting
  if ((buf.getPosition() + 17) <= buf.size())
  {
    if (buf.readUInt8Fast())                    // emit enabled
    {
      s.emissiveColor =
          readFP32Vec4Clamped(buf, FloatVector4(0.0f),
                              FloatVector4(1.0f, 1.0f, 1.0f, 8.0f));
    }
    else if (version != 2 && (texturePathMask & 0x0004))
    {
      s.emissiveColor[3] = readFloatClamped(buf, 0.0f, 8.0f);
    }
    else
    {
      flags = flags & ~(std::uint32_t(Flag_Glow));
      s.emissiveColor = FloatVector4(0.0f);
    }
  }
  if (texturePathMask & 0x0008)
  {
    buf.setPosition(buf.size() - (version == 2 ? 5 : 6));
    s.gradientMapV = readFloatClamped(buf, 0.0f, 1.0f);
  }
  updateAlphaProperties();
}

void BGSMFile::loadBGSMFile(const BA2File& ba2File, const std::string& fileName)
{
  std::vector< unsigned char >  tmpBuf;
  ba2File.extractFile(tmpBuf, fileName);
  FileBuffer  buf(tmpBuf.data(), tmpBuf.size());
  loadBGSMFile(buf);
}

void BGSMFile::updateAlphaProperties()
{
  alphaThresholdFloat = 0.0f;
  if (BRANCH_UNLIKELY(flags & Flag_TSWater))
  {
    flags = flags | Flag_TSAlphaBlending;
    return;
  }
  bool    isAlphaBlending = bool(alphaFlags & 0x0001);
  bool    isAlphaTesting = !(~alphaFlags & 0x1200);
  flags = flags & ~(std::uint32_t(Flag_TSAlphaBlending));
  if (BRANCH_LIKELY(!(isAlphaBlending || isAlphaTesting)))
    return;
  if (BRANCH_UNLIKELY(!(alpha >= (1.0f / 512.0f))))
  {
    alphaThresholdFloat = 256.0f;
  }
  else if (BRANCH_LIKELY(isAlphaTesting))
  {
    if (BRANCH_UNLIKELY(alphaFlags & 0x0400))
    {
      if (alphaFlags & 0x0800)
        alphaThresholdFloat = 256.0f;
    }
    else
    {
      float   t = float(int(alphaThreshold)) / alpha;
      t = t + (!(alphaFlags & 0x0800) ? (1.0f / 512.0f) : (-1.0f / 512.0f));
      alphaThresholdFloat = std::max(std::min(t, 256.0f), 0.0f);
    }
  }
  if (isAlphaBlending && alphaThresholdFloat < 256.0f)
    flags = flags | Flag_TSAlphaBlending;
}

void BGSMFile::setWaterColor(std::uint32_t c, float reflectionLevel)
{
  flags = flags | (Flag_TSWater | Flag_TSAlphaBlending | Flag_TwoSided);
  alphaThresholdFloat = 0.0f;
  FloatVector4  tmp(c);
  float   d = tmp[3];
  if (nifVersion < 0x90U && version < 20)
    tmp *= (1.0f / 255.0f);
  else
    tmp.srgbExpand();
  w.maxDepth = (d * 0.125f - 64.0f) * d + 8192.0f;
  w.envMapScale = reflectionLevel;
  w.specularSmoothness = 1.0f;
  tmp[3] = 0.5f;
  w.shallowColor = tmp;
  tmp[3] = 0.9375f;
  w.deepColor = tmp;
}

