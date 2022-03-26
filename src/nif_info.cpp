
#include "common.hpp"
#include "nif_file.hpp"
#include "ba2file.hpp"
#include "ddstxt.hpp"
#include "plot3d.hpp"

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
        std::fprintf(f, "    Material version: 0x%04X\n",
                     (unsigned int) lspBlock->bgsmVersion);
        std::fprintf(f, "    Material flags: 0x%04X\n",
                     (unsigned int) lspBlock->bgsmFlags);
        std::fprintf(f, "    Material alpha blend mode: 0x%04X\n",
                     (unsigned int) lspBlock->bgsmAlphaBlendMode);
        std::fprintf(f, "    Material alpha threshold: %3d\n",
                     int(lspBlock->bgsmAlphaThreshold));
        for (size_t j = 0; j < lspBlock->bgsmTextures.size(); j++)
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
    std::fprintf(f, "  Vertex count: %d\n", int(meshData[i].vertexCnt));
    std::fprintf(f, "  Triangle count: %d\n", int(meshData[i].triangleCnt));
    std::fprintf(f, "  Is water: %d\n", int(meshData[i].isWater));
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
      meshData[i].vertexTransform.transformVertex(v);
      std::fprintf(f, "    %4d: XYZ: (%f, %f, %f), normals: (%f, %f, %f), "
                   "UV: (%f, %f), color: 0x%08X\n",
                   int(j), v.x, v.y, v.z, v.normalX, v.normalY, v.normalZ,
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
      meshData[i].vertexTransform.transformVertex(v);
      std::fprintf(f, "v %.8f %.8f %.8f\n", v.x, v.y, v.z);
    }
    for (size_t j = 0; j < meshData[i].vertexCnt; j++)
    {
      NIFFile::NIFVertex  v = meshData[i].vertexData[j];
      std::fprintf(f, "vt %.8f %.8f\n", v.getU(), 1.0 - v.getV());
    }
    for (size_t j = 0; j < meshData[i].vertexCnt; j++)
    {
      NIFFile::NIFVertex  v = meshData[i].vertexData[j];
      meshData[i].vertexTransform.transformVertex(v);
      std::fprintf(f, "vn %.8f %.8f %.8f\n", v.normalX, v.normalY, v.normalZ);
    }
    for (size_t j = 0; j < meshData[i].triangleCnt; j++)
    {
      unsigned int  v0 = meshData[i].triangleData[j].v0 + vertexNumBase;
      unsigned int  v1 = meshData[i].triangleData[j].v1 + vertexNumBase;
      unsigned int  v2 = meshData[i].triangleData[j].v2 + vertexNumBase;
      std::fprintf(f, "f %u/%u/%u %u/%u/%u %u/%u/%u\n",
                   v0, v0, v0, v1, v1, v1, v2, v2, v2);
    }
    vertexNumBase = vertexNumBase + (unsigned int) meshData[i].vertexCnt;
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
    std::fprintf(f, "Ks 1.0 1.0 1.0\n");
    std::fprintf(f, "d 1.0\n");
    std::fprintf(f, "Ns 33.0\n");
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

static unsigned int downsample2xFilter(const unsigned int *buf,
                                       int imageWidth, int imageHeight,
                                       int x, int y)
{
  static const int  filterTable[105] =
  {
    // Y offset, (X offset, value) * 7
    -5, -5,   1, -3,  -3, -1,  10,  0,  16,  1,  10,  3,  -3,  5,   1,
    -3, -5,  -3, -3,   9, -1, -30,  0, -48,  1, -30,  3,   9,  5,  -3,
    -1, -5,  10, -3, -30, -1, 100,  0, 160,  1, 100,  3, -30,  5,  10,
     0, -5,  16, -3, -48, -1, 160,  0, 256,  1, 160,  3, -48,  5,  16,
     1, -5,  10, -3, -30, -1, 100,  0, 160,  1, 100,  3, -30,  5,  10,
     3, -5,  -3, -3,   9, -1, -30,  0, -48,  1, -30,  3,   9,  5,  -3,
     5, -5,   1, -3,  -3, -1,  10,  0,  16,  1,  10,  3,  -3,  5,   1
  };
  const int   *p = filterTable;
  int     r = 0;
  int     g = 0;
  int     b = 0;
  for (int i = 0; i < 7; i++)
  {
    int     yy = y + p[0];
    p++;
    yy = (yy > 0 ? (yy < (imageHeight - 1) ? yy : (imageHeight - 1)) : 0);
    const unsigned int  *bufp = buf + (size_t(yy) * size_t(imageWidth));
    if (BRANCH_EXPECT(x < 5 || x >= (imageWidth - 5), false))
    {
      for (int j = 0; j < 7; j++, p = p + 2)
      {
        int     xx = x + p[0];
        int     m = p[1];
        xx = (xx > 0 ? (xx < (imageWidth - 1) ? xx : (imageWidth - 1)) : 0);
        unsigned int  c = bufp[xx];
        r = r + (int(c & 0xFF) * m);
        g = g + (int((c >> 8) & 0xFF) * m);
        b = b + (int((c >> 16) & 0xFF) * m);
      }
    }
    else
    {
      bufp = bufp + x;
      for (int j = 0; j < 7; j++, p = p + 2)
      {
        unsigned int  c = *(bufp + p[0]);
        int     m = p[1];
        r = r + (int(c & 0xFF) * m);
        g = g + (int((c >> 8) & 0xFF) * m);
        b = b + (int((c >> 16) & 0xFF) * m);
      }
    }
  }
  unsigned int  c = buf[size_t(y) * size_t(imageWidth) + size_t(x)];
  r = (r > 0 ? (r < 0x0003FC00 ? r : 0x0003FC00) : 0) + 512;
  g = ((g > 0 ? (g < 0x0003FC00 ? g : 0x0003FC00) : 0) + 512) & 0x0003FC00;
  b = ((b > 0 ? (b < 0x0003FC00 ? b : 0x0003FC00) : 0) + 512) & 0x0003FC00;
  return ((c & 0xFF000000U) | (unsigned int) ((r >> 10) | (g >> 2) | (b << 6)));
}

static void renderMeshToFile(const char *outFileName, const NIFFile& nifFile,
                             const BA2File& ba2File,
                             int imageWidth, int imageHeight)
{
  std::map< const std::string *, DDSTexture * > textures;
  try
  {
    size_t  imageDataSize = size_t(imageWidth) * size_t(imageHeight);
    std::vector< unsigned int > outBufRGBW(imageDataSize, 0U);
    std::vector< float >  outBufZ(imageDataSize, 16777216.0f);
    std::vector< unsigned char >  fileBuf;
    std::vector< NIFFile::NIFTriShape > meshData;
    nifFile.getMesh(meshData);
    // isometric view (rotate by 54.7356, 180, 45 degrees)
    NIFFile::NIFVertexTransform viewTransform(1.0f,
                                              float(std::atan(std::sqrt(2.0))),
                                              float(std::atan(1.0) * 4.0),
                                              float(std::atan(1.0)),
                                              0.0f, 0.0f, 0.0f);
    {
      NIFFile::NIFBounds  b;
      for (size_t i = 0; i < meshData.size(); i++)
        meshData[i].calculateBounds(b, &viewTransform);
      if (b.xMax > b.xMin && b.yMax > b.yMin)
      {
        float   xScale = float(imageWidth) * 0.9375f / (b.xMax - b.xMin);
        float   yScale = float(imageHeight) * 0.9375f / (b.yMax - b.yMin);
        viewTransform.scale = (xScale < yScale ? xScale : yScale);
        viewTransform.offsX =
            (float(imageWidth) - ((b.xMin + b.xMax) * viewTransform.scale))
            * 0.5f;
        viewTransform.offsY =
            (float(imageHeight) - ((b.yMin + b.yMax) * viewTransform.scale))
            * 0.5f;
        viewTransform.offsZ = 1.0f - (b.zMin * viewTransform.scale);
      }
    }
    for (size_t i = 0; i < meshData.size(); i++)
    {
      const DDSTexture  *textureD = (DDSTexture *) 0;
      const DDSTexture  *textureN = (DDSTexture *) 0;
      for (size_t j = 0; j < meshData[i].texturePathCnt && j < 2; j++)
      {
        if (meshData[i].texturePaths[j]->empty())
          continue;
        std::map< const std::string *, DDSTexture * >::iterator k =
            textures.find(meshData[i].texturePaths[j]);
        if (k == textures.end())
        {
          ba2File.extractFile(fileBuf, *(meshData[i].texturePaths[j]));
          DDSTexture  *texture =
              new DDSTexture(&(fileBuf.front()), fileBuf.size());
          k = textures.insert(std::pair< const std::string *, DDSTexture * >(
                                  meshData[i].texturePaths[j], texture)).first;
        }
        if (j == 0)
          textureD = k->second;
        else
          textureN = k->second;
      }
      Plot3D_TriShape plot3d(&(outBufRGBW.front()), &(outBufZ.front()),
                             imageWidth, imageHeight, meshData[i]);
      // light direction: 45, -35.2644, 0 degrees
      NIFFile::NIFVertexTransform tmp(1.0f,
                                      float(std::atan(1.0)),
                                      -(float(std::atan(std::sqrt(0.5)))), 0.0f,
                                      0.0f, 0.0f, 0.0f);
      plot3d.drawTriShape(NIFFile::NIFVertexTransform(), viewTransform,
                          tmp.rotateXZ, tmp.rotateYZ, tmp.rotateZZ,
                          textureD, textureN);
    }
    for (size_t i = 0; i < imageDataSize; i++)
    {
      unsigned int  c = outBufRGBW[i];
      unsigned int  a = c >> 24;
      if (a >= 1 && a <= 254)
      {
        unsigned int  waterColor = Plot3D_TriShape::multiplyWithLight(
                                       0xFF804000U, 0U, int((a - 1) << 2));
        waterColor = (waterColor & 0x00FFFFFFU) | 0xC0000000U;
        outBufRGBW[i] = blendRGBA32(c, waterColor, int(waterColor >> 24));
      }
    }
    DDSOutputFile outFile(outFileName, imageWidth >> 1, imageHeight >> 1,
                          DDSInputFile::pixelFormatRGB24);
    for (int y = 0; y < imageHeight; y = y + 2)
    {
      for (int x = 0; x < imageWidth; x = x + 2)
      {
        unsigned int  c = downsample2xFilter(&(outBufRGBW.front()),
                                             imageWidth, imageHeight, x, y);
        outFile.writeByte((unsigned char) ((c >> 16) & 0xFF));
        outFile.writeByte((unsigned char) ((c >> 8) & 0xFF));
        outFile.writeByte((unsigned char) (c & 0xFF));
      }
    }
    for (std::map< const std::string *, DDSTexture * >::iterator
             i = textures.begin(); i != textures.end(); i++)
    {
      delete i->second;
    }
  }
  catch (...)
  {
    for (std::map< const std::string *, DDSTexture * >::iterator
             i = textures.begin(); i != textures.end(); i++)
    {
      delete i->second;
    }
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
    int     outFmt = 0;
    int     renderWidth = 2400;
    int     renderHeight = 1600;
    if (argc >= 2)
    {
      static const char *formatOptions[5] =
      {
        "-q", "-v", "-obj", "-mtl", "-render"
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
      if (outFmt == 0 && std::strncmp(argv[1], "-render", 7) == 0)
      {
        std::string tmp(argv[1] + 7);
        outFmt = 5;
        argc--;
        argv++;
        size_t  n = tmp.find('x');
        if (n != std::string::npos)
        {
          renderHeight = int(parseInteger(tmp.c_str() + (n + 1), 10,
                                          "invalid image height", 8, 16384));
          tmp.resize(n);
          renderWidth = int(parseInteger(tmp.c_str(), 10,
                                         "invalid image width", 8, 16384));
          renderWidth = renderWidth << 1;
          renderHeight = renderHeight << 1;
        }
        else
        {
          throw errorMessage("invalid image dimensions");
        }
      }
    }
    if (argc < (outFmt != 5 ? 3 : 4))
    {
      std::fprintf(stderr, "Usage: nif_info [-q|-v|-obj|-mtl|-render DDSFILE] "
                           "ARCHIVEPATH PATTERN...\n");
      std::fprintf(stderr, "    -q      Print author name, file name, "
                           "and file size only\n");
      std::fprintf(stderr, "    -v      Verbose mode, print block list, "
                           "and vertex and triangle data\n");
      std::fprintf(stderr, "    -obj    Print model data in .obj format\n");
      std::fprintf(stderr, "    -mtl    Print material data in .mtl format\n");
      std::fprintf(stderr, "    -render[WIDTHxHEIGHT] DDSFILE       "
                           "Render model to DDS file (experimental)\n");
      return 1;
    }
    std::vector< std::string >  fileNames;
    for (int i = 2; i < argc; i++)
      fileNames.push_back(argv[i]);
    fileNames.push_back(".bgsm");
    if (outFmt == 5)
    {
      fileNames.push_back(".dds");
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
      if (outFmt == 5)
      {
        std::fprintf(stderr, "%s\n", fileNames[i].c_str());
        renderMeshToFile(argv[0], nifFile, ba2File, renderWidth, renderHeight);
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

