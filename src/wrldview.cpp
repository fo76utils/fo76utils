
#include "common.hpp"
#include "fp32vec4.hpp"
#include "render.hpp"
#include "sdlvideo.hpp"

#include <ctime>

static const char *usageStrings[] =
{
  "Usage: wrldview INFILE.ESM[,...] W H ARCHIVEPATH [OPTIONS...]",
  "",
  "Options:",
  "    --help              print usage",
  "    --list-defaults     print defaults for all options, and exit",
  "    --                  remaining options are file names",
  "    -threads INT        set the number of threads to use",
  "    -debug INT          set debug render mode (0: disabled, 1: form IDs,",
  "                        2: depth, 3: normals, 4: diffuse, 5: light only)",
  "    -scol BOOL          enable the use of pre-combined meshes",
  "    -a                  render all object types",
  "    -textures BOOL      make all diffuse textures white if false",
  "    -txtcache INT       texture cache size in megabytes",
  "    -rq INT             set render quality (0 to 15, see doc/render.md)",
  "",
  "    -btd FILENAME.BTD   read terrain data from Fallout 76 .btd file",
  "    -w FORMID           form ID of world, cell, or object to render",
  "    -r X0 Y0 X1 Y1      terrain cell range, X0,Y0 = SW to X1,Y1 = NE",
  "    -l INT              level of detail to use from BTD file (0 to 4)",
  "    -deftxt FORMID      form ID of default land texture",
  "    -defclr 0x00RRGGBB  default color for untextured terrain",
  "    -ltxtres INT        land texture resolution per cell",
  "    -mip INT            base mip level for all textures",
  "    -lmult FLOAT        land texture RGB level scale",
  "",
  "    -light SCALE RY RZ  set RGB scale and Y, Z rotation (0, 0 = top)",
  "    -lcolor LMULT LCOLOR EMULT ECOLOR ACOLOR",
  "                        set light source, environment, and ambient light",
  "                        colors and levels (colors in 0xRRGGBB format)",
  "    -rscale FLOAT       reflection view vector Z scale",
  "",
  "    -mlod INT           set level of detail for models, 0 (best) to 4",
  "    -vis BOOL           render only objects visible from distance",
  "    -ndis BOOL          do not render initially disabled objects",
  "    -xm STRING          add excluded model path name pattern",
  "",
  "    -env FILENAME.DDS   default environment map texture path in archives",
  "    -wtxt FILENAME.DDS  water normal map texture path in archives",
  "    -watercolor UINT32  water color (A7R8G8B8), 0 disables water",
  "    -wrefl FLOAT        water environment map scale",
  "    -wscale INT         water texture tile size",
  (char *) 0
};

static const float  viewRotations[27] =
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

int main(int argc, char **argv)
{
  std::vector< std::uint32_t >  imageBuf;
  std::vector< SDLDisplay::SDLEvent > eventBuf;
  SDLDisplay  *display = (SDLDisplay *) 0;
  int     err = 1;
  try
  {
    std::vector< const char * > args;
    int     threadCnt = -1;
    int     textureCacheSize = 1024;
    bool    distantObjectsOnly = false;
    bool    noDisabledObjects = true;
    bool    enableSCOL = false;
    bool    enableAllObjects = false;
    bool    enableTextures = true;
    unsigned char debugMode = 0;
    unsigned int  formID = 0U;
    int     btdLOD = 0;
    const char  *btdPath = (char *) 0;
    int     terrainX0 = -32768;
    int     terrainY0 = -32768;
    int     terrainX1 = 32767;
    int     terrainY1 = 32767;
    unsigned int  defTxtID = 0U;
    std::uint32_t ltxtDefColor = 0x003F3F3FU;
    int     ltxtResolution = 128;
    int     textureMip = 7;
    float   landTextureMult = 1.0f;
    int     modelLOD = 0;
    std::uint32_t waterColor = 0x7FFFFFFFU;
    float   waterReflectionLevel = 1.0f;
    float   viewScale = 0.0625f;
    int     viewRotation = 4;
    float   camPositionX = 0.0f;
    float   camPositionY = 0.0f;
    float   camPositionZ = 65536.0f;
    float   viewOffsX = 0.0f;
    float   viewOffsY = 0.0f;
    float   viewOffsZ = 0.0f;
    float   rgbScale = 1.0f;
    float   lightRotationY = 70.5288f;
    float   lightRotationZ = 135.0f;
    int     lightColor = 0x00FFFFFF;
    int     ambientColor = -1;
    int     envColor = 0x00FFFFFF;
    float   lightLevel = 1.0f;
    float   envLevel = 1.0f;
    float   reflZScale = 2.0f;
    int     waterUVScale = 2048;
    unsigned char renderQuality = 0;
    const char  *defaultEnvMap =
        "textures/shared/cubemaps/mipblur_defaultoutside1.dds";
    const char  *waterTexture = "textures/water/defaultwater.dds";
    std::vector< const char * > excludeModelPatterns;
    float   d = float(std::atan(1.0) / 45.0);   // degrees to radians

    for (int i = 1; i < argc; i++)
    {
      if (std::strcmp(argv[i], "--") == 0)
      {
        while (++i < argc)
          args.push_back(argv[i]);
        break;
      }
      else if (std::strcmp(argv[i], "--help") == 0)
      {
        args.clear();
        err = 0;
        break;
      }
      else if (std::strcmp(argv[i], "--list-defaults") == 0)
      {
        SDLDisplay::enableConsole();
        std::printf("-threads %d", threadCnt);
        if (threadCnt <= 0)
        {
          std::printf(" (defaults to hardware threads: %d)",
                      int(std::thread::hardware_concurrency()));
        }
        std::printf("\n-debug %d\n", int(debugMode));
        std::printf("-scol %d\n", int(enableSCOL));
        std::printf("-textures %d\n", int(enableTextures));
        std::printf("-txtcache %d\n", textureCacheSize);
        std::printf("-rq %d\n", int(renderQuality));
        std::printf("-w 0x%08X", formID);
        if (!formID)
          std::printf(" (defaults to 0x0000003C or 0x0025DA15)");
        std::printf("\n-r %d %d %d %d\n",
                    terrainX0, terrainY0, terrainX1, terrainY1);
        std::printf("-l %d\n", btdLOD);
        std::printf("-deftxt 0x%08X\n", defTxtID);
        std::printf("-defclr 0x%08X\n", (unsigned int) ltxtDefColor);
        std::printf("-ltxtres %d\n", ltxtResolution);
        std::printf("-mip %d\n", textureMip);
        std::printf("-lmult %.1f\n", landTextureMult);
        std::printf("-light %.1f %.4f %.4f\n",
                    rgbScale, lightRotationY, lightRotationZ);
        {
          NIFFile::NIFVertexTransform
              vt(1.0f, 0.0f, lightRotationY * d, lightRotationZ * d,
                 0.0f, 0.0f, 0.0f);
          std::printf("    Light vector: %9.6f %9.6f %9.6f\n",
                      vt.rotateZX, vt.rotateZY, vt.rotateZZ);
        }
        std::printf("-lcolor %.3f 0x%06X %.3f 0x%06X ",
                    lightLevel, (unsigned int) lightColor,
                    envLevel, (unsigned int) envColor);
        if (ambientColor < 0)
          std::printf("%d (calculated from default cube map)\n", ambientColor);
        else
          std::printf("0x%06X\n", (unsigned int) ambientColor);
        std::printf("-rscale %.3f\n", reflZScale);
        std::printf("-mlod %d\n", modelLOD);
        std::printf("-vis %d\n", int(distantObjectsOnly));
        std::printf("-ndis %d\n", int(noDisabledObjects));
        std::printf("-watercolor 0x%08X\n", (unsigned int) waterColor);
        std::printf("-wrefl %.3f\n", waterReflectionLevel);
        std::printf("-wscale %d\n", waterUVScale);
        return 0;
      }
      else if (argv[i][0] != '-')
      {
        args.push_back(argv[i]);
      }
      else if (std::strcmp(argv[i], "-threads") == 0)
      {
        if (++i >= argc)
          throw FO76UtilsError("missing argument for %s", argv[i - 1]);
        threadCnt = int(parseInteger(argv[i], 10, "invalid number of threads",
                                     1, 16));
      }
      else if (std::strcmp(argv[i], "-debug") == 0)
      {
        if (++i >= argc)
          throw FO76UtilsError("missing argument for %s", argv[i - 1]);
        debugMode = (unsigned char) parseInteger(argv[i], 10,
                                                 "invalid debug mode", 0, 5);
      }
      else if (std::strcmp(argv[i], "-scol") == 0)
      {
        if (++i >= argc)
          throw FO76UtilsError("missing argument for %s", argv[i - 1]);
        enableSCOL =
            bool(parseInteger(argv[i], 0, "invalid argument for -scol", 0, 1));
      }
      else if (std::strcmp(argv[i], "-a") == 0)
      {
        enableAllObjects = true;
      }
      else if (std::strcmp(argv[i], "-textures") == 0)
      {
        if (++i >= argc)
          throw FO76UtilsError("missing argument for %s", argv[i - 1]);
        enableTextures =
            bool(parseInteger(argv[i], 0, "invalid argument for -textures",
                              0, 1));
      }
      else if (std::strcmp(argv[i], "-txtcache") == 0)
      {
        if (++i >= argc)
          throw FO76UtilsError("missing argument for %s", argv[i - 1]);
        textureCacheSize =
            int(parseInteger(argv[i], 0, "invalid texture cache size",
                             256, 4095));
      }
      else if (std::strcmp(argv[i], "-rq") == 0)
      {
        if (++i >= argc)
          throw FO76UtilsError("missing argument for %s", argv[i - 1]);
        renderQuality =
            (unsigned char) parseInteger(argv[i], 0,
                                         "invalid render quality", 0, 15);
      }
      else if (std::strcmp(argv[i], "-btd") == 0)
      {
        if (++i >= argc)
          throw FO76UtilsError("missing argument for %s", argv[i - 1]);
        btdPath = argv[i];
      }
      else if (std::strcmp(argv[i], "-w") == 0)
      {
        if (++i >= argc)
          throw FO76UtilsError("missing argument for %s", argv[i - 1]);
        formID = (unsigned int) parseInteger(argv[i], 0, "invalid form ID",
                                             0, 0x0FFFFFFF);
      }
      else if (std::strcmp(argv[i], "-r") == 0)
      {
        if ((i + 4) >= argc)
          throw FO76UtilsError("missing argument for %s", argv[i]);
        terrainX0 = int(parseInteger(argv[i + 1], 10, "invalid terrain X0",
                                     -32768, 32767));
        terrainY0 = int(parseInteger(argv[i + 2], 10, "invalid terrain Y0",
                                     -32768, 32767));
        terrainX1 = int(parseInteger(argv[i + 3], 10, "invalid terrain X1",
                                     terrainX0, 32767));
        terrainY1 = int(parseInteger(argv[i + 4], 10, "invalid terrain Y1",
                                     terrainY0, 32767));
        i = i + 4;
      }
      else if (std::strcmp(argv[i], "-l") == 0)
      {
        if (++i >= argc)
          throw FO76UtilsError("missing argument for %s", argv[i - 1]);
        btdLOD = int(parseInteger(argv[i], 10,
                                  "invalid terrain level of detail", 0, 4));
      }
      else if (std::strcmp(argv[i], "-deftxt") == 0)
      {
        if (++i >= argc)
          throw FO76UtilsError("missing argument for %s", argv[i - 1]);
        defTxtID = (unsigned int) parseInteger(argv[i], 0, "invalid form ID",
                                               0, 0x0FFFFFFF);
      }
      else if (std::strcmp(argv[i], "-defclr") == 0)
      {
        if (++i >= argc)
          throw FO76UtilsError("missing argument for %s", argv[i - 1]);
        ltxtDefColor = (std::uint32_t) parseInteger(
                           argv[i], 0, "invalid land texture color",
                           0, 0x00FFFFFF);
      }
      else if (std::strcmp(argv[i], "-ltxtres") == 0)
      {
        if (++i >= argc)
          throw FO76UtilsError("missing argument for %s", argv[i - 1]);
        ltxtResolution = int(parseInteger(argv[i], 0,
                                          "invalid land texture resolution",
                                          8, 4096));
        if (ltxtResolution & (ltxtResolution - 1))
          errorMessage("invalid land texture resolution");
      }
      else if (std::strcmp(argv[i], "-mip") == 0)
      {
        if (++i >= argc)
          throw FO76UtilsError("missing argument for %s", argv[i - 1]);
        textureMip = int(parseInteger(argv[i], 10, "invalid texture mip level",
                                      0, 15));
      }
      else if (std::strcmp(argv[i], "-lmult") == 0)
      {
        if (++i >= argc)
          throw FO76UtilsError("missing argument for %s", argv[i - 1]);
        landTextureMult = float(parseFloat(argv[i],
                                           "invalid land texture RGB scale",
                                           0.5, 8.0));
      }
      else if (std::strcmp(argv[i], "-light") == 0)
      {
        if ((i + 3) >= argc)
          throw FO76UtilsError("missing argument for %s", argv[i]);
        rgbScale = float(parseFloat(argv[i + 1], "invalid RGB scale",
                                    0.125, 4.0));
        lightRotationY = float(parseFloat(argv[i + 2],
                                          "invalid light Y rotation",
                                          -360.0, 360.0));
        lightRotationZ = float(parseFloat(argv[i + 3],
                                          "invalid light Z rotation",
                                          -360.0, 360.0));
        i = i + 3;
      }
      else if (std::strcmp(argv[i], "-lcolor") == 0)
      {
        if ((i + 5) >= argc)
          throw FO76UtilsError("missing argument for %s", argv[i]);
        lightLevel = float(parseFloat(argv[i + 1], "invalid light source level",
                                      0.125, 4.0));
        lightColor = int(parseInteger(argv[i + 2], 0,
                                      "invalid light source color",
                                      -1, 0x00FFFFFF)) & 0x00FFFFFF;
        envLevel = float(parseFloat(argv[i + 3],
                                    "invalid environment light level",
                                    0.125, 4.0));
        envColor = int(parseInteger(argv[i + 4], 0,
                                    "invalid environment light color",
                                    -1, 0x00FFFFFF)) & 0x00FFFFFF;
        ambientColor = int(parseInteger(argv[i + 5], 0, "invalid ambient light",
                                        -1, 0x00FFFFFF));
        i = i + 5;
      }
      else if (std::strcmp(argv[i], "-rscale") == 0)
      {
        if (++i >= argc)
          throw FO76UtilsError("missing argument for %s", argv[i - 1]);
        reflZScale =
            float(parseFloat(argv[i], "invalid reflection view vector Z scale",
                             0.25, 16.0));
      }
      else if (std::strcmp(argv[i], "-mlod") == 0)
      {
        if (++i >= argc)
          throw FO76UtilsError("missing argument for %s", argv[i - 1]);
        modelLOD = int(parseInteger(argv[i], 10, "invalid model LOD", 0, 4));
      }
      else if (std::strcmp(argv[i], "-vis") == 0)
      {
        if (++i >= argc)
          throw FO76UtilsError("missing argument for %s", argv[i - 1]);
        distantObjectsOnly =
            bool(parseInteger(argv[i], 0, "invalid argument for -vis", 0, 1));
      }
      else if (std::strcmp(argv[i], "-ndis") == 0)
      {
        if (++i >= argc)
          throw FO76UtilsError("missing argument for %s", argv[i - 1]);
        noDisabledObjects =
            bool(parseInteger(argv[i], 0, "invalid argument for -ndis", 0, 1));
      }
      else if (std::strcmp(argv[i], "-xm") == 0)
      {
        if (++i >= argc)
          throw FO76UtilsError("missing argument for %s", argv[i - 1]);
        excludeModelPatterns.push_back(argv[i]);
      }
      else if (std::strcmp(argv[i], "-env") == 0)
      {
        if (++i >= argc)
          throw FO76UtilsError("missing argument for %s", argv[i - 1]);
        defaultEnvMap = argv[i];
      }
      else if (std::strcmp(argv[i], "-wtxt") == 0)
      {
        if (++i >= argc)
          throw FO76UtilsError("missing argument for %s", argv[i - 1]);
        waterTexture = argv[i];
      }
      else if (std::strcmp(argv[i], "-watercolor") == 0)
      {
        if (++i >= argc)
          throw FO76UtilsError("missing argument for %s", argv[i - 1]);
        waterColor =
            std::uint32_t(parseInteger(argv[i], 0, "invalid water color",
                                       0, 0x7FFFFFFF));
      }
      else if (std::strcmp(argv[i], "-wrefl") == 0)
      {
        if (++i >= argc)
          throw FO76UtilsError("missing argument for %s", argv[i - 1]);
        waterReflectionLevel =
            float(parseFloat(argv[i], "invalid water environment map scale",
                             0.0, 4.0));
      }
      else if (std::strcmp(argv[i], "-wscale") == 0)
      {
        if (++i >= argc)
          throw FO76UtilsError("missing argument for %s", argv[i - 1]);
        waterUVScale =
            int(parseInteger(argv[i], 0, "invalid water texture tile size",
                             2, 32768));
      }
      else
      {
        throw FO76UtilsError("invalid option: %s", argv[i]);
      }
    }
    if (args.size() != 4)
    {
      SDLDisplay::enableConsole();
      for (size_t i = 0; usageStrings[i]; i++)
        std::fprintf(stderr, "%s\n", usageStrings[i]);
      return err;
    }
    if (!(btdPath && *btdPath != '\0'))
    {
      btdPath = (char *) 0;
      btdLOD = 2;
    }
    if (debugMode == 1)
    {
      enableSCOL = true;
      ltxtResolution = 128 >> btdLOD;
    }
    if (!formID)
      formID = (!btdPath ? 0x0000003CU : 0x0025DA15U);
    waterColor =
        waterColor + (waterColor & 0x7F000000U) + ((waterColor >> 30) << 24);
    if (!(waterColor & 0xFF000000U))
      waterColor = 0U;
    int     width =
        int(parseInteger(args[1], 0, "invalid image width", 640, 16384));
    int     height =
        int(parseInteger(args[2], 0, "invalid image height", 360, 16384));

    BA2File ba2File(args[3]);
    ESMFile esmFile(args[0]);
    unsigned int  worldID = Renderer::findParentWorld(esmFile, formID);
    if (worldID == 0xFFFFFFFFU)
      errorMessage("form ID not found in ESM, or invalid record type");

    size_t  imageDataSize = size_t(width) * size_t(height);
    imageBuf.resize(imageDataSize, 0U);
    Renderer  renderer(width, height, ba2File, esmFile, imageBuf.data());
    if (threadCnt > 0)
      renderer.setThreadCount(threadCnt);
    renderer.setTextureCacheSize(size_t(textureCacheSize) << 20);
    renderer.setDistantObjectsOnly(distantObjectsOnly);
    renderer.setNoDisabledObjects(noDisabledObjects);
    renderer.setEnableSCOL(enableSCOL);
    renderer.setEnableAllObjects(enableAllObjects);
    renderer.setRenderQuality(renderQuality);
    renderer.setEnableTextures(enableTextures);
    renderer.setDebugMode(debugMode);
    renderer.setLandDefaultColor(ltxtDefColor);
    renderer.setLandTxtResolution(ltxtResolution);
    renderer.setTextureMipLevel(textureMip);
    renderer.setLandTextureMip(0.0f);
    renderer.setLandTxtRGBScale(landTextureMult);
    renderer.setModelLOD(modelLOD);
    renderer.setWaterColor(waterColor);
    renderer.setWaterEnvMapScale(waterReflectionLevel);
    if (defaultEnvMap && *defaultEnvMap)
      renderer.setDefaultEnvMap(std::string(defaultEnvMap));
    if (waterColor && waterTexture && *waterTexture)
      renderer.setWaterTexture(std::string(waterTexture));

    for (size_t i = 0; i < excludeModelPatterns.size(); i++)
    {
      if (excludeModelPatterns[i] && excludeModelPatterns[i][0])
        renderer.addExcludeModelPattern(std::string(excludeModelPatterns[i]));
    }

    display = new SDLDisplay(width, height, "World space viewer", 4U,
                             (height < 540 ? 24 : (height < 720 ? 36 : 48)));
    display->setDefaultTextColor(0x00, 0xC1);
    int     renderPass = 0;
    bool    renderPassCompleted = false;
    bool    quitFlag = false;
    bool    redrawScreenFlag = true;
    bool    redrawWorldFlag = true;
    std::time_t lastRedrawTime = std::time((std::time_t *) 0);
    do
    {
      if (redrawWorldFlag)
      {
        redrawWorldFlag = false;
        renderPass = 0;
        renderPassCompleted = false;
        renderer.clearImage();
        display->clearTextBuffer();
        viewOffsX = -camPositionX;
        viewOffsY = -camPositionY;
        viewOffsZ = -camPositionZ;
        float   rX = viewRotations[viewRotation * 3] * d;
        float   rY = viewRotations[viewRotation * 3 + 1] * d;
        float   rZ = viewRotations[viewRotation * 3 + 2] * d;
        NIFFile::NIFVertexTransform vt(viewScale, rX, rY, rZ, 0.0f, 0.0f, 0.0f);
        vt.transformXYZ(viewOffsX, viewOffsY, viewOffsZ);
        viewOffsX = viewOffsX + (float(width) * 0.5f);
        viewOffsY = viewOffsY + (float(height - 2) * 0.5f);
        viewOffsX = float(roundFloat(viewOffsX)) + 0.125f;
        viewOffsY = float(roundFloat(viewOffsY)) + 0.125f;
        renderer.setViewTransform(
            viewScale, rX, rY, rZ, viewOffsX, viewOffsY, viewOffsZ);
        renderer.setLightDirection(lightRotationY * d, lightRotationZ * d);
        renderer.setRenderParameters(
            lightColor, ambientColor, envColor, lightLevel, envLevel,
            rgbScale, reflZScale, waterUVScale);
        if (worldID)
        {
          display->consolePrint(
              "Loading terrain data and landscape textures\n");
          display->clearSurface();
          display->drawText(0, -1, display->getTextRows(), 0.75f, 1.0f);
          display->blitSurface();
          renderer.loadTerrain(btdPath, worldID, defTxtID, btdLOD,
                               terrainX0, terrainY0, terrainX1, terrainY1);
          display->consolePrint("Rendering terrain\n");
          renderer.initRenderPass(0, worldID);
          redrawScreenFlag = true;
        }
        else
        {
          renderPassCompleted = true;
        }
      }
      else if (renderPassCompleted)
      {
        renderPassCompleted = false;
        if (worldID && viewScale < 0.046875f)
          renderPass = 2;
        if (++renderPass < 3)
        {
          if (renderPass == 1)
            display->consolePrint("Rendering objects\n");
          else
            display->consolePrint("Rendering water and transparent objects\n");
          renderer.initRenderPass(renderPass, formID);
        }
        redrawScreenFlag = true;
      }
      else if (renderPass < 3)
      {
        renderPassCompleted = renderer.renderObjects();
        if (!renderPassCompleted)
        {
          display->consolePrint("\r    %7u / %7u  ",
                                (unsigned int) renderer.getObjectsRendered(),
                                (unsigned int) renderer.getObjectCount());
        }
        else
        {
          display->consolePrint("\r    %7u / %7u  \n",
                                (unsigned int) renderer.getObjectCount(),
                                (unsigned int) renderer.getObjectCount());
        }
        if (std::time((std::time_t *) 0) != lastRedrawTime)
          redrawScreenFlag = true;
      }
      if (redrawScreenFlag)
      {
        redrawScreenFlag = false;
        lastRedrawTime = std::time((std::time_t *) 0);
        std::memcpy(display->lockDrawSurface(), imageBuf.data(),
                    imageDataSize * sizeof(std::uint32_t));
        display->unlockDrawSurface();
        display->drawText(0, -1, display->getTextRows(), 0.75f, 1.0f);
        display->blitSurface();
      }

      display->pollEvents(eventBuf, (renderPass < 3 ? 0 : -1000), false, true);
      float   xStep = 0.0f;
      float   yStep = 0.0f;
      float   zStep = 0.0f;
      for (size_t i = 0; i < eventBuf.size(); i++)
      {
        if (eventBuf[i].type() == SDLDisplay::SDLEventWindow)
        {
          if (eventBuf[i].data1() == 0)
            quitFlag = true;
          else if (eventBuf[i].data1() == 1)
            redrawScreenFlag = true;
        }
        if (eventBuf[i].type() == SDLDisplay::SDLEventMButtonDown &&
            (eventBuf[i].data3() == 1 || eventBuf[i].data3() == 3) &&
            eventBuf[i].data4() == 2)
        {
          int     x = eventBuf[i].data1();
          int     y = eventBuf[i].data2();
          x = (x > 0 ? (x < (width - 1) ? x : (width - 1)) : 0);
          y = (y > 0 ? (y < (height - 1) ? y : (height - 1)) : 0);
          int     x0 = (width + 1) >> 1;
          int     y0 = (height - 1) >> 1;
          xStep += float(x - x0);
          yStep += float(y - y0);
          if (eventBuf[i].data3() == 3)
          {
            const float *zBuf = renderer.getZBufferData();
            float   z = zBuf[size_t(y) * size_t(width) + size_t(x)];
            if (z < 1000000.0f)
              zStep += (z - 1.0f);
          }
        }
        if (eventBuf[i].type() != SDLDisplay::SDLEventKeyDown)
          continue;
        switch (eventBuf[i].data1())
        {
          case '-':
          case SDLDisplay::SDLKeySymKPMinus:
            {
              int     tmp = roundFloat(float(std::log2(viewScale)) * 2.0f);
              if (tmp > -18)
              {
                viewScale = float(std::exp2(float(tmp - 1) * 0.5f));
                redrawWorldFlag = true;
              }
            }
            break;
          case '=':
          case SDLDisplay::SDLKeySymKPPlus:
            {
              int     tmp = roundFloat(float(std::log2(viewScale)) * 2.0f);
              if (tmp < 4)
              {
                viewScale = float(std::exp2(float(tmp + 1) * 0.5f));
                redrawWorldFlag = true;
              }
            }
            break;
          case SDLDisplay::SDLKeySymKP7:
            redrawWorldFlag = redrawWorldFlag | (viewRotation != 0);
            viewRotation = 0;
            break;
          case SDLDisplay::SDLKeySymKP1:
            redrawWorldFlag = redrawWorldFlag | (viewRotation != 1);
            viewRotation = 1;
            break;
          case SDLDisplay::SDLKeySymKP3:
            redrawWorldFlag = redrawWorldFlag | (viewRotation != 2);
            viewRotation = 2;
            break;
          case SDLDisplay::SDLKeySymKP9:
            redrawWorldFlag = redrawWorldFlag | (viewRotation != 3);
            viewRotation = 3;
            break;
          case SDLDisplay::SDLKeySymKP5:
            redrawWorldFlag = redrawWorldFlag | (viewRotation != 4);
            viewRotation = 4;
            break;
          case SDLDisplay::SDLKeySymKP2:
            redrawWorldFlag = redrawWorldFlag | (viewRotation != 5);
            viewRotation = 5;
            break;
          case SDLDisplay::SDLKeySymKP6:
            redrawWorldFlag = redrawWorldFlag | (viewRotation != 6);
            viewRotation = 6;
            break;
          case SDLDisplay::SDLKeySymKP8:
            redrawWorldFlag = redrawWorldFlag | (viewRotation != 7);
            viewRotation = 7;
            break;
          case SDLDisplay::SDLKeySymKP4:
            redrawWorldFlag = redrawWorldFlag | (viewRotation != 8);
            viewRotation = 8;
            break;
          case 'a':
          case 'd':
          case 'e':
          case 's':
          case 'w':
          case 'x':
            {
              float   stepSize = float(width < height ? width : height) * 0.5f;
              switch (eventBuf[i].data1())
              {
                case 'a':
                  xStep -= stepSize;
                  break;
                case 'd':
                  xStep += stepSize;
                  break;
                case 'e':
                  yStep -= stepSize;
                  break;
                case 's':
                  zStep -= stepSize;
                  break;
                case 'w':
                  zStep += stepSize;
                  break;
                case 'x':
                  yStep += stepSize;
                  break;
                default:
                  break;
              }
            }
            break;
          case SDLDisplay::SDLKeySymEscape:
            quitFlag = true;
            break;
          default:
            break;
        }
      }
      if (!(xStep == 0.0f && yStep == 0.0f && zStep == 0.0f))
      {
        float   rX = viewRotations[viewRotation * 3] * d;
        float   rY = viewRotations[viewRotation * 3 + 1] * d;
        float   rZ = viewRotations[viewRotation * 3 + 2] * d;
        NIFFile::NIFVertexTransform viewTransformInv(
            1.0f / viewScale, rX, rY, rZ, 0.0f, 0.0f, 0.0f);
        NIFFile::NIFVertexTransform tmp(viewTransformInv);
        viewTransformInv.rotateYX = tmp.rotateXY;
        viewTransformInv.rotateZX = tmp.rotateXZ;
        viewTransformInv.rotateXY = tmp.rotateYX;
        viewTransformInv.rotateZY = tmp.rotateYZ;
        viewTransformInv.rotateXZ = tmp.rotateZX;
        viewTransformInv.rotateYZ = tmp.rotateZY;
        viewTransformInv.transformXYZ(xStep, yStep, zStep);
        camPositionX += xStep;
        camPositionY += yStep;
        camPositionZ += zStep;
        redrawWorldFlag = true;
      }
    }
    while (!quitFlag);

    if (false)
    {
      const NIFFile::NIFBounds& b = renderer.getBounds();
      if (b.xMax() > b.xMin())
      {
        display->consolePrint(
            "Bounds: %6.0f, %6.0f, %6.0f to %6.0f, %6.0f, %6.0f\n",
            b.xMin(), b.yMin(), b.zMin(), b.xMax(), b.yMax(), b.zMax());
      }
    }
    err = 0;
  }
  catch (std::exception& e)
  {
    if (!display)
    {
      SDLDisplay::enableConsole();
      std::fprintf(stderr, "wrldview: %s\n", e.what());
    }
    else
    {
      display->unlockScreenSurface();
      display->clearSurface();
      display->clearTextBuffer();
      display->consolePrint("\033[41m\033[33m\033[1m    Error: %s    ",
                            e.what());
      display->drawText(0, -1, display->getTextRows(), 1.0f, 1.0f);
      display->blitSurface();
      bool    quitFlag = false;
      do
      {
        display->pollEvents(eventBuf);
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
            display->blitSurface();
          }
        }
      }
      while (!quitFlag);
      delete display;
      display = (SDLDisplay *) 0;
    }
    err = 1;
  }
  if (display)
    delete display;
  return err;
}

