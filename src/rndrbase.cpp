
#include "common.hpp"
#include "fp32vec4.hpp"
#include "rndrbase.hpp"
#include "sfcube.hpp"

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
  CachedTextureKey  k;
  k.fd = ba2File.findFile(fileName);
  if (!k.fd)
    return (DDSTexture *) 0;
  k.mipLevel = mipLevel;
  textureCacheMutex.lock();
  std::map< CachedTextureKey, CachedTexture >::iterator i =
      textureCache.find(k);
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
      i = textureCache.insert(std::pair< CachedTextureKey, CachedTexture >(
                                  k, tmp)).first;
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
    size_t  fileBufSize = fileBuf.size();
    if (fileBufSize >= 148 &&
        (fileBuf[113] & 0x02) != 0 &&   // DDSCAPS2_CUBEMAP
        fileBuf[128] != 0x43)           // DXGI_FORMAT_R9G9B9E5_SHAREDEXP
    {
      SFCubeMapFilter cubeMapFilter(256);
      size_t  bufCapacityRequired = 256 * 256 * 8 * sizeof(std::uint32_t) + 148;
      if (fileBufSize < bufCapacityRequired)
        fileBuf.resize(bufCapacityRequired);
      size_t  newSize =
          cubeMapFilter.convertImage(fileBuf.data(), fileBufSize, false,
                                     bufCapacityRequired);
      fileBuf.resize(newSize);
    }
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
    std::map< CachedTextureKey, CachedTexture >::iterator i = firstTexture->i;
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

void Renderer_Base::TriShapeSortObject::orderedNodeFix(
    std::vector< TriShapeSortObject >& sortBuf,
    const std::vector< NIFFile::NIFTriShape >& meshData)
{
  if (sortBuf.size() < 2)
    return;
  size_t  j = sortBuf.size() - 1;
  size_t  i0 = size_t(sortBuf[j]);
  while (i0 > 0 && (meshData[i0].flags & NIFFile::NIFTriShape::Flag_TSOrdered))
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
    if (meshData[size_t(sortBuf[j])].flags & CE2Material::Flag_AlphaBlending)
      sortBuf[j].n = (sortBuf[j].n & 0xFFFFFFFFU) | zMax;
    else
      sortBuf[j].n = (sortBuf[j].n & 0xFFFFFFFFU) | zMin;
  }
}

unsigned int Renderer_Base::getWaterMaterial(
    std::map< unsigned int, WaterProperties >& m,
    ESMFile& esmFile, const ESMFile::ESMRecord *r,
    unsigned int defaultColor, bool storeAsDefault)
{
  unsigned int  waterFormID = 0U;
  if (r)
  {
    if (*r == "WATR")
    {
      waterFormID = r->formID;
    }
    else
    {
      ESMFile::ESMField f(esmFile, *r);
      while (f.next())
      {
        if (((f == "WTFM" && *r == "ACTI") || (f == "XCWT" && *r == "CELL") ||
             (f == "NAM3" && *r == "WRLD")) && f.size() >= 4)
        {
          waterFormID = f.readUInt32Fast();
          break;
        }
      }
    }
  }
  std::map< unsigned int, WaterProperties >::iterator i = m.find(waterFormID);
  if (i != m.end() && !(storeAsDefault && !waterFormID))
  {
    if (storeAsDefault)
      m[0U] = i->second;
    return waterFormID;
  }
  WaterProperties tmp;
  tmp.deepColor = std::uint32_t(defaultColor | 0xFF000000U);
  tmp.normalScale = 1.0f;
  tmp.reflectivity = 0.02032076f;
  tmp.specularScale = 1.0f;
  tmp.alphaDepth0 = FloatVector4(0.25f);
  tmp.texturePath = "textures/water/wavesdefault_normal.dds";
  bool    haveWaterParams = false;
  if (waterFormID && esmFile.getESMVersion() > 0xFF)    // Starfield
  {
    const ESMFile::ESMRecord  *r2 = esmFile.findRecord(waterFormID);
    if (r2 && *r2 == "WATR")
    {
      ESMFile::ESMField f(*r2, esmFile);
      while (f.next())
      {
        if (f == "DNAM" && f.size() >= 64)
        {
          float   d = std::min(std::max(f.readFloat(), 0.01f), 200.0f);
          f.setPosition(f.getPosition() + 12);          // skip 3 unknown floats
          FloatVector4  a(f.readFloatVector4());        // absorption
          a.maxValues(FloatVector4(0.0f)).minValues(FloatVector4(100.0f));
          tmp.deepColor = f.readUInt32Fast() | 0xFF000000U;
          tmp.depthMult = a * (0.3321928f / d);
          haveWaterParams = true;
        }
      }
    }
  }
  if (!haveWaterParams)
  {
    float   d = float(int((defaultColor >> 24) & 0xFFU));
    d = d * d * (1.0f / 512.0f) + 128.0f - d;
    FloatVector4  a(tmp.deepColor);
    a.srgbExpand().maxValues(FloatVector4(0.000005f));
    tmp.depthMult[0] = float(std::log2(a[0]));
    tmp.depthMult[1] = float(std::log2(a[1]));
    tmp.depthMult[2] = float(std::log2(a[2]));
    tmp.depthMult[3] = 0.0f;
    tmp.depthMult *= (-4.0f / d);
  }
  if (i == m.end())
  {
    i = m.insert(std::pair< unsigned int, WaterProperties >(
                     waterFormID, tmp)).first;
  }
  if (storeAsDefault)
  {
    if (!waterFormID)
      i->second = tmp;
    else
      m[0U] = tmp;
  }
  return waterFormID;
}

