
#ifndef NIF_VIEW_HPP_INCLUDED
#define NIF_VIEW_HPP_INCLUDED

#include "common.hpp"
#include "nif_file.hpp"
#include "ba2file.hpp"
#include "ddstxt.hpp"
#include "plot3d.hpp"
#include "sdlvideo.hpp"

#include <thread>
#include <mutex>

class Renderer
{
 protected:
  struct TriShapeSortObject
  {
    const NIFFile::NIFTriShape  *ts;
    double  z;
    inline bool operator<(const TriShapeSortObject& r) const
    {
      return (z < r.z);
    }
  };
  std::vector< Plot3D_TriShape * >  renderers;
  std::vector< std::string >  threadErrMsg;
  std::vector< int >  viewOffsetY;
  std::vector< NIFFile::NIFTriShape > meshData;
  std::map< std::string, DDSTexture * > textureSet;
  std::mutex  textureSetMutex;
  NIFFile::NIFVertexTransform modelTransform;
  NIFFile::NIFVertexTransform viewTransform;
  float   lightX;
  float   lightY;
  float   lightZ;
  float   waterEnvMapLevel;
  std::uint32_t waterColor;
  int     threadCnt;
  const BA2File *ba2File;
  std::vector< unsigned char >  fileBuf;
  std::string defaultEnvMap;
  std::string waterTexture;
  DDSTexture  whiteTexture;
 public:
  Renderer(std::uint32_t *outBufRGBA, float *outBufZ,
           int imageWidth, int imageHeight, unsigned int nifVersion);
  virtual ~Renderer();
  void setBuffers(std::uint32_t *outBufRGBA, float *outBufZ,
                  int imageWidth, int imageHeight, float envMapScale);
  const DDSTexture *loadTexture(const std::string *texturePath);
  static void threadFunction(Renderer *p, size_t n);
  void renderModel();
  static void renderMeshToFile(const char *outFileName,
                               const NIFFile& nifFile, const BA2File& ba2File,
                               int imageWidth, int imageHeight);
  static void viewMeshes(SDLDisplay& display, const BA2File& ba2File,
                         const std::vector< std::string >& nifFileNames);
};

#endif

