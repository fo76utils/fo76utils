
#include "common.hpp"
#include "nif_file.hpp"
#include "ba2file.hpp"
#include "ddstxt.hpp"
#include "plot3d.hpp"

#ifdef HAVE_SDL
#  include <SDL/SDL.h>
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
          if (!lspBlock->bgsmTextures[j]->empty())
          {
            std::fprintf(f, "    Material texture %d: %s\n",
                         int(j), lspBlock->bgsmTextures[j]->c_str());
          }
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
    if (meshData[i].flags)
    {
      std::fprintf(f, "  Flags: ");
      if (meshData[i].flags & 0x01)
        std::fprintf(f, "hidden");
      if ((meshData[i].flags & 0x03) > 0x02)
        std::fprintf(f, ", ");
      if (meshData[i].flags & 0x02)
        std::fprintf(f, "is water");
      if ((meshData[i].flags & 0x07) > 0x04)
        std::fprintf(f, ", ");
      if (meshData[i].flags & 0x04)
        std::fprintf(f, "is effect");
      if ((meshData[i].flags & 0x0F) > 0x08)
        std::fprintf(f, ", ");
      if (meshData[i].flags & 0x08)
        std::fprintf(f, "is decal");
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

static void renderMeshToFile(const char *outFileName, const NIFFile& nifFile,
                             const BA2File& ba2File,
                             int imageWidth, int imageHeight)
{
  std::map< const std::string *, DDSTexture * > textureSet;
#ifdef HAVE_SDL
  if (!outFileName)
  {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
      throw errorMessage("error initializing SDL");
  }
#endif
  try
  {
#ifdef HAVE_SDL
    SDL_Surface *sdlScreen = (SDL_Surface *) 0;
    if (!outFileName)
    {
      sdlScreen = SDL_SetVideoMode(imageWidth, imageHeight, 24, SDL_SWSURFACE);
      if (!sdlScreen)
        throw errorMessage("error setting SDL video mode");
    }
#endif
    size_t  imageDataSize = size_t(imageWidth) * size_t(imageHeight);
    std::vector< unsigned int > outBufRGBW(imageDataSize, 0U);
    std::vector< float >  outBufZ(imageDataSize, 16777216.0f);
    std::vector< unsigned char >  fileBuf;
    std::vector< NIFFile::NIFTriShape > meshData;
    nifFile.getMesh(meshData);
    float   modelRotationX = 0.0f;
    float   modelRotationY = 0.0f;
    float   modelRotationZ = 0.0f;
    // light direction: 45, -35.2644, 0 degrees
    float   lightRotationX = float(std::atan(1.0));
    float   lightRotationY = -(float(std::atan(std::sqrt(0.5))));
    // isometric view (rotate by 54.7356, 180, 45 degrees)
    float   viewRotationX = float(std::atan(std::sqrt(2.0)));
    float   viewRotationY = float(std::atan(1.0) * 4.0);
    float   viewRotationZ = float(std::atan(1.0));
    std::string
        defaultEnvMap("textures/shared/cubemaps/metalchrome01cube_e.dds");
    Plot3D_TriShape plot3d(&(outBufRGBW.front()), &(outBufZ.front()),
                           imageWidth, imageHeight);
    while (true)
    {
      NIFFile::NIFVertexTransform
          modelTransform(1.0f, modelRotationX, modelRotationY, modelRotationZ,
                         0.0f, 0.0f, 0.0f);
      NIFFile::NIFVertexTransform
          viewTransform(1.0f, viewRotationX, viewRotationY, viewRotationZ,
                        0.0f, 0.0f, 0.0f);
      {
        NIFFile::NIFVertexTransform t(modelTransform);
        t *= viewTransform;
        NIFFile::NIFBounds  b;
        for (size_t i = 0; i < meshData.size(); i++)
        {
          if (!(meshData[i].flags & 0x0005))    // ignore if hidden or effect
            meshData[i].calculateBounds(b, &t);
        }
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
        if (meshData[i].flags & 0x0005)         // ignore if hidden or effect
          continue;
        plot3d = meshData[i];
        const DDSTexture  *textures[9];
        for (size_t j = 9; j-- > 0; )
        {
          textures[j] = (DDSTexture *) 0;
          if (!(j < meshData[i].texturePathCnt && ((1 << int(j)) & 0x011B)))
            continue;
          const std::string *texturePath = meshData[i].texturePaths[j];
          if (texturePath->empty())
          {
            if (!(j == 4 && textures[8]))
              continue;
            texturePath = &defaultEnvMap;
          }
          std::map< const std::string *, DDSTexture * >::iterator k =
              textureSet.find(texturePath);
          if (k == textureSet.end())
          {
            ba2File.extractFile(fileBuf, *texturePath);
            DDSTexture  *texture =
                new DDSTexture(&(fileBuf.front()), fileBuf.size());
            k = textureSet.insert(
                    std::pair< const std::string *, DDSTexture * >(
                        texturePath, texture)).first;
          }
          textures[j] = k->second;
        }
        NIFFile::NIFVertexTransform
            tmp(1.0f, lightRotationX, lightRotationY, 0.0f, 0.0f, 0.0f, 0.0f);
        plot3d.drawTriShape(modelTransform, viewTransform,
                            tmp.rotateXZ, tmp.rotateYZ, tmp.rotateZZ,
                            textures, 9);
      }
#ifdef HAVE_SDL
      if (outFileName)
#endif
      {
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
      }
#ifdef HAVE_SDL
      else
      {
        SDL_LockSurface(sdlScreen);
        unsigned int  *srcPtr = &(outBufRGBW.front());
        unsigned char *dstPtr =
            reinterpret_cast< unsigned char * >(sdlScreen->pixels);
        for (int y = 0; y < imageHeight; y++)
        {
          for (int x = 0; x < imageWidth; x++, srcPtr++, dstPtr = dstPtr + 3)
          {
            unsigned int  c = *srcPtr;
            unsigned int  a = c >> 24;
            if (a >= 1 && a <= 254)
            {
              unsigned int  waterColor =
                  Plot3D_TriShape::multiplyWithLight(0xFF804000U, 0U,
                                                     int((a - 1) << 2));
              waterColor = (waterColor & 0x00FFFFFFU) | 0xC0000000U;
              c = blendRGBA32(c, waterColor, int(waterColor >> 24));
            }
            dstPtr[0] = (unsigned char) ((c >> 16) & 0xFF);     // B
            dstPtr[1] = (unsigned char) ((c >> 8) & 0xFF);      // G
            dstPtr[2] = (unsigned char) (c & 0xFF);             // R
            *srcPtr = 0U;
            outBufZ[size_t(srcPtr - &(outBufRGBW.front()))] = 16777216.0f;
          }
          dstPtr = dstPtr + (int(sdlScreen->pitch) - (imageWidth * 3));
        }
        SDL_UnlockSurface(sdlScreen);
        SDL_UpdateRect(sdlScreen, 0, 0, imageWidth, imageHeight);
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
              case SDLK_LEFT:
                lightRotationX += 0.19634954f;
                break;
              case SDLK_RIGHT:
                lightRotationX -= 0.19634954f;
                break;
              case SDLK_DOWN:
                lightRotationY += 0.19634954f;
                break;
              case SDLK_UP:
                lightRotationY -= 0.19634954f;
                break;
              case SDLK_ESCAPE:
                quitFlag = true;
                break;
              default:
                continue;
            }
            keyPressed = true;
            break;
          }
        }
        if (!quitFlag)
          continue;
      }
#endif
      break;
    }
#ifdef HAVE_SDL
    if (outFileName)
#endif
    {
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
    }
    for (std::map< const std::string *, DDSTexture * >::iterator
             i = textureSet.begin(); i != textureSet.end(); i++)
    {
      delete i->second;
    }
#ifdef HAVE_SDL
    if (!outFileName)
      SDL_Quit();
#endif
  }
  catch (...)
  {
    for (std::map< const std::string *, DDSTexture * >::iterator
             i = textureSet.begin(); i != textureSet.end(); i++)
    {
      delete i->second;
    }
#ifdef HAVE_SDL
    if (!outFileName)
      SDL_Quit();
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
    // 6: render and view in real time (requires SDL 1.2)
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
#ifdef HAVE_SDL
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
#ifdef HAVE_SDL
          renderMeshToFile((char *) 0, nifFile, ba2File,
                           renderWidth, renderHeight);
#else
          throw errorMessage("viewing the model requires SDL 1.2");
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

