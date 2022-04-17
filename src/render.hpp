
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
  // maximum number of NIF files to keep loaded (must be power of two)
  static const unsigned int modelBatchCnt = 8U;
  struct RenderObject
  {
    unsigned short  flags;      // 1: terrain, 2: solid object, 4: water
    // -1:        not rendered
    // 0 to 63:   single tile, N = Y * 8 + X
    // 64 to 319:   2*2 tiles, N = 64 + ((Y0 & 6) * 8) + (X0 & 6)
    //                             + ((Y0 & 1) * 128) + ((X0 & 1) * 64)
    // 320 to 1343: 4*4 tiles, N = 320 + ((Y0 & 4) * 8) + (X0 & 4)
    //                             + ((Y0 & 3) * 256) + ((X0 & 3) * 64)
    // 1344:        8*8 tiles
    signed short  tileIndex;
    unsigned int  modelID;      // NIF file number
    unsigned int  formID;       // form ID of reference
    unsigned int  mswpFormID;   // material swap form ID if non-zero
    NIFFile::NIFVertexTransform modelTransform;
    signed short  x0;           // object bounds or terrain area
    signed short  y0;
    signed short  z0;
    signed short  x1;
    signed short  y1;
    signed short  z1;
    bool operator<(const RenderObject& r) const;
  };
  struct CachedTexture
  {
    DDSTexture    *texture;
    std::map< std::string, CachedTexture >::iterator  i;
    CachedTexture *prv;
    CachedTexture *nxt;
  };
  struct ModelData
  {
    NIFFile *nifFile;
    size_t  totalTriangleCnt;
    std::vector< NIFFile::NIFTriShape > meshData;
    bool    isHDModel;
    ModelData();
    ~ModelData();
    void clear();
  };
  struct MaterialSwap
  {
    std::string materialPath;
    BGSMFile    bgsmFile;
    std::vector< std::string >  texturePaths;
  };
  struct RenderThread
  {
    std::thread *t;
    std::string errMsg;
    TerrainMesh *terrainMesh;
    Plot3D_TriShape *renderer;
    // objects not rendered due to incorrect bounds
    std::vector< unsigned int > objectsRemaining;
    RenderThread();
    ~RenderThread();
    void join();
    void clear();
  };
  unsigned int  *outBufRGBW;
  float   *outBufZ;
  int     width;
  int     height;
  const BA2File&  ba2File;
  ESMFile&  esmFile;
  NIFFile::NIFVertexTransform viewTransform;
  float   lightX;
  float   lightY;
  float   lightZ;
  float   lightingPolynomial[6];
  int     textureMip;
  float   landTextureMip;
  float   landTxtRGBScale;
  unsigned int  landTxtDefColor;
  LandscapeData *landData;
  int     cellTextureResolution;
  float   defaultWaterLevel;
  int     modelLOD;                     // 0 (maximum detail) to 4
  bool    distantObjectsOnly;           // ignore if not visible from distance
  bool    noDisabledObjects;            // ignore if initially disabled
  bool    enableSCOL;
  bool    enableTextures;
  bool    bufRGBWAllocFlag;
  bool    bufZAllocFlag;
  int     threadCnt;
  size_t  textureDataSize;
  size_t  textureCacheSize;
  CachedTexture *firstTexture;
  CachedTexture *lastTexture;
  std::mutex  textureCacheMutex;
  std::map< std::string, CachedTexture >  textureCache;
  std::vector< const DDSTexture * > landTextures;
  std::vector< RenderObject > objectList;
  std::map< std::string, unsigned int > modelPathMap;
  std::vector< const std::string * >  modelPaths;
  std::vector< std::string >  excludeModelPatterns;
  std::vector< std::string >  hdModelNamePatterns;
  std::string defaultEnvMap;
  std::string defaultWaterTexture;
  std::vector< ModelData >    nifFiles;
  std::map< unsigned int, std::vector< MaterialSwap > > materialSwaps;
  std::vector< RenderThread > renderThreads;
  std::string stringBuf;
  std::vector< unsigned char >  fileBuf;
  unsigned int  waterColor;
  bool    verboseMode;
  NIFFile::NIFBounds  worldBounds;
  std::string whiteTexturePath;
  // bit Y * 8 + X of the return value is set if the bounds of the object
  // overlap with tile (X, Y) of the screen, using 8*8 tiles and (0, 0)
  // in the top left corner
  unsigned long long calculateTileMask(int x0, int y0, int x1, int y1) const;
  static signed short calculateTileIndex(unsigned long long screenAreasUsed);
  int setScreenAreaUsed(RenderObject& p);
  unsigned int getDefaultWorldID() const;
  void addTerrainCell(const ESMFile::ESMRecord& r);
  void addWaterCell(const ESMFile::ESMRecord& r);
  void addSCOLObjects(const ESMFile::ESMRecord& r,      // REFR
                      const ESMFile::ESMRecord& r2,     // SCOL
                      float scale, float rX, float rY, float rZ,
                      float offsX, float offsY, float offsZ);
  // type = 0: terrain, type = 1: solid objects, type = 2: water
  void findObjects(unsigned int formID, int type, unsigned int parentID = 0U);
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
  const DDSTexture *loadTexture(const std::string& fileName);
  void shrinkTextureCache();
  bool isExcludedModel(const std::string& modelPath) const;
  bool isHighQualityModel(const std::string& modelPath) const;
  void loadModel(unsigned int modelID);
  unsigned int loadMaterialSwap(unsigned int formID);
  void renderObjectList();
  void materialSwap(Plot3D_TriShape& t,
                    const std::string **texturePaths, size_t texturePathCnt,
                    unsigned int formID);
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
           unsigned int *bufRGBW = 0, float *bufZ = 0);
  virtual ~Renderer();
  // returns 0 if formID is not in an exterior world, 0xFFFFFFFF on error
  static unsigned int findParentWorld(ESMFile& esmFile, unsigned int formID);
  inline const unsigned int *getImageData() const
  {
    return outBufRGBW;
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
  // rotations are in radians
  void setViewTransform(float scale,
                        float rotationX, float rotationY, float rotationZ,
                        float offsetX, float offsetY, float offsetZ);
  void setLightDirection(float rotationX, float rotationY);
  // set polynomial a[0..5] for mapping dot product (-1.0 to 1.0)
  // to RGB multiplier
  void setLightingFunction(const float *a);
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
  void setLandDefaultColor(unsigned int n)
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
  void setEnableTextures(bool n)
  {
    enableTextures = n;         // if false, make all diffuse textures white
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
  void setWaterColor(unsigned int n)
  {
    waterColor = (n & 0xFF00FF00U)
                 | ((n & 0x000000FFU) << 16) | ((n & 0x00FF0000U) >> 16);
  }
  // disable printing messages if n is false
  void setVerboseMode(bool n)
  {
    verboseMode = n;
  }
  void loadTerrain(const char *btdFileName = (char *) 0,
                   unsigned int worldID = 0U, unsigned int defTxtID = 0U,
                   int mipLevel = 2, int xMin = -32768, int yMin = -32768,
                   int xMax = 32767, int yMax = 32767);
  void renderTerrain(unsigned int worldID = 0U);
  void renderObjects(unsigned int formID = 0U);
  void renderWater(unsigned int formID = 0U);
  inline const NIFFile::NIFBounds& getBounds() const
  {
    return worldBounds;
  }
};

#endif

