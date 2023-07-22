
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

#ifndef ENABLE_TXST_DECALS
#  define ENABLE_TXST_DECALS  1
#endif

class Renderer : protected Renderer_Base
{
 protected:
  // maximum number of NIF files to keep loaded (must be power of two and <= 64)
  static const unsigned int modelBatchCnt = 16U;
  struct BaseObject
  {
    unsigned short  type;               // first 2 characters, or 0 if excluded
    unsigned short  flags;              // same as RenderObject flags
    unsigned int  modelID;
    unsigned int  mswpFormID;           // TXST form ID for decals
    signed short  obndX0;
    signed short  obndY0;
    signed short  obndZ0;
    signed short  obndX1;
    signed short  obndY1;
    signed short  obndZ1;
    std::string modelPath;              // "~0x%08X" % formID for decals
  };
  struct RenderObject
  {
    // 1: terrain, 2: solid object, 4: water cell, 6: water mesh
    // 0x08: uses alpha blending
    // 0x10: is decal (TXST object)
    // 0x20: is marker
    // 0x40: high quality model
    // 0x80: upper byte is gradient map V from MODC
    unsigned short  flags;
    // -1:        not rendered
    // 0 to 63:   single tile, N = Y * 8 + X
    // 64 to 319:   2*2 tiles, N = 64 + ((Y0 & 6) * 8) + (X0 & 6)
    //                             + ((Y0 & 1) * 128) + ((X0 & 1) * 64)
    // 320 to 1343: 4*4 tiles, N = 320 + ((Y0 & 4) * 8) + (X0 & 4)
    //                             + ((Y0 & 3) * 256) + ((X0 & 3) * 64)
    // 1344:        8*8 tiles
    signed short  tileIndex;
    int     z;                  // Z coordinate for sorting / debugMode 1 formID
    union
    {
      const BaseObject  *o;     // valid if flags & 2 is set
      struct
      {
        signed short  x0;       // terrain area, or water cell bounds
        signed short  y0;
        signed short  x1;
        signed short  y1;
      }
      t;
    }
    model;
    // for solid objects: material swap form ID if non-zero
    // for water and decals: form ID of base object (WATR or TXST)
    unsigned int  mswpFormID;
    NIFFile::NIFVertexTransform modelTransform;
    bool operator<(const RenderObject& r) const;
  };
  struct ModelData
  {
    NIFFile *nifFile;
    size_t  totalTriangleCnt;
    std::vector< NIFFile::NIFTriShape > meshData;
    const BaseObject  *o;
    std::thread *loadThread;
    std::string threadErrMsg;
    std::vector< unsigned char >  fileBuf;
    ModelData();
    ~ModelData();
    void clear();
  };
  struct RenderThread
  {
    std::thread *t;
    std::string errMsg;
    TerrainMesh *terrainMesh;
    Plot3D_TriShape *renderer;
    // objects not rendered due to incorrect bounds
    std::vector< unsigned int > objectsRemaining;
    std::vector< unsigned char >  fileBuf;
    std::vector< Renderer_Base::TriShapeSortObject >  sortBuf;
    RenderThread();
    ~RenderThread();
    void join();
    void clear();
  };
  struct ThreadSortObject
  {
    unsigned long long  tileMask;
    size_t  triangleCnt;
    inline ThreadSortObject();
    inline bool operator<(const ThreadSortObject& r) const;
    inline ThreadSortObject& operator+=(const ThreadSortObject& r);
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
  unsigned char renderMode;
  unsigned char debugMode;
  // 1: terrain, 2: objects, 4: objects with alpha blending
  unsigned char renderPass;
  int     threadCnt;
  TextureCache  textureCache;
  LandscapeTextureSet *landTextures;
  size_t  objectListPos;
  unsigned int  modelIDBase;
  std::vector< RenderObject > objectList;
  std::vector< ThreadSortObject > threadSortBuf;
  std::vector< std::string >  excludeModelPatterns;
  std::vector< std::string >  hdModelNamePatterns;
  std::string defaultEnvMap;
  std::string defaultWaterTexture;
  std::vector< ModelData >    nifFiles;
  MaterialSwaps materialSwaps;
  std::vector< RenderThread > renderThreads;
  std::string stringBuf;
  float   waterReflectionLevel;
  int     zRangeMax;
  bool    disableEffectMeshes;
  bool    disableEffectFilters;
  // 0: disable water, 1: default color, -1: use water parameters from the ESM
  signed char   waterRenderMode;
  unsigned char bufAllocFlags;          // bit 0: RGBA buffer, bit 1: Z buffer
  NIFFile::NIFBounds  worldBounds;
  std::map< unsigned int, BaseObject >  baseObjects;
  DDSTexture  whiteTexture;
  // for TXST and WATR objects, materials[0] is the default water
  std::map< unsigned int, BGSMFile >  materials;
  // bit Y * 8 + X of the return value is set if the bounds of the object
  // overlap with tile (X, Y) of the screen, using 8*8 tiles and (0, 0)
  // in the top left corner
  unsigned long long calculateTileMask(int x0, int y0, int x1, int y1) const;
  static signed short calculateTileIndex(unsigned long long screenAreasUsed);
  int setScreenAreaUsed(RenderObject& p);
  unsigned int getDefaultWorldID() const;
  void addTerrainCell(const ESMFile::ESMRecord& r);
  void addWaterCell(const ESMFile::ESMRecord& r);
  void readDecalProperties(BaseObject& p, const ESMFile::ESMRecord& r);
  // returns NULL on excluded model or invalid object
  const BaseObject *readModelProperties(RenderObject& p,
                                        const ESMFile::ESMRecord& r);
  void addSCOLObjects(const ESMFile::ESMRecord& r,      // SCOL
                      float scale, float rX, float rY, float rZ,
                      float offsX, float offsY, float offsZ,
                      unsigned int refrMSWPFormID);
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
  void loadModels(unsigned int t, unsigned long long modelIDMask);
  static void loadModelsThread(Renderer *p,
                               unsigned int t, unsigned long long modelIDMask);
  void renderObjectList();
  void renderDecal(RenderThread& t, const RenderObject& p);
  bool renderObject(RenderThread& t, size_t i,
                    unsigned long long tileMask = ~0ULL);
  void renderThread(size_t threadNum, size_t startPos, size_t endPos,
                    unsigned long long tileIndexMask);
  static void threadFunction(Renderer *p, size_t threadNum,
                             size_t startPos, size_t endPos,
                             unsigned long long tileIndexMask);
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
  void clearImage();
  inline void clearTextureCache()
  {
    textureCache.clear();
  }
  inline void clearObjectPropertyCache()
  {
    baseObjects.clear();
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
  void setTextureCacheSize(size_t n)
  {
    textureCache.textureCacheSize = n;
  }
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
  // + 16: disable effect meshes
  // + 32: disable built-in exclude patterns for effect meshes
  // + 64: enable TXST decals
  // + 128: enable marker objects
  void setRenderQuality(unsigned char n)
  {
    enableMarkers = bool(n & 0x80);
#if ENABLE_TXST_DECALS
    enableDecals = bool(n & 0x40);
#endif
    enableSCOL |= bool(n & 1);
    enableAllObjects |= bool(n & 2);
    renderQuality = (n >> 2) & 3;
    disableEffectMeshes = bool(n & 0x10);
    disableEffectFilters = bool(n & 0x20);
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
  // returns true if all objects have been rendered
  bool renderObjects();
  inline size_t getObjectsRendered() const
  {
    return objectListPos;       // <= getObjectCount()
  }
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

