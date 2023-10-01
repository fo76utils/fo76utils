
#include "common.hpp"
#include "render.hpp"
#include "fp32vec4.hpp"

#include <algorithm>
#include <bit>
#include <chrono>

bool Renderer::RenderObject::operator<(const RenderObject& r) const
{
  if ((flags ^ r.flags) & 0xE000U)
    return (flags < r.flags);
  if (flags & 0x12)
  {
    unsigned int  modelID1 = model.o.b->modelID;
    unsigned int  modelID2 = r.model.o.b->modelID;
    if ((modelID1 ^ modelID2) & ~0xFFU)
      return (modelID1 < modelID2);
  }
  if (z != r.z)
    return (z < r.z);
  return (formID < r.formID);
}

Renderer::ModelData::ModelData()
  : nifFile((NIFFile *) 0),
    o((BaseObject *) 0),
    usesAlphaBlending(false)
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
  o = (BaseObject *) 0;
  objectBounds = NIFFile::NIFBounds();
  meshData.clear();
  usesAlphaBlending = false;
}

inline Renderer::TileMask::TileMask()
{
#if ENABLE_X86_64_AVX
  const YMM_UInt64  tmp = { 0U, 0U, 0U, 0U };
  m = tmp;
#else
  m[0] = 0U;
  m[1] = 0U;
  m[2] = 0U;
  m[3] = 0U;
#endif
}

inline Renderer::TileMask::TileMask(unsigned int x0, unsigned int y0,
                                    unsigned int x1, unsigned int y1)
{
  unsigned int  xMask = (2U << x1) - (1U << x0);
  unsigned int  yMask = (2U << y1) - (1U << y0);
#if ENABLE_X86_64_AVX2
  const YMM_UInt16  maskTbl =
  {
    0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080,
    0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000, 0x8000
  };
  YMM_UInt64  tmp1, tmp2;
  XMM_UInt32  tmp3 = { xMask, 0U, 0U, 0U };
  XMM_UInt32  tmp4 = { yMask, 0U, 0U, 0U };
  __asm__ ("vpbroadcastw %1, %t0" : "=x" (tmp1) : "x" (tmp3));
  __asm__ ("vpbroadcastw %1, %t0" : "=x" (tmp2) : "x" (tmp4));
  __asm__ ("vpand %t1, %t0, %t0" : "+x" (tmp2) : "x" (maskTbl));
  __asm__ ("vpcmpeqw %t1, %t0, %t0" : "+x" (tmp2) : "x" (maskTbl));
  __asm__ ("vpblendvb %t1, %t2, %t1, %t0" : "=x" (m) : "x" (tmp2), "x" (tmp1));
#else
  std::uint64_t yMask0 = std::uint64_t(yMask & 0x0001U)
                         | (std::uint64_t(yMask & 0x0002U) << 15)
                         | (std::uint64_t(yMask & 0x0004U) << 30)
                         | (std::uint64_t(yMask & 0x0008U) << 45);
  std::uint64_t yMask1 = (std::uint64_t(yMask & 0x0010U) >> 4)
                         | (std::uint64_t(yMask & 0x0020U) << 11)
                         | (std::uint64_t(yMask & 0x0040U) << 26)
                         | (std::uint64_t(yMask & 0x0080U) << 41);
  std::uint64_t yMask2 = (std::uint64_t(yMask & 0x0100U) >> 8)
                         | (std::uint64_t(yMask & 0x0200U) << 7)
                         | (std::uint64_t(yMask & 0x0400U) << 22)
                         | (std::uint64_t(yMask & 0x0800U) << 37);
  std::uint64_t yMask3 = (std::uint64_t(yMask & 0x1000U) >> 12)
                         | (std::uint64_t(yMask & 0x2000U) << 3)
                         | (std::uint64_t(yMask & 0x4000U) << 18)
                         | (std::uint64_t(yMask & 0x8000U) << 33);
  m[0] = yMask0 * xMask;
  m[1] = yMask1 * xMask;
  m[2] = yMask2 * xMask;
  m[3] = yMask3 * xMask;
#endif
}

Renderer::TileMask::TileMask(
    const NIFFile::NIFBounds& b, const NIFFile::NIFVertexTransform& vt,
    int w, int h)
{
  FloatVector4  rX(vt.rotateXX, vt.rotateYX, vt.rotateZX, vt.rotateXY);
  FloatVector4  rY(vt.rotateXY, vt.rotateYY, vt.rotateZY, vt.rotateXZ);
  FloatVector4  rZ(vt.rotateXZ, vt.rotateYZ, vt.rotateZZ, vt.scale);
  rX *= vt.scale;
  rY *= vt.scale;
  rZ *= vt.scale;
  NIFFile::NIFBounds  screenBounds;
  for (int i = 0; i < 8; i++)
  {
    screenBounds += ((rX * (!(i & 1) ? b.xMin() : b.xMax()))
                     + (rY * (!(i & 2) ? b.yMin() : b.yMax()))
                     + (rZ * (!(i & 4) ? b.zMin() : b.zMax())));
  }
  FloatVector4  vtOffs(vt.offsX, vt.offsY, vt.offsZ, vt.rotateXX);
  screenBounds.boundsMin += vtOffs;
  screenBounds.boundsMax += vtOffs;
  if (screenBounds.boundsMin[0] > float(w) ||
      screenBounds.boundsMin[1] > float(h) ||
      screenBounds.boundsMin[2] > 16777216.0f ||
      screenBounds.boundsMax[0] < -1.0f || screenBounds.boundsMax[1] < -1.0f ||
      screenBounds.boundsMax[2] < 0.0f || w < 1 || h < 1)
  {
    *this = TileMask();
    return;
  }
  FloatVector4  tmp(screenBounds.xMin(), screenBounds.yMin(),
                    screenBounds.xMax(), screenBounds.yMax());
  tmp += FloatVector4(-1.25f, -1.25f, 1.25f, 1.25f);
  tmp /= FloatVector4(float(w), float(h), float(w), float(h));
  tmp *= 16.0f;
  tmp.maxValues(FloatVector4(0.0f));
  tmp.minValues(FloatVector4(15.5f));
  (void) new(this) TileMask((unsigned int) int(tmp[0]),
                            (unsigned int) int(tmp[1]),
                            (unsigned int) int(tmp[2]),
                            (unsigned int) int(tmp[3]));
}

inline bool Renderer::TileMask::overlapsWith(const TileMask& r) const
{
#if ENABLE_X86_64_AVX
  bool    nz;
  __asm__ ("vptest %t2, %t1" : "=@ccnz" (nz) : "x" (m), "xm" (r.m));
  return nz;
#else
  return bool((m[0] & r.m[0]) | (m[1] & r.m[1])
              | (m[2] & r.m[2]) | (m[3] & r.m[3]));
#endif
}

inline Renderer::TileMask& Renderer::TileMask::operator|=(const TileMask& r)
{
#if ENABLE_X86_64_AVX
  m = m | r.m;
#else
  m[0] = m[0] | r.m[0];
  m[1] = m[1] | r.m[1];
  m[2] = m[2] | r.m[2];
  m[3] = m[3] | r.m[3];
#endif
  return *this;
}

inline Renderer::TileMask::operator bool() const
{
#if ENABLE_X86_64_AVX
  bool    nz;
  __asm__ ("vptest %t1, %t1" : "=@ccnz" (nz) : "x" (m));
  return nz;
#else
  return bool(m[0] | m[1] | m[2] | m[3]);
#endif
}

inline bool Renderer::TileMask::operator==(const TileMask& r) const
{
#if ENABLE_X86_64_AVX
  YMM_UInt64  tmp = m ^ r.m;
  bool    z;
  __asm__ ("vptest %t1, %t1" : "=@ccz" (z) : "x" (tmp));
  return z;
#else
  return !((m[0] ^ r.m[0]) | (m[1] ^ r.m[1])
           | (m[2] ^ r.m[2]) | (m[3] ^ r.m[3]));
#endif
}

inline Renderer::RenderObjectQueue::ObjectList::ObjectList()
  : front((RenderObjectQueueObj *) 0),
    back((RenderObjectQueueObj *) 0)
{
}

inline void Renderer::RenderObjectQueue::ObjectList::push_front(
    RenderObjectQueueObj *o)
{
  o->prv = (RenderObjectQueueObj *) 0;
  o->nxt = front;
  if (!front)
    back = o;
  else
    front->prv = o;
  front = o;
}

inline void Renderer::RenderObjectQueue::ObjectList::push_back(
    RenderObjectQueueObj *o)
{
  o->prv = back;
  o->nxt = (RenderObjectQueueObj *) 0;
  if (!back)
    front = o;
  else
    back->nxt = o;
  back = o;
}

inline void Renderer::RenderObjectQueue::ObjectList::pop(
    RenderObjectQueueObj *o)
{
  RenderObjectQueueObj*&  prvNxt = (o != front ? o->prv->nxt : front);
  RenderObjectQueueObj*&  nxtPrv = (o != back ? o->nxt->prv : back);
  prvNxt = o->nxt;
  nxtPrv = o->prv;
}

inline Renderer::RenderObjectQueue::ObjectList::operator bool() const
{
  return bool(front);
}

Renderer::RenderObjectQueue::RenderObjectQueue(size_t bufSize)
  : buf(bufSize + maxThreadCnt),
    doneFlag(false),
    pauseFlag(false),
    loadingModelsFlag(false),
    allowReorder(false)
{
  clear();
}

Renderer::RenderObjectQueue::~RenderObjectQueue()
{
  clear();
}

void Renderer::RenderObjectQueue::clear()
{
  {
    std::unique_lock< std::mutex >  tmpLock(m);
    doneFlag = true;
    pauseFlag = false;
    loadingModelsFlag = false;
    while (objectsRendered)
      cv2.wait_for(tmpLock, std::chrono::milliseconds(20));
    queuedObjects = ObjectList();
    objectsReady = ObjectList();
    objectsRendered = ObjectList();
    freeObjects = ObjectList();
    for (std::vector< RenderObjectQueueObj >::iterator
             i = buf.begin(); i != buf.end(); i++)
    {
      i->o = (RenderObject *) 0;
      i->threadNum = std::intptr_t(-1);
      i->m = TileMask();
      freeObjects.push_back(&(*i));
    }
  }
  cv1.notify_all();
}

bool Renderer::RenderObjectQueue::queueObject(RenderObjectQueueObj *o)
{
  RenderObjectQueueObj  *p;
  bool    readyFlag;
  if (BRANCH_UNLIKELY(o->threadNum >= 256L))
  {
    unsigned int  n = 0xFFU;
    if (BRANCH_LIKELY(o->o->flags & 0x02))
      n = o->o->model.o.b->modelID & 0xFFU;
    readyFlag = true;
    for (int i = 0; i < 3 && readyFlag; i++)
    {
      p = (i == 0 ? objectsRendered.front
                    : (i == 1 ? objectsReady.front : queuedObjects.front));
      for ( ; p && readyFlag; p = p->nxt)
      {
        if ((p->o->flags & 0x02) && (p->o->model.o.b->modelID & 0xFFU) == n)
          readyFlag = false;
      }
    }
  }
  else
  {
    TileMask  tmpMask;
    for (p = objectsRendered.front; p; p = p->nxt)
      tmpMask |= p->m;
    for (p = objectsReady.front; p; p = p->nxt)
      tmpMask |= p->m;
    readyFlag = !tmpMask.overlapsWith(o->m);
    if (readyFlag && !(allowReorder && !(o->o->flags & 0x1C)))
    {
      tmpMask = o->m;
      for (p = queuedObjects.front; p; p = p->nxt)
      {
        if (tmpMask.overlapsWith(p->m))
        {
          readyFlag = false;
          break;
        }
      }
    }
  }
  if (readyFlag)
    objectsReady.push_back(o);
  else
    queuedObjects.push_back(o);
  return readyFlag;
}

void Renderer::RenderObjectQueue::findObjects()
{
  if (!queuedObjects)
    return;
  RenderObjectQueueObj  *o, *nxt;
  {
    TileMask  tileMask;
    size_t  n = 0;
    for (o = objectsRendered.front; o; o = o->nxt, n++)
      tileMask |= o->m;
    for (o = objectsReady.front; o; o = o->nxt, n++)
      tileMask |= o->m;
    if (n >= 32)
      return;
    if (!allowReorder)
    {
      for (o = queuedObjects.front; o; o = nxt)
      {
        nxt = o->nxt;
        if (BRANCH_UNLIKELY(!tileMask.overlapsWith(o->m)))
        {
          if (BRANCH_UNLIKELY(o->threadNum >= 256L))
            break;
          queuedObjects.pop(o);
          objectsReady.push_back(o);
        }
        tileMask |= o->m;
      }
    }
    else
    {
      for (o = queuedObjects.front; o; o = nxt)
      {
        nxt = o->nxt;
        if (BRANCH_UNLIKELY(!tileMask.overlapsWith(o->m)))
        {
          if (BRANCH_UNLIKELY(o->threadNum >= 256L))
            break;
          queuedObjects.pop(o);
          objectsReady.push_back(o);
        }
        if (o->o->flags & 0x1C)
          tileMask |= o->m;     // do not reorder water, effects and decals
      }
    }
  }
  if (BRANCH_UNLIKELY(loadingModelsFlag))
  {
    std::uint64_t modelIDMask = 0U;
    for (o = objectsRendered.front; o; o = o->nxt)
    {
      if (o->o->flags & 0x02)
        modelIDMask |= (std::uint64_t(1) << (o->o->model.o.b->modelID & 0xFFU));
    }
    for (o = objectsReady.front; o; o = o->nxt)
    {
      if (o->o->flags & 0x02)
        modelIDMask |= (std::uint64_t(1) << (o->o->model.o.b->modelID & 0xFFU));
    }
    for (o = queuedObjects.front; o && o->threadNum < 256L; o = o->nxt)
    {
      if (o->o->flags & 0x02)
        modelIDMask |= (std::uint64_t(1) << (o->o->model.o.b->modelID & 0xFFU));
    }
    for ( ; o; o = nxt)
    {
      nxt = o->nxt;
      if ((o->o->flags & 0x02) &&
          (modelIDMask
           & (std::uint64_t(1) << (o->o->model.o.b->modelID & 0xFFU))))
      {
        continue;
      }
      queuedObjects.pop(o);
      objectsReady.push_back(o);
    }
  }
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

bool Renderer::setScreenAreaUsed(RenderObject& p)
{
  NIFFile::NIFVertexTransform vt(p.modelTransform);
  NIFFile::NIFBounds  modelBounds;
  if (p.flags & 0x12)
  {
    FloatVector4  b0(p.model.o.b->objectBounds.boundsMin);
    FloatVector4  b1(p.model.o.b->objectBounds.boundsMax);
    FloatVector4  tmp(1.0f, 1.0f, 1.0f, 0.0f);
    if (BRANCH_UNLIKELY(p.flags & 0x10))
    {
      tmp[0] = p.model.d.scaleX;
      tmp[2] = p.model.d.scaleZ;
      if (!(p.flags & 0x80))
      {
        // add decal depth search range, unless using XPRM
        b0[1] = b0[1] + getDecalYOffsetMin(b0);
        b1[1] = b1[1] + getDecalYOffsetMax(b1);
      }
    }
    modelBounds.boundsMin = b0;
    modelBounds.boundsMin.minValues(b1);
    modelBounds.boundsMin -= 2.0f;
    modelBounds.boundsMax = b0;
    modelBounds.boundsMax.maxValues(b1);
    modelBounds.boundsMax += 2.0f;
    modelBounds.boundsMin *= tmp;
    modelBounds.boundsMax *= tmp;
    vt *= viewTransform;
  }
  else if (p.flags & 1)
  {
    if (!landData)
      return false;
    float   xyScale = 100.0f / float(landData->getCellResolution());
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
    p.model.t.z0 = z0;
    p.model.t.z1 = z1;
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
    vt *= viewTransform;
  }
  else
  {
    return false;
  }
  NIFFile::NIFBounds  screenBounds;
  FloatVector4  vtOffs(vt.offsX, vt.offsY, vt.offsZ, vt.rotateXX);
  FloatVector4  vtRotateX(vt.rotateXX, vt.rotateYX, vt.rotateZX, vt.rotateXY);
  FloatVector4  vtRotateY(vt.rotateXY, vt.rotateYY, vt.rotateZY, vt.rotateXZ);
  FloatVector4  vtRotateZ(vt.rotateXZ, vt.rotateYZ, vt.rotateZZ, vt.scale);
  vtRotateX *= vt.scale;
  vtRotateY *= vt.scale;
  vtRotateZ *= vt.scale;
  for (int i = 0; i < 8; i++)
  {
    FloatVector4  v(vtRotateX
                    * (!(i & 1) ? modelBounds.xMin() : modelBounds.xMax()));
    v += (vtRotateY * (!(i & 2) ? modelBounds.yMin() : modelBounds.yMax()));
    v += (vtRotateZ * (!(i & 4) ? modelBounds.zMin() : modelBounds.zMax()));
    v += vtOffs;
    screenBounds += v;
  }
  worldBounds += screenBounds.boundsMin;
  worldBounds += screenBounds.boundsMax;
  screenBounds.boundsMin -= 2.5f;
  screenBounds.boundsMax += 2.0f;
  if (screenBounds.xMin() > float(width) ||
      screenBounds.yMin() > float(height) ||
      screenBounds.zMin() > float(zRangeMax) ||
      screenBounds.xMax() < 0.0f || screenBounds.yMax() < 0.0f ||
      screenBounds.zMax() < 0.0f)
  {
    if (BRANCH_LIKELY(!ignoreOBND))
      return false;
    if (!(p.flags & 0x02))
      return false;
  }
  p.z = roundFloat(screenBounds.zMin() * 64.0f);
  if (BRANCH_UNLIKELY(!(p.flags & 0x02)))
  {
    FloatVector4  tmp(screenBounds.xMin(), screenBounds.yMin(),
                      screenBounds.xMax(), screenBounds.yMax());
    tmp += FloatVector4(1.25f, 1.25f, -0.75f, -0.75f);
    tmp /= FloatVector4(float(width), float(height),
                        float(width), float(height));
    tmp *= 16.0f;
    tmp.maxValues(FloatVector4(0.0f));
    tmp.minValues(FloatVector4(15.5f));
    p.flags2 = std::uint16_t(int(tmp[0]) | (int(tmp[1]) << 4)
                             | (int(tmp[2]) << 8) | (int(tmp[3]) << 12));
  }
  return true;
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
  tmp.flags2 = 0;
  tmp.z = 0;
  tmp.model.t.z0 = 0;           // will be calculated by setScreenAreaUsed()
  tmp.model.t.z1 = 0;
  tmp.model.t.waterFormID = 0U;
  tmp.formID = r.formID;
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
    if (threadCnt > 1 && n >= 64)
    {
      int     m = roundFloat(float(width) * float(height) * (1.0f / 10000.0f)
                             / (viewTransform.scale * viewTransform.scale));
      if (n >= 128 && m < (int(threadCnt) << 2))
        n = n >> 1;
      if (m < int(threadCnt))
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
      if (setScreenAreaUsed(tmp))
        objectList.push_back(tmp);
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
  if (!(waterLevel >= -32768.0f && waterLevel <= 32768.0f))
  {
    if (cellFlags & 0x01)
      return;
    waterLevel = defaultWaterLevel;
  }
  RenderObject  tmp;
  tmp.flags = 0x000C;                   // water cell, alpha blending
  tmp.flags2 = 0;
  tmp.z = 0;
  tmp.model.t.x0 = 0;
  tmp.model.t.y0 = 0;
  tmp.model.t.x1 = 100;
  tmp.model.t.y1 = 100;
  tmp.model.t.z0 = 0;
  tmp.model.t.z1 = 0;
  tmp.model.t.waterFormID = 0U;
  tmp.formID = r.formID;
  tmp.modelTransform.offsX = float(cellX) * 100.0f;
  tmp.modelTransform.offsY = float(cellY) * 100.0f;
  tmp.modelTransform.offsZ = waterLevel;
  if (setScreenAreaUsed(tmp))
  {
    if (waterRenderMode < 0)
    {
      tmp.model.t.waterFormID =
          getWaterMaterial(waterMaterials, esmFile, &r, 0U);
    }
    objectList.push_back(tmp);
  }
}

bool Renderer::getNPCModel(BaseObject& p, const ESMFile::ESMRecord& r)
{
  ESMFile::ESMField f(esmFile, r);
  while (f.next())
  {
    if (f == "WNAM" && f.size() == 4)
    {
      const ESMFile::ESMRecord  *r2 = esmFile.findRecord(f.readUInt32Fast());
      if (r2 && *r2 == "ARMO")
      {
        unsigned int  modelFormID = 0U;
        bool    haveOBND = false;
        ESMFile::ESMField f2(*r2, esmFile);
        while (f2.next())
        {
          if (f2 == "MODL" && f2.size() == 4)
          {
            unsigned int  tmp = f2.readUInt32Fast();
            if (!modelFormID ||
                std::abs(int(tmp) - int(r2->formID))
                < std::abs(int(modelFormID) - int(r2->formID)))
            {
              modelFormID = tmp;
            }
          }
          else if (f2 == "MODL" && f2.size() > 4)
          {
            f2.readPath(stringBuf, std::string::npos, "meshes/", ".nif");
          }
          else if (f2 == "OBND" && f2.size() >= 24)
          {
            p.objectBounds.boundsMin = f2.readFloatVector4();
            p.objectBounds.boundsMax[0] = p.objectBounds.boundsMin[3];
            p.objectBounds.boundsMax[1] = f2.readFloat();
            p.objectBounds.boundsMax[2] = f2.readFloat();
            p.objectBounds.boundsMin.clearV3();
            p.objectBounds.boundsMax.clearV3();
            haveOBND = true;
          }
        }
        if (stringBuf.empty() && modelFormID)
        {
          r2 = esmFile.findRecord(modelFormID);
          if (r2 && (*r2 == "ARMA" || *r2 == "ARMO"))
          {
            ESMFile::ESMField f3(*r2, esmFile);
            while (f3.next())
            {
              if ((f3 == "MOD2" || f3 == "MODL") && f3.size() > 4)
              {
                f3.readPath(stringBuf, std::string::npos, "meshes/", ".nif");
              }
              else if (f3 == "OBND" && f3.size() >= 24)
              {
                p.objectBounds.boundsMin = f3.readFloatVector4();
                p.objectBounds.boundsMax[0] = p.objectBounds.boundsMin[3];
                p.objectBounds.boundsMax[1] = f3.readFloat();
                p.objectBounds.boundsMax[2] = f3.readFloat();
                p.objectBounds.boundsMin.clearV3();
                p.objectBounds.boundsMax.clearV3();
                haveOBND = true;
              }
            }
          }
        }
        if (haveOBND && !stringBuf.empty())
          return true;
      }
    }
  }
  if (enableMarkers)
  {
    stringBuf = "meshes/markers/humanmarker.nif";
    p.flags = p.flags | 0x20;
    p.objectBounds.boundsMin = FloatVector4(-0.31f, -0.31f, 0.0f, 0.0f);
    p.objectBounds.boundsMax = FloatVector4(0.31f, 0.31f, 1.8f, 0.0f);
    return true;
  }
  return false;
}

void Renderer::readDecalProperties(BaseObject& p, const ESMFile::ESMRecord& r)
{
  if (!(r == "PDCL"))
    return;
  ESMFile::ESMField f(r, esmFile);
  bool    haveDATA = false;
  bool    haveDODT = false;
  while (f.next())
  {
    if (f == "DATA" && f.size() >= 24)
    {
      // 6 floats: min width, max width, min height, max height, depth, unknown
      FloatVector4  decalBounds(f.readFloatVector4());
      float   decalWidth = decalBounds[0] * decalBounds[1];
      float   decalHeight = decalBounds[2] * decalBounds[3];
      float   decalDepth = f.readFloat();
      decalWidth = std::min(std::max(decalWidth, 0.0f), 10000.0f);
      decalHeight = std::min(std::max(decalHeight, 0.0f), 10000.0f);
      decalDepth = std::min(std::max(decalDepth, 0.0f), 100.0f);
      if (!(decalWidth > 0.0f && decalHeight > 0.0f && decalDepth > 0.0f))
        return;
      decalWidth = float(std::sqrt(decalWidth));
      decalHeight = float(std::sqrt(decalHeight));
      p.objectBounds.boundsMin =
          FloatVector4(decalWidth, decalDepth, decalHeight, 0.0f);
      p.objectBounds.boundsMax = p.objectBounds.boundsMin;
      p.objectBounds.boundsMin *= -0.5f;
      p.objectBounds.boundsMax *= 0.5f;
      haveDATA = true;
      break;
    }
    else if (f == "DODT" && f.size() >= 4)
    {
      const ESMFile::ESMRecord  *r2 = esmFile.findRecord(f.readUInt32Fast());
      if (!(r2 && *r2 == "MTPT"))
        continue;
      ESMFile::ESMField f2(*r2, esmFile);
      while (f2.next() && !haveDODT)
      {
        if (!(f2 == "REFL" && f2.size() > 16))
          continue;
        ESMFile::CDBRecord  f3(f2);
        while (f3.next())
        {
          if (!(f3 == "OBJT" && f3.size() > 6))
            continue;
          const char  *componentType =
              f3.getStringFromTable(f3.readUInt32Fast());
          if (std::strcmp(componentType, "BGSMaterialPathForm") != 0)
            continue;
          unsigned int  len = f3.readUInt16Fast();
          if (len == (f3.size() - f3.getPosition()))
          {
            f3.readPath(stringBuf, len, "materials/", ".mat");
            if (!stringBuf.empty())
            {
              haveDODT = true;
              break;
            }
          }
        }
      }
    }
  }
  if (!(haveDATA && haveDODT))
    return;
  p.flags = 0x4010;                     // is decal, sort group = 2
  p.gradientMapV = 0;
  p.modelID = 0U;
  p.mswpFormID = 0xFFFFFFFFU;
  p.modelPath = stringBuf;
}

const Renderer::BaseObject * Renderer::readModelProperties(
    RenderObject& p, const ESMFile::ESMRecord& r)
{
  if (BRANCH_UNLIKELY(baseObjects.size() < size_t(baseObjHashMask + 1)))
    baseObjects.resize(size_t(baseObjHashMask + 1), std::int32_t(-1));
  BaseObject    *o = (BaseObject *) 0;
  std::uint32_t h;
  {
    std::uint64_t tmp = 0xFFFFFFFFU;
    hashFunctionUInt64(tmp, r.formID);
    h = std::uint32_t(tmp & baseObjHashMask);
  }
  for (std::int32_t i = baseObjects[h]; i >= 0; )
  {
    std::uint32_t n = std::uint32_t(i);
    BaseObject& tmp = baseObjectBufs[n >> baseObjBufShift][n & baseObjBufMask];
    if (tmp.formID == r.formID)
    {
      o = &tmp;
      break;
    }
    i = tmp.prv;
  }
  if (!o)
  {
    BaseObject  tmp;
    tmp.formID = r.formID;
    tmp.flags = std::uint16_t((r.flags >> 18) & 0x20);  // is marker
    tmp.gradientMapV = 0;
    tmp.modelID = 0xFFFFFFFFU;
    tmp.mswpFormID = 0U;
    tmp.objectBounds.boundsMin = FloatVector4(0.0f);
    tmp.objectBounds.boundsMax = FloatVector4(0.0f);
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
        case 0x4C434450:                // "PDCL"
          if (enableDecals)
            readDecalProperties(tmp, r);
          continue;
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
        if (f == "OBND" && f.size() >= 24)
        {
          tmp.objectBounds.boundsMin = f.readFloatVector4();
          tmp.objectBounds.boundsMax[0] = tmp.objectBounds.boundsMin[3];
          tmp.objectBounds.boundsMax[1] = f.readFloat();
          tmp.objectBounds.boundsMax[2] = f.readFloat();
          tmp.objectBounds.boundsMin.clearV3();
          tmp.objectBounds.boundsMax.clearV3();
          haveOBND = true;
        }
        else if ((f == "MODL" || (f == "MOD2" && r == "ARMO")) &&
                 f.size() > 4 && (!modelLOD || stringBuf.empty()))
        {
          f.readPath(stringBuf, std::string::npos, "meshes/", ".nif");
          isHDModel = isHighQualityModel(stringBuf);
        }
        else if (f == "MOLM" && f.size() >= 6)
        {
          // TODO: implement material swapping
        }
        else if (f == "WTFM" && r == "ACTI" && f.size() >= 4)
        {
          isWater = true;
        }
      }
      if (!haveOBND || stringBuf.empty())
      {
        if (BRANCH_LIKELY(!(enableActors && r == "NPC_")) ||
            !(haveOBND = getNPCModel(tmp, r)))
        {
          continue;
        }
      }
#if 0
      if (!isWater)
      {
        if (stringBuf.starts_with("meshes/water/") ||
            stringBuf.find("/fxwaterfall") != std::string::npos)
        {
          isWater = true;
        }
      }
#endif
      if ((!isWater && isExcludedModel(stringBuf)) ||
          (isWater && !waterRenderMode))
      {
        continue;
      }
      tmp.flags = tmp.flags | 0x2002
                  | (!isWater ? 0 : 0x0004) | (!isHDModel ? 0 : 0x0040)
                  | (!(r == "NPC_" || r == "CONT") ? 0 : 0x4000);
      const BA2File::FileDeclaration  *fd = ba2File.findFile(stringBuf);
      if (!fd)
        tmp.flags = 0;
      else
        tmp.modelPath = stringBuf;
    }
    while (false);
    if (baseObjectBufs.size() < 1 ||
        baseObjectBufs.back().size() >= (baseObjBufMask + 1U))
    {
      baseObjectBufs.emplace_back();
      baseObjectBufs.back().reserve(baseObjBufMask + 1U);
    }
    std::vector< BaseObject >&  objBuf = baseObjectBufs.back();
    size_t  n =
        ((baseObjectBufs.size() - 1) << baseObjBufShift) | objBuf.size();
    tmp.prv = baseObjects[h];
    objBuf.push_back(tmp);
    baseObjects[h] = std::int32_t(n);
    o = &(objBuf.back());
  }
  if (!(o->flags & 0x17))
    return (BaseObject *) 0;
  p.flags = o->flags;
  p.flags2 = o->gradientMapV;
  p.model.o.b = o;
  return o;
}

void Renderer::addSCOLObjects(
    const ESMFile::ESMRecord& r, const NIFFile::NIFVertexTransform& vt,
    unsigned int refrFormID)
{
  RenderObject  tmp;
  tmp.z = 0;
  tmp.formID = refrFormID;
  ESMFile::ESMField f(esmFile, r);
  const BaseObject  *o = (BaseObject *) 0;
  while (f.next())
  {
    if (f == "ONAM" && f.size() >= 4)
    {
      o = (BaseObject *) 0;
      unsigned int  modelFormID = f.readUInt32Fast();
      if (!modelFormID)
        continue;
      const ESMFile::ESMRecord  *r3 = esmFile.findRecord(modelFormID);
      if (!r3)
        continue;
      if (!(o = readModelProperties(tmp, *r3)) || (tmp.flags & 0x14))
      {
        o = (BaseObject *) 0;
        continue;
      }
    }
    else if (f == "DATA" && o)
    {
      while ((f.getPosition() + 28) <= f.size())
      {
        FloatVector4  d1(f.readFloatVector4()); // X, Y, Z, RX
        f.setPosition(f.getPosition() - 4);
        FloatVector4  d2(f.readFloatVector4()); // RX, RY, RZ, scale
        tmp.modelTransform =
            NIFFile::NIFVertexTransform(d2[3], d2[0], d2[1], d2[2],
                                        d1[0], d1[1], d1[2]);
        tmp.modelTransform *= vt;
        if (setScreenAreaUsed(tmp))
        {
          tmp.model.o.mswpFormID = 0U;
          tmp.model.o.mswpFormID2 = 0U;
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
    r = esmFile.findRecord(formID);
    if (BRANCH_UNLIKELY(!r))
      break;
    if (BRANCH_UNLIKELY(!(*r == "REFR")))
    {
      if (*r == "CELL")
      {
        const ESMFile::ESMRecord  *r2;
        if (!(r->parent && bool(r2 = esmFile.findRecord(r->parent)) &&
              r2->formID == 1U))        // ignore starting cell at 0, 0
        {
          if (type == 0)
            addTerrainCell(*r);
          else
            addWaterCell(*r);
        }
      }
      if (!(*r == "ACHR"))
      {
        if (!isRecursive)
          break;
        if (*r == "GRUP" && r->children && r->formID < 10U)
        {
          // group types 1 to 5 for terrain, + 6, 8, 9 for objects
          if ((1U << r->formID) & (!type ? 0x003EU : 0x037EU))
            findObjects(r->children, type, true);
        }
        continue;
      }
    }
    if ((r->flags & (!noDisabledObjects ? 0x00000020 : 0x00000820)) || !type)
      continue;                         // ignore deleted and disabled records
    RenderObject  tmp;
    tmp.flags = 0;
    tmp.flags2 = 0;
    tmp.z = 0;
    tmp.model.o.mswpFormID = 0U;
    tmp.model.o.mswpFormID2 = 0U;
    tmp.formID = r->formID;
    const ESMFile::ESMRecord  *r2 = (ESMFile::ESMRecord *) 0;
    const BaseObject  *o = (BaseObject *) 0;
    float   scale = 1.0f;
    float   decalScaleX = 1.0f;
    float   decalScaleZ = 1.0f;
    bool    objectVisible = false;
    {
      ESMFile::ESMField f(*r, esmFile);
      while (f.next())
      {
        switch (f.type)
        {
          case 0x454D414EU:             // "NAME" (must be first)
            if (f.size() >= 4 && !r2)
            {
              r2 = esmFile.findRecord(f.readUInt32Fast());
              if (r2 && !(*r2 == "SCOL"))
                o = readModelProperties(tmp, *r2);
            }
            break;
          case 0x4D525058U:             // "XPRM"
            if (f.size() >= 29 && (tmp.flags & 0x10) && f[28] == 0x01)  // box
            {
              tmp.flags = tmp.flags | 0x0080;
              FloatVector4  decalDimensions =
                  o->objectBounds.boundsMax - o->objectBounds.boundsMin;
              decalDimensions.maxValues(FloatVector4(0.01f));
              FloatVector4  b(f.readFloatVector4());
              b /= (decalDimensions * 0.5f);
              decalScaleX = b[0];
              scale = b[1];
              decalScaleZ = b[2];
            }
            break;
          case 0x534D4C58U:             // "XLMS"
            if (f.size() >= 6 && !(tmp.flags & 0x10))
            {
              // TODO: implement material swapping
            }
            break;
          case 0x44445058U:             // "XPDD"
            if (f.size() >= 8 && (tmp.flags & 0x90) == 0x10)
            {
              decalScaleX = f.readFloat();
              decalScaleZ = f.readFloat();
            }
            break;
          case 0x4C435358U:             // "XSCL"
            if (f.size() >= 4 && !(tmp.flags & 0x10))
              scale = f.readFloat();
            break;
          case 0x41544144U:             // "DATA" (must be last)
            if (f.size() >= 24 && r2)
            {
              FloatVector4  d1(f.readFloatVector4());   // X, Y, Z, RX
              f.setPosition(f.getPosition() - 8);
              FloatVector4  d2(f.readFloatVector4());   // Z, RX, RY, RZ
              tmp.modelTransform =
                  NIFFile::NIFVertexTransform(
                      scale, d2[1], d2[2], d2[3], d1[0], d1[1], d2[0]);
              if (BRANCH_UNLIKELY(tmp.flags & 0x10))
              {
                tmp.model.d.scaleX =
                    std::min(std::max(decalScaleX / scale, 0.0625f), 16.0f);
                tmp.model.d.scaleZ =
                    std::min(std::max(decalScaleZ / scale, 0.0625f), 16.0f);
              }
              if (*r2 == "SCOL")
                addSCOLObjects(*r2, tmp.modelTransform, r->formID);
              else if (BRANCH_LIKELY(o))
                objectVisible = setScreenAreaUsed(tmp);
            }
            break;
        }
      }
    }
    if (!objectVisible)
      continue;
    if (BRANCH_LIKELY(!(tmp.flags & 0x10)))
    {
      tmp.model.o.mswpFormID = 0U;
      if (BRANCH_UNLIKELY(tmp.flags & 0x04))
      {
        if (waterRenderMode < 0)
        {
          tmp.model.o.mswpFormID =
              getWaterMaterial(waterMaterials, esmFile, r2, 0U);
        }
      }
    }
    objectList.push_back(tmp);
  }
  while ((formID = r->next) != 0U && isRecursive);
}

void Renderer::findObjects(unsigned int formID, int type)
{
  if (!formID)
    return;
  const ESMFile::ESMRecord  *r = esmFile.findRecord(formID);
  if (!r)
    return;
  if (*r == "WRLD")
  {
    r = esmFile.findRecord(0U);
    while (r && r->next)
    {
      r = esmFile.findRecord(r->next);
      if (!r)
        break;
      if (*r == "GRUP" && r->formID == 0 && r->children)
      {
        unsigned int  groupID = r->children;
        while (groupID)
        {
          const ESMFile::ESMRecord  *r2 = esmFile.findRecord(groupID);
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
    r = esmFile.findRecord(0U);
    while (r)
    {
      if (*r == "GRUP")
      {
        if (r->formID <= 5U && r->children)
        {
          r = esmFile.findRecord(r->children);
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
        r = esmFile.findRecord(r->parent);
      if (!(r && r->next))
        break;
      r = esmFile.findRecord(r->next);
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
  std::map< ModelPathSortObject, unsigned int > modelPathsUsed;
  for (std::vector< RenderObject >::iterator
           i = objectList.begin(); i != objectList.end(); i++)
  {
    if (i->flags & 0x12)
    {
      ModelPathSortObject tmp;
      tmp.o = const_cast< BaseObject * >(i->model.o.b);
      modelPathsUsed[tmp] = 0U;
    }
  }
  unsigned int  n = 0U;
  unsigned int  prvFlags = 0U;
  for (std::map< ModelPathSortObject, unsigned int >::iterator
           i = modelPathsUsed.begin(); i != modelPathsUsed.end(); i++, n++)
  {
    if (BRANCH_UNLIKELY((n & 0xFFU) >= modelBatchCnt) ||
        BRANCH_UNLIKELY((i->first.o->flags ^ prvFlags) & 0xE000U))
    {
      n = (n + 0xFFU) & ~0xFFU;
      prvFlags = i->first.o->flags;
    }
    i->second = n;
  }
  for (size_t i = 0; i < baseObjectBufs.size(); i++)
  {
    for (std::vector< BaseObject >::iterator
             j = baseObjectBufs[i].begin(); j != baseObjectBufs[i].end(); j++)
    {
      ModelPathSortObject tmp;
      tmp.o = &(*j);
      std::map< ModelPathSortObject, unsigned int >::iterator k =
          modelPathsUsed.find(tmp);
      if (k == modelPathsUsed.end())
        j->modelID = 0xFFFFFFFFU;
      else
        j->modelID = k->second;
    }
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
  if (flags & 0x18)
  {
    renderObjectQueue->clear();
    for (size_t i = 0; i < renderThreads.size(); i++)
      renderThreads[i].clear();
  }
  if (flags & 0x08)
  {
    objectList.clear();
    renderPass = 0;
    objectListPos = 0;
    modelIDBase = 0xFFFFFFFFU;
  }
  if (flags & 0x20)
  {
    for (size_t i = 0; i < nifFiles.size(); i++)
      nifFiles[i].clear();
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

bool Renderer::loadModel(const BaseObject& o, size_t threadNum)
{
  size_t  n = o.modelID & 0xFFU;
  if (n >= modelBatchCnt)
    return false;
  nifFiles[n].clear();
  nifFiles[n].o = &o;
  if (o.modelPath.empty())
    return false;
  if (BRANCH_UNLIKELY(renderPass & 4) && !(o.flags & 4))
  {
    if (effectMeshMode & 2)
      return false;
    if (!(effectMeshMode & 1))
    {
      if (o.modelPath.starts_with("meshes/sky/"))
        return false;
      if (o.modelPath.starts_with("meshes/effects/"))
      {
        if (o.modelPath.compare(15, 8, "ambient/") == 0 ||
            o.modelPath.ends_with("fog.nif") ||
            o.modelPath.ends_with("cloud.nif"))
        {
          return false;
        }
      }
    }
  }
  try
  {
    std::vector< unsigned char >& fileBuf = renderThreads[threadNum].fileBuf;
    ba2File.extractFile(fileBuf, o.modelPath);
    unsigned char l = 0;
    if (!(o.flags & 0x0040))
      l = modelLOD;
    nifFiles[n].nifFile =
        new NIFFile(fileBuf.data(), fileBuf.size(), ba2File, &materials, l);
    nifFiles[n].nifFile->getMesh(nifFiles[n].meshData, 0U, 0U, true);
    size_t  meshCnt = nifFiles[n].meshData.size();
    for (size_t i = 0; i < meshCnt; i++)
    {
      const NIFFile::NIFTriShape& ts = nifFiles[n].meshData[i];
      // check hidden (0x8000) and alpha blending (0x1000) flags
      if (((ts.flags >> 10) ^ renderPass) & 0x24U)
      {
        const CE2Material::TextureSet *txtSet = findTextureSet(ts.m);
        if ((ts.flags & CE2Material::Flag_AlphaBlending) &&
            ((unsigned int) (txtSet && txtSet->texturePathMask)
             | (o.flags & 0x0020) | (ts.flags & CE2Material::Flag_IsWater)))
        {
          nifFiles[n].usesAlphaBlending = true;
        }
        continue;
      }
      if (ts.vertexCnt && ts.triangleCnt)
        ts.calculateBounds(nifFiles[n].objectBounds);
    }
  }
  catch (FO76UtilsError&)
  {
    nifFiles[n].clear();
    return false;
  }
  return true;
}

void Renderer::renderDecal(RenderThread& t, const RenderObject& p)
{
  NIFFile::NIFVertexTransform vt(p.modelTransform);
  vt *= viewTransform;
  FloatVector4  xpddScale(p.model.d.scaleX, 1.0f, p.model.d.scaleZ, 1.0f);
  NIFFile::NIFBounds  decalBounds;
  decalBounds.boundsMin = p.model.d.b->objectBounds.boundsMin * xpddScale;
  decalBounds.boundsMax = p.model.d.b->objectBounds.boundsMax * xpddScale;
  float   yOffset = 0.0f;
  if (!(p.flags & 0x80))
  {
    decalBounds.boundsMin[1] = getDecalYOffsetMin(decalBounds.boundsMin);
    decalBounds.boundsMax[1] = getDecalYOffsetMax(decalBounds.boundsMax);
    yOffset = t.renderer->findDecalYOffset(p.modelTransform, decalBounds);
    yOffset = std::max(yOffset, 0.0f);
  }
  decalBounds.boundsMin[1] = p.model.d.b->objectBounds.boundsMin[1] + yOffset;
  decalBounds.boundsMax[1] = p.model.d.b->objectBounds.boundsMax[1] + yOffset;
  NIFFile::NIFBounds  b;
  NIFFile::NIFVertex  v;
  for (int i = 0; i < 8; i++)
  {
    v.xyz[0] = (!(i & 1) ? decalBounds.boundsMin[0] : decalBounds.boundsMax[0]);
    v.xyz[1] = (!(i & 2) ? decalBounds.boundsMin[1] : decalBounds.boundsMax[1]);
    v.xyz[2] = (!(i & 4) ? decalBounds.boundsMin[2] : decalBounds.boundsMax[2]);
    v.xyz = vt.transformXYZ(v.xyz);
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
  if (p.model.d.b->modelPath.empty())
    return;
  const CE2Material *m = materials.findMaterial(p.model.d.b->modelPath);
  const CE2Material::TextureSet *txtSet = findTextureSet(m);
  if (!txtSet)
    return;
  t.renderer->m = m;
  const DDSTexture  *textures[10];
  unsigned int  textureMask = 0U;
  unsigned int  texturePathMask = txtSet->texturePathMask;
  std::uint16_t texturePathMaskBase = 0x0005;
  if (renderModeQuality & 2)
    texturePathMaskBase = 0x003F;
  else if (renderQuality >= 1)
    texturePathMaskBase = 0x0007;
  texturePathMask &=
      (((unsigned int) t.renderer->flags & 0x80U) | texturePathMaskBase);
  if (!texturePathMask)
    return;
  if (BRANCH_UNLIKELY(!enableTextures))
  {
    texturePathMask &= ~0x0001U;
    textures[0] = &whiteTexture;
    textureMask |= 0x0001U;
  }
  for (unsigned int j = 0x00080200U; texturePathMask; j = j >> 1)
  {
    unsigned int  tmp = texturePathMask & j;
    if (!tmp)
      continue;
    int     k = int(std::bit_width(tmp)) - 1;
    bool    waitFlag = false;
    textures[k] = textureCache.loadTexture(
                      ba2File, *(txtSet->texturePaths[k]), t.fileBuf,
                      textureMip, (j > 0x03FFU ? &waitFlag : (bool *) 0));
    if (!waitFlag)
    {
      texturePathMask &= ~tmp;
      if (textures[k])
        textureMask |= tmp;
    }
  }
  if (!defaultEnvMap.empty())
  {
    t.renderer->setEnvironmentMap(textureCache.loadTexture(
                                      ba2File, defaultEnvMap, t.fileBuf, 0));
  }
  t.renderer->setRenderMode(renderModeQuality);
  std::uint32_t decalColor = std::uint32_t(p.model.d.b->mswpFormID);
  if (!(decalColor & 0x08000000U))
  {
    // generate pseudo-random subtexture index
    std::uint64_t h = 0xFFFFFFFFU;
    hashFunctionUInt64(
        h, FileBuffer::readUInt64Fast(&(p.modelTransform.offsX)));
    hashFunctionUInt64(
        h, FileBuffer::readUInt32Fast(&(p.modelTransform.offsZ)));
    decalColor = (decalColor & 0x3FFFFFFFU) | std::uint32_t(h & 0xC0000000U);
  }
  t.renderer->drawDecal(p.modelTransform, textures, textureMask,
                        decalBounds, decalColor);
}

void Renderer::renderObject(RenderThread& t, const RenderObject& p)
{
  t.renderer->setDebugMode(debugMode, p.formID);
  if (p.flags & 0x02)                   // object or water mesh
  {
    NIFFile::NIFVertexTransform vt(p.modelTransform);
    vt *= viewTransform;
    size_t  n = p.model.o.b->modelID & 0xFFU;
    t.sortBuf.clear();
    t.sortBuf.reserve(nifFiles[n].meshData.size());
    for (size_t j = 0; j < nifFiles[n].meshData.size(); j++)
    {
      const NIFFile::NIFTriShape& ts = nifFiles[n].meshData[j];
      // check hidden (0x8000) and alpha blending (0x1000) flags
      if ((((ts.flags >> 10) ^ renderPass) & 0x24U) || !ts.triangleCnt)
        continue;
      NIFFile::NIFBounds  b;
      ts.calculateBounds(b, &vt);
      int     x0 = roundFloat(b.xMin());
      int     y0 = roundFloat(b.yMin());
      int     x1 = roundFloat(b.xMax());
      int     y1 = roundFloat(b.yMax());
      if (x0 >= width || y0 >= height || x1 < 0 || y1 < 0)
        continue;
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
                        bool(ts.flags & CE2Material::Flag_AlphaBlending));
      if (BRANCH_UNLIKELY(ts.flags & NIFFile::NIFTriShape::Flag_TSOrdered))
        TriShapeSortObject::orderedNodeFix(t.sortBuf, nifFiles[n].meshData);
    }
    if (t.sortBuf.begin() == t.sortBuf.end())
      return;
    std::sort(t.sortBuf.begin(), t.sortBuf.end());
    std::uint16_t renderModeQuality =
        renderMode | renderQuality | ((p.flags >> 5) & 2);
    std::uint16_t texturePathMaskBase = 0x0005;
    if (renderModeQuality & 2)
      texturePathMaskBase = 0x003F;
    else if (renderQuality >= 1)
      texturePathMaskBase = 0x0007;
    for (size_t j = 0; j < t.sortBuf.size(); j++)
    {
      *(t.renderer) = nifFiles[n].meshData[size_t(t.sortBuf[j])];
      const DDSTexture  *textures[10];
      unsigned int  textureMask = 0U;
      if (BRANCH_UNLIKELY(t.renderer->flags & CE2Material::Flag_IsWater))
      {
        t.renderer->setRenderMode(3U | renderMode);
        std::map< unsigned int, WaterProperties >::const_iterator k =
            waterMaterials.find(p.model.o.mswpFormID);
        if (k == waterMaterials.end())
          continue;
        t.renderer->m = (CE2Material *) 0;
        textures[1] = textureCache.loadTexture(ba2File, defaultWaterTexture,
                                               t.fileBuf, 0);
        if (textures[1])
          textureMask |= 0x0002U;
        if (!defaultEnvMap.empty())
        {
          t.renderer->setEnvironmentMap(textureCache.loadTexture(
                                            ba2File, defaultEnvMap,
                                            t.fileBuf, 0));
        }
        t.renderer->setWaterProperties(
            k->second.deepColor, k->second.alphaDepth0, k->second.depthMult,
            waterUVScale, 1.0f, 0.02032076f, waterReflectionLevel);
      }
      else
      {
        // TODO: implement material swapping
        const CE2Material::TextureSet *txtSet = findTextureSet(t.renderer->m);
        if (!txtSet)
          continue;
        unsigned int  texturePathMask = txtSet->texturePathMask;
        texturePathMask &=
            (((unsigned int) t.renderer->flags & 0x80U) | texturePathMaskBase);
        if (!texturePathMask)
        {
          if (!(p.flags & 0x0020))
            continue;
          textures[0] = &whiteTexture;  // marker with vertex colors only
          textureMask |= 0x0001U;
        }
        if (BRANCH_UNLIKELY(!enableTextures))
        {
          texturePathMask &= ~0x0001U;
          textures[0] = &whiteTexture;
          textureMask |= 0x0001U;
        }
        for (unsigned int m = 0x00080200U; texturePathMask; m = m >> 1)
        {
          unsigned int  tmp = texturePathMask & m;
          if (!tmp)
            continue;
          int     k = int(std::bit_width(tmp)) - 1;
          bool    waitFlag = false;
          textures[k] = textureCache.loadTexture(
                            ba2File, *(txtSet->texturePaths[k]), t.fileBuf,
                            textureMip, (m > 0x03FFU ? &waitFlag : (bool *) 0));
          if (!waitFlag)
          {
            texturePathMask &= ~tmp;
            if (textures[k])
              textureMask |= tmp;
          }
        }
        if (!defaultEnvMap.empty())
        {
          t.renderer->setEnvironmentMap(textureCache.loadTexture(
                                            ba2File, defaultEnvMap,
                                            t.fileBuf, 0));
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
      unsigned int  ltexMask =
          (renderQuality < 1 ? 0x81U : (renderQuality < 3 ? 0x83U : 0xBBU));
      t.terrainMesh->createMesh(
          *landData, landTxtScale,
          p.model.t.x0, p.model.t.y0, p.model.t.x1, p.model.t.y1,
          landTextures, landData->getTextureCount(), ltexMask,
          landTextureMip - float(int(landTextureMip)),
          landTxtRGBScale, landTxtDefColor);
      t.renderer->setRenderMode((renderQuality - (renderQuality & 2))
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
      vTmp[j].xyz = FloatVector4(float(j == 0 || j == 3 ? x0 : x1),
                                 float(j == 0 || j == 1 ? y0 : y1), 0.0f, 1.0f);
      vTmp[j].texCoord = FloatVector4((j == 0 || j == 3 ? 0.0f : 2.0f),
                                      (j == 0 || j == 1 ? 0.0f : 2.0f),
                                      0.0f, 0.0f);
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
    std::map< unsigned int, WaterProperties >::const_iterator j =
        waterMaterials.find(p.model.t.waterFormID);
    if (j == waterMaterials.end())
      return;
    tmp.m = (CE2Material *) 0;
    t.renderer->setRenderMode(3U | renderMode);
    *(t.renderer) = tmp;
    const DDSTexture  *textures[2];
    unsigned int  textureMask = 0U;
    textures[1] = textureCache.loadTexture(ba2File, defaultWaterTexture,
                                           t.fileBuf, 0);
    if (textures[1])
      textureMask |= 0x0002U;
    if (!defaultEnvMap.empty())
    {
      t.renderer->setEnvironmentMap(textureCache.loadTexture(
                                        ba2File, defaultEnvMap, t.fileBuf, 0));
    }
    t.renderer->setWaterProperties(
        j->second.deepColor, j->second.alphaDepth0, j->second.depthMult,
        waterUVScale, 1.0f, 0.02032076f, waterReflectionLevel);
    t.renderer->drawTriShape(p.modelTransform, textures, textureMask);
  }
  else if ((p.flags & 0x10) && !(renderPass & 0x04))    // decal
  {
    renderDecal(t, p);
  }
}

void Renderer::renderThread(size_t threadNum)
{
  RenderThread& t = renderThreads[threadNum];
  if (!t.renderer)
    return;
  RenderObjectQueue&    q = *renderObjectQueue;
  RenderObjectQueueObj  *o = (RenderObjectQueueObj *) 0;
  bool    isModel = false;
  while (true)
  {
    while (!o)
    {
      std::unique_lock< std::mutex >  tmpLock(q.m);
      while (!((q.objectsReady && !q.pauseFlag) || q.doneFlag))
        q.cv1.wait(tmpLock);
      if (q.doneFlag)
        return;
      o = q.objectsReady.front;
      q.objectsReady.pop(o);
      isModel = (o->threadNum >= 256L);
      o->threadNum = std::intptr_t(threadNum);
      q.objectsRendered.push_back(o);
    }
    if (BRANCH_LIKELY(!isModel))
      renderObject(t, *(o->o));
    else if (o->o->flags & 0x02)
      (void) loadModel(*(o->o->model.o.b), threadNum);
    bool    notifyFlag;
    {
      std::lock_guard< std::mutex > tmpLock(q.m);
      q.objectsRendered.pop(o);
      q.freeObjects.push_front(o);
      o = (RenderObjectQueueObj *) 0;
      notifyFlag = !q.objectsReady;
      q.findObjects();
      if (q.objectsReady && !(q.doneFlag || q.pauseFlag))
      {
        o = q.objectsReady.front;
        q.objectsReady.pop(o);
        isModel = (o->threadNum >= 256L);
        o->threadNum = std::intptr_t(threadNum);
        q.objectsRendered.push_back(o);
      }
      notifyFlag = notifyFlag && q.objectsReady;
    }
    if (notifyFlag)
      q.cv1.notify_all();
    q.cv2.notify_all();
  }
}

void Renderer::threadFunction(Renderer *p, size_t threadNum)
{
  RenderObjectQueue&  q = *(p->renderObjectQueue);
  try
  {
    p->renderThreads[threadNum].errMsg.clear();
    p->renderThread(threadNum);
  }
  catch (std::exception& e)
  {
    try
    {
      const char  *s = e.what();
      if (!s || *s == '\0')
        s = "unknown error in render thread";
      p->renderThreads[threadNum].errMsg = s;
    }
    catch (...)
    {
      p->renderThreads[threadNum].errMsg = "std::bad_alloc";
    }
  }
  {
    std::lock_guard< std::mutex > tmpLock(q.m);
    for (RenderObjectQueueObj *o = q.objectsRendered.front; o; )
    {
      RenderObjectQueueObj  *nxt = o->nxt;
      if (o->threadNum == std::intptr_t(threadNum))
      {
        q.objectsRendered.pop(o);
        q.freeObjects.push_front(o);
      }
      o = nxt;
    }
    if (!p->renderThreads[threadNum].errMsg.empty())
      q.doneFlag = true;
  }
  q.cv2.notify_all();
  q.cv1.notify_all();
}

Renderer::Renderer(int imageWidth, int imageHeight, const BA2File& archiveFiles,
                   ESMFile& masterFiles, const char *materialDBPath,
                   std::uint32_t *bufRGBA, float *bufZ, int zMax)
  : outBufRGBA(bufRGBA),
    outBufZ(bufZ),
    width(imageWidth),
    height(imageHeight),
    ba2File(archiveFiles),
    esmFile(masterFiles),
    viewTransform(4.0f, 3.14159265f, 0.0f, 0.0f,
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
    renderMode(0U),
    debugMode(0),
    renderPass(0),
    ignoreOBND(false),
    threadCnt(0),
    modelBatchCnt(16),
    landTextures((LandscapeTextureSet *) 0),
    objectListPos(0),
    modelIDBase(0xFFFFFFFFU),
    renderObjectQueue((RenderObjectQueue *) 0),
    waterUVScale(0.032258f),
    waterReflectionLevel(1.0f),
    zRangeMax(zMax),
    effectMeshMode(0),
    enableActors(false),
    waterRenderMode(0),
    bufAllocFlags((unsigned char) (int(!bufRGBA) | (int(!bufZ) << 1))),
    whiteTexture(0xFFFFFFFFU),
    outBufN((std::uint32_t *) 0),
    materials(ba2File, materialDBPath)
{
  if (!renderMode)
    renderMode = 4;
  size_t  imageDataSize = size_t(width) * size_t(height);
  if (bufAllocFlags & 0x01)
    outBufRGBA = new std::uint32_t[imageDataSize];
  if (bufAllocFlags & 0x02)
    outBufZ = new float[imageDataSize];
  clear(bufAllocFlags);
  renderThreads.reserve(maxThreadCnt);
  renderObjectQueue = new RenderObjectQueue(renderObjQueueSize);
  try
  {
    nifFiles.resize(modelBatchCnt);
    setThreadCount(0);
    setWaterColor(0xFFFFFFFFU);
  }
  catch (...)
  {
    delete renderObjectQueue;
    throw;
  }
}

Renderer::~Renderer()
{
  clear(0x7C);
  deallocateBuffers(0x03);
  delete renderObjectQueue;
}

unsigned int Renderer::findParentWorld(ESMFile& esmFile, unsigned int formID)
{
  if (!formID)
    return 0xFFFFFFFFU;
  const ESMFile::ESMRecord  *r = esmFile.findRecord(formID);
  if (!(r && (*r == "WRLD" || *r == "GRUP" || *r == "CELL" || *r == "REFR")))
    return 0xFFFFFFFFU;
  if (!(*r == "WRLD"))
  {
    while (!(*r == "GRUP" && r->formID == 1U))
    {
      if (!r->parent)
        return 0U;
      r = esmFile.findRecord(r->parent);
      if (!r)
        return 0U;
    }
    formID = r->flags;
    r = esmFile.findRecord(formID);
  }
  return formID;
}

void Renderer::clear()
{
  clear(0x7C);
  baseObjects.clear();
  baseObjectBufs.clear();
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
  (void) new(&viewTransform) NIFFile::NIFVertexTransform(
                                 scale, rotationX, rotationY, rotationZ,
                                 offsetX, offsetY, offsetZ);
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
    n = getDefaultThreadCount();
  int     maxThreads = int(renderThreads.capacity());
  n = (n > 1 ? (n < maxThreads ? n : maxThreads) : 1);
  if (n == int(threadCnt))
    return;
  clear(0x10);
  renderThreads.resize(size_t(n));
  threadCnt = (unsigned char) renderThreads.size();
  for (size_t i = 0; i < threadCnt; i++)
  {
    if (!renderThreads[i].renderer)
    {
      renderThreads[i].renderer =
          new Plot3D_TriShape(outBufRGBA, outBufZ, width, height, renderMode);
    }
  }
}

void Renderer::setModelCacheSize(int n)
{
  size_t  tmp = size_t(std::min(std::max(n, 1), 64));
  if (tmp == modelBatchCnt)
    return;
  clear(0x20);
  nifFiles.resize(tmp);
  modelBatchCnt = (unsigned char) nifFiles.size();
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
  getWaterMaterial(waterMaterials, esmFile, (ESMFile::ESMRecord *) 0, n, true);
}

void Renderer::setRenderParameters(
    int lightColor, int ambientColor, int envColor,
    float lightLevel, float envLevel, float rgbScale, float reflZScale)
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
  }
}

static int calculateLandTxtMip(long fileSize)
{
  unsigned long long  n = (unsigned long) std::max(fileSize - 128L, 2L);
  unsigned int  m = (unsigned int) std::bit_width(n);
  // return log2(resolution)
  return int(m >> 1);
}

void Renderer::loadTerrain(const char *btdFileName, unsigned int worldID,
                           int mipLevel, int xMin, int yMin, int xMax, int yMax)
{
  if (!landData)
  {
    landData = new LandscapeData(&esmFile, btdFileName, &ba2File, &materials,
                                 worldID, mipLevel, xMin, yMin, xMax, yMax);
    defaultWaterLevel = landData->getWaterLevel();
  }
  if (waterRenderMode < 0)
  {
    const ESMFile::ESMRecord  *r =
        esmFile.findRecord(landData->getWaterFormID());
    if (r && *r == "WATR")
      getWaterMaterial(waterMaterials, esmFile, r, 0xC0302010U, true);
  }
  size_t  textureCnt = landData->getTextureCount();
  if (!landTextures)
    landTextures = new LandscapeTextureSet[textureCnt];
  std::vector< unsigned char >& fileBuf(renderThreads[0].fileBuf);
  int     mipLevelD = std::min(textureMip + int(landTextureMip), 15);
  for (size_t i = 0; i < textureCnt; i++)
  {
    const CE2Material *m = landData->getTextureMaterial(i);
    const CE2Material::TextureSet *txtSet = findTextureSet(m);
    long    fileSizeD = -1L;
    for (unsigned int j = 0U; j < 8U; j++)
    {
      landTextures[i][j] = (DDSTexture *) 0;
      if (!((1U << j) & (renderQuality < 1 ?
                         0x85U : (renderQuality < 3 ? 0x87U : 0xBFU))))
      {
        continue;
      }
      if (!(txtSet && (txtSet->texturePathMask & (1U << j))))
        continue;
      const std::string&  fileName = *(txtSet->texturePaths[j]);
      int     mipLevelN = mipLevelD;
      if (!j)
      {
        fileSizeD = ba2File.getFileSize(fileName);
        if (!enableTextures)
        {
          landTextures[i][j] = &whiteTexture;
          continue;
        }
      }
      else if (fileSizeD > 0L)
      {
        long    fileSizeN = ba2File.getFileSize(fileName);
        mipLevelN += (calculateLandTxtMip(fileSizeN)
                      - calculateLandTxtMip(fileSizeD));
        mipLevelN = (mipLevelN > 0 ? (mipLevelN < 15 ? mipLevelN : 15) : 0);
      }
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
  renderObjectQueue->doneFlag = false;
  objectListPos = 0;
  modelIDBase = 0xFFFFFFFFU;
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
      break;
    default:
      return;
  }
  sortObjectList();
  if (enableDecals && !debugMode && !outBufN)
  {
    size_t  imageDataSize = size_t(width) * size_t(height);
    outBufN = new std::uint32_t[imageDataSize];
    for (size_t i = 0; i < imageDataSize; i++)
      outBufN[i] = 0U;
  }
  for (size_t i = 0; i < renderThreads.size(); i++)
  {
    renderThreads[i].renderer->setBuffers(outBufRGBA, outBufZ, width, height,
                                          outBufN);
    renderThreads[i].renderer->setViewAndLightVector(viewTransform,
                                                     lightX, lightY, lightZ);
  }
  for (size_t i = 0; i < renderThreads.size(); i++)
    renderThreads[i].t = new std::thread(threadFunction, this, i);
}

bool Renderer::renderObjects(int t)
{
  std::chrono::time_point< std::chrono::steady_clock >  endTime =
      std::chrono::steady_clock::now();
  if (t > 0)
  {
    endTime +=
        std::chrono::duration_cast< std::chrono::steady_clock::duration >(
            std::chrono::milliseconds(t));
  }
  RenderObjectQueue&  q = *renderObjectQueue;
  {
    std::lock_guard< std::mutex > tmpLock(q.m);
    q.pauseFlag = false;
    q.findObjects();
  }
  q.cv1.notify_all();
  while (true)
  {
    bool    objectListEndFlag = (objectListPos >= objectList.size());
    // check for events
    {
      std::unique_lock< std::mutex >  tmpLock(q.m);
      while (!q.doneFlag)
      {
        bool    queueEmptyFlag =
            !(q.queuedObjects || q.objectsReady || q.objectsRendered);
        if (objectListEndFlag && queueEmptyFlag)
        {
          q.doneFlag = true;
          break;
        }
        if (q.pauseFlag && !q.objectsRendered)
          return false;
        if (BRANCH_UNLIKELY(q.loadingModelsFlag))
        {
          if (queueEmptyFlag)
          {
            q.loadingModelsFlag = false;
            if (!(renderPass & 1))
            {
              tmpLock.unlock();
              textureCache.shrinkTextureCache();
              tmpLock.lock();
            }
            continue;
          }
        }
        else if (q.freeObjects && !objectListEndFlag)
        {
          break;
        }
        if (t > 0 && std::chrono::steady_clock::now() >= endTime)
        {
          q.pauseFlag = true;
          t = 0;
          continue;
        }
        q.cv2.wait(tmpLock);
      }
      if (q.doneFlag)
        break;
    }
    size_t  i = objectListPos;
    RenderObject& o = objectList[i];
    unsigned int  modelID = 0xFFFFFFFFU;
    if ((o.flags & 0x12) &&
        (((modelID = o.model.o.b->modelID) ^ modelIDBase) & ~0xFFU))
    {
      if (BRANCH_UNLIKELY(q.pauseFlag))
        continue;
      // schedule loading new set of models
      modelIDBase = modelID & ~0xFFU;
      unsigned long long  m = 0ULL;
      bool    notifyFlag = false;
      for (std::vector< RenderObject >::iterator
               j = objectList.begin() + i; true; j++)
      {
        if (notifyFlag)
        {
          notifyFlag = false;
          q.cv1.notify_one();
        }
        if (j == objectList.end())
          break;
        if (!(j->flags & 0x02))
          continue;
        unsigned int  n = j->model.o.b->modelID;
        if ((n & ~0xFFU) != modelIDBase)
          break;
        n = n & 0xFFU;
        if (m & (1ULL << n))
          continue;
        m = m | (1ULL << n);
        std::unique_lock< std::mutex >  tmpLock(q.m);
        while (!(q.freeObjects || q.doneFlag))
          q.cv2.wait_for(tmpLock, std::chrono::milliseconds(20));
        if (q.doneFlag)
          break;
        RenderObjectQueueObj  *tmp = q.freeObjects.front;
        q.freeObjects.pop(tmp);
        q.loadingModelsFlag = true;
        tmp->o = &(*j);
        tmp->threadNum = 256L;
        tmp->m = TileMask();
        notifyFlag = q.queueObject(tmp);
      }
      continue;
    }
    objectListPos++;
    // calculate object bounds on screen
    TileMask  tileMask;
    if (BRANCH_LIKELY(o.flags & 0x02))
    {
      if (nifFiles[modelID & 0xFFU].usesAlphaBlending)
        o.flags = o.flags | 0x08;
      NIFFile::NIFBounds  b(nifFiles[modelID & 0xFFU].objectBounds);
      if (BRANCH_UNLIKELY(!b))
        continue;
      NIFFile::NIFVertexTransform vt(o.modelTransform);
      vt *= viewTransform;
      (void) new(&tileMask) TileMask(b, vt, width, height);
    }
    else
    {
      (void) new(&tileMask) TileMask(o.flags2 & 15U, (o.flags2 >> 4) & 15U,
                                     (o.flags2 >> 8) & 15U,
                                     (o.flags2 >> 12) & 15U);
    }
    if (!tileMask)
      continue;
    // add next object to queue
    bool    notifyFlag;
    {
      std::lock_guard< std::mutex > tmpLock(q.m);
      RenderObjectQueueObj  *tmp = q.freeObjects.front;
      q.freeObjects.pop(tmp);
      tmp->o = &o;
      tmp->threadNum = -1L;
      tmp->m = tileMask;
      notifyFlag = q.queueObject(tmp);
    }
    if (notifyFlag)
      q.cv1.notify_one();
  }
  q.clear();
  bool    haveThreads = false;
  for (size_t i = 0; i < renderThreads.size(); i++)
  {
    if (renderThreads[i].t)
    {
      renderThreads[i].join();
      haveThreads = true;
    }
  }
  textureCache.shrinkTextureCache();
  clear(0x20);
  if (haveThreads)
  {
    for (size_t i = 0; i < renderThreads.size(); i++)
    {
      if (!renderThreads[i].errMsg.empty())
        throw FO76UtilsError(1, renderThreads[i].errMsg.c_str());
    }
  }
  return true;
}

size_t Renderer::getObjectsRendered()
{
  std::intptr_t n = std::intptr_t(objectListPos);
  RenderObjectQueue&  q = *renderObjectQueue;
  std::lock_guard< std::mutex > tmpLock(q.m);
  RenderObjectQueueObj  *o;
  for (o = q.objectsRendered.front; o; o = o->nxt)
    n -= std::intptr_t(bool(o->m));
  for (o = q.objectsReady.front; o; o = o->nxt)
    n -= std::intptr_t(bool(o->m));
  for (o = q.queuedObjects.front; o; o = o->nxt)
    n -= std::intptr_t(bool(o->m));
  return size_t(std::max(n, std::intptr_t(0)));
}

