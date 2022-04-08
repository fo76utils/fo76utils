
#include "common.hpp"
#include "render.hpp"

#include <algorithm>

bool Renderer::RenderObject::operator<(const RenderObject& r) const
{
  unsigned int  modelMask1 = ~(Renderer::modelBatchCnt - 1U);
  if ((modelID & modelMask1) != (r.modelID & modelMask1))
    return ((modelID & modelMask1) < (r.modelID & modelMask1));
  if ((flags & 7U) != (r.flags & 7U))
    return ((flags & 7U) < (r.flags & 7U));
  if (tileIndex != r.tileIndex)
    return (tileIndex < r.tileIndex);
  unsigned int  modelMask2 = Renderer::modelBatchCnt - 1U;
  if ((modelID & modelMask2) != (r.modelID & modelMask2))
    return ((modelID & modelMask2) < (r.modelID & modelMask2));
  if (formID != r.formID)
    return (formID < r.formID);
  if (y0 != r.y0)
    return (y0 < r.y0);
  return (x0 < r.x0);
}

Renderer::ModelData::ModelData()
  : nifFile((NIFFile *) 0),
    totalTriangleCnt(0)
{
}

Renderer::ModelData::~ModelData()
{
  clear();
}

void Renderer::ModelData::clear()
{
  if (nifFile)
  {
    delete nifFile;
    nifFile = (NIFFile *) 0;
  }
  totalTriangleCnt = 0;
  meshData.clear();
  textures.clear();
}

Renderer::RenderThread::RenderThread()
  : t((std::thread *) 0),
    terrainMesh((TerrainMesh *) 0),
    renderer((Plot3D_TriShape *) 0)
{
}

Renderer::RenderThread::~RenderThread()
{
  if (renderer)
  {
    delete renderer;
    renderer = (Plot3D_TriShape *) 0;
  }
  clear();
}

void Renderer::RenderThread::join()
{
  if (t)
  {
    t->join();
    delete t;
    t = (std::thread *) 0;
  }
}

void Renderer::RenderThread::clear()
{
  join();
  errMsg.clear();
  if (terrainMesh)
  {
    delete terrainMesh;
    terrainMesh = (TerrainMesh *) 0;
  }
}

unsigned long long Renderer::calculateTileMask(int x0, int y0,
                                               int x1, int y1) const
{
  unsigned int  xMask = 0U;
  for (int i = 0; i < 8; i++)
  {
    if (x0 < ((width * ((i & 7) + 1)) >> 3) && x1 >= ((width * (i & 7)) >> 3))
      xMask = xMask | (1U << (unsigned int) i);
  }
  unsigned long long  m = 0ULL;
  for (int i = 0; i < 8; i++)
  {
    if (y0 < ((height * ((i & 7) + 1)) >> 3) && y1 >= ((height * (i & 7)) >> 3))
      m = m | ((unsigned long long) xMask << (unsigned int) (i << 3));
  }
  return m;
}

int Renderer::calculateTileIndex(unsigned long long screenAreasUsed)
{
  if (!screenAreasUsed)
    return -1;
  if (!(screenAreasUsed & (screenAreasUsed - 1ULL)))
  {
    unsigned int  n0 = 0;
    unsigned int  n2 = 64;
    while ((1ULL << n0) < screenAreasUsed)
    {
      unsigned int  n1 = (n0 + n2) >> 1;
      if ((1ULL << n1) > screenAreasUsed)
        n2 = n1;
      else
        n0 = n1;
    }
    return int(n0);
  }
  unsigned long long  m = screenAreasUsed;
  int     x0 = 7;
  int     x1 = 0;
  for (int x = 0; m; x++, m = ((m >> 1) & 0x7F7F7F7F7F7F7F7FULL))
  {
    if (m & 0x0101010101010101ULL)
    {
      x0 = (x < x0 ? x : x0);
      x1 = x;
    }
  }
  m = screenAreasUsed;
  int     y0 = 7;
  int     y1 = 0;
  for (int y = 0; m; y++, m = ((m >> 8) & 0x00FFFFFFFFFFFFFFULL))
  {
    if (m & 0xFFULL)
    {
      y0 = (y < y0 ? y : y0);
      y1 = y;
    }
  }
  if ((x1 - x0) <= 1 && (y1 - y0) <= 1)
  {
    return (((y0 & 6) << 3) + (x0 & 6) + 64
            + ((y0 & 1) << 7) + ((x0 & 1) << 6));
  }
  if ((x1 - x0) <= 3 && (y1 - y0) <= 3)
  {
    return (((y0 & 4) << 3) + (x0 & 4) + 320
            + ((y0 & 3) << 8) + ((x0 & 3) << 6));
  }
  return 1344;
}

int Renderer::setScreenAreaUsed(RenderObject& p)
{
  p.tileIndex = -1;
  NIFFile::NIFVertexTransform vt;
  NIFFile::NIFBounds  modelBounds;
  if (p.flags & 1U)
  {
    if (!landData)
      return -1;
    float   xyScale = 4096.0f / float(landData->getCellResolution());
    float   xOffset = -xyScale * float(landData->getOriginX());
    float   yOffset = xyScale * float(landData->getOriginY());
    float   zScale = (landData->getZMax() - landData->getZMin()) / 65535.0f;
    float   zOffset = landData->getZMin();
    int     x0 = (p.x0 < p.x1 ? p.x0 : p.x1);
    int     y0 = (p.y0 < p.y1 ? p.y0 : p.y1);
    int     x1 = x0 + std::abs(int(p.x1) - int(p.x0));
    int     y1 = y0 + std::abs(int(p.y1) - int(p.y0));
    modelBounds.xMin = float(x0) * xyScale + xOffset;
    modelBounds.yMin = float(y1) * -xyScale + yOffset;
    modelBounds.xMax = float(x1) * xyScale + xOffset;
    modelBounds.yMax = float(y0) * -xyScale + yOffset;
    int     w = landData->getImageWidth();
    int     h = landData->getImageHeight();
    unsigned short  z0 = 0xFFFF;
    unsigned short  z1 = 0;
    for (int y = y0; y <= y1; y++)
    {
      int     yc = (y > 0 ? (y < (h - 1) ? y : (h - 1)) : 0);
      const unsigned short  *srcPtr = landData->getHeightMap();
      srcPtr = srcPtr + (size_t(yc) * size_t(w));
      for (int x = x0; x <= x1; x++)
      {
        int     xc = (x > 0 ? (x < (w - 1) ? x : (w - 1)) : 0);
        unsigned short  z = srcPtr[xc];
        z0 = (z < z0 ? z : z0);
        z1 = (z > z1 ? z : z1);
      }
    }
    modelBounds.zMin = float(int(z0)) * zScale + zOffset;
    modelBounds.zMax = float(int(z1)) * zScale + zOffset;
    vt = viewTransform;
  }
  else if (p.flags & 6U)
  {
    int     x0 = int(p.x0 < p.x1 ? p.x0 : p.x1) - 2;
    int     x1 = x0 + std::abs(int(p.x1) - int(p.x0)) + 4;
    int     y0 = int(p.y0 < p.y1 ? p.y0 : p.y1) - 2;
    int     y1 = y0 + std::abs(int(p.y1) - int(p.y0)) + 4;
    int     z0 = int(p.z0 < p.z1 ? p.z0 : p.z1) - 2;
    int     z1 = z0 + std::abs(int(p.z1) - int(p.z0)) + 4;
    modelBounds.xMin = float(x0);
    modelBounds.yMin = float(y0);
    modelBounds.zMin = float(z0);
    modelBounds.xMax = float(x1);
    modelBounds.yMax = float(y1);
    modelBounds.zMax = float(z1);
    vt = p.modelTransform;
    vt *= viewTransform;
  }
  else
  {
    return -1;
  }
  NIFFile::NIFBounds  screenBounds;
  NIFFile::NIFVertex  v;
  for (int i = 0; i < 8; i++)
  {
    v.x = (!(i & 1) ? modelBounds.xMin : modelBounds.xMax);
    v.y = (!(i & 2) ? modelBounds.yMin : modelBounds.yMax);
    v.z = (!(i & 4) ? modelBounds.zMin : modelBounds.zMax);
    vt.transformXYZ(v.x, v.y, v.z);
    screenBounds += v;
  }
  v.x = screenBounds.xMin;
  v.y = screenBounds.yMin;
  v.z = screenBounds.zMin;
  worldBounds += v;
  v.x = screenBounds.xMax;
  v.y = screenBounds.yMax;
  v.z = screenBounds.zMax;
  worldBounds += v;
  int     xMin = roundFloat(screenBounds.xMin - 2.0f);
  int     yMin = roundFloat(screenBounds.yMin - 2.0f);
  int     zMin = roundFloat(screenBounds.zMin - 2.0f);
  int     xMax = roundFloat(screenBounds.xMax + 2.0f);
  int     yMax = roundFloat(screenBounds.yMax + 2.0f);
  int     zMax = roundFloat(screenBounds.zMax + 2.0f);
  if (xMin >= width || xMax < 0 || yMin >= height || yMax < 0 ||
      zMin >= 16777216 || zMax < 0)
  {
    return -1;
  }
  p.tileIndex = calculateTileIndex(calculateTileMask(xMin, yMin, xMax, yMax));
  return p.tileIndex;
}

unsigned int Renderer::getDefaultWorldID() const
{
  if (landData && landData->getLandTexture16())
    return 0x0025DA15U;
  return 0x0000003CU;
}

void Renderer::addTerrainCell(const ESMFile::ESMRecord& r)
{
  if (!(r == "CELL" && landData))
    return;
  int     cellX = 0;
  int     cellY = 0;
  unsigned int  cellFlags = 0;
  ESMFile::ESMField f(esmFile, r);
  while (f.next())
  {
    if (f == "DATA" && f.size() >= 1)
    {
      cellFlags = f.readUInt8Fast();
    }
    else if (f == "XCLC" && f.size() >= 8)
    {
      cellX = f.readInt32();
      cellY = f.readInt32();
    }
  }
  if (cellFlags & 0x01)                 // interior cell
    return;
  RenderObject  tmp;
  tmp.flags = 0x0001;                   // terrain
  tmp.modelID = 0;
  tmp.formID = 0;
  tmp.tileIndex = -1;
  tmp.z0 = 0;
  tmp.z1 = 0;
  int     w = landData->getImageWidth();
  int     h = landData->getImageHeight();
  int     n = landData->getCellResolution();
  int     x0 = landData->getOriginX() + (cellX * n);
  int     y0 = landData->getOriginY() - ((cellY + 1) * n);
  int     x1 = x0 + n;
  int     y1 = y0 + n;
  if (n > 24)
    n = n - 8;
  int     y = y0 - (y0 % n);
  y = (y >= 0 ? y : (y - n));
  for (int i = 0; i < 3; i++, y = y + n)
  {
    int     y2 = y + n;
    if (!(y >= y0 && y < y1 && y < h && y2 > 0))
      continue;
    int     x = x0 - (x0 % n);
    x = (x >= 0 ? x : (x - n));
    for (int j = 0; j < 3; j++, x = x + n)
    {
      int     x2 = x + n;
      if (!(x >= x0 && x < x1 && x < w && x2 > 0))
        continue;
      tmp.x0 = (signed short) (x > 0 ? x : 0);
      tmp.y0 = (signed short) (y > 0 ? y : 0);
      tmp.x1 = (signed short) (x2 < w ? x2 : w);
      tmp.y1 = (signed short) (y2 < h ? y2 : h);
      if (setScreenAreaUsed(tmp) >= 0)
        objectList.push_back(tmp);
    }
  }
}

void Renderer::addWaterCell(const ESMFile::ESMRecord& r)
{
  if (!(r == "CELL"))
    return;
  int     cellX = 0;
  int     cellY = 0;
  unsigned int  cellFlags = 0;
  float   waterLevel = 1.0e12f;
  ESMFile::ESMField f(esmFile, r);
  while (f.next())
  {
    if (f == "DATA" && f.size() >= 1)
    {
      cellFlags = f.readUInt8Fast();
    }
    else if (f == "XCLC" && f.size() >= 8)
    {
      cellX = f.readInt32();
      cellY = f.readInt32();
    }
    else if (f == "XCLW" && f.size() >= 4)
    {
      waterLevel = f.readFloat();
    }
  }
  if (!(cellFlags & 0x02))              // cell has no water
    return;
  if (!(waterLevel >= -1048576.0f && waterLevel <= 1048576.0f))
    waterLevel = defaultWaterLevel;
  RenderObject  tmp;
  tmp.flags = 0x0004;                   // water
  tmp.modelID = 0;
  tmp.formID = 0;
  tmp.tileIndex = -1;
  tmp.modelTransform.offsX = float(cellX) * 4096.0f;
  tmp.modelTransform.offsY = float(cellY) * 4096.0f;
  tmp.modelTransform.offsZ = waterLevel;
  tmp.x0 = 0;
  tmp.y0 = 0;
  tmp.z0 = 0;
  tmp.x1 = 4096;
  tmp.y1 = 4096;
  tmp.z1 = 0;
  if (setScreenAreaUsed(tmp) >= 0)
    objectList.push_back(tmp);
}

void Renderer::findObjects(unsigned int formID, int type)
{
  if (!formID)
    return;
  const ESMFile::ESMRecord  *r = (ESMFile::ESMRecord *) 0;
  do
  {
    r = esmFile.getRecordPtr(formID);
    if (!r)
      break;
    if (BRANCH_EXPECT(!(*r == "REFR"), false))
    {
      if (*r == "GRUP")
      {
        if (r->formID >= 1U && r->formID <= 9U && r->formID != 7U)
          findObjects(r->children, type);
      }
      else if (*r == "CELL")
      {
        if (type == 0)
          addTerrainCell(*r);
        else if (type == 2)
          addWaterCell(*r);
      }
      else if (*r == "WRLD")
      {
        if (!r->next || !(r = esmFile.getRecordPtr(r->next)))
          return;
        if (!(*r == "GRUP" && r->formID == 1))          // world children
          return;
        if (!(formID = r->children) || !(r = esmFile.getRecordPtr(formID)))
          return;
        if (type == 0)
        {
          while (!(*r == "GRUP" && r->formID == 4))     // exterior cell block
          {
            if (!(formID = r->next) || !(r = esmFile.getRecordPtr(formID)))
              return;
          }
        }
        findObjects(formID, type);
        return;
      }
      continue;
    }
    if ((r->flags & 0x00000020) || !type ||     // ignore deleted records
        (noDisabledObjects && (r->flags & 0x00000800)))
    {
      continue;
    }
    unsigned int  refrName = 0U;
    float   scale = 1.0f;
    float   rX = 0.0f;
    float   rY = 0.0f;
    float   rZ = 0.0f;
    float   offsX = 0.0f;
    float   offsY = 0.0f;
    float   offsZ = 0.0f;
    {
      ESMFile::ESMField f(esmFile, *r);
      while (f.next())
      {
        if (f == "NAME" && f.size() >= 4)
        {
          refrName = f.readUInt32Fast();
        }
        else if (f == "DATA" && f.size() >= 24)
        {
          offsX = f.readFloat();
          offsY = f.readFloat();
          offsZ = f.readFloat();
          rX = f.readFloat();
          rY = f.readFloat();
          rZ = f.readFloat();
        }
        else if (f == "XSCL" && f.size() >= 4)
        {
          scale = f.readFloat();
        }
      }
    }
    const ESMFile::ESMRecord  *r2;
    if (!(refrName && bool(r2 = esmFile.getRecordPtr(refrName))))
      continue;
    if (*r2 == "STAT" || *r2 == "SCOL" || *r2 == "TREE")
    {
      if (distantObjectsOnly && !(r2->flags & 0x00008000))
        continue;
    }
    else if (!(*r2 == "MSTT" || *r2 == "ACTI" || *r2 == "PWAT"))
    {
      continue;
    }
    RenderObject  tmp;
    tmp.flags = (type == 1 ? 2U : 4U);
    tmp.formID = formID;
    tmp.tileIndex = -1;
    bool    haveOBND = false;
    bool    isWater = (*r2 == "PWAT");
    bool    isHDModel = false;
    stringBuf.clear();
    {
      ESMFile::ESMField f(esmFile, *r2);
      while (f.next())
      {
        if (f == "OBND" && f.size() >= 6)
        {
          tmp.x0 = (signed short) uint16ToSigned(f.readUInt16Fast());
          tmp.y0 = (signed short) uint16ToSigned(f.readUInt16Fast());
          tmp.z0 = (signed short) uint16ToSigned(f.readUInt16Fast());
          tmp.x1 = (signed short) uint16ToSigned(f.readUInt16Fast());
          tmp.y1 = (signed short) uint16ToSigned(f.readUInt16Fast());
          tmp.z1 = (signed short) uint16ToSigned(f.readUInt16Fast());
          haveOBND = true;
        }
        else if (f == "MODL" && f.size() > 1 &&
                 (!modelLOD || stringBuf.empty()))
        {
          f.readPath(stringBuf, std::string::npos, "meshes/", ".nif");
          isHDModel = isHighQualityModel(stringBuf);
        }
        else if (f == "MNAM" && f.size() == 1040 && modelLOD && !isHDModel)
        {
          for (int i = (modelLOD - 1) & 3; i >= 0; i--)
          {
            f.setPosition(size_t(i * 260));
            if (f[f.getPosition()] != 0)
            {
              f.readPath(stringBuf, std::string::npos, "meshes/", ".nif");
              if (!stringBuf.empty())
                break;
            }
          }
        }
        else if (f == "WNAM" && *r2 == "ACTI" && f.size() >= 4)
        {
          isWater = true;
        }
      }
    }
    if (!isWater)
    {
      if (std::strncmp(stringBuf.c_str(), "meshes/water/", 13) == 0 ||
          stringBuf.find("/fxwaterfall") != std::string::npos)
      {
        isWater = true;
      }
    }
    if (!haveOBND || stringBuf.empty() || (type == 2) != isWater)
      continue;
    if (*r2 == "ACTI" && !isWater)
      continue;
    tmp.modelTransform = NIFFile::NIFVertexTransform(scale, rX, rY, rZ,
                                                     offsX, offsY, offsZ);
    if (setScreenAreaUsed(tmp) < 0)
      continue;
    std::map< std::string, unsigned int >::iterator i =
        modelPathMap.find(stringBuf);
    if (i == modelPathMap.end())
    {
      unsigned int  n = (unsigned int) modelPathMap.size();
      i = modelPathMap.insert(std::pair< std::string, unsigned int >(
                                  stringBuf, n)).first;
    }
    tmp.modelID = i->second;
    objectList.push_back(tmp);
  }
  while ((formID = r->next) != 0U);
}

void Renderer::sortObjectList()
{
  modelPaths.clear();
  size_t  modelCnt = modelPathMap.size();
  if (modelCnt > 0)
  {
    modelPaths.resize(modelCnt, (std::string *) 0);
    std::vector< unsigned int > modelIDTable(modelCnt, 0U);
    unsigned int  j = 0U;
    for (std::map< std::string, unsigned int >::const_iterator
             i = modelPathMap.begin(); i != modelPathMap.end(); i++, j++)
    {
      modelIDTable[i->second] = j;
      modelPaths[j] = &(i->first);
    }
    for (size_t i = 0; i < objectList.size(); i++)
      objectList[i].modelID = modelIDTable[objectList[i].modelID];
  }
  std::sort(objectList.begin(), objectList.end());
#if 0
  std::printf("Object list size = %6u\n", (unsigned int) objectList.size());
  for (size_t i = 0; i < objectList.size(); i++)
  {
    std::printf("#%06u: 0x%04X, 0x%08X, %6u, %4d, (%6d, %6d), (%6d, %6d)\n",
                (unsigned int) i, objectList[i].flags, objectList[i].formID,
                objectList[i].modelID, objectList[i].tileIndex,
                int(objectList[i].x0), int(objectList[i].y0),
                int(objectList[i].x1), int(objectList[i].y1));
  }
#endif
}

void Renderer::clear(unsigned int flags)
{
  if (flags & 0x03)
  {
    size_t  imageDataSize = size_t(width) * size_t(height);
    if (flags & 0x01)
    {
      for (size_t i = 0; i < imageDataSize; i++)
        outBufRGBW[i] = 0U;
    }
    if (flags & 0x02)
    {
      for (size_t i = 0; i < imageDataSize; i++)
        outBufZ[i] = 16777216.0f;
    }
  }
  if (flags & 0x04)
  {
    if (landData)
    {
      delete landData;
      landData = (LandscapeData *) 0;
    }
    landTextures.clear();
  }
  if (flags & 0x08)
  {
    objectList.clear();
    modelPathMap.clear();
    modelPaths.clear();
  }
  if (flags & 0x10)
  {
    for (size_t i = 0; i < renderThreads.size(); i++)
      renderThreads[i].clear();
  }
  if (flags & 0x20)
  {
    for (size_t i = 0; i < nifFiles.size(); i++)
      nifFiles[i].clear();
  }
  if (flags & 0x40)
  {
    textureDataSize = 0;
    firstTexture = (CachedTexture *) 0;
    lastTexture = (CachedTexture *) 0;
    for (std::map< std::string, CachedTexture >::iterator
             i = textureCache.begin(); i != textureCache.end(); i++)
    {
      if (i->second.texture)
        delete i->second.texture;
    }
    textureCache.clear();
  }
}

size_t Renderer::getTextureDataSize(const DDSTexture *t)
{
  if (!t)
    return 0;
  size_t  n =
      size_t(t->getWidth()) * size_t(t->getHeight()) * sizeof(unsigned int);
  size_t  dataSize = 1024;
  for ( ; n > 0; n = n >> 2)
    dataSize = dataSize + n;
  return dataSize;
}

const DDSTexture * Renderer::loadTexture(const std::string& fileName)
{
  if (fileName.find("/temp_ground") != std::string::npos)
    return (DDSTexture *) 0;
  std::map< std::string, CachedTexture >::iterator  i =
      textureCache.find(fileName);
  if (i != textureCache.end())
  {
    if (i->second.prv)
      i->second.prv->nxt = i->second.nxt;
    else
      firstTexture = i->second.nxt;
    if (i->second.nxt)
      i->second.nxt->prv = i->second.prv;
    else
      lastTexture = i->second.prv;
  }
  else
  {
    DDSTexture  *t = (DDSTexture *) 0;
    try
    {
      int     m = ba2File.extractTexture(fileBuf, fileName, textureMip);
      t = new DDSTexture(&(fileBuf.front()), fileBuf.size(), m);
      CachedTexture tmp;
      tmp.texture = t;
      tmp.i = textureCache.end();
      tmp.prv = (CachedTexture *) 0;
      tmp.nxt = (CachedTexture *) 0;
      i = textureCache.insert(std::pair< std::string, CachedTexture >(
                                  fileName, tmp)).first;
      i->second.i = i;
      textureDataSize = textureDataSize + getTextureDataSize(t);
    }
    catch (std::runtime_error&)
    {
      if (t)
        delete t;
      return (DDSTexture *) 0;
    }
    catch (...)
    {
      if (t)
        delete t;
      throw;
    }
  }
  if (lastTexture)
    lastTexture->nxt = &(i->second);
  else
    firstTexture = &(i->second);
  i->second.prv = lastTexture;
  i->second.nxt = (CachedTexture *) 0;
  lastTexture = &(i->second);
  return i->second.texture;
}

void Renderer::shrinkTextureCache()
{
  while (textureDataSize > textureCacheSize && firstTexture)
  {
    size_t  dataSize = getTextureDataSize(firstTexture->texture);
    if (firstTexture->texture)
      delete firstTexture->texture;
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

bool Renderer::isHighQualityModel(const std::string& modelPath) const
{
  for (size_t i = 0; i < hdModelNamePatterns.size(); i++)
  {
    if (modelPath.find(hdModelNamePatterns[i]) != std::string::npos)
      return true;
  }
  return false;
}

void Renderer::loadModel(unsigned int modelID)
{
  size_t  n = modelID & (modelBatchCnt - 1U);
  nifFiles[n].clear();
  if (modelID >= modelPaths.size() || modelPaths[modelID]->empty())
    return;
  try
  {
    ba2File.extractFile(fileBuf, *(modelPaths[modelID]));
    nifFiles[n].nifFile =
        new NIFFile(&(fileBuf.front()), fileBuf.size(), &ba2File);
    bool    isHDModel = isHighQualityModel(*(modelPaths[modelID]));
    nifFiles[n].nifFile->getMesh(nifFiles[n].meshData, 0U,
                                 (unsigned int) (modelLOD > 0 && !isHDModel));
    size_t  meshCnt = nifFiles[n].meshData.size();
    nifFiles[n].textures.resize(meshCnt * 10U, (DDSTexture *) 0);
    for (size_t i = 0; i < meshCnt; i++)
    {
      const NIFFile::NIFTriShape& t = nifFiles[n].meshData[i];
      if (t.flags & 0x05)               // ignore if hidden or effect
        continue;
      nifFiles[n].totalTriangleCnt += t.triangleCnt;
      size_t  nTextures = size_t(!(t.flags & 0x02) ? (!isHDModel ? 4 : 10) : 0);
      for (size_t j = nTextures; j-- > 0; )
      {
        const std::string *texturePath = (std::string *) 0;
        if (j < t.texturePathCnt &&
            t.texturePaths[j] && !t.texturePaths[j]->empty())
        {
          texturePath = t.texturePaths[j];
        }
        else if (j == 4 && nifFiles[n].textures[i * 10U + 8U])
        {
          texturePath = &defaultEnvMap;
        }
        if (texturePath && !texturePath->empty())
          nifFiles[n].textures[i * 10U + j] = loadTexture(*texturePath);
        if (j > 1 && !isHDModel)
          j = 1;
      }
    }
  }
  catch (std::runtime_error&)
  {
    nifFiles[n].clear();
  }
}

void Renderer::renderObjectList()
{
  clear(0x30);
  std::vector< size_t > threadTriangleCnt(64, 0);
  std::vector< unsigned long long > threadTileMask(64, 0ULL);
  if (landData)
  {
    for (size_t i = 0; i < renderThreads.size(); i++)
    {
      if (!renderThreads[i].terrainMesh)
        renderThreads[i].terrainMesh = new TerrainMesh();
    }
  }
  for (size_t i = 0; i < objectList.size(); )
  {
    for (size_t k = 0; k < 64; k++)
    {
      threadTriangleCnt[k] = 0;
      threadTileMask[k] = 1ULL << (unsigned int) k;
    }
    size_t  j = i;
    unsigned int  modelIDMask = modelBatchCnt - 1U;
    while (j < objectList.size() &&
           !((objectList[i].tileIndex ^ objectList[j].tileIndex) & ~63) &&
           !((objectList[i].modelID ^ objectList[j].modelID) & ~modelIDMask))
    {
      const RenderObject& p = objectList[j];
      size_t  triangleCnt = 2;
      if ((p.flags & 0x01) && landData)
      {
        triangleCnt = size_t(landData->getCellResolution());
        if (triangleCnt > 24)
          triangleCnt = triangleCnt - 8;
        triangleCnt = triangleCnt * triangleCnt * 2U;
      }
      else if (p.formID)
      {
        if (!nifFiles[p.modelID & modelIDMask].totalTriangleCnt)
        {
          loadModel(p.modelID);
          if (!nifFiles[p.modelID & modelIDMask].totalTriangleCnt)
            nifFiles[p.modelID & modelIDMask].totalTriangleCnt = 1;
        }
        triangleCnt = nifFiles[p.modelID & modelIDMask].totalTriangleCnt;
      }
      threadTriangleCnt[p.tileIndex & 63] += triangleCnt;
      j++;
    }
    size_t  threadsUsed = 64;
    while (int(threadsUsed) > threadCnt)
    {
      // merge the areas with the lowest triangle counts
      // until there are not more left than the number of threads
      size_t  minCnt1 = 0x7FFFFFFF;
      size_t  minCnt2 = 0x7FFFFFFF;
      size_t  minCntThread1 = 0;
      size_t  minCntThread2 = 1;
      for (size_t k = 0; k < threadsUsed && minCnt2 > 0; k++)
      {
        if (threadTriangleCnt[k] < minCnt1)
        {
          minCnt2 = minCnt1;
          minCntThread2 = minCntThread1;
          minCnt1 = threadTriangleCnt[k];
          minCntThread1 = k;
        }
        else if (threadTriangleCnt[k] < minCnt2)
        {
          minCnt2 = threadTriangleCnt[k];
          minCntThread2 = k;
        }
      }
      threadTriangleCnt[minCntThread1] += threadTriangleCnt[minCntThread2];
      threadTileMask[minCntThread1] |= threadTileMask[minCntThread2];
      threadsUsed--;
      threadTriangleCnt[minCntThread2] = threadTriangleCnt[threadsUsed];
      threadTileMask[minCntThread2] = threadTileMask[threadsUsed];
    }
    for (size_t k = 0; k < threadsUsed; k++)
    {
      if (threadTriangleCnt[k] > 0)
      {
        renderThreads[k].t = new std::thread(threadFunction, this, k, i, j,
                                             threadTileMask[k]);
      }
    }
    for (size_t k = 0; k < threadsUsed; k++)
      renderThreads[k].join();
    for (size_t k = 0; k < threadsUsed; k++)
    {
      if (!renderThreads[k].errMsg.empty())
        throw errorMessage("%s", renderThreads[k].errMsg.c_str());
    }
    // render any remaining objects that were skipped due to incorrect bounds
    renderThread(0, i, j, ~0ULL);
    i = j;
    clear(0x20);
    shrinkTextureCache();
    if (verboseMode)
    {
      std::fprintf(stderr, "\r    %7u / %7u  ",
                   (unsigned int) i, (unsigned int) objectList.size());
    }
  }
  if (verboseMode)
    std::fputc('\n', stderr);
}

void Renderer::renderThread(size_t threadNum, size_t startPos, size_t endPos,
                            unsigned long long tileIndexMask)
{
  RenderThread& t = renderThreads[threadNum];
  if (!t.renderer)
    return;
  t.renderer->setLightingFunction(lightingPolynomial);
  int     landTxtScale = 0;
  if (landData)
  {
    while ((landData->getCellResolution() << landTxtScale)
           < cellTextureResolution && landTxtScale < 8)
    {
      landTxtScale++;
    }
  }
  unsigned long long  tileMask = 0ULL;
  for (size_t i = startPos; i < endPos; i++)
  {
    int     tileIndex = objectList[i].tileIndex;
    if (tileIndex < 0 ||
        !((1ULL << (unsigned int) (tileIndex & 63)) & tileIndexMask) ||
        ((1ULL << (unsigned int) (tileIndex & 63)) & ~tileIndexMask))
    {
      continue;
    }
    if (BRANCH_EXPECT(!tileMask, false))
    {
      // calculate mask of areas this thread is allowed to access
      if (tileIndex < 64 || tileIndexMask == ~0ULL)
      {
        tileMask = tileIndexMask;
      }
      else if (tileIndex < 1344)
      {
        for (int l = 0; l < 64; l++)
        {
          if (!(tileIndexMask & (1ULL << (unsigned int) l)))
            continue;
          int     n = (tileIndex & ~63) + l - (tileIndex < 320 ? 64 : 320);
          int     x0 = (n & 6) + ((n >> 6) & (tileIndex < 320 ? 1 : 3));
          int     y0 = ((n >> 3) & 6) + ((n >> (tileIndex < 320 ? 7 : 8)) & 3);
          n = (tileIndex < 320 ? 2 : 4);
          for (int j = 0; j < n && (y0 + j) < 8; j++)
          {
            for (int k = 0; k < n && (x0 + k) < 8; k++)
              tileMask = tileMask | (1ULL << (unsigned int) ((j << 3) | k));
          }
        }
      }
      else
      {
        tileMask = ~0ULL;
      }
    }
    bool    invalidOBND = false;
    if (objectList[i].flags & 0x01)             // terrain
    {
      if (landData && t.terrainMesh)
      {
        t.terrainMesh->createMesh(
            *landData, landTxtScale,
            objectList[i].x0, objectList[i].y0,
            objectList[i].x1, objectList[i].y1,
            &(landTextures.front()), landTextures.size(),
            landTextureMip, landTxtRGBScale, landTxtDefColor);
        *(t.renderer) = *(t.terrainMesh);
        t.renderer->drawTriShape(
            objectList[i].modelTransform, viewTransform, lightX, lightY, lightZ,
            t.terrainMesh->getTextures(), t.terrainMesh->getTextureCount());
      }
    }
    else if (objectList[i].formID)              // object or water mesh
    {
      NIFFile::NIFVertexTransform vt(objectList[i].modelTransform);
      vt *= viewTransform;
      size_t  n = objectList[i].modelID & (modelBatchCnt - 1U);
      for (size_t j = 0; j < nifFiles[n].meshData.size(); j++)
      {
        if (nifFiles[n].meshData[j].flags & 0x05)       // hidden or effect
          continue;
        if (nifFiles[n].meshData[j].triangleCnt < 1)
          continue;
        NIFFile::NIFBounds  b;
        nifFiles[n].meshData[j].calculateBounds(b, &vt);
        unsigned long long  m =
            calculateTileMask(roundFloat(b.xMin), roundFloat(b.yMin),
                              roundFloat(b.xMax), roundFloat(b.yMax));
        if (!m)
          continue;
        if (m & ~tileMask)
        {
          invalidOBND = true;
          break;
        }
        *(t.renderer) = nifFiles[n].meshData[j];
        t.renderer->drawTriShape(
            objectList[i].modelTransform, viewTransform, lightX, lightY, lightZ,
            &(nifFiles[n].textures.front()) + (j * 10U), 10);
      }
    }
    else if (objectList[i].flags & 0x04)        // water cell
    {
      NIFFile::NIFTriShape  tmp;
      NIFFile::NIFVertex    vTmp[4];
      NIFFile::NIFTriangle  tTmp[2];
      int     x0 = objectList[i].x0;
      int     y0 = objectList[i].y0;
      int     x1 = objectList[i].x1;
      int     y1 = objectList[i].y1;
      for (int j = 0; j < 4; j++)
      {
        vTmp[j].x = float(j == 0 || j == 3 ? x0 : x1);
        vTmp[j].y = float(j == 0 || j == 1 ? y0 : y1);
        vTmp[j].z = 0.0f;
        vTmp[j].normalX = 0.0f;
        vTmp[j].normalY = 0.0f;
        vTmp[j].normalZ = 1.0f;
        vTmp[j].u = (unsigned short) (j == 0 || j == 3 ? 0x0000 : 0x3C00);
        vTmp[j].v = (unsigned short) (j == 0 || j == 1 ? 0x0000 : 0x3C00);
        vTmp[j].vertexColor = 0xFFFFFFFFU;
      }
      tTmp[0].v0 = 0;
      tTmp[0].v1 = 1;
      tTmp[0].v2 = 2;
      tTmp[1].v0 = 0;
      tTmp[1].v1 = 2;
      tTmp[1].v2 = 3;
      tmp.vertexCnt = 4;
      tmp.triangleCnt = 2;
      tmp.vertexData = vTmp;
      tmp.triangleData = tTmp;
      tmp.flags = 0x32;                 // water
      tmp.texturePathCnt = 0;
      tmp.texturePaths = (std::string **) 0;
      *(t.renderer) = tmp;
      t.renderer->drawTriShape(
          objectList[i].modelTransform, viewTransform, lightX, lightY, lightZ,
          (DDSTexture **) 0, 0);
    }
    if (!invalidOBND)
      objectList[i].tileIndex = -1;
  }
}

void Renderer::threadFunction(Renderer *p, size_t threadNum,
                              size_t startPos, size_t endPos,
                              unsigned long long tileIndexMask)
{
  try
  {
    p->renderThreads[threadNum].errMsg.clear();
    p->renderThread(threadNum, startPos, endPos, tileIndexMask);
  }
  catch (std::exception& e)
  {
    p->renderThreads[threadNum].errMsg = e.what();
    if (p->renderThreads[threadNum].errMsg.empty())
      p->renderThreads[threadNum].errMsg = "unknown error in render thread";
  }
}

Renderer::Renderer(int imageWidth, int imageHeight,
                   const BA2File& archiveFiles, ESMFile& masterFiles,
                   unsigned int *bufRGBW, float *bufZ)
  : outBufRGBW(bufRGBW),
    outBufZ(bufZ),
    width(imageWidth),
    height(imageHeight),
    ba2File(archiveFiles),
    esmFile(masterFiles),
    viewTransform(0.0625f, 3.14159265f, 0.0f, 0.0f,
                  float(imageWidth) * 0.5f, float(imageHeight) * 0.5f,
                  32768.0f),
    lightX(-0.6667f),
    lightY(0.6667f),
    lightZ(0.3333f),
    textureMip(2),
    landTextureMip(3.0f),
    landTxtRGBScale(1.0f),
    landTxtDefColor(0x003F3F3FU),
    landData((LandscapeData *) 0),
    cellTextureResolution(256),
    defaultLandLevel(-8192.0f),
    defaultWaterLevel(-8192.0f),
    modelLOD(0),
    distantObjectsOnly(false),
    noDisabledObjects(false),
    bufRGBWAllocFlag(!bufRGBW),
    bufZAllocFlag(!bufZ),
    threadCnt(0),
    textureDataSize(0),
    firstTexture((CachedTexture *) 0),
    lastTexture((CachedTexture *) 0),
    waterColor(0xC0302010U),
    verboseMode(true)
{
  size_t  imageDataSize = size_t(width) * size_t(height);
  if (bufRGBWAllocFlag)
    outBufRGBW = new unsigned int[imageDataSize];
  if (bufZAllocFlag)
    outBufZ = new float[imageDataSize];
  clear((unsigned int) bufRGBWAllocFlag | ((unsigned int) bufZAllocFlag << 1));
  Plot3D_TriShape::getDefaultLightingFunction(lightingPolynomial);
  renderThreads.reserve(16);
  nifFiles.resize(modelBatchCnt);
  setThreadCount(-1);
}

Renderer::~Renderer()
{
  clear(0x7C);
  if (bufZAllocFlag && outBufZ)
    delete[] outBufZ;
  if (bufRGBWAllocFlag && outBufRGBW)
    delete[] outBufRGBW;
}

void Renderer::clear()
{
  clear(0x7C);
}

void Renderer::clearImage()
{
  clear(0x03);
}

void Renderer::setViewTransform(
    float scale, float rotationX, float rotationY, float rotationZ,
    float offsetX, float offsetY, float offsetZ)
{
  NIFFile::NIFVertexTransform t(scale, rotationX, rotationY, rotationZ,
                                offsetX, offsetY, offsetZ);
  viewTransform = t;
}

void Renderer::setLightDirection(float rotationX, float rotationY)
{
  NIFFile::NIFVertexTransform t(1.0f, rotationX, rotationY, 0.0f,
                                0.0f, 0.0f, 0.0f);
  lightX = t.rotateXZ;
  lightY = t.rotateYZ;
  lightZ = t.rotateZZ;
}

void Renderer::setLightingFunction(const float *a)
{
  for (int i = 0; i < 6; i++)
    lightingPolynomial[i] = a[i];
}

void Renderer::setThreadCount(int n)
{
  if (n <= 0)
    n = int(std::thread::hardware_concurrency());
  int     maxThreads = int(renderThreads.capacity());
  n = (n > 1 ? (n < maxThreads ? n : maxThreads) : 1);
  if (n == threadCnt)
    return;
  threadCnt = n;
  renderThreads.resize(size_t(n));
  for (size_t i = 0; i < renderThreads.size(); i++)
  {
    if (!renderThreads[i].renderer)
    {
      renderThreads[i].renderer = new Plot3D_TriShape(outBufRGBW, outBufZ,
                                                      width, height);
    }
  }
}

void Renderer::addHDModelPattern(const std::string& s)
{
  if (s.empty())
    return;
  for (size_t i = 0; i < hdModelNamePatterns.size(); i++)
  {
    if (s.find(hdModelNamePatterns[i]) != std::string::npos)
      return;
    if (hdModelNamePatterns[i].find(s) != std::string::npos)
      hdModelNamePatterns.erase(hdModelNamePatterns.begin() + i);
  }
  hdModelNamePatterns.push_back(s);
}

void Renderer::setDefaultEnvMap(const std::string& s)
{
  defaultEnvMap = s;
}

void Renderer::setWaterTexture(const std::string& s)
{
  defaultWaterTexture = s;
}

void Renderer::loadTerrain(const char *btdFileName,
                           unsigned int worldID, unsigned int defTxtID,
                           int mipLevel, int xMin, int yMin, int xMax, int yMax)
{
  clear(0x44);
  if (verboseMode)
    std::fprintf(stderr, "Loading terrain data\n");
  landData = new LandscapeData(&esmFile, btdFileName, &ba2File, 0x1B, worldID,
                               defTxtID, mipLevel, xMin, yMin, xMax, yMax);
  defaultLandLevel = landData->getDefaultLandLevel();
  defaultWaterLevel = landData->getWaterLevel();
  if (verboseMode)
    std::fprintf(stderr, "Loading landscape textures\n");
  landTextures.resize(landData->getTextureCount(), (DDSTexture *) 0);
  for (size_t i = 0; i < landTextures.size(); i++)
  {
    if (!landData->getTextureDiffuse(i).empty())
      landTextures[i] = loadTexture(landData->getTextureDiffuse(i));
  }
}

void Renderer::renderTerrain(unsigned int worldID)
{
  if (verboseMode)
    std::fprintf(stderr, "Rendering terrain\n");
  if (!worldID)
    worldID = getDefaultWorldID();
  clear(0x38);
  findObjects(worldID, 0);
  sortObjectList();
  renderObjectList();
}

void Renderer::renderObjects(unsigned int formID)
{
  if (verboseMode)
    std::fprintf(stderr, "Rendering objects\n");
  if (!formID)
    formID = getDefaultWorldID();
  clear(0x38);
  findObjects(formID, 1);
  sortObjectList();
  renderObjectList();
}

void Renderer::renderWater(unsigned int formID)
{
  if (verboseMode)
    std::fprintf(stderr, "Rendering water\n");
  if (!formID)
    formID = getDefaultWorldID();
  clear(0x38);
  findObjects(formID, 2);
  sortObjectList();
  renderObjectList();
  size_t  imageDataSize = size_t(width) * size_t(height);
  for (size_t i = 0; i < imageDataSize; i++)
  {
    unsigned int  c = outBufRGBW[i];
    unsigned int  a = c >> 24;
    if (a >= 1 && a <= 254)
    {
      unsigned int  tmp =
          Plot3D_TriShape::multiplyWithLight(waterColor, 0U, int((a - 1) << 2));
      outBufRGBW[i] = blendRGBA32(c, tmp, int(waterColor >> 24)) | 0xFF000000U;
    }
  }
}


static const char *usageStrings[] =
{
  "Usage: render INFILE.ESM[,...] OUTFILE.DDS W H ARCHIVEPATH [OPTIONS...]",
  "",
  "Options:",
  "    --help              print usage",
  "    --                  remaining options are file names",
  "    -threads INT        set the number of threads to use",
  "    -ssaa BOOL          render at double resolution and downsample",
  "    -q                  do not print messages other than errors",
  "",
  "    -btd FILENAME.BTD   read terrain data from Fallout 76 .btd file",
  "    -w FORMID           form ID of world, cell, or object to render",
  "    -r X0 Y0 X1 Y1      terrain cell range, X0,Y0 = SW to X1,Y1 = NE",
  "    -l INT              level of detail to use from BTD file (0 to 4)",
  "    -deftxt FORMID      form ID of default land texture",
  "    -defclr 0x00RRGGBB  default color for untextured terrain",
  "    -ltxtres INT        land texture resolution per cell",
  "    -mip INT            base mip level for all textures",
  "    -lmip FLOAT         additional mip level for land textures",
  "    -lmult FLOAT        land texture RGB level scale",
  "",
  "    -view SCALE RX RY RZ OFFS_X OFFS_Y OFFS_Z",
  "                        set transform from world coordinates to image",
  "                        coordinates, rotations are in degrees",
  "    -light SCALE RX RY  set light level and X, Y rotation (0, 0 = top)",
  "    -lpoly A5 A4 A3 A2 A1 A0    set lighting polynomial, -1 <= x <= 1",
  "",
  "    -mlod INT           set level of detail for models, 0 (best) to 4",
  "    -vis BOOL           render only objects visible from distance",
  "    -ndis BOOL          do not render initially disabled objects",
  "    -hqm STRING         add high quality model path name pattern",
  "",
  "    -env FILENAME.DDS   default environment map texture path in archives",
  "    -wtxt FILENAME.DDS  water normal map texture path in archives",
  "    -watercolor UINT32  water color (A7R8G8B8), 0 disables water",
  (char *) 0
};

int main(int argc, char **argv)
{
  int     err = 1;
  try
  {
    std::vector< const char * > args;
    int     threadCnt = -1;
    bool    verboseMode = true;
    bool    distantObjectsOnly = false;
    bool    noDisabledObjects = false;
    bool    enableDownscale = false;
    unsigned int  formID = 0U;
    int     btdLOD = 2;
    const char  *btdPath = (char *) 0;
    int     terrainX0 = -32768;
    int     terrainY0 = -32768;
    int     terrainX1 = 32767;
    int     terrainY1 = 32767;
    unsigned int  defTxtID = 0U;
    unsigned int  ltxtDefColor = 0x003F3F3FU;
    int     ltxtResolution = 128;
    int     textureMip = 2;
    float   landTextureMip = 3.0f;
    float   landTextureMult = 1.0f;
    int     modelLOD = 0;
    unsigned int  waterColor = 0x60102030U;
    float   viewScale = 0.0625f;
    float   viewRotationX = 180.0f;
    float   viewRotationY = 0.0f;
    float   viewRotationZ = 0.0f;
    float   viewOffsX = 0.0f;
    float   viewOffsY = 0.0f;
    float   viewOffsZ = 0.0f;
    float   lightLevel = 1.0f;
    float   lightRotationX = 63.435f;
    float   lightRotationY = 41.8103f;
    float   lightingPolynomial[6];
    const char  *defaultEnvMap = (char *) 0;
    const char  *waterTexture = (char *) 0;
    std::vector< const char * > hdModelNamePatterns;
    Plot3D_TriShape::getDefaultLightingFunction(lightingPolynomial);

    for (int i = 1; i < argc; i++)
    {
      if (std::strcmp(argv[i], "--") == 0)
      {
        while (++i < argc)
          args.push_back(argv[i]);
        break;
      }
      else if (std::strcmp(argv[i], "--help") == 0)
      {
        args.clear();
        err = 0;
        break;
      }
      else if (argv[i][0] != '-')
      {
        args.push_back(argv[i]);
      }
      else if (std::strcmp(argv[i], "-threads") == 0)
      {
        if (++i >= argc)
          throw errorMessage("missing argument for %s", argv[i - 1]);
        threadCnt = int(parseInteger(argv[i], 10, "invalid number of threads",
                                     1, 16));
      }
      else if (std::strcmp(argv[i], "-ssaa") == 0)
      {
        if (++i >= argc)
          throw errorMessage("missing argument for %s", argv[i - 1]);
        enableDownscale =
            bool(parseInteger(argv[i], 0, "invalid argument for -ssaa", 0, 1));
      }
      else if (std::strcmp(argv[i], "-q") == 0)
      {
        verboseMode = false;
      }
      else if (std::strcmp(argv[i], "-btd") == 0)
      {
        if (++i >= argc)
          throw errorMessage("missing argument for %s", argv[i - 1]);
        btdPath = argv[i];
      }
      else if (std::strcmp(argv[i], "-w") == 0)
      {
        if (++i >= argc)
          throw errorMessage("missing argument for %s", argv[i - 1]);
        formID = (unsigned int) parseInteger(argv[i], 0, "invalid form ID",
                                             0, 0x0FFFFFFF);
      }
      else if (std::strcmp(argv[i], "-r") == 0)
      {
        if ((i + 4) >= argc)
          throw errorMessage("missing argument for %s", argv[i]);
        terrainX0 = int(parseInteger(argv[i + 1], 10, "invalid terrain X0",
                                     -32768, 32767));
        terrainY0 = int(parseInteger(argv[i + 2], 10, "invalid terrain Y0",
                                     -32768, 32767));
        terrainX1 = int(parseInteger(argv[i + 3], 10, "invalid terrain X1",
                                     terrainX0, 32767));
        terrainY1 = int(parseInteger(argv[i + 4], 10, "invalid terrain Y1",
                                     terrainY0, 32767));
        i = i + 4;
      }
      else if (std::strcmp(argv[i], "-l") == 0)
      {
        if (++i >= argc)
          throw errorMessage("missing argument for %s", argv[i - 1]);
        btdLOD = int(parseInteger(argv[i], 10,
                                  "invalid terrain level of detail", 0, 4));
      }
      else if (std::strcmp(argv[i], "-deftxt") == 0)
      {
        if (++i >= argc)
          throw errorMessage("missing argument for %s", argv[i - 1]);
        defTxtID = (unsigned int) parseInteger(argv[i], 0, "invalid form ID",
                                               0, 0x0FFFFFFF);
      }
      else if (std::strcmp(argv[i], "-defclr") == 0)
      {
        if (++i >= argc)
          throw errorMessage("missing argument for %s", argv[i - 1]);
        ltxtDefColor = (unsigned int) parseInteger(argv[i], 0,
                                                   "invalid land texture color",
                                                   0, 0x00FFFFFF);
      }
      else if (std::strcmp(argv[i], "-ltxtres") == 0)
      {
        if (++i >= argc)
          throw errorMessage("missing argument for %s", argv[i - 1]);
        ltxtResolution = int(parseInteger(argv[i], 0,
                                          "invalid land texture resolution",
                                          8, 4096));
        if (ltxtResolution & (ltxtResolution - 1))
          throw errorMessage("invalid land texture resolution");
      }
      else if (std::strcmp(argv[i], "-mip") == 0)
      {
        if (++i >= argc)
          throw errorMessage("missing argument for %s", argv[i - 1]);
        textureMip = int(parseInteger(argv[i], 10, "invalid texture mip level",
                                      0, 15));
      }
      else if (std::strcmp(argv[i], "-lmip") == 0)
      {
        if (++i >= argc)
          throw errorMessage("missing argument for %s", argv[i - 1]);
        landTextureMip = float(parseFloat(argv[i],
                                          "invalid land texture mip level",
                                          0.0, 15.0));
      }
      else if (std::strcmp(argv[i], "-lmult") == 0)
      {
        if (++i >= argc)
          throw errorMessage("missing argument for %s", argv[i - 1]);
        landTextureMult = float(parseFloat(argv[i],
                                           "invalid land texture RGB scale",
                                           0.5, 8.0));
      }
      else if (std::strcmp(argv[i], "-view") == 0)
      {
        if ((i + 7) >= argc)
          throw errorMessage("missing argument for %s", argv[i]);
        viewScale = float(parseFloat(argv[i + 1], "invalid view scale",
                                     1.0 / 512.0, 16.0));
        viewRotationX = float(parseFloat(argv[i + 2], "invalid view X rotation",
                                         -360.0, 360.0));
        viewRotationY = float(parseFloat(argv[i + 3], "invalid view Y rotation",
                                         -360.0, 360.0));
        viewRotationZ = float(parseFloat(argv[i + 4], "invalid view Z rotation",
                                         -360.0, 360.0));
        viewOffsX = float(parseFloat(argv[i + 5], "invalid view X offset",
                                     -1048576.0, 1048576.0));
        viewOffsY = float(parseFloat(argv[i + 6], "invalid view Y offset",
                                     -1048576.0, 1048576.0));
        viewOffsZ = float(parseFloat(argv[i + 7], "invalid view Z offset",
                                     -1048576.0, 1048576.0));
        i = i + 7;
      }
      else if (std::strcmp(argv[i], "-light") == 0)
      {
        if ((i + 3) >= argc)
          throw errorMessage("missing argument for %s", argv[i]);
        lightLevel = float(parseFloat(argv[i + 1], "invalid light level",
                                      0.125, 4.0));
        lightRotationX = float(parseFloat(argv[i + 2],
                                          "invalid light X rotation",
                                          -360.0, 360.0));
        lightRotationY = float(parseFloat(argv[i + 3],
                                          "invalid light Y rotation",
                                          -360.0, 360.0));
        i = i + 3;
      }
      else if (std::strcmp(argv[i], "-lpoly") == 0)
      {
        for (int j = 5; j >= 0; j--)
        {
          if (++i >= argc)
            throw errorMessage("missing argument for -lpoly");
          lightingPolynomial[j] =
              float(parseFloat(argv[i], "invalid lighting polynomial",
                               -16.0, 16.0));
        }
      }
      else if (std::strcmp(argv[i], "-mlod") == 0)
      {
        if (++i >= argc)
          throw errorMessage("missing argument for %s", argv[i - 1]);
        modelLOD = int(parseInteger(argv[i], 10, "invalid model LOD", 0, 4));
      }
      else if (std::strcmp(argv[i], "-vis") == 0)
      {
        if (++i >= argc)
          throw errorMessage("missing argument for %s", argv[i - 1]);
        distantObjectsOnly =
            bool(parseInteger(argv[i], 0, "invalid argument for -vis", 0, 1));
      }
      else if (std::strcmp(argv[i], "-ndis") == 0)
      {
        if (++i >= argc)
          throw errorMessage("missing argument for %s", argv[i - 1]);
        noDisabledObjects =
            bool(parseInteger(argv[i], 0, "invalid argument for -ndis", 0, 1));
      }
      else if (std::strcmp(argv[i], "-hqm") == 0)
      {
        if (++i >= argc)
          throw errorMessage("missing argument for %s", argv[i - 1]);
        hdModelNamePatterns.push_back(argv[i]);
      }
      else if (std::strcmp(argv[i], "-env") == 0)
      {
        if (++i >= argc)
          throw errorMessage("missing argument for %s", argv[i - 1]);
        defaultEnvMap = argv[i];
      }
      else if (std::strcmp(argv[i], "-wtxt") == 0)
      {
        if (++i >= argc)
          throw errorMessage("missing argument for %s", argv[i - 1]);
        waterTexture = argv[i];
      }
      else if (std::strcmp(argv[i], "-watercolor") == 0)
      {
        if (++i >= argc)
          throw errorMessage("missing argument for %s", argv[i - 1]);
        waterColor = (unsigned int) parseInteger(argv[i], 0,
                                                 "invalid water color",
                                                 0, 0x7FFFFFFF);
      }
      else
      {
        throw errorMessage("invalid option: %s", argv[i]);
      }
    }
    if (args.size() != 5)
    {
      for (size_t i = 0; usageStrings[i]; i++)
        std::fprintf(stderr, "%s\n", usageStrings[i]);
      return err;
    }
    if (btdPath && *btdPath == '\0')
      btdPath = (char *) 0;
    if (!formID)
      formID = (!btdPath ? 0x0000003CU : 0x0025DA15U);
    for (int i = 0; i < 6; i++)
      lightingPolynomial[i] = lightingPolynomial[i] * lightLevel;
    for (int i = -128; i <= 128; i++)
    {
      float   x = float(i) * (1.0f / 128.0f);
      float   y = 0.0f;
      for (int j = 5; j >= 0; j--)
        y = y * x + lightingPolynomial[j];
      if (!(y >= 0.0f && y <= (253.0f / 64.0f)))
        throw errorMessage("lighting polynomial is out of range at x = %f", x);
    }
    waterColor = (waterColor & 0x00FFFFFFU) | ((waterColor & 0x7F000000U) << 1);
    if (!(waterColor & 0xFF000000U))
      waterColor = 0U;
    int     width =
        int(parseInteger(args[2], 0, "invalid image width", 1, 32768));
    int     height =
        int(parseInteger(args[3], 0, "invalid image height", 1, 32768));
    if (enableDownscale)
    {
      width = width << 1;
      height = height << 1;
      viewScale = viewScale * 2.0f;
      viewOffsX = viewOffsX * 2.0f;
      viewOffsY = viewOffsY * 2.0f;
      viewOffsZ = viewOffsZ * 2.0f;
    }
    unsigned int  worldID = formID;

    BA2File ba2File(args[4]);
    ESMFile esmFile(args[0]);
    Renderer  renderer(width, height, ba2File, esmFile);
    if (threadCnt > 0)
      renderer.setThreadCount(threadCnt);
    renderer.setVerboseMode(verboseMode);
    renderer.setDistantObjectsOnly(distantObjectsOnly);
    renderer.setNoDisabledObjects(noDisabledObjects);
    renderer.setLandDefaultColor(ltxtDefColor);
    renderer.setLandTxtResolution(ltxtResolution);
    renderer.setTextureMipLevel(textureMip);
    renderer.setLandTextureMip(landTextureMip);
    renderer.setLandTxtRGBScale(landTextureMult);
    renderer.setModelLOD(modelLOD);
    renderer.setWaterColor(waterColor);
    {
      float   tmp = float(std::atan(1.0) / 45.0);       // degrees to radians
      renderer.setViewTransform(
          viewScale,
          viewRotationX * tmp, viewRotationY * tmp, viewRotationZ * tmp,
          viewOffsX + (float(width) * 0.5f), viewOffsY + (float(height) * 0.5f),
          viewOffsZ);
      renderer.setLightDirection(lightRotationX * tmp, lightRotationY * tmp);
    }
    renderer.setLightingFunction(lightingPolynomial);
    if (defaultEnvMap && *defaultEnvMap)
      renderer.setDefaultEnvMap(std::string(defaultEnvMap));
    if (waterColor && waterTexture && *waterTexture)
      renderer.setWaterTexture(std::string(waterTexture));
    for (size_t i = 0; i < hdModelNamePatterns.size(); i++)
    {
      if (hdModelNamePatterns[i] && hdModelNamePatterns[i][0])
        renderer.addHDModelPattern(std::string(hdModelNamePatterns[i]));
    }

    if (worldID)
    {
      renderer.loadTerrain(btdPath, worldID, defTxtID,
                           btdLOD, terrainX0, terrainY0, terrainX1, terrainY1);
      renderer.renderTerrain(worldID);
      renderer.clear();
    }
    renderer.renderObjects(formID);
    if (waterColor)
      renderer.renderWater(formID);
    if (verboseMode)
    {
      const NIFFile::NIFBounds& b = renderer.getBounds();
      if (b.xMax > b.xMin)
      {
        float   scale = (!enableDownscale ? 1.0f : 0.5f);
        std::fprintf(stderr,
                     "Bounds: %6.0f, %6.0f, %6.0f to %6.0f, %6.0f, %6.0f\n",
                     b.xMin * scale, b.yMin * scale, b.zMin * scale,
                     b.xMax * scale, b.yMax * scale, b.zMax * scale);
      }
    }

    DDSOutputFile outFile(args[1],
                          width >> (!enableDownscale ? 0 : 1),
                          height >> (!enableDownscale ? 0 : 1),
                          DDSInputFile::pixelFormatRGB24);
    if (!enableDownscale)
    {
      size_t  imageDataSize = size_t(width) * size_t(height);
      for (size_t i = 0; i < imageDataSize; i++)
      {
        unsigned int  c = renderer.getImageData()[i];
        outFile.writeByte((unsigned char) ((c >> 16) & 0xFF));
        outFile.writeByte((unsigned char) ((c >> 8) & 0xFF));
        outFile.writeByte((unsigned char) (c & 0xFF));
      }
    }
    else
    {
      for (int y = 0; y < height; y = y + 2)
      {
        for (int x = 0; x < width; x = x + 2)
        {
          unsigned int  c = downsample2xFilter(renderer.getImageData(),
                                               width, height, x, y);
          outFile.writeByte((unsigned char) ((c >> 16) & 0xFF));
          outFile.writeByte((unsigned char) ((c >> 8) & 0xFF));
          outFile.writeByte((unsigned char) (c & 0xFF));
        }
      }
    }
    err = 0;
  }
  catch (std::exception& e)
  {
    std::fprintf(stderr, "render: %s\n", e.what());
    err = 1;
  }
  return err;
}

