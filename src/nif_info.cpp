
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
        lightingShaderPropertyBlock = nifFile.getLightingShaderProperty(i);
    const NIFFile::NIFBlkBSShaderTextureSet *
        shaderTextureSetBlock = nifFile.getShaderTextureSet(i);
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
    else if (lightingShaderPropertyBlock)
    {
      if (lightingShaderPropertyBlock->controller >= 0)
      {
        std::fprintf(f, "    Controller: %3d\n",
                     lightingShaderPropertyBlock->controller);
      }
      if (lightingShaderPropertyBlock->textureSet >= 0)
      {
        std::fprintf(f, "    Texture set: %3d\n",
                     lightingShaderPropertyBlock->textureSet);
      }
    }
    else if (shaderTextureSetBlock)
    {
      for (size_t j = 0; j < shaderTextureSetBlock->texturePaths.size(); j++)
      {
        if (!shaderTextureSetBlock->texturePaths[j].empty())
        {
          std::fprintf(f, "    Texture %2d: %s\n",
                       int(j), shaderTextureSetBlock->texturePaths[j].c_str());
        }
      }
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
    printVertexTransform(f, meshData[i].vertexTransform);
    if (meshData[i].materialPath)
      std::fprintf(f, "  Material: %s\n", meshData[i].materialPath->c_str());
    std::fprintf(f, "  Texture UV offset, scale: (%f, %f), (%f, %f)\n",
                 meshData[i].textureOffsetU, meshData[i].textureOffsetV,
                 meshData[i].textureScaleU, meshData[i].textureScaleV);
    for (size_t j = 0; j < meshData[i].texturePathCnt; j++)
    {
      std::fprintf(f, "  Texture %2d: %s\n",
                   int(j), meshData[i].texturePaths[j].c_str());
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
          !meshData[i].texturePaths[j].empty())
      {
        std::fprintf(f, "%s %s\n",
                     (!j ? "map_Kd" : "map_Kn"),
                     meshData[i].texturePaths[j].c_str());
      }
    }
    std::fprintf(f, "\n");
  }
}

static void renderMeshToFile(const char *outFileName, const NIFFile& nifFile,
                             const BA2File& ba2File,
                             int imageWidth, int imageHeight)
{
  std::map< std::string, DDSTexture * > textures;
  try
  {
    size_t  imageDataSize = size_t(imageWidth) * size_t(imageHeight);
    std::vector< unsigned int > outBufRGBW(imageDataSize, 0U);
    std::vector< float >  outBufZ(imageDataSize, 16777216.0f);
    std::vector< unsigned char >  fileBuf;
    std::vector< NIFFile::NIFTriShape > meshData;
    nifFile.getMesh(meshData);
    NIFFile::NIFVertexTransform viewTransform;
    // isometric view
    viewTransform.rotateXX =   float(std::sqrt(1.0 / 2.0));
    viewTransform.rotateXY =   float(std::sqrt(1.0 / 2.0));
    viewTransform.rotateXZ =   0.0f;
    viewTransform.rotateYX =   float(std::sqrt(1.0 / 6.0));
    viewTransform.rotateYY = -(float(std::sqrt(1.0 / 6.0)));
    viewTransform.rotateYZ = -(float(std::sqrt(2.0 / 3.0)));
    viewTransform.rotateZX = -(float(std::sqrt(1.0 / 3.0)));
    viewTransform.rotateZY =   float(std::sqrt(1.0 / 3.0));
    viewTransform.rotateZZ = -(float(std::sqrt(1.0 / 3.0)));
    {
      float   xMin = 1.0e9f;
      float   yMin = 1.0e9f;
      float   zMin = 1.0e9f;
      float   xMax = -1.0e9f;
      float   yMax = -1.0e9f;
      float   zMax = -1.0e9f;
      for (size_t i = 0; i < meshData.size(); i++)
      {
        NIFFile::NIFVertexTransform vt = meshData[i].vertexTransform;
        vt *= viewTransform;
        for (size_t j = 0; j < meshData[i].vertexCnt; j++)
        {
          NIFFile::NIFVertex  v = meshData[i].vertexData[j];
          vt.transformXYZ(v.x, v.y, v.z);
          xMin = (v.x < xMin ? v.x : xMin);
          yMin = (v.y < yMin ? v.y : yMin);
          zMin = (v.z < zMin ? v.z : zMin);
          xMax = (v.x > xMax ? v.x : xMax);
          yMax = (v.y > yMax ? v.y : yMax);
          zMax = (v.z > zMax ? v.z : zMax);
        }
      }
      if (xMax > xMin && yMax > yMin && zMax > zMin)
      {
        float   xScale = float(imageWidth) * 0.875f / (xMax - xMin);
        float   yScale = float(imageHeight) * 0.875f / (yMax - yMin);
        viewTransform.scale = (xScale < yScale ? xScale : yScale);
        viewTransform.offsX =
            (float(imageWidth) - ((xMin + xMax) * viewTransform.scale)) * 0.5f;
        viewTransform.offsY =
            (float(imageHeight) - ((yMin + yMax) * viewTransform.scale)) * 0.5f;
        viewTransform.offsZ = 1.0f - (zMin * viewTransform.scale);
      }
    }
    for (size_t i = 0; i < meshData.size(); i++)
    {
      const DDSTexture  *textureD = (DDSTexture *) 0;
      const DDSTexture  *textureN = (DDSTexture *) 0;
      for (size_t j = 0; j < meshData[i].texturePathCnt && j < 2; j++)
      {
        if (meshData[i].texturePaths[j].empty())
          continue;
        if (textures.find(meshData[i].texturePaths[j]) != textures.end())
          continue;
        ba2File.extractFile(fileBuf, meshData[i].texturePaths[j]);
        DDSTexture  *texture =
            new DDSTexture(&(fileBuf.front()), fileBuf.size());
        textures.insert(std::pair< std::string, DDSTexture * >(
                            meshData[i].texturePaths[j], texture));
        if (j == 0)
          textureD = texture;
        else
          textureN = texture;
      }
      Plot3D_TriShape plot3d(&(outBufRGBW.front()), &(outBufZ.front()),
                             imageWidth, imageHeight,
                             meshData[i], viewTransform);
      plot3d.drawTriShape(-0.57735f, 0.57735f, 0.57735f, textureD, textureN);
    }
    DDSOutputFile outFile(outFileName, imageWidth, imageHeight,
                          DDSInputFile::pixelFormatRGB24);
    for (size_t i = 0; i < imageDataSize; i++)
    {
      unsigned int  c = outBufRGBW[i];
      unsigned int  a = c >> 24;
      if (a >= 1 && a <= 254)
      {
        unsigned int  waterColor = 0x00804000U;
        unsigned int  lightLevel = (a - 1) << 2;
        unsigned int  r = ((waterColor & 0xFF) * lightLevel + 0x80) >> 8;
        unsigned int  g = (((waterColor >> 8) & 0xFF) * lightLevel + 0x80) >> 8;
        unsigned int  b = ((waterColor >> 16) * lightLevel + 0x80) >> 8;
        c = blendRGBA32(c, 0xC0000000U | (b << 16) | (g << 8) | r, 0xC0);
      }
      outFile.writeByte((unsigned char) ((c >> 16) & 0xFF));
      outFile.writeByte((unsigned char) ((c >> 8) & 0xFF));
      outFile.writeByte((unsigned char) (c & 0xFF));
    }
    for (std::map< std::string, DDSTexture * >::iterator i = textures.begin();
         i != textures.end(); i++)
    {
      delete i->second;
    }
  }
  catch (...)
  {
    for (std::map< std::string, DDSTexture * >::iterator i = textures.begin();
         i != textures.end(); i++)
    {
      delete i->second;
    }
    throw;
  }
}

int main(int argc, char **argv)
{
  // 0: default (block list)
  // 1: quiet (author info)
  // 2: verbose (block list and model data)
  // 3: .obj format
  // 4: .mtl format
  // 5: render to DDS file
  int     outFmt = 0;
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
    std::fprintf(stderr, "    -render Render model to DDS file "
                         "(experimental)\n");
    return 1;
  }
  std::FILE *outFile = stdout;
  try
  {
    std::vector< std::string >  fileNames;
    for (int i = 2; i < argc; i++)
      fileNames.push_back(argv[i]);
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
      if (fileNames[i].length() < 5 ||
          std::strcmp(fileNames[i].c_str() + fileNames[i].length() - 4, ".nif")
          != 0)
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
      NIFFile nifFile(&(fileBuf.front()), fileBuf.size());
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
        renderMeshToFile(argv[0], nifFile, ba2File, 1200, 800);
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

