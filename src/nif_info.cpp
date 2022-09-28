
#include "common.hpp"
#include "nif_file.hpp"
#include "ba2file.hpp"
#include "ddstxt.hpp"
#include "plot3d.hpp"
#include "sdlvideo.hpp"
#include "nif_view.hpp"

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
      std::fprintf(f, "    Material version: 0x%02X\n",
                   (unsigned int) lspBlock->material.version);
      std::fprintf(f, "    Material flags: 0x%02X\n",
                   (unsigned int) lspBlock->material.flags);
      std::fprintf(f, "    Material alpha flags: 0x%04X\n",
                   (unsigned int) lspBlock->material.alphaFlags);
      std::fprintf(f, "    Material alpha threshold: %3u\n",
                   (unsigned int) lspBlock->material.alphaThreshold);
      std::fprintf(f, "    Material alpha: %.3f\n",
                   double(int(lspBlock->material.alpha)) / 128.0);
      std::fprintf(f, "    Material gradient map scale: %.3f\n",
                   double(int(lspBlock->material.gradientMapV)) / 255.0);
      std::fprintf(f, "    Material environment map scale: %.3f\n",
                   double(int(lspBlock->material.envMapScale)) / 128.0);
      std::fprintf(f, "    Material specular color (0xBBGGRR): 0x%06X\n",
                   (unsigned int) lspBlock->material.specularColor & 0xFFFFFFU);
      std::fprintf(f, "    Material specular scale: %.3f\n",
                   double(int(lspBlock->material.specularColor >> 24)) / 128.0);
      std::fprintf(f, "    Material specular smoothness: %.3f\n",
                   double(int(lspBlock->material.specularSmoothness)) / 255.0);
      for (size_t j = 0; j < lspBlock->texturePaths.size(); j++)
      {
        if (lspBlock->material.texturePathMask & (1U << (unsigned int) j))
        {
          std::fprintf(f, "    Material texture %d: %s\n",
                       int(j), lspBlock->texturePaths[j]->c_str());
        }
      }
    }
    else if (shaderTextureSetBlock)
    {
      for (size_t j = 0; j < shaderTextureSetBlock->texturePaths.size(); j++)
      {
        if (shaderTextureSetBlock->texturePathMask & (1U << (unsigned int) j))
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
    const NIFFile::NIFTriShape& ts = meshData[i];
    std::fprintf(f, "TriShape %3d (%s):\n", int(i), ts.name);
    std::fprintf(f, "  Vertex count: %u\n", ts.vertexCnt);
    std::fprintf(f, "  Triangle count: %u\n", ts.triangleCnt);
    if (ts.flags)
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
        if ((ts.flags & mPrv) > m)
          std::fprintf(f, ", ");
        if (ts.flags & m)
          std::fprintf(f, "%s", flagNames[j]);
      }
      std::fputc('\n', f);
    }
    if (ts.alphaThreshold)
      std::fprintf(f, "  Alpha threshold: %d\n", int(ts.alphaThreshold));
    if (ts.alphaBlendScale)
    {
      std::fprintf(f, "  Alpha blend scale: %.3f\n",
                   double(int(ts.alphaBlendScale)) / 128.0);
    }
    printVertexTransform(f, ts.vertexTransform);
    if (ts.materialPath)
      std::fprintf(f, "  Material: %s\n", ts.materialPath->c_str());
    std::fprintf(f, "  Texture UV offset, scale: (%f, %f), (%f, %f)\n",
                 ts.textureOffsetU, ts.textureOffsetV,
                 ts.textureScaleU, ts.textureScaleV);
    unsigned int  m = ts.texturePathMask;
    for (int j = 0; m; j++, m = m >> 1)
    {
      if (m & 1U)
      {
        std::fprintf(f, "  Texture %2d: %s\n",
                     j, ts.texturePaths[j]->c_str());
      }
    }
    std::fprintf(f, "  Vertex list:\n");
    for (size_t j = 0; j < ts.vertexCnt; j++)
    {
      NIFFile::NIFVertex  v = ts.vertexData[j];
      ts.vertexTransform.transformXYZ(v.x, v.y, v.z);
      FloatVector4  normal(ts.vertexTransform.rotateXYZ(v.getNormal()));
      std::fprintf(f, "    %4d: XYZ: (%f, %f, %f), normals: (%f, %f, %f), "
                   "UV: (%f, %f), color: 0x%08X\n",
                   int(j), v.x, v.y, v.z, normal[0], normal[1], normal[2],
                   v.getU(), v.getV(), (unsigned int) v.vertexColor);
    }
    std::fprintf(f, "  Triangle list:\n");
    for (size_t j = 0; j < ts.triangleCnt; j++)
    {
      std::fprintf(f, "    %4d: %4u, %4u, %4u\n",
                   int(j), ts.triangleData[j].v0,
                   ts.triangleData[j].v1, ts.triangleData[j].v2);
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
    const NIFFile::NIFTriShape& ts = meshData[i];
    std::fprintf(f, "# %s\n\ng %s\n", ts.name, ts.name);
    std::fprintf(f, "usemtl Material%06u\n\n", (unsigned int) (i + 1));
    for (size_t j = 0; j < ts.vertexCnt; j++)
    {
      NIFFile::NIFVertex  v = ts.vertexData[j];
      ts.vertexTransform.transformXYZ(v.x, v.y, v.z);
      std::fprintf(f, "v %.8f %.8f %.8f\n", v.x, v.y, v.z);
    }
    for (size_t j = 0; j < ts.vertexCnt; j++)
    {
      NIFFile::NIFVertex  v = ts.vertexData[j];
      std::fprintf(f, "vt %.8f %.8f\n", v.getU(), 1.0 - v.getV());
    }
    for (size_t j = 0; j < ts.vertexCnt; j++)
    {
      FloatVector4  normal(ts.vertexTransform.rotateXYZ(
                               ts.vertexData[j].getNormal()));
      std::fprintf(f, "vn %.8f %.8f %.8f\n", normal[0], normal[1], normal[2]);
    }
    for (size_t j = 0; j < ts.triangleCnt; j++)
    {
      unsigned int  v0 = ts.triangleData[j].v0 + vertexNumBase;
      unsigned int  v1 = ts.triangleData[j].v1 + vertexNumBase;
      unsigned int  v2 = ts.triangleData[j].v2 + vertexNumBase;
      std::fprintf(f, "f %u/%u/%u %u/%u/%u %u/%u/%u\n",
                   v0, v0, v0, v1, v1, v1, v2, v2, v2);
    }
    vertexNumBase = vertexNumBase + ts.vertexCnt;
    std::fprintf(f, "\n");
  }
}

static void printMTLData(std::FILE *f, const NIFFile& nifFile)
{
  std::vector< NIFFile::NIFTriShape > meshData;
  nifFile.getMesh(meshData);
  for (size_t i = 0; i < meshData.size(); i++)
  {
    const NIFFile::NIFTriShape& ts = meshData[i];
    std::fprintf(f, "newmtl Material%06u\n", (unsigned int) (i + 1));
    std::fprintf(f, "Ka 1.0 1.0 1.0\n");
    std::fprintf(f, "Kd 1.0 1.0 1.0\n");
    FloatVector4  specularColor(ts.specularColor);
    float   specularScale = specularColor[3] / (128.0f * 255.0f);
    float   specularGlossiness = float(int(ts.specularSmoothness));
    if (ts.texturePathMask & 0x0240)
    {
      specularScale *= 0.5f;
      specularGlossiness *= 0.5f;
    }
    specularColor *= specularScale;
    specularGlossiness =
        float(std::pow(2.0f, specularGlossiness * (9.0f / 255.0f) + 1.0f));
    std::fprintf(f, "Ks %.3f %.3f %.3f\n",
                 specularColor[0], specularColor[1], specularColor[2]);
    std::fprintf(f, "d 1.0\n");
    std::fprintf(f, "Ns %.1f\n", specularGlossiness);
    for (size_t j = 0; j < 2; j++)
    {
      if (ts.texturePathMask & (1U << (unsigned int) j))
      {
        std::fprintf(f, "%s %s\n",
                     (!j ? "map_Kd" : "map_Kn"), ts.texturePaths[j]->c_str());
      }
    }
    std::fprintf(f, "\n");
  }
}

static const char *cubeMapPaths[24] =
{
  "textures/cubemaps/bleakfallscube_e.dds",                     // Skyrim
  "textures/shared/cubemaps/mipblur_defaultoutside1.dds",       // Fallout 4
  "textures/shared/cubemaps/mipblur_defaultoutside1.dds",       // Fallout 76
  "textures/cubemaps/wrtemple_e.dds",
  "textures/shared/cubemaps/outsideoldtownreflectcube_e.dds",
  "textures/shared/cubemaps/outsideoldtownreflectcube_e.dds",
  "textures/cubemaps/duncaveruingreen_e.dds",
  "textures/shared/cubemaps/cgprewarstreet_e.dds",
  "textures/shared/cubemaps/swampcube.dds",
  "textures/cubemaps/chrome_e.dds",
  "textures/shared/cubemaps/metalchrome01cube_e.dds",
  "textures/shared/cubemaps/metalchrome01cube_e.dds",
  "textures/cubemaps/cavegreencube_e.dds",
  "textures/shared/cubemaps/outsideday01.dds",
  "textures/shared/cubemaps/outsideday01.dds",
  "textures/cubemaps/mghallcube_e.dds",
  "textures/shared/cubemaps/cgplayerhousecube.dds",
  "textures/shared/cubemaps/chrome_e.dds",
  "textures/cubemaps/caveicecubemap_e.dds",
  "textures/shared/cubemaps/inssynthproductionpoolcube.dds",
  "textures/shared/cubemaps/vault111cryocube.dds",
  "textures/cubemaps/minecube_e.dds",
  "textures/shared/cubemaps/memorydencube.dds",
  "textures/shared/cubemaps/mipblur_defaultoutside_pitt.dds"
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
                       imageWidth, imageHeight, nifFile.getVersion());
    renderer.ba2File = &ba2File;
    nifFile.getMesh(renderer.meshData);
    renderer.waterTexture = "textures/water/defaultwater.dds";
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
        // ignore if hidden
        if (!(renderer.meshData[i].flags & 0x01))
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
      FloatVector4  a(renderer.renderers[0]->cubeMapToAmbient(
                          renderer.loadTexture(&(renderer.defaultEnvMap))));
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
#if USE_PIXELFMT_RGB10A2
  DDSOutputFile outFile(outFileName, w, h,
                        DDSInputFile::pixelFormatR10G10B10A2);
  outFile.writeImageData(&(downsampleBuf.front()), imageDataSize,
                         DDSInputFile::pixelFormatR10G10B10A2);
#else
  DDSOutputFile outFile(outFileName, w, h, DDSInputFile::pixelFormatRGB24);
  outFile.writeImageData(&(downsampleBuf.front()), imageDataSize,
                         DDSInputFile::pixelFormatRGB24);
#endif
}

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
    "  \033[4m\033[38;5;228mInsert\033[m, "
    "\033[4m\033[38;5;228mDelete\033[m        "
    "Zoom reflected environment in or out.                           \n"
    "  \033[4m\033[38;5;228mCaps Lock\033[m             "
    "Toggle fine adjustment of view and lighting parameters.         \n"
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
    "  \033[4m\033[38;5;228mP\033[m                     "
    "Print current settings and file list.                           \n"
    "  \033[4m\033[38;5;228mH\033[m                     "
    "Show help screen.                                               \n"
    "  \033[4m\033[38;5;228mC\033[m                     "
    "Clear messages.                                                 \n"
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
    int     d = 4;              // scale of adjusting parameters

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
                         imageWidth, imageHeight, nifFile.getVersion());
      renderer.ba2File = &ba2File;
      nifFile.getMesh(renderer.meshData);
      renderer.waterTexture = "textures/water/defaultwater.dds";

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
            // ignore if hidden
            if (!(renderer.meshData[i].flags & 0x01))
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
          FloatVector4  a(renderer.renderers[0]->cubeMapToAmbient(
                              renderer.loadTexture(&(renderer.defaultEnvMap))));
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
                updateValueLogScale(viewScale, -d, 0.0625f, 16.0f, messageBuf,
                                    "View scale");
                break;
              case '=':
              case SDLDisplay::SDLKeySymKPPlus:
                updateValueLogScale(viewScale, d, 0.0625f, 16.0f, messageBuf,
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
              case SDLDisplay::SDLKeySymF1 + 5:
              case SDLDisplay::SDLKeySymF1 + 6:
              case SDLDisplay::SDLKeySymF1 + 7:
                envMapNum = d1 - SDLDisplay::SDLKeySymF1;
                messageBuf += "Default environment map: ";
                messageBuf += cubeMapPaths[envMapNum * 3
                                           + int(nifFile.getVersion() >= 0x80)
                                           + int(nifFile.getVersion() >= 0x90)];
                messageBuf += '\n';
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
                updateValueLogScale(lightLevel, -d, 0.25f, 4.0f, messageBuf,
                                    "Brightness (linear color space)");
                break;
              case 'l':
                updateValueLogScale(lightLevel, d, 0.25f, 4.0f, messageBuf,
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
                updateValueLogScale(envMapScale, d, 0.5f, 4.0f, messageBuf,
                                    "Reflection f scale");
                break;
              case SDLDisplay::SDLKeySymDelete:
                updateValueLogScale(envMapScale, -d, 0.5f, 4.0f, messageBuf,
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
              case 'p':
                display.clearTextBuffer();
                updateRotation(modelRotationX, modelRotationY, modelRotationZ,
                               0, 0, 0, messageBuf, "Model rotation");
                display.printString(messageBuf.c_str());
                updateRotation(lightRotationX, lightRotationY, lightRotationZ,
                               0, 0, 0, messageBuf, "Light rotation");
                display.printString(messageBuf.c_str());
                updateValueLogScale(lightLevel, 0, 0.25f, 4.0f, messageBuf,
                                    "Brightness (linear color space)");
                display.printString(messageBuf.c_str());
                updateLightColor(lightColor, 0, 0, 0, messageBuf);
                display.printString(messageBuf.c_str());
                updateValueLogScale(viewScale, 0, 0.0625f, 16.0f, messageBuf,
                                    "View scale");
                display.printString(messageBuf.c_str());
                updateValueLogScale(envMapScale, 0, 0.5f, 4.0f, messageBuf,
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
                messageBuf += cubeMapPaths[envMapNum * 3
                                           + int(nifFile.getVersion() >= 0x80)
                                           + int(nifFile.getVersion() >= 0x90)];
                messageBuf += "\nFile list:\n";
                {
                  int     n0 = 0;
                  int     n1 = int(nifFileNames.size());
                  while ((n1 - n0) > (display.getTextRows() - 12))
                  {
                    if (fileNum < ((n0 + n1) >> 1))
                      n1--;
                    else
                      n0++;
                  }
                  for ( ; n0 < n1; n0++)
                  {
                    messageBuf += (n0 != fileNum ?
                                   "  " : "  \033[44m\033[37m\033[1m");
                    messageBuf += nifFileNames[n0];
                    messageBuf += "\033[m  \n";
                  }
                }
                continue;
              case 'h':
                messageBuf = keyboardUsageString;
                break;
              case 'c':
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
  std::FILE   *outFile = (std::FILE *) 0;
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
      else if (std::strcmp(argv[1], "-o") == 0)
      {
        if (argc < 3)
          throw errorMessage("missing output file name");
        outFileName = argv[2];
        argc--;
        argv++;
      }
      else if (std::strcmp(argv[1], "-h") == 0 ||
               std::strcmp(argv[1], "--help") == 0)
      {
        argc = 1;
        outFmt = 0;
        break;
      }
      else
      {
        throw errorMessage("invalid option: %s", argv[1]);
      }
    }
    consoleFlag = false;
    if (outFmt != 6)
      SDLDisplay::enableConsole();
    if (argc < 3)
    {
      std::fprintf(stderr,
                   "Usage: nif_info [OPTIONS] ARCHIVEPATH PATTERN...\n");
      std::fprintf(stderr, "Options:\n");
      std::fprintf(stderr, "    --      Remaining options are file names\n");
      std::fprintf(stderr, "    -o FILENAME     Set output file name "
                           "(default: standard output)\n");
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
    std::vector< std::string >  fileNames;
    for (int i = 2; i < argc; i++)
      fileNames.push_back(argv[i]);
    fileNames.push_back(".bgem");
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
    if (outFileName)
    {
      outFile = std::fopen(outFileName, (outFmt != 5 ? "w" : "wb"));
      if (!outFile)
        throw errorMessage("error opening output file");
    }
    else
    {
      outFile = stdout;
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
    if (outFileName && outFile)
      std::fclose(outFile);
    std::fprintf(stderr, "nif_info: %s\n", e.what());
    return 1;
  }
  if (outFileName && outFile)
    std::fclose(outFile);
  return 0;
}

