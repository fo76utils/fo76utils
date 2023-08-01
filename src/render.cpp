
#include "common.hpp"
#include "render.hpp"
#include "fp32vec4.hpp"

#include <algorithm>

bool Renderer::RenderObject::operator<(const RenderObject& r) const
{
  if ((flags & 0x12) != (r.flags & 0x12))
    return ((flags & 0x12) < (r.flags & 0x12));
  if (flags & 0x12)
  {
    unsigned int  modelID1 = model.o->modelID;
    unsigned int  modelID2 = r.model.o->modelID;
    unsigned int  modelMask1 = ~(Renderer::modelBatchCnt - 1U);
    if ((modelID1 & modelMask1) != (modelID2 & modelMask1))
      return ((modelID1 & modelMask1) < (modelID2 & modelMask1));
    if (tileIndex != r.tileIndex)
      return (tileIndex < r.tileIndex);
    if (z != r.z)
      return (z < r.z);
    unsigned int  modelMask2 = Renderer::modelBatchCnt - 1U;
    return ((modelID1 & modelMask2) < (modelID2 & modelMask2));
  }
  if (tileIndex != r.tileIndex)
    return (tileIndex < r.tileIndex);
  if (z != r.z)
    return (z < r.z);
  if (model.t.y0 != r.model.t.y0)
    return (model.t.y0 < r.model.t.y0);
  return (model.t.x0 < r.model.t.x0);
}

Renderer::ModelData::ModelData()
  : nifFile((NIFFile *) 0),
    totalTriangleCnt(0),
    o((BaseObject *) 0),
    loadThread((std::thread *) 0)
{
}

Renderer::ModelData::~ModelData()
{
  if (loadThread)
  {
    loadThread->join();
    delete loadThread;
    loadThread = (std::thread *) 0;
  }
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
  o = (BaseObject *) 0;
  threadErrMsg.clear();
  fileBuf.clear();
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
  objectsRemaining.clear();
}

inline Renderer::ThreadSortObject::ThreadSortObject()
  : tileMask(0ULL),
    triangleCnt(0)
{
}

inline bool Renderer::ThreadSortObject::operator<(
    const ThreadSortObject& r) const
{
  return (triangleCnt > r.triangleCnt);         // reverse sort
}

inline Renderer::ThreadSortObject& Renderer::ThreadSortObject::operator+=(
    const ThreadSortObject& r)
{
  tileMask |= r.tileMask;
  triangleCnt += r.triangleCnt;
  return (*this);
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

signed short Renderer::calculateTileIndex(unsigned long long screenAreasUsed)
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
    return (signed short) n0;
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
    return (signed short) (((y0 & 6) << 3) + (x0 & 6) + 64
                           + ((y0 & 1) << 7) + ((x0 & 1) << 6));
  }
  if ((x1 - x0) <= 3 && (y1 - y0) <= 3)
  {
    return (signed short) (((y0 & 4) << 3) + (x0 & 4) + 320
                           + ((y0 & 3) << 8) + ((x0 & 3) << 6));
  }
  return 1344;
}

int Renderer::setScreenAreaUsed(RenderObject& p)
{
  p.tileIndex = -1;
  NIFFile::NIFVertexTransform vt;
  NIFFile::NIFBounds  modelBounds;
  if (p.flags & 0x12)
  {
    FloatVector4  b0(float(p.model.o->obndX0), float(p.model.o->obndY0),
                     float(p.model.o->obndZ0), 0.0f);
    FloatVector4  b1(float(p.model.o->obndX1), float(p.model.o->obndY1),
                     float(p.model.o->obndZ1), 0.0f);
    FloatVector4  tmp(1.0f, 1.0f, 1.0f, 0.0f);
#if ENABLE_TXST_DECALS
    if (BRANCH_UNLIKELY(p.flags & 0x10))
    {
      tmp = FloatVector4::convertFloat16(std::uint64_t(p.mswpFormID));
      tmp[2] = tmp[1];
      tmp[1] = 1.0f;
      b0[1] = b0[1] + getDecalYOffsetMin(b0);
      b1[1] = b1[1] + getDecalYOffsetMax(b1);
    }
#endif
    modelBounds.boundsMin = b0;
    modelBounds.boundsMin.minValues(b1);
    modelBounds.boundsMin -= 2.0f;
    modelBounds.boundsMax = b0;
    modelBounds.boundsMax.maxValues(b1);
    modelBounds.boundsMax += 2.0f;
    modelBounds.boundsMin *= tmp;
    modelBounds.boundsMax *= tmp;
    vt = p.modelTransform;
    vt *= viewTransform;
  }
  else if (p.flags & 1)
  {
    if (!landData)
      return -1;
    float   xyScale = 4096.0f / float(landData->getCellResolution());
    float   xOffset = -xyScale * float(landData->getOriginX());
    float   yOffset = xyScale * float(landData->getOriginY());
    float   zScale = (landData->getZMax() - landData->getZMin()) / 65535.0f;
    float   zOffset = landData->getZMin();
    int     x0 = (p.model.t.x0 < p.model.t.x1 ? p.model.t.x0 : p.model.t.x1);
    int     y0 = (p.model.t.y0 < p.model.t.y1 ? p.model.t.y0 : p.model.t.y1);
    int     x1 = x0 + std::abs(int(p.model.t.x1) - int(p.model.t.x0));
    int     y1 = y0 + std::abs(int(p.model.t.y1) - int(p.model.t.y0));
    modelBounds.boundsMin =
        FloatVector4(float(x0) * xyScale + xOffset,
                     float(y1) * -xyScale + yOffset, 0.0f, 0.0f);
    modelBounds.boundsMax =
        FloatVector4(float(x1) * xyScale + xOffset,
                     float(y0) * -xyScale + yOffset, 0.0f, 0.0f);
    int     w = landData->getImageWidth();
    int     h = landData->getImageHeight();
    x0 = (x0 > 0 ? (x0 < (w - 1) ? x0 : (w - 1)) : 0);
    x1 = (x1 > 0 ? (x1 < (w - 1) ? x1 : (w - 1)) : 0);
    std::uint16_t z0 = 0xFFFF;
    std::uint16_t z1 = 0;
    for (int y = y0; y <= y1; y++)
    {
      int     yc = (y > 0 ? (y < (h - 1) ? y : (h - 1)) : 0);
      const std::uint16_t *srcPtr = landData->getHeightMap();
      srcPtr = srcPtr + (size_t(yc) * size_t(w));
      for (int x = x0; x <= x1; x++)
      {
        std::uint16_t z = srcPtr[x];
        z0 = (z < z0 ? z : z0);
        z1 = (z > z1 ? z : z1);
      }
    }
    modelBounds.boundsMin[2] = float(int(z0)) * zScale + zOffset;
    modelBounds.boundsMax[2] = float(int(z1)) * zScale + zOffset;
    vt = viewTransform;
  }
  else if (p.flags & 4)
  {
    int     x0 = int(p.model.t.x0 < p.model.t.x1 ? p.model.t.x0 : p.model.t.x1);
    int     x1 = x0 + std::abs(int(p.model.t.x1) - int(p.model.t.x0));
    int     y0 = int(p.model.t.y0 < p.model.t.y1 ? p.model.t.y0 : p.model.t.y1);
    int     y1 = y0 + std::abs(int(p.model.t.y1) - int(p.model.t.y0));
    modelBounds.boundsMin = FloatVector4(float(x0), float(y0), 0.0f, 0.0f);
    modelBounds.boundsMax = FloatVector4(float(x1), float(y1), 0.0f, 0.0f);
    vt = p.modelTransform;
    vt *= viewTransform;
  }
  else
  {
    return -1;
  }
  NIFFile::NIFBounds  screenBounds;
  for (int i = 0; i < 8; i++)
  {
    FloatVector4  v((!(i & 1) ? modelBounds.xMin() : modelBounds.xMax()),
                    (!(i & 2) ? modelBounds.yMin() : modelBounds.yMax()),
                    (!(i & 4) ? modelBounds.zMin() : modelBounds.zMax()), 0.0f);
    v = vt.transformXYZ(v);
    screenBounds += v;
  }
  worldBounds += screenBounds.boundsMin;
  worldBounds += screenBounds.boundsMax;
  screenBounds.boundsMin -= 2.0f;
  screenBounds.boundsMax += 2.0f;
  int     xMin = roundFloat(screenBounds.xMin());
  int     yMin = roundFloat(screenBounds.yMin());
  int     zMin = roundFloat(screenBounds.zMin());
  int     xMax = roundFloat(screenBounds.xMax());
  int     yMax = roundFloat(screenBounds.yMax());
  int     zMax = roundFloat(screenBounds.zMax());
  if (xMin >= width || xMax < 0 || yMin >= height || yMax < 0 ||
      zMin >= zRangeMax || zMax < 0)
  {
    return -1;
  }
  p.tileIndex = calculateTileIndex(calculateTileMask(xMin, yMin, xMax, yMax));
  p.z = roundFloat(screenBounds.zMin() * 64.0f);
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
  tmp.tileIndex = -1;
  tmp.z = 0;
  tmp.mswpFormID = 0;
  int     w = landData->getImageWidth();
  int     h = landData->getImageHeight();
  int     n = landData->getCellResolution();
  int     x0 = landData->getOriginX() + (cellX * n);
  int     y0 = landData->getOriginY() - ((cellY + 1) * n);
  int     x1 = x0 + n;
  int     y1 = y0 + n;
  x0 = (x0 > 0 ? x0 : 0);
  y0 = (y0 > 0 ? y0 : 0);
  x1 = (x1 < w ? x1 : w);
  y1 = (y1 < h ? y1 : h);
  if (debugMode == 1)
  {
    n = n >> 1;
  }
  else if (n > 24)
  {
    if (renderThreads.size() > 1 && n >= 64)
    {
      int     m = roundFloat(float(width) * float(height) * (1.0f / 16777216.0f)
                             / (viewTransform.scale * viewTransform.scale));
      if (n >= 128 && m < int(renderThreads.size() << 2))
        n = n >> 1;
      if (m < int(renderThreads.size()))
        n = n >> 1;
    }
    n = n - 8;
  }
  for (int y = ((y0 + (n - 1)) / n) * n; y < y1; y = y + n)
  {
    int     y2 = y + n;
    for (int x = ((x0 + (n - 1)) / n) * n; x < x1; x = x + n)
    {
      int     x2 = x + n;
      tmp.model.t.x0 = (signed short) x;
      tmp.model.t.y0 = (signed short) y;
      tmp.model.t.x1 = (signed short) (x2 < w ? x2 : w);
      tmp.model.t.y1 = (signed short) (y2 < h ? y2 : h);
      if (setScreenAreaUsed(tmp) >= 0)
      {
        if (debugMode == 1)
          tmp.z = int(r.formID);
        objectList.push_back(tmp);
      }
    }
  }
}

void Renderer::addWaterCell(const ESMFile::ESMRecord& r)
{
  if (!waterRenderMode)
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
  {
    if (cellFlags & 0x01)
      return;
    waterLevel = defaultWaterLevel;
  }
  RenderObject  tmp;
  tmp.flags = 0x000C;                   // water cell, alpha blending
  tmp.tileIndex = -1;
  tmp.z = 0;
  tmp.model.t.x0 = 0;
  tmp.model.t.y0 = 0;
  tmp.model.t.x1 = 4096;
  tmp.model.t.y1 = 4096;
  tmp.mswpFormID = 0;
  tmp.modelTransform.offsX = float(cellX) * 4096.0f;
  tmp.modelTransform.offsY = float(cellY) * 4096.0f;
  tmp.modelTransform.offsZ = waterLevel;
  if (setScreenAreaUsed(tmp) >= 0)
  {
    if (debugMode == 1)
      tmp.z = int(r.formID);
    if (waterRenderMode < 0)
      tmp.mswpFormID = getWaterMaterial(materials, esmFile, &r, 0U);
    objectList.push_back(tmp);
  }
}

void Renderer::readDecalProperties(BaseObject& p, const ESMFile::ESMRecord& r)
{
#if ENABLE_TXST_DECALS
  if (!(r == "TXST"))
    return;
  ESMFile::ESMField f(esmFile, r);
  float   specularSmoothness = 0.0f;
  unsigned int  decalFlags = 0U;
  bool    haveDODT = false;
  while (f.next())
  {
    if (f == "DODT" && f.size() >= 24)
    {
      FloatVector4  decalBounds(f.readFloatVector4());
      float   decalWidth = (decalBounds[0] + decalBounds[1]) * 0.5f;
      float   decalHeight = (decalBounds[2] + decalBounds[3]) * 0.5f;
      float   decalDepth = f.readFloat();
      decalWidth = std::min(std::max(decalWidth, 0.0f), 4096.0f);
      decalHeight = std::min(std::max(decalHeight, 0.0f), 4096.0f);
      decalDepth = std::min(std::max(decalDepth, 0.0f), 4096.0f);
      p.obndX0 = (signed short) roundFloat(decalWidth * -0.5f);
      p.obndY0 = (signed short) roundFloat(decalDepth * -0.5f);
      p.obndZ0 = (signed short) roundFloat(decalHeight * -0.5f);
      p.obndX1 = p.obndX0 + (signed short) roundFloat(decalWidth);
      p.obndY1 = p.obndY0 + (signed short) roundFloat(decalDepth);
      p.obndZ1 = p.obndZ0 + (signed short) roundFloat(decalHeight);
      if (!(p.obndX1 > p.obndX0 && p.obndY1 > p.obndY0 && p.obndZ1 > p.obndZ0))
        return;
      float   s = f.readFloat();        // shininess
      if (esmFile.getESMVersion() < 0x80)
        s = (s > 2.0f ? ((float(std::log2(s)) - 1.0f) * (1.0f / 9.0f)) : 0.0f);
      specularSmoothness = std::min(std::max(s, 0.0f), 8.0f);
      if (f.size() == 36)
        decalFlags = FileBuffer::readUInt32Fast(f.getDataPtr() + 28);
      haveDODT = true;
      break;
    }
  }
  if (!haveDODT)
    return;
  unsigned int  formID = r.formID;
  std::map< unsigned int, BGSMFile >::iterator  i = materials.find(formID);
  if (i == materials.end())
  {
    BGSMFile  m;
    m.nifVersion = 155U;                // Fallout 76
    if (esmFile.getESMVersion() < 0x80)
      m.nifVersion = 100U;              // Skyrim or older
    else if (esmFile.getESMVersion() < 0xC0)
      m.nifVersion = 130U;              // Fallout 4
    m.s.specularSmoothness = specularSmoothness;
    ESMFile::ESMField f2(esmFile, r);
    while (f2.next())
    {
      if (f2 == "MNAM" && f2.size() > 0)
      {
        f2.readPath(stringBuf, std::string::npos, "materials/", ".bgsm");
        if (!stringBuf.empty())
          m.texturePaths.setMaterialPath(stringBuf);
      }
      else if ((f2.type & 0xF0FFFFFFU) == 0x30305854U)  // "TX00"
      {
        size_t  n = (f2.type >> 24) & 0x0F;
        if (n < 10 && f2.size() > 0)
        {
          if (n == 3 || n == 5 || n == 7)   // glow, environment, FO4 specular
            n--;
          else if (n == 2 && esmFile.getESMVersion() < 0x80)
            n = 5;                          // Skyrim environment mask
          f2.readPath(stringBuf, std::string::npos, "textures/", ".dds");
          if (!stringBuf.empty())
          {
            m.texturePaths.setTexturePath(n, stringBuf.c_str());
            m.texturePathMask = m.texturePathMask | std::uint32_t(1U << n);
          }
        }
      }
    }
    if (m.texturePaths && !m.texturePaths.materialPath().empty())
    {
      try
      {
        m.loadBGSMFile(ba2File, m.texturePaths.materialPath());
      }
      catch (FO76UtilsError&)
      {
        return;
      }
    }
    if (!(m.texturePathMask & 0x0001))
      return;
    if (decalFlags)
    {
      m.alphaThreshold = (unsigned char) std::min(decalFlags >> 16, 0xFFU);
      m.alphaFlags = 0x00EC;
      m.alpha = 1.0f;
      if (decalFlags & 0x0200U)         // alpha blending
        m.alphaFlags = std::uint16_t(!(decalFlags & 0x1000U) ? 0x00ED : 0x0029);
      if (decalFlags & 0x0400U)         // alpha testing
        m.alphaFlags = m.alphaFlags | 0x1200;
      m.updateAlphaProperties();
    }
    else if (!(m.flags & BGSMFile::Flag_TSAlphaBlending) &&
             (~m.alphaFlags & 0x1200))
    {
      // default alpha testing and blending
      m.flags = m.flags | BGSMFile::Flag_TSAlphaBlending;
      m.alphaThreshold = 1;
      m.alphaFlags = 0x12ED;
      m.alpha = 1.0f;
      m.alphaThresholdFloat = 1.0f;
    }
    materials.insert(std::pair< unsigned int, BGSMFile >(formID, m));
  }
  p.type = 0x5854;                      // "TX"
  p.flags = 0x0010;                     // is decal
  p.modelID = 0U;
  p.mswpFormID = formID;
  char    tmpBuf[32];
  std::sprintf(tmpBuf, "~0x%08X", formID);
  p.modelPath = tmpBuf;
#else
  (void) p;
  (void) r;
#endif
}

const Renderer::BaseObject * Renderer::readModelProperties(
    RenderObject& p, const ESMFile::ESMRecord& r)
{
  std::map< unsigned int, BaseObject >::const_iterator  i =
      baseObjects.find(r.formID);
  if (i == baseObjects.end())
  {
    BaseObject  tmp;
    tmp.type = 0;
    tmp.flags = (unsigned short) ((r.flags >> 18) & 0x20U);     // is marker
    tmp.modelID = 0xFFFFFFFFU;
    tmp.mswpFormID = 0U;
    tmp.obndX0 = 0;
    tmp.obndY0 = 0;
    tmp.obndZ0 = 0;
    tmp.obndX1 = 0;
    tmp.obndY1 = 0;
    tmp.obndZ1 = 0;
    do
    {
      if (tmp.flags && !enableMarkers)
        continue;
      switch (r.type)
      {
        case 0x49544341:                // "ACTI"
        case 0x5454534D:                // "MSTT"
        case 0x54415750:                // "PWAT"
          break;
        case 0x4E525546:                // "FURN"
        case 0x4C4F4353:                // "SCOL"
        case 0x54415453:                // "STAT"
        case 0x45455254:                // "TREE"
          // optionally ignore objects not visible from distance
          if ((r.flags & 0x8000U) < std::uint32_t(distantObjectsOnly))
            continue;
          break;
#if ENABLE_TXST_DECALS
        case 0x54535854:                // "TXST"
          if (enableDecals)
            readDecalProperties(tmp, r);
          continue;
#endif
        default:
          if (!enableAllObjects ||
              (r.flags & 0x8000U) < std::uint32_t(distantObjectsOnly))
          {
            continue;
          }
          break;
      }
      bool    haveOBND = false;
      bool    isWater = (r == "PWAT");
      bool    isHDModel = false;
      stringBuf.clear();
      ESMFile::ESMField f(esmFile, r);
      while (f.next())
      {
        if (f == "OBND" && f.size() >= 6)
        {
          tmp.obndX0 = (signed short) uint16ToSigned(f.readUInt16Fast());
          tmp.obndY0 = (signed short) uint16ToSigned(f.readUInt16Fast());
          tmp.obndZ0 = (signed short) uint16ToSigned(f.readUInt16Fast());
          tmp.obndX1 = (signed short) uint16ToSigned(f.readUInt16Fast());
          tmp.obndY1 = (signed short) uint16ToSigned(f.readUInt16Fast());
          tmp.obndZ1 = (signed short) uint16ToSigned(f.readUInt16Fast());
          haveOBND = true;
        }
        else if (f == "MODL" && f.size() > 4 &&
                 (!modelLOD || stringBuf.empty()))
        {
          f.readPath(stringBuf, std::string::npos, "meshes/", ".nif");
          isHDModel = isHighQualityModel(stringBuf);
          if (r == "SCOL" && esmFile.getESMVersion() >= 0xC0)
          {
            if (stringBuf.find(".esm/", 0, 5) == std::string::npos)
            {
              // fix invalid SCOL model paths in SeventySix.esm
              stringBuf = "meshes/scol/seventysix.esm/cm00000000.nif";
              unsigned int  n = r.formID;
              for (int j = 36; n; j--, n = n >> 4)
                stringBuf[j] = char(n & 15U) + ((n & 15U) < 10U ? '0' : 'W');
            }
          }
        }
        else if (f == "MNAM" && f.size() == 1040 && modelLOD && !isHDModel)
        {
          for (int j = (modelLOD - 1) & 3; j >= 0; j--)
          {
            f.setPosition(size_t(j * 260));
            if (f[f.getPosition()] != 0)
            {
              f.readPath(stringBuf, std::string::npos, "meshes/", ".nif");
              if (!stringBuf.empty())
                break;
            }
          }
        }
        else if (f == "MODS" && f.size() >= 4)
        {
          tmp.mswpFormID = f.readUInt32Fast();
        }
        else if (f == "MODC" && f.size() >= 4)
        {
          tmp.flags |= (unsigned short) (((roundFloat(f.readFloat() * 255.0f)
                                           & 0xFF) << 8) | 0x80);
        }
        else if (f == "WNAM" && r == "ACTI" && f.size() >= 4)
        {
          isWater = true;
        }
      }
      if (!haveOBND || stringBuf.empty())
        continue;
      if (!isWater)
      {
        if (std::strncmp(stringBuf.c_str(), "meshes/water/", 13) == 0 ||
            stringBuf.find("/fxwaterfall") != std::string::npos)
        {
          isWater = true;
        }
      }
      if ((!isWater && isExcludedModel(stringBuf)) ||
          (isWater && !waterRenderMode))
      {
        continue;
      }
      tmp.type = (unsigned short) (r.type & 0xFFFFU);
      tmp.flags = tmp.flags | (unsigned short) ((!isWater ? 0x02 : 0x06)
                                                | (!isHDModel ? 0x00 : 0x40));
      tmp.modelPath = stringBuf;
    }
    while (false);
    if (tmp.mswpFormID && !(tmp.flags & 0x0010))
    {
      tmp.mswpFormID =
          materialSwaps.loadMaterialSwap(ba2File, esmFile, tmp.mswpFormID);
    }
    i = baseObjects.insert(std::pair< unsigned int, BaseObject >(
                               r.formID, tmp)).first;
  }
  if (!(i->second.flags & 0x17))
    return (BaseObject *) 0;
  p.flags = i->second.flags;
  p.model.o = &(i->second);
  p.mswpFormID = i->second.mswpFormID;
  return &(i->second);
}

void Renderer::addSCOLObjects(const ESMFile::ESMRecord& r,
                              float scale, float rX, float rY, float rZ,
                              float offsX, float offsY, float offsZ,
                              unsigned int refrMSWPFormID)
{
  NIFFile::NIFVertexTransform vt(scale, rX, rY, rZ, offsX, offsY, offsZ);
  RenderObject  tmp;
  tmp.tileIndex = -1;
  tmp.z = 0;
  ESMFile::ESMField f(esmFile, r);
  unsigned int  mswpFormID_SCOL = 0U;
  unsigned int  mswpFormID_ONAM = 0U;
  const BaseObject  *o = (BaseObject *) 0;
  while (f.next())
  {
    if (f == "MODS" && f.size() >= 4)
    {
      mswpFormID_SCOL = f.readUInt32Fast();
    }
    else if (f == "ONAM" && f.size() >= 4)
    {
      o = (BaseObject *) 0;
      unsigned int  modelFormID = f.readUInt32Fast();
      if (!modelFormID)
        continue;
      mswpFormID_ONAM = 0U;
      if (f.size() >= 8)
        mswpFormID_ONAM = f.readUInt32Fast();
      const ESMFile::ESMRecord  *r3 = esmFile.getRecordPtr(modelFormID);
      if (!r3)
        continue;
      if (!(o = readModelProperties(tmp, *r3)) || (tmp.flags & 0x14))
      {
        o = (BaseObject *) 0;
        continue;
      }
      tmp.mswpFormID = 0U;
    }
    else if (f == "DATA" && o)
    {
      while ((f.getPosition() + 28) <= f.size())
      {
        offsX = f.readFloat();
        offsY = f.readFloat();
        offsZ = f.readFloat();
        rX = f.readFloat();
        rY = f.readFloat();
        rZ = f.readFloat();
        scale = f.readFloat();
        tmp.modelTransform =
            NIFFile::NIFVertexTransform(scale, rX, rY, rZ, offsX, offsY, offsZ);
        tmp.modelTransform *= vt;
        if (setScreenAreaUsed(tmp) >= 0)
        {
          if (!tmp.mswpFormID)
          {
            if (refrMSWPFormID)
              tmp.mswpFormID = refrMSWPFormID;
            else if (mswpFormID_ONAM)
              tmp.mswpFormID = mswpFormID_ONAM;
            else if (mswpFormID_SCOL)
              tmp.mswpFormID = mswpFormID_SCOL;
            else
              tmp.mswpFormID = o->mswpFormID;
            if (tmp.mswpFormID && tmp.mswpFormID != o->mswpFormID)
            {
              tmp.mswpFormID = materialSwaps.loadMaterialSwap(
                                   ba2File, esmFile, tmp.mswpFormID);
            }
          }
          objectList.push_back(tmp);
        }
      }
    }
  }
}

void Renderer::findObjects(unsigned int formID, int type, bool isRecursive)
{
  const ESMFile::ESMRecord  *r = (ESMFile::ESMRecord *) 0;
  do
  {
    r = esmFile.getRecordPtr(formID);
    if (BRANCH_UNLIKELY(!r))
      break;
    if (BRANCH_UNLIKELY(!(*r == "REFR")))
    {
      if (*r == "CELL")
      {
        const ESMFile::ESMRecord  *r2;
        if (!(r->parent && bool(r2 = esmFile.getRecordPtr(r->parent)) &&
              r2->formID == 1U))        // ignore starting cell at 0, 0
        {
          if (type == 0)
            addTerrainCell(*r);
          else
            addWaterCell(*r);
        }
      }
      if (!isRecursive)
        break;
      if (*r == "GRUP" && r->children)
      {
        if (r->formID > 0U && r->formID < (!type ? 6U : 10U) && r->formID != 7U)
          findObjects(r->children, type, true);
      }
      continue;
    }
    if ((r->flags & (!noDisabledObjects ? 0x00000020 : 0x00000820)) || !type)
      continue;                         // ignore deleted and disabled records
    const ESMFile::ESMRecord  *r2 = (ESMFile::ESMRecord *) 0;
    float   scale = 1.0f;
    float   rX = 0.0f;
    float   rY = 0.0f;
    float   rZ = 0.0f;
    float   offsX = 0.0f;
    float   offsY = 0.0f;
    float   offsZ = 0.0f;
    unsigned int  refrMSWPFormID = 0U;
#if ENABLE_TXST_DECALS
    unsigned int  xpddScale = 0x3C003C00U;      // 1.0, 1.0 in FP16 format
#endif
    {
      ESMFile::ESMField f(esmFile, *r);
      while (f.next())
      {
        if (f == "NAME" && f.size() >= 4)
        {
          r2 = esmFile.getRecordPtr(f.readUInt32Fast());
        }
        else if (f == "DATA" && f.size() >= 24)
        {
          FloatVector4  tmp(f.readFloatVector4());
          offsX = tmp[0];
          offsY = tmp[1];
          f.setPosition(f.getPosition() - 8);
          tmp = f.readFloatVector4();
          offsZ = tmp[0];
          rX = tmp[1];
          rY = tmp[2];
          rZ = tmp[3];
        }
        else if (f == "XSCL" && f.size() >= 4)
        {
          scale = f.readFloat();
        }
        else if (f == "XMSP" && f.size() >= 4)
        {
          refrMSWPFormID = f.readUInt32Fast();
        }
#if ENABLE_TXST_DECALS
        else if (BRANCH_UNLIKELY(f == "XPDD") && r2 && *r2 == "TXST" &&
                 f.size() >= 8)
        {
          float   xScale = f.readFloat();
          float   zScale = f.readFloat();
          xScale = std::min(std::max(xScale, 0.125f), 8.0f);
          zScale = std::min(std::max(zScale, 0.125f), 8.0f);
          xpddScale = std::uint32_t(convertToFloat16(xScale))
                      | (std::uint32_t(convertToFloat16(zScale)) << 16);
        }
#endif
      }
    }
    if (!r2)
      continue;
    if (*r2 == "SCOL" && !enableSCOL)
    {
      addSCOLObjects(*r2, scale, rX, rY, rZ, offsX, offsY, offsZ,
                     refrMSWPFormID);
      continue;
    }
    RenderObject  tmp;
    tmp.tileIndex = -1;
    tmp.z = 0;
    const BaseObject  *o = readModelProperties(tmp, *r2);
    if (!o)
      continue;
#if ENABLE_TXST_DECALS
    if (BRANCH_UNLIKELY(tmp.flags & 0x10))
    {
      tmp.mswpFormID = xpddScale;
      scale = 1.0f;
      refrMSWPFormID = 0U;
    }
#endif
    tmp.modelTransform = NIFFile::NIFVertexTransform(scale, rX, rY, rZ,
                                                     offsX, offsY, offsZ);
    if (setScreenAreaUsed(tmp) < 0)
      continue;
    if (debugMode == 1)
      tmp.z = int(r->formID);
    if (tmp.flags & 4)
    {
      if (waterRenderMode >= 0)
        tmp.mswpFormID = 0U;
      else
        tmp.mswpFormID = getWaterMaterial(materials, esmFile, r2, 0U);
    }
    else if (refrMSWPFormID)
    {
      if (refrMSWPFormID != o->mswpFormID)
      {
        refrMSWPFormID =
            materialSwaps.loadMaterialSwap(ba2File, esmFile, refrMSWPFormID);
      }
      tmp.mswpFormID = refrMSWPFormID;
    }
    objectList.push_back(tmp);
  }
  while ((formID = r->next) != 0U && isRecursive);
}

void Renderer::findObjects(unsigned int formID, int type)
{
  if (!formID)
    return;
  const ESMFile::ESMRecord  *r = esmFile.getRecordPtr(formID);
  if (!r)
    return;
  if (*r == "WRLD")
  {
    r = esmFile.getRecordPtr(0U);
    while (r && r->next)
    {
      r = esmFile.getRecordPtr(r->next);
      if (!r)
        break;
      if (*r == "GRUP" && r->formID == 0 && r->children)
      {
        unsigned int  groupID = r->children;
        while (groupID)
        {
          const ESMFile::ESMRecord  *r2 = esmFile.getRecordPtr(groupID);
          if (!r2)
            break;
          if (*r2 == "GRUP" && r2->formID == 1 && r2->flags == formID &&
              r2->children)
          {
            // world children
            findObjects(r2->children, type, true);
          }
          groupID = r2->next;
        }
      }
    }
  }
  else if (*r == "CELL")
  {
    findObjects(formID, type, false);
    r = esmFile.getRecordPtr(0U);
    while (r)
    {
      if (*r == "GRUP")
      {
        if (r->formID <= 5U && r->children)
        {
          r = esmFile.getRecordPtr(r->children);
          continue;
        }
        else if (r->formID <= 9U && r->formID != 7U && r->flags == formID &&
                 r->children)
        {
          // cell children
          findObjects(r->children, type, true);
        }
      }
      while (r && !r->next && r->parent)
        r = esmFile.getRecordPtr(r->parent);
      if (!(r && r->next))
        break;
      r = esmFile.getRecordPtr(r->next);
    }
  }
  else if (!(*r == "GRUP"))
  {
    findObjects(formID, type, false);
  }
  else if (r->children)
  {
    findObjects(r->children, type, true);
  }
}

void Renderer::sortObjectList()
{
  if (renderPass & 4)
  {
    for (size_t i = 0; i < objectList.size(); )
    {
      if (!(objectList[i].flags & 0x08))
      {
        objectList[i] = objectList.back();
        objectList.pop_back();
      }
      else
      {
        if (debugMode != 1)
          objectList[i].z = 0x70000000 - objectList[i].z;       // reverse sort
        i++;
      }
    }
  }
  std::map< std::string, unsigned int > modelPathsUsed;
  for (size_t i = 0; i < objectList.size(); i++)
  {
    if (objectList[i].flags & 0x12)
      modelPathsUsed[objectList[i].model.o->modelPath] = 0U;
  }
  unsigned int  n = 0U;
#if ENABLE_TXST_DECALS
  bool    decalFlag = false;
#endif
  for (std::map< std::string, unsigned int >::iterator
           i = modelPathsUsed.begin(); i != modelPathsUsed.end(); i++, n++)
  {
#if ENABLE_TXST_DECALS
    if (BRANCH_UNLIKELY(i->first.c_str()[0] == '~') && !decalFlag)
    {
      n = (n + modelBatchCnt - 1U) & ~(modelBatchCnt - 1U);
      decalFlag = true;
    }
#endif
    i->second = n;
  }
  for (std::map< unsigned int, BaseObject >::iterator
           i = baseObjects.begin(); i != baseObjects.end(); i++)
  {
    std::map< std::string, unsigned int >::const_iterator j =
        modelPathsUsed.find(i->second.modelPath);
    if (j == modelPathsUsed.end())
      i->second.modelID = 0xFFFFFFFFU;
    else
      i->second.modelID = j->second;
  }
  std::sort(objectList.begin(), objectList.end());
}

void Renderer::clear(unsigned int flags)
{
  if (flags & 0x03)
  {
    size_t  imageDataSize = size_t(width) * size_t(height);
    if (flags & 0x01)
    {
      for (size_t i = 0; i < imageDataSize; i++)
        outBufRGBA[i] = 0U;
    }
    if (flags & 0x02)
    {
      float   tmp = float(zRangeMax);
      for (size_t i = 0; i < imageDataSize; i++)
        outBufZ[i] = tmp;
      if (outBufN)
      {
        for (size_t i = 0; i < imageDataSize; i++)
          outBufN[i] = 0U;
      }
    }
  }
  if (flags & 0x04)
  {
    if (landData)
    {
      delete landData;
      landData = (LandscapeData *) 0;
    }
    if (landTextures)
    {
      delete[] landTextures;
      landTextures = (LandscapeTextureSet *) 0;
    }
  }
  if (flags & 0x08)
  {
    objectList.clear();
    renderPass = 0;
    objectListPos = 0;
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
    modelIDBase = 0xFFFFFFFFU;
  }
  if (flags & 0x40)
    textureCache.clear();
}

bool Renderer::isExcludedModel(const std::string& modelPath) const
{
  for (size_t i = 0; i < excludeModelPatterns.size(); i++)
  {
    if (modelPath.find(excludeModelPatterns[i]) != std::string::npos)
      return true;
  }
  return false;
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

void Renderer::loadModels(unsigned int t, unsigned long long modelIDMask)
{
  for (size_t n = 0; modelIDMask; modelIDMask = modelIDMask >> 1, n++)
  {
    if (!(modelIDMask & 1U))
      continue;
    const BaseObject& o = *(nifFiles[n].o);
    nifFiles[n].clear();
    if (o.modelID == 0xFFFFFFFFU || o.modelPath.empty())
      continue;
    if (BRANCH_UNLIKELY(renderPass & 4) && !(o.flags & 4))
    {
      if (disableEffectMeshes)
        continue;
      if (!disableEffectFilters)
      {
        if (std::strncmp(o.modelPath.c_str(), "meshes/sky/", 11) == 0)
          continue;
        if (std::strncmp(o.modelPath.c_str(), "meshes/effects/", 15) == 0)
        {
          size_t  len = o.modelPath.length();
          if (std::strncmp(o.modelPath.c_str() + 15, "ambient/", 8) == 0 ||
              std::strcmp(o.modelPath.c_str() + (len - 7), "fog.nif") == 0 ||
              std::strcmp(o.modelPath.c_str() + (len - 9), "cloud.nif") == 0)
          {
            continue;
          }
        }
      }
    }
    try
    {
      ba2File.extractFile(nifFiles[t].fileBuf, o.modelPath);
      nifFiles[n].nifFile = new NIFFile(nifFiles[t].fileBuf.data(),
                                        nifFiles[t].fileBuf.size(), &ba2File);
      bool    isHDModel = bool(o.flags & 0x0040);
      nifFiles[n].nifFile->getMesh(nifFiles[n].meshData, 0U,
                                   (unsigned int) (modelLOD && !isHDModel),
                                   true);
      size_t  meshCnt = nifFiles[n].meshData.size();
      for (size_t i = 0; i < meshCnt; i++)
      {
        const NIFFile::NIFTriShape& ts = nifFiles[n].meshData[i];
        // check hidden (0x8000) and alpha blending (0x1000) flags
        if (((ts.m.flags >> 10) ^ renderPass) & 0x24U)
          continue;
        nifFiles[n].totalTriangleCnt += size_t(ts.triangleCnt);
      }
    }
    catch (FO76UtilsError&)
    {
      nifFiles[n].clear();
    }
  }
}

void Renderer::loadModelsThread(Renderer *p,
                                unsigned int t, unsigned long long modelIDMask)
{
  try
  {
    p->nifFiles[t].threadErrMsg.clear();
    p->loadModels(t, modelIDMask);
  }
  catch (std::exception& e)
  {
    p->nifFiles[t].threadErrMsg = e.what();
    if (p->nifFiles[t].threadErrMsg.empty())
      p->nifFiles[t].threadErrMsg = "unknown error in load model thread";
  }
}

void Renderer::renderDecal(RenderThread& t, const RenderObject& p)
{
#if ENABLE_TXST_DECALS
  NIFFile::NIFVertexTransform vt(p.modelTransform);
  vt *= viewTransform;
  FloatVector4  xpddScale(FloatVector4::convertFloat16(
                              std::uint64_t(p.mswpFormID)));
  xpddScale[2] = xpddScale[1];
  xpddScale[1] = 1.0f;
  NIFFile::NIFBounds  decalBounds;
  decalBounds.boundsMin =
      FloatVector4(float(p.model.o->obndX0), float(p.model.o->obndY0),
                   float(p.model.o->obndZ0), 0.0f) * xpddScale;
  decalBounds.boundsMax =
      FloatVector4(float(p.model.o->obndX1), float(p.model.o->obndY1),
                   float(p.model.o->obndZ1), 0.0f) * xpddScale;
  decalBounds.boundsMin[1] = getDecalYOffsetMin(decalBounds.boundsMin);
  decalBounds.boundsMax[1] = getDecalYOffsetMax(decalBounds.boundsMax);
  float   yOffset = 0.0f;
  if (!t.renderer->findDecalYOffset(yOffset, p.modelTransform, decalBounds))
    return;
  decalBounds.boundsMin[1] = float(p.model.o->obndY0) + yOffset;
  decalBounds.boundsMax[1] = float(p.model.o->obndY1) + yOffset;
  NIFFile::NIFBounds  b;
  NIFFile::NIFVertex  v;
  for (int i = 0; i < 8; i++)
  {
    v.x = float(!(i & 1) ? decalBounds.boundsMin[0] : decalBounds.boundsMax[0]);
    v.y = float(!(i & 2) ? decalBounds.boundsMin[1] : decalBounds.boundsMax[1]);
    v.z = float(!(i & 4) ? decalBounds.boundsMin[2] : decalBounds.boundsMax[2]);
    vt.transformXYZ(v.x, v.y, v.z);
    b += v;
  }
  int     x0 = roundFloat(b.xMin());
  int     y0 = roundFloat(b.yMin());
  int     z0 = roundFloat(b.zMin());
  int     x1 = roundFloat(b.xMax());
  int     y1 = roundFloat(b.yMax());
  int     z1 = roundFloat(b.zMax());
  if (x0 >= width || x1 < 0 || y0 >= height || y1 < 0 ||
      z0 >= zRangeMax || z1 < 0)
  {
    return;
  }
  x0 = (x0 > 0 ? x0 : 0);
  y0 = (y0 > 0 ? y0 : 0);
  x1 = (x1 < (width - 1) ? x1 : (width - 1)) + 1;
  y1 = (y1 < (height - 1) ? y1 : (height - 1)) + 1;
  bool    isVisible = false;
  for (int y = y0; y < y1; y++)
  {
    FloatVector4  zMax(0.0f);
    const float *zPtr = outBufZ + (size_t(y) * size_t(width) + size_t(x0));
    int     w = x1 + 1 - x0;
    for ( ; w >= 4; w = w - 4, zPtr = zPtr + 4)
      zMax.maxValues(FloatVector4(zPtr));
    zMax.maxValues(FloatVector4(zMax[1], zMax[0], zMax[3], zMax[2]));
    float   tmp = (zMax[0] > zMax[2] ? zMax[0] : zMax[2]);
    for ( ; w > 0; w--, zPtr++)
      tmp = (*zPtr > tmp ? *zPtr : tmp);
    if (tmp < b.zMin())
      continue;
    isVisible = true;
    break;
  }
  if (!isVisible)
    return;
  std::uint16_t renderModeQuality =
      renderMode | renderQuality | ((p.flags >> 5) & 2);
  std::uint16_t texturePathMaskBase = 0x0009;
  if (renderModeQuality & 2)
    texturePathMaskBase = 0x037B;
  else if (renderQuality >= 1)
    texturePathMaskBase = 0x000B;
  std::map< unsigned int, BGSMFile >::const_iterator  i =
      materials.find(p.model.o->mswpFormID);
  if (i == materials.end())
    return;
  t.renderer->m = i->second;
  const DDSTexture  *textures[10];
  unsigned int  textureMask = 0U;
  if (p.flags & 0x80)
    t.renderer->m.s.gradientMapV = float(int(p.flags >> 8)) * (1.0f / 255.0f);
  unsigned int  texturePathMask = t.renderer->m.texturePathMask;
  texturePathMask &= ((((unsigned int) t.renderer->m.flags & 0x80U) >> 5)
                      | texturePathMaskBase);
  if (!(texturePathMask & 0x0001U) ||
      t.renderer->m.texturePaths[0].find("/temp_ground") != std::string::npos)
  {
    return;
  }
  if (BRANCH_UNLIKELY(!enableTextures))
  {
    if (t.renderer->m.alphaThresholdFloat > 0.0f || (texturePathMask & 0x0008U))
    {
      texturePathMask &= ~0x0008U;
      textures[3] = &whiteTexture;
      textureMask |= 0x0008U;
    }
    else
    {
      texturePathMask &= ~0x0001U;
      textures[0] = &whiteTexture;
      textureMask |= 0x0001U;
    }
  }
  for (unsigned int m = 0x00080200U; texturePathMask; m = m >> 1)
  {
    unsigned int  tmp = texturePathMask & m;
    if (!tmp)
      continue;
    int     k = FloatVector4::log2Int(int(tmp));
    bool    waitFlag = false;
    textures[k] = textureCache.loadTexture(
                      ba2File, t.renderer->m.texturePaths[k], t.fileBuf,
                      (!(m & 0x0018U) ? textureMip : 0),
                      (m > 0x03FFU ? &waitFlag : (bool *) 0));
    if (!waitFlag)
    {
      texturePathMask &= ~tmp;
      if (textures[k])
        textureMask |= tmp;
    }
  }
  if (((textureMask ^ texturePathMaskBase) & 0x0010U) &&
      t.renderer->m.s.envMapScale > 0.0f)
  {
    textures[4] = textureCache.loadTexture(ba2File, defaultEnvMap,
                                           t.fileBuf, 0);
    if (textures[4])
      textureMask |= 0x0010U;
  }
  t.renderer->setRenderMode(renderModeQuality);
  t.renderer->drawDecal(p.modelTransform, textures, textureMask,
                        decalBounds, 0xFFFFFFFFU);
#else
  (void) t;
  (void) p;
#endif
}

bool Renderer::renderObject(RenderThread& t, size_t i,
                            unsigned long long tileMask)
{
  const RenderObject& p = objectList[i];
  t.renderer->setDebugMode(debugMode, (unsigned int) p.z);
  if (p.flags & 0x02)                   // object or water mesh
  {
    NIFFile::NIFVertexTransform vt(p.modelTransform);
    vt *= viewTransform;
    size_t  n = p.model.o->modelID & (modelBatchCnt - 1U);
    t.sortBuf.clear();
    t.sortBuf.reserve(nifFiles[n].meshData.size());
    for (size_t j = 0; j < nifFiles[n].meshData.size(); j++)
    {
      const NIFFile::NIFTriShape& ts = nifFiles[n].meshData[j];
      // check hidden (0x8000) and alpha blending (0x1000) flags
      if (((ts.m.flags >> 10) ^ renderPass) & 0x24U)
      {
        if ((ts.m.flags & BGSMFile::Flag_TSAlphaBlending) &&
            (ts.m.texturePathMask | (p.flags & 0x0020)
             | (ts.m.flags & BGSMFile::Flag_TSWater)))
        {
          objectList[i].flags |= (unsigned short) 0x08; // uses alpha blending
        }
        continue;
      }
      if (!ts.triangleCnt)
        continue;
      NIFFile::NIFBounds  b;
      ts.calculateBounds(b, &vt);
      int     x0 = roundFloat(b.xMin());
      int     y0 = roundFloat(b.yMin());
      int     x1 = roundFloat(b.xMax());
      int     y1 = roundFloat(b.yMax());
      unsigned long long  m = calculateTileMask(x0, y0, x1, y1);
      if (!m)
        continue;
      if (m & ~tileMask)
        return false;
      x0 = (x0 > 0 ? x0 : 0);
      y0 = (y0 > 0 ? y0 : 0);
      x1 = (x1 < (width - 1) ? x1 : (width - 1)) + 1;
      y1 = (y1 < (height - 1) ? y1 : (height - 1)) + 1;
      bool    isVisible = false;
      for (int y = y0; y < y1; y++)
      {
        FloatVector4  zMax(0.0f);
        const float *zPtr = outBufZ + (size_t(y) * size_t(width) + size_t(x0));
        int     w = x1 + 1 - x0;
        for ( ; w >= 4; w = w - 4, zPtr = zPtr + 4)
          zMax.maxValues(FloatVector4(zPtr));
        zMax.maxValues(FloatVector4(zMax[1], zMax[0], zMax[3], zMax[2]));
        float   tmp = (zMax[0] > zMax[2] ? zMax[0] : zMax[2]);
        for ( ; w > 0; w--, zPtr++)
          tmp = (*zPtr > tmp ? *zPtr : tmp);
        if (tmp < b.zMin())
          continue;
        isVisible = true;
        break;
      }
      if (!isVisible)
        continue;
      t.sortBuf.emplace(t.sortBuf.end(), j, b.zMin(),
                        bool(ts.m.flags & BGSMFile::Flag_TSAlphaBlending));
      if (BRANCH_UNLIKELY(ts.m.flags & BGSMFile::Flag_TSOrdered))
        TriShapeSortObject::orderedNodeFix(t.sortBuf, nifFiles[n].meshData);
    }
    if (t.sortBuf.size() < 1)
      return true;
    std::sort(t.sortBuf.begin(), t.sortBuf.end());
    std::uint16_t renderModeQuality =
        renderMode | renderQuality | ((p.flags >> 5) & 2);
    std::uint16_t texturePathMaskBase = 0x0009;
    if (renderModeQuality & 2)
      texturePathMaskBase = 0x037B;
    else if (renderQuality >= 1)
      texturePathMaskBase = 0x000B;
    for (size_t j = 0; j < t.sortBuf.size(); j++)
    {
      *(t.renderer) = nifFiles[n].meshData[size_t(t.sortBuf[j])];
      const DDSTexture  *textures[10];
      unsigned int  textureMask = 0U;
      if (BRANCH_UNLIKELY(t.renderer->m.flags & BGSMFile::Flag_TSWater))
      {
        t.renderer->setRenderMode(3U | renderMode);
        std::map< unsigned int, BGSMFile >::const_iterator  k =
            materials.find(p.mswpFormID);
        if (k == materials.end())
          continue;
        t.renderer->setMaterial(k->second);
        t.renderer->m.w.envMapScale = waterReflectionLevel;
        textures[1] = textureCache.loadTexture(ba2File, defaultWaterTexture,
                                               t.fileBuf, 0);
        if (textures[1])
          textureMask |= 0x0002U;
        textures[4] = textureCache.loadTexture(ba2File, defaultEnvMap,
                                               t.fileBuf, 0);
        if (textures[4])
          textureMask |= 0x0010U;
      }
      else
      {
        if (p.model.o->mswpFormID && p.mswpFormID != p.model.o->mswpFormID)
          materialSwaps.materialSwap(*(t.renderer), p.model.o->mswpFormID);
        if (p.flags & 0x80)
        {
          t.renderer->m.s.gradientMapV =
              float(int(p.flags >> 8)) * (1.0f / 255.0f);
        }
        if (p.mswpFormID)
          materialSwaps.materialSwap(*(t.renderer), p.mswpFormID);
        unsigned int  texturePathMask = t.renderer->m.texturePathMask;
        texturePathMask &= ((((unsigned int) t.renderer->m.flags & 0x80U) >> 5)
                            | texturePathMaskBase);
        if (!(texturePathMask & 0x0001U) ||
            t.renderer->m.texturePaths[0].find("/temp_ground")
            != std::string::npos)
        {
          if ((texturePathMask & 0x0001U) || !(p.flags & 0x0020))
            continue;
          textures[0] = &whiteTexture;  // marker with vertex colors only
          textureMask |= 0x0001U;
        }
        if (BRANCH_UNLIKELY(!enableTextures))
        {
          if (t.renderer->m.alphaThresholdFloat > 0.0f ||
              (texturePathMask & 0x0008U))
          {
            texturePathMask &= ~0x0008U;
            textures[3] = &whiteTexture;
            textureMask |= 0x0008U;
          }
          else
          {
            texturePathMask &= ~0x0001U;
            textures[0] = &whiteTexture;
            textureMask |= 0x0001U;
          }
        }
        for (unsigned int m = 0x00080200U; texturePathMask; m = m >> 1)
        {
          unsigned int  tmp = texturePathMask & m;
          if (!tmp)
            continue;
          int     k = FloatVector4::log2Int(int(tmp));
          bool    waitFlag = false;
          textures[k] = textureCache.loadTexture(
                            ba2File, t.renderer->m.texturePaths[k], t.fileBuf,
                            (!(m & 0x0018U) ? textureMip : 0),
                            (m > 0x03FFU ? &waitFlag : (bool *) 0));
          if (!waitFlag)
          {
            texturePathMask &= ~tmp;
            if (textures[k])
              textureMask |= tmp;
          }
        }
        if (((textureMask ^ texturePathMaskBase) & 0x0010U) &&
            t.renderer->m.s.envMapScale > 0.0f)
        {
          textures[4] = textureCache.loadTexture(ba2File, defaultEnvMap,
                                                 t.fileBuf, 0);
          if (textures[4])
            textureMask |= 0x0010U;
        }
      }
      t.renderer->setRenderMode(renderModeQuality);
      t.renderer->drawTriShape(p.modelTransform, textures, textureMask);
    }
  }
  else if (p.flags & 0x01)              // terrain
  {
    if (landData && t.terrainMesh)
    {
      int     landTxtScale = 0;
      for (int j = landData->getCellResolution(); j < cellTextureResolution; )
      {
        landTxtScale++;
        j = j << 1;
      }
      unsigned int  ltexMask = 0x0001U;
      if (renderQuality >= 1)
      {
        ltexMask = 0x0003U;
        if (renderQuality >= 3 && esmFile.getESMVersion() >= 0x80U)
          ltexMask = (esmFile.getESMVersion() < 0xC0U ? 0x0043U : 0x0303U);
      }
      t.terrainMesh->createMesh(
          *landData, landTxtScale,
          p.model.t.x0, p.model.t.y0, p.model.t.x1, p.model.t.y1,
          landTextures, landData->getTextureCount(), ltexMask,
          landTextureMip - float(int(landTextureMip)),
          landTxtRGBScale, landTxtDefColor);
      t.renderer->setRenderMode(
          (((ltexMask >> 1) | (ltexMask >> 5) | (ltexMask >> 8)) & 3U)
          | renderMode);
      *(t.renderer) = *(t.terrainMesh);
      t.renderer->drawTriShape(
          p.modelTransform,
          t.terrainMesh->getTextures(), t.terrainMesh->getTextureMask());
    }
  }
  else if ((p.flags & renderPass) & 0x04)       // water cell
  {
    NIFFile::NIFTriShape  tmp;
    NIFFile::NIFVertex    vTmp[4];
    NIFFile::NIFTriangle  tTmp[2];
    int     x0 = p.model.t.x0;
    int     y0 = p.model.t.y0;
    int     x1 = p.model.t.x1;
    int     y1 = p.model.t.y1;
    for (int j = 0; j < 4; j++)
    {
      vTmp[j].x = float(j == 0 || j == 3 ? x0 : x1);
      vTmp[j].y = float(j == 0 || j == 1 ? y0 : y1);
      vTmp[j].u = (unsigned short) (j == 0 || j == 3 ? 0x0000 : 0x4000);
      vTmp[j].v = (unsigned short) (j == 0 || j == 1 ? 0x0000 : 0x4000);
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
    std::map< unsigned int, BGSMFile >::const_iterator  j =
        materials.find(p.mswpFormID);
    if (j == materials.end())
      return true;
    tmp.m = j->second;
    tmp.m.w.envMapScale = waterReflectionLevel;
    t.renderer->setRenderMode(3U | renderMode);
    *(t.renderer) = tmp;
    const DDSTexture  *textures[5];
    unsigned int  textureMask = 0U;
    textures[1] = textureCache.loadTexture(ba2File, defaultWaterTexture,
                                           t.fileBuf, 0);
    if (textures[1])
      textureMask |= 0x0002U;
    textures[4] = textureCache.loadTexture(ba2File, defaultEnvMap,
                                           t.fileBuf, 0);
    if (textures[4])
      textureMask |= 0x0010U;
    t.renderer->drawTriShape(p.modelTransform, textures, textureMask);
  }
#if ENABLE_TXST_DECALS
  else if ((p.flags & 0x10) && !(renderPass & 0x04))    // decal
  {
    renderDecal(t, p);
  }
#endif
  return true;
}

void Renderer::renderThread(size_t threadNum, size_t startPos, size_t endPos,
                            unsigned long long tileIndexMask)
{
  RenderThread& t = renderThreads[threadNum];
  if (!t.renderer)
    return;
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
    if (BRANCH_UNLIKELY(!tileMask))
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
    if (!renderObject(t, i, tileMask))
      t.objectsRemaining.push_back((unsigned int) i);
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
                   std::uint32_t *bufRGBA, float *bufZ, int zMax)
  : outBufRGBA(bufRGBA),
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
    defaultWaterLevel(0.0f),
    modelLOD(0),
    enableMarkers(false),
    distantObjectsOnly(false),
    noDisabledObjects(false),
    enableDecals(false),
    enableSCOL(false),
    enableAllObjects(false),
    enableTextures(true),
    renderQuality(0),
    renderMode((unsigned char) ((masterFiles.getESMVersion() >> 4) & 0x0CU)),
    debugMode(0),
    renderPass(0),
    threadCnt(0),
    landTextures((LandscapeTextureSet *) 0),
    objectListPos(0),
    modelIDBase(0xFFFFFFFFU),
    waterReflectionLevel(1.0f),
    zRangeMax(zMax),
    disableEffectMeshes(false),
    disableEffectFilters(false),
    waterRenderMode(0),
    bufAllocFlags((unsigned char) (int(!bufRGBA) | (int(!bufZ) << 1))),
    whiteTexture(0xFFFFFFFFU),
    outBufN((std::uint32_t *) 0)
{
  if (!renderMode)
    renderMode = 4;
  size_t  imageDataSize = size_t(width) * size_t(height);
  if (bufAllocFlags & 0x01)
    outBufRGBA = new std::uint32_t[imageDataSize];
  if (bufAllocFlags & 0x02)
    outBufZ = new float[imageDataSize];
  clear(bufAllocFlags);
  renderThreads.reserve(16);
  nifFiles.resize(modelBatchCnt);
  setThreadCount(-1);
  setWaterColor(0xFFFFFFFFU);
}

Renderer::~Renderer()
{
  clear(0x7C);
  deallocateBuffers(0x03);
}

unsigned int Renderer::findParentWorld(ESMFile& esmFile, unsigned int formID)
{
  if (!formID)
    return 0xFFFFFFFFU;
  const ESMFile::ESMRecord  *r = esmFile.getRecordPtr(formID);
  if (!(r && (*r == "WRLD" || *r == "GRUP" || *r == "CELL" || *r == "REFR")))
    return 0xFFFFFFFFU;
  if (!(*r == "WRLD"))
  {
    while (!(*r == "GRUP" && r->formID == 1U))
    {
      if (!r->parent)
        return 0U;
      r = esmFile.getRecordPtr(r->parent);
      if (!r)
        return 0U;
    }
    formID = r->flags;
    r = esmFile.getRecordPtr(formID);
  }
  return formID;
}

void Renderer::clear()
{
  clear(0x7C);
  baseObjects.clear();
}

void Renderer::clearImage(unsigned int flags)
{
  clear(flags & 3U);
}

void Renderer::deallocateBuffers(unsigned int mask)
{
  if ((bufAllocFlags & mask) & 0x02)
  {
    if (outBufZ)
    {
      delete[] outBufZ;
      outBufZ = (float *) 0;
    }
    if (outBufN)
    {
      delete[] outBufN;
      outBufN = (std::uint32_t *) 0;
    }
  }
  if (((bufAllocFlags & mask) & 0x01) && outBufRGBA)
  {
    delete[] outBufRGBA;
    outBufRGBA = (std::uint32_t *) 0;
  }
  bufAllocFlags = bufAllocFlags & ~mask;
}

void Renderer::setViewTransform(
    float scale, float rotationX, float rotationY, float rotationZ,
    float offsetX, float offsetY, float offsetZ)
{
  NIFFile::NIFVertexTransform t(scale, rotationX, rotationY, rotationZ,
                                offsetX, offsetY, offsetZ);
  viewTransform = t;
}

void Renderer::setLightDirection(float rotationY, float rotationZ)
{
  NIFFile::NIFVertexTransform t(1.0f, 0.0f, rotationY, rotationZ,
                                0.0f, 0.0f, 0.0f);
  lightX = t.rotateZX;
  lightY = t.rotateZY;
  lightZ = t.rotateZZ;
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
      renderThreads[i].renderer =
          new Plot3D_TriShape(outBufRGBA, outBufZ, width, height, renderMode);
    }
  }
}

void Renderer::addExcludeModelPattern(const std::string& s)
{
  if (s.empty())
    return;
  for (size_t i = 0; i < excludeModelPatterns.size(); i++)
  {
    if (s.find(excludeModelPatterns[i]) != std::string::npos)
      return;
    if (excludeModelPatterns[i].find(s) != std::string::npos)
      excludeModelPatterns.erase(excludeModelPatterns.begin() + i);
  }
  excludeModelPatterns.push_back(s);
}

void Renderer::addHDModelPattern(const std::string& s)
{
  if (s.empty())
    return;
  if (renderQuality < 1)
    renderQuality = 1;
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

void Renderer::setWaterColor(std::uint32_t n)
{
  waterRenderMode = (signed char) (n == 0xFFFFFFFFU ? -1 : (!n ? 0 : 1));
  if (waterRenderMode <= 0)
    n = 0xC0302010U;                    // default water color if not specified
  else
    n = bgraToRGBA(n);
  getWaterMaterial(materials, esmFile, (ESMFile::ESMRecord *) 0, n, true);
}

void Renderer::setRenderParameters(
    int lightColor, int ambientColor, int envColor,
    float lightLevel, float envLevel, float rgbScale,
    float reflZScale, int waterUVScale)
{
  if (renderThreads.size() < 1)
    return;
  FloatVector4  c(bgraToRGBA((std::uint32_t) lightColor));
  FloatVector4  a(bgraToRGBA((std::uint32_t) ambientColor & 0x00FFFFFFU));
  FloatVector4  e(bgraToRGBA((std::uint32_t) envColor));
  FloatVector4  l(lightLevel, envLevel, rgbScale, 0.0f);
  l = ((l * -0.01728125f + 0.17087940f) * l + 0.83856107f) * l + 0.00784078f;
  l *= l;
  c = c.srgbExpand() * l[0];
  e = e.srgbExpand() * l[1];
  if (ambientColor >= 0)
  {
    a.srgbExpand();
  }
  else
  {
    a = renderThreads[0].renderer->cubeMapToAmbient(
            textureCache.loadTexture(
                ba2File, defaultEnvMap, renderThreads[0].fileBuf, 0));
  }
  for (size_t i = 0; i < renderThreads.size(); i++)
  {
    renderThreads[i].renderer->setLighting(c, a, e, l[2]);
    renderThreads[i].renderer->setEnvMapOffset(
        float(width) * -0.5f, float(height) * -0.5f,
        float(height) * reflZScale);
    renderThreads[i].renderer->setWaterUVScale(1.0f / float(waterUVScale));
  }
}

static int calculateLandTxtMip(long fileSize)
{
  int     m = 0;
  while ((8L << (m + m)) <= (fileSize - 128L))
    m++;
  // return log2(resolution)
  return (m - 1);
}

void Renderer::loadTerrain(const char *btdFileName,
                           unsigned int worldID, unsigned int defTxtID,
                           int mipLevel, int xMin, int yMin, int xMax, int yMax)
{
  if (!landData)
  {
    landData = new LandscapeData(&esmFile, btdFileName, &ba2File, 0x0B, worldID,
                                 defTxtID, mipLevel, xMin, yMin, xMax, yMax);
    defaultWaterLevel = landData->getWaterLevel();
  }
  size_t  textureCnt = landData->getTextureCount();
  if (!landTextures)
    landTextures = new LandscapeTextureSet[textureCnt];
  std::vector< unsigned char >& fileBuf(renderThreads[0].fileBuf);
  int     mipLevelD = std::min(textureMip + int(landTextureMip), 15);
  for (size_t i = 0; i < textureCnt; i++)
  {
    if (!enableTextures)
    {
      landTextures[i][0] = &whiteTexture;
    }
    else
    {
      landTextures[i][0] = textureCache.loadTexture(
                               ba2File, landData->getTextureDiffuse(i), fileBuf,
                               mipLevelD);
    }
    if (renderQuality < 1)
      continue;
    long    fileSizeD = ba2File.getFileSize(landData->getTextureDiffuse(i));
    for (size_t j = 1; j < 10; j++)
    {
      if (j >= 2 && !(renderQuality >= 3 && (j == 6 || j >= 8)))
        continue;
      const std::string&  fileName =
          landData->getTextureMaterial(i).texturePaths[j];
      long    fileSizeN = ba2File.getFileSize(fileName);
      int     mipLevelN = mipLevelD + calculateLandTxtMip(fileSizeN)
                          - calculateLandTxtMip(fileSizeD);
      mipLevelN = (mipLevelN > 0 ? (mipLevelN < 15 ? mipLevelN : 15) : 0);
      landTextures[i][j] = textureCache.loadTexture(
                               ba2File, fileName, fileBuf, mipLevelN);
    }
  }
}

void Renderer::initRenderPass(int n, unsigned int formID)
{
  if (!formID)
    formID = getDefaultWorldID();
  clear(n != 2 ? 0x38U : 0x30U);
  objectListPos = 0;
  switch (n)
  {
    case 0:
      if (!landData)
        return;
      renderPass = 1;
      findObjects(formID, 0);
      for (size_t i = 0; i < renderThreads.size(); i++)
      {
        if (!renderThreads[i].terrainMesh)
          renderThreads[i].terrainMesh = new TerrainMesh();
      }
      break;
    case 1:
      renderPass = 2;
      findObjects(formID, 1);
      break;
    case 2:
      renderPass = 4;
      if (waterRenderMode < 0 && landData)
      {
        const ESMFile::ESMRecord  *r =
            esmFile.getRecordPtr(landData->getWaterFormID());
        if (r && *r == "WATR")
          getWaterMaterial(materials, esmFile, r, 0xC0302010U, true);
      }
      break;
    default:
      return;
  }
  sortObjectList();
  threadSortBuf.resize(64);
#if ENABLE_TXST_DECALS
  if (enableDecals && !debugMode && !outBufN)
  {
    size_t  imageDataSize = size_t(width) * size_t(height);
    outBufN = new std::uint32_t[imageDataSize];
    for (size_t i = 0; i < imageDataSize; i++)
      outBufN[i] = 0U;
  }
#endif
  for (size_t i = 0; i < renderThreads.size(); i++)
  {
    renderThreads[i].renderer->setBuffers(outBufRGBA, outBufZ, width, height,
                                          outBufN);
    renderThreads[i].renderer->setViewAndLightVector(viewTransform,
                                                     lightX, lightY, lightZ);
  }
}

bool Renderer::renderObjects()
{
  size_t  i = objectListPos;
  if (BRANCH_UNLIKELY(i >= objectList.size()))
    return true;
  for (size_t k = 0; k < 64; k++)
  {
    threadSortBuf[k].tileMask = 1ULL << (unsigned int) k;
    threadSortBuf[k].triangleCnt = 0;
  }
  const RenderObject& o = objectList[i];
  unsigned int  modelIDMask = modelBatchCnt - 1U;
  if ((o.flags & 0x12) && (o.model.o->modelID & ~modelIDMask) != modelIDBase)
  {
    // load new set of models
    modelIDBase = o.model.o->modelID & ~modelIDMask;
    for (unsigned int n = 0; n < modelBatchCnt; n++)
      nifFiles[n].clear();
    unsigned long long  m = 0ULL;
    for (size_t j = i; j < objectList.size(); j++)
    {
      const RenderObject& p = objectList[j];
      if (!(p.flags & 0x02))
        continue;
      if ((p.model.o->modelID & ~modelIDMask) != modelIDBase)
        break;
      unsigned int  n = p.model.o->modelID & modelIDMask;
      nifFiles[n].o = p.model.o;
      m = m | (1ULL << n);
    }
    unsigned long long  tmp = 0ULL;
    unsigned int  nThreads = (unsigned int) threadCnt;
    nThreads = (nThreads < modelBatchCnt ? nThreads : modelBatchCnt);
    for (unsigned int k = 0; k < modelBatchCnt; k = k + nThreads)
      tmp = tmp | (1ULL << k);
    for (unsigned int k = 0; k < nThreads; k++, tmp = tmp << 1)
    {
      nifFiles[k].loadThread =
          new std::thread(loadModelsThread, this, k, m & tmp);
    }
    for (unsigned int k = 0; k < nThreads; k++)
    {
      nifFiles[k].loadThread->join();
      delete nifFiles[k].loadThread;
      nifFiles[k].loadThread = (std::thread *) 0;
    }
    for (unsigned int k = 0; k < nThreads; k++)
    {
      if (!nifFiles[k].threadErrMsg.empty())
        throw FO76UtilsError(1, nifFiles[k].threadErrMsg.c_str());
    }
    for (unsigned int n = 0; n < modelBatchCnt; n++)
    {
      if (!nifFiles[n].totalTriangleCnt)
        nifFiles[n].totalTriangleCnt = 1;
    }
  }
  size_t  j = i;
  while (j < objectList.size())
  {
    const RenderObject& p = objectList[j];
    if ((o.tileIndex ^ p.tileIndex) & ~63)
      break;
    if ((p.flags & 0x12) && (p.model.o->modelID & ~modelIDMask) != modelIDBase)
      break;
    size_t  triangleCnt = 2;
    if (p.flags & 0x02)
    {
      triangleCnt = nifFiles[p.model.o->modelID & modelIDMask].totalTriangleCnt;
    }
    else if ((p.flags & 0x01) && landData)
    {
      triangleCnt = size_t(landData->getCellResolution());
      if (triangleCnt > 24)
        triangleCnt = triangleCnt - 8;
      triangleCnt = triangleCnt * triangleCnt * 2U;
    }
    threadSortBuf[p.tileIndex & 63].triangleCnt += triangleCnt;
    j++;
  }
  // sort in descending order by triangle count
  std::sort(threadSortBuf.begin(), threadSortBuf.end());
  for (size_t k = size_t(threadCnt); k < threadSortBuf.size(); k++)
  {
    if (threadSortBuf[k].triangleCnt < 1)
      break;
    // find the thread with the lowest triangle count
    size_t  n = 0;
    for (size_t l = 1; l < size_t(threadCnt); l++)
    {
      if (threadSortBuf[l].triangleCnt < threadSortBuf[n].triangleCnt)
        n = l;
    }
    // add the remaining area with the highest triangle count
    threadSortBuf[n] += threadSortBuf[k];
  }
  size_t  threadsUsed = size_t(threadCnt);
  while (threadsUsed > 1 && threadSortBuf[threadsUsed - 1].triangleCnt < 1)
    threadsUsed--;
  for (size_t k = 1; k < threadsUsed; k++)
  {
    renderThreads[k].t = new std::thread(threadFunction, this, k, i, j,
                                         threadSortBuf[k].tileMask);
  }
  if (threadSortBuf[0].triangleCnt > 0)
    renderThread(0, i, j, threadSortBuf[0].tileMask);
  for (size_t k = 1; k < threadsUsed; k++)
    renderThreads[k].join();
  for (size_t k = 1; k < threadsUsed; k++)
  {
    if (!renderThreads[k].errMsg.empty())
      throw FO76UtilsError(1, renderThreads[k].errMsg.c_str());
  }
  // render any remaining objects that were skipped due to incorrect bounds
  for (size_t k = 0; k < threadsUsed; k++)
  {
    for (size_t l = 0; l < renderThreads[k].objectsRemaining.size(); l++)
      renderObject(renderThreads[k], renderThreads[k].objectsRemaining[l]);
    renderThreads[k].objectsRemaining.clear();
  }
  i = j;
  objectListPos = i;
  if (BRANCH_LIKELY(!landData))
    textureCache.shrinkTextureCache();
  if (BRANCH_UNLIKELY(i >= objectList.size()))
  {
    clear(0x20);
    return true;
  }
  return false;
}

