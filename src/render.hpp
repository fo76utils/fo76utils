
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

#include <thread>
#include <mutex>

class Renderer
{
 protected:
  // maximum number of NIF files to keep loaded (must be power of two and <= 64)
  static const unsigned int modelBatchCnt = 16U;
  struct BaseObject
  {
    unsigned short  type;               // first 2 characters, or 0 if excluded
    unsigned short  flags;              // same as RenderObject flags
    unsigned int  modelID;
    unsigned int  mswpFormID;
    signed short  obndX0;
    signed short  obndY0;
    signed short  obndZ0;
    signed short  obndX1;
    signed short  obndY1;
    signed short  obndZ1;
    std::string modelPath;
  };
  struct RenderObject
  {
    // 1: terrain, 2: solid object, 4: water cell, 6: water mesh
    // 0x08: uses effect shader
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
    // for water (flags & 4 set): water color in 0xAABBGGRR format
    unsigned int  mswpFormID;
    NIFFile::NIFVertexTransform modelTransform;
    bool operator<(const RenderObject& r) const;
  };
  struct CachedTexture
  {
    DDSTexture    *texture;
    std::map< std::string, CachedTexture >::iterator  i;
    CachedTexture *prv;
    CachedTexture *nxt;
    std::mutex    *textureLoadMutex;
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
  struct MaterialSwap
  {
    std::string materialPath;
    BGSMFile    bgsmFile;
    std::vector< std::string >  texturePaths;
    const std::string *texturePathPtrs[10];
  };
  struct RenderThread
  {
    struct TriShapeSortObject
    {
      NIFFile::NIFTriShape  *ts;
      double  z;
      inline bool operator<(const TriShapeSortObject& r) const
      {
        return (z < r.z);
      }
    };
    std::thread *t;
    std::string errMsg;
    TerrainMesh *terrainMesh;
    Plot3D_TriShape *renderer;
    // objects not rendered due to incorrect bounds
    std::vector< unsigned int > objectsRemaining;
    std::vector< unsigned char >  fileBuf;
    std::vector< TriShapeSortObject > sortBuf;
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
  int     modelLOD;                     // 0 (maximum detail) to 4
  bool    distantObjectsOnly;           // ignore if not visible from distance
  bool    noDisabledObjects;            // ignore if initially disabled
  bool    enableSCOL;
  bool    enableAllObjects;
  bool    enableTextures;
  unsigned char renderMode;
  unsigned char debugMode;
  // 1: terrain, 2: objects, 4: effects
  unsigned char renderPass;
  int     threadCnt;
  size_t  textureDataSize;
  size_t  textureCacheSize;
  CachedTexture *firstTexture;
  CachedTexture *lastTexture;
  std::mutex  textureCacheMutex;
  std::map< std::string, CachedTexture >  textureCache;
  std::vector< const DDSTexture * > landTextures;
  std::vector< const DDSTexture * > landTexturesN;
  std::vector< RenderObject > objectList;
  std::vector< std::string >  excludeModelPatterns;
  std::vector< std::string >  hdModelNamePatterns;
  std::string defaultEnvMap;
  std::string defaultWaterTexture;
  std::vector< ModelData >    nifFiles;
  std::map< unsigned int, std::vector< MaterialSwap > > materialSwaps;
  std::vector< RenderThread > renderThreads;
  std::string stringBuf;
  std::uint32_t waterColor;
  float   waterReflectionLevel;
  int     zRangeMax;
  bool    verboseMode;
  bool    useESMWaterColors;
  unsigned char bufAllocFlags;          // bit 0: RGBA buffer, bit 1: Z buffer
  NIFFile::NIFBounds  worldBounds;
  std::map< unsigned int, BaseObject >  baseObjects;
  DDSTexture  whiteTexture;
  // bit Y * 8 + X of the return value is set if the bounds of the object
  // overlap with tile (X, Y) of the screen, using 8*8 tiles and (0, 0)
  // in the top left corner
  unsigned long long calculateTileMask(int x0, int y0, int x1, int y1) const;
  static signed short calculateTileIndex(unsigned long long screenAreasUsed);
  int setScreenAreaUsed(RenderObject& p);
  unsigned int getDefaultWorldID() const;
  void addTerrainCell(const ESMFile::ESMRecord& r);
  void addWaterCell(const ESMFile::ESMRecord& r);
  void getWaterColor(RenderObject& p, const ESMFile::ESMRecord& r);
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
  static size_t getTextureDataSize(const DDSTexture *t);
  // returns NULL on failure
  // *waitFlag is set to true if the texture is locked by another thread
  // m = mip level (< 0: use default)
  const DDSTexture *loadTexture(const std::string& fileName,
                                std::vector< unsigned char >& fileBuf,
                                int m = -1, bool *waitFlag = (bool *) 0);
  void shrinkTextureCache();
  bool isExcludedModel(const std::string& modelPath) const;
  bool isHighQualityModel(const std::string& modelPath) const;
  void loadModels(unsigned int t, unsigned long long modelIDMask);
  static void loadModelsThread(Renderer *p,
                               unsigned int t, unsigned long long modelIDMask);
  unsigned int loadMaterialSwap(unsigned int formID);
  void renderObjectList();
  void materialSwap(Plot3D_TriShape& t, unsigned int formID);
  bool renderObject(RenderThread& t, size_t i,
                    unsigned long long tileMask = ~0ULL);
  void renderThread(size_t threadNum, size_t startPos, size_t endPos,
                    unsigned long long tileIndexMask);
  static void threadFunction(Renderer *p, size_t threadNum,
                             size_t startPos, size_t endPos,
                             unsigned long long tileIndexMask);
  static inline std::uint32_t bgraToRGBA(std::uint32_t c)
  {
    return ((c & 0xFF00FF00U) | ((c & 0xFFU) << 16) | ((c >> 16) & 0xFFU));
  }
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
  inline const float *getZBufferData() const
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
    textureCacheSize = n;
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
  void setModelLOD(int n)
  {
    modelLOD = n;               // 0 (maximum detail) to 4
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
  void addHDModelPattern(const std::string& s);
  // default environment map texture path
  void setDefaultEnvMap(const std::string& s);
  // water normal map texture path
  void setWaterTexture(const std::string& s);
  // 0xAARRGGBB
  void setWaterColor(std::uint32_t n)
  {
    useESMWaterColors = (n == 0xFFFFFFFFU);
    waterColor = bgraToRGBA(n);
  }
  void setWaterEnvMapScale(float n)
  {
    waterReflectionLevel = n;
  }
  // disable printing messages if n is false
  void setVerboseMode(bool n)
  {
    verboseMode = n;
  }
  // colors are in 0xRRGGBB format, ambientColor < 0 takes the color from
  // the default environment map
  void setLighting(int lightColor, int ambientColor, int envColor,
                   float lightLevel, float envLevel, float rgbScale);
  void loadTerrain(const char *btdFileName = (char *) 0,
                   unsigned int worldID = 0U, unsigned int defTxtID = 0U,
                   int mipLevel = 2, int xMin = -32768, int yMin = -32768,
                   int xMax = 32767, int yMax = 32767);
  void renderTerrain(unsigned int worldID = 0U);
  void renderObjects(unsigned int formID = 0U);
  inline const NIFFile::NIFBounds& getBounds() const
  {
    return worldBounds;
  }
};

#endif

