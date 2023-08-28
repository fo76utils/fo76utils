
#include "common.hpp"
#include "fp32vec4.hpp"
#include "render.hpp"
#include "sdlvideo.hpp"
#include "markers.hpp"

#include <bit>
#include <ctime>
#include <SDL2/SDL.h>

#include "viewrtbl.cpp"

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
  "1mc",                // 17
  "1minscale",          // 18
  "1mip",               // 19
  "1mlod",              // 20
  "1ndis",              // 21
  "4r",                 // 22
  "1rq",                // 23
  "1rscale",            // 24
  "1tc",                // 25
  "1textures",          // 26
  "1threads",           // 27
  "1txtcache",          // 28
  "1vis",               // 29
  "1w",                 // 30
  "1watercolor",        // 31
  "1wrefl",             // 32
  "1wscale",            // 33
  "1wtxt",              // 34
  "1xm",                // 35
  "0xm_clear",          // 36
  "1zrange"             // 37
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
  "    -tc | -txtcache INT texture cache size in megabytes",
  "    -mc INT             number of models to load at once (1 to 64)",
  "    -rq INT             set render quality (0 - 2047, see doc/render.md)",
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
  "    -cam SCALE -1 RX RY RZ X Y Z",
  "                        set view scale from world to image coordinates,",
  "                        view direction (0 to 19, see src/viewrtbl.cpp),",
  "                        and camera position",
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

static const char *keyboardUsageString =
    "  \033[4m\033[38;5;228m0\033[m "
    "to \033[4m\033[38;5;228m5\033[m                "
    "Set debug render mode:                                          \n"
    "                        "
    "    0: disabled (default)                                       \n"
    "                        "
    "    1: reference and cell form IDs as RGB colors                \n"
    "                        "
    "    2: depth                                                    \n"
    "                        "
    "    3: normals                                                  \n"
    "                        "
    "    4: diffuse texture only with no lighting                    \n"
    "                        "
    "    5: light only (white textures)                              \n"
    "  \033[4m\033[38;5;228m+\033[m, "
    "\033[4m\033[38;5;228m-\033[m                  "
    "Zoom in or out by a factor of 1.4142.                           \n"
    "  \033[4m\033[38;5;228mKeypad 0, 5\033[m           "
    "Set view from the bottom or top (default = top).                \n"
    "  \033[4m\033[38;5;228mKeypad 1 to 9\033[m         "
    "Set isometric view from the SW to NE.                           \n"
    "  \033[4m\033[38;5;228mShift + Keypad 0 to 9\033[m "
    "Set side view, or top/bottom view rotated by 45 degrees.        \n"
    "  \033[4m\033[38;5;228mA\033[m, "
    "\033[4m\033[38;5;228mD\033[m                  "
    "Move to the left or right.                                      \n"
    "  \033[4m\033[38;5;228mS\033[m, "
    "\033[4m\033[38;5;228mW\033[m                  "
    "Move backward or forward (-Z or +Z in the view space).          \n"
    "  \033[4m\033[38;5;228mE\033[m, "
    "\033[4m\033[38;5;228mX\033[m                  "
    "Move up or down (-Y or +Y in the view space).                   \n"
    "  \033[4m\033[38;5;228mMouse L button double\033[m "
    "Move camera position so that the selected pixel is at the center\n"
    "                        "
    "of the screen.                                                  \n"
    "  \033[4m\033[38;5;228mMouse R button double\033[m "
    "Similar to above, but also adjust the Z coordinate in view space\n"
    "                        "
    "so that the selected pixel is just in front of the new camera   \n"
    "                        "
    "position.                                                       \n"
    "  \033[4m\033[38;5;228mMouse L button single\033[m "
    "In debug mode 1 only: print the form ID of the selected object  \n"
    "                        "
    "based on the color of the pixel, and also copy it to the        \n"
    "                        "
    "clipboard.                                                      \n"
    "  \033[4m\033[38;5;228mMouse R button single\033[m "
    "Print the X, Y, Z coordinates in the world space.               \n"
    "  \033[4m\033[38;5;228mF11\033[m                   "
    "Save raw screenshot without downsampling or format conversion.  \n"
    "  \033[4m\033[38;5;228mF12\033[m "
    "or \033[4m\033[38;5;228mPrint Screen\033[m   "
    "Save screenshot.                                                \n"
    "  \033[4m\033[38;5;228mP\033[m                     "
    "Print current -light and -view parameters (for use with render  \n"
    "                        "
    "or markers), and camera position. The information printed is    \n"
    "                        "
    "also copied to the clipboard.                                   \n"
    "  \033[4m\033[38;5;228mV\033[m                     "
    "Print all current settings, similarly to the 'list' command.    \n"
    "  \033[4m\033[38;5;228mR\033[m                     "
    "Print the list of view directions that can be used with 'cam'.  \n"
    "  \033[4m\033[38;5;228mH\033[m                     "
    "Show help screen.                                               \n"
    "  \033[4m\033[38;5;228mC\033[m                     "
    "Clear messages.                                                 \n"
    "  \033[4m\033[38;5;228m`\033[m                     "
    "Open the console. In this mode, keyboard and mouse controls work\n"
    "                        "
    "similarly to esmview, and any of the command line options can be\n"
    "                        "
    "entered as a command (without the leading - character). Entering\n"
    "                        "
    "the hexadecimal form ID of a reference moves the camera position\n"
    "                        "
    "to its coordinates. The command q or ` returns to view mode.    \n"
    "  \033[4m\033[38;5;228mEsc\033[m                   "
    "Quit program.                                                   \n";

struct WorldSpaceViewer
{
  static constexpr float  d = 0.01745329252f;   // degrees to radians
  std::vector< std::uint32_t >  imageBuf;
  std::vector< SDLDisplay::SDLEvent > eventBuf;
  Renderer  *renderer;
  int     width;
  int     height;
  unsigned char threadCnt;
  unsigned char modelBatchCnt;
  unsigned int  textureCacheSize;
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
  std::uint16_t renderQuality;
  bool    quitFlag;
  signed char viewRotation;             // -1: using custom rotations
  float   viewScale;
  float   viewRotationX;
  float   viewRotationY;
  float   viewRotationZ;
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
  // 0: rendering terrain, 1: objects, 2: water and effects, 3: done
  // OR -256: begin the next pass
  int     renderPass;
  bool    redrawScreenFlag;
  bool    redrawWorldFlag;
  std::uint16_t frameTimeMin;
  std::map< size_t, std::string > cmdHistory1;
  std::map< std::string, size_t > cmdHistory2;
  std::string cmdBuf;
  const char  *dataPath;
  std::vector< char > tmpBuf;
  WorldSpaceViewer(int w, int h, const char *esmFiles, const char *archivePath,
                   int argc, const char * const *argv);
  virtual ~WorldSpaceViewer();
  void setViewTransform();
  bool screenToWorldSpace(FloatVector4& v, int x, int y) const;
  void setRenderParams(int argc, const char * const *argv);
#ifdef __GNUC__
  __attribute__ ((__format__ (__printf__, 2, 3)))
#endif
  void printError(const char *fmt, ...);
  void updateDisplay();
  void printReferenceInfo(unsigned int refrFormID);
  void printWorldSpaceXYZ(int x, int y);
  void pollEvents();
  void consoleInput();
  void saveScreenshot(bool disableDownsampling = false);
};

WorldSpaceViewer::WorldSpaceViewer(
    int w, int h, const char *esmFiles, const char *archivePath,
    int argc, const char * const *argv)
  : renderer((Renderer *) 0),
    threadCnt(0),
    modelBatchCnt(16),
    textureCacheSize(1024U),
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
    renderQuality(0),
    quitFlag(false),
    viewRotation(4),
    viewScale(0.0625f),
    viewRotationX(0.0f),
    viewRotationY(0.0f),
    viewRotationZ(180.0f),
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
    redrawScreenFlag(true),
    redrawWorldFlag(true),
    frameTimeMin(500),
    dataPath(archivePath),
    tmpBuf(2048)
{
  display.setDefaultTextColor(0x00, 0xC1);
  width = display.getWidth();
  height = display.getHeight();
  imageDataSize = size_t(width) * size_t(height);
  imageBuf.resize(imageDataSize, 0U);
  renderer = new Renderer(width, height, ba2File, esmFile, imageBuf.data());
  try
  {
    if (esmFile.getESMVersion() < 0xC0U)
      btdLOD = 2;
    if (!formID)
      formID = (esmFile.getESMVersion() < 0xC0U ? 0x0000003CU : 0x0025DA15U);
    worldID = Renderer::findParentWorld(esmFile, formID);
    if (worldID == 0xFFFFFFFFU)
    {
      worldID = 0U;
      errorMessage("form ID not found in ESM, or invalid record type");
    }
    renderer->setDefaultEnvMap(defaultEnvMap);
    renderer->setWaterTexture(waterTexture);
    for (size_t i = 0; i < excludeModelPatterns.size(); i++)
      renderer->addExcludeModelPattern(excludeModelPatterns[i]);
    setRenderParams(argc, argv);
  }
  catch (std::exception& e)
  {
    printError("%s", e.what());
  }
  catch (...)
  {
    delete renderer;
    throw;
  }
}

WorldSpaceViewer::~WorldSpaceViewer()
{
  if (renderer)
    delete renderer;
}

void WorldSpaceViewer::setViewTransform()
{
  float   x = -camPositionX;
  float   y = -camPositionY;
  float   z = -camPositionZ;
  NIFFile::NIFVertexTransform vt(viewScale, viewRotationX * d,
                                 viewRotationY * d, viewRotationZ * d,
                                 0.0f, 0.0f, 0.0f);
  vt.transformXYZ(x, y, z);
  x = float(roundFloat(x)) + 0.1f;
  y = float(roundFloat(y)) + 0.1f;
  renderer->setViewTransform(
      viewScale, viewRotationX * d, viewRotationY * d, viewRotationZ * d,
      x + (float(width) * 0.5f), y + (float(height - 2) * 0.5f), z);
}

bool WorldSpaceViewer::screenToWorldSpace(FloatVector4& v, int x, int y) const
{
  v = FloatVector4(camPositionX, camPositionY, camPositionZ, 0.0f);
  size_t  offs = size_t(y) * size_t(width) + size_t(x);
  float   z = renderer->getZBufferData()[offs];
  if (!((renderer->getImageData()[offs] & 0x80000000U) && z < zMax))
    return false;
  NIFFile::NIFVertexTransform tmp(renderer->getViewTransform());
  NIFFile::NIFVertexTransform viewTransformInv(tmp);
  viewTransformInv.rotateYX = tmp.rotateXY;
  viewTransformInv.rotateZX = tmp.rotateXZ;
  viewTransformInv.rotateXY = tmp.rotateYX;
  viewTransformInv.rotateZY = tmp.rotateYZ;
  viewTransformInv.rotateXZ = tmp.rotateZX;
  viewTransformInv.rotateYZ = tmp.rotateZY;
  viewTransformInv.scale = 1.0f / tmp.scale;
  v = FloatVector4(float(x) - tmp.offsX, float(y) - tmp.offsY, z - tmp.offsZ,
                   0.0f) * viewTransformInv.scale;
  v = viewTransformInv.rotateXYZ(v);
  return true;
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
          renderer->clear();
          redrawWorldFlag = true;
        }
        break;
      case 1:                   // "cam"
        viewScale = float(parseFloat(argv[i + 1], "invalid view scale",
                                     1.0f / 512.0f, 16.0f));
        viewRotation =
            (signed char) parseInteger(argv[i + 2], 10,
                                       "invalid view direction", -1, 19);
        if (viewRotation < 0)
        {
          i = i + 3;
          if ((i + n) >= argc)
            throw FO76UtilsError("missing argument for %s", argv[i]);
          viewRotationX = float(parseFloat(argv[i], "invalid view X rotation",
                                           -360.0, 360.0));
          viewRotationY = float(parseFloat(argv[i + 1],
                                           "invalid view Y rotation",
                                           -360.0, 360.0));
          viewRotationZ = float(parseFloat(argv[i + 2],
                                           "invalid view Z rotation",
                                           -360.0, 360.0));
        }
        else
        {
          viewRotationX = viewRotations[size_t(viewRotation) * 3];
          viewRotationY = viewRotations[size_t(viewRotation) * 3 + 1];
          viewRotationZ = viewRotations[size_t(viewRotation) * 3 + 2];
        }
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
        redrawWorldFlag = true;
        break;
      case 4:                   // "deftxt"
        defTxtID =
            (unsigned int) parseInteger(argv[i + 1], 0,
                                        "invalid form ID", 0, 0x0FFFFFFF);
        renderer->clear();
        redrawWorldFlag = true;
        break;
      case 5:                   // "env"
        defaultEnvMap = argv[i + 1];
        renderer->setDefaultEnvMap(defaultEnvMap);
        redrawWorldFlag = true;
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
        if (cmdBuf.empty())
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
        else
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
        display.consolePrint("threads: %u", (unsigned int) threadCnt);
        if (!threadCnt)
        {
          display.consolePrint(" (uses default: %d)",
                               Renderer::getDefaultThreadCount());
        }
        display.consolePrint("\ndebug: %d\n", int(debugMode));
        display.consolePrint("textures: %d\n", int(enableTextures));
        display.consolePrint("txtcache: %u\n", textureCacheSize);
        display.consolePrint("mc: %u\n", (unsigned int) modelBatchCnt);
        display.consolePrint("rq: 0x%04X\n", (unsigned int) renderQuality);
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
        if (viewRotation < 0)
        {
          display.consolePrint("cam: %.7g -1 %.7g %.7g %.7g %.7g %.7g %.7g\n",
                               viewScale,
                               viewRotationX, viewRotationY, viewRotationZ,
                               camPositionX, camPositionY, camPositionZ);
        }
        else
        {
          display.consolePrint("cam: %.7g %d %.7g %.7g %.7g\n",
                               viewScale, int(viewRotation),
                               camPositionX, camPositionY, camPositionZ);
        }
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
        if (cmdBuf.empty())
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
        redrawWorldFlag = true;
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
      case 17:                  // "mc"
        modelBatchCnt =
            (unsigned char) parseInteger(argv[i + 1], 0,
                                         "invalid model cache size", 1, 64);
        redrawWorldFlag = true;
        break;
      case 18:                  // "minscale"
        objectsMinScale = float(parseFloat(argv[i + 1], "invalid view scale",
                                           1.0f / 512.0f, 16.0f));
        redrawWorldFlag = true;
        break;
      case 19:                  // "mip"
        textureMip = int(parseInteger(argv[i + 1], 10,
                                      "invalid texture mip level", 0, 15));
        renderer->clearTextureCache();
        redrawWorldFlag = true;
        break;
      case 20:                  // "mlod"
        modelLOD = int(parseInteger(argv[i + 1], 10,
                                    "invalid model LOD", 0, 4));
        redrawWorldFlag = true;
        break;
      case 21:                  // "ndis"
        noDisabledObjects =
            bool(parseInteger(argv[i + 1], 0,
                              "invalid argument for -ndis", 0, 1));
        redrawWorldFlag = true;
        break;
      case 22:                  // "r"
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
        renderer->clear();
        redrawWorldFlag = true;
        break;
      case 23:                  // "rq"
        {
          std::uint16_t tmp = renderQuality;
          renderQuality =
              std::uint16_t(parseInteger(argv[i + 1], 0,
                                         "invalid render quality", 0, 2047));
          if ((tmp ^ renderQuality) & 0x73)
            renderer->clearObjectPropertyCache();
        }
        redrawWorldFlag = true;
        break;
      case 24:                  // "rscale"
        reflZScale =
            float(parseFloat(argv[i + 1],
                             "invalid reflection view vector Z scale",
                             0.25, 16.0));
        redrawWorldFlag = true;
        break;
#if 0
      case 25:                  // "tc"
        break;
#endif
      case 26:                  // "textures"
        enableTextures =
            bool(parseInteger(argv[i + 1], 0,
                              "invalid argument for -textures", 0, 1));
        redrawWorldFlag = true;
        break;
      case 27:                  // "threads"
        threadCnt =
            (unsigned char) parseInteger(argv[i + 1], 10,
                                         "invalid number of threads", 0, 16);
        redrawWorldFlag = true;
        break;
      case 25:                  // "tc"
      case 28:                  // "txtcache"
        textureCacheSize =
            (unsigned int) parseInteger(argv[i + 1], 0,
                                        "invalid texture cache size",
                                        256, 65535);
        renderer->setTextureCacheSize(std::uint64_t(textureCacheSize) << 20);
        break;
      case 29:                  // "vis"
        distantObjectsOnly =
            bool(parseInteger(argv[i + 1], 0,
                              "invalid argument for -vis", 0, 1));
        redrawWorldFlag = true;
        break;
      case 30:                  // "w"
        {
          unsigned int  tmp1, tmp2;
          tmp1 = (unsigned int) parseInteger(argv[i + 1], 0,
                                             "invalid form ID", 0, 0x0FFFFFFF);
          if (!tmp1)
          {
            tmp1 = (esmFile.getESMVersion() < 0xC0U ?
                    0x0000003CU : 0x0025DA15U);
          }
          tmp2 = Renderer::findParentWorld(esmFile, tmp1);
          if (tmp2 == 0xFFFFFFFFU)
            errorMessage("form ID not found in ESM, or invalid record type");
          formID = tmp1;
          worldID = tmp2;
        }
        renderer->clear();
        redrawWorldFlag = true;
        break;
      case 31:                  // "watercolor"
        {
          std::uint32_t tmp =
              std::uint32_t(parseInteger(argv[i + 1], 0, "invalid water color",
                                         0, 0x7FFFFFFF));
          tmp = tmp + (tmp & 0x7F000000U) + ((tmp >> 30) << 24);
          if (!(tmp & 0xFF000000U))
            tmp = 0U;
          if (bool(tmp) != bool(waterColor))
            renderer->clearObjectPropertyCache();
          waterColor = tmp;
          redrawWorldFlag = true;
        }
        break;
      case 32:                  // "wrefl"
        waterReflectionLevel =
            float(parseFloat(argv[i + 1],
                             "invalid water environment map scale", 0.0, 4.0));
        redrawWorldFlag = true;
        break;
      case 33:                  // "wscale"
        waterUVScale =
            int(parseInteger(argv[i + 1], 0,
                             "invalid water texture tile size", 2, 32768));
        redrawWorldFlag = true;
        break;
      case 34:                  // "wtxt"
        waterTexture = argv[i + 1];
        renderer->setWaterTexture(waterTexture);
        redrawWorldFlag = true;
        break;
      case 35:                  // "xm"
        if (argv[i + 1][0])
        {
          excludeModelPatterns.push_back(std::string(argv[i + 1]));
          renderer->addExcludeModelPattern(excludeModelPatterns.back());
          renderer->clearObjectPropertyCache();
          redrawWorldFlag = true;
        }
        break;
      case 36:                  // "xm_clear"
        if (excludeModelPatterns.size() > 0)
        {
          excludeModelPatterns.clear();
          renderer->clearExcludeModelPatterns();
          renderer->clearObjectPropertyCache();
          redrawWorldFlag = true;
        }
        break;
      case 37:                  // "zrange"
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
  std::vsnprintf(tmpBuf.data(), tmpBuf.size(), fmt, ap);
  va_end(ap);
  tmpBuf.back() = '\0';
  display.unlockScreenSurface();
  display.clearSurface();
  display.clearTextBuffer();
  display.consolePrint("\033[41m\033[33m\033[1m    Error: %s    \n",
                       tmpBuf.data());
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
  if (BRANCH_UNLIKELY(redrawWorldFlag))
  {
    redrawWorldFlag = false;
    renderPass = 0;
    std::memset(imageBuf.data(), 0, imageDataSize * sizeof(std::uint32_t));
    float   *p = renderer->getZBufferData();
    float   z = zMax;
    for (size_t i = imageDataSize; i-- > 0; p++)
      *p = z;
    display.clearTextBuffer();
    renderer->setThreadCount(threadCnt);
    renderer->setTextureCacheSize(std::uint64_t(textureCacheSize) << 20);
    renderer->setModelCacheSize(modelBatchCnt);
    renderer->setDistantObjectsOnly(distantObjectsOnly);
    renderer->setNoDisabledObjects(noDisabledObjects);
    renderer->setEnableTextures(enableTextures);
    renderer->setLandDefaultColor(ltxtDefColor);
    renderer->setTextureMipLevel(textureMip);
    renderer->setLandTxtRGBScale(landTextureMult);
    renderer->setModelLOD(modelLOD);
    renderer->setWaterColor(waterColor);
    renderer->setWaterEnvMapScale(waterReflectionLevel);
    renderer->setEnableSCOL(false);
    renderer->setEnableAllObjects(false);
    renderer->setRenderQuality(renderQuality);
    renderer->setDebugMode(debugMode);
    setViewTransform();
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
      int     ltxtMip =
          (esmFile.getESMVersion() < 0x80U ? 14 : 15)
          - (textureMip + int(std::bit_width((unsigned int) ltxtResolution)));
      ltxtMip = std::max(std::min(ltxtMip, 15 - textureMip), 0);
      renderer->setLandTextureMip(float(ltxtMip));
      renderer->loadTerrain((btdPath.empty() ? dataPath : btdPath.c_str()),
                            worldID, defTxtID, btdLOD,
                            terrainX0, terrainY0, terrainX1, terrainY1);
      display.consolePrint("Rendering terrain\n");
      renderer->initRenderPass(0, worldID);
      redrawScreenFlag = true;
    }
    else
    {
      renderPass = -256;                // skip terrain rendering
    }
  }
  else if (renderPass < 0)
  {
    renderPass = renderPass & 255;
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
      try
      {
        MapImage  mapImage(esmFile, markerDefsFileName.c_str(),
                           imageBuf.data(), width, height,
                           renderer->getViewTransform());
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
    if (!renderer->renderObjects(int(frameTimeMin)))
    {
      display.consolePrint("\r    %7u / %7u  ",
                           (unsigned int) renderer->getObjectsRendered(),
                           (unsigned int) renderer->getObjectCount());
    }
    else
    {
      // the current pass is complete
      renderPass = renderPass | -256;
      display.consolePrint("\r    %7u / %7u  \n",
                           (unsigned int) renderer->getObjectCount(),
                           (unsigned int) renderer->getObjectCount());
    }
    redrawScreenFlag = true;
  }
  if (redrawScreenFlag)
  {
    redrawScreenFlag = false;
    std::memcpy(display.lockDrawSurface(), imageBuf.data(),
                imageDataSize * sizeof(std::uint32_t));
    display.unlockDrawSurface();
    display.drawText(0, -1, display.getTextRows(), 0.75f, 1.0f);
    display.blitSurface();
  }
}

static void printRecordType(char *buf, unsigned int n)
{
  for (int i = 0; i < 4; i++)
  {
    unsigned char c = (unsigned char) (n & 0x7F);
    n = n >> 8;
    if (c < 0x20 || c >= 0x7F)
      c = 0x3F;
    buf[i] = char(c);
  }
}

void WorldSpaceViewer::printReferenceInfo(unsigned int refrFormID)
{
  refrFormID = (refrFormID & 0x00FFFFFFU) | (formID & 0x0F000000U);
  std::sprintf(tmpBuf.data(), "0x%08X", refrFormID);
  (void) SDL_SetClipboardText(tmpBuf.data());
  const ESMFile::ESMRecord  *r = esmFile.findRecord(refrFormID);
  if (!r)
  {
    display.consolePrint("Form ID = %s\n", tmpBuf.data());
    return;
  }
  std::sprintf(tmpBuf.data(), "???? form ID: 0x%08X", refrFormID);
  printRecordType(tmpBuf.data(), r->type);
  std::string buf(tmpBuf.data());
  std::string edid;
  unsigned int  refrName = 0U;
  float   refrX = 0.0f;
  float   refrY = 0.0f;
  float   refrZ = 0.0f;
  float   refrRX = 0.0f;
  float   refrRY = 0.0f;
  float   refrRZ = 0.0f;
  unsigned int  cellFlags = 0U;
  int     cellX = 0;
  int     cellY = 0;
  ESMFile::ESMField f(esmFile, *r);
  while (f.next())
  {
    if (f == "EDID" && f.size() > 0)
    {
      f.readString(edid);
    }
    else if (f == "NAME" && (*r == "REFR" || *r == "ACHR") && f.size() >= 4)
    {
      refrName = f.readUInt32Fast();
    }
    else if (f == "DATA" && (*r == "REFR" || *r == "ACHR") && f.size() >= 24)
    {
      refrX = f.readFloat();
      refrY = f.readFloat();
      refrZ = f.readFloat();
      refrRX = f.readFloat() * (180.0f / 3.14159265f);
      refrRY = f.readFloat() * (180.0f / 3.14159265f);
      refrRZ = f.readFloat() * (180.0f / 3.14159265f);
    }
    else if (f == "DATA" && *r == "CELL" && f.size() >= 1)
    {
      if (f.size() >= 2)
        cellFlags = f.readUInt16Fast();
      else
        cellFlags = f.readUInt8Fast();
    }
    else if (f == "XCLC" && *r == "CELL" && f.size() >= 8)
    {
      cellX = f.readInt32();
      cellY = f.readInt32();
    }
  }
  if (!edid.empty())
  {
    buf += " (\"";
    buf += edid;
    buf += "\")";
  }
  if (r->flags)
  {
    std::sprintf(tmpBuf.data(), ", flags: 0x%08X", r->flags);
    buf += tmpBuf.data();
  }
  ESMFile::ESMVCInfo  vcInfo;
  esmFile.getVersionControlInfo(vcInfo, *r);
  std::sprintf(tmpBuf.data(), ", timestamp: %04u-%02u-%02u, user: 0x%04X\n",
               vcInfo.year, vcInfo.month, vcInfo.day, vcInfo.userID1);
  buf += tmpBuf.data();
  if (*r == "REFR" || *r == "ACHR")
  {
    edid.clear();
    std::string modelPath;
    const ESMFile::ESMRecord  *r2 = (ESMFile::ESMRecord *) 0;
    if (refrName)
    {
      r2 = esmFile.findRecord(refrName);
      if (r2)
      {
        ESMFile::ESMField f2(esmFile, *r2);
        while (f2.next())
        {
          if (f2 == "EDID" && f2.size() > 0)
            f2.readString(edid);
          else if ((f2 == "MODL" || f2 == "MOD2") && f2.size() > 4)
            f2.readPath(modelPath, std::string::npos, "meshes/", ".nif");
        }
      }
    }
    std::sprintf(tmpBuf.data(), "    NAME: 0x%08X", refrName);
    buf += tmpBuf.data();
    if (r2)
    {
      buf += " (????";
      printRecordType(&(buf[buf.length() - 4]), r2->type);
      if (!edid.empty())
      {
        buf += " \"";
        buf += edid;
        buf += '"';
      }
      buf += ')';
      if (!modelPath.empty())
      {
        buf += ", \"";
        buf += modelPath;
        buf += '"';
      }
    }
    std::sprintf(tmpBuf.data(), "\n    DATA: X: %.7g, Y: %.7g, Z: %.7g"
                                ", RX: %.5g, RY: %.5g, RZ: %.5g\n",
                 refrX, refrY, refrZ, refrRX, refrRY, refrRZ);
    buf += tmpBuf.data();
  }
  else if (*r == "CELL")
  {
    std::sprintf(tmpBuf.data(), "    DATA: 0x%04X", cellFlags);
    buf += tmpBuf.data();
    if (!(cellFlags & 1U))
    {
      std::sprintf(tmpBuf.data(), ", XCLC: %3d, %3d", cellX, cellY);
      buf += tmpBuf.data();
    }
    buf += '\n';
  }
  display.consolePrint("%s", buf.c_str());
}

void WorldSpaceViewer::printWorldSpaceXYZ(int x, int y)
{
  FloatVector4  v;
  if (screenToWorldSpace(v, x, y))
    display.consolePrint("X: %.7g, Y: %.7g, Z: %.7g\n", v[0], v[1], v[2]);
}

void WorldSpaceViewer::pollEvents()
{
  display.pollEvents(eventBuf, (renderPass < 3 ? 0 : -1000), false, true);
  int     x0 = (width + 1) >> 1;
  int     y0 = (height - 1) >> 1;
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
#if USE_PIXELFMT_RGB10A2
        c = ((c >> 2) & 0xFFU) | ((c >> 4) & 0xFF00U) | ((c >> 6) & 0xFF0000U);
#else
        c = ((c & 0xFFU) << 16) | ((c >> 16) & 0xFFU) | (c & 0xFF00U);
#endif
        printReferenceInfo(c);
        redrawScreenFlag = true;
      }
      else if (eventBuf[i].data3() == 3 && eventBuf[i].data4() == 1)
      {
        printWorldSpaceXYZ(x, y);
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
        {
          int     newViewDir =
              getViewRotation(eventBuf[i].data1(), eventBuf[i].data2());
          if (newViewDir >= 0 && newViewDir != viewRotation)
          {
            viewRotation = newViewDir;
            viewRotationX = viewRotations[size_t(viewRotation) * 3];
            viewRotationY = viewRotations[size_t(viewRotation) * 3 + 1];
            viewRotationZ = viewRotations[size_t(viewRotation) * 3 + 2];
            FloatVector4  v;
            if (screenToWorldSpace(v, x0, y0))
            {
              camPositionX = v[0];
              camPositionY = v[1];
              camPositionZ = v[2];
              size_t  offs = size_t(y0) * size_t(width) + size_t(x0);
              zStep -= renderer->getZBufferData()[offs];
            }
            setViewTransform();
            redrawWorldFlag = true;
          }
        }
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
          float   viewOffsX = renderer->getViewTransform().offsX;
          float   viewOffsY = renderer->getViewTransform().offsY;
          float   viewOffsZ = renderer->getViewTransform().offsZ;
          viewOffsX -= (float(width) * 0.5f);
          viewOffsY -= (float(height - 2) * 0.5f);
          if (viewRotation < 0)
          {
            std::sprintf(tmpBuf.data(),
                         "-light %.7g %.7g %.7g "
                         "-view %.7g %.7g %.7g %.7g %.7g %.7g %.7g "
                         "(cam: %.7g -1 %.7g %.7g %.7g %.7g %.7g %.7g)",
                         rgbScale, lightRotationY, lightRotationZ,
                         viewScale, viewRotationX, viewRotationY, viewRotationZ,
                         viewOffsX, viewOffsY, viewOffsZ,
                         viewScale, viewRotationX, viewRotationY, viewRotationZ,
                         camPositionX, camPositionY, camPositionZ);
          }
          else
          {
            std::sprintf(tmpBuf.data(),
                         "-light %.7g %.7g %.7g "
                         "-view %.7g %.7g %.7g %.7g %.7g %.7g %.7g "
                         "(cam: %.7g %d %.7g %.7g %.7g)",
                         rgbScale, lightRotationY, lightRotationZ,
                         viewScale, viewRotationX, viewRotationY, viewRotationZ,
                         viewOffsX, viewOffsY, viewOffsZ,
                         viewScale, int(viewRotation),
                         camPositionX, camPositionY, camPositionZ);
          }
          display.consolePrint("%s\n", tmpBuf.data());
          (void) SDL_SetClipboardText(tmpBuf.data());
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
      case 'r':
        display.clearTextBuffer();
        for (size_t j = 0; j < (sizeof(viewRotationMessages) / sizeof(char *));
             j++)
        {
          if (j == size_t(viewRotation))
          {
            display.consolePrint("\033[4m\033[38;5;228m%s\033[m",
                                 viewRotationMessages[j]);
          }
          else
          {
            display.consolePrint("%s", viewRotationMessages[j]);
          }
        }
        redrawScreenFlag = true;
        break;
      case 'h':
        display.clearTextBuffer();
        display.consolePrint("%s", keyboardUsageString);
        redrawScreenFlag = true;
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
      case SDLDisplay::SDLKeySymF11:
        saveScreenshot(true);
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
    NIFFile::NIFVertexTransform tmp(renderer->getViewTransform());
    NIFFile::NIFVertexTransform viewTransformInv(tmp);
    viewTransformInv.rotateYX = tmp.rotateXY;
    viewTransformInv.rotateZX = tmp.rotateXZ;
    viewTransformInv.rotateXY = tmp.rotateYX;
    viewTransformInv.rotateZY = tmp.rotateYZ;
    viewTransformInv.rotateXZ = tmp.rotateZX;
    viewTransformInv.rotateYZ = tmp.rotateZY;
    viewTransformInv.scale = 1.0f / tmp.scale;
    viewTransformInv.rotateXYZ(xStep, yStep, zStep);
    camPositionX += (xStep * viewTransformInv.scale);
    camPositionY += (yStep * viewTransformInv.scale);
    camPositionZ += (zStep * viewTransformInv.scale);
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
    if (cmdBuf.length() > (tmpBuf.size() - 1))
      cmdBuf.resize(tmpBuf.size() - 1);
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
        args.push_back(tmpBuf.data() + i);
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
        const ESMFile::ESMRecord  *r = esmFile.findRecord(n);
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

void WorldSpaceViewer::saveScreenshot(bool disableDownsampling)
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
    int     w, h;
    size_t  pitch;
    const std::uint32_t *p;
    int     pixelFmt = DDSInputFile::pixelFormatRGBA32;
    if (disableDownsampling)
    {
      w = display.getWidth();
      h = display.getHeight();
      pitch = size_t(w);
      p = imageBuf.data();
#if USE_PIXELFMT_RGB10A2
      pixelFmt = DDSInputFile::pixelFormatA2R10G10B10;
#endif
    }
    else
    {
      std::memcpy(display.lockDrawSurface(), imageBuf.data(),
                  imageDataSize * sizeof(std::uint32_t));
      display.unlockDrawSurface();
      display.blitSurface();
      w = display.getWidth() >> int(display.getIsDownsampled());
      h = display.getHeight() >> int(display.getIsDownsampled());
      pitch = display.getPitch();
      p = display.lockScreenSurface();
      screenSurfaceLocked = true;
    }
    DDSOutputFile f(fileName.c_str(), w, h, pixelFmt);
    for (int y = 0; y < h; y++, p = p + pitch)
      f.writeImageData(p, size_t(w), pixelFmt);
    if (screenSurfaceLocked)
    {
      display.unlockScreenSurface();
      screenSurfaceLocked = false;
    }
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
      if (n0 == 1 && (i + 2) < argc && argv[i + 2][0] == '-')
        n = n + 3;                      // "cam" with custom view rotations
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

