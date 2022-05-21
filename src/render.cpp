
#include "common.hpp"
#include "render.hpp"

#include <algorithm>

bool Renderer::RenderObject::operator<(const RenderObject& r) const
{
  if ((flags & 7) != (r.flags & 7))
    return ((flags & 7) < (r.flags & 7));
  if (flags & 2)
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
  if (p.flags & 2)
  {
    int     x0 = int(p.model.o->obndX0 < p.model.o->obndX1 ?
                     p.model.o->obndX0 : p.model.o->obndX1) - 2;
    int     x1 =
        x0 + std::abs(int(p.model.o->obndX1) - int(p.model.o->obndX0)) + 4;
    int     y0 = int(p.model.o->obndY0 < p.model.o->obndY1 ?
                     p.model.o->obndY0 : p.model.o->obndY1) - 2;
    int     y1 =
        y0 + std::abs(int(p.model.o->obndY1) - int(p.model.o->obndY0)) + 4;
    int     z0 = int(p.model.o->obndZ0 < p.model.o->obndZ1 ?
                     p.model.o->obndZ0 : p.model.o->obndZ1) - 2;
    int     z1 =
        z0 + std::abs(int(p.model.o->obndZ1) - int(p.model.o->obndZ0)) + 4;
    modelBounds.xMin = float(x0);
    modelBounds.yMin = float(y0);
    modelBounds.zMin = float(z0);
    modelBounds.xMax = float(x1);
    modelBounds.yMax = float(y1);
    modelBounds.zMax = float(z1);
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
    modelBounds.xMin = float(x0) * xyScale + xOffset;
    modelBounds.yMin = float(y1) * -xyScale + yOffset;
    modelBounds.xMax = float(x1) * xyScale + xOffset;
    modelBounds.yMax = float(y0) * -xyScale + yOffset;
    int     w = landData->getImageWidth();
    int     h = landData->getImageHeight();
    x0 = (x0 > 0 ? (x0 < (w - 1) ? x0 : (w - 1)) : 0);
    x1 = (x1 > 0 ? (x1 < (w - 1) ? x1 : (w - 1)) : 0);
    unsigned short  z0 = 0xFFFF;
    unsigned short  z1 = 0;
    for (int y = y0; y <= y1; y++)
    {
      int     yc = (y > 0 ? (y < (h - 1) ? y : (h - 1)) : 0);
      const unsigned short  *srcPtr = landData->getHeightMap();
      srcPtr = srcPtr + (size_t(yc) * size_t(w));
      for (int x = x0; x <= x1; x++)
      {
        unsigned short  z = srcPtr[x];
        z0 = (z < z0 ? z : z0);
        z1 = (z > z1 ? z : z1);
      }
    }
    modelBounds.zMin = float(int(z0)) * zScale + zOffset;
    modelBounds.zMax = float(int(z1)) * zScale + zOffset;
    vt = viewTransform;
  }
  else if (p.flags & 4)
  {
    int     x0 = int(p.model.t.x0 < p.model.t.x1 ? p.model.t.x0 : p.model.t.x1);
    int     x1 = x0 + std::abs(int(p.model.t.x1) - int(p.model.t.x0));
    int     y0 = int(p.model.t.y0 < p.model.t.y1 ? p.model.t.y0 : p.model.t.y1);
    int     y1 = y0 + std::abs(int(p.model.t.y1) - int(p.model.t.y0));
    modelBounds.xMin = float(x0);
    modelBounds.yMin = float(y0);
    modelBounds.zMin = 0.0f;
    modelBounds.xMax = float(x1);
    modelBounds.yMax = float(y1);
    modelBounds.zMax = 0.0f;
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
      zMin >= zRangeMax || zMax < 0)
  {
    return -1;
  }
  p.tileIndex = calculateTileIndex(calculateTileMask(xMin, yMin, xMax, yMax));
  p.z = roundFloat(screenBounds.zMin * 64.0f);
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
  if (debugMode == 1)
    n = n >> 1;
  else if (n > 24)
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
      tmp.model.t.x0 = (signed short) (x > 0 ? x : 0);
      tmp.model.t.y0 = (signed short) (y > 0 ? y : 0);
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
  {
    if (cellFlags & 0x01)
      return;
    waterLevel = defaultWaterLevel;
  }
  RenderObject  tmp;
  tmp.flags = 0x0004;                   // water cell
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
    objectList.push_back(tmp);
  }
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
    tmp.flags = 0x0000;
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
      switch (r.type)
      {
        case 0x49544341:                // "ACTI"
          // ignore markers
          if (r.flags & 0x00800000U)
            continue;
          break;
        case 0x4E525546:                // "FURN"
        case 0x4C4F4353:                // "SCOL"
        case 0x54415453:                // "STAT"
        case 0x45455254:                // "TREE"
          // ignore markers and (optionally) objects not visible from distance
          if ((r.flags | (!distantObjectsOnly ? 0xFF7FFFFFU : 0xFF7F7FFFU))
              != 0xFF7FFFFFU)
          {
            continue;
          }
          break;
        case 0x5454534D:                // "MSTT"
        case 0x54415750:                // "PWAT"
          break;
        default:
          if (!enableAllObjects ||
              (r.flags | (!distantObjectsOnly ? 0xFF7FFFFFU : 0xFF7F7FFFU))
              != 0xFF7FFFFFU)
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
        else if (f == "MODL" && f.size() > 1 &&
                 (!modelLOD || stringBuf.empty()))
        {
          f.readPath(stringBuf, std::string::npos, "meshes/", ".nif");
          isHDModel = isHighQualityModel(stringBuf);
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
          tmp.flags = (unsigned short) (((roundFloat(f.readFloat() * 255.0f)
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
      if (!isWater && isExcludedModel(stringBuf))
        continue;
      tmp.type = (unsigned short) (r.type & 0xFFFFU);
      tmp.flags = tmp.flags | (unsigned short) ((!isWater ? 0x02 : 0x06)
                                                | (!isHDModel ? 0x00 : 0x40));
      tmp.modelPath = stringBuf;
    }
    while (false);
    i = baseObjects.insert(std::pair< unsigned int, BaseObject >(
                               r.formID, tmp)).first;
  }
  if (!(i->second.flags & 7))
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
  unsigned int  mswpFormIDBase = 0U;
  unsigned int  modelMSWPFormID = 0U;
  const BaseObject  *o = (BaseObject *) 0;
  while (f.next())
  {
    if (f == "MODS" && f.size() >= 4)
    {
      mswpFormIDBase = f.readUInt32Fast();
    }
    else if (f == "ONAM" && f.size() >= 4)
    {
      o = (BaseObject *) 0;
      unsigned int  modelFormID = f.readUInt32Fast();
      if (!modelFormID)
        continue;
      modelMSWPFormID = refrMSWPFormID;
      if (!modelMSWPFormID && f.size() >= 8)
        modelMSWPFormID = f.readUInt32Fast();
      const ESMFile::ESMRecord  *r3 = esmFile.getRecordPtr(modelFormID);
      if (!r3 || (r3->flags | (!distantObjectsOnly ? 0xFF7FFFFFU : 0xFF7F7FFFU))
                 != 0xFF7FFFFFU)
      {
        // ignore markers and (optionally) objects not visible from distance
        continue;
      }
      if (!(o = readModelProperties(tmp, *r3)) || (tmp.flags & 4))
      {
        o = (BaseObject *) 0;
        continue;
      }
      if (!modelMSWPFormID)
        modelMSWPFormID = tmp.mswpFormID;
      if (!modelMSWPFormID)
        modelMSWPFormID = mswpFormIDBase;
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
          if (!tmp.mswpFormID && modelMSWPFormID)
            tmp.mswpFormID = loadMaterialSwap(modelMSWPFormID);
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
    if (BRANCH_EXPECT(!r, false))
      break;
    if (BRANCH_EXPECT(!(*r == "REFR"), false))
    {
      if (*r == "CELL")
      {
        if (type == 0)
          addTerrainCell(*r);
        else if (type == 2)
          addWaterCell(*r);
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
        else if (f == "XMSP" && f.size() >= 4)
        {
          refrMSWPFormID = f.readUInt32Fast();
        }
      }
    }
    if (!r2)
      continue;
    if (*r2 == "SCOL" && !enableSCOL)
    {
      if (type == 1)
      {
        addSCOLObjects(*r2, scale, rX, rY, rZ, offsX, offsY, offsZ,
                       refrMSWPFormID);
      }
      continue;
    }
    RenderObject  tmp;
    tmp.tileIndex = -1;
    tmp.z = 0;
    const BaseObject  *o = readModelProperties(tmp, *r2);
    if (!o || (type & 2) != int((tmp.flags >> 1) & 2))
      continue;
    tmp.modelTransform = NIFFile::NIFVertexTransform(scale, rX, rY, rZ,
                                                     offsX, offsY, offsZ);
    if (setScreenAreaUsed(tmp) < 0)
      continue;
    if (debugMode == 1)
      tmp.z = int(r->formID);
    tmp.mswpFormID =
        loadMaterialSwap(!refrMSWPFormID ? tmp.mswpFormID : refrMSWPFormID);
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
            if (type == 0)
            {
              unsigned int  cellBlockID = r2->children;
              while (cellBlockID)
              {
                const ESMFile::ESMRecord  *r3 =
                    esmFile.getRecordPtr(cellBlockID);
                if (!r3)
                  break;
                if (*r3 == "GRUP" && r3->formID == 4 && r3->children)
                {
                  // exterior cell block
                  findObjects(r3->children, type, true);
                }
                cellBlockID = r3->next;
              }
            }
            else
            {
              findObjects(r2->children, type, true);
            }
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
  std::map< std::string, unsigned int > modelPathsUsed;
  for (size_t i = 0; i < objectList.size(); i++)
  {
    if (objectList[i].flags & 0x02)
      modelPathsUsed[objectList[i].model.o->modelPath] = 0U;
  }
  unsigned int  n = 0U;
  for (std::map< std::string, unsigned int >::iterator
           i = modelPathsUsed.begin(); i != modelPathsUsed.end(); i++, n++)
  {
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
        outBufRGBW[i] = 0U;
    }
    if (flags & 0x02)
    {
      float   tmp = float(zRangeMax);
      for (size_t i = 0; i < imageDataSize; i++)
        outBufZ[i] = tmp;
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
    landTexturesN.clear();
  }
  if (flags & 0x08)
    objectList.clear();
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

const DDSTexture * Renderer::loadTexture(const std::string& fileName, int m)
{
  if (fileName.find("/temp_ground") != std::string::npos)
    return (DDSTexture *) 0;
  textureCacheMutex.lock();
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
      if (m < 0)
        m = (fileName.find("/cubemaps/") == std::string::npos ? textureMip : 0);
      m = ba2File.extractTexture(fileBuf, fileName, m);
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
      textureCacheMutex.unlock();
      if (t)
        delete t;
      return (DDSTexture *) 0;
    }
    catch (...)
    {
      textureCacheMutex.unlock();
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
  const DDSTexture  *t = i->second.texture;
  textureCacheMutex.unlock();
  return t;
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
    try
    {
      ba2File.extractFile(nifFiles[t].fileBuf, o.modelPath);
      nifFiles[n].nifFile = new NIFFile(&(nifFiles[t].fileBuf.front()),
                                        nifFiles[t].fileBuf.size(), &ba2File);
      bool    isHDModel = bool(o.flags & 0x0040);
      nifFiles[n].nifFile->getMesh(nifFiles[n].meshData, 0U,
                                   (unsigned int) (modelLOD > 0 && !isHDModel),
                                   true);
      size_t  meshCnt = nifFiles[n].meshData.size();
      for (size_t i = 0; i < meshCnt; i++)
      {
        const NIFFile::NIFTriShape& ts = nifFiles[n].meshData[i];
        if (ts.flags & 0x05)            // ignore if hidden or effect
          continue;
        nifFiles[n].totalTriangleCnt += size_t(ts.triangleCnt);
      }
    }
    catch (std::runtime_error&)
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

unsigned int Renderer::loadMaterialSwap(unsigned int formID)
{
  if (!formID)
    return 0U;
  {
    std::map< unsigned int, std::vector< MaterialSwap > >::const_iterator i =
        materialSwaps.find(formID);
    if (i != materialSwaps.end())
      return (i->second.size() > 0 ? formID : 0U);
  }
  std::vector< MaterialSwap > *v;
  {
    std::map< unsigned int, std::vector< MaterialSwap > >::iterator i =
        materialSwaps.insert(
            std::pair< unsigned int, std::vector< MaterialSwap > >(
                formID, std::vector< MaterialSwap >())).first;
    v = &(i->second);
  }
  const ESMFile::ESMRecord  *r = esmFile.getRecordPtr(formID);
  if (!(r && *r == "MSWP"))
    return 0U;
  MaterialSwap  m;
  ESMFile::ESMField f(esmFile, *r);
  while (f.next())
  {
    if (f == "BNAM")
    {
      f.readPath(stringBuf, std::string::npos, "materials/", ".bgsm");
      m.materialPath = stringBuf;
    }
    else if (f == "SNAM" && !m.materialPath.empty())
    {
      f.readPath(stringBuf, std::string::npos, "materials/", ".bgsm");
      if (!stringBuf.empty())
      {
        static const char *bgsmNamePatterns[14] =
        {
          "*",            "",             "01",           "01decal",
          "02",           "_2sided",      "_8bit",        "alpha",
          "alpha_2sided", "_decal",       "decal",        "decal_8bit",
          "noalpha",      "wet"
        };
        size_t  n1 = m.materialPath.find('*');
        size_t  n2 = std::string::npos;
        if (n1 != std::string::npos)
          n2 = stringBuf.find('*');
        for (size_t i = 1; i < (sizeof(bgsmNamePatterns) / sizeof(char *)); i++)
        {
          if (n1 != std::string::npos)
          {
            m.materialPath.erase(n1, std::strlen(bgsmNamePatterns[i - 1]));
            m.materialPath.insert(n1, bgsmNamePatterns[i]);
            if (n2 != std::string::npos)
            {
              stringBuf.erase(n2, std::strlen(bgsmNamePatterns[i - 1]));
              stringBuf.insert(n2, bgsmNamePatterns[i]);
            }
            if (ba2File.getFileSize(m.materialPath, true) < 0L)
              continue;
          }
          try
          {
            ba2File.extractFile(fileBuf, stringBuf);
            FileBuffer  tmp(&(fileBuf.front()), fileBuf.size());
            m.bgsmFile.loadBGSMFile(m.texturePaths, tmp);
            v->push_back(m);
          }
          catch (std::runtime_error&)
          {
          }
          if (n1 == std::string::npos)
            break;
        }
      }
      m.materialPath.clear();
    }
  }
  if (v->size() > 0)
    return formID;
  return 0U;
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
  unsigned int  modelIDBase = 0xFFFFFFFFU;
  for (size_t i = 0; i < objectList.size(); )
  {
    for (size_t k = 0; k < 64; k++)
    {
      threadTriangleCnt[k] = 0;
      threadTileMask[k] = 1ULL << (unsigned int) k;
    }
    const RenderObject& o = objectList[i];
    unsigned int  modelIDMask = modelBatchCnt - 1U;
    if ((o.flags & 0x02) && (o.model.o->modelID & ~modelIDMask) != modelIDBase)
    {
      // load new set of models
      modelIDBase = o.model.o->modelID & ~modelIDMask;
      for (unsigned int n = 0; n < modelBatchCnt; n++)
        nifFiles[n].clear();
      unsigned long long  m = 0ULL;
      for (size_t j = i; j < objectList.size(); j++)
      {
        const RenderObject& p = objectList[j];
        if ((o.flags ^ p.flags) & 7)
          break;
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
          throw errorMessage("%s", nifFiles[k].threadErrMsg.c_str());
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
      if (((o.tileIndex ^ p.tileIndex) & ~63) || ((o.flags ^ p.flags) & 7))
        break;
      if ((o.flags & 2) &&
          ((o.model.o->modelID ^ p.model.o->modelID) & ~modelIDMask))
      {
        break;
      }
      size_t  triangleCnt = 2;
      if (p.flags & 0x02)
      {
        triangleCnt =
            nifFiles[p.model.o->modelID & modelIDMask].totalTriangleCnt;
      }
      else if ((p.flags & 0x01) && landData)
      {
        triangleCnt = size_t(landData->getCellResolution());
        if (triangleCnt > 24)
          triangleCnt = triangleCnt - 8;
        triangleCnt = triangleCnt * triangleCnt * 2U;
      }
      threadTriangleCnt[p.tileIndex & 63] += triangleCnt;
      j++;
    }
    size_t  threadsUsed = 64;
    while (threadsUsed > 1)
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
      if (int(threadsUsed) <= threadCnt && minCnt1 > 0)
        break;
      threadTriangleCnt[minCntThread1] += threadTriangleCnt[minCntThread2];
      threadTileMask[minCntThread1] |= threadTileMask[minCntThread2];
      threadsUsed--;
      threadTriangleCnt[minCntThread2] = threadTriangleCnt[threadsUsed];
      threadTileMask[minCntThread2] = threadTileMask[threadsUsed];
    }
    for (size_t k = 1; k < threadsUsed; k++)
    {
      renderThreads[k].t = new std::thread(threadFunction, this, k, i, j,
                                           threadTileMask[k]);
    }
    if (threadTriangleCnt[0] > 0)
      renderThread(0, i, j, threadTileMask[0]);
    for (size_t k = 1; k < threadsUsed; k++)
      renderThreads[k].join();
    for (size_t k = 1; k < threadsUsed; k++)
    {
      if (!renderThreads[k].errMsg.empty())
        throw errorMessage("%s", renderThreads[k].errMsg.c_str());
    }
    // render any remaining objects that were skipped due to incorrect bounds
    for (size_t k = 0; k < threadsUsed; k++)
    {
      for (size_t l = 0; l < renderThreads[k].objectsRemaining.size(); l++)
        renderObject(renderThreads[k], renderThreads[k].objectsRemaining[l]);
      renderThreads[k].objectsRemaining.clear();
    }
    i = j;
    shrinkTextureCache();
    if (verboseMode)
    {
      std::fprintf(stderr, "\r    %7u / %7u  ",
                   (unsigned int) i, (unsigned int) objectList.size());
    }
  }
  clear(0x20);
  if (verboseMode)
    std::fputc('\n', stderr);
}

void Renderer::materialSwap(
    Plot3D_TriShape& t, const std::string **texturePaths, size_t texturePathCnt,
    unsigned int formID)
{
  std::map< unsigned int, std::vector< MaterialSwap > >::const_iterator i =
      materialSwaps.find(formID);
  if (i == materialSwaps.end() || !t.materialPath)
    return;
  for (size_t j = 0; j < i->second.size(); j++)
  {
    if (!(*(t.materialPath) == i->second[j].materialPath))
      continue;
    const BGSMFile& bgsmFile = i->second[j].bgsmFile;
    // decal, two sided, tree
    t.flags = (t.flags & 0x07) | ((bgsmFile.flags & 0x1C) << 1);
    t.flags = t.flags | ((t.flags & 0x02) << 3);
    t.gradientMapV = bgsmFile.gradientMapV;
    t.envMapScale = bgsmFile.envMapScale;
    t.alphaThreshold = bgsmFile.calculateAlphaThreshold();
    t.textureOffsetU = bgsmFile.offsetU;
    t.textureOffsetV = bgsmFile.offsetV;
    t.textureScaleU = bgsmFile.scaleU;
    t.textureScaleV = bgsmFile.scaleV;
    for (size_t k = 0; k < texturePathCnt; k++)
    {
      if (k < i->second[j].texturePaths.size())
        texturePaths[k] = &(i->second[j].texturePaths.front()) + k;
      else
        texturePaths[k] = (std::string *) 0;
    }
    break;
  }
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
    for (size_t j = 0; j < nifFiles[n].meshData.size(); j++)
    {
      if (nifFiles[n].meshData[j].flags & 0x05) // hidden or effect
        continue;
      if (!nifFiles[n].meshData[j].triangleCnt)
        continue;
      NIFFile::NIFBounds  b;
      nifFiles[n].meshData[j].calculateBounds(b, &vt);
      unsigned long long  m =
          calculateTileMask(roundFloat(b.xMin), roundFloat(b.yMin),
                            roundFloat(b.xMax), roundFloat(b.yMax));
      if (!m)
        continue;
      if (m & ~tileMask)
        return false;
      *(t.renderer) = nifFiles[n].meshData[j];
      if (p.flags & 0x80)
        t.renderer->gradientMapV = (unsigned char) (p.flags >> 8);
      const std::string *texturePaths[10];
      const DDSTexture  *textures[10];
      for (size_t k = 0; k < 10; k++)
      {
        texturePaths[k] = (std::string *) 0;
        if (k < t.renderer->texturePathCnt)
          texturePaths[k] = t.renderer->texturePaths[k];
        textures[k] = (DDSTexture *) 0;
      }
      if (t.renderer->flags & 0x02)
      {
        if (!defaultWaterTexture.empty())
          textures[1] = loadTexture(defaultWaterTexture);
      }
      else
      {
        if (p.mswpFormID)
          materialSwap(*(t.renderer), texturePaths, 10, p.mswpFormID);
        if (BRANCH_EXPECT(!enableTextures, false))
        {
          if (t.renderer->alphaThreshold ||
              (texturePaths[3] && !texturePaths[3]->empty()))
          {
            texturePaths[3] = &whiteTexturePath;
          }
          else
          {
            texturePaths[0] = &whiteTexturePath;
          }
        }
        bool    isHDModel = bool(p.flags & 0x40);
        unsigned int  txtSetMask = (!isHDModel ? 0x0009U : 0x015BU);
        for (size_t k = size_t(!isHDModel ? 4 : 10); k-- > 0; )
        {
          if (!((1U << (unsigned char) k) & txtSetMask))
            continue;
          const std::string *texturePath = (std::string *) 0;
          if (texturePaths[k] && !texturePaths[k]->empty())
            texturePath = texturePaths[k];
          else if (k == 4 && (textures[6] || textures[8]))
            texturePath = &defaultEnvMap;
          if (texturePath && !texturePath->empty())
            textures[k] = loadTexture(*texturePath);
        }
      }
      t.renderer->drawTriShape(
          p.modelTransform, viewTransform, lightX, lightY, lightZ,
          textures, 10);
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
      const DDSTexture * const  *ltxtN = (DDSTexture **) 0;
      if (landTexturesN.size() >= landTextures.size())
        ltxtN = &(landTexturesN.front());
      t.terrainMesh->createMesh(
          *landData, landTxtScale,
          p.model.t.x0, p.model.t.y0, p.model.t.x1, p.model.t.y1,
          &(landTextures.front()), ltxtN, landTextures.size(),
          landTextureMip - float(int(landTextureMip)),
          landTxtRGBScale, landTxtDefColor);
      *(t.renderer) = *(t.terrainMesh);
      t.renderer->drawTriShape(
          p.modelTransform, viewTransform, lightX, lightY, lightZ,
          t.terrainMesh->getTextures(), t.terrainMesh->getTextureCount());
    }
  }
  else if (p.flags & 0x04)              // water cell
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
      vTmp[j].z = 0.0f;
      vTmp[j].normalX = 0.0f;
      vTmp[j].normalY = 0.0f;
      vTmp[j].normalZ = 1.0f;
      vTmp[j].u = (unsigned short) (j == 0 || j == 3 ? 0x0000 : 0x4000);
      vTmp[j].v = (unsigned short) (j == 0 || j == 1 ? 0x0000 : 0x4000);
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
    tmp.flags = 0x12;                   // water
    tmp.texturePathCnt = 0;
    tmp.texturePaths = (std::string **) 0;
    const DDSTexture  *textures[2];
    textures[0] = (DDSTexture *) 0;
    textures[1] = (DDSTexture *) 0;
    if (!defaultWaterTexture.empty())
      textures[1] = loadTexture(defaultWaterTexture);
    *(t.renderer) = tmp;
    t.renderer->drawTriShape(
        p.modelTransform, viewTransform, lightX, lightY, lightZ, textures, 2);
  }
  return true;
}

void Renderer::renderThread(size_t threadNum, size_t startPos, size_t endPos,
                            unsigned long long tileIndexMask)
{
  RenderThread& t = renderThreads[threadNum];
  if (!t.renderer)
    return;
  t.renderer->setLightingFunction(lightingPolynomial);
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
                   unsigned int *bufRGBW, float *bufZ, int zMax)
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
    defaultWaterLevel(0.0f),
    modelLOD(0),
    distantObjectsOnly(false),
    noDisabledObjects(false),
    enableSCOL(false),
    enableAllObjects(false),
    enableTextures(true),
    debugMode(0),
    bufRGBWAllocFlag(!bufRGBW),
    bufZAllocFlag(!bufZ),
    threadCnt(0),
    textureDataSize(0),
    textureCacheSize(0x40000000),
    firstTexture((CachedTexture *) 0),
    lastTexture((CachedTexture *) 0),
    waterColor(0xC0302010U),
    zRangeMax(zMax),
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
  if (esmFile.getESMVersion() < 0x80U)
    whiteTexturePath = "textures/white.dds";
  else
    whiteTexturePath = "textures/effects/rainscene/test_flat_white.dds";
}

Renderer::~Renderer()
{
  clear(0x7C);
  if (bufZAllocFlag && outBufZ)
    delete[] outBufZ;
  if (bufRGBWAllocFlag && outBufRGBW)
    delete[] outBufRGBW;
}

unsigned int Renderer::findParentWorld(ESMFile& esmFile, unsigned int formID)
{
  if (!formID)
    return 0xFFFFFFFFU;
  const ESMFile::ESMRecord  *r = esmFile.getRecordPtr(formID);
  if (!(r && (*r == "WRLD" || *r == "GRUP" || *r == "CELL" || *r == "REFR")))
    return 0xFFFFFFFFU;
  if (*r == "WRLD")
    return formID;
  while (!(*r == "GRUP" && r->formID == 1U))
  {
    if (!r->parent)
      return 0U;
    r = esmFile.getRecordPtr(r->parent);
    if (!r)
      return 0U;
  }
  return r->flags;
}

void Renderer::clear()
{
  clear(0x7C);
  baseObjects.clear();
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
      renderThreads[i].renderer =
          new Plot3D_TriShape(outBufRGBW, outBufZ, width, height,
                              (esmFile.getESMVersion() < 0xC0 ? 1.0f : 2.2f));
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
  clear(0x44);
  if (verboseMode)
    std::fprintf(stderr, "Loading terrain data\n");
  landData = new LandscapeData(&esmFile, btdFileName, &ba2File, 0x0B, worldID,
                               defTxtID, mipLevel, xMin, yMin, xMax, yMax);
  defaultWaterLevel = landData->getWaterLevel();
  if (verboseMode)
    std::fprintf(stderr, "Loading landscape textures\n");
  landTextures.resize(landData->getTextureCount(), (DDSTexture *) 0);
  if (cellTextureResolution > landData->getCellResolution())
    landTexturesN.resize(landData->getTextureCount(), (DDSTexture *) 0);
  int     mipLevelD = textureMip + int(landTextureMip);
  for (size_t i = 0; i < landTextures.size(); i++)
  {
    if (!enableTextures)
      landTextures[i] = loadTexture(whiteTexturePath);
    else if (!landData->getTextureDiffuse(i).empty())
      landTextures[i] = loadTexture(landData->getTextureDiffuse(i), mipLevelD);
    if (i < landTexturesN.size())
    {
      long    fileSizeD = ba2File.getFileSize(landData->getTextureDiffuse(i));
      long    fileSizeN = ba2File.getFileSize(landData->getTextureNormal(i));
      int     mipLevelN = mipLevelD + calculateLandTxtMip(fileSizeN)
                          - calculateLandTxtMip(fileSizeD);
      mipLevelN = (mipLevelN > 0 ? (mipLevelN < 15 ? mipLevelN : 15) : 0);
      landTexturesN[i] = loadTexture(landData->getTextureNormal(i), mipLevelN);
    }
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
  baseObjects.clear();
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
  if (renderThreads[0].renderer)
  {
    const DDSTexture  *envMap = (DDSTexture *) 0;
    if (!defaultEnvMap.empty())
      envMap = loadTexture(defaultEnvMap);
    renderThreads[0].renderer->renderWater(
        viewTransform, waterColor, lightX, lightY, lightZ, envMap, 0.375f);
  }
}

static const char *usageStrings[] =
{
  "Usage: render INFILE.ESM[,...] OUTFILE.DDS W H ARCHIVEPATH [OPTIONS...]",
  "",
  "Options:",
  "    --help              print usage",
  "    --list-defaults     print defaults for all options, and exit",
  "    --                  remaining options are file names",
  "    -threads INT        set the number of threads to use",
  "    -debug INT          set debug render mode (0: disabled, 1: form IDs,",
  "                        2: depth, 3: normals, 4: diffuse, 5: light only)",
  "    -scol BOOL          enable the use of pre-combined meshes",
  "    -a                  render all object types",
  "    -textures BOOL      make all diffuse textures white if false",
  "    -txtcache INT       texture cache size in megabytes",
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
  "    -zrange MIN MAX     limit view Z range to MIN <= Z < MAX",
  "    -light SCALE RX RY  set light level and X, Y rotation (0, 0 = top)",
  "    -lpoly A5 A4 A3 A2 A1 A0    set lighting polynomial, -1 <= x <= 1",
  "",
  "    -mlod INT           set level of detail for models, 0 (best) to 4",
  "    -vis BOOL           render only objects visible from distance",
  "    -ndis BOOL          do not render initially disabled objects",
  "    -hqm STRING         add high quality model path name pattern",
  "    -xm STRING          add excluded model path name pattern",
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
    int     textureCacheSize = 1024;
    bool    verboseMode = true;
    bool    distantObjectsOnly = false;
    bool    noDisabledObjects = true;
    bool    enableDownscale = false;
    bool    enableSCOL = false;
    bool    enableAllObjects = false;
    bool    enableTextures = true;
    unsigned char debugMode = 0;
    unsigned int  formID = 0U;
    int     btdLOD = 0;
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
    int     zMin = 0;
    int     zMax = 16777216;
    float   viewScale = 0.0625f;
    float   viewRotationX = 180.0f;
    float   viewRotationY = 0.0f;
    float   viewRotationZ = 0.0f;
    float   viewOffsX = 0.0f;
    float   viewOffsY = 0.0f;
    float   viewOffsZ = 32768.0f;
    float   lightLevel = 1.0f;
    float   lightRotationX = 63.435f;
    float   lightRotationY = 41.8103f;
    float   lightingPolynomial[6];
    const char  *defaultEnvMap = (char *) 0;
    const char  *waterTexture = (char *) 0;
    std::vector< const char * > hdModelNamePatterns;
    std::vector< const char * > excludeModelPatterns;
    Plot3D_TriShape::getDefaultLightingFunction(lightingPolynomial);
    float   d = float(std::atan(1.0) / 45.0);   // degrees to radians

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
      else if (std::strcmp(argv[i], "--list-defaults") == 0)
      {
        std::printf("-threads %d", threadCnt);
        if (threadCnt <= 0)
        {
          std::printf(" (defaults to hardware threads: %d)",
                      int(std::thread::hardware_concurrency()));
        }
        std::printf("\n-debug %d\n", int(debugMode));
        std::printf("-scol %d\n", int(enableSCOL));
        std::printf("-textures %d\n", int(enableTextures));
        std::printf("-txtcache %d\n", textureCacheSize);
        std::printf("-ssaa %d\n", int(enableDownscale));
        std::printf("-w 0x%08X", formID);
        if (!formID)
          std::printf(" (defaults to 0x0000003C or 0x0025DA15)");
        std::printf("\n-r %d %d %d %d\n",
                    terrainX0, terrainY0, terrainX1, terrainY1);
        std::printf("-l %d\n", btdLOD);
        std::printf("-deftxt 0x%08X\n", defTxtID);
        std::printf("-defclr 0x%08X\n", ltxtDefColor);
        std::printf("-ltxtres %d\n", ltxtResolution);
        std::printf("-mip %d\n", textureMip);
        std::printf("-lmip %.1f\n", landTextureMip);
        std::printf("-lmult %.1f\n", landTextureMult);
        std::printf("-view %.6f %.1f %.1f %.1f %.1f %.1f %.1f\n",
                    viewScale, viewRotationX, viewRotationY, viewRotationZ,
                    viewOffsX, viewOffsY, viewOffsZ);
        {
          NIFFile::NIFVertexTransform
              vt(1.0f, viewRotationX * d, viewRotationY * d, viewRotationZ * d,
                 0.0f, 0.0f, 0.0f);
          std::printf("    Rotation matrix:\n");
          std::printf("        X: %9.6f %9.6f %9.6f\n",
                      vt.rotateXX, vt.rotateXY, vt.rotateXZ);
          std::printf("        Y: %9.6f %9.6f %9.6f\n",
                      vt.rotateYX, vt.rotateYY, vt.rotateYZ);
          std::printf("        Z: %9.6f %9.6f %9.6f\n",
                      vt.rotateZX, vt.rotateZY, vt.rotateZZ);
        }
        std::printf("-zrange %d %d\n", zMin, zMax);
        std::printf("-light %.1f %.4f %.4f\n",
                    lightLevel, lightRotationX, lightRotationY);
        {
          NIFFile::NIFVertexTransform
              vt(1.0f, lightRotationX * d, lightRotationY * d, 0.0f,
                 0.0f, 0.0f, 0.0f);
          std::printf("    Light vector: %9.6f %9.6f %9.6f\n",
                      vt.rotateXZ, vt.rotateYZ, vt.rotateZZ);
        }
        std::printf("-lpoly %.6f %.6f %.6f %.6f %.6f %.6f\n",
                    lightingPolynomial[5], lightingPolynomial[4],
                    lightingPolynomial[3], lightingPolynomial[2],
                    lightingPolynomial[1], lightingPolynomial[0]);
        std::printf("-mlod %d\n", modelLOD);
        std::printf("-vis %d\n", int(distantObjectsOnly));
        std::printf("-ndis %d\n", int(noDisabledObjects));
        std::printf("-watercolor 0x%08X\n", waterColor);
        return 0;
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
      else if (std::strcmp(argv[i], "-debug") == 0)
      {
        if (++i >= argc)
          throw errorMessage("missing argument for %s", argv[i - 1]);
        debugMode = (unsigned char) parseInteger(argv[i], 10,
                                                 "invalid debug mode", 0, 5);
      }
      else if (std::strcmp(argv[i], "-scol") == 0)
      {
        if (++i >= argc)
          throw errorMessage("missing argument for %s", argv[i - 1]);
        enableSCOL =
            bool(parseInteger(argv[i], 0, "invalid argument for -scol", 0, 1));
      }
      else if (std::strcmp(argv[i], "-a") == 0)
      {
        enableAllObjects = true;
      }
      else if (std::strcmp(argv[i], "-textures") == 0)
      {
        if (++i >= argc)
          throw errorMessage("missing argument for %s", argv[i - 1]);
        enableTextures =
            bool(parseInteger(argv[i], 0, "invalid argument for -textures",
                              0, 1));
      }
      else if (std::strcmp(argv[i], "-txtcache") == 0)
      {
        if (++i >= argc)
          throw errorMessage("missing argument for %s", argv[i - 1]);
        textureCacheSize =
            int(parseInteger(argv[i], 0, "invalid texture cache size",
                             256, 4095));
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
      else if (std::strcmp(argv[i], "-zrange") == 0)
      {
        if ((i + 2) >= argc)
          throw errorMessage("missing argument for %s", argv[i]);
        zMin = int(parseInteger(argv[i + 1], 10, "invalid Z range",
                                0, 1048575));
        zMax = int(parseInteger(argv[i + 2], 10, "invalid Z range",
                                zMin + 1, 16777216));
        i = i + 2;
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
      else if (std::strcmp(argv[i], "-xm") == 0)
      {
        if (++i >= argc)
          throw errorMessage("missing argument for %s", argv[i - 1]);
        excludeModelPatterns.push_back(argv[i]);
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
    if (!(btdPath && *btdPath != '\0'))
    {
      btdPath = (char *) 0;
      btdLOD = 2;
    }
    if (debugMode == 1)
    {
      enableDownscale = false;
      enableSCOL = true;
      ltxtResolution = 128 >> btdLOD;
    }
    if (!formID)
      formID = (!btdPath ? 0x0000003CU : 0x0025DA15U);
    for (int i = 0; i < 6; i++)
      lightingPolynomial[i] = lightingPolynomial[i] * lightLevel;
    waterColor = (waterColor & 0x00FFFFFFU) | ((waterColor & 0x7F000000U) << 1);
    if (!(waterColor & 0xFF000000U))
      waterColor = 0U;
    int     width =
        int(parseInteger(args[2], 0, "invalid image width", 2, 32768));
    int     height =
        int(parseInteger(args[3], 0, "invalid image height", 2, 32768));
    viewOffsX = viewOffsX + (float(width) * 0.5f);
    viewOffsY = viewOffsY + (float(height - 2) * 0.5f);
    viewOffsZ = viewOffsZ - float(zMin);
    zMax = zMax - zMin;
    if (enableDownscale)
    {
      width = width << 1;
      height = height << 1;
      viewScale = viewScale * 2.0f;
      viewOffsX = viewOffsX * 2.0f;
      viewOffsY = viewOffsY * 2.0f;
      viewOffsZ = viewOffsZ * 2.0f;
      zMax = zMax << 1;
      zMax = (zMax < 16777216 ? zMax : 16777216);
    }

    BA2File ba2File(args[4]);
    ESMFile esmFile(args[0]);
    unsigned int  worldID = Renderer::findParentWorld(esmFile, formID);
    if (worldID == 0xFFFFFFFFU)
      throw errorMessage("form ID not found in ESM, or invalid record type");
    Renderer  renderer(width, height, ba2File, esmFile,
                       (unsigned int *) 0, (float *) 0, zMax);
    if (threadCnt > 0)
      renderer.setThreadCount(threadCnt);
    renderer.setTextureCacheSize(size_t(textureCacheSize) << 20);
    renderer.setVerboseMode(verboseMode);
    renderer.setDistantObjectsOnly(distantObjectsOnly);
    renderer.setNoDisabledObjects(noDisabledObjects);
    renderer.setEnableSCOL(enableSCOL);
    renderer.setEnableAllObjects(enableAllObjects);
    renderer.setEnableTextures(enableTextures);
    renderer.setDebugMode(debugMode);
    renderer.setLandDefaultColor(ltxtDefColor);
    renderer.setLandTxtResolution(ltxtResolution);
    renderer.setTextureMipLevel(textureMip);
    renderer.setLandTextureMip(landTextureMip);
    renderer.setLandTxtRGBScale(landTextureMult);
    renderer.setModelLOD(modelLOD);
    renderer.setWaterColor(waterColor);
    renderer.setViewTransform(
        viewScale, viewRotationX * d, viewRotationY * d, viewRotationZ * d,
        viewOffsX, viewOffsY, viewOffsZ);
    renderer.setLightDirection(lightRotationX * d, lightRotationY * d);
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
    for (size_t i = 0; i < excludeModelPatterns.size(); i++)
    {
      if (excludeModelPatterns[i] && excludeModelPatterns[i][0])
        renderer.addExcludeModelPattern(std::string(excludeModelPatterns[i]));
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

