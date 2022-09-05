
#include "common.hpp"
#include "nif_file.hpp"
#include "ba2file.hpp"
#include "ddstxt.hpp"
#include "plot3d.hpp"
#include "sdlvideo.hpp"

#include <thread>
#include <mutex>
#include <algorithm>
#include <ctime>

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
                   (unsigned int) lspBlock->bgsmSpecularColor);
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
                   v.getU(), v.getV(), (unsigned int) v.vertexColor);
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
  std::uint32_t waterColor;
  int     threadCnt;
  const BA2File *ba2File;
  std::vector< unsigned char >  fileBuf;
  std::string defaultEnvMap;
  std::string waterTexture;
  std::string whiteTexture;
  Renderer(std::uint32_t *outBufRGBA, float *outBufZ,
           int imageWidth, int imageHeight, bool isFO76);
  ~Renderer();
  void setBuffers(std::uint32_t *outBufRGBA, float *outBufZ,
                  int imageWidth, int imageHeight, float envMapScale);
  const DDSTexture *loadTexture(const std::string *texturePath, bool isDiffuse);
  static void threadFunction(Renderer *p, size_t n);
  void renderModel();
};

Renderer::Renderer(std::uint32_t *outBufRGBA, float *outBufZ,
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
  if (outBufRGBA && outBufZ)
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

void Renderer::setBuffers(std::uint32_t *outBufRGBA, float *outBufZ,
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
  float   modelRotationX = 0.0f;
  float   modelRotationY = 0.0f;
  float   modelRotationZ = 0.0f;
  float   lightRotationY = 56.25f;
  float   lightRotationZ = -135.0f;
  float   lightLevel = 1.0f;
  int     viewRotation = 0;     // isometric from NW
  float   viewScale = 1.0f;
  int     envMapNum = 0;
  float   envMapScale = 1.0f;
  unsigned int  debugMode = 0;

  size_t  imageDataSize = size_t(imageWidth) * size_t(imageHeight);
  std::vector< std::uint32_t >  outBufRGBA(imageDataSize, 0U);
  {
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
    renderer.modelTransform =
        NIFFile::NIFVertexTransform(
            1.0f, degreesToRadians(modelRotationX),
            degreesToRadians(modelRotationY),
            degreesToRadians(modelRotationZ), 0.0f, 0.0f, 0.0f);
    renderer.viewTransform =
        NIFFile::NIFVertexTransform(
            1.0f,
            degreesToRadians(viewRotations[viewRotation * 3]),
            degreesToRadians(viewRotations[viewRotation * 3 + 1]),
            degreesToRadians(viewRotations[viewRotation * 3 + 2]),
            0.0f, 0.0f, 0.0f);
    NIFFile::NIFVertexTransform
        lightTransform(1.0f, 0.0f, degreesToRadians(lightRotationY),
                       degreesToRadians(lightRotationZ), 0.0f, 0.0f, 0.0f);
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
      float   xScale = float(imageWidth) * 0.96875f;
      if (b.xMax() > b.xMin())
        xScale = xScale / (b.xMax() - b.xMin());
      float   yScale = float(imageHeight) * 0.96875f;
      if (b.yMax() > b.yMin())
        yScale = yScale / (b.yMax() - b.yMin());
      float   scale = (xScale < yScale ? xScale : yScale) * viewScale;
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
  }

  int     w = (imageWidth + 1) >> 1;
  int     h = (imageHeight + 1) >> 1;
  imageDataSize = size_t(w) * size_t(h);
  std::vector< std::uint32_t >  downsampleBuf(imageDataSize);
  downsample2xFilter(&(downsampleBuf.front()), &(outBufRGBA.front()),
                     imageWidth, imageHeight, w);
  DDSOutputFile outFile(outFileName, w, h, DDSInputFile::pixelFormatRGB24);
  for (size_t i = 0; i < imageDataSize; i++)
  {
    std::uint32_t c = downsampleBuf[i];
    outFile.writeByte((unsigned char) ((c >> 16) & 0xFF));
    outFile.writeByte((unsigned char) ((c >> 8) & 0xFF));
    outFile.writeByte((unsigned char) (c & 0xFF));
  }
}

static void updateRotation(float& rx, float& ry, float& rz,
                           float dx, float dy, float dz,
                           std::string& messageBuf, const char *msg)
{
  rx += dx;
  ry += dy;
  rz += dz;
  rx = (rx < -180.0f ? (rx + 360.0f) : (rx > 180.0f ? (rx - 360.0f) : rx));
  ry = (ry < -180.0f ? (ry + 360.0f) : (ry > 180.0f ? (ry - 360.0f) : ry));
  rz = (rz < -180.0f ? (rz + 360.0f) : (rz > 180.0f ? (rz - 360.0f) : rz));
  if (!msg)
    msg = "";
  char    buf[64];
  std::snprintf(buf, 64, "%s %7.2f %7.2f %7.2f\n", msg, rx, ry, rz);
  buf[63] = '\0';
  messageBuf = buf;
}

static void updateLightColor(FloatVector4& lightColor, FloatVector4 d,
                             std::string& messageBuf)
{
  lightColor *= d;
  lightColor.maxValues(FloatVector4(0.0625f));
  lightColor.minValues(FloatVector4(4.0f));
  char    buf[64];
  std::snprintf(buf, 64,
                "Light color (linear color space): %7.4f %7.4f %7.4f\n",
                lightColor[0], lightColor[1], lightColor[2]);
  buf[63] = '\0';
  messageBuf = buf;
}

static void updateValueLogScale(float& s, int d, float minVal, float maxVal,
                                std::string& messageBuf, const char *msg)
{
  int     tmp = roundFloat(float(std::log2(s)) * 4.0f);
  s = float(std::exp2(float(tmp + d) * 0.25f));
  s = (s > minVal ? (s < maxVal ? s : maxVal) : minVal);
  if (!msg)
    msg = "";
  char    buf[64];
  std::snprintf(buf, 64, "%s: %7.4f\n", msg, s);
  buf[63] = '\0';
  messageBuf = buf;
}

static void saveScreenshot(SDLDisplay& display, const std::string& nifFileName)
{
  size_t  n1 = nifFileName.rfind('/');
  n1 = (n1 != std::string::npos ? (n1 + 1) : 0);
  size_t  n2 = nifFileName.rfind('.');
  n2 = (n2 != std::string::npos ? n2 : 0);
  std::string fileName;
  if (n2 > n1)
    fileName.assign(nifFileName, n1, n2 - n1);
  else
    fileName = "nif_info";
  std::time_t t = std::time((std::time_t *) 0);
  {
    unsigned int  s = (unsigned int) (t % (std::time_t) (24 * 60 * 60));
    unsigned int  m = s / 60U;
    s = s % 60U;
    unsigned int  h = m / 60U;
    m = m % 60U;
    h = h % 24U;
    char    buf[16];
    std::sprintf(buf, "_%02u%02u%02u.dds", h, m, s);
    fileName += buf;
  }
  {
    display.blitSurface();
    const std::uint32_t *p = display.lockScreenSurface();
    int     w = display.getWidth() >> int(display.getIsDownsampled());
    int     h = display.getHeight() >> int(display.getIsDownsampled());
    DDSOutputFile f(fileName.c_str(), w, h, DDSInputFile::pixelFormatRGB24);
    size_t  pitch = display.getPitch();
    for (int y = 0; y < h; y++, p = p + pitch)
    {
      for (int x = 0; x < w; x++)
      {
        std::uint32_t c = p[x];
        f.writeByte((unsigned char) ((c >> 16) & 0xFF));
        f.writeByte((unsigned char) ((c >> 8) & 0xFF));
        f.writeByte((unsigned char) (c & 0xFF));
      }
    }
    display.unlockScreenSurface();
  }
  display.printString("Saved screenshot to ");
  display.printString(fileName.c_str());
  display.printString("\n");
}

static const char *keyboardUsageString =
    "  \033[4m\033[38;5;228m0\033[m "
    "to \033[4m\033[38;5;228m5\033[m                "
    "Set debug render mode.                                          \n"
    "  \033[4m\033[38;5;228m+\033[m, "
    "\033[4m\033[38;5;228m-\033[m                  "
    "Zoom in or out.                                                 \n"
    "  \033[4m\033[38;5;228mKeypad 1, 3, 9, 7\033[m     "
    "Set isometric view from the SW, SE, NE, or NW (default).        \n"
    "  \033[4m\033[38;5;228mKeypad 2, 6, 8, 4, 5\033[m  "
    "Set view from the S, E, N, W, or top.                           \n"
    "  \033[4m\033[38;5;228mF1\033[m "
    "to \033[4m\033[38;5;228mF5\033[m              "
    "Select default cube map.                                        \n"
    "  \033[4m\033[38;5;228mA\033[m, "
    "\033[4m\033[38;5;228mD\033[m                  "
    "Rotate model around the Z axis.                                 \n"
    "  \033[4m\033[38;5;228mS\033[m, "
    "\033[4m\033[38;5;228mW\033[m                  "
    "Rotate model around the X axis.                                 \n"
    "  \033[4m\033[38;5;228mQ\033[m, "
    "\033[4m\033[38;5;228mE\033[m                  "
    "Rotate model around the Y axis.                                 \n"
    "  \033[4m\033[38;5;228mK\033[m, "
    "\033[4m\033[38;5;228mL\033[m                  "
    "Decrease or increase overall brightness.                        \n"
    "  \033[4m\033[38;5;228mU\033[m, "
    "\033[4m\033[38;5;228m7\033[m                  "
    "Decrease or increase light source red level.                    \n"
    "  \033[4m\033[38;5;228mI\033[m, "
    "\033[4m\033[38;5;228m8\033[m                  "
    "Decrease or increase light source green level.                  \n"
    "  \033[4m\033[38;5;228mO\033[m, "
    "\033[4m\033[38;5;228m9\033[m                  "
    "Decrease or increase light source blue level.                   \n"
    "  \033[4m\033[38;5;228mLeft\033[m, "
    "\033[4m\033[38;5;228mRight\033[m           "
    "Rotate light vector around the Z axis.                          \n"
    "  \033[4m\033[38;5;228mUp\033[m, "
    "\033[4m\033[38;5;228mDown\033[m              "
    "Rotate light vector around the Y axis.                          \n"
    "  \033[4m\033[38;5;228mInsert\033[m, "
    "\033[4m\033[38;5;228mDelete\033[m        "
    "Zoom reflected environment in or out.                           \n"
    "  \033[4m\033[38;5;228mPage Up\033[m               "
    "Enable downsampling (slow).                                     \n"
    "  \033[4m\033[38;5;228mPage Down\033[m             "
    "Disable downsampling.                                           \n"
    "  \033[4m\033[38;5;228mSpace\033[m, "
    "\033[4m\033[38;5;228mBackspace\033[m      "
    "Load next or previous file matching the pattern.                \n"
    "  \033[4m\033[38;5;228mF12\033[m "
    "or \033[4m\033[38;5;228mPrint Screen\033[m   "
    "Save screenshot.                                                \n"
    "  \033[4m\033[38;5;228mH\033[m                     "
    "Show help screen.                                               \n"
    "  \033[4m\033[38;5;228mEsc\033[m                   "
    "Quit program.                                                   \n";

static void viewMeshes(const BA2File& ba2File,
                       const std::vector< std::string >& nifFileNames,
                       int imageWidth, int imageHeight)
{
  if (nifFileNames.size() < 1)
    return;
  std::vector< SDLDisplay::SDLEvent > eventBuf;
  std::string messageBuf;
  SDLDisplay  display(imageWidth, imageHeight, "nif_info", 4U, 48);
  bool    quitFlag = false;
  try
  {
    display.setDefaultTextColor(0x00, 0xC1);
    imageWidth = display.getWidth();
    imageHeight = display.getHeight();
    float   modelRotationX = 0.0f;
    float   modelRotationY = 0.0f;
    float   modelRotationZ = 0.0f;
    float   lightRotationX = 0.0f;
    float   lightRotationY = 56.25f;
    float   lightRotationZ = -135.0f;
    FloatVector4  lightColor(1.0f);
    float   lightLevel = 1.0f;
    int     viewRotation = 0;   // isometric from NW
    float   viewScale = 1.0f;
    int     envMapNum = 0;
    float   envMapScale = 1.0f;
    unsigned int  debugMode = 0;
    int     fileNum = 0;

    size_t  imageDataSize = size_t(imageWidth) * size_t(imageHeight);
    std::vector< float >  outBufZ(imageDataSize);
    std::vector< unsigned char >  fileBuf;
    while (!quitFlag)
    {
      messageBuf += nifFileNames[fileNum];
      messageBuf += '\n';
      ba2File.extractFile(fileBuf, nifFileNames[fileNum]);
      NIFFile   nifFile(&(fileBuf.front()), fileBuf.size(), &ba2File);
      Renderer  renderer((std::uint32_t *) 0, (float *) 0,
                         imageWidth, imageHeight,
                         (nifFile.getVersion() >= 0x90));
      renderer.ba2File = &ba2File;
      nifFile.getMesh(renderer.meshData);
      renderer.waterTexture = "textures/water/defaultwater.dds";
      renderer.whiteTexture =
          (nifFile.getVersion() < 0x80U ?
           "textures/white.dds"
           : "textures/effects/rainscene/test_flat_white.dds");

      bool    nextFileFlag = false;
      bool    screenshotFlag = false;
      unsigned char redrawFlags = 3;    // bit 0: blit only, bit 1: render
      while (!(nextFileFlag || quitFlag))
      {
        if (!messageBuf.empty())
        {
          display.printString(messageBuf.c_str());
          messageBuf.clear();
        }
        if (redrawFlags & 2)
        {
          renderer.modelTransform =
              NIFFile::NIFVertexTransform(
                  1.0f, degreesToRadians(modelRotationX),
                  degreesToRadians(modelRotationY),
                  degreesToRadians(modelRotationZ), 0.0f, 0.0f, 0.0f);
          renderer.viewTransform =
              NIFFile::NIFVertexTransform(
                  1.0f,
                  degreesToRadians(viewRotations[viewRotation * 3]),
                  degreesToRadians(viewRotations[viewRotation * 3 + 1]),
                  degreesToRadians(viewRotations[viewRotation * 3 + 2]),
                  0.0f, 0.0f, 0.0f);
          NIFFile::NIFVertexTransform
              lightTransform(
                  1.0f, degreesToRadians(lightRotationX),
                  degreesToRadians(lightRotationY),
                  degreesToRadians(lightRotationZ), 0.0f, 0.0f, 0.0f);
          renderer.lightX = lightTransform.rotateZX;
          renderer.lightY = lightTransform.rotateZY;
          renderer.lightZ = lightTransform.rotateZZ;
          renderer.waterEnvMapLevel = 1.0f;
          renderer.waterColor = 0xC0804000U;
          NIFFile::NIFVertexTransform t(renderer.modelTransform);
          t *= renderer.viewTransform;
          NIFFile::NIFBounds  b;
          for (size_t i = 0; i < renderer.meshData.size(); i++)
          {
            // ignore if hidden or effect
            if (!(renderer.meshData[i].flags & 0x05))
              renderer.meshData[i].calculateBounds(b, &t);
          }
          float   xScale = float(imageWidth) * 0.96875f;
          if (b.xMax() > b.xMin())
            xScale = xScale / (b.xMax() - b.xMin());
          float   yScale = float(imageHeight) * 0.96875f;
          if (b.yMax() > b.yMin())
            yScale = yScale / (b.yMax() - b.yMin());
          float   scale = (xScale < yScale ? xScale : yScale) * viewScale;
          renderer.viewTransform.scale = scale;
          renderer.viewTransform.offsX =
              0.5f * (float(imageWidth) - ((b.xMin() + b.xMax()) * scale));
          renderer.viewTransform.offsY =
              0.5f * (float(imageHeight) - ((b.yMin() + b.yMax()) * scale));
          renderer.viewTransform.offsZ = 1.0f - (b.zMin() * scale);
          renderer.defaultEnvMap =
              std::string(cubeMapPaths[envMapNum * 3
                                       + int(nifFile.getVersion() >= 0x80)
                                       + int(nifFile.getVersion() >= 0x90)]);
          display.clearSurface();
          for (size_t i = 0; i < imageDataSize; i++)
            outBufZ[i] = 16777216.0f;
          std::uint32_t *outBufRGBA = display.lockDrawSurface();
          renderer.setBuffers(outBufRGBA, &(outBufZ.front()),
                              imageWidth, imageHeight, envMapScale);
          FloatVector4  a(Plot3D_TriShape::cubeMapToAmbient(
                              renderer.loadTexture(&(renderer.defaultEnvMap),
                                                   false),
                              (nifFile.getVersion() >= 0x90)));
          for (size_t i = 0; i < renderer.renderers.size(); i++)
          {
            renderer.renderers[i]->setLighting(
                lightColor, a, FloatVector4(1.0f), lightLevel);
            renderer.renderers[i]->setDebugMode(debugMode, 0);
          }
          renderer.renderModel();
          display.unlockDrawSurface();
          if (screenshotFlag)
          {
            saveScreenshot(display, nifFileNames[fileNum]);
            screenshotFlag = false;
          }
          display.drawText(0, -1, display.getTextRows(), 0.75f, 1.0f);
          redrawFlags = 1;
        }
        display.blitSurface();
        redrawFlags = 0;

        while (!(redrawFlags || nextFileFlag || quitFlag))
        {
          display.pollEvents(eventBuf, 10, false, false);
          for (size_t i = 0; i < eventBuf.size(); i++)
          {
            int     t = eventBuf[i].type();
            int     d1 = eventBuf[i].data1();
            if (t == SDLDisplay::SDLEventWindow)
            {
              if (d1 == 0)
                quitFlag = true;
              else if (d1 == 1)
                redrawFlags = 1;
              continue;
            }
            if (!(t == SDLDisplay::SDLEventKeyRepeat ||
                  t == SDLDisplay::SDLEventKeyDown))
            {
              continue;
            }
            redrawFlags = 2;
            switch (d1)
            {
              case '0':
              case '1':
              case '2':
              case '3':
              case '4':
              case '5':
                debugMode = (unsigned int) (d1 != '1' ? (d1 - '0') : 0);
                messageBuf += "Debug mode set to ";
                messageBuf += char(debugMode | 0x30U);
                messageBuf += '\n';
                break;
              case '-':
              case SDLDisplay::SDLKeySymKPMinus:
                updateValueLogScale(viewScale, -1, 0.0625f, 16.0f, messageBuf,
                                    "View scale");
                break;
              case '=':
              case SDLDisplay::SDLKeySymKPPlus:
                updateValueLogScale(viewScale, 1, 0.0625f, 16.0f, messageBuf,
                                    "View scale");
                break;
              case SDLDisplay::SDLKeySymKP1 + 6:
                viewRotation = 0;
                messageBuf += "Isometric view from the NW\n";
                break;
              case SDLDisplay::SDLKeySymKP1:
                viewRotation = 1;
                messageBuf += "Isometric view from the SW\n";
                break;
              case SDLDisplay::SDLKeySymKP1 + 2:
                viewRotation = 2;
                messageBuf += "Isometric view from the SE\n";
                break;
              case SDLDisplay::SDLKeySymKP1 + 8:
                viewRotation = 3;
                messageBuf += "Isometric view from the NE\n";
                break;
              case SDLDisplay::SDLKeySymKP1 + 4:
                viewRotation = 4;
                messageBuf += "Top view\n";
                break;
              case SDLDisplay::SDLKeySymKP1 + 1:
                viewRotation = 5;
                messageBuf += "S view\n";
                break;
              case SDLDisplay::SDLKeySymKP1 + 5:
                viewRotation = 6;
                messageBuf += "E view\n";
                break;
              case SDLDisplay::SDLKeySymKP1 + 7:
                viewRotation = 7;
                messageBuf += "N view\n";
                break;
              case SDLDisplay::SDLKeySymKP1 + 3:
                viewRotation = 8;
                messageBuf += "W view\n";
                break;
              case SDLDisplay::SDLKeySymF1:
              case SDLDisplay::SDLKeySymF1 + 1:
              case SDLDisplay::SDLKeySymF1 + 2:
              case SDLDisplay::SDLKeySymF1 + 3:
              case SDLDisplay::SDLKeySymF1 + 4:
                envMapNum = d1 - SDLDisplay::SDLKeySymF1;
                messageBuf += "Default environment map: ";
                messageBuf += cubeMapPaths[envMapNum * 3
                                           + int(nifFile.getVersion() >= 0x80)
                                           + int(nifFile.getVersion() >= 0x90)];
                messageBuf += '\n';
                break;
              case 'a':
                updateRotation(modelRotationX, modelRotationY, modelRotationZ,
                               0.0f, 0.0f, 11.25f,
                               messageBuf, "Model rotation");
                break;
              case 'd':
                updateRotation(modelRotationX, modelRotationY, modelRotationZ,
                               0.0f, 0.0f, -11.25f,
                               messageBuf, "Model rotation");
                break;
              case 's':
                updateRotation(modelRotationX, modelRotationY, modelRotationZ,
                               11.25f, 0.0f, 0.0f,
                               messageBuf, "Model rotation");
                break;
              case 'w':
                updateRotation(modelRotationX, modelRotationY, modelRotationZ,
                               -11.25f, 0.0f, 0.0f,
                               messageBuf, "Model rotation");
                break;
              case 'q':
                updateRotation(modelRotationX, modelRotationY, modelRotationZ,
                               0.0f, -11.25f, 0.0f,
                               messageBuf, "Model rotation");
                break;
              case 'e':
                updateRotation(modelRotationX, modelRotationY, modelRotationZ,
                               0.0f, 11.25f, 0.0f,
                               messageBuf, "Model rotation");
                break;
              case 'k':
                updateValueLogScale(lightLevel, -1, 0.25f, 4.0f, messageBuf,
                                    "Brightness (linear color space)");
                break;
              case 'l':
                updateValueLogScale(lightLevel, 1, 0.25f, 4.0f, messageBuf,
                                    "Brightness (linear color space)");
                break;
              case SDLDisplay::SDLKeySymLeft:
                updateRotation(lightRotationX, lightRotationY, lightRotationZ,
                               0.0f, 0.0f, 11.25f,
                               messageBuf, "Light rotation");
                break;
              case SDLDisplay::SDLKeySymRight:
                updateRotation(lightRotationX, lightRotationY, lightRotationZ,
                               0.0f, 0.0f, -11.25f,
                               messageBuf, "Light rotation");
                break;
              case SDLDisplay::SDLKeySymDown:
                updateRotation(lightRotationX, lightRotationY, lightRotationZ,
                               0.0f, 11.25f, 0.0f,
                               messageBuf, "Light rotation");
                break;
              case SDLDisplay::SDLKeySymUp:
                updateRotation(lightRotationX, lightRotationY, lightRotationZ,
                               0.0f, -11.25f, 0.0f,
                               messageBuf, "Light rotation");
                break;
              case '7':
                updateLightColor(lightColor,
                                 FloatVector4(1.18920712f, 1.0f, 1.0f, 1.0f),
                                 messageBuf);
                break;
              case 'u':
                updateLightColor(lightColor,
                                 FloatVector4(0.84089642f, 1.0f, 1.0f, 1.0f),
                                 messageBuf);
                break;
              case '8':
                updateLightColor(lightColor,
                                 FloatVector4(1.0f, 1.18920712f, 1.0f, 1.0f),
                                 messageBuf);
                break;
              case 'i':
                updateLightColor(lightColor,
                                 FloatVector4(1.0f, 0.84089642f, 1.0f, 1.0f),
                                 messageBuf);
                break;
              case '9':
                updateLightColor(lightColor,
                                 FloatVector4(1.0f, 1.0f, 1.18920712f, 1.0f),
                                 messageBuf);
                break;
              case 'o':
                updateLightColor(lightColor,
                                 FloatVector4(1.0f, 1.0f, 0.84089642f, 1.0f),
                                 messageBuf);
                break;
              case SDLDisplay::SDLKeySymInsert:
                updateValueLogScale(envMapScale, 1, 0.5f, 4.0f, messageBuf,
                                    "Reflection f scale");
                break;
              case SDLDisplay::SDLKeySymDelete:
                updateValueLogScale(envMapScale, -1, 0.5f, 4.0f, messageBuf,
                                    "Reflection f scale");
                break;
              case SDLDisplay::SDLKeySymPageUp:
              case SDLDisplay::SDLKeySymPageDown:
                if ((d1 == SDLDisplay::SDLKeySymPageUp)
                    == display.getIsDownsampled())
                {
                  redrawFlags = 0;
                  continue;
                }
                display.setEnableDownsample(d1 == SDLDisplay::SDLKeySymPageUp);
                imageWidth = display.getWidth();
                imageHeight = display.getHeight();
                imageDataSize = size_t(imageWidth) * size_t(imageHeight);
                outBufZ.resize(imageDataSize);
                if (display.getIsDownsampled())
                  messageBuf += "Downsampling enabled\n";
                else
                  messageBuf += "Downsampling disabled\n";
                break;
              case SDLDisplay::SDLKeySymBackspace:
                fileNum = (fileNum > 0 ? fileNum : int(nifFileNames.size()));
                fileNum--;
                nextFileFlag = true;
                break;
              case ' ':
                fileNum++;
                fileNum = (size_t(fileNum) < nifFileNames.size() ? fileNum : 0);
                nextFileFlag = true;
                break;
              case SDLDisplay::SDLKeySymF1 + 11:
              case SDLDisplay::SDLKeySymPrintScr:
                screenshotFlag = true;
                break;
              case 'h':
                messageBuf = keyboardUsageString;
                break;
              case SDLDisplay::SDLKeySymEscape:
                quitFlag = true;
                break;
              default:
                redrawFlags = 0;
                continue;
            }
            display.clearTextBuffer();
          }
        }
      }
    }
  }
  catch (std::exception& e)
  {
    display.unlockScreenSurface();
    messageBuf += "\033[41m\033[33m\033[1m    Error: ";
    messageBuf += e.what();
    messageBuf += "    ";
    display.printString(messageBuf.c_str());
    display.drawText(0, -1, display.getTextRows(), 1.0f, 1.0f);
    display.blitSurface();
    do
    {
      display.pollEvents(eventBuf, 10, false, false);
      for (size_t i = 0; i < eventBuf.size(); i++)
      {
        if ((eventBuf[i].type() == SDLDisplay::SDLEventWindow &&
             eventBuf[i].data1() == 0) ||
            eventBuf[i].type() == SDLDisplay::SDLEventKeyDown)
        {
          quitFlag = true;
          break;
        }
        else if (eventBuf[i].type() == SDLDisplay::SDLEventWindow &&
                 eventBuf[i].data1() == 1)
        {
          display.blitSurface();
        }
      }
    }
    while (!quitFlag);
    throw;
  }
}

int main(int argc, char **argv)
{
  std::FILE *outFile = stdout;
  const char  *outFileName = (char *) 0;
  bool    consoleFlag = true;
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
    for ( ; argc >= 2 && argv[1][0] == '-'; argc--, argv++)
    {
      if (std::strcmp(argv[1], "--") == 0)
      {
        argc--;
        argv++;
        break;
      }
      if (std::strcmp(argv[1], "-q") == 0)
      {
        outFmt = 1;
      }
      else if (std::strcmp(argv[1], "-v") == 0)
      {
        outFmt = 2;
      }
      else if (std::strcmp(argv[1], "-obj") == 0)
      {
        outFmt = 3;
      }
      else if (std::strcmp(argv[1], "-mtl") == 0)
      {
        outFmt = 4;
      }
      else if (std::strncmp(argv[1], "-render", 7) == 0 ||
               std::strncmp(argv[1], "-view", 5) == 0)
      {
        outFmt = (argv[1][1] == 'r' ? 5 : 6);
        std::string tmp(argv[1] + (outFmt == 5 ? 7 : 5));
        size_t  n = tmp.find('x');
        if (n != std::string::npos)
        {
          renderHeight = int(parseInteger(tmp.c_str() + (n + 1), 10,
                                          "invalid image height", 8, 16384));
          tmp.resize(n);
          renderWidth = int(parseInteger(tmp.c_str(), 10,
                                         "invalid image width", 8, 16384));
        }
        else if (!tmp.empty())
        {
          throw errorMessage("invalid image dimensions");
        }
        if (outFmt == 5)
        {
          if (argc < 3)
            throw errorMessage("missing output file name for -render");
          outFileName = argv[2];
          argc--;
          argv++;
        }
      }
      else
      {
        throw errorMessage("invalid option: %s", argv[1]);
      }
    }
    if (argc < 3)
    {
      std::fprintf(stderr,
                   "Usage: nif_info [OPTIONS] ARCHIVEPATH PATTERN...\n");
      std::fprintf(stderr, "Options:\n");
      std::fprintf(stderr, "    --      Remaining options are file names\n");
      std::fprintf(stderr, "    -q      Print author name, file name, "
                           "and file size only\n");
      std::fprintf(stderr, "    -v      Verbose mode, print block list, "
                           "and vertex and triangle data\n");
      std::fprintf(stderr, "    -obj    Print model data in .obj format\n");
      std::fprintf(stderr, "    -mtl    Print material data in .mtl format\n");
      std::fprintf(stderr, "    -render[WIDTHxHEIGHT] DDSFILE   "
                           "Render model to DDS file\n");
#ifdef HAVE_SDL2
      std::fprintf(stderr, "    -view[WIDTHxHEIGHT]     View model\n");
#endif
      return 1;
    }
    consoleFlag = false;
    if (outFmt != 6)
      SDLDisplay::enableConsole();
    std::vector< std::string >  fileNames;
    for (int i = 2; i < argc; i++)
      fileNames.push_back(argv[i]);
    fileNames.push_back(".bgsm");
    if (outFmt >= 5)
      fileNames.push_back(".dds");
    BA2File ba2File(argv[1], &fileNames);
    ba2File.getFileList(fileNames);
    if (outFmt == 6)
    {
      for (size_t i = 0; i < fileNames.size(); )
      {
        if (fileNames[i].length() >= 5)
        {
          const char  *suffix =
              fileNames[i].c_str() + (fileNames[i].length() - 4);
          if (std::strcmp(suffix, ".nif") == 0 ||
              std::strcmp(suffix, ".btr") == 0 ||
              std::strcmp(suffix, ".bto") == 0)
          {
            i++;
            continue;
          }
        }
        if ((i + 1) < fileNames.size())
          fileNames[i] = fileNames[fileNames.size() - 1];
        fileNames.resize(fileNames.size() - 1);
      }
      std::sort(fileNames.begin(), fileNames.end());
      viewMeshes(ba2File, fileNames, renderWidth, renderHeight);
      return 0;
    }
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
      if (outFmt == 5)
      {
        std::fprintf(stderr, "%s\n", fileNames[i].c_str());
        renderMeshToFile(outFileName, nifFile, ba2File,
                         renderWidth << 1, renderHeight << 1);
        break;
      }
    }
  }
  catch (std::exception& e)
  {
    if (consoleFlag)
      SDLDisplay::enableConsole();
    std::fprintf(stderr, "nif_info: %s\n", e.what());
    return 1;
  }
  return 0;
}

