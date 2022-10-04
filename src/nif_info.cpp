
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
    throw FO76UtilsError("%s: end of input file", nifFileName);
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
      std::fprintf(f, "    Material version: %2u\n",
                   (unsigned int) lspBlock->material.version);
      std::fprintf(f, "    Material flags: 0x%04X\n",
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
      unsigned int  emissiveColor = lspBlock->material.emissiveColor;
      double  emissiveScale = double(int(emissiveColor >> 24)) / 128.0;
      emissiveColor = emissiveColor & 0x00FFFFFFU;
      if (lspBlock->material.flags & BGSMFile::Flag_TSWater)
      {
        std::fprintf(f, "    Material water color (0xBBGGRR): 0x%06X\n",
                     emissiveColor);
        std::fprintf(f, "    Material water depth: %.3f\n",
                     (2.0 - emissiveScale) * (2.0 - emissiveScale) * 2048.0);
      }
      else if (lspBlock->material.flags & BGSMFile::Flag_IsEffect)
      {
        std::fprintf(f, "    Material base color (0xBBGGRR): 0x%06X\n",
                     emissiveColor);
        std::fprintf(f, "    Material base color scale: %.3f\n", emissiveScale);
      }
      else
      {
        std::fprintf(f, "    Material emissive color (0xBBGGRR): 0x%06X\n",
                     emissiveColor);
        std::fprintf(f, "    Material emissive scale: %.3f\n", emissiveScale);
      }
      unsigned int  m = lspBlock->material.texturePathMask;
      for (size_t j = 0; m; j++, m = m >> 1)
      {
        if (m & 1U)
        {
          std::fprintf(f, "    Material texture %d: %s\n",
                       int(j), lspBlock->texturePaths[j]->c_str());
        }
      }
    }
    else if (shaderTextureSetBlock)
    {
      unsigned int  m = shaderTextureSetBlock->texturePathMask;
      for (size_t j = 0; m; j++, m = m >> 1)
      {
        if (m & 1U)
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
    if (ts.m.flags)
    {
      static const char *flagNames[16] =
      {
        "tile U", "tile V", "is effect", "decal", "two sided", "tree",
        "grayscale to alpha", "glow", "no Z buffer write", "", "", "",
        "alpha blending", "has vertex colors", "is water", "hidden"
      };
      std::fprintf(f, "  Flags: ");
      std::uint16_t m = 0x0001;
      std::uint16_t mPrv = 0x0001;
      for (int j = 0; j < 8; j++, m = m << 1, mPrv = (mPrv << 1) | 1)
      {
        if ((ts.m.flags & mPrv) > m)
          std::fprintf(f, ", ");
        if (ts.m.flags & m)
          std::fprintf(f, "%s", flagNames[j]);
      }
      std::fputc('\n', f);
    }
    float   alphaThreshold = ts.m.getAlphaThreshold();
    alphaThreshold = (alphaThreshold < 256.0f ? alphaThreshold : 256.0f);
    if (alphaThreshold > 0.0f)
      std::fprintf(f, "  Alpha threshold: %.3f\n", alphaThreshold);
    if (ts.m.flags & BGSMFile::Flag_TSAlphaBlending)
    {
      std::fprintf(f, "  Alpha blend scale: %.3f\n",
                   double(int(ts.m.alpha)) / 128.0);
    }
    printVertexTransform(f, ts.vertexTransform);
    if (ts.haveMaterialPath())
      std::fprintf(f, "  Material: %s\n", ts.materialPath().c_str());
    std::fprintf(f, "  Texture UV offset, scale: (%f, %f), (%f, %f)\n",
                 ts.m.textureOffsetU, ts.m.textureOffsetV,
                 ts.m.textureScaleU, ts.m.textureScaleV);
    unsigned int  m = ts.m.texturePathMask;
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
    FloatVector4  specularColor(ts.m.specularColor);
    float   specularScale = specularColor[3] / (128.0f * 255.0f);
    float   specularGlossiness = float(int(ts.m.specularSmoothness));
    if (ts.m.texturePathMask & 0x0240)
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
      if (ts.m.texturePathMask & (1U << (unsigned int) j))
      {
        std::fprintf(f, "%s %s\n",
                     (!j ? "map_Kd" : "map_Kn"), ts.texturePaths[j]->c_str());
      }
    }
    std::fprintf(f, "\n");
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
          errorMessage("invalid image dimensions");
        }
        if (outFmt == 5)
        {
          if (argc < 3)
            errorMessage("missing output file name for -render");
          outFileName = argv[2];
          argc--;
          argv++;
        }
      }
      else if (std::strcmp(argv[1], "-o") == 0)
      {
        if (argc < 3)
          errorMessage("missing output file name");
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
        throw FO76UtilsError("invalid option: %s", argv[1]);
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
      if (fileNames.size() > 0)
      {
        std::sort(fileNames.begin(), fileNames.end());
        SDLDisplay  display(renderWidth, renderHeight, "nif_info", 4U, 48);
        display.setDefaultTextColor(0x00, 0xC1);
        Renderer::viewMeshes(display, ba2File, fileNames);
      }
      return 0;
    }
    if (outFileName)
    {
      outFile = std::fopen(outFileName, (outFmt != 5 ? "w" : "wb"));
      if (!outFile)
        errorMessage("error opening output file");
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
        Renderer::renderMeshToFile(outFileName, nifFile, ba2File,
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

