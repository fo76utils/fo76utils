
#ifndef RENDER_HPP_INCLUDED
#define RENDER_HPP_INCLUDED

#include "common.hpp"
#include "filebuf.hpp"
#include "ba2file.hpp"
#include "esmfile.hpp"
#include "landdata.hpp"
#include "ddstxt.hpp"
#include "landtxt.hpp"
#include "bgsmfile.hpp"
#include "nif_file.hpp"
#include "terrmesh.hpp"
#include "plot3d.hpp"
#include "rndrbase.hpp"

#include <thread>
#include <mutex>
#include <condition_variable>

class Renderer : protected Renderer_Base
{
 protected:
  enum
  {
    maxThreadCnt = 16,
    baseObjBufShift = 12,
    baseObjBufMask = 0x0FFF,
    baseObjHashMask = 0xFFFF,
    renderObjQueueSize = 512
  };
  struct BaseObject
  {
    unsigned int  formID;
    std::int32_t  prv;                  // previous object with the same hash
    unsigned int  flags;                // same as RenderObject flags
    unsigned int  modelID;              // b0 to b7: nifFiles index
    unsigned int  mswpFormID;           // color + flags (in alpha) for decals
    signed short  obndX0;
    signed short  obndY0;
    signed short  obndZ0;
    signed short  obndX1;
    signed short  obndY1;
    signed short  obndZ1;
    const std::string *modelPath;       // NULL for decals
  };
  struct RenderObject
  {
    // 1: terrain, 2: solid object, 4: water cell, 6: water mesh
    // 0x08: uses alpha blending
    // 0x10: is decal (TXST object)
    // 0x20: is marker
    // 0x40: high quality model
    // 0x80: for objects: bits 8-15 = grayscale to palette map scale from MODC
    //       for decals:  uses XPRM (disables XPDD and finding point of impact)
    // bits 8 to 15: (objects only if b7 is set) grayscale to palette map scale
    // bits 29 to 31: sort group (higher = rendered later)
    //           0 = cell (terrain or water), no valid model ID
    //           1 = object
    //           2 = decal (TXST)
    //           3 = object (e.g. actor) that decals cannot be projected onto
    unsigned int  flags;
    int     z;                  // Z coordinate for sorting
    union
    {
      struct                    // object data (valid if flags & 2 is set)
      {
        const BaseObject  *b;
        // for solid objects: material swap form ID if non-zero
        // for water: form ID of WATR object
        unsigned int  mswpFormID;
        // form ID of second material swap
        unsigned int  mswpFormID2;
      }
      o;
      struct                    // decal data (valid if flags & 16 is set)
      {
        const BaseObject  *b;
        // from XPDD
        float   scaleX;
        float   scaleZ;
      }
      d;
      struct                    // cell data (terrain or water)
      {
        // For terrain, the X and Y bounds are heightmap image coordinates,
        // and the Z bounds are raw 16-bit height values - 32768.
        // For water cells, the bounds are always x0 = y0 = z0 = z1 = 0
        // and x1 = y1 = 4096.
        signed short  x0;
        signed short  y0;
        signed short  x1;
        signed short  y1;
        signed short  z0;
        signed short  z1;
        unsigned int  waterFormID;      // WATR object
      }
      t;
    }
    model;
    unsigned int  formID;               // form ID of reference or cell
    NIFFile::NIFVertexTransform modelTransform;
    bool operator<(const RenderObject& r) const;
  };
  struct ModelPathSortObject
  {
    BaseObject  *o;
    inline bool operator<(const ModelPathSortObject& r) const
    {
      if ((o->flags ^ r.o->flags) & 0xE0000000U)
        return (o->flags < r.o->flags);
      if (o->modelPath && r.o->modelPath)
        return (*(o->modelPath) < *(r.o->modelPath));
      return (o->formID < r.o->formID);
    }
  };
  struct ModelData
  {
    NIFFile *nifFile;
    const BaseObject  *o;
    NIFFile::NIFBounds  objectBounds;
    std::vector< NIFFile::NIFTriShape > meshData;
    bool    usesAlphaBlending;
    ModelData();
    ~ModelData();
    void clear();
  };
  struct TileMask
  {
#if ENABLE_X86_64_AVX
    YMM_UInt64    m;
#else
    std::uint64_t m[4];
#endif
    inline TileMask();
    TileMask(const NIFFile::NIFBounds& screenBounds, int w, int h);
    inline bool overlapsWith(const TileMask& r) const;
    inline TileMask& operator|=(const TileMask& r);
    inline operator bool() const;
    inline bool isFull() const;
  };
  struct RenderObjectQueueObj
  {
    const RenderObject  *o;
    RenderObjectQueueObj  *prv;         // previous object in queue
    RenderObjectQueueObj  *nxt;         // next object in queue
    // thread working on object (< 0: none, >= 256: this is a model load event,
    // o->model.o.b contains the model ID and path, all bits of m are set)
    std::intptr_t threadNum;
    TileMask  m;
  };
  struct RenderObjectQueue
  {
    std::vector< RenderObjectQueueObj > buf;
    RenderObjectQueueObj  *firstObject;         // NULL: empty queue
    RenderObjectQueueObj  *lastObject;
    RenderObjectQueueObj  *firstFreeObject;     // NULL: full queue
    // true on all objects rendered / error
    bool    doneFlag;
    // true stops worker threads from processing new objects
    bool    pauseFlag;
    // true if load model events have been added to the queue
    bool    loadingModelsFlag;
    // bit mask of threads currently rendering
    std::uint32_t threadsActive;
    std::mutex  m;
    std::condition_variable cv;
    RenderObjectQueue(size_t bufSize);
    void clear();
    inline void push_back(const RenderObject& p, const TileMask& tileMask);
    // move object to front of queue
    inline void move_front(RenderObjectQueueObj *o);
    // move to free object stack
    inline void pop(RenderObjectQueueObj *o);
    // check if a model is scheduled to be loaded and the previous model
    // in the same nifFiles slot is no longer used (returns object pointer,
    // or NULL if none was found)
    RenderObjectQueueObj *findModelToLoad();
    // find an object ready to render, and move it to the front of the queue
    // returns pointer to the object found (NULL if none)
    RenderObjectQueueObj *findObject();
  };
  struct RenderThread
  {
    std::thread *t;
    std::string errMsg;
    TerrainMesh *terrainMesh;
    Plot3D_TriShape *renderer;
    std::vector< unsigned char >  fileBuf;
    std::vector< Renderer_Base::TriShapeSortObject >  sortBuf;
    RenderThread();
    ~RenderThread();
    void join();
    void clear();
  };
  std::uint32_t *outBufRGBA;
  float   *outBufZ;
  int     width;
  int     height;
  const BA2File&  ba2File;
  ESMFile&  esmFile;
  NIFFile::NIFVertexTransform viewTransform;
  float   lightX;
  float   lightY;
  float   lightZ;
  int     textureMip;
  float   landTextureMip;
  float   landTxtRGBScale;
  std::uint32_t landTxtDefColor;
  LandscapeData *landData;
  int     cellTextureResolution;
  float   defaultWaterLevel;
  unsigned char modelLOD;               // 0 (maximum detail) to 4
  bool    enableMarkers;
  bool    distantObjectsOnly;           // ignore if not visible from distance
  bool    noDisabledObjects;            // ignore if initially disabled
  bool    enableDecals;
  bool    enableSCOL;
  bool    enableAllObjects;
  bool    enableTextures;
  // 0: diffuse only, 1: normal mapping, 2: PBR on objects only, 3: full PBR
  unsigned char renderQuality;
  // 4: Skyrim or older, 8: Fallout 4, 12: Fallout 76
  unsigned char renderMode;
  unsigned char debugMode;
  // 1: terrain, 2: objects, 4: objects with alpha blending
  unsigned char renderPass;
  bool    ignoreOBND;
  unsigned char threadCnt;
  unsigned char modelBatchCnt;
  TextureCache  textureCache;
  LandscapeTextureSet *landTextures;
  size_t  objectListPos;
  unsigned int  modelIDBase;
  std::vector< RenderObject > objectList;
  std::vector< std::string >  excludeModelPatterns;
  std::vector< std::string >  hdModelNamePatterns;
  std::string defaultEnvMap;
  std::string defaultWaterTexture;
  std::vector< ModelData >    nifFiles;
  MaterialSwaps materialSwaps;
  std::vector< RenderThread > renderThreads;
  RenderObjectQueue *renderObjectQueue;
  std::string stringBuf;
  float   waterReflectionLevel;
  int     zRangeMax;
  // 0: default, 1: disable built-in exclude patterns, 2 or 3: disable effects
  unsigned char effectMeshMode;
  bool    enableActors;
  // 0: disable water, 1: default color, -1: use water parameters from the ESM
  signed char   waterRenderMode;
  unsigned char bufAllocFlags;          // bit 0: RGBA buffer, bit 1: Z buffer
  NIFFile::NIFBounds  worldBounds;
  std::vector< std::int32_t > baseObjects;      // baseObjHashMask + 1 elements
  std::vector< std::vector< BaseObject > >  baseObjectBufs;
  DDSTexture  whiteTexture;
  // for TXST and WATR objects, materials[0] is the default water
  std::map< unsigned int, BGSMFile >  materials;
  std::uint32_t *outBufN;               // normals for decal rendering
  // returns false if the object is not visible
  bool setScreenAreaUsed(RenderObject& p);
  unsigned int getDefaultWorldID() const;
  void addTerrainCell(const ESMFile::ESMRecord& r);
  void addWaterCell(const ESMFile::ESMRecord& r);
  bool getNPCModel(BaseObject& p, const ESMFile::ESMRecord& r);
  void readDecalProperties(BaseObject& p, const ESMFile::ESMRecord& r);
  // returns NULL on excluded model or invalid object
  const BaseObject *readModelProperties(RenderObject& p,
                                        const ESMFile::ESMRecord& r);
  void addSCOLObjects(const ESMFile::ESMRecord& r,      // SCOL
                      const NIFFile::NIFVertexTransform& vt,
                      unsigned int refrFormID, unsigned int refrMSWPFormID);
  // type = 0: terrain, type = 1: objects
  void findObjects(unsigned int formID, int type, bool isRecursive);
  void findObjects(unsigned int formID, int type);
  void sortObjectList();
  // 0x0001: clear image data
  // 0x0002: clear Z buffer
  // 0x0004: clear landscape data
  // 0x0008: clear object list and model paths
  // 0x0010: clear threads
  // 0x0020: clear model cache
  // 0x0040: clear texture cache
  void clear(unsigned int flags);
  bool isExcludedModel(const std::string& modelPath) const;
  bool isHighQualityModel(const std::string& modelPath) const;
  bool loadModel(const BaseObject& o, size_t threadNum);
  static inline float getDecalYOffsetMin(FloatVector4 boundsMin)
  {
    (void) boundsMin;
    return 0.0f;
  }
  static inline float getDecalYOffsetMax(FloatVector4 boundsMax)
  {
    (void) boundsMax;
    return 1000.0f;
  }
  void renderDecal(RenderThread& t, const RenderObject& p);
  void renderObject(RenderThread& t, const RenderObject& p);
  void renderThread(size_t threadNum);
  static void threadFunction(Renderer *p, size_t threadNum);
 public:
  Renderer(int imageWidth, int imageHeight,
           const BA2File& archiveFiles, ESMFile& masterFiles,
           std::uint32_t *bufRGBA = 0, float *bufZ = 0, int zMax = 16777216);
  virtual ~Renderer();
  // returns 0 if formID is not in an exterior world, 0xFFFFFFFF on error
  static unsigned int findParentWorld(ESMFile& esmFile, unsigned int formID);
  inline const std::uint32_t *getImageData() const
  {
    return outBufRGBA;
  }
  inline float *getZBufferData()
  {
    return outBufZ;
  }
  inline int getWidth() const
  {
    return width;
  }
  inline int getHeight() const
  {
    return height;
  }
  void clear();
  // flags = 1: clear RGBA buffer only
  // flags = 2: clear depth and normal buffer only
  void clearImage(unsigned int flags = 3U);
  inline void clearTextureCache()
  {
    textureCache.clear();
  }
  inline void clearObjectPropertyCache()
  {
    baseObjects.clear();
    baseObjectBufs.clear();
  }
  inline void clearExcludeModelPatterns()
  {
    excludeModelPatterns.clear();
  }
  void deallocateBuffers(unsigned int mask);    // mask & 1: RGBA, 2: depth
  // rotations are in radians
  void setViewTransform(float scale,
                        float rotationX, float rotationY, float rotationZ,
                        float offsetX, float offsetY, float offsetZ);
  void setLightDirection(float rotationY, float rotationZ);
  // default to std::thread::hardware_concurrency() if n <= 0
  void setThreadCount(int n);
  void setTextureCacheSize(std::uint64_t n)
  {
    if (sizeof(size_t) < 8)
      n = std::min(n, std::uint64_t(0xFFFFFFFFU));
    textureCache.textureCacheSize = size_t(n);
  }
  // set the number of models to load at once (1 to 64)
  void setModelCacheSize(int n);
  void setTextureMipLevel(int n)
  {
    textureMip = n;             // base mip level for all textures
  }
  void setLandTextureMip(float n)
  {
    landTextureMip = n;         // additional mip level for landscape textures
  }
  void setLandTxtRGBScale(float n)
  {
    landTxtRGBScale = n;
  }
  void setLandDefaultColor(std::uint32_t n)
  {
    landTxtDefColor = n;        // 0x00RRGGBB
  }
  void setLandTxtResolution(int n)
  {
    cellTextureResolution = n;  // must be power of two and >= cell resolution
  }
  // 0: diffuse only, 4: normal mapping, 8: PBR on objects only, 12: full PBR
  // + 2: render references to any object type
  // + 1: do not split pre-combined meshes
  // + 16: enable actors (experimental)
  // + 32: enable TXST decals
  // + 64: enable marker objects
  // + 128: disable built-in exclude patterns for effect meshes
  // + 256: disable effect meshes
  // + 512: ignore object bounds
  void setRenderQuality(unsigned int n)
  {
    enableMarkers = bool(n & 0x40);
    enableDecals = bool(n & 0x20);
    enableSCOL |= bool(n & 1);
    enableAllObjects |= bool(n & 2);
    renderQuality = (unsigned char) ((n >> 2) & 3);
    ignoreOBND = bool(n & 0x0200);
    effectMeshMode = (unsigned char) ((n >> 7) & 3);
    enableActors = bool(n & 0x10);
  }
  void setModelLOD(int n)
  {
    modelLOD = (unsigned char) n;       // 0 (maximum detail) to 4
  }
  void setDistantObjectsOnly(bool n)
  {
    distantObjectsOnly = n;     // ignore if not visible from distance
  }
  void setNoDisabledObjects(bool n)
  {
    noDisabledObjects = n;      // ignore if initially disabled
  }
  void setEnableSCOL(bool n)
  {
    enableSCOL = n;             // do not split pre-combined meshes
  }
  void setEnableAllObjects(bool n)
  {
    enableAllObjects = n;       // render references to any object type
  }
  void setEnableTextures(bool n)
  {
    enableTextures = n;         // if false, make all diffuse textures white
  }
  void setDebugMode(unsigned char n)
  {
    // n = 0: default mode (disable debug)
    // n = 1: render form ID as a solid color (0x00RRGGBB)
    // n = 2: Z * 16 (blue = LSB, red = MSB)
    // n = 3: normal map
    // n = 4: disable lighting and reflections
    // n = 5: lighting only (white texture)
    debugMode = n;
  }
  // models with path name including s are not rendered
  void addExcludeModelPattern(const std::string& s);
  // models with path name including s are rendered at full quality
  // implies enabling normal maps (setRenderQuality() >= 4)
  void addHDModelPattern(const std::string& s);
  // default environment map texture path
  void setDefaultEnvMap(const std::string& s);
  // water normal map texture path
  void setWaterTexture(const std::string& s);
  // 0xAARRGGBB, -1 to use ESM water parameters, 0 to disable water
  void setWaterColor(std::uint32_t n);
  void setWaterEnvMapScale(float n)
  {
    waterReflectionLevel = n;
  }
  // colors are in 0xRRGGBB format, ambientColor < 0 takes the color from
  // the default environment map
  void setRenderParameters(int lightColor, int ambientColor, int envColor,
                           float lightLevel, float envLevel, float rgbScale,
                           float reflZScale = 2.0f, int waterUVScale = 2048);
  void loadTerrain(const char *btdFileName = (char *) 0,
                   unsigned int worldID = 0U, unsigned int defTxtID = 0U,
                   int mipLevel = 2, int xMin = -32768, int yMin = -32768,
                   int xMax = 32767, int yMax = 32767);
  // n = 0: terrain (exterior cells only, loadTerrain() must be called first)
  // n = 1: objects
  // n = 2: water and objects with alpha blending
  void initRenderPass(int n, unsigned int formID = 0U);
  // Returns true if all objects have been rendered.
  // If t is greater than zero, the function may return false after at least
  // the specified number of milliseconds.
  bool renderObjects(int t = 0);
  // return value <= getObjectCount()
  size_t getObjectsRendered();
  inline size_t getObjectCount() const
  {
    return objectList.size();
  }
  inline const NIFFile::NIFBounds& getBounds() const
  {
    return worldBounds;
  }
  inline void resetBounds()
  {
    worldBounds = NIFFile::NIFBounds();
  }
};

#endif

