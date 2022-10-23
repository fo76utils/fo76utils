
#include "common.hpp"
#include "fp32vec4.hpp"
#include "rndrbase.hpp"

size_t Renderer_Base::TextureCache::getTextureDataSize(const DDSTexture *t)
{
  if (!t)
    return 0;
  size_t  n = size_t(t->getWidth()) * size_t(t->getHeight());
  return (size_t((n * 1431655765ULL) >> 30) * sizeof(std::uint32_t) + 1024U);
}

Renderer_Base::TextureCache::~TextureCache()
{
  clear();
}

const DDSTexture * Renderer_Base::TextureCache::loadTexture(
    const BA2File& ba2File, const std::string& fileName,
    std::vector< unsigned char >& fileBuf, int mipLevel, bool *waitFlag)
{
  if (fileName.empty())
    return (DDSTexture *) 0;
  textureCacheMutex.lock();
  std::map< std::string, CachedTexture >::iterator  i =
      textureCache.find(fileName);
  if (i != textureCache.end())
  {
    CachedTexture *cachedTexture = &(i->second);
    if (cachedTexture->nxt)
    {
      if (cachedTexture->prv)
        cachedTexture->prv->nxt = cachedTexture->nxt;
      else
        firstTexture = cachedTexture->nxt;
      cachedTexture->nxt->prv = cachedTexture->prv;
      lastTexture->nxt = cachedTexture;
      cachedTexture->prv = lastTexture;
      cachedTexture->nxt = (CachedTexture *) 0;
      lastTexture = cachedTexture;
    }
    textureCacheMutex.unlock();
    if (!waitFlag)
      cachedTexture->textureLoadMutex->lock();
    else if ((*waitFlag = !cachedTexture->textureLoadMutex->try_lock()) == true)
      return (DDSTexture *) 0;
    const DDSTexture  *t = cachedTexture->texture;
    cachedTexture->textureLoadMutex->unlock();
    return t;
  }

  CachedTexture *cachedTexture = (CachedTexture *) 0;
  std::mutex  *textureLoadMutex = new std::mutex();
  try
  {
    {
      CachedTexture tmp;
      tmp.texture = (DDSTexture *) 0;
      tmp.i = textureCache.end();
      tmp.prv = (CachedTexture *) 0;
      tmp.nxt = (CachedTexture *) 0;
      tmp.textureLoadMutex = textureLoadMutex;
      i = textureCache.insert(std::pair< std::string, CachedTexture >(
                                  fileName, tmp)).first;
    }
    textureLoadMutex = (std::mutex *) 0;
    cachedTexture = &(i->second);
    cachedTexture->i = i;
    if (lastTexture)
      lastTexture->nxt = cachedTexture;
    else
      firstTexture = cachedTexture;
    cachedTexture->prv = lastTexture;
    cachedTexture->nxt = (CachedTexture *) 0;
    lastTexture = cachedTexture;
  }
  catch (...)
  {
    textureCacheMutex.unlock();
    delete textureLoadMutex;
    throw;
  }

  DDSTexture  *t = (DDSTexture *) 0;
  cachedTexture->textureLoadMutex->lock();
  textureCacheMutex.unlock();
  try
  {
    mipLevel = ba2File.extractTexture(fileBuf, fileName, mipLevel);
    t = new DDSTexture(fileBuf.data(), fileBuf.size(), mipLevel);
    cachedTexture->texture = t;
    textureCacheMutex.lock();
    textureDataSize = textureDataSize + getTextureDataSize(t);
    textureCacheMutex.unlock();
  }
  catch (FO76UtilsError&)
  {
  }
  catch (...)
  {
    cachedTexture->textureLoadMutex->unlock();
    throw;
  }
  cachedTexture->textureLoadMutex->unlock();
  return t;
}

void Renderer_Base::TextureCache::shrinkTextureCache()
{
  while (textureDataSize > textureCacheSize && firstTexture)
  {
    size_t  dataSize = getTextureDataSize(firstTexture->texture);
    if (firstTexture->texture)
      delete firstTexture->texture;
    delete firstTexture->textureLoadMutex;
    std::map< std::string, CachedTexture >::iterator  i = firstTexture->i;
    if (firstTexture->nxt)
      firstTexture->nxt->prv = (CachedTexture *) 0;
    else
      lastTexture = (CachedTexture *) 0;
    firstTexture = firstTexture->nxt;
    textureCache.erase(i);
    textureDataSize = textureDataSize - dataSize;
  }
  if (!firstTexture)
    textureDataSize = 0;
}

void Renderer_Base::TextureCache::clear()
{
  for (CachedTexture *p = firstTexture; p; p = p->nxt)
  {
    if (p->texture)
      delete p->texture;
    delete p->textureLoadMutex;
  }
  textureDataSize = 0;
  firstTexture = (CachedTexture *) 0;
  lastTexture = (CachedTexture *) 0;
  textureCache.clear();
}

unsigned int Renderer_Base::MaterialSwaps::loadMaterialSwap(
    const BA2File& ba2File, ESMFile& esmFile, unsigned int formID)
{
  if (!formID)
    return 0U;
  {
    std::map< unsigned int,
              std::map< std::string, BGSMFile > >::const_iterator i =
        materialSwaps.find(formID);
    if (i != materialSwaps.end())
      return (i->second.begin() != i->second.end() ? formID : 0U);
  }
  std::vector< unsigned char >  fileBuf;
  std::string bnamPath;
  std::string snamPath;
  std::map< std::string, BGSMFile >&  v = materialSwaps[formID];
  const ESMFile::ESMRecord  *r = esmFile.getRecordPtr(formID);
  if (!(r && *r == "MSWP"))
    return 0U;
  ESMFile::ESMField f(esmFile, *r);
  while (f.next())
  {
    if (f == "BNAM")
    {
      f.readPath(bnamPath, std::string::npos, "materials/");
      if (bnamPath.length() >= 5 &&
          !(bnamPath[bnamPath.length() - 5] == '.' &&
            bnamPath[bnamPath.length() - 4] == 'b' &&
            bnamPath[bnamPath.length() - 1] == 'm'))
      {
        bnamPath += ".bgsm";
      }
    }
    else if (f == "SNAM" && !bnamPath.empty())
    {
      f.readPath(snamPath, std::string::npos, "materials/");
      if (!snamPath.empty())
      {
        if (snamPath.length() >= 5 &&
            !(snamPath[snamPath.length() - 5] == '.' &&
              snamPath[snamPath.length() - 4] == 'b' &&
              snamPath[snamPath.length() - 1] == 'm'))
        {
          snamPath += ".bgsm";
        }
        float   gradientMapV = -1.0f;
        if (f.dataRemaining >= 4U &&
            FileBuffer::readUInt32Fast(f.getDataPtr() + f.size())
            == 0x4D414E43U)             // "CNAM"
        {
          if (f.next() && f.size() >= 4)
          {
            gradientMapV = f.readFloat();
            gradientMapV = std::min(std::max(gradientMapV, 0.0f), 1.0f);
          }
        }
        static const char *bgsmNamePatterns[14] =
        {
          "*",            "",             "01",           "01decal",
          "02",           "_2sided",      "_8bit",        "alpha",
          "alpha_2sided", "_decal",       "decal",        "decal_8bit",
          "noalpha",      "wet"
        };
        size_t  n1 = bnamPath.find('*');
        size_t  n2 = std::string::npos;
        if (n1 != std::string::npos)
          n2 = snamPath.find('*');
        for (size_t i = 1; i < (sizeof(bgsmNamePatterns) / sizeof(char *)); i++)
        {
          if (n1 != std::string::npos)
          {
            bnamPath.erase(n1, std::strlen(bgsmNamePatterns[i - 1]));
            bnamPath.insert(n1, bgsmNamePatterns[i]);
            if (n2 != std::string::npos)
            {
              snamPath.erase(n2, std::strlen(bgsmNamePatterns[i - 1]));
              snamPath.insert(n2, bgsmNamePatterns[i]);
            }
            if (ba2File.getFileSize(bnamPath, true) < 0L)
              continue;
          }
          try
          {
            BGSMFile& m = v[bnamPath];
            ba2File.extractFile(fileBuf, snamPath);
            FileBuffer  tmp(fileBuf.data(), fileBuf.size());
            m.loadBGSMFile(tmp);
            if (gradientMapV >= 0.0f)
              m.s.gradientMapV = gradientMapV;
            m.texturePaths.setMaterialPath(bnamPath);   // FIXME: or SNAM path?
          }
          catch (FO76UtilsError&)
          {
            v.erase(bnamPath);
          }
          if (n1 == std::string::npos)
            break;
        }
      }
      bnamPath.clear();
    }
  }
  if (v.begin() == v.end())
    return 0U;
  return formID;
}

void Renderer_Base::MaterialSwaps::materialSwap(
    Plot3D_TriShape& t, unsigned int formID) const
{
  if (BRANCH_UNLIKELY(!t.haveMaterialPath()))
    return;
  std::map< unsigned int,
            std::map< std::string, BGSMFile > >::const_iterator i =
      materialSwaps.find(formID);
  if (i == materialSwaps.end())
    return;
  std::map< std::string, BGSMFile >::const_iterator j =
      i->second.find(t.materialPath());
  if (j != i->second.end())
    t.setMaterial(j->second);
}

void Renderer_Base::TriShapeSortObject::orderedNodeFix(
    std::vector< TriShapeSortObject >& sortBuf,
    const std::vector< NIFFile::NIFTriShape >& meshData)
{
  if (sortBuf.size() < 2)
    return;
  size_t  j = sortBuf.size() - 1;
  size_t  i0 = size_t(sortBuf[j]);
  while (i0 > 0 && (meshData[i0].m.flags & BGSMFile::Flag_TSOrdered))
    i0--;
  std::uint64_t zMin = sortBuf[j].n & ~0xFFFFFFFFULL;
  std::uint64_t zMax = zMin;
  while (j-- > 0)
  {
    if (size_t(sortBuf[j]) < i0)
      break;
    std::uint64_t z = sortBuf[j].n & ~0xFFFFFFFFULL;
    zMin = (z < zMin ? z : zMin);
    zMax = (z > zMax ? z : zMax);
  }
  while (++j < sortBuf.size())
  {
    if (meshData[size_t(sortBuf[j])].m.flags & BGSMFile::Flag_TSAlphaBlending)
      sortBuf[j].n = (sortBuf[j].n & 0xFFFFFFFFU) | zMax;
    else
      sortBuf[j].n = (sortBuf[j].n & 0xFFFFFFFFU) | zMin;
  }
}

std::uint32_t Renderer_Base::getWaterColor(
    ESMFile& esmFile, const ESMFile::ESMRecord& r, unsigned int defaultColor)
{
  if (esmFile.getESMVersion() < 0x28)           // not implemented for
    return std::uint32_t(defaultColor);         // games older than Skyrim
  unsigned int  waterFormID = r.formID;
  ESMFile::ESMField f(esmFile, r);
  while (f.next())
  {
    if (((f == "WNAM" && r == "ACTI") || (f == "XCWT" && r == "CELL") ||
         (f == "NAM2" && r == "WRLD")) && f.size() >= 4)
    {
      waterFormID = f.readUInt32Fast();
    }
  }
  const ESMFile::ESMRecord  *r2 = esmFile.getRecordPtr(waterFormID);
  if (r2 && *r2 == "WATR")
  {
    ESMFile::ESMField f2(esmFile, *r2);
    while (f2.next())
    {
      if (!(f2 == "DNAM" && f2.size() >= 64))
        continue;
      FloatVector4  c;
      float   d = 0.0f;
      if (esmFile.getESMVersion() < 0x80)       // Skyrim
      {
        f2.setPosition(36);
        d = f2.readFloat();                     // fog distance (far plane)
        (void) f2.readUInt32Fast();             // shallow color
        c = FloatVector4(f2.readUInt32Fast());  // deep color
      }
      else if (esmFile.getESMVersion() < 0xC0)  // Fallout 4
      {
        d = f2.readFloat();                     // depth amount
        (void) f2.readUInt32Fast();             // shallow color
        c = FloatVector4(f2.readUInt32Fast());  // deep color
      }
      else                                      // Fallout 76
      {
        d = f2.readFloat();
        f2.setPosition(16);
        c = f2.readFloatVector4().srgbCompress();
      }
      d = (d > 0.125f ? (d < 8128.0f ? d : 8128.0f) : 0.125f);
      c[3] = 256.0f - float(std::sqrt(d * 8.0f));
      return std::uint32_t(c);
    }
  }
  return std::uint32_t(defaultColor);
}

