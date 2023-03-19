
#include "common.hpp"
#include "fp32vec4.hpp"
#include "render.hpp"
#include "sdlvideo.hpp"
#include "markers.hpp"

#include <ctime>
#include <SDL2/SDL.h>

static const char *cmdOptsTable[] =
{
  "1btd",               //  0
  "5cam",               //  1
  "1debug",             //  2
  "1defclr",            //  3
  "1deftxt",            //  4
  "1env",               //  5
  "1ft",                //  6
  "0h",                 //  7
  "0help",              //  8
  "1l",                 //  9
  "5lcolor",            // 10
  "3light",             // 11
  "0list",              // 12
  "0list-defaults",     // 13
  "1lmult",             // 14
  "1ltxtres",           // 15
  "1markers",           // 16
  "1minscale",          // 17
  "1mip",               // 18
  "1mlod",              // 19
  "1ndis",              // 20
  "4r",                 // 21
  "1rq",                // 22
  "1rscale",            // 23
  "1textures",          // 24
  "1threads",           // 25
  "1txtcache",          // 26
  "1vis",               // 27
  "1w",                 // 28
  "1watercolor",        // 29
  "1wrefl",             // 30
  "1wscale",            // 31
  "1wtxt",              // 32
  "1xm",                // 33
  "0xm_clear",          // 34
  "1zrange"             // 35
};

static const char *usageStrings[] =
{
  "Usage: wrldview INFILE.ESM[,...] W H ARCHIVEPATH [OPTIONS...]",
  "",
  "Options:",
  "    --help | -h         print usage",
  "    --list              print defaults for all options, and exit",
  "    --                  remaining options are file names",
  "    -threads INT        set the number of threads to use",
  "    -debug INT          set debug render mode (0: disabled, 1: form IDs,",
  "                        2: depth, 3: normals, 4: diffuse, 5: light only)",
  "    -textures BOOL      make all diffuse textures white if false",
  "    -txtcache INT       texture cache size in megabytes",
  "    -rq INT             set render quality (0 to 15, see doc/render.md)",
  "    -ft INT             minimum frame time in milliseconds",
  "    -markers FILENAME   read marker definitions from the specified file",
  "",
  "    -btd FILENAME.BTD   read terrain data from Fallout 76 .btd file",
  "    -w FORMID           form ID of world, cell, or object to render",
  "    -r X0 Y0 X1 Y1      terrain cell range, X0,Y0 = SW to X1,Y1 = NE",
  "    -l INT              level of detail to use from BTD file (0 to 4)",
  "    -deftxt FORMID      form ID of default land texture",
  "    -defclr 0x00RRGGBB  default color for untextured terrain",
  "    -ltxtres INT        maximum land texture resolution per cell",
  "    -mip INT            base mip level for all textures",
  "    -lmult FLOAT        land texture RGB level scale",
  "",
  "    -cam SCALE DIR X Y Z",
  "                        set view scale from world to image coordinates,",
  "                        view direction (0 to 19: NW, SW, SE, NE, top,",
  "                        S, E, N, W, bottom), and camera position",
  "    -zrange FLOAT       limit Z range in view space",
  "    -light SCALE RY RZ  set RGB scale and Y, Z rotation (0, 0 = top)",
  "    -lcolor LMULT LCOLOR EMULT ECOLOR ACOLOR",
  "                        set light source, environment, and ambient light",
  "                        colors and levels (colors in 0xRRGGBB format)",
  "    -rscale FLOAT       reflection view vector Z scale",
  "",
  "    -mlod INT           set level of detail for models, 0 (best) to 4",
  "    -vis BOOL           render only objects visible from distance",
  "    -ndis BOOL          do not render initially disabled objects",
  "    -minscale FLOAT     minimum view scale to render exterior objects at",
  "    -xm STRING          add excluded model path name pattern",
  "    -xm_clear           clear excluded model path name patterns",
  "",
  "    -env FILENAME.DDS   default environment map texture path in archives",
  "    -wtxt FILENAME.DDS  water normal map texture path in archives",
  "    -watercolor UINT32  water color (A7R8G8B8), 0 disables water",
  "    -wrefl FLOAT        water environment map scale",
  "    -wscale INT         water texture tile size",
  (char *) 0
};

static const float  viewRotations[60] =
{
  54.73561f,  180.0f,     45.0f,        // isometric from NW
  54.73561f,  180.0f,     135.0f,       // isometric from SW
  54.73561f,  180.0f,     -135.0f,      // isometric from SE
  54.73561f,  180.0f,     -45.0f,       // isometric from NE
  180.0f,     0.0f,       0.0f,         // top
  -90.0f,     0.0f,       0.0f,         // S
  -90.0f,     0.0f,       90.0f,        // E
  -90.0f,     0.0f,       180.0f,       // N
  -90.0f,     0.0f,       -90.0f,       // W
    0.0f,     0.0f,       0.0f,         // bottom
  54.73561f,  180.0f,     0.0f,         // isometric from N
  54.73561f,  180.0f,     90.0f,        // isometric from W
  54.73561f,  180.0f,     180.0f,       // isometric from S
  54.73561f,  180.0f,     -90.0f,       // isometric from E
  180.0f,     0.0f,       -45.0f,       // top (up = NE)
  -90.0f,     0.0f,       -45.0f,       // SW
  -90.0f,     0.0f,       45.0f,        // SE
  -90.0f,     0.0f,       135.0f,       // NE
  -90.0f,     0.0f,       -135.0f,      // NW
    0.0f,     0.0f,       -45.0f        // bottom
};

struct WorldSpaceViewer
{
  static constexpr float  d = 0.01745329252f;   // degrees to radians
  std::vector< std::uint32_t >  imageBuf;
  std::vector< SDLDisplay::SDLEvent > eventBuf;
  Renderer  *renderer;
  int     width;
  int     height;
  int     threadCnt;
  int     textureCacheSize;
  bool    distantObjectsOnly;
  bool    noDisabledObjects;
  bool    enableTextures;
  unsigned char debugMode;
  unsigned int  formID;
  unsigned int  worldID;                // 0 for interior cells
  int     btdLOD;
  std::string btdPath;
  std::int16_t  terrainX0;
  std::int16_t  terrainY0;
  std::int16_t  terrainX1;
  std::int16_t  terrainY1;
  unsigned int  defTxtID;
  std::uint32_t ltxtDefColor;
  int     ltxtMaxResolution;
  int     textureMip;
  float   landTextureMult;
  int     modelLOD;
  std::uint32_t waterColor;
  float   waterReflectionLevel;
  std::uint16_t frameTimeMin;
  unsigned char renderQuality;
  unsigned char viewRotation;
  float   viewScale;
  float   camPositionX;
  float   camPositionY;
  float   camPositionZ;
  float   zMax;
  float   rgbScale;
  float   lightRotationY;
  float   lightRotationZ;
  int     lightColor;
  int     ambientColor;
  int     envColor;
  float   lightLevel;
  float   envLevel;
  float   reflZScale;
  int     waterUVScale;
  float   objectsMinScale;      // terrain only exterior below this view scale
  std::string defaultEnvMap;
  std::string waterTexture;
  std::vector< std::string >  excludeModelPatterns;
  std::string markerDefsFileName;
  BA2File ba2File;
  ESMFile esmFile;
  SDLDisplay  display;
  size_t  imageDataSize;
  int     renderPass;
  bool    renderPassCompleted;
  bool    quitFlag;
  bool    redrawScreenFlag;
  bool    redrawWorldFlag;
  std::uint64_t lastRedrawTime;
  std::map< size_t, std::string > cmdHistory1;
  std::map< std::string, size_t > cmdHistory2;
  std::string cmdBuf;
  char    tmpBuf[1024];
  WorldSpaceViewer(int w, int h, const char *esmFiles, const char *archivePath,
                   int argc, const char * const *argv);
  virtual ~WorldSpaceViewer();
  void calculateViewOffset(float& x, float& y, float& z) const;
  void setRenderParams(int argc, const char * const *argv);
#ifdef __GNUC__
  __attribute__ ((__format__ (__printf__, 2, 3)))
#endif
  void printError(const char *fmt, ...);
  void updateDisplay();
  void pollEvents();
  void consoleInput();
  void saveScreenshot();
};

WorldSpaceViewer::WorldSpaceViewer(
    int w, int h, const char *esmFiles, const char *archivePath,
    int argc, const char * const *argv)
  : renderer((Renderer *) 0),
    threadCnt(-1),
    textureCacheSize(1024),
    distantObjectsOnly(false),
    noDisabledObjects(true),
    enableTextures(true),
    debugMode(0),
    formID(0U),
    worldID(0U),
    btdLOD(0),
    terrainX0(-32768),
    terrainY0(-32768),
    terrainX1(32767),
    terrainY1(32767),
    defTxtID(0U),
    ltxtDefColor(0x003F3F3FU),
    ltxtMaxResolution(2048),
    textureMip(2),
    landTextureMult(1.0f),
    modelLOD(0),
    waterColor(0xFFFFFFFFU),
    waterReflectionLevel(1.0f),
    frameTimeMin(500),
    renderQuality(0),
    viewRotation(4),
    viewScale(0.0625f),
    camPositionX(0.0f),
    camPositionY(0.0f),
    camPositionZ(65536.0f),
    zMax(16777216.0f),
    rgbScale(1.0f),
    lightRotationY(70.5288f),
    lightRotationZ(135.0f),
    lightColor(0x00FFFFFF),
    ambientColor(-1),
    envColor(0x00FFFFFF),
    lightLevel(1.0f),
    envLevel(1.0f),
    reflZScale(2.0f),
    waterUVScale(2048),
    objectsMinScale(0.046875f),
    defaultEnvMap("textures/shared/cubemaps/mipblur_defaultoutside1.dds"),
    waterTexture("textures/water/defaultwater.dds"),
    ba2File(archivePath),
    esmFile(esmFiles),
    display(w, h, "World space viewer", 4U,
            (h < 540 ? 24 : (h < 720 ? 36 : 48))),
    renderPass(0),
    renderPassCompleted(false),
    quitFlag(false),
    redrawScreenFlag(true),
    redrawWorldFlag(true)
{
  display.setDefaultTextColor(0x00, 0xC1);
  width = display.getWidth();
  height = display.getHeight();
  imageDataSize = size_t(width) * size_t(height);
  imageBuf.resize(imageDataSize, 0U);
  lastRedrawTime = SDL_GetTicks64();
  try
  {
    setRenderParams(argc, argv);
  }
  catch (std::exception& e)
  {
    printError("%s", e.what());
  }
}

WorldSpaceViewer::~WorldSpaceViewer()
{
  if (renderer)
    delete renderer;
}

void WorldSpaceViewer::calculateViewOffset(float& x, float& y, float& z) const
{
  x = -camPositionX;
  y = -camPositionY;
  z = -camPositionZ;
  float   rX = viewRotations[viewRotation * 3] * d;
  float   rY = viewRotations[viewRotation * 3 + 1] * d;
  float   rZ = viewRotations[viewRotation * 3 + 2] * d;
  NIFFile::NIFVertexTransform vt(viewScale, rX, rY, rZ, 0.0f, 0.0f, 0.0f);
  vt.transformXYZ(x, y, z);
  x = float(roundFloat(x)) + 0.1f;
  y = float(roundFloat(y)) + 0.1f;
}

void WorldSpaceViewer::setRenderParams(int argc, const char * const *argv)
{
  for (int i = 0; i < argc; i++)
  {
    const char  *s = argv[i];
    size_t  n0 = 0;
    size_t  n2 = sizeof(cmdOptsTable) / sizeof(char *);
    while (n2 > (n0 + 1))
    {
      size_t  n1 = (n0 + n2) >> 1;
      int     tmp = std::strcmp(s, cmdOptsTable[n1] + 1);
      n0 = (tmp < 0 ? n0 : n1);
      n2 = (tmp < 0 ? n1 : n2);
    }
    if (std::strcmp(s, cmdOptsTable[n0] + 1) != 0)
      throw FO76UtilsError("invalid option: %s", argv[i]);
    int     n = int(cmdOptsTable[n0][0] - '0');
    if ((i + n) >= argc)
      throw FO76UtilsError("missing argument for %s", argv[i]);
    switch (n0)
    {
      case 0:                   // "btd"
        if (esmFile.getESMVersion() >= 0xC0U)
        {
          btdPath = argv[i + 1];
          if (renderer)
          {
            renderer->clear();
            redrawWorldFlag = true;
          }
        }
        break;
      case 1:                   // "cam"
        viewScale = float(parseFloat(argv[i + 1], "invalid view scale",
                                     1.0f / 512.0f, 16.0f));
        viewRotation =
            (unsigned char) parseInteger(argv[i + 2], 10,
                                         "invalid view direction", 0, 19);
        camPositionX = float(parseFloat(argv[i + 3], "invalid camera position",
                                        -1000000.0, 1000000.0));
        camPositionY = float(parseFloat(argv[i + 4], "invalid camera position",
                                        -1000000.0, 1000000.0));
        camPositionZ = float(parseFloat(argv[i + 5], "invalid camera position",
                                        -1000000.0, 1000000.0));
        redrawWorldFlag = true;
        break;
      case 2:                   // "debug"
        debugMode = (unsigned char) parseInteger(argv[i + 1], 10,
                                                 "invalid debug mode", 0, 5);
        redrawWorldFlag = true;
        break;
      case 3:                   // "defclr"
        ltxtDefColor =
            std::uint32_t(parseInteger(argv[i + 1], 0,
                                       "invalid land texture color",
                                       0, 0x00FFFFFF));
        if (renderer)
        {
          renderer->setLandDefaultColor(ltxtDefColor);
          redrawWorldFlag = true;
        }
        break;
      case 4:                   // "deftxt"
        defTxtID =
            (unsigned int) parseInteger(argv[i + 1], 0,
                                        "invalid form ID", 0, 0x0FFFFFFF);
        if (renderer)
        {
          renderer->clear();
          redrawWorldFlag = true;
        }
        break;
      case 5:                   // "env"
        defaultEnvMap = argv[i + 1];
        if (renderer)
        {
          renderer->setDefaultEnvMap(defaultEnvMap);
          redrawWorldFlag = true;
        }
        break;
      case 6:                   // "ft"
        frameTimeMin =
            std::uint16_t(parseInteger(argv[i + 1], 10,
                                       "invalid frame time", 10, 10000));
        break;
      case 7:                   // "h"
      case 8:                   // "help"
        for (size_t j = 0; usageStrings[j]; j++)
          display.consolePrint("%s\n", usageStrings[j]);
        if (!renderer)
        {
          display.viewTextBuffer();
          quitFlag = true;
          return;
        }
        break;
      case 9:                   // "l"
        btdLOD = int(parseInteger(argv[i + 1], 10,
                                  "invalid terrain level of detail", 0, 4));
        if (esmFile.getESMVersion() < 0xC0U)
        {
          btdLOD = 2;
        }
        else if (renderer)
        {
          renderer->clear();
          redrawWorldFlag = true;
        }
        break;
      case 10:                  // "lcolor"
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
        redrawWorldFlag = true;
        break;
      case 11:                  // "light"
        rgbScale = float(parseFloat(argv[i + 1], "invalid RGB scale",
                                    0.125, 4.0));
        lightRotationY = float(parseFloat(argv[i + 2],
                                          "invalid light Y rotation",
                                          -360.0, 360.0));
        lightRotationZ = float(parseFloat(argv[i + 3],
                                          "invalid light Z rotation",
                                          -360.0, 360.0));
        redrawWorldFlag = true;
        break;
      case 12:                  // "list"
      case 13:                  // "list-defaults"
        display.consolePrint("threads: %d", threadCnt);
        if (threadCnt <= 0)
        {
          display.consolePrint(" (defaults to hardware threads: %d)",
                               int(std::thread::hardware_concurrency()));
        }
        display.consolePrint("\ndebug: %d\n", int(debugMode));
        display.consolePrint("textures: %d\n", int(enableTextures));
        display.consolePrint("txtcache: %d\n", textureCacheSize);
        display.consolePrint("rq: %d\n", int(renderQuality));
        display.consolePrint("ft: %d\n", int(frameTimeMin));
        if (!markerDefsFileName.empty())
          display.consolePrint("markers: %s\n", markerDefsFileName.c_str());
        display.consolePrint("w: 0x%08X", formID);
        if (!formID)
          display.consolePrint(" (defaults to 0x0000003C or 0x0025DA15)");
        display.consolePrint("\nr: %d %d %d %d\n",
                             int(terrainX0), int(terrainY0),
                             int(terrainX1), int(terrainY1));
        display.consolePrint("l: %d\n", btdLOD);
        display.consolePrint("deftxt: 0x%08X\n", defTxtID);
        display.consolePrint("defclr: 0x%08X\n", (unsigned int) ltxtDefColor);
        display.consolePrint("ltxtres: %d\n", ltxtMaxResolution);
        display.consolePrint("mip: %d\n", textureMip);
        display.consolePrint("lmult: %.1f\n", landTextureMult);
        display.consolePrint("cam: %.7g %d %.7g %.7g %.7g\n",
                             viewScale, int(viewRotation),
                             camPositionX, camPositionY, camPositionZ);
        display.consolePrint("zrange: %.7g\n", zMax);
        display.consolePrint("light: %.1f %.4f %.4f\n",
                             rgbScale, lightRotationY, lightRotationZ);
        {
          NIFFile::NIFVertexTransform
              vt(1.0f, 0.0f, lightRotationY * d, lightRotationZ * d,
                 0.0f, 0.0f, 0.0f);
          display.consolePrint("    Light vector: %9.6f %9.6f %9.6f\n",
                               vt.rotateZX, vt.rotateZY, vt.rotateZZ);
        }
        display.consolePrint("lcolor: %.3f 0x%06X %.3f 0x%06X ",
                             lightLevel, (unsigned int) lightColor,
                             envLevel, (unsigned int) envColor);
        if (ambientColor < 0)
        {
          display.consolePrint("%d (calculated from default cube map)\n",
                               ambientColor);
        }
        else
        {
          display.consolePrint("0x%06X\n", (unsigned int) ambientColor);
        }
        display.consolePrint("rscale: %.3f\n", reflZScale);
        display.consolePrint("mlod: %d\n", modelLOD);
        display.consolePrint("vis: %d\n", int(distantObjectsOnly));
        display.consolePrint("ndis: %d\n", int(noDisabledObjects));
        display.consolePrint("minscale: %.7g\n", objectsMinScale);
        display.consolePrint("env: %s\n", defaultEnvMap.c_str());
        display.consolePrint("wtxt: %s\n", waterTexture.c_str());
        display.consolePrint("watercolor: 0x%08X\n",
                             (unsigned int) (((waterColor >> 1) & 0x7F000000U)
                                             | (waterColor & 0x00FFFFFFU)));
        display.consolePrint("wrefl: %.3f\n", waterReflectionLevel);
        display.consolePrint("wscale: %d\n", waterUVScale);
        if (!renderer)
        {
          display.viewTextBuffer();
          quitFlag = true;
          return;
        }
        break;
      case 14:                  // "lmult"
        landTextureMult = float(parseFloat(argv[i + 1],
                                           "invalid land texture RGB scale",
                                           0.5, 8.0));
        if (renderer)
        {
          renderer->setLandTxtRGBScale(landTextureMult);
          redrawWorldFlag = true;
        }
        break;
      case 15:                  // "ltxtres"
        ltxtMaxResolution =
            int(parseInteger(argv[i + 1], 0,
                             "invalid land texture resolution", 32, 4096));
        if (ltxtMaxResolution & (ltxtMaxResolution - 1))
          errorMessage("invalid land texture resolution");
        redrawWorldFlag = true;
        break;
      case 16:                  // "markers"
        markerDefsFileName = argv[i + 1];
        redrawWorldFlag = true;
        break;
      case 17:                  // "minscale"
        objectsMinScale = float(parseFloat(argv[i + 1], "invalid view scale",
                                           1.0f / 512.0f, 16.0f));
        redrawWorldFlag = true;
        break;
      case 18:                  // "mip"
        textureMip = int(parseInteger(argv[i + 1], 10,
                                      "invalid texture mip level", 0, 15));
        if (renderer)
        {
          renderer->clearTextureCache();
          renderer->setTextureMipLevel(textureMip);
          redrawWorldFlag = true;
        }
        break;
      case 19:                  // "mlod"
        modelLOD = int(parseInteger(argv[i + 1], 10,
                                    "invalid model LOD", 0, 4));
        if (renderer)
        {
          renderer->setModelLOD(modelLOD);
          redrawWorldFlag = true;
        }
        break;
      case 20:                  // "ndis"
        noDisabledObjects =
            bool(parseInteger(argv[i + 1], 0,
                              "invalid argument for -ndis", 0, 1));
        if (renderer)
        {
          renderer->setNoDisabledObjects(noDisabledObjects);
          redrawWorldFlag = true;
        }
        break;
      case 21:                  // "r"
        terrainX0 =
            std::int16_t(parseInteger(argv[i + 1], 10,
                                      "invalid terrain X0", -32768, 32767));
        terrainY0 =
            std::int16_t(parseInteger(argv[i + 2], 10,
                                      "invalid terrain Y0", -32768, 32767));
        terrainX1 =
            std::int16_t(parseInteger(argv[i + 3], 10,
                                      "invalid terrain X1", terrainX0, 32767));
        terrainY1 =
            std::int16_t(parseInteger(argv[i + 4], 10,
                                      "invalid terrain Y1", terrainY0, 32767));
        if (renderer)
        {
          renderer->clear();
          redrawWorldFlag = true;
        }
        break;
      case 22:                  // "rq"
        renderQuality =
            (unsigned char) parseInteger(argv[i + 1], 0,
                                         "invalid render quality", 0, 15);
        redrawWorldFlag = true;
        break;
      case 23:                  // "rscale"
        reflZScale =
            float(parseFloat(argv[i + 1],
                             "invalid reflection view vector Z scale",
                             0.25, 16.0));
        redrawWorldFlag = true;
        break;
      case 24:                  // "textures"
        enableTextures =
            bool(parseInteger(argv[i + 1], 0,
                              "invalid argument for -textures", 0, 1));
        if (renderer)
        {
          renderer->setEnableTextures(enableTextures);
          redrawWorldFlag = true;
        }
        break;
      case 25:                  // "threads"
        threadCnt = int(parseInteger(argv[i + 1], 10,
                                     "invalid number of threads", 1, 16));
        if (renderer)
        {
          renderer->setThreadCount(threadCnt);
          redrawWorldFlag = true;
        }
        break;
      case 26:                  // "txtcache"
        textureCacheSize =
            int(parseInteger(argv[i + 1], 0,
                             "invalid texture cache size", 256, 4095));
        if (renderer)
          renderer->setTextureCacheSize(size_t(textureCacheSize) << 20);
        break;
      case 27:                  // "vis"
        distantObjectsOnly =
            bool(parseInteger(argv[i + 1], 0,
                              "invalid argument for -vis", 0, 1));
        if (renderer)
        {
          renderer->setDistantObjectsOnly(distantObjectsOnly);
          redrawWorldFlag = true;
        }
        break;
      case 28:                  // "w"
        formID = (unsigned int) parseInteger(argv[i + 1], 0,
                                             "invalid form ID", 0, 0x0FFFFFFF);
        if (renderer)
        {
          delete renderer;
          renderer = (Renderer *) 0;
          redrawWorldFlag = true;
        }
        break;
      case 29:                  // "watercolor"
        {
          std::uint32_t tmp =
              std::uint32_t(parseInteger(argv[i + 1], 0, "invalid water color",
                                         0, 0x7FFFFFFF));
          tmp = tmp + (tmp & 0x7F000000U) + ((tmp >> 30) << 24);
          if (!(tmp & 0xFF000000U))
            tmp = 0U;
          if (renderer)
          {
            renderer->setWaterColor(tmp);
            if ((tmp == 0U) != (waterColor == 0U))
              renderer->clearObjectPropertyCache();
            redrawWorldFlag = true;
          }
          waterColor = tmp;
        }
        break;
      case 30:                  // "wrefl"
        waterReflectionLevel =
            float(parseFloat(argv[i + 1],
                             "invalid water environment map scale", 0.0, 4.0));
        if (renderer)
        {
          renderer->setWaterEnvMapScale(waterReflectionLevel);
          redrawWorldFlag = true;
        }
        break;
      case 31:                  // "wscale"
        waterUVScale =
            int(parseInteger(argv[i + 1], 0,
                             "invalid water texture tile size", 2, 32768));
        redrawWorldFlag = true;
        break;
      case 32:                  // "wtxt"
        waterTexture = argv[i + 1];
        if (renderer)
        {
          renderer->setWaterTexture(waterTexture);
          redrawWorldFlag = true;
        }
        break;
      case 33:                  // "xm"
        if (argv[i + 1][0])
        {
          excludeModelPatterns.push_back(std::string(argv[i + 1]));
          if (renderer)
          {
            renderer->addExcludeModelPattern(excludeModelPatterns.back());
            renderer->clearObjectPropertyCache();
            redrawWorldFlag = true;
          }
        }
        break;
      case 34:                  // "xm_clear"
        if (excludeModelPatterns.size() > 0)
        {
          excludeModelPatterns.clear();
          if (renderer)
          {
            renderer->clearExcludeModelPatterns();
            renderer->clearObjectPropertyCache();
            redrawWorldFlag = true;
          }
        }
        break;
      case 35:                  // "zrange"
        zMax = float(parseFloat(argv[i + 1],
                                "invalid Z range", 1.0, 16777216.0));
        redrawWorldFlag = true;
        break;
    }
    i = i + n;
  }
}

#ifdef __GNUC__
__attribute__ ((__format__ (__printf__, 2, 3)))
#endif
void WorldSpaceViewer::printError(const char *fmt, ...)
{
  std::va_list  ap;
  va_start(ap, fmt);
  std::vsnprintf(tmpBuf, sizeof(tmpBuf), fmt, ap);
  va_end(ap);
  tmpBuf[sizeof(tmpBuf) - 1] = '\0';
  display.unlockScreenSurface();
  display.clearSurface();
  display.clearTextBuffer();
  display.consolePrint("\033[41m\033[33m\033[1m    Error: %s    \n", tmpBuf);
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
}

void WorldSpaceViewer::updateDisplay()
{
  if (BRANCH_UNLIKELY(!renderer))
  {
    if (btdPath.empty())
      btdLOD = 2;
    if (!formID)
      formID = (btdPath.empty() ? 0x0000003CU : 0x0025DA15U);
    worldID = Renderer::findParentWorld(esmFile, formID);
    if (worldID == 0xFFFFFFFFU)
    {
      worldID = 0U;
      errorMessage("form ID not found in ESM, or invalid record type");
    }
    renderer = new Renderer(width, height, ba2File, esmFile, imageBuf.data());
    if (threadCnt > 0)
      renderer->setThreadCount(threadCnt);
    renderer->setTextureCacheSize(size_t(textureCacheSize) << 20);
    renderer->setDistantObjectsOnly(distantObjectsOnly);
    renderer->setNoDisabledObjects(noDisabledObjects);
    renderer->setEnableTextures(enableTextures);
    renderer->setLandDefaultColor(ltxtDefColor);
    renderer->setTextureMipLevel(textureMip);
    renderer->setLandTxtRGBScale(landTextureMult);
    renderer->setModelLOD(modelLOD);
    renderer->setWaterColor(waterColor);
    renderer->setWaterEnvMapScale(waterReflectionLevel);
    if (!defaultEnvMap.empty())
      renderer->setDefaultEnvMap(defaultEnvMap);
    if (waterColor && !waterTexture.empty())
      renderer->setWaterTexture(waterTexture);
    for (size_t i = 0; i < excludeModelPatterns.size(); i++)
      renderer->addExcludeModelPattern(excludeModelPatterns[i]);
    redrawWorldFlag = true;
  }

  if (BRANCH_UNLIKELY(redrawWorldFlag))
  {
    redrawWorldFlag = false;
    renderPass = 0;
    renderPassCompleted = false;
    std::memset(imageBuf.data(), 0, imageDataSize * sizeof(std::uint32_t));
    float   *p = renderer->getZBufferData();
    float   z = zMax;
    for (size_t i = imageDataSize; i-- > 0; p++)
      *p = z;
    display.clearTextBuffer();
    renderer->setEnableSCOL(false);
    renderer->setEnableAllObjects(false);
    renderer->setRenderQuality(renderQuality
                               | (unsigned char) (debugMode == 1));
    renderer->setDebugMode(debugMode);
    float   viewOffsX, viewOffsY, viewOffsZ;
    calculateViewOffset(viewOffsX, viewOffsY, viewOffsZ);
    renderer->setViewTransform(
        viewScale, viewRotations[viewRotation * 3] * d,
        viewRotations[viewRotation * 3 + 1] * d,
        viewRotations[viewRotation * 3 + 2] * d,
        viewOffsX + (float(width) * 0.5f),
        viewOffsY + (float(height - 2) * 0.5f), viewOffsZ);
    renderer->setLightDirection(lightRotationY * d, lightRotationZ * d);
    renderer->setRenderParameters(
        lightColor, ambientColor, envColor, lightLevel, envLevel,
        rgbScale, reflZScale, waterUVScale);
    if (worldID)
    {
      display.consolePrint("Loading terrain data and landscape textures\n");
      display.clearSurface();
      display.drawText(0, -1, display.getTextRows(), 0.75f, 1.0f);
      display.blitSurface();
      int     ltxtResolution = int(debugMode != 1 && debugMode != 2);
      ltxtResolution =
          ltxtResolution << roundFloat(float(std::log2(viewScale)) + 11.9f);
      ltxtResolution = std::min(ltxtResolution, ltxtMaxResolution);
      ltxtResolution = std::max(ltxtResolution, 128 >> btdLOD);
      renderer->setLandTxtResolution(ltxtResolution);
      int     ltxtMip = (esmFile.getESMVersion() < 0x80U ? 13 : 14)
                        - (textureMip + FloatVector4::log2Int(ltxtResolution));
      ltxtMip = std::max(std::min(ltxtMip, 15 - textureMip), 0);
      renderer->setLandTextureMip(float(ltxtMip));
      renderer->loadTerrain(btdPath.c_str(), worldID, defTxtID, btdLOD,
                            terrainX0, terrainY0, terrainX1, terrainY1);
      display.consolePrint("Rendering terrain\n");
      renderer->initRenderPass(0, worldID);
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
    if (worldID && viewScale < objectsMinScale)
      renderPass = 2;
    if (++renderPass < 3)
    {
      if (renderPass == 1)
        display.consolePrint("Rendering objects\n");
      else
        display.consolePrint("Rendering water and transparent objects\n");
      renderer->initRenderPass(renderPass, formID);
    }
    else if (BRANCH_UNLIKELY(!markerDefsFileName.empty()))
    {
      display.consolePrint("Finding markers\n");
      float   viewOffsX, viewOffsY, viewOffsZ;
      calculateViewOffset(viewOffsX, viewOffsY, viewOffsZ);
      NIFFile::NIFVertexTransform vt(
          viewScale, viewRotations[viewRotation * 3] * d,
          viewRotations[viewRotation * 3 + 1] * d,
          viewRotations[viewRotation * 3 + 2] * d,
          viewOffsX + (float(width) * 0.5f),
          viewOffsY + (float(height - 2) * 0.5f), viewOffsZ);
      try
      {
        MapImage  mapImage(esmFile, markerDefsFileName.c_str(),
                           imageBuf.data(), width, height, vt);
        mapImage.findMarkers(formID);
      }
      catch (std::exception& e)
      {
        markerDefsFileName.clear();
        display.consolePrint("\033[41m\033[33m\033[1mError: %s\033[m\n",
                             e.what());
      }
    }
    redrawScreenFlag = true;
  }
  else if (renderPass < 3)
  {
    renderPassCompleted = renderer->renderObjects();
    if (!renderPassCompleted)
    {
      display.consolePrint("\r    %7u / %7u  ",
                           (unsigned int) renderer->getObjectsRendered(),
                           (unsigned int) renderer->getObjectCount());
    }
    else
    {
      display.consolePrint("\r    %7u / %7u  \n",
                           (unsigned int) renderer->getObjectCount(),
                           (unsigned int) renderer->getObjectCount());
    }
    if (SDL_GetTicks64() >= (lastRedrawTime + frameTimeMin))
      redrawScreenFlag = true;
  }
  if (redrawScreenFlag)
  {
    redrawScreenFlag = false;
    lastRedrawTime = SDL_GetTicks64();
    std::memcpy(display.lockDrawSurface(), imageBuf.data(),
                imageDataSize * sizeof(std::uint32_t));
    display.unlockDrawSurface();
    display.drawText(0, -1, display.getTextRows(), 0.75f, 1.0f);
    display.blitSurface();
  }
}

void WorldSpaceViewer::pollEvents()
{
  display.pollEvents(eventBuf, (renderPass < 3 ? 0 : -1000), false, true);
  float   xStep = 0.0f;
  float   yStep = 0.0f;
  float   zStep = 0.0f;
  for (size_t i = 0; i < eventBuf.size() && BRANCH_LIKELY(!quitFlag); i++)
  {
    if (eventBuf[i].type() == SDLDisplay::SDLEventWindow)
    {
      if (eventBuf[i].data1() == 0)
        quitFlag = true;
      else if (eventBuf[i].data1() == 1)
        redrawScreenFlag = true;
    }
    else if (eventBuf[i].type() == SDLDisplay::SDLEventMButtonDown)
    {
      int     x = eventBuf[i].data1();
      int     y = eventBuf[i].data2();
      x = (x > 0 ? (x < (width - 1) ? x : (width - 1)) : 0);
      y = (y > 0 ? (y < (height - 1) ? y : (height - 1)) : 0);
      if ((eventBuf[i].data3() == 1 || eventBuf[i].data3() == 3) &&
          eventBuf[i].data4() == 2)
      {
        int     x0 = (width + 1) >> 1;
        int     y0 = (height - 1) >> 1;
        xStep += float(x - x0);
        yStep += float(y - y0);
        if (eventBuf[i].data3() == 3)
        {
          const float *zBuf = renderer->getZBufferData();
          float   z = zBuf[size_t(y) * size_t(width) + size_t(x)];
          if (z < 1000000.0f)
            zStep += (z - 1.0f);
        }
      }
      else if (debugMode == 1 &&
               eventBuf[i].data3() == 1 && eventBuf[i].data4() == 1)
      {
        std::uint32_t c = imageBuf[size_t(y) * size_t(width) + size_t(x)];
        c = ((c & 0xFFU) << 16) | ((c >> 16) & 0xFFU) | (c & 0xFF00U);
        std::sprintf(tmpBuf, "0x%08X", (unsigned int) c);
        display.consolePrint("Reference form ID = %s\n", tmpBuf);
        (void) SDL_SetClipboardText(tmpBuf);
        redrawScreenFlag = true;
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
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
        {
          unsigned char tmp = (unsigned char) (eventBuf[i].data1() - '0');
          if (tmp != debugMode)
          {
            debugMode = tmp;
            redrawWorldFlag = true;
          }
        }
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
      case 'p':
        {
          float   viewOffsX, viewOffsY, viewOffsZ;
          calculateViewOffset(viewOffsX, viewOffsY, viewOffsZ);
          std::sprintf(tmpBuf, "-light %.7g %.7g %.7g "
                               "-view %.7g %.7g %.7g %.7g %.7g %.7g %.7g "
                               "(cam: %.7g %.7g %.7g)",
                       rgbScale, lightRotationY, lightRotationZ, viewScale,
                       viewRotations[viewRotation * 3],
                       viewRotations[viewRotation * 3 + 1],
                       viewRotations[viewRotation * 3 + 2],
                       viewOffsX, viewOffsY, viewOffsZ,
                       camPositionX, camPositionY, camPositionZ);
          display.consolePrint("%s\n", tmpBuf);
          (void) SDL_SetClipboardText(tmpBuf);
          redrawScreenFlag = true;
        }
        break;
      case 'v':
        {
          const char  *tmp = "list";
          setRenderParams(1, &tmp);
          redrawScreenFlag = true;
        }
        break;
      case 'c':
        display.clearTextBuffer();
        redrawScreenFlag = true;
        break;
      case '`':
        consoleInput();
        display.clearTextBuffer();
        redrawScreenFlag = true;
        break;
      case SDLDisplay::SDLKeySymF12:
      case SDLDisplay::SDLKeySymPrintScr:
        saveScreenshot();
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

void WorldSpaceViewer::consoleInput()
{
  while (true)
  {
    if (!display.consoleInput(cmdBuf, cmdHistory1, cmdHistory2))
    {
      quitFlag = true;
      break;
    }
    if (cmdBuf.length() > (sizeof(tmpBuf) - 1))
      cmdBuf.resize(sizeof(tmpBuf) - 1);
    for (size_t i = 0; i < cmdBuf.length(); i++)
    {
      if (cmdBuf[i] >= 'A' && cmdBuf[i] <= 'Z')
        cmdBuf[i] = cmdBuf[i] + ('a' - 'A');
    }
    if (cmdBuf == "q" || cmdBuf == "`")
      break;
    try
    {
      std::vector< const char * > args;
      for (size_t i = 0; i < cmdBuf.length(); )
      {
        if ((unsigned char) cmdBuf[i] <= 0x20)
        {
          i++;
          continue;
        }
        bool    quoteFlag = false;
        if (cmdBuf[i] == '"')
        {
          quoteFlag = true;
          i++;
        }
        args.push_back(&(tmpBuf[i]));
        for ( ; i < cmdBuf.length(); i++)
        {
          if (!quoteFlag && (unsigned char) cmdBuf[i] <= 0x20)
            break;
          if (quoteFlag && cmdBuf[i] == '"')
          {
            tmpBuf[i++] = '\0';
            break;
          }
          tmpBuf[i] = cmdBuf[i];
        }
        tmpBuf[i] = '\0';
      }
      if (args.size() == 1 && args[0][0] >= '0' && args[0][0] <= '9')
      {
        unsigned int  n =
            (unsigned int) parseInteger(args[0], 0,
                                        "invalid form ID", 0, 0x0FFFFFFF);
        const ESMFile::ESMRecord  *r = esmFile.getRecordPtr(n);
        if (!r)
          errorMessage("invalid form ID");
        if (!(*r == "REFR" || *r == "ACHR"))
          errorMessage("form ID is not a reference");
        ESMFile::ESMField f(esmFile, *r);
        while (f.next())
        {
          if (f == "DATA" && f.size() >= 16)
          {
            FloatVector4  tmp(f.readFloatVector4());
            tmp.minValues(FloatVector4(1000000.0f));
            tmp.maxValues(FloatVector4(-1000000.0f));
            camPositionX = tmp[0];
            camPositionY = tmp[1];
            camPositionZ = tmp[2];
            redrawWorldFlag = true;
          }
        }
      }
      else
      {
        setRenderParams(int(args.size()), args.data());
      }
    }
    catch (std::exception& e)
    {
      display.consolePrint("\033[41m\033[33m\033[1mError: %s\033[m\n",
                           e.what());
    }
  }
}

void WorldSpaceViewer::saveScreenshot()
{
  bool    screenSurfaceLocked = false;
  try
  {
    std::string fileName("wrldview");
    std::time_t t = std::time((std::time_t *) 0);
    {
      unsigned int  s = (unsigned int) (t % std::time_t(24 * 60 * 60));
      unsigned int  m = s / 60U;
      s = s % 60U;
      unsigned int  h = m / 60U;
      m = m % 60U;
      h = h % 24U;
      char    buf[16];
      std::sprintf(buf, "_%02u%02u%02u.dds", h, m, s);
      fileName += buf;
    }
    std::memcpy(display.lockDrawSurface(), imageBuf.data(),
                imageDataSize * sizeof(std::uint32_t));
    display.unlockDrawSurface();
    display.blitSurface();
    int     w = display.getWidth() >> int(display.getIsDownsampled());
    int     h = display.getHeight() >> int(display.getIsDownsampled());
    const std::uint32_t *p = display.lockScreenSurface();
    screenSurfaceLocked = true;
    DDSOutputFile f(fileName.c_str(), w, h, DDSInputFile::pixelFormatRGBA32);
    size_t  pitch = display.getPitch();
    for (int y = 0; y < h; y++, p = p + pitch)
      f.writeImageData(p, size_t(w), DDSInputFile::pixelFormatRGBA32);
    display.unlockScreenSurface();
    screenSurfaceLocked = false;
    display.consolePrint("Saved screenshot to %s\n", fileName.c_str());
  }
  catch (std::exception& e)
  {
    if (screenSurfaceLocked)
      display.unlockScreenSurface();
    display.consolePrint("\033[41m\033[33m\033[1mError: %s\033[m\n",
                         e.what());
  }
  redrawScreenFlag = true;
}

int main(int argc, char **argv)
{
  WorldSpaceViewer  *wrldView = (WorldSpaceViewer *) 0;
  try
  {
    std::vector< const char * > args;
    std::vector< const char * > args2;
    for (int i = 1; i < argc; i++)
    {
      if (argv[i][0] != '-')
      {
        args.push_back(argv[i]);
        continue;
      }
      if (std::strcmp(argv[i], "--") == 0)
      {
        while (++i < argc)
          args.push_back(argv[i]);
        break;
      }
      const char  *s = argv[i] + 1;
      if (*s == '-')
        s++;
      size_t  n0 = 0;
      size_t  n2 = sizeof(cmdOptsTable) / sizeof(char *);
      while (n2 > (n0 + 1))
      {
        size_t  n1 = (n0 + n2) >> 1;
        int     tmp = std::strcmp(s, cmdOptsTable[n1] + 1);
        n0 = (tmp < 0 ? n0 : n1);
        n2 = (tmp < 0 ? n1 : n2);
      }
      if (std::strcmp(s, cmdOptsTable[n0] + 1) != 0)
        throw FO76UtilsError("invalid option: %s", argv[i]);
      int     n = int(cmdOptsTable[n0][0] - '0');
      if ((i + n) >= argc)
        throw FO76UtilsError("missing argument for %s", argv[i]);
      args2.push_back(s);
      for ( ; n > 0; i++, n--)
        args2.push_back(argv[i + 1]);
    }
    if (args.size() != 4)
    {
      SDLDisplay::enableConsole();
      for (size_t i = 0; usageStrings[i]; i++)
        std::fprintf(stderr, "%s\n", usageStrings[i]);
      std::fprintf(stderr, "\nwrldview: invalid number of arguments\n");
      return 1;
    }
    int     width =
        int(parseInteger(args[1], 0, "invalid image width", 640, 16384));
    int     height =
        int(parseInteger(args[2], 0, "invalid image height", 360, 16384));
    wrldView = new WorldSpaceViewer(width, height, args[0], args[3],
                                    int(args2.size()), args2.data());
    while (!wrldView->quitFlag)
    {
      wrldView->updateDisplay();
      wrldView->pollEvents();
    }
  }
  catch (std::exception& e)
  {
    if (!wrldView)
    {
      SDLDisplay::enableConsole();
      std::fprintf(stderr, "wrldview: %s\n", e.what());
    }
    else
    {
      wrldView->printError("%s", e.what());
      delete wrldView;
    }
    return 1;
  }
  if (wrldView)
    delete wrldView;
  return 0;
}

