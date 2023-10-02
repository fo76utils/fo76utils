
#include "common.hpp"
#include "nif_view.hpp"
#include "downsamp.hpp"

#include <algorithm>
#ifdef HAVE_SDL2
#  include <SDL2/SDL.h>
#endif

#include "viewrtbl.cpp"

static const std::uint32_t  defaultWaterColor = 0xC0402010U;

static const char *cubeMapPaths[32] =
{
  "textures/cubemaps/bleakfallscube_e.dds",                     // Skyrim
  "textures/shared/cubemaps/mipblur_defaultoutside1.dds",       // Fallout 4
  "textures/shared/cubemaps/mipblur_defaultoutside1.dds",       // Fallout 76
  "textures/cubemaps/cell_spacecube.dds",                       // Starfield
  "textures/cubemaps/wrtemple_e.dds",
  "textures/shared/cubemaps/outsideoldtownreflectcube_e.dds",
  "textures/shared/cubemaps/outsideoldtownreflectcube_e.dds",
  "textures/cubemaps/newatlantislodgeint_cm.dds",
  "textures/cubemaps/duncaveruingreen_e.dds",
  "textures/shared/cubemaps/cgprewarstreet_e.dds",
  "textures/shared/cubemaps/swampcube.dds",
  "textures/cubemaps/cell_cavecube.dds",
  "textures/cubemaps/chrome_e.dds",
  "textures/shared/cubemaps/metalchrome01cube_e.dds",
  "textures/shared/cubemaps/metalchrome01cube_e.dds",
  "textures/cubemaps/cell_shipinteriorcube.dds",
  "textures/cubemaps/cavegreencube_e.dds",
  "textures/shared/cubemaps/outsideday01.dds",
  "textures/shared/cubemaps/outsideday01.dds",
  "textures/cubemaps/milkywaycubemap.dds",
  "textures/cubemaps/mghallcube_e.dds",
  "textures/shared/cubemaps/cgplayerhousecube.dds",
  "textures/shared/cubemaps/chrome_e.dds",
  "textures/cubemaps/cell_hitechhallcube.dds",
  "textures/cubemaps/caveicecubemap_e.dds",
  "textures/shared/cubemaps/inssynthproductionpoolcube.dds",
  "textures/shared/cubemaps/vault111cryocube.dds",
  "textures/cubemaps/cell_kreetbase01cube.dds",
  "textures/cubemaps/minecube_e.dds",
  "textures/shared/cubemaps/memorydencube.dds",
  "textures/shared/cubemaps/mipblur_defaultoutside_pitt.dds",
  "textures/cubemaps/lgt_cubemap_research_002.dds"
};

void NIF_View::threadFunction(NIF_View *p, size_t n)
{
  p->threadErrMsg[n].clear();
  try
  {
    std::vector< TriShapeSortObject > sortBuf;
    sortBuf.reserve(p->meshData.size());
    NIFFile::NIFVertexTransform mt(p->modelTransform);
    NIFFile::NIFVertexTransform vt(p->viewTransform);
    mt *= vt;
    vt.offsY = vt.offsY - float(p->viewOffsetY[n]);
    p->renderers[n]->setViewAndLightVector(vt, p->lightX, p->lightY, p->lightZ);
    for (size_t i = 0; i < p->meshData.size(); i++)
    {
      const NIFFile::NIFTriShape& ts = p->meshData[i];
      if (ts.flags & NIFFile::NIFTriShape::Flag_TSHidden)
        continue;
      NIFFile::NIFBounds  b;
      ts.calculateBounds(b, &mt);
      if (roundFloat(b.xMax()) < 0 ||
          roundFloat(b.yMin()) > p->viewOffsetY[n + 1] ||
          roundFloat(b.yMax()) < p->viewOffsetY[n] ||
          b.zMax() < 0.0f)
      {
        continue;
      }
      sortBuf.emplace(sortBuf.end(), i, b.zMin(),
                      bool(ts.flags & CE2Material::Flag_AlphaBlending));
      if (BRANCH_UNLIKELY(ts.flags & NIFFile::NIFTriShape::Flag_TSOrdered))
        TriShapeSortObject::orderedNodeFix(sortBuf, p->meshData);
    }
    if (sortBuf.size() < 1)
      return;
    std::sort(sortBuf.begin(), sortBuf.end());
    for (size_t i = 0; i < sortBuf.size(); i++)
    {
      Plot3D_TriShape&  ts = *(p->renderers[n]);
      {
        size_t  j = size_t(sortBuf[i]);
        ts = p->meshData[j];
        if (BRANCH_UNLIKELY(p->debugMode == 1))
        {
          std::uint32_t c = std::uint32_t(j + 1);
          c = ((c & 1U) << 23) | ((c & 2U) << 14) | ((c & 4U) << 5)
              | ((c & 8U) << 19) | ((c & 16U) << 10) | ((c & 32U) << 1)
              | ((c & 64U) << 15) | ((c & 128U) << 6) | ((c & 256U) >> 3)
              | ((c & 512U) << 11) | ((c & 1024U) << 2) | ((c & 2048U) >> 7);
          c = c ^ 0xFFFFFFFFU;
          ts.setDebugMode(1, c);
        }
      }
      const DDSTexture  *textures[10];
      unsigned int  textureMask = 0U;
      if (BRANCH_UNLIKELY(ts.flags & CE2Material::Flag_IsWater))
      {
        if (bool(textures[1] = p->loadTexture(p->waterTexture, n)))
          textureMask |= 0x0002U;
        if (!p->defaultEnvMap.empty())
          ts.setEnvironmentMap(p->loadTexture(p->defaultEnvMap, n));
        std::map< unsigned int, WaterProperties >::const_iterator j =
            p->waterMaterials.find(p->waterFormID);
        if (j == p->waterMaterials.end())
          j = p->waterMaterials.find(0U);
        ts.setWaterProperties(
            j->second.deepColor, j->second.alphaDepth0, j->second.depthMult,
            1.0f / 31.0f, 1.0f, 0.02032076f, p->waterEnvMapLevel);
      }
      else
      {
        const CE2Material::TextureSet *txtSet = findTextureSet(ts.m);
        unsigned int  texturePathMask = 0U;
        if (txtSet)
        {
          texturePathMask =
              (!(ts.flags & CE2Material::Flag_Glow) ? 0x013FU : 0x01BFU)
              & txtSet->texturePathMask;
        }
        for (size_t j = 0; texturePathMask; j++, texturePathMask >>= 1)
        {
          if (texturePathMask & 1)
          {
            textures[j] = p->loadTexture(*(txtSet->texturePaths[j]), n);
            if (textures[j])
              textureMask |= (1U << (unsigned char) j);
          }
        }
        if (!p->defaultEnvMap.empty())
          ts.setEnvironmentMap(p->loadTexture(p->defaultEnvMap, n));
      }
      ts.drawTriShape(p->modelTransform, textures, textureMask);
    }
  }
  catch (std::exception& e)
  {
    p->threadErrMsg[n] = e.what();
    if (p->threadErrMsg[n].empty())
      p->threadErrMsg[n] = "unknown error in render thread";
  }
}

const DDSTexture * NIF_View::loadTexture(const std::string& texturePath,
                                         size_t threadNum)
{
  const DDSTexture  *t =
      textureSet.loadTexture(ba2File, texturePath,
                             threadFileBuffers[threadNum], 0);
#if 0
  if (!t && !texturePath.empty())
  {
    std::fprintf(stderr, "Warning: failed to load texture '%s'\n",
                 texturePath.c_str());
  }
#endif
  return t;
}

void NIF_View::setDefaultTextures()
{
  int     n = 3;
  if (nifFile)
    n = std::min(std::max(int(nifFile->getVersion() >> 4) - 7, 0), 3);
  n = n + ((defaultEnvMapNum & 7) << 2);
  defaultEnvMap = cubeMapPaths[n];
  waterTexture = "textures/water/wavesdefault_normal.dds";
}

NIF_View::NIF_View(const BA2File& archiveFiles, ESMFile *esmFilePtr,
                   const char *materialDBPath)
  : ba2File(archiveFiles),
    esmFile(esmFilePtr),
    textureSet(0x10000000),
    lightX(0.0f),
    lightY(0.0f),
    lightZ(1.0f),
    nifFile((NIFFile *) 0),
    defaultTexture(0xFFFFFFFFU),
    materials(ba2File, materialDBPath),
    modelRotationX(0.0f),
    modelRotationY(0.0f),
    modelRotationZ(0.0f),
    viewRotationX(54.73561f),
    viewRotationY(180.0f),
    viewRotationZ(45.0f),
    viewOffsX(0.0f),
    viewOffsY(0.0f),
    viewOffsZ(0.0f),
    viewScale(1.0f),
    lightRotationY(56.25f),
    lightRotationZ(-135.0f),
    lightColor(1.0f),
    envColor(1.0f),
    rgbScale(1.0f),
    reflZScale(1.0f),
    waterEnvMapLevel(1.0f),
    waterFormID(0U),
    defaultEnvMapNum(0),
    debugMode(0)
{
  threadCnt = int(std::thread::hardware_concurrency());
  threadCnt = (threadCnt > 1 ? (threadCnt < 8 ? threadCnt : 8) : 1);
  renderers.resize(size_t(threadCnt), (Plot3D_TriShape *) 0);
  threadErrMsg.resize(size_t(threadCnt));
  viewOffsetY.resize(size_t(threadCnt + 1), 0);
  threadFileBuffers.resize(size_t(threadCnt));
  setDefaultTextures();
  getWaterMaterial(waterMaterials, *esmFile, (ESMFile::ESMRecord *) 0,
                   defaultWaterColor, true);
  try
  {
    for (size_t i = 0; i < renderers.size(); i++)
    {
      renderers[i] =
          new Plot3D_TriShape((std::uint32_t *) 0, (float *) 0, 0, 0, 4U);
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
}

NIF_View::~NIF_View()
{
  if (nifFile)
  {
    delete nifFile;
    nifFile = (NIFFile *) 0;
  }
  textureSet.clear();
  for (size_t i = 0; i < renderers.size(); i++)
  {
    if (renderers[i])
      delete renderers[i];
  }
}

void NIF_View::loadModel(const std::string& fileName, int l)
{
  meshData.clear();
  textureSet.shrinkTextureCache();
  if (nifFile)
  {
    delete nifFile;
    nifFile = (NIFFile *) 0;
  }
  if (fileName.empty())
    return;
  bool    isMaterialFile =
      (fileName.starts_with("materials/") && fileName.ends_with(".mat"));
  std::vector< unsigned char >& fileBuf = threadFileBuffers[0];
  if (isMaterialFile)
  {
    ba2File.extractFile(fileBuf,
                        std::string("meshes/test/materialcalibration/"
                                    "materialcalibrationobject.nif"));
  }
  else
  {
    ba2File.extractFile(fileBuf, fileName);
  }
  nifFile = new NIFFile(fileBuf.data(), fileBuf.size(), ba2File, &materials, l);
  try
  {
    nifFile->getMesh(meshData);
    if (isMaterialFile)
    {
      const CE2Material *m = materials.findMaterial(fileName);
      if (m)
      {
        for (size_t i = 0; i < meshData.size(); i++)
          meshData[i].setMaterial(m);
      }
    }
  }
  catch (...)
  {
    meshData.clear();
    delete nifFile;
    nifFile = (NIFFile *) 0;
    throw;
  }
}

static inline float degreesToRadians(float x)
{
  return float(double(x) * (std::atan(1.0) / 45.0));
}

void NIF_View::renderModel(std::uint32_t *outBufRGBA, float *outBufZ,
                           int imageWidth, int imageHeight)
{
  if (!(meshData.size() > 0 &&
        outBufRGBA && outBufZ && imageWidth > 0 && imageHeight >= threadCnt))
  {
    return;
  }
  unsigned int  renderMode = (nifFile->getVersion() < 0x80U ?
                              7U : (nifFile->getVersion() < 0x90U ? 11U : 15U));
  float   y0 = 0.0f;
  for (size_t i = 0; i < renderers.size(); i++)
  {
    int     y0i = roundFloat(y0);
    int     y1i = imageHeight;
    if ((i + 1) < renderers.size())
    {
      float   y1 = float(int(i + 1)) / float(int(renderers.size()));
      if (imageHeight >= 32)
        y1 = ((y1 * 2.0f - 3.0f) * y1 + 2.0f) * y1;
      y1 = y1 * float(imageHeight);
      y1i = roundFloat(y1);
      y0 = y1;
    }
    viewOffsetY[i] = y0i;
    size_t  offs = size_t(y0i) * size_t(imageWidth);
    renderers[i]->setRenderMode(renderMode);
    renderers[i]->setBuffers(outBufRGBA + offs, outBufZ + offs,
                             imageWidth, y1i - y0i);
    renderers[i]->setEnvMapOffset(float(imageWidth) * -0.5f,
                                  float(imageHeight) * -0.5f + float(y0i),
                                  float(imageHeight) * reflZScale);
  }
  viewOffsetY[renderers.size()] = imageHeight;
  setDefaultTextures();
  {
    FloatVector4  ambientLight =
        renderers[0]->cubeMapToAmbient(loadTexture(defaultEnvMap, 0));
    for (size_t i = 0; i < renderers.size(); i++)
    {
      renderers[i]->setLighting(lightColor, ambientLight, envColor, rgbScale);
      renderers[i]->setDebugMode((unsigned int) debugMode, 0);
    }
  }
  modelTransform = NIFFile::NIFVertexTransform(
                       1.0f, degreesToRadians(modelRotationX),
                       degreesToRadians(modelRotationY),
                       degreesToRadians(modelRotationZ), 0.0f, 0.0f, 0.0f);
  viewTransform = NIFFile::NIFVertexTransform(
                      1.0f, degreesToRadians(viewRotationX),
                      degreesToRadians(viewRotationY),
                      degreesToRadians(viewRotationZ), 0.0f, 0.0f, 0.0f);
  {
    NIFFile::NIFVertexTransform
        lightTransform(1.0f, 0.0f, degreesToRadians(lightRotationY),
                       degreesToRadians(lightRotationZ), 0.0f, 0.0f, 0.0f);
    lightX = lightTransform.rotateZX;
    lightY = lightTransform.rotateZY;
    lightZ = lightTransform.rotateZZ;
  }
  {
    NIFFile::NIFVertexTransform t(modelTransform);
    t *= viewTransform;
    NIFFile::NIFBounds  b;
    for (size_t i = 0; i < meshData.size(); i++)
    {
      // ignore if hidden
      if (!(meshData[i].flags & NIFFile::NIFTriShape::Flag_TSHidden))
        meshData[i].calculateBounds(b, &t);
    }
    float   xScale = float(imageWidth) * 0.96875f;
    if (b.xMax() > b.xMin())
      xScale = xScale / (b.xMax() - b.xMin());
    float   yScale = float(imageHeight) * 0.96875f;
    if (b.yMax() > b.yMin())
      yScale = yScale / (b.yMax() - b.yMin());
    float   scale = (xScale < yScale ? xScale : yScale) * viewScale;
    viewTransform.scale = scale;
    viewTransform.offsX = (float(imageWidth) - ((b.xMin() + b.xMax()) * scale))
                          * 0.5f + viewOffsX;
    viewTransform.offsY = (float(imageHeight) - ((b.yMin() + b.yMax()) * scale))
                          * 0.5f + viewOffsY;
    viewTransform.offsZ = viewOffsZ + 1.0f - (b.zMin() * scale);
  }
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
        throw FO76UtilsError(1, threadErrMsg[i].c_str());
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

void NIF_View::setWaterColor(unsigned int watrFormID)
{
  if (watrFormID && esmFile)
  {
    const ESMFile::ESMRecord  *r = esmFile->findRecord(watrFormID);
    if (r && *r == "WATR")
    {
      waterFormID = getWaterMaterial(waterMaterials, *esmFile, r, 0U);
      return;
    }
  }
  waterFormID = 0U;
}

void NIF_View::renderModelToFile(const char *outFileName,
                                 int imageWidth, int imageHeight)
{
  size_t  imageDataSize = size_t(imageWidth << 1) * size_t(imageHeight << 1);
  std::vector< std::uint32_t >  outBufRGBA(imageDataSize, 0U);
  std::vector< float >  outBufZ(imageDataSize, 16777216.0f);
  renderModel(outBufRGBA.data(), outBufZ.data(),
              imageWidth << 1, imageHeight << 1);
  imageDataSize = imageDataSize >> 2;
  std::vector< std::uint32_t >  downsampleBuf(imageDataSize);
  downsample2xFilter(downsampleBuf.data(), outBufRGBA.data(),
                     imageWidth << 1, imageHeight << 1, imageWidth);
  int     pixelFormat =
      (!USE_PIXELFMT_RGB10A2 ?
       DDSInputFile::pixelFormatRGBA32 : DDSInputFile::pixelFormatA2R10G10B10);
  DDSOutputFile outFile(outFileName, imageWidth, imageHeight, pixelFormat);
  outFile.writeImageData(downsampleBuf.data(), imageDataSize, pixelFormat);
}

#ifdef HAVE_SDL2
static void updateRotation(float& rx, float& ry, float& rz,
                           int dx, int dy, int dz,
                           std::string& messageBuf, const char *msg)
{
  rx += (float(dx) * 2.8125f);
  ry += (float(dy) * 2.8125f);
  rz += (float(dz) * 2.8125f);
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

static void updateLightColor(FloatVector4& lightColor, int dR, int dG, int dB,
                             std::string& messageBuf)
{
  int     r = roundFloat(float(std::log2(lightColor[0])) * 16.0f);
  lightColor[0] = float(std::exp2(float(r + dR) * 0.0625f));
  int     g = roundFloat(float(std::log2(lightColor[1])) * 16.0f);
  lightColor[1] = float(std::exp2(float(g + dG) * 0.0625f));
  int     b = roundFloat(float(std::log2(lightColor[2])) * 16.0f);
  lightColor[2] = float(std::exp2(float(b + dB) * 0.0625f));
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
  int     tmp = roundFloat(float(std::log2(s)) * 16.0f);
  s = float(std::exp2(float(tmp + d) * 0.0625f));
  s = (s > minVal ? (s < maxVal ? s : maxVal) : minVal);
  if (!msg)
    return;
  char    buf[64];
  std::snprintf(buf, 64, "%s: %7.4f\n", msg, s);
  buf[63] = '\0';
  messageBuf = buf;
}

static void printViewScale(SDLDisplay& display, float viewScale,
                           const NIFFile::NIFVertexTransform& vt)
{
  float   z = (vt.rotateXX * vt.rotateXX) + (vt.rotateYX * vt.rotateYX);
  z = std::max(z, (vt.rotateXY * vt.rotateXY) + (vt.rotateYY * vt.rotateYY));
  z = std::max(z, (vt.rotateXZ * vt.rotateXZ) + (vt.rotateYZ * vt.rotateYZ));
  z = float(std::sqrt(std::max(z, 0.25f)));
  float   vtScale = vt.scale * z / float(int(display.getIsDownsampled()) + 1);
  char    buf[256];
  std::snprintf(buf, 256, "View scale: %7.4f (1 meter = %7.2f pixels)\n",
                viewScale, vtScale);
  buf[255] = '\0';
  display.printString(buf);
}

static void saveScreenshot(SDLDisplay& display, const std::string& nifFileName,
                           NIF_View *renderer = (NIF_View *) 0)
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
  int     w = display.getWidth() >> int(display.getIsDownsampled());
  int     h = display.getHeight() >> int(display.getIsDownsampled());
  if (!renderer)
  {
    display.blitSurface();
    const std::uint32_t *p = display.lockScreenSurface();
    DDSOutputFile f(fileName.c_str(), w, h, DDSInputFile::pixelFormatRGBA32);
    size_t  pitch = display.getPitch();
    for (int y = 0; y < h; y++, p = p + pitch)
      f.writeImageData(p, size_t(w), DDSInputFile::pixelFormatRGBA32);
    display.unlockScreenSurface();
  }
  else
  {
    renderer->renderModelToFile(fileName.c_str(), w << 1, h << 1);
  }
  display.printString("Saved screenshot to ");
  display.printString(fileName.c_str());
  display.printString("\n");
}

static bool viewModelInfo(SDLDisplay& display, const NIFFile& nifFile)
{
  display.clearTextBuffer();
  display.consolePrint("BS version: 0x%08X\n", nifFile.getVersion());
  display.consolePrint("Author name: %s\n", nifFile.getAuthorName().c_str());
  display.consolePrint("Process script: %s\n",
                       nifFile.getProcessScriptName().c_str());
  display.consolePrint("Export script: %s\n",
                       nifFile.getExportScriptName().c_str());
  display.consolePrint("Block count: %u\n",
                       (unsigned int) nifFile.getBlockCount());
  for (size_t i = 0; i < nifFile.getBlockCount(); i++)
  {
    display.consolePrint("  Block %3d: offset = 0x%08X, size = %7u, "
                         "name = \"%s\", type = %s\n",
                         int(i),
                         (unsigned int) nifFile.getBlockOffset(i),
                         (unsigned int) nifFile.getBlockSize(i),
                         nifFile.getBlockName(i),
                         nifFile.getBlockTypeAsString(i));
    const NIFFile::NIFBlkNiNode *nodeBlock = nifFile.getNode(i);
    const NIFFile::NIFBlkBSTriShape *triShapeBlock = nifFile.getTriShape(i);
    const NIFFile::NIFBlkBSLightingShaderProperty *
        lspBlock = nifFile.getLightingShaderProperty(i);
    const NIFFile::NIFBlkNiAlphaProperty *
        alphaPropertyBlock = nifFile.getAlphaProperty(i);
    if (nodeBlock)
    {
      if (nodeBlock->controller >= 0)
        display.consolePrint("    Controller: %3d\n", nodeBlock->controller);
      if (nodeBlock->collisionObject >= 0)
      {
        display.consolePrint("    Collision object: %3d\n",
                             nodeBlock->collisionObject);
      }
      if (nodeBlock->children.size() > 0)
      {
        display.consolePrint("    Children: ");
        for (size_t j = 0; j < nodeBlock->children.size(); j++)
        {
          display.consolePrint("%s%3u", (!j ? "" : ", "),
                               nodeBlock->children[j]);
        }
        display.consolePrint("\n");
      }
    }
    else if (triShapeBlock)
    {
      if (triShapeBlock->controller >= 0)
      {
        display.consolePrint("    Controller: %3d\n",
                             triShapeBlock->controller);
      }
      display.consolePrint("    Flags: 0x%08X\n", triShapeBlock->flags);
      if (triShapeBlock->collisionObject >= 0)
      {
        display.consolePrint("    Collision object: %3d\n",
                             triShapeBlock->collisionObject);
      }
      if (triShapeBlock->skinID >= 0)
        display.consolePrint("    Skin: %3d\n", triShapeBlock->skinID);
      if (triShapeBlock->shaderProperty >= 0)
      {
        display.consolePrint("    Shader property: %3d\n",
                             triShapeBlock->shaderProperty);
      }
      if (triShapeBlock->alphaProperty >= 0)
      {
        display.consolePrint("    Alpha property: %3d\n",
                             triShapeBlock->alphaProperty);
      }
      if (!triShapeBlock->meshFileName.empty())
      {
        display.consolePrint("    Mesh file: %s\n",
                             triShapeBlock->meshFileName.c_str());
      }
      display.consolePrint("    Vertex count: %lu\n",
                           (unsigned long) triShapeBlock->vertexData.size());
      display.consolePrint("    Triangle count: %lu\n",
                           (unsigned long) triShapeBlock->triangleData.size());
    }
    else if (lspBlock)
    {
      if (lspBlock->controller >= 0)
        display.consolePrint("    Controller: %3d\n", lspBlock->controller);
      if (lspBlock->material)
      {
        std::string tmpBuf("    MaterialFile ");
        lspBlock->material->printObjectInfo(tmpBuf, 4, false);
        std::string lineBuf;
        for (size_t j = 0; j < tmpBuf.length(); )
        {
          size_t  n = tmpBuf.find('\n', j);
          if (n == std::string::npos)
            n = tmpBuf.length();
          lineBuf.assign(tmpBuf, j, n - j);
          j = n + 1;
          display.consolePrint("%s\n", lineBuf.c_str());
        }
      }
    }
    else if (alphaPropertyBlock)
    {
      if (alphaPropertyBlock->controller >= 0)
      {
        display.consolePrint("    Controller: %3d\n",
                             alphaPropertyBlock->controller);
      }
      display.consolePrint("    Flags: 0x%04X\n",
                           (unsigned int) alphaPropertyBlock->flags);
      display.consolePrint("    Alpha threshold: %u\n",
                           (unsigned int) alphaPropertyBlock->alphaThreshold);
    }
  }
  return display.viewTextBuffer();
}

static bool viewMaterialInfo(
    SDLDisplay& display, const CE2MaterialDB& materials,
    const std::string& fileName)
{
  display.clearTextBuffer();
  display.consolePrint("Material path: \"%s\"\n", fileName.c_str());
  const CE2Material *m = materials.findMaterial(fileName);
  if (!m)
  {
    display.consolePrint("Not found in database\n");
  }
  else
  {
    std::string tmpBuf("MaterialFile ");
    m->printObjectInfo(tmpBuf, 0, false);
    std::string lineBuf;
    for (size_t j = 0; j < tmpBuf.length(); )
    {
      size_t  n = tmpBuf.find('\n', j);
      if (n == std::string::npos)
        n = tmpBuf.length();
      lineBuf.assign(tmpBuf, j, n - j);
      j = n + 1;
      display.consolePrint("%s\n", lineBuf.c_str());
    }
  }
  return display.viewTextBuffer();
}

static void printGeometryBlockInfo(
    SDLDisplay& display, int x, int y,
    const NIFFile *nifFile, const std::vector< NIFFile::NIFTriShape >& meshData)
{
  std::uint32_t c =
      display.lockDrawSurface()[size_t(y) * size_t(display.getWidth())
                                + size_t(x)];
  display.unlockDrawSurface();
#if USE_PIXELFMT_RGB10A2
  c = ((c >> 2) & 0xFFU) | ((c >> 4) & 0xFF00U) | ((c >> 6) & 0xFF0000U);
#else
  c = ((c & 0xFFU) << 16) | ((c >> 16) & 0xFFU) | (c & 0xFF00U);
#endif
  c = c ^ 0xFFFFFFFFU;
  c = ((c >> 23) & 0x0001U) | ((c >> 14) & 0x0002U) | ((c >> 5) & 0x0004U)
      | ((c >> 19) & 0x0008U) | ((c >> 10) & 0x0010U) | ((c >> 1) & 0x0020U)
      | ((c >> 15) & 0x0040U) | ((c >> 6) & 0x0080U) | ((c << 3) & 0x0100U)
      | ((c >> 11) & 0x0200U) | ((c >> 2) & 0x0400U) | ((c << 7) & 0x0800U)
      | ((c >> 7) & 0x1000U) | ((c << 2) & 0x2000U) | ((c << 11) & 0x4000U);
  if (!c || c > meshData.size() || !nifFile)
    return;
  c--;
  std::string tmpBuf;
  const std::string *blkName = nifFile->getString(meshData[c].ts->nameID);
  if (blkName)
  {
    tmpBuf += "BSGeometry: \"";
    tmpBuf += *blkName;
    tmpBuf += "\"\n";
  }
  if (!meshData[c].ts->meshFileName.empty())
  {
    tmpBuf += "Mesh file: \"";
    tmpBuf += meshData[c].ts->meshFileName;
    tmpBuf += "\"\n";
  }
  if (meshData[c].ts->shaderProperty >= 0)
  {
    const NIFFile::NIFBlkBSLightingShaderProperty *lspBlock =
        nifFile->getLightingShaderProperty(
            size_t(meshData[c].ts->shaderProperty));
    if (lspBlock)
    {
      blkName = nifFile->getString(lspBlock->nameID);
      if (blkName)
      {
        tmpBuf += "Material path: \"";
        tmpBuf += *blkName;
        tmpBuf += "\"\n";
      }
    }
  }
  if (!tmpBuf.empty())
  {
    display.printString(tmpBuf.c_str());
    display.drawText(0, -1, display.getTextRows(), 0.75f, 1.0f);
    (void) SDL_SetClipboardText(tmpBuf.c_str());
  }
}

static const char *keyboardUsageString =
    "  \033[4m\033[38;5;228m0\033[m "
    "to \033[4m\033[38;5;228m5\033[m                "
    "Set debug render mode.                                          \n"
    "  \033[4m\033[38;5;228m+\033[m, "
    "\033[4m\033[38;5;228m-\033[m                  "
    "Zoom in or out.                                                 \n"
    "  \033[4m\033[38;5;228mKeypad 0, 5\033[m           "
    "Set view from the bottom or top.                                \n"
    "  \033[4m\033[38;5;228mKeypad 1 to 9\033[m         "
    "Set isometric view from the SW to NE (default = NW).            \n"
    "  \033[4m\033[38;5;228mShift + Keypad 0 to 9\033[m "
    "Set side view, or top/bottom view rotated by 45 degrees.        \n"
    "  \033[4m\033[38;5;228mF1\033[m "
    "to \033[4m\033[38;5;228mF8\033[m              "
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
    "  \033[4m\033[38;5;228mHome\033[m                  "
    "Reset rotations.                                                \n"
    "  \033[4m\033[38;5;228mInsert\033[m, "
    "\033[4m\033[38;5;228mDelete\033[m        "
    "Zoom reflected environment in or out.                           \n"
    "  \033[4m\033[38;5;228mCaps Lock\033[m             "
    "Toggle fine adjustment of view and lighting parameters.         \n"
    "  \033[4m\033[38;5;228mPage Up\033[m               "
    "Enable downsampling (slow).                                     \n"
    "  \033[4m\033[38;5;228mPage Down\033[m             "
    "Disable downsampling.                                           \n"
    "  \033[4m\033[38;5;228mB\033[m                     "
    "Toggle black/checkerboard background.                           \n"
    "  \033[4m\033[38;5;228mSpace\033[m, "
    "\033[4m\033[38;5;228mBackspace\033[m      "
    "Load next or previous file matching the pattern.                \n"
    "  \033[4m\033[38;5;228mF9\033[m                    "
    "Select file from list.                                          \n"
    "  \033[4m\033[38;5;228mF12\033[m "
    "or \033[4m\033[38;5;228mPrint Screen\033[m   "
    "Save screenshot.                                                \n"
    "  \033[4m\033[38;5;228mF11\033[m                   "
    "Save high quality screenshot (double resolution and downsample).\n"
    "  \033[4m\033[38;5;228mP\033[m                     "
    "Print current settings and file name.                           \n"
    "  \033[4m\033[38;5;228mV\033[m                     "
    "View detailed model information.                                \n"
    "  \033[4m\033[38;5;228mMouse L button\033[m        "
    "In debug mode 1 only: print the geometry block, mesh and        \n"
    "                        "
    "material path of the selected shape based on the color of the   \n"
    "                        "
    "pixel, and also copy it to the clipboard.                       \n"
    "  \033[4m\033[38;5;228mH\033[m                     "
    "Show help screen.                                               \n"
    "  \033[4m\033[38;5;228mC\033[m                     "
    "Clear messages.                                                 \n"
    "  \033[4m\033[38;5;228mEsc\033[m                   "
    "Quit viewer.                                                    \n";
#endif

bool NIF_View::viewModels(SDLDisplay& display,
                          const std::vector< std::string >& nifFileNames, int l)
{
#ifdef HAVE_SDL2
  if (nifFileNames.size() < 1)
    return true;
  std::vector< SDLDisplay::SDLEvent > eventBuf;
  std::string messageBuf;
  unsigned char quitFlag = 0;   // 1 = Esc key pressed, 2 = window closed
  bool    checkerboardBgnd = true;
  try
  {
    int     imageWidth = display.getWidth();
    int     imageHeight = display.getHeight();
    int     viewRotation = -1;  // < 0: do not change
    float   lightRotationX = 0.0f;
    int     fileNum = 0;
    int     d = 4;              // scale of adjusting parameters
    if (nifFileNames.size() > 10)
    {
      int     n = display.browseFile(nifFileNames, "Select file", fileNum,
                                     0x0B080F04FFFFULL);
      if (n >= 0 && size_t(n) < nifFileNames.size())
        fileNum = n;
      else if (n < -1)
        quitFlag = 2;
    }

    size_t  imageDataSize = size_t(imageWidth) * size_t(imageHeight);
    std::vector< float >  outBufZ(imageDataSize);
    while (!quitFlag)
    {
      messageBuf += nifFileNames[fileNum];
      messageBuf += '\n';
      loadModel(nifFileNames[fileNum], l);

      bool    nextFileFlag = false;
      // 1: screenshot, 2: high quality screenshot, 4: view info, 8: browse file
      unsigned char eventFlags = 0;
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
          if (viewRotation >= 0)
          {
            viewRotationX = viewRotations[viewRotation * 3];
            viewRotationY = viewRotations[viewRotation * 3 + 1];
            viewRotationZ = viewRotations[viewRotation * 3 + 2];
            display.printString(viewRotationMessages[viewRotation]);
            viewRotation = -1;
          }
          display.clearSurface(!checkerboardBgnd ? 0U : 0x02666333U);
          for (size_t i = 0; i < imageDataSize; i++)
            outBufZ[i] = 16777216.0f;
          std::uint32_t *outBufRGBA = display.lockDrawSurface();
          renderModel(outBufRGBA, outBufZ.data(), imageWidth, imageHeight);
          display.unlockDrawSurface();
          if (eventFlags)
          {
            if (eventFlags & 2)
              saveScreenshot(display, nifFileNames[fileNum], this);
            else if (eventFlags & 1)
              saveScreenshot(display, nifFileNames[fileNum]);
            if (eventFlags & 8)
            {
              int     n = display.browseFile(
                              nifFileNames, "Select file", fileNum,
                              0x0B080F04FFFFULL);
              if (n >= 0 && size_t(n) < nifFileNames.size() && n != fileNum)
              {
                fileNum = n;
                nextFileFlag = true;
              }
              else if (n < -1)
              {
                quitFlag = 2;
              }
            }
            else if ((eventFlags & 4) && nifFile)
            {
              const std::string&  fileName = nifFileNames[fileNum];
              if (fileName.starts_with("materials/") &&
                  fileName.ends_with(".mat"))
              {
                if (!viewMaterialInfo(display, materials, fileName))
                  quitFlag = 2;
              }
              else if (!viewModelInfo(display, *nifFile))
              {
                quitFlag = 2;
              }
              display.clearTextBuffer();
            }
            eventFlags = 0;
          }
          display.drawText(0, -1, display.getTextRows(), 0.75f, 1.0f);
          redrawFlags = 1;
        }
        display.blitSurface();
        redrawFlags = 0;

        while (!(redrawFlags || nextFileFlag || quitFlag))
        {
          display.pollEvents(eventBuf, -1000, false, true);
          for (size_t i = 0; i < eventBuf.size(); i++)
          {
            int     t = eventBuf[i].type();
            int     d1 = eventBuf[i].data1();
            if (t == SDLDisplay::SDLEventWindow)
            {
              if (d1 == 0)
                quitFlag = 2;
              else if (d1 == 1)
                redrawFlags = redrawFlags | 1;
              continue;
            }
            if (!(t == SDLDisplay::SDLEventKeyRepeat ||
                  t == SDLDisplay::SDLEventKeyDown))
            {
              if (t == SDLDisplay::SDLEventMButtonDown)
              {
                int     x = std::min(std::max(d1, 0), imageWidth - 1);
                int     y = eventBuf[i].data2();
                y = std::min(std::max(y, 0), imageHeight - 1);
                if (debugMode == 1 &&
                    eventBuf[i].data3() == 1 && eventBuf[i].data4() == 1)
                {
                  printGeometryBlockInfo(display, x, y, nifFile, meshData);
                  redrawFlags = redrawFlags | 1;
                }
              }
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
                debugMode = int(d1 - '0');
                messageBuf += "Debug mode set to ";
                messageBuf += char(d1);
                messageBuf += '\n';
                break;
              case '-':
              case SDLDisplay::SDLKeySymKPMinus:
                updateValueLogScale(viewScale, -d, 0.0625f, 16.0f, messageBuf,
                                    "View scale");
                break;
              case '=':
              case SDLDisplay::SDLKeySymKPPlus:
                updateValueLogScale(viewScale, d, 0.0625f, 16.0f, messageBuf,
                                    "View scale");
                break;
              case SDLDisplay::SDLKeySymKP1:
              case SDLDisplay::SDLKeySymKP2:
              case SDLDisplay::SDLKeySymKP3:
              case SDLDisplay::SDLKeySymKP4:
              case SDLDisplay::SDLKeySymKP5:
              case SDLDisplay::SDLKeySymKP6:
              case SDLDisplay::SDLKeySymKP7:
              case SDLDisplay::SDLKeySymKP8:
              case SDLDisplay::SDLKeySymKP9:
              case SDLDisplay::SDLKeySymKPIns:
                viewRotation = getViewRotation(d1, eventBuf[i].data2());
                break;
              case SDLDisplay::SDLKeySymF1:
              case SDLDisplay::SDLKeySymF2:
              case SDLDisplay::SDLKeySymF3:
              case SDLDisplay::SDLKeySymF4:
              case SDLDisplay::SDLKeySymF5:
              case SDLDisplay::SDLKeySymF6:
              case SDLDisplay::SDLKeySymF7:
              case SDLDisplay::SDLKeySymF8:
                defaultEnvMapNum = d1 - SDLDisplay::SDLKeySymF1;
                setDefaultTextures();
                messageBuf += "Default environment map: ";
                messageBuf += defaultEnvMap;
                messageBuf += '\n';
                break;
              case SDLDisplay::SDLKeySymF9:
                eventFlags = eventFlags | 8;    // browse file list
                break;
              case 'a':
                updateRotation(modelRotationX, modelRotationY, modelRotationZ,
                               0, 0, d, messageBuf, "Model rotation");
                break;
              case 'd':
                updateRotation(modelRotationX, modelRotationY, modelRotationZ,
                               0, 0, -d, messageBuf, "Model rotation");
                break;
              case 's':
                updateRotation(modelRotationX, modelRotationY, modelRotationZ,
                               d, 0, 0, messageBuf, "Model rotation");
                break;
              case 'w':
                updateRotation(modelRotationX, modelRotationY, modelRotationZ,
                               -d, 0, 0, messageBuf, "Model rotation");
                break;
              case 'q':
                updateRotation(modelRotationX, modelRotationY, modelRotationZ,
                               0, -d, 0, messageBuf, "Model rotation");
                break;
              case 'e':
                updateRotation(modelRotationX, modelRotationY, modelRotationZ,
                               0, d, 0, messageBuf, "Model rotation");
                break;
              case 'k':
                updateValueLogScale(rgbScale, -d, 0.0625f, 16.0f, messageBuf,
                                    "Brightness (linear color space)");
                break;
              case 'l':
                updateValueLogScale(rgbScale, d, 0.0625f, 16.0f, messageBuf,
                                    "Brightness (linear color space)");
                break;
              case SDLDisplay::SDLKeySymLeft:
                updateRotation(lightRotationX, lightRotationY, lightRotationZ,
                               0, 0, d, messageBuf, "Light rotation");
                break;
              case SDLDisplay::SDLKeySymRight:
                updateRotation(lightRotationX, lightRotationY, lightRotationZ,
                               0, 0, -d, messageBuf, "Light rotation");
                break;
              case SDLDisplay::SDLKeySymDown:
                updateRotation(lightRotationX, lightRotationY, lightRotationZ,
                               0, d, 0, messageBuf, "Light rotation");
                break;
              case SDLDisplay::SDLKeySymUp:
                updateRotation(lightRotationX, lightRotationY, lightRotationZ,
                               0, -d, 0, messageBuf, "Light rotation");
                break;
              case SDLDisplay::SDLKeySymHome:
                messageBuf = "Model and light rotations reset to defaults";
                modelRotationX = 0.0f;
                modelRotationY = 0.0f;
                modelRotationZ = 0.0f;
                lightRotationY = 56.25f;
                lightRotationZ = -135.0f;
                break;
              case '7':
                updateLightColor(lightColor, d, 0, 0, messageBuf);
                break;
              case 'u':
                updateLightColor(lightColor, -d, 0, 0, messageBuf);
                break;
              case '8':
                updateLightColor(lightColor, 0, d, 0, messageBuf);
                break;
              case 'i':
                updateLightColor(lightColor, 0, -d, 0, messageBuf);
                break;
              case '9':
                updateLightColor(lightColor, 0, 0, d, messageBuf);
                break;
              case 'o':
                updateLightColor(lightColor, 0, 0, -d, messageBuf);
                break;
              case SDLDisplay::SDLKeySymInsert:
                updateValueLogScale(reflZScale, d, 0.25f, 8.0f, messageBuf,
                                    "Reflection f scale");
                break;
              case SDLDisplay::SDLKeySymDelete:
                updateValueLogScale(reflZScale, -d, 0.25f, 8.0f, messageBuf,
                                    "Reflection f scale");
                break;
              case SDLDisplay::SDLKeySymCapsLock:
                d = (d == 1 ? 4 : 1);
                if (d == 1)
                  messageBuf += "Step size: 2.8125\302\260, exp2(1/16)\n";
                else
                  messageBuf += "Step size: 11.25\302\260, exp2(1/4)\n";
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
              case 'b':
                checkerboardBgnd = !checkerboardBgnd;
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
              case SDLDisplay::SDLKeySymF11:
                eventFlags = eventFlags | 2;    // high quality screenshot
                break;
              case SDLDisplay::SDLKeySymF12:
              case SDLDisplay::SDLKeySymPrintScr:
                eventFlags = eventFlags | 1;    // screenshot
                break;
              case 'p':
                display.clearTextBuffer();
                updateRotation(modelRotationX, modelRotationY, modelRotationZ,
                               0, 0, 0, messageBuf, "Model rotation");
                display.printString(messageBuf.c_str());
                updateRotation(lightRotationX, lightRotationY, lightRotationZ,
                               0, 0, 0, messageBuf, "Light rotation");
                display.printString(messageBuf.c_str());
                updateValueLogScale(rgbScale, 0, 0.25f, 4.0f, messageBuf,
                                    "Brightness (linear color space)");
                display.printString(messageBuf.c_str());
                updateLightColor(lightColor, 0, 0, 0, messageBuf);
                display.printString(messageBuf.c_str());
                printViewScale(display, viewScale, viewTransform);
                updateValueLogScale(reflZScale, 0, 0.5f, 4.0f, messageBuf,
                                    "Reflection f scale");
                if (d == 1)
                  messageBuf += "Step size: 2.8125\302\260, exp2(1/16)\n";
                else
                  messageBuf += "Step size: 11.25\302\260, exp2(1/4)\n";
                if (display.getIsDownsampled())
                  messageBuf += "Downsampling enabled\n";
                else
                  messageBuf += "Downsampling disabled\n";
                messageBuf += "Default environment map: ";
                messageBuf += defaultEnvMap;
                messageBuf += "\nFile name:\n  \033[44m\033[37m\033[1m";
                messageBuf += nifFileNames[fileNum];
                messageBuf += "\033[m  \n";
                continue;
              case 'v':
                eventFlags = eventFlags | 4;    // view model info
                break;
              case 'h':
                messageBuf = keyboardUsageString;
                break;
              case 'c':
                break;
              case SDLDisplay::SDLKeySymEscape:
                quitFlag = 1;
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
      display.pollEvents(eventBuf);
      for (size_t i = 0; i < eventBuf.size(); i++)
      {
        if ((eventBuf[i].type() == SDLDisplay::SDLEventWindow &&
             eventBuf[i].data1() == 0) ||
            eventBuf[i].type() == SDLDisplay::SDLEventKeyDown)
        {
          quitFlag = (eventBuf[i].type() == SDLDisplay::SDLEventWindow ? 2 : 1);
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
  }
  return (quitFlag < 2);
#else
  (void) display;
  (void) nifFileNames;
  return false;
#endif
}

