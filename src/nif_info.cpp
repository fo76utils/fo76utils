
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
  if (std::memcmp(fileBuf.data(), "Gamebryo File Format, Version 20", 32) != 0)
    return;
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

static void printBlockList(std::FILE *f, const NIFFile& nifFile,
                           bool verboseMaterialInfo)
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
      if (!triShapeBlock->meshFileName.empty())
      {
        std::fprintf(f, "    Mesh file: %s\n",
                     triShapeBlock->meshFileName.c_str());
      }
    }
    else if (lspBlock)
    {
      if (lspBlock->controller >= 0)
        std::fprintf(f, "    Controller: %3d\n", lspBlock->controller);
      if (verboseMaterialInfo && lspBlock->material)
      {
        std::string tmpBuf("    MaterialFile ");
        lspBlock->material->printObjectInfo(tmpBuf, 4, false);
        std::fwrite(tmpBuf.c_str(), sizeof(char), tmpBuf.length(), f);
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
    const char  *tsName = "";
    if (ts.ts && nifFile.getString(ts.ts->nameID))
      tsName = nifFile.getString(ts.ts->nameID)->c_str();
    std::fprintf(f, "TriShape %3d (%s):\n", int(i), tsName);
    std::fprintf(f, "  Vertex count: %u\n", ts.vertexCnt);
    std::fprintf(f, "  Triangle count: %u\n", ts.triangleCnt);
    if (ts.flags)
    {
      std::uint32_t m = ts.flags;
      for (unsigned int j = 0U; m; j++, m = m >> 1)
      {
        if (!(m & 1U))
          continue;
        if ((m << j) != ts.flags)
          std::fprintf(f, ", %s", CE2Material::materialFlagNames[j]);
        else
          std::fprintf(f, "  Flags: %s", CE2Material::materialFlagNames[j]);
      }
      std::fputc('\n', f);
    }
    if (ts.m && ts.m->alphaThreshold > 0.0f)
      std::fprintf(f, "  Alpha threshold: %.3f\n", ts.m->alphaThreshold);
    printVertexTransform(f, ts.vertexTransform);
    if (ts.m)
      std::fprintf(f, "  Material: %s\n", ts.m->name->c_str());
    std::fprintf(f, "  Vertex list:\n");
    for (size_t j = 0; j < ts.vertexCnt; j++)
    {
      NIFFile::NIFVertex  v = ts.vertexData[j];
      v.xyz = ts.vertexTransform.transformXYZ(v.xyz);
      FloatVector4  normal(ts.vertexTransform.rotateXYZ(v.getNormal()));
      std::fprintf(f, "    %4d: XYZ: (%f, %f, %f), normals: (%f, %f, %f), "
                   "UV: (%f, %f), color: 0x%08X\n",
                   int(j), v.xyz[0], v.xyz[1], v.xyz[2],
                   normal[0], normal[1], normal[2],
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

struct ObjFileMaterialNames
{
  std::set< std::string > namesUsed;
  std::map< const CE2Material *, std::string >  nameMap;
  bool getMaterialName(std::string& s, const NIFFile::NIFTriShape& ts);
};

bool ObjFileMaterialNames::getMaterialName(
    std::string& s, const NIFFile::NIFTriShape& ts)
{
  std::map< const CE2Material *, std::string >::const_iterator  i =
      nameMap.find(ts.m);
  if (i != nameMap.end())
  {
    s = i->second;
    return false;
  }
  if (!(ts.m && !ts.m->name->empty()))
    s = "DefaultMaterial";
  else
    s = *(ts.m->name);
  for (size_t j = 0; j < s.length(); j++)
  {
    char    c = s[j];
    if (!((c >= '0' && c <= '9') ||
          (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')))
    {
      s[j] = '_';
    }
  }
  while (namesUsed.find(s) != namesUsed.end())
  {
    if (!(s.length() > 5 && s[s.length() - 5] == '_' &&
          (FileBuffer::readUInt32Fast(s.c_str() + (s.length() - 4))
           & 0xF0F0F0F0U) == 0x30303030U))
    {
      s += "_0001";
    }
    else
    {
      for (size_t j = s.length() - 1; s[j] >= '0' && s[j] <= '9'; j--)
      {
        if (s[j] == '9')
        {
          s[j] = '0';
          continue;
        }
        s[j]++;
        break;
      }
      if (s.ends_with("_0000"))
        s += '1';
    }
  }
  namesUsed.insert(s);
  nameMap.insert(std::pair< const CE2Material *, std::string >(ts.m, s));
  return true;
}

static void printOBJData(std::FILE *f, const NIFFile& nifFile,
                         const char *mtlFileName,
                         bool enableVertexColors, bool enableVertexWeights)
{
  ObjFileMaterialNames  matNames;
  std::string matName;
  std::vector< NIFFile::NIFTriShape > meshData;
  nifFile.getMesh(meshData);
  unsigned int  vertexNumBase = 1;
  std::fprintf(f, "mtllib %s\n\n", mtlFileName);
  for (size_t i = 0; i < meshData.size(); i++)
  {
    const NIFFile::NIFTriShape& ts = meshData[i];
    const char  *tsName = "";
    if (ts.ts && nifFile.getString(ts.ts->nameID))
      tsName = nifFile.getString(ts.ts->nameID)->c_str();
    std::fprintf(f, "# %s\n\ng ", tsName);
    for (size_t j = 0; tsName[j]; j++)
    {
      char    c = tsName[j];
      if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
          (c >= '0' && c <= '9'))
      {
        std::fputc(c, f);
      }
    }
    matNames.getMaterialName(matName, ts);
    std::fprintf(f, "\nusemtl %s\n\n", matName.c_str());
    bool    haveVertexColors = false;
    bool    haveVertexWeights = false;
    if (enableVertexColors)
    {
      for (size_t j = 0; j < ts.vertexCnt; j++)
      {
        if (ts.vertexData[j].vertexColor != 0xFFFFFFFFU)
        {
          haveVertexColors = true;
          break;
        }
      }
    }
    if (enableVertexWeights)
    {
      for (size_t j = 0; j < ts.vertexCnt; j++)
      {
        if (ts.vertexData[j].xyz[3] != 1.0f)
        {
          haveVertexWeights = true;
          break;
        }
      }
    }
    for (size_t j = 0; j < ts.vertexCnt; j++)
    {
      NIFFile::NIFVertex  v = ts.vertexData[j];
      v.xyz = ts.vertexTransform.transformXYZ(v.xyz);
      if (!(haveVertexColors || haveVertexWeights))
      {
        std::fprintf(f, "v %.8f %.8f %.8f\n", v.xyz[0], v.xyz[1], v.xyz[2]);
      }
      else
      {
        std::fprintf(f, "v %.8f %.8f %.8f", v.xyz[0], v.xyz[1], v.xyz[2]);
        if (haveVertexWeights)
          std::fprintf(f, " %.6f", ts.vertexData[j].xyz[3]);
        if (haveVertexColors)
        {
          FloatVector4  c(&(v.vertexColor));
          c *= (1.0f / 255.0f);
          std::fprintf(f, " %.4f %.4f %.4f", c[0], c[1], c[2]);
        }
        std::fputc('\n', f);
      }
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

static void printMTLTexture(std::FILE *f, const char *textureName,
                            const CE2Material::TextureSet *txtSet, size_t n)
{
  if ((txtSet->texturePathMask & (1U << (unsigned int) n)) &&
      !txtSet->texturePaths[n]->empty())
  {
    std::fprintf(f, "map_%s %s\n",
                 textureName, txtSet->texturePaths[n]->c_str());
  }
  else if (n == 3 || n == 4)
  {
    FloatVector4  c(&(txtSet->textureReplacements[n]));
    c /= 255.0f;
    std::fprintf(f, "%s %.3f %.3f %.3f\n", textureName, c[0], c[1], c[2]);
  }
}

static void printMTLData(std::FILE *f, const NIFFile& nifFile)
{
  ObjFileMaterialNames  matNames;
  std::string matName;
  std::vector< NIFFile::NIFTriShape > meshData;
  nifFile.getMesh(meshData);
  for (size_t i = 0; i < meshData.size(); i++)
  {
    const NIFFile::NIFTriShape& ts = meshData[i];
    if (!matNames.getMaterialName(matName, ts))
      continue;
    FloatVector4  baseColor(1.0f);
    const CE2Material::TextureSet *txtSet = (CE2Material::TextureSet *) 0;
    if (ts.m)
    {
      for (unsigned int j = 0U; j < CE2Material::maxLayers; j++)
      {
        if ((ts.m->layerMask & (1U << j)) && ts.m->layers[j])
        {
          if (ts.m->layers[j]->material)
          {
            baseColor = ts.m->layers[j]->material->color;
            txtSet = ts.m->layers[j]->material->textureSet;
            break;
          }
        }
      }
    }
    std::fprintf(f, "newmtl %s\n", matName.c_str());
    std::fprintf(f, "Ka 1.000 1.000 1.000\n");
    if (txtSet &&
        !((txtSet->texturePathMask & 1U) && !txtSet->texturePaths[0]->empty()))
    {
      baseColor *= (FloatVector4(&(txtSet->textureReplacements[0])) / 255.0f);
    }
    std::fprintf(f, "Kd %.3f %.3f %.3f\n",
                 baseColor[0], baseColor[1], baseColor[2]);
    std::fprintf(f, "Ks 1.000 1.000 1.000\n");
    std::fprintf(f, "d %.3f\n", baseColor[3]);
    std::fprintf(f, "Ns 32.0\n");
    if (txtSet)
    {
      printMTLTexture(f, "Kd", txtSet, 0);
      printMTLTexture(f, "Bump", txtSet, 1);
      printMTLTexture(f, "d", txtSet, 2);
      printMTLTexture(f, "Pr", txtSet, 3);
      printMTLTexture(f, "Pm", txtSet, 4);
      printMTLTexture(f, "Ke", txtSet, 7);
    }
    std::fprintf(f, "\n");
  }
}

int main(int argc, char **argv)
{
  std::FILE   *outFile = (std::FILE *) 0;
  const char  *outFileName = (char *) 0;
  NIF_View    *renderer = (NIF_View *) 0;
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
    int     renderWidth = 1792;
    int     renderHeight = 896;
    unsigned char l = 0;
    bool    verboseMaterialInfo = false;
    bool    enableVertexColors = false;
    bool    enableVertexWeights = false;
    const char  *materialDBPath = (char *) 0;
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
      else if (std::strcmp(argv[1], "-c") == 0)
      {
        enableVertexColors = true;
      }
      else if (std::strcmp(argv[1], "-w") == 0)
      {
        enableVertexWeights = true;
      }
      else if (argv[1][1] == 'l' && argv[1][2] >= '0' && argv[1][2] <= '9')
      {
        l = (unsigned char) (argv[1][2] & 0x0F);
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
      else if (std::strcmp(argv[1], "-cdb") == 0)
      {
        if (argc < 3)
          errorMessage("missing material database file name");
        materialDBPath = argv[2];
        argc--;
        argv++;
      }
      else if (std::strcmp(argv[1], "-m") == 0)
      {
        verboseMaterialInfo = true;
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
      std::fprintf(stderr, "    -cdb FILENAME   Set material database path\n");
      std::fprintf(stderr, "    -q      Print author name, file name, "
                           "and file size only\n");
      std::fprintf(stderr, "    -v      Verbose mode, print block list, "
                           "and vertex and triangle data\n");
      std::fprintf(stderr, "    -m      Print detailed material information\n");
      std::fprintf(stderr, "    -obj    Print model data in .obj format\n");
      std::fprintf(stderr, "    -mtl    Print material data in .mtl format\n");
      std::fprintf(stderr, "    -c      Enable vertex colors in .obj output\n");
      std::fprintf(stderr, "    -w      Enable vertex weights in .obj "
                           "output\n");
      std::fprintf(stderr, "    -lN     Set level of detail (default: -l0)\n");
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
    fileNames.push_back(".cdb");
    fileNames.push_back(".mesh");
    if (outFmt >= 5)
      fileNames.push_back(".dds");
    BA2File ba2File(argv[1], &fileNames);
    ba2File.getFileList(fileNames);
    if (outFmt >= 5)
      renderer = new NIF_View(ba2File, (ESMFile *) 0, materialDBPath);
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
          fileNames[i] = fileNames.back();
        fileNames.pop_back();
      }
      if (fileNames.size() > 0)
      {
        std::sort(fileNames.begin(), fileNames.end());
        SDLDisplay  display(renderWidth, renderHeight, "nif_info", 4U, 48);
        display.setDefaultTextColor(0x00, 0xC0);
        renderer->viewModels(display, fileNames, l);
      }
      delete renderer;
      return 0;
    }
    CE2MaterialDB materials(ba2File, materialDBPath);
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
      NIFFile nifFile(fileBuf.data(), fileBuf.size(), ba2File, &materials, l);
      if (outFmt == 0 || outFmt == 2)
        printBlockList(outFile, nifFile, verboseMaterialInfo);
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
        printOBJData(outFile, nifFile, mtlName.c_str(),
                     enableVertexColors, enableVertexWeights);
      }
      if (outFmt == 4)
        printMTLData(outFile, nifFile);
      if (outFmt == 5)
      {
        std::fprintf(stderr, "%s\n", fileNames[i].c_str());
        renderer->loadModel(fileNames[i], l);
        renderer->renderModelToFile(outFileName, renderWidth, renderHeight);
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
    if (renderer)
      delete renderer;
    std::fprintf(stderr, "nif_info: %s\n", e.what());
    return 1;
  }
  if (outFileName && outFile)
    std::fclose(outFile);
  if (renderer)
    delete renderer;
  return 0;
}

