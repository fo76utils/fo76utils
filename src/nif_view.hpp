
#ifndef NIF_VIEW_HPP_INCLUDED
#define NIF_VIEW_HPP_INCLUDED

#include "common.hpp"
#include "ba2file.hpp"
#include "material.hpp"
#include "nif_file.hpp"
#include "ddstxt.hpp"
#include "plot3d.hpp"
#include "rndrbase.hpp"
#include "sdlvideo.hpp"
#include "bsmatcdb.hpp"

#include <thread>
#include <mutex>

class NIF_View : protected Renderer_Base
{
 protected:
  const BA2File&  ba2File;
  ESMFile *esmFile;
  std::vector< Plot3D_TriShape * >  renderers;
  std::vector< std::string >  threadErrMsg;
  std::vector< int >  viewOffsetY;
  std::vector< NIFFile::NIFTriShape > meshData;
  TextureCache  textureSet;
  std::vector< std::vector< unsigned char > > threadFileBuffers;
  int     threadCnt;
  float   lightX, lightY, lightZ;
  NIFFile *nifFile;
  NIFFile::NIFVertexTransform modelTransform;
  NIFFile::NIFVertexTransform viewTransform;
  DDSTexture  defaultTexture;
  std::string defaultEnvMap;
  std::string waterTexture;
  CE2MaterialDB materials;
  BSMaterialsCDB  *materialConverter;
  static void threadFunction(NIF_View *p, size_t n);
  const DDSTexture *loadTexture(const std::string& texturePath,
                                size_t threadNum = 0);
  void setDefaultTextures();
 public:
  float   modelRotationX, modelRotationY, modelRotationZ;
  float   viewRotationX, viewRotationY, viewRotationZ;
  float   viewOffsX, viewOffsY, viewOffsZ, viewScale;
  float   lightRotationY, lightRotationZ;
  FloatVector4  lightColor;
  FloatVector4  envColor;
  float   rgbScale;
  float   reflZScale;
  float   waterEnvMapLevel;
  unsigned int  waterFormID;
  int     defaultEnvMapNum;     // 0 to 7
  int     debugMode;            // 0 to 5
  std::map< unsigned int, WaterProperties > waterMaterials;
  NIF_View(const BA2File& archiveFiles, ESMFile *esmFilePtr = (ESMFile *) 0,
           const char *materialDBPath = (char *) 0, bool matViewMode = false);
  virtual ~NIF_View();
  void loadModel(const std::string& fileName, int l = 0);
  void renderModel(std::uint32_t *outBufRGBA, float *outBufZ,
                   int imageWidth, int imageHeight);
  void addMaterialSwap(unsigned int formID)
  {
    // TODO: implement material swapping
    (void) formID;
  }
  void clearMaterialSwaps()
  {
  }
  void setWaterColor(unsigned int watrFormID);
  void renderModelToFile(const char *outFileName,
                         int imageWidth, int imageHeight);
  // returns false if the window is closed
  bool viewModels(SDLDisplay& display,
                  const std::vector< std::string >& nifFileNames, int l = 0);
};

#endif

