
#ifndef NIF_VIEW_HPP_INCLUDED
#define NIF_VIEW_HPP_INCLUDED

#include "common.hpp"
#include "ba2file.hpp"
#include "bgsmfile.hpp"
#include "nif_file.hpp"
#include "ddstxt.hpp"
#include "plot3d.hpp"
#include "rndrbase.hpp"
#include "sdlvideo.hpp"

#include <thread>
#include <mutex>

class NIF_View : protected Renderer_Base
{
 public:
  static const char *materialFlagNames[32];
 protected:
  const BA2File&  ba2File;
  ESMFile *esmFile;
  std::vector< Plot3D_TriShape * >  renderers;
  std::vector< std::string >  threadErrMsg;
  std::vector< int >  viewOffsetY;
  std::vector< NIFFile::NIFTriShape > meshData;
  TextureCache  textureSet;
  std::vector< BA2File::UCharArray >  threadFileBuffers;
  int     threadCnt;
  float   lightX, lightY, lightZ;
  NIFFile *nifFile;
  NIFFile::NIFVertexTransform modelTransform;
  NIFFile::NIFVertexTransform viewTransform;
  // MSWP form ID or (unsigned int) -1 - (gradientMapV * 16777216.0)
  unsigned int  materialSwapTable[8];
  MaterialSwaps materialSwaps;
  DDSTexture  defaultTexture;
  static void threadFunction(NIF_View *p, size_t n);
  const DDSTexture *loadTexture(const std::string& texturePath,
                                size_t threadNum = 0);
  void setDefaultTextures(int envMapNum = 0);
 public:
  std::string defaultEnvMap;
  std::string waterTexture;
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
  bool    enableHidden;
  int     debugMode;            // 0 to 5
  std::map< unsigned int, BGSMFile >  waterMaterials;
  NIF_View(const BA2File& archiveFiles, ESMFile *esmFilePtr = (ESMFile *) 0);
  virtual ~NIF_View();
  void loadModel(const std::string& fileName);
  void renderModel(std::uint32_t *outBufRGBA, float *outBufZ,
                   int imageWidth, int imageHeight);
  void addMaterialSwap(unsigned int formID);
  inline void addColorSwap(float gradientMapV)
  {
    float   tmp = std::min(std::max(gradientMapV, 0.0f), 1.0f) * 16777216.0f;
    addMaterialSwap(~((unsigned int) roundFloat(tmp)));
  }
  void clearMaterialSwaps();
  void setWaterColor(unsigned int watrFormID);
  void renderModelToFile(const char *outFileName,
                         int imageWidth, int imageHeight);
  // returns false if the window is closed
  bool viewModels(SDLDisplay& display,
                  const std::vector< std::string >& nifFileNames);
};

#endif

