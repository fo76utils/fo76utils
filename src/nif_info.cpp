
#include "common.hpp"
#include "nif_file.hpp"
#include "ba2file.hpp"
#include "ddstxt.hpp"
#include "plot3d.hpp"

#include <thread>
#include <mutex>
#include <algorithm>

#ifdef HAVE_SDL2
#  include <SDL2/SDL.h>
#endif

static void printAuthorName(std::FILE *f,
                            const std::vector< unsigned char >& fileBuf,
                            const char *nifFileName)
{
  if (fileBuf.size() < 57)
    return;
  if (std::memcmp(&(fileBuf.front()), "Gamebryo File Format, Version 20", 32)
      != 0)
  {
    return;
  }
  size_t  nameLen = fileBuf[56];
  if ((nameLen + 57) > fileBuf.size())
    throw errorMessage("%s: end of input file", nifFileName);
  std::string authorName;
  for (size_t i = 0; i < nameLen; i++)
  {
    char    c = char(fileBuf[i + 57]);
    if (!c)
      continue;
    if ((unsigned char) c < 0x20)
      c = ' ';
    authorName += c;
  }
  std::fprintf(f, "%s\t%s\t%lu\n",
               authorName.c_str(), nifFileName, (unsigned long) fileBuf.size());
}

static void printVertexTransform(std::FILE *f,
                                 const NIFFile::NIFVertexTransform& t)
{
  std::fprintf(f, "  NIFVertexTransform:\n");
  std::fprintf(f, "    Scale: %f\n", t.scale);
  std::fprintf(f, "    Rotation matrix: %8f, %8f, %8f\n",
               t.rotateXX, t.rotateXY, t.rotateXZ);
  std::fprintf(f, "                     %8f, %8f, %8f\n",
               t.rotateYX, t.rotateYY, t.rotateYZ);
  std::fprintf(f, "                     %8f, %8f, %8f\n",
               t.rotateZX, t.rotateZY, t.rotateZZ);
  std::fprintf(f, "    Offset: %f, %f, %f\n", t.offsX, t.offsY, t.offsZ);
}

static void printBlockList(std::FILE *f, const NIFFile& nifFile)
{
  std::fprintf(f, "BS version: 0x%08X\n", nifFile.getVersion());
  std::fprintf(f, "Author name: %s\n", nifFile.getAuthorName().c_str());
  std::fprintf(f, "Process script: %s\n",
               nifFile.getProcessScriptName().c_str());
  std::fprintf(f, "Export script: %s\n", nifFile.getExportScriptName().c_str());
  std::fprintf(f, "Block count: %u\n", (unsigned int) nifFile.getBlockCount());
  for (size_t i = 0; i < nifFile.getBlockCount(); i++)
  {
    std::fprintf(f, "  Block %3d: offset = 0x%08X, size = %7u, "
                 "name = \"%s\", type = %s\n",
                 int(i),
                 (unsigned int) nifFile.getBlockOffset(i),
                 (unsigned int) nifFile.getBlockSize(i),
                 nifFile.getBlockName(i), nifFile.getBlockTypeAsString(i));
    const NIFFile::NIFBlkNiNode *nodeBlock = nifFile.getNode(i);
    const NIFFile::NIFBlkBSTriShape *triShapeBlock = nifFile.getTriShape(i);
    const NIFFile::NIFBlkBSLightingShaderProperty *
        lspBlock = nifFile.getLightingShaderProperty(i);
    const NIFFile::NIFBlkBSShaderTextureSet *
        shaderTextureSetBlock = nifFile.getShaderTextureSet(i);
    const NIFFile::NIFBlkNiAlphaProperty *
        alphaPropertyBlock = nifFile.getAlphaProperty(i);
    if (nodeBlock)
    {
      if (nodeBlock->controller >= 0)
        std::fprintf(f, "    Controller: %3d\n", nodeBlock->controller);
      if (nodeBlock->collisionObject >= 0)
      {
        std::fprintf(f, "    Collision object: %3d\n",
                     nodeBlock->collisionObject);
      }
      if (nodeBlock->children.size() > 0)
      {
        std::fprintf(f, "    Children: ");
        for (size_t j = 0; j < nodeBlock->children.size(); j++)
          std::fprintf(f, "%s%3u", (!j ? "" : ", "), nodeBlock->children[j]);
        std::fprintf(f, "\n");
      }
    }
    else if (triShapeBlock)
    {
      if (triShapeBlock->controller >= 0)
        std::fprintf(f, "    Controller: %3d\n", triShapeBlock->controller);
      if (triShapeBlock->collisionObject >= 0)
      {
        std::fprintf(f, "    Collision object: %3d\n",
                     triShapeBlock->collisionObject);
      }
      if (triShapeBlock->skinID >= 0)
        std::fprintf(f, "    Skin: %3d\n", triShapeBlock->skinID);
      if (triShapeBlock->shaderProperty >= 0)
      {
        std::fprintf(f, "    Shader property: %3d\n",
                     triShapeBlock->shaderProperty);
      }
      if (triShapeBlock->alphaProperty >= 0)
      {
        std::fprintf(f, "    Alpha property: %3d\n",
                     triShapeBlock->alphaProperty);
      }
    }
    else if (lspBlock)
    {
      if (lspBlock->controller >= 0)
        std::fprintf(f, "    Controller: %3d\n", lspBlock->controller);
      if (lspBlock->textureSet >= 0)
        std::fprintf(f, "    Texture set: %3d\n", lspBlock->textureSet);
      if (lspBlock->bgsmTextures.size() > 0)
      {
        std::fprintf(f, "    Material version: 0x%02X\n",
                     (unsigned int) lspBlock->bgsmVersion);
        std::fprintf(f, "    Material flags: 0x%02X\n",
                     (unsigned int) lspBlock->bgsmFlags);
        std::fprintf(f, "    Material alpha flags: 0x%04X\n",
                     (unsigned int) lspBlock->bgsmAlphaFlags);
        std::fprintf(f, "    Material alpha threshold: %3u\n",
                     (unsigned int) lspBlock->bgsmAlphaThreshold);
      }
      std::fprintf(f, "    Material alpha: %.3f\n",
                   double(int(lspBlock->bgsmAlpha)) * (1.0 / 128.0));
      std::fprintf(f, "    Material gradient map scale: %.3f\n",
                   double(int(lspBlock->bgsmGradientMapV)) * (1.0 / 255.0));
      std::fprintf(f, "    Material environment map scale: %.3f\n",
                   double(int(lspBlock->bgsmEnvMapScale)) * (1.0 / 128.0));
      std::fprintf(f, "    Material specular color (0xAABBGGRR): 0x%08X\n",
                   lspBlock->bgsmSpecularColor);
      std::fprintf(f, "    Material specular scale: %.3f\n",
                   double(int(lspBlock->bgsmSpecularScale)) * (1.0 / 128.0));
      std::fprintf(f, "    Material specular smoothness: %.3f\n",
                   double(int(lspBlock->bgsmSpecularSmoothness))
                   * (1.0 / 255.0));
      for (size_t j = 0; j < lspBlock->bgsmTextures.size(); j++)
      {
        if (!lspBlock->bgsmTextures[j]->empty())
        {
          std::fprintf(f, "    Material texture %d: %s\n",
                       int(j), lspBlock->bgsmTextures[j]->c_str());
        }
      }
    }
    else if (shaderTextureSetBlock)
    {
      for (size_t j = 0; j < shaderTextureSetBlock->texturePaths.size(); j++)
      {
        if (!shaderTextureSetBlock->texturePaths[j]->empty())
        {
          std::fprintf(f, "    Texture %2d: %s\n",
                       int(j), shaderTextureSetBlock->texturePaths[j]->c_str());
        }
      }
    }
    else if (alphaPropertyBlock)
    {
      if (alphaPropertyBlock->controller >= 0)
      {
        std::fprintf(f, "    Controller: %3d\n",
                     alphaPropertyBlock->controller);
      }
      std::fprintf(f, "    Flags: 0x%04X\n",
                   (unsigned int) alphaPropertyBlock->flags);
      std::fprintf(f, "    Alpha threshold: %u\n",
                   (unsigned int) alphaPropertyBlock->alphaThreshold);
    }
  }
}

static void printMeshData(std::FILE *f, const NIFFile& nifFile)
{
  std::vector< NIFFile::NIFTriShape > meshData;
  nifFile.getMesh(meshData);
  for (size_t i = 0; i < meshData.size(); i++)
  {
    std::fprintf(f, "TriShape %3d (%s):\n", int(i), meshData[i].name);
    std::fprintf(f, "  Vertex count: %u\n", meshData[i].vertexCnt);
    std::fprintf(f, "  Triangle count: %u\n", meshData[i].triangleCnt);
    if (meshData[i].flags)
    {
      static const char *flagNames[8] =
      {
        "hidden", "is water", "is effect", "decal", "two sided", "tree",
        "has vertex colors", "uses glow map"
      };
      std::fprintf(f, "  Flags: ");
      unsigned char m = 0x01;
      unsigned char mPrv = 0x01;
      for (int j = 0; j < 8; j++, m = m << 1, mPrv = (mPrv << 1) | 1)
      {
        if ((meshData[i].flags & mPrv) > m)
          std::fprintf(f, ", ");
        if (meshData[i].flags & m)
          std::fprintf(f, "%s", flagNames[j]);
      }
      std::fputc('\n', f);
    }
    std::fprintf(f, "  Alpha threshold: %d\n", int(meshData[i].alphaThreshold));
    printVertexTransform(f, meshData[i].vertexTransform);
    if (meshData[i].materialPath)
      std::fprintf(f, "  Material: %s\n", meshData[i].materialPath->c_str());
    std::fprintf(f, "  Texture UV offset, scale: (%f, %f), (%f, %f)\n",
                 meshData[i].textureOffsetU, meshData[i].textureOffsetV,
                 meshData[i].textureScaleU, meshData[i].textureScaleV);
    for (size_t j = 0; j < meshData[i].texturePathCnt; j++)
    {
      std::fprintf(f, "  Texture %2d: %s\n",
                   int(j), meshData[i].texturePaths[j]->c_str());
    }
    std::fprintf(f, "  Vertex list:\n");
    for (size_t j = 0; j < meshData[i].vertexCnt; j++)
    {
      NIFFile::NIFVertex  v = meshData[i].vertexData[j];
      meshData[i].vertexTransform.transformXYZ(v.x, v.y, v.z);
      float   normalX, normalY, normalZ;
      v.getNormal(normalX, normalY, normalZ);
      meshData[i].vertexTransform.rotateXYZ(normalX, normalY, normalZ);
      std::fprintf(f, "    %4d: XYZ: (%f, %f, %f), normals: (%f, %f, %f), "
                   "UV: (%f, %f), color: 0x%08X\n",
                   int(j), v.x, v.y, v.z, normalX, normalY, normalZ,
                   v.getU(), v.getV(), v.vertexColor);
    }
    std::fprintf(f, "  Triangle list:\n");
    for (size_t j = 0; j < meshData[i].triangleCnt; j++)
    {
      std::fprintf(f, "    %4d: %4u, %4u, %4u\n",
                   int(j),
                   meshData[i].triangleData[j].v0,
                   meshData[i].triangleData[j].v1,
                   meshData[i].triangleData[j].v2);
    }
  }
}

static void printOBJData(std::FILE *f, const NIFFile& nifFile,
                         const char *mtlFileName)
{
  std::vector< NIFFile::NIFTriShape > meshData;
  nifFile.getMesh(meshData);
  unsigned int  vertexNumBase = 1;
  std::fprintf(f, "mtllib %s\n\n", mtlFileName);
  for (size_t i = 0; i < meshData.size(); i++)
  {
    std::fprintf(f, "# %s\n\ng %s\n", meshData[i].name, meshData[i].name);
    std::fprintf(f, "usemtl Material%06u\n\n", (unsigned int) (i + 1));
    for (size_t j = 0; j < meshData[i].vertexCnt; j++)
    {
      NIFFile::NIFVertex  v = meshData[i].vertexData[j];
      meshData[i].vertexTransform.transformXYZ(v.x, v.y, v.z);
      std::fprintf(f, "v %.8f %.8f %.8f\n", v.x, v.y, v.z);
    }
    for (size_t j = 0; j < meshData[i].vertexCnt; j++)
    {
      NIFFile::NIFVertex  v = meshData[i].vertexData[j];
      std::fprintf(f, "vt %.8f %.8f\n", v.getU(), 1.0 - v.getV());
    }
    for (size_t j = 0; j < meshData[i].vertexCnt; j++)
    {
      float   normalX, normalY, normalZ;
      meshData[i].vertexData[j].getNormal(normalX, normalY, normalZ);
      meshData[i].vertexTransform.rotateXYZ(normalX, normalY, normalZ);
      std::fprintf(f, "vn %.8f %.8f %.8f\n", normalX, normalY, normalZ);
    }
    for (size_t j = 0; j < meshData[i].triangleCnt; j++)
    {
      unsigned int  v0 = meshData[i].triangleData[j].v0 + vertexNumBase;
      unsigned int  v1 = meshData[i].triangleData[j].v1 + vertexNumBase;
      unsigned int  v2 = meshData[i].triangleData[j].v2 + vertexNumBase;
      std::fprintf(f, "f %u/%u/%u %u/%u/%u %u/%u/%u\n",
                   v0, v0, v0, v1, v1, v1, v2, v2, v2);
    }
    vertexNumBase = vertexNumBase + meshData[i].vertexCnt;
    std::fprintf(f, "\n");
  }
}

static void printMTLData(std::FILE *f, const NIFFile& nifFile)
{
  std::vector< NIFFile::NIFTriShape > meshData;
  nifFile.getMesh(meshData);
  for (size_t i = 0; i < meshData.size(); i++)
  {
    std::fprintf(f, "newmtl Material%06u\n", (unsigned int) (i + 1));
    std::fprintf(f, "Ka 1.0 1.0 1.0\n");
    std::fprintf(f, "Kd 1.0 1.0 1.0\n");
    float   specularR = float(int(meshData[i].specularColor & 0xFFU));
    float   specularG = float(int((meshData[i].specularColor >> 8) & 0xFFU));
    float   specularB = float(int((meshData[i].specularColor >> 16) & 0xFFU));
    float   specularScale =
        float(int(meshData[i].specularScale)) / (128.0f * 255.0f);
    float   specularGlossiness = float(int(meshData[i].specularSmoothness));
    if ((meshData[i].texturePathCnt >= 7 && meshData[i].texturePaths[6] &&
         !meshData[i].texturePaths[6]->empty()) ||
        (meshData[i].texturePathCnt >= 10 && meshData[i].texturePaths[9] &&
         !meshData[i].texturePaths[9]->empty()))
    {
      specularScale *= 0.5f;
      specularGlossiness *= 0.5f;
    }
    specularR *= specularScale;
    specularG *= specularScale;
    specularB *= specularScale;
    specularGlossiness =
        float(std::pow(2.0f, specularGlossiness * (9.0f / 255.0f) + 1.0f));
    std::fprintf(f, "Ks %.3f %.3f %.3f\n", specularR, specularG, specularB);
    std::fprintf(f, "d 1.0\n");
    std::fprintf(f, "Ns %.1f\n", specularGlossiness);
    for (size_t j = 0; j < 2; j++)
    {
      if (j < meshData[i].texturePathCnt &&
          !meshData[i].texturePaths[j]->empty())
      {
        std::fprintf(f, "%s %s\n",
                     (!j ? "map_Kd" : "map_Kn"),
                     meshData[i].texturePaths[j]->c_str());
      }
    }
    std::fprintf(f, "\n");
  }
}

static const char *cubeMapPaths[15] =
{
  "textures/cubemaps/chrome_e.dds",                             // Skyrim
  "textures/shared/cubemaps/mipblur_defaultoutside1.dds",       // Fallout 4
  "textures/shared/cubemaps/mipblur_defaultoutside1.dds",       // Fallout 76
  "textures/cubemaps/wrtemple_e.dds",
  "textures/shared/cubemaps/outsideoldtownreflectcube_e.dds",
  "textures/shared/cubemaps/outsideoldtownreflectcube_e.dds",
  "textures/cubemaps/cavegreencube_e.dds",
  "textures/shared/cubemaps/cgprewarstreet_e.dds",
  "textures/shared/cubemaps/swampcube.dds",
  "textures/cubemaps/quickskydark_e.dds",
  "textures/shared/cubemaps/metalchrome01cube_e.dds",
  "textures/shared/cubemaps/metalchrome01cube_e.dds",
  "textures/cubemaps/mghallcube_e.dds",
  "textures/shared/cubemaps/outsideday01.dds",
  "textures/shared/cubemaps/outsideday01.dds"
};

static float viewRotations[27] =
{
  54.73561f,  180.0f,     45.0f,        // isometric from NW
  54.73561f,  180.0f,     135.0f,       // isometric from SW
  54.73561f,  180.0f,     -135.0f,      // isometric from SE
  54.73561f,  180.0f,     -45.0f,       // isometric from NE
  180.0f,     0.0f,       0.0f,         // top
  -90.0f,     0.0f,       0.0f,         // front
  -90.0f,     0.0f,       90.0f,        // right
  -90.0f,     0.0f,       180.0f,       // back
  -90.0f,     0.0f,       -90.0f        // left
};

static inline float degreesToRadians(float x)
{
  return float(double(x) * (std::atan(1.0) / 45.0));
}

struct Renderer
{
  struct TriShapeSortObject
  {
    NIFFile::NIFTriShape  *ts;
    float   z;
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
  unsigned int  waterColor;
  int     threadCnt;
  const BA2File *ba2File;
  std::vector< unsigned char >  fileBuf;
  std::string defaultEnvMap;
  std::string waterTexture;
  std::string whiteTexture;
  Renderer(unsigned int *outBufRGBA, float *outBufZ,
           int imageWidth, int imageHeight, bool isFO76);
  ~Renderer();
  void setBuffers(unsigned int *outBufRGBA, float *outBufZ,
                  int imageWidth, int imageHeight, float envMapScale);
  const DDSTexture *loadTexture(const std::string *texturePath, bool isDiffuse);
  static void threadFunction(Renderer *p, size_t n);
  void renderModel();
};

Renderer::Renderer(unsigned int *outBufRGBA, float *outBufZ,
                   int imageWidth, int imageHeight, bool isFO76)
{
  lightX = 0.0f;
  lightY = 0.0f;
  lightZ = 1.0f;
  waterEnvMapLevel = 1.0f;
  waterColor = 0xC0804000U;
  threadCnt = int(std::thread::hardware_concurrency());
  if (threadCnt < 1 || imageHeight < 64)
    threadCnt = 1;
  else if (threadCnt > 8)
    threadCnt = 8;
  ba2File = (BA2File *) 0;
  renderers.resize(size_t(threadCnt), (Plot3D_TriShape *) 0);
  threadErrMsg.resize(size_t(threadCnt));
  viewOffsetY.resize(size_t(threadCnt + 1), 0);
  try
  {
    for (size_t i = 0; i < renderers.size(); i++)
    {
      renderers[i] = new Plot3D_TriShape(outBufRGBA, outBufZ,
                                         imageWidth, imageHeight, isFO76);
    }
  }
  catch (...)
  {
    for (size_t i = 0; i < renderers.size(); i++)
    {
      if (renderers[i])
      {
        delete renderers[i];
        renderers[i] = (Plot3D_TriShape *) 0;
      }
    }
    throw;
  }
  setBuffers(outBufRGBA, outBufZ, imageWidth, imageHeight, 1.0f);
}

Renderer::~Renderer()
{
  for (size_t i = 0; i < renderers.size(); i++)
  {
    if (renderers[i])
      delete renderers[i];
  }
  for (std::map< std::string, DDSTexture * >::iterator
           i = textureSet.begin(); i != textureSet.end(); i++)
  {
    if (i->second)
      delete i->second;
  }
}

void Renderer::setBuffers(unsigned int *outBufRGBA, float *outBufZ,
                          int imageWidth, int imageHeight, float envMapScale)
{
  float   y0 = 0.0f;
  for (size_t i = 0; i < renderers.size(); i++)
  {
    int     y0i = roundFloat(y0);
    int     y1i = imageHeight;
    if ((i + 1) < renderers.size())
    {
      float   y1 = float(int(i + 1)) / float(int(renderers.size()));
      y1 = (y1 - 0.5f) * (y1 - 0.5f) * 2.0f;
      if (i < (renderers.size() >> 1))
        y1 = 0.5f - y1;
      else
        y1 = 0.5f + y1;
      y1 = y1 * float(imageHeight);
      y1i = roundFloat(y1);
      y0 = y1;
    }
    viewOffsetY[i] = y0i;
    size_t  offs = size_t(y0i) * size_t(imageWidth);
    renderers[i]->setBuffers(outBufRGBA + offs, outBufZ + offs,
                             imageWidth, y1i - y0i);
    renderers[i]->setEnvMapOffset(float(imageWidth) * -0.5f,
                                  float(imageHeight) * -0.5f + float(y0i),
                                  float(imageHeight) * envMapScale);
  }
  viewOffsetY[renderers.size()] = imageHeight;
}

const DDSTexture * Renderer::loadTexture(const std::string *texturePath,
                                         bool isDiffuse)
{
  if (!texturePath || texturePath->empty() || !ba2File)
    return (DDSTexture *) 0;
  DDSTexture  *t = (DDSTexture *) 0;
  textureSetMutex.lock();
  try
  {
    std::map< std::string, DDSTexture * >::iterator i =
        textureSet.find(*texturePath);
    if (i == textureSet.end())
    {
      try
      {
        ba2File->extractFile(fileBuf, *texturePath);
        t = new DDSTexture(&(fileBuf.front()), fileBuf.size());
      }
      catch (std::runtime_error& e)
      {
        std::fprintf(stderr, "Warning: failed to load texture '%s': %s\n",
                     texturePath->c_str(), e.what());
        t = (DDSTexture *) 0;
        if (isDiffuse && !whiteTexture.empty() && *texturePath != whiteTexture)
        {
          ba2File->extractFile(fileBuf, whiteTexture);
          t = new DDSTexture(&(fileBuf.front()), fileBuf.size());
        }
      }
      i = textureSet.insert(std::pair< std::string, DDSTexture * >(
                                *texturePath, t)).first;
    }
    t = i->second;
  }
  catch (...)
  {
    textureSetMutex.unlock();
    if (t)
      delete t;
    return (DDSTexture *) 0;
  }
  textureSetMutex.unlock();
  return t;
}

void Renderer::threadFunction(Renderer *p, size_t n)
{
  p->threadErrMsg[n].clear();
  try
  {
    std::vector< TriShapeSortObject > sortBuf;
    sortBuf.reserve(p->meshData.size());
    NIFFile::NIFVertexTransform vt(p->viewTransform);
    NIFFile::NIFVertexTransform mt(p->modelTransform);
    mt *= vt;
    vt.offsY = vt.offsY - float(p->viewOffsetY[n]);
    bool    haveWater = false;
    for (size_t i = 0; i < p->meshData.size(); i++)
    {
      if (p->meshData[i].flags & 0x05)          // ignore if hidden or effect
        continue;
      if (p->meshData[i].flags & 0x02)
        haveWater = true;
      NIFFile::NIFBounds  b;
      p->meshData[i].calculateBounds(b, &mt);
      if (roundFloat(b.xMax()) < 0 ||
          roundFloat(b.yMin()) > p->viewOffsetY[n + 1] ||
          roundFloat(b.yMax()) < p->viewOffsetY[n] ||
          b.zMax() < 0.0f)
      {
        continue;
      }
      TriShapeSortObject  tmp;
      tmp.ts = &(p->meshData.front()) + i;
      tmp.z = b.zMin();
      sortBuf.push_back(tmp);
    }
    if (sortBuf.size() < 1)
      return;
    std::stable_sort(sortBuf.begin(), sortBuf.end());
    for (size_t i = 0; i < sortBuf.size(); i++)
    {
      const NIFFile::NIFTriShape& ts = *(sortBuf[i].ts);
      *(p->renderers[n]) = ts;
      const DDSTexture  *textures[10];
      unsigned int  texturePathMask = (!(ts.flags & 0x80) ? 0x037BU : 0x037FU)
                                      & ((1U << ts.texturePathCnt) - 1U);
      for (size_t j = 0; j < 10; j++, texturePathMask >>= 1)
      {
        textures[j] = (DDSTexture *) 0;
        const std::string *texturePath = (std::string *) 0;
        if (texturePathMask & 1)
          texturePath = ts.texturePaths[j];
        if (!texturePath || texturePath->empty())
        {
          if (j != 0)
            continue;
          texturePath = &(p->whiteTexture);
        }
        textures[j] = p->loadTexture(texturePath, (j == 0));
      }
      if (!textures[4] && (textures[6] || textures[8]))
        textures[4] = p->loadTexture(&(p->defaultEnvMap), false);
      p->renderers[n]->drawTriShape(p->modelTransform, vt,
                                    p->lightX, p->lightY, p->lightZ,
                                    textures, 10);
    }
    if (haveWater)
    {
      for (size_t i = 0; i < sortBuf.size(); i++)
      {
        const NIFFile::NIFTriShape& ts = *(sortBuf[i].ts);
        if ((ts.flags & 0x07) != 0x02)          // ignore if not water
          continue;
        *(p->renderers[n]) = ts;
        const DDSTexture  *textures[10];
        for (size_t j = 10; j-- > 0; )
          textures[j] = (DDSTexture *) 0;
        textures[1] = p->loadTexture(&(p->waterTexture), false);
        textures[4] = p->loadTexture(&(p->defaultEnvMap), false);
        p->renderers[n]->drawWater(p->modelTransform, vt,
                                   p->lightX, p->lightY, p->lightZ,
                                   textures, 10,
                                   p->waterColor, p->waterEnvMapLevel);
      }
    }
  }
  catch (std::exception& e)
  {
    p->threadErrMsg[n] = e.what();
    if (p->threadErrMsg[n].empty())
      p->threadErrMsg[n] = "unknown error in render thread";
  }
}

void Renderer::renderModel()
{
  std::vector< std::thread * >  threads(renderers.size(), (std::thread *) 0);
  try
  {
    for (size_t i = 0; i < threads.size(); i++)
      threads[i] = new std::thread(threadFunction, this, i);
    for (size_t i = 0; i < threads.size(); i++)
    {
      if (threads[i])
      {
        threads[i]->join();
        delete threads[i];
        threads[i] = (std::thread *) 0;
      }
      if (!threadErrMsg[i].empty())
        throw errorMessage("%s", threadErrMsg[i].c_str());
    }
  }
  catch (...)
  {
    for (size_t i = 0; i < threads.size(); i++)
    {
      if (threads[i])
      {
        threads[i]->join();
        delete threads[i];
      }
    }
    throw;
  }
}

static void renderMeshToFile(const char *outFileName, const NIFFile& nifFile,
                             const BA2File& ba2File,
                             int imageWidth, int imageHeight)
{
#ifdef HAVE_SDL2
  int     screenWidth = imageWidth;
  int     screenHeight = imageHeight;
  bool    enableDownscale = bool(outFileName);
  bool    enableFullScreen = false;
  SDL_Window  *sdlWindow = (SDL_Window *) 0;
  SDL_Surface *sdlScreen = (SDL_Surface *) 0;
  if (!outFileName)
  {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS) != 0)
      throw errorMessage("error initializing SDL");
    SDL_DisplayMode m;
    if (SDL_GetDesktopDisplayMode(0, &m) == 0)
    {
      screenWidth = m.w;
      screenHeight = m.h;
      if ((imageWidth > screenWidth || imageHeight > screenHeight) &&
          !((imageWidth | imageHeight) & 1))
      {
        enableDownscale = true;
      }
      if ((imageWidth >> int(enableDownscale)) == screenWidth &&
          (imageHeight >> int(enableDownscale)) == screenHeight)
      {
        enableFullScreen = true;
      }
      screenWidth = imageWidth >> int(enableDownscale);
      screenHeight = imageHeight >> int(enableDownscale);
    }
  }
#endif

  try
  {
    size_t  imageDataSize = size_t(imageWidth) * size_t(imageHeight);
    std::vector< unsigned int > outBufRGBA(imageDataSize, 0U);
    std::vector< float >  outBufZ(imageDataSize, 16777216.0f);
    Renderer  renderer(&(outBufRGBA.front()), &(outBufZ.front()),
                       imageWidth, imageHeight, (nifFile.getVersion() >= 0x90));
    renderer.ba2File = &ba2File;
    nifFile.getMesh(renderer.meshData);
    renderer.waterTexture = "textures/water/defaultwater.dds";
    if (nifFile.getVersion() < 0x80U)
      renderer.whiteTexture = "textures/white.dds";
    else
      renderer.whiteTexture = "textures/effects/rainscene/test_flat_white.dds";
#ifdef HAVE_SDL2
    if (!outFileName)
    {
      sdlWindow = SDL_CreateWindow(
#if defined(_WIN32) || defined(_WIN64)
                      "nif_view",
#else
                      "nif_info",
#endif
                      SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                      screenWidth, screenHeight,
                      (enableFullScreen ? SDL_WINDOW_FULLSCREEN : 0));
      if (!sdlWindow)
        throw errorMessage("error creating SDL window");
      sdlScreen = SDL_CreateRGBSurface(0, screenWidth, screenHeight, 32,
                                       0x000000FFU, 0x0000FF00U,
                                       0x00FF0000U, 0xFF000000U);
      if (!sdlScreen)
        throw errorMessage("error setting SDL video mode");
      SDL_SetSurfaceBlendMode(sdlScreen, SDL_BLENDMODE_NONE);
    }
#endif

    float   modelRotationX = 0.0f;
    float   modelRotationY = 0.0f;
    float   modelRotationZ = 0.0f;
    // light direction: 0, 54.7356, -135 degrees
    float   lightRotationY = float(std::atan(std::sqrt(2.0)));
    float   lightRotationZ = float(std::atan(1.0) * -3.0);
    float   lightLevel = 1.0f;
    int     viewRotation = 0;   // isometric from NW
    int     viewScale = 0;      // 1.0
    int     envMapNum = 0;
    float   envMapScale = 1.0f;
    unsigned int  debugMode = 0;
    while (true)
    {
      renderer.modelTransform =
          NIFFile::NIFVertexTransform(
              1.0f, modelRotationX, modelRotationY, modelRotationZ,
              0.0f, 0.0f, 0.0f);
      renderer.viewTransform =
          NIFFile::NIFVertexTransform(
              1.0f,
              degreesToRadians(viewRotations[viewRotation * 3]),
              degreesToRadians(viewRotations[viewRotation * 3 + 1]),
              degreesToRadians(viewRotations[viewRotation * 3 + 2]),
              0.0f, 0.0f, 0.0f);
      NIFFile::NIFVertexTransform
          lightTransform(1.0f, 0.0f, lightRotationY, lightRotationZ,
                         0.0f, 0.0f, 0.0f);
      renderer.lightX = lightTransform.rotateZX;
      renderer.lightY = lightTransform.rotateZY;
      renderer.lightZ = lightTransform.rotateZZ;
      renderer.waterEnvMapLevel = 1.0f;
      renderer.waterColor = 0xC0804000U;
      {
        NIFFile::NIFVertexTransform t(renderer.modelTransform);
        t *= renderer.viewTransform;
        NIFFile::NIFBounds  b;
        for (size_t i = 0; i < renderer.meshData.size(); i++)
        {
          // ignore if hidden or effect
          if (!(renderer.meshData[i].flags & 0x05))
            renderer.meshData[i].calculateBounds(b, &t);
        }
        float   xScale = float(imageWidth) * 0.9375f;
        if (b.xMax() > b.xMin())
          xScale = xScale / (b.xMax() - b.xMin());
        float   yScale = float(imageHeight) * 0.9375f;
        if (b.yMax() > b.yMin())
          yScale = yScale / (b.yMax() - b.yMin());
        float   scale = (xScale < yScale ? xScale : yScale)
                        * float(std::pow(2.0f, float(viewScale) * 0.25f));
        renderer.viewTransform.scale = scale;
        renderer.viewTransform.offsX =
            0.5f * (float(imageWidth) - ((b.xMin() + b.xMax()) * scale));
        renderer.viewTransform.offsY =
            0.5f * (float(imageHeight) - ((b.yMin() + b.yMax()) * scale));
        renderer.viewTransform.offsZ = 1.0f - (b.zMin() * scale);
      }
      renderer.defaultEnvMap =
          std::string(cubeMapPaths[envMapNum * 3
                                   + int(nifFile.getVersion() >= 0x80)
                                   + int(nifFile.getVersion() >= 0x90)]);
      renderer.setBuffers(&(outBufRGBA.front()), &(outBufZ.front()),
                          imageWidth, imageHeight, envMapScale);
      {
        FloatVector4  a(Plot3D_TriShape::cubeMapToAmbient(
                            renderer.loadTexture(&(renderer.defaultEnvMap),
                                                 false),
                            (nifFile.getVersion() >= 0x90)));
        for (size_t i = 0; i < renderer.renderers.size(); i++)
        {
          renderer.renderers[i]->setLighting(
              FloatVector4(1.0f), a, FloatVector4(1.0f), lightLevel);
          renderer.renderers[i]->setDebugMode(debugMode, 0);
        }
      }
      renderer.renderModel();

#ifdef HAVE_SDL2
      if (!outFileName)
      {
        SDL_LockSurface(sdlScreen);
        Uint32  *dstPtr = reinterpret_cast< Uint32 * >(sdlScreen->pixels);
        if (!enableDownscale)
        {
          unsigned int  *srcPtr = &(outBufRGBA.front());
          for (int y = 0; y < imageHeight; y++)
          {
            for (int x = 0; x < imageWidth; x++, srcPtr++, dstPtr++)
            {
              *dstPtr = *srcPtr;
              *srcPtr = 0U;
              outBufZ[size_t(srcPtr - &(outBufRGBA.front()))] = 16777216.0f;
            }
            dstPtr = dstPtr + (int(sdlScreen->pitch >> 2) - screenWidth);
          }
        }
        else
        {
          for (int y = 0; y < imageHeight; y = y + 2)
          {
            for (int x = 0; x < imageWidth; x = x + 2, dstPtr++)
            {
              *dstPtr = downsample2xFilter(&(outBufRGBA.front()),
                                           imageWidth, imageHeight, x, y);
            }
            dstPtr = dstPtr + (int(sdlScreen->pitch >> 2) - screenWidth);
          }
          std::memset(&(outBufRGBA.front()), 0,
                      sizeof(unsigned int) * imageDataSize);
          for (size_t i = 0; i < imageDataSize; i++)
            outBufZ[i] = 16777216.0f;
        }
        SDL_UnlockSurface(sdlScreen);
        SDL_BlitSurface(sdlScreen, (SDL_Rect *) 0,
                        SDL_GetWindowSurface(sdlWindow), (SDL_Rect *) 0);
        SDL_UpdateWindowSurface(sdlWindow);
        bool    keyPressed = false;
        bool    quitFlag = false;
        while (!(keyPressed || quitFlag))
        {
          SDL_Delay(10);
          SDL_Event event;
          while (SDL_PollEvent(&event))
          {
            if (event.type != SDL_KEYDOWN)
            {
              if (event.type == SDL_QUIT)
                quitFlag = true;
              continue;
            }
            switch (event.key.keysym.sym)
            {
              case SDLK_0:
              case SDLK_1:
                debugMode = 0;
                break;
              case SDLK_2:
              case SDLK_3:
              case SDLK_4:
              case SDLK_5:
                debugMode = (unsigned int) (event.key.keysym.sym - SDLK_0);
                break;
              case SDLK_MINUS:
              case SDLK_KP_MINUS:
                viewScale -= int(viewScale > -16);
                break;
              case SDLK_EQUALS:
              case SDLK_KP_PLUS:
                viewScale += int(viewScale < 16);
                break;
              case SDLK_KP_7:
                viewRotation = 0;                       // isometric / NW
                break;
              case SDLK_KP_1:
                viewRotation = 1;                       // isometric / SW
                break;
              case SDLK_KP_3:
                viewRotation = 2;                       // isometric / SE
                break;
              case SDLK_KP_9:
                viewRotation = 3;                       // isometric / NE
                break;
              case SDLK_KP_5:
                viewRotation = 4;                       // top
                break;
              case SDLK_KP_2:
                viewRotation = 5;                       // front
                break;
              case SDLK_KP_6:
                viewRotation = 6;                       // right
                break;
              case SDLK_KP_8:
                viewRotation = 7;                       // back
                break;
              case SDLK_KP_4:
                viewRotation = 8;                       // left
                break;
              case SDLK_F1:
              case SDLK_F2:
              case SDLK_F3:
              case SDLK_F4:
              case SDLK_F5:
                envMapNum = int(event.key.keysym.sym - SDLK_F1);
                break;
              case SDLK_a:
                modelRotationZ += 0.19634954f;          // 11.25 degrees
                break;
              case SDLK_d:
                modelRotationZ -= 0.19634954f;
                break;
              case SDLK_s:
                modelRotationX += 0.19634954f;
                break;
              case SDLK_w:
                modelRotationX -= 0.19634954f;
                break;
              case SDLK_q:
                modelRotationY -= 0.19634954f;
                break;
              case SDLK_e:
                modelRotationY += 0.19634954f;
                break;
              case SDLK_k:
                lightLevel = lightLevel * 0.84089642f;
                lightLevel = (lightLevel > 0.25f ? lightLevel : 0.25f);
                break;
              case SDLK_l:
                lightLevel = lightLevel * 1.18920712f;
                lightLevel = (lightLevel < 4.0f ? lightLevel : 4.0f);
                break;
              case SDLK_LEFT:
                lightRotationZ += 0.19634954f;
                break;
              case SDLK_RIGHT:
                lightRotationZ -= 0.19634954f;
                break;
              case SDLK_DOWN:
                lightRotationY += 0.19634954f;
                break;
              case SDLK_UP:
                lightRotationY -= 0.19634954f;
                break;
              case SDLK_INSERT:
                envMapScale = envMapScale * 1.18920712f;
                envMapScale = (envMapScale < 4.0f ? envMapScale : 4.0f);
                break;
              case SDLK_DELETE:
                envMapScale = envMapScale * 0.84089642f;
                envMapScale = (envMapScale > 0.5f ? envMapScale : 0.5f);
                break;
              case SDLK_PAGEUP:
              case SDLK_PAGEDOWN:
                enableDownscale = (event.key.keysym.sym == SDLK_PAGEUP);
                imageWidth = screenWidth << int(enableDownscale);
                imageHeight = screenHeight << int(enableDownscale);
                imageDataSize = size_t(imageWidth) * size_t(imageHeight);
                outBufRGBA.resize(imageDataSize, 0U);
                outBufZ.resize(imageDataSize, 16777216.0f);
                break;
              case SDLK_ESCAPE:
                quitFlag = true;
                break;
              default:
                continue;
            }
            keyPressed = true;
          }
        }
        if (!quitFlag)
          continue;
      }
#endif
      break;
    }
#ifdef HAVE_SDL2
    if (outFileName)
#endif
    {
      DDSOutputFile outFile(outFileName, imageWidth >> 1, imageHeight >> 1,
                            DDSInputFile::pixelFormatRGB24);
      for (int y = 0; y < imageHeight; y = y + 2)
      {
        for (int x = 0; x < imageWidth; x = x + 2)
        {
          unsigned int  c = downsample2xFilter(&(outBufRGBA.front()),
                                               imageWidth, imageHeight, x, y);
          outFile.writeByte((unsigned char) ((c >> 16) & 0xFF));
          outFile.writeByte((unsigned char) ((c >> 8) & 0xFF));
          outFile.writeByte((unsigned char) (c & 0xFF));
        }
      }
    }
#ifdef HAVE_SDL2
    if (!outFileName)
    {
      SDL_FreeSurface(sdlScreen);
      SDL_DestroyWindow(sdlWindow);
      SDL_Quit();
    }
#endif
  }
  catch (...)
  {
#ifdef HAVE_SDL2
    if (!outFileName)
    {
      if (sdlScreen)
        SDL_FreeSurface(sdlScreen);
      if (sdlWindow)
        SDL_DestroyWindow(sdlWindow);
      SDL_Quit();
    }
#endif
    throw;
  }
}

int main(int argc, char **argv)
{
  std::FILE *outFile = stdout;
  try
  {
    // 0: default (block list)
    // 1: quiet (author info)
    // 2: verbose (block list and model data)
    // 3: .obj format
    // 4: .mtl format
    // 5: render to DDS file
    // 6: render and view in real time (requires SDL 2)
    int     outFmt = 0;
    int     renderWidth = 1344;
    int     renderHeight = 896;
    if (argc >= 2)
    {
      static const char *formatOptions[6] =
      {
        "-q", "-v", "-obj", "-mtl", "-render", "-view"
      };
      for (size_t i = 0; i < (sizeof(formatOptions) / sizeof(char *)); i++)
      {
        if (std::strcmp(argv[1], formatOptions[i]) == 0)
        {
          outFmt = int(i + 1);
          argc--;
          argv++;
          break;
        }
      }
      if (outFmt == 0 &&
          (std::strncmp(argv[1], "-render", 7) == 0 ||
           std::strncmp(argv[1], "-view", 5) == 0))
      {
        outFmt = (argv[1][1] == 'r' ? 5 : 6);
        argc--;
        argv++;
        std::string tmp(argv[0] + (outFmt == 5 ? 7 : 5));
        size_t  n = tmp.find('x');
        if (n != std::string::npos)
        {
          renderHeight = int(parseInteger(tmp.c_str() + (n + 1), 10,
                                          "invalid image height", 8, 16384));
          tmp.resize(n);
          renderWidth = int(parseInteger(tmp.c_str(), 10,
                                         "invalid image width", 8, 16384));
        }
        else
        {
          throw errorMessage("invalid image dimensions");
        }
      }
    }
    if (argc < (outFmt != 5 ? 3 : 4))
    {
      std::fprintf(stderr, "Usage: nif_info [OPTION] ARCHIVEPATH PATTERN...\n");
      std::fprintf(stderr, "Options:\n");
      std::fprintf(stderr, "    -q      Print author name, file name, "
                           "and file size only\n");
      std::fprintf(stderr, "    -v      Verbose mode, print block list, "
                           "and vertex and triangle data\n");
      std::fprintf(stderr, "    -obj    Print model data in .obj format\n");
      std::fprintf(stderr, "    -mtl    Print material data in .mtl format\n");
      std::fprintf(stderr, "    -render[WIDTHxHEIGHT] DDSFILE       "
                           "Render model to DDS file (experimental)\n");
#ifdef HAVE_SDL2
      std::fprintf(stderr, "    -view[WIDTHxHEIGHT] View model\n");
#endif
      return 1;
    }
    std::vector< std::string >  fileNames;
    for (int i = 2; i < argc; i++)
      fileNames.push_back(argv[i]);
    fileNames.push_back(".bgsm");
    if (outFmt >= 5)
      fileNames.push_back(".dds");
    if (outFmt == 5)
    {
      argc--;
      argv++;
    }
    BA2File ba2File(argv[1], &fileNames);
    ba2File.getFileList(fileNames);
    for (size_t i = 0; i < fileNames.size(); i++)
    {
      if (fileNames[i].length() < 5)
        continue;
      const char  *suffix = fileNames[i].c_str() + (fileNames[i].length() - 4);
      if (std::strcmp(suffix, ".nif") != 0 &&
          std::strcmp(suffix, ".btr") != 0 && std::strcmp(suffix, ".bto") != 0)
      {
        continue;
      }
      if (outFmt == 0 || outFmt == 2)
        std::fprintf(outFile, "==== %s ====\n", fileNames[i].c_str());
      else if (outFmt == 3 || outFmt == 4)
        std::fprintf(outFile, "# %s\n\n", fileNames[i].c_str());
      std::vector< unsigned char >  fileBuf;
      ba2File.extractFile(fileBuf, fileNames[i]);
      if (outFmt == 1)
      {
        printAuthorName(outFile, fileBuf, fileNames[i].c_str());
        continue;
      }
      NIFFile nifFile(&(fileBuf.front()), fileBuf.size(), &ba2File);
      if (outFmt == 0 || outFmt == 2)
        printBlockList(outFile, nifFile);
      if (outFmt == 2)
        printMeshData(outFile, nifFile);
      if (outFmt == 3)
      {
        size_t  n = fileNames[i].rfind('/');
        if (n == std::string::npos)
          n = 0;
        else
          n++;
        std::string mtlName(fileNames[i].c_str() + n);
        mtlName.resize(mtlName.length() - 3);
        mtlName += "mtl";
        printOBJData(outFile, nifFile, mtlName.c_str());
      }
      if (outFmt == 4)
        printMTLData(outFile, nifFile);
      if (outFmt == 5 || outFmt == 6)
      {
        std::fprintf(stderr, "%s\n", fileNames[i].c_str());
        if (outFmt == 5)
        {
          renderMeshToFile(argv[0], nifFile, ba2File,
                           renderWidth << 1, renderHeight << 1);
        }
        else
        {
#ifdef HAVE_SDL2
          renderMeshToFile((char *) 0, nifFile, ba2File,
                           renderWidth, renderHeight);
#else
          throw errorMessage("viewing the model requires SDL 2");
#endif
        }
        break;
      }
    }
  }
  catch (std::exception& e)
  {
    std::fprintf(stderr, "nif_info: %s\n", e.what());
    return 1;
  }
  return 0;
}

