
#include "common.hpp"
#include "fp32vec4.hpp"
#include "downsamp.hpp"
#include "render.hpp"

static const char *usageStrings[] =
{
  "Usage: render INFILE.ESM[,...] OUTFILE.DDS W H ARCHIVEPATH [OPTIONS...]",
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
  "    -ssaa INT           render at 2^N resolution and downsample",
  "    -f INT              output format, 0: RGB24, 1: A8R8G8B8, 2: RGB10A2",
  "    -rq INT             set render quality (0 to 255, see doc/render.md)",
  "    -watermask BOOL     make non-water surfaces transparent or black",
  "    -q                  do not print messages other than errors",
  "",
  "    -btd FILENAME.BTD   read terrain data from Fallout 76 .btd file",
  "    -w FORMID           form ID of world, cell, or object to render",
  "    -r X0 Y0 X1 Y1      terrain cell range, X0,Y0 = SW to X1,Y1 = NE",
  "    -l INT              level of detail to use from BTD file (0 to 4)",
  "    -deftxt FORMID      form ID of default land texture",
  "    -defclr 0x00RRGGBB  default color for untextured terrain",
  "    -ltxtres INT        land texture resolution per cell",
  "    -mip INT            base mip level for all textures",
  "    -lmip FLOAT         additional mip level for land textures",
  "    -lmult FLOAT        land texture RGB level scale",
  "",
  "    -view SCALE RX RY RZ OFFS_X OFFS_Y OFFS_Z",
  "                        set transform from world coordinates to image",
  "                        coordinates, rotations are in degrees",
  "    -cam SCALE RX RY RZ X Y Z",
  "                        set view transform for camera position X,Y,Z",
  "    -zrange MIN MAX     limit view Z range to MIN <= Z < MAX",
  "    -light SCALE RY RZ  set RGB scale and Y, Z rotation (0, 0 = top)",
  "    -lcolor LMULT LCOLOR EMULT ECOLOR ACOLOR",
  "                        set light source, environment, and ambient light",
  "                        colors and levels (colors in 0xRRGGBB format)",
  "    -rscale FLOAT       reflection view vector Z scale",
  "",
  "    -mlod INT           set level of detail for models, 0 (best) to 4",
  "    -vis BOOL           render only objects visible from distance",
  "    -ndis BOOL          do not render initially disabled objects",
  "    -hqm STRING         add high quality model path name pattern",
  "    -xm STRING          add excluded model path name pattern",
  "",
  "    -env FILENAME.DDS   default environment map texture path in archives",
  "    -wtxt FILENAME.DDS  water normal map texture path in archives",
  "    -watercolor UINT32  water color (A7R8G8B8), 0 disables water",
  "    -wrefl FLOAT        water environment map scale",
  "    -wscale INT         water texture tile size",
  (char *) 0
};

int main(int argc, char **argv)
{
  int     err = 1;
  try
  {
    std::vector< const char * > args;
    int     threadCnt = -1;
    int     textureCacheSize = 1024;
    bool    verboseMode = true;
    bool    distantObjectsOnly = false;
    bool    noDisabledObjects = true;
    unsigned char ssaaLevel = 0;
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
    int     textureMip = 2;
    float   landTextureMip = 3.0f;
    float   landTextureMult = 1.0f;
    int     modelLOD = 0;
    std::uint32_t waterColor = 0x7FFFFFFFU;
    float   waterReflectionLevel = 1.0f;
    int     zMin = 0;
    int     zMax = 16777216;
    float   viewScale = 0.0625f;
    float   viewRotationX = 180.0f;
    float   viewRotationY = 0.0f;
    float   viewRotationZ = 0.0f;
    float   viewOffsX = 0.0f;
    float   viewOffsY = 0.0f;
    float   viewOffsZ = 32768.0f;
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
    int     outputFormat = 0;
    unsigned char renderQuality = 0;
    bool    waterMaskMode = false;
    const char  *defaultEnvMap =
        "textures/shared/cubemaps/mipblur_defaultoutside1.dds";
    const char  *waterTexture = "textures/water/defaultwater.dds";
    std::vector< const char * > hdModelNamePatterns;
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
        std::printf("-ssaa %d\n", int(ssaaLevel));
        std::printf("-f %d\n", outputFormat);
        std::printf("-rq %d\n", int(renderQuality));
        std::printf("-watermask %d\n", int(waterMaskMode));
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
        std::printf("-lmip %.1f\n", landTextureMip);
        std::printf("-lmult %.1f\n", landTextureMult);
        std::printf("-view %.6f %.1f %.1f %.1f %.1f %.1f %.1f\n",
                    viewScale, viewRotationX, viewRotationY, viewRotationZ,
                    viewOffsX, viewOffsY, viewOffsZ);
        {
          NIFFile::NIFVertexTransform
              vt(1.0f, viewRotationX * d, viewRotationY * d, viewRotationZ * d,
                 0.0f, 0.0f, 0.0f);
          std::printf("    Rotation matrix:\n");
          std::printf("        X: %9.6f %9.6f %9.6f\n",
                      vt.rotateXX, vt.rotateXY, vt.rotateXZ);
          std::printf("        Y: %9.6f %9.6f %9.6f\n",
                      vt.rotateYX, vt.rotateYY, vt.rotateYZ);
          std::printf("        Z: %9.6f %9.6f %9.6f\n",
                      vt.rotateZX, vt.rotateZY, vt.rotateZZ);
        }
        std::printf("-zrange %d %d\n", zMin, zMax);
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
      else if (std::strcmp(argv[i], "-ssaa") == 0)
      {
        if (++i >= argc)
          throw FO76UtilsError("missing argument for %s", argv[i - 1]);
        ssaaLevel =
            (unsigned char) parseInteger(argv[i], 0,
                                         "invalid argument for -ssaa", 0, 2);
      }
      else if (std::strcmp(argv[i], "-f") == 0)
      {
        if (++i >= argc)
          throw FO76UtilsError("missing argument for %s", argv[i - 1]);
        outputFormat =
            int(parseInteger(argv[i], 10, "invalid output format", 0, 2));
      }
      else if (std::strcmp(argv[i], "-rq") == 0)
      {
        if (++i >= argc)
          throw FO76UtilsError("missing argument for %s", argv[i - 1]);
        renderQuality =
            (unsigned char) parseInteger(argv[i], 0,
                                         "invalid render quality", 0, 255);
      }
      else if (std::strcmp(argv[i], "-watermask") == 0)
      {
        if (++i >= argc)
          throw FO76UtilsError("missing argument for %s", argv[i - 1]);
        waterMaskMode =
            bool(parseInteger(argv[i], 0, "invalid argument for -watermask",
                              0, 1));
      }
      else if (std::strcmp(argv[i], "-q") == 0)
      {
        verboseMode = false;
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
      else if (std::strcmp(argv[i], "-lmip") == 0)
      {
        if (++i >= argc)
          throw FO76UtilsError("missing argument for %s", argv[i - 1]);
        landTextureMip = float(parseFloat(argv[i],
                                          "invalid land texture mip level",
                                          0.0, 15.0));
      }
      else if (std::strcmp(argv[i], "-lmult") == 0)
      {
        if (++i >= argc)
          throw FO76UtilsError("missing argument for %s", argv[i - 1]);
        landTextureMult = float(parseFloat(argv[i],
                                           "invalid land texture RGB scale",
                                           0.5, 8.0));
      }
      else if (std::strcmp(argv[i], "-view") == 0 ||
               std::strcmp(argv[i], "-cam") == 0)
      {
        if ((i + 7) >= argc)
          throw FO76UtilsError("missing argument for %s", argv[i]);
        viewScale = float(parseFloat(argv[i + 1], "invalid view scale",
                                     1.0 / 512.0, 16.0));
        viewRotationX = float(parseFloat(argv[i + 2], "invalid view X rotation",
                                         -360.0, 360.0));
        viewRotationY = float(parseFloat(argv[i + 3], "invalid view Y rotation",
                                         -360.0, 360.0));
        viewRotationZ = float(parseFloat(argv[i + 4], "invalid view Z rotation",
                                         -360.0, 360.0));
        viewOffsX = float(parseFloat(argv[i + 5], "invalid view X offset",
                                     -1048576.0, 1048576.0));
        viewOffsY = float(parseFloat(argv[i + 6], "invalid view Y offset",
                                     -1048576.0, 1048576.0));
        viewOffsZ = float(parseFloat(argv[i + 7], "invalid view Z offset",
                                     -1048576.0, 1048576.0));
        if (argv[i][1] == 'c')
        {
          NIFFile::NIFVertexTransform vt(
              viewScale,
              viewRotationX * d, viewRotationY * d, viewRotationZ * d,
              0.0f, 0.0f, 0.0f);
          viewOffsX = -viewOffsX;
          viewOffsY = -viewOffsY;
          viewOffsZ = -viewOffsZ;
          vt.transformXYZ(viewOffsX, viewOffsY, viewOffsZ);
          if (verboseMode)
          {
            std::fprintf(stderr, "View offset: %.2f, %.2f, %.2f\n",
                         viewOffsX, viewOffsY, viewOffsZ);
          }
        }
        i = i + 7;
      }
      else if (std::strcmp(argv[i], "-zrange") == 0)
      {
        if ((i + 2) >= argc)
          throw FO76UtilsError("missing argument for %s", argv[i]);
        zMin = int(parseInteger(argv[i + 1], 10, "invalid Z range",
                                0, 1048575));
        zMax = int(parseInteger(argv[i + 2], 10, "invalid Z range",
                                zMin + 1, 16777216));
        i = i + 2;
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
      else if (std::strcmp(argv[i], "-hqm") == 0)
      {
        if (++i >= argc)
          throw FO76UtilsError("missing argument for %s", argv[i - 1]);
        hdModelNamePatterns.push_back(argv[i]);
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
    if (args.size() != 5)
    {
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
      ssaaLevel = 0;
      enableSCOL = true;
      ltxtResolution = 128 >> btdLOD;
    }
    if (waterMaskMode)
    {
      enableTextures = false;
      debugMode = 4;
      ltxtResolution = 128 >> btdLOD;
      waterColor = 0x7FFFFFFFU;
      renderQuality = (renderQuality & 0x03) | 0x10;
      waterTexture = (char *) 0;
    }
    if (!formID)
      formID = (!btdPath ? 0x0000003CU : 0x0025DA15U);
    waterColor =
        waterColor + (waterColor & 0x7F000000U) + ((waterColor >> 30) << 24);
    if (!(waterColor & 0xFF000000U))
      waterColor = 0U;
    int     width =
        int(parseInteger(args[2], 0, "invalid image width", 2, 32768));
    int     height =
        int(parseInteger(args[3], 0, "invalid image height", 2, 32768));
    viewOffsX = viewOffsX + (float(width) * 0.5f);
    viewOffsY = viewOffsY + (float(height - 2) * 0.5f);
    viewOffsZ = viewOffsZ - float(zMin);
    zMax = zMax - zMin;
    if (ssaaLevel > 0)
    {
      width = width << ssaaLevel;
      height = height << ssaaLevel;
      float   ssaaScale = float(1 << ssaaLevel);
      viewScale = viewScale * ssaaScale;
      viewOffsX = viewOffsX * ssaaScale;
      viewOffsY = viewOffsY * ssaaScale;
      viewOffsZ = viewOffsZ * ssaaScale;
      zMax = zMax << ssaaLevel;
      zMax = (zMax < 16777216 ? zMax : 16777216);
    }

    BA2File ba2File(args[4]);
    ESMFile esmFile(args[0]);
    unsigned int  worldID = Renderer::findParentWorld(esmFile, formID);
    if (worldID == 0xFFFFFFFFU)
      errorMessage("form ID not found in ESM, or invalid record type");

    Renderer  renderer(width, height, ba2File, esmFile,
                       (std::uint32_t *) 0, (float *) 0, zMax);
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
    renderer.setLandTextureMip(landTextureMip);
    renderer.setLandTxtRGBScale(landTextureMult);
    renderer.setModelLOD(modelLOD);
    renderer.setWaterColor(waterColor);
    renderer.setWaterEnvMapScale(waterReflectionLevel);
    renderer.setViewTransform(
        viewScale, viewRotationX * d, viewRotationY * d, viewRotationZ * d,
        viewOffsX, viewOffsY, viewOffsZ);
    renderer.setLightDirection(lightRotationY * d, lightRotationZ * d);
    if (defaultEnvMap && *defaultEnvMap)
      renderer.setDefaultEnvMap(std::string(defaultEnvMap));
    if (waterColor && waterTexture && *waterTexture)
      renderer.setWaterTexture(std::string(waterTexture));
    renderer.setRenderParameters(
        lightColor, ambientColor, envColor, lightLevel, envLevel, rgbScale,
        reflZScale, waterUVScale);

    for (size_t i = 0; i < hdModelNamePatterns.size(); i++)
    {
      if (hdModelNamePatterns[i] && hdModelNamePatterns[i][0])
        renderer.addHDModelPattern(std::string(hdModelNamePatterns[i]));
    }
    for (size_t i = 0; i < excludeModelPatterns.size(); i++)
    {
      if (excludeModelPatterns[i] && excludeModelPatterns[i][0])
        renderer.addExcludeModelPattern(std::string(excludeModelPatterns[i]));
    }

    int     renderPass = 1;
    if (worldID)
    {
      if (verboseMode)
        std::fprintf(stderr, "Loading terrain data and landscape textures\n");
      renderer.loadTerrain(btdPath, worldID, defTxtID,
                           btdLOD, terrainX0, terrainY0, terrainX1, terrainY1);
      renderPass = 0;
    }
    do
    {
      static const char *renderObjectTypes[3] =
      {
        "terrain", "objects", "water and transparent objects"
      };
      if (verboseMode)
        std::fprintf(stderr, "Rendering %s\n", renderObjectTypes[renderPass]);
      if (waterMaskMode)
        renderer.clearImage(0x01);
      renderer.initRenderPass(renderPass, (!renderPass ? worldID : formID));
      while (!renderer.renderObjects())
      {
        if (verboseMode)
        {
          std::fprintf(stderr, "\r    %7u / %7u  ",
                       (unsigned int) renderer.getObjectsRendered(),
                       (unsigned int) renderer.getObjectCount());
        }
      }
      if (verboseMode)
      {
        std::fprintf(stderr, "\r    %7u / %7u  \n",
                     (unsigned int) renderer.getObjectCount(),
                     (unsigned int) renderer.getObjectCount());
      }
      if (renderPass != 1)
        renderer.clear();
    }
    while (++renderPass <= 2);
    renderer.deallocateBuffers(0x02);
    if (verboseMode)
    {
      const NIFFile::NIFBounds& b = renderer.getBounds();
      if (b.xMax() > b.xMin())
      {
        float   scale = float(8 >> ssaaLevel) * 0.125f;
        std::fprintf(stderr,
                     "Bounds: %6.0f, %6.0f, %6.0f to %6.0f, %6.0f, %6.0f\n",
                     b.xMin() * scale, b.yMin() * scale, b.zMin() * scale,
                     b.xMax() * scale, b.yMax() * scale, b.zMax() * scale);
      }
    }

    width = width >> ssaaLevel;
    height = height >> ssaaLevel;
    const std::uint32_t *imageDataPtr = renderer.getImageData();
    size_t  imageDataSize = size_t(width) * size_t(height);
    std::vector< std::uint32_t >  downsampleBuf;
    if (ssaaLevel > 0)
    {
      downsampleBuf.resize(imageDataSize);
      if (ssaaLevel == 1)
      {
        downsample2xFilter(
            downsampleBuf.data(), imageDataPtr, width << 1, height << 1,
            width, (unsigned char) ((outputFormat & 2) | USE_PIXELFMT_RGB10A2));
      }
      else
      {
        downsample4xFilter(
            downsampleBuf.data(), imageDataPtr, width << 2, height << 2,
            width, (unsigned char) ((outputFormat & 2) | USE_PIXELFMT_RGB10A2));
      }
      imageDataPtr = downsampleBuf.data();
    }
    if (!outputFormat)
      outputFormat = DDSInputFile::pixelFormatRGB24;
    else if (outputFormat == 1)
      outputFormat = DDSInputFile::pixelFormatRGBA32;
    else
      outputFormat = DDSInputFile::pixelFormatA2R10G10B10;
    DDSOutputFile outFile(args[1], width, height, outputFormat);
    outFile.writeImageData(imageDataPtr, imageDataSize,
                           outputFormat, outputFormat);
    err = 0;
  }
  catch (std::exception& e)
  {
    std::fprintf(stderr, "render: %s\n", e.what());
    err = 1;
  }
  return err;
}

