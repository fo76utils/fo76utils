
#include "common.hpp"
#include "filebuf.hpp"
#include "ddstxt.hpp"
#include "ba2file.hpp"
#include "esmfile.hpp"
#include "landdata.hpp"

#include <thread>

static bool checkNameExtension(const char *fileName, const char *suffix)
{
  unsigned int  n = 0U;
  unsigned int  s = 0U;
  size_t  len;
  if (fileName && *fileName && (len = std::strlen(fileName)) >= 4)
    n = FileBuffer::readUInt32Fast(fileName + (len - 4));
  if (suffix && *suffix && (len = std::strlen(suffix)) >= 4)
    s = FileBuffer::readUInt32Fast(suffix + (len - 4));
  n = n | ((n & 0x40404000U) >> 1);
  s = s | ((s & 0x40404000U) >> 1);
  return (n == s);
}

static void loadTextures(
    std::vector< DDSTexture * >& textures,
    const char *listFileName, const LandscapeData *landData,
    bool verboseMode, int mipOffset = 0, const BA2File *ba2File = 0)
{
  std::vector< std::string >  fileNames;
  if (!landData)
  {
    FileBuffer  inFile(listFileName);
    std::string s;
    while (true)
    {
      unsigned char c = '\n';
      if (inFile.getPosition() < inFile.size())
        c = inFile.readUInt8();
      if (c == '\t')
      {
        if (!checkNameExtension(s.c_str(), ".dds"))
        {
          s.clear();
          continue;
        }
        c = '\n';
      }
      if (c == '\r' || c == '\n')
      {
        while (s.length() > 0 && s[s.length() - 1] == ' ')
          s.resize(s.length() - 1);
        if (!s.empty())
        {
          fileNames.push_back(s);
          s.clear();
        }
      }
      if (inFile.getPosition() >= inFile.size())
        break;
      if (c > (unsigned char) ' ' || (c == ' ' && !s.empty()))
        s += char(c);
    }
  }
  else
  {
    for (size_t i = 0; i < landData->getTextureCount(); i++)
      fileNames.push_back(landData->getTextureDiffuse(i));
  }
  if (fileNames.size() < 1)
    throw errorMessage("texture file list is empty");
  textures.resize(fileNames.size(), (DDSTexture *) 0);
  std::vector< unsigned char >  tmpBuf;
  for (size_t i = 0; i < fileNames.size(); i++)
  {
    if (fileNames[i].empty())
      continue;
    try
    {
      if (verboseMode)
      {
        std::fprintf(stderr, "\rLoading texture %3d: %-58s",
                     int(i), fileNames[i].c_str());
      }
      if (ba2File)
      {
        int     n = ba2File->extractTexture(tmpBuf, fileNames[i], mipOffset);
        textures[i] = new DDSTexture(&(tmpBuf.front()), tmpBuf.size(), n);
      }
      else
      {
        textures[i] = new DDSTexture(fileNames[i].c_str(), mipOffset);
      }
    }
    catch (...)
    {
      if (checkNameExtension(fileNames[i].c_str(), ".dds") && !(ba2File && i))
      {
        if (verboseMode)
          std::fputc('\n', stderr);
        throw;
      }
    }
  }
  if (verboseMode)
    std::fputc('\n', stderr);
}

struct RenderThread
{
  std::vector< unsigned char >  outBuf;
  const unsigned char *txtSetData;
  const unsigned char *ltexData32;
  const unsigned char *vclrData24;
  const unsigned char *ltexData16;
  const unsigned char *vclrData16;
  const unsigned char *gcvrData;
  const DDSTexture * const  *landTextures;
  size_t  landTextureCnt;
  float   mipLevel;
  unsigned int  rgbScale;
  unsigned int  defaultColor;
  int     width;
  int     height;
  int     xyScale;
  float   txtScale;
  bool    integerMip;
  bool    isFO76;
  unsigned char txtSetMip;
  unsigned char fo76VClrMip;
  static inline unsigned long long rgb24ToRGB64(unsigned int c)
  {
    return ((unsigned long long) (c & 0x000000FF)
            | ((unsigned long long) (c & 0x0000FF00) << 12)
            | ((unsigned long long) (c & 0x00FF0000) << 24));
  }
  inline unsigned int getFO76VertexColor(size_t offs) const
  {
    unsigned int  c = FileBuffer::readUInt16Fast(vclrData16 + (offs << 1));
    c = ((c & 0x7C00) << 6) | ((c & 0x03E0) << 3) | (c & 0x001F);
    return (c * 6U + 0x00202020);
  }
  void getFO76VertexColor(unsigned int& r, unsigned int& g, unsigned int& b,
                          int x, int y) const;
  unsigned int renderPixel(int x, int y, int txtX, int txtY);
  void renderLines(int y0, int y1);
  static void runThread(RenderThread *p, int y0, int y1);
};

void RenderThread::getFO76VertexColor(unsigned int& r, unsigned int& g,
                                      unsigned int& b, int x, int y) const
{
  y = y - ((1 << fo76VClrMip) - 1);
  y = (y >= 0 ? y : 0);
  x = x << (8 - fo76VClrMip);
  y = y << (8 - fo76VClrMip);
  int     xf = x & 255;
  int     yf = y & 255;
  x = x >> 8;
  y = y >> 8;
  size_t  w = size_t(width >> (xyScale + fo76VClrMip));
  size_t  h = size_t(height >> (xyScale + fo76VClrMip));
  size_t  offs0 = size_t(y) * w + size_t(x);
  size_t  offs2 = offs0;
  if (y < int(h - 1))
    offs2 += w;
  size_t  offs1 = offs0;
  size_t  offs3 = offs2;
  if (x < int(w - 1))
  {
    offs1++;
    offs3++;
  }
  unsigned int  c =
      blendRGBA32(blendRGBA32(getFO76VertexColor(offs0),
                              getFO76VertexColor(offs1), xf),
                  blendRGBA32(getFO76VertexColor(offs2),
                              getFO76VertexColor(offs3), xf), yf);
  r = (r * ((c >> 16) & 0xFF) + 64) >> 7;
  g = (g * ((c >> 8) & 0xFF) + 64) >> 7;
  b = (b * (c & 0xFF) + 64) >> 7;
}

unsigned int RenderThread::renderPixel(int x, int y, int txtX, int txtY)
{
  x = (x < width ? x : (width - 1)) >> xyScale;
  y = (y < height ? y : (height - 1)) >> xyScale;
  size_t  offs = size_t(y) * size_t(width >> xyScale) + size_t(x);
  const unsigned char *p0 =
      txtSetData
      + (((size_t(y) >> txtSetMip) * (size_t(width >> xyScale) >> txtSetMip)
          + (size_t(x) >> txtSetMip)) << 4);
  unsigned long long  a = 0U;
  if (ltexData32)
  {
    a = FileBuffer::readUInt32Fast(ltexData32 + (offs << 2));
    a = (a & 0x000000000000FFFFULL) | ((a & 0x00000000FFFF0000ULL) << 12);
    a = (a & 0x0000000FF00000FFULL) | ((a & 0x00000FF00000FF00ULL) << 6);
    a = (a & 0x00003C00F003C00FULL) | ((a & 0x0003C00F003C00F0ULL) << 3);
    a = (a * 0x0380U) | 105U;
  }
  else if (ltexData16)
  {
    a = FileBuffer::readUInt16Fast(ltexData16 + (offs << 1));
    a = (a & 0x000000000000003FULL) | ((a & 0x0000000000007FC0ULL) << 8);
    a = (a & 0x000000000001C007ULL) | ((a & 0x00000000007E0038ULL) << 4);
    a = (a & 0x0000000000E1C387ULL) | ((a & 0x0000000007000000ULL) << 4);
    a = (a * 0x0780U) | 105U;
  }
  unsigned long long  c = rgb24ToRGB64(defaultColor);
  unsigned char prvTexture = 0xFF;
  if (!integerMip)
  {
    for ( ; a; a = a >> 7, p0++)
    {
      unsigned int  aTmp = (unsigned int) (a & 0x7FU);
      if (!aTmp)
        continue;
      unsigned char t = *p0;
      if (t >= landTextureCnt || t == prvTexture || !landTextures[t])
        continue;
      unsigned long long  cTmp =
          rgb24ToRGB64(landTextures[t]->getPixelT(float(txtX) * txtScale,
                                                  float(txtY) * txtScale,
                                                  mipLevel));
      prvTexture = (aTmp == 105 ? t : (unsigned char) 0xFF);
      if (aTmp != 105)
        cTmp = blendRGB64(c, cTmp, (aTmp * 39U + 8U) >> 4);
      c = cTmp;
    }
    if (gcvrData)
    {
      p0 = txtSetData + ((size_t(p0 - txtSetData) & ~7UL) | 8UL);
      a = gcvrData[offs];
      for ( ; a; a = a >> 1, p0++)
      {
        if (!(a & 1))
          continue;
        unsigned char t = *p0;
        if (t >= landTextureCnt || !landTextures[t])
          continue;
        unsigned int  cTmp =
            landTextures[t]->getPixelT(float(txtX) * txtScale,
                                       float(txtY) * txtScale, mipLevel);
        c = blendRGB64(c, rgb24ToRGB64(cTmp), cTmp >> 25);
      }
    }
  }
  else
  {
    for ( ; a; a = a >> 7, p0++)
    {
      unsigned int  aTmp = (unsigned int) (a & 0x7FU);
      if (!aTmp)
        continue;
      unsigned char t = *p0;
      if (t >= landTextureCnt || t == prvTexture || !landTextures[t])
        continue;
      unsigned long long  cTmp =
          rgb24ToRGB64(landTextures[t]->getPixelN(txtX, txtY, int(mipLevel)));
      prvTexture = (aTmp == 105 ? t : (unsigned char) 0xFF);
      if (aTmp != 105)
        cTmp = blendRGB64(c, cTmp, (aTmp * 39U + 8U) >> 4);
      c = cTmp;
    }
    if (gcvrData)
    {
      p0 = txtSetData + ((size_t(p0 - txtSetData) & ~7UL) | 8UL);
      a = gcvrData[offs];
      for ( ; a; a = a >> 1, p0++)
      {
        if (!(a & 1))
          continue;
        unsigned char t = *p0;
        if (t >= landTextureCnt || !landTextures[t])
          continue;
        unsigned int  cTmp =
            landTextures[t]->getPixelN(txtX, txtY, int(mipLevel));
        c = blendRGB64(c, rgb24ToRGB64(cTmp), cTmp >> 25);
      }
    }
  }
  c = c * rgbScale;
  unsigned int  r = (unsigned int) (c & 0x000FFFFF);
  unsigned int  g = (unsigned int) ((c >> 20) & 0x000FFFFF);
  unsigned int  b = (unsigned int) ((c >> 40) & 0x000FFFFF);
  if (vclrData16)
  {
    getFO76VertexColor(r, g, b, x, y);
  }
  else if (vclrData24)
  {
    b = (b * vclrData24[offs * 3] + 127U) / 255U;
    g = (g * vclrData24[offs * 3 + 1] + 127U) / 255U;
    r = (r * vclrData24[offs * 3 + 2] + 127U) / 255U;
  }
  r = (r < 0xFF00 ? ((r + 128) >> 8) : 255U);
  g = (g < 0xFF00 ? ((g + 128) >> 8) : 255U);
  b = (b < 0xFF00 ? ((b + 128) >> 8) : 255U);
  return (0xFF000000U | (r << 16) | (g << 8) | b);
}

void RenderThread::renderLines(int y0, int y1)
{
  outBuf.resize(size_t(width) * size_t(y1 - y0) * 3U);
  for (int y = y0; y < y1; y++)
  {
    int     yf = (y << (8 - xyScale)) & 0xFF;
    for (int x = 0; x < width; x++)
    {
      unsigned int  c = renderPixel(x, y, x, y);
      int     xf = (x << (8 - xyScale)) & 0xFF;
      if (xf)
        c = blendRGBA32(c, renderPixel(x + (1 << xyScale), y, x, y), xf);
      if (yf)
      {
        unsigned int  c2 = renderPixel(x, y + (1 << xyScale), x, y);
        if (xf)
        {
          c2 = blendRGBA32(c2, renderPixel(x + (1 << xyScale),
                                           y + (1 << xyScale), x, y), xf);
        }
        c = blendRGBA32(c, c2, yf);
      }
      size_t  offs = (size_t(y - y0) * size_t(width) + size_t(x)) * 3U;
      outBuf[offs] = (unsigned char) (c & 0xFF);
      outBuf[offs + 1] = (unsigned char) ((c >> 8) & 0xFF);
      outBuf[offs + 2] = (unsigned char) ((c >> 16) & 0xFF);
    }
  }
}

void RenderThread::runThread(RenderThread *p, int y0, int y1)
{
  p->renderLines(y0, y1);
}

static const char *usageStrings[] =
{
  "Usage:",
  "    landtxt INFILE.DDS OUTFILE.DDS TXTSET.DDS LTEXLIST.TXT [OPTIONS...]",
  "    landtxt INFILE.ESM[,...] OUTFILE.DDS [OPTIONS...]",
  "",
  "General options:",
  "    --                  remaining options are file names",
  "    -d PATHNAME         game data path or texture archive file",
  "    -mip FLOAT          texture mip level",
  "    -mult FLOAT         multiply RGB output with the specified value",
  "    -defclr 0x00RRGGBB  default color for untextured areas",
  "    -scale INT          scale output resolution by 2^N",
  "    -threads INT        set the number of threads to use",
  "    -q                  do not print texture file names",
  "",
  "DDS input file options:",
  "    -vclr FILENAME.DDS  vertex color file name",
  "    -gcvr FILENAME.DDS  ground cover file name",
  "",
  "ESM/BTD input file options:",
  "    -btd FILENAME.BTD   read terrain data from Fallout 76 .btd file",
  "    -w FORMID           form ID of world to use from ESM input file",
  "    -r X0 Y0 X1 Y1      limit range of cells to X0,Y0 (SW) to X1,Y1 (NE)",
  "    -l INT              level of detail to use from BTD file (0 to 4)",
  "    -deftxt FORMID      form ID of default texture",
  "    -no-vclr            do not use vertex color data",
  "    -no-gcvr            do not use ground cover data",
  (char *) 0
};

int main(int argc, char **argv)
{
  std::vector< DDSTexture * >   landTextures;
  std::vector< std::thread * >  threads;
  DDSInputFile  *inFile = (DDSInputFile *) 0;
  DDSInputFile  *txtSetFile = (DDSInputFile *) 0;
  DDSInputFile  *vclrFile = (DDSInputFile *) 0;
  DDSInputFile  *gcvrFile = (DDSInputFile *) 0;
  ESMFile       *esmFile = (ESMFile *) 0;
  BA2File       *ba2File = (BA2File *) 0;
  LandscapeData *landData = (LandscapeData *) 0;
  int     err = 1;
  try
  {
    const char    *vclrFileName = (char *) 0;
    const char    *gcvrFileName = (char *) 0;
    float         mipLevel = 5.0f;
    unsigned int  rgbScale = 256;
    unsigned int  defaultColor = 0x003F3F3F;
    int           xyScale = 0;
    int           threadCnt = int(std::thread::hardware_concurrency());
    threadCnt = (threadCnt > 1 ? (threadCnt < 256 ? threadCnt : 256) : 1);
    bool          verboseMode = true;
    bool          isFO76 = false;
    unsigned char txtSetMip = 0;
    unsigned char fo76VClrMip = 0;
    const char    *archivePath = (char *) 0;
    const char    *btdFileName = (char *) 0;
    unsigned int  worldFormID = 0U;
    unsigned int  defTxtID = 0U;
    int           xMin = -32768;
    int           yMin = -32768;
    int           xMax = 32767;
    int           yMax = 32767;
    unsigned char btdLOD = 2;
    bool          disableVCLR = false;
    bool          disableGCVR = false;
    unsigned int  hdrBuf[11];

    std::vector< const char * > args;
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
      if (std::strcmp(argv[i], "-h") == 0 ||
          std::strcmp(argv[i], "--help") == 0)
      {
        args.clear();
        err = 0;
        break;
      }
      if (std::strcmp(argv[i], "-d") == 0)
      {
        if (++i >= argc)
          throw errorMessage("missing argument for %s", argv[i - 1]);
        archivePath = argv[i];
      }
      else if (std::strcmp(argv[i], "-mip") == 0)
      {
        if (++i >= argc)
          throw errorMessage("missing argument for %s", argv[i - 1]);
        mipLevel = float(parseFloat(argv[i], "invalid mip level", 0.0, 16.0));
      }
      else if (std::strcmp(argv[i], "-mult") == 0)
      {
        if (++i >= argc)
          throw errorMessage("missing argument for %s", argv[i - 1]);
        double  tmp =
            parseFloat(argv[i], "invalid RGB output multiplier", 0.5, 8.0);
        rgbScale = (unsigned int) int(tmp * 256.0 + 0.5);
      }
      else if (std::strcmp(argv[i], "-defclr") == 0)
      {
        if (++i >= argc)
          throw errorMessage("missing argument for %s", argv[i - 1]);
        defaultColor =
            (unsigned int) parseInteger(argv[i], 0, "invalid default color",
                                        0L, 0x00FFFFFFL);
      }
      else if (std::strcmp(argv[i], "-scale") == 0)
      {
        if (++i >= argc)
          throw errorMessage("missing argument for %s", argv[i - 1]);
        xyScale = int(parseInteger(argv[i], 10, "invalid scale", 0, 6));
      }
      else if (std::strcmp(argv[i], "-threads") == 0)
      {
        if (++i >= argc)
          throw errorMessage("missing argument for %s", argv[i - 1]);
        threadCnt =
            int(parseInteger(argv[i], 10, "invalid thread count", 1, 64));
      }
      else if (std::strcmp(argv[i], "-q") == 0)
      {
        verboseMode = false;
      }
      else if (std::strcmp(argv[i], "-vclr") == 0)
      {
        if (++i >= argc)
          throw errorMessage("missing argument for %s", argv[i - 1]);
        vclrFileName = argv[i];
      }
      else if (std::strcmp(argv[i], "-gcvr") == 0)
      {
        if (++i >= argc)
          throw errorMessage("missing argument for %s", argv[i - 1]);
        gcvrFileName = argv[i];
      }
      else if (std::strcmp(argv[i], "-btd") == 0)
      {
        if (++i >= argc)
          throw errorMessage("missing argument for %s", argv[i - 1]);
        btdFileName = argv[i];
      }
      else if (std::strcmp(argv[i], "-w") == 0)
      {
        if (++i >= argc)
          throw errorMessage("missing argument for %s", argv[i - 1]);
        worldFormID =
            (unsigned int) parseInteger(argv[i], 0, "invalid world form ID",
                                        0L, 0x0FFFFFFFL);
      }
      else if (std::strcmp(argv[i], "-r") == 0)
      {
        if ((i + 4) >= argc)
          throw errorMessage("missing argument for %s", argv[i]);
        xMin = int(parseInteger(argv[++i], 10, "invalid xMin", -32768, 32767));
        yMin = int(parseInteger(argv[++i], 10, "invalid yMin", -32768, 32767));
        xMax = int(parseInteger(argv[++i], 10, "invalid xMax", xMin, 32767));
        yMax = int(parseInteger(argv[++i], 10, "invalid yMax", yMin, 32767));
      }
      else if (std::strcmp(argv[i], "-l") == 0)
      {
        if (++i >= argc)
          throw errorMessage("missing argument for %s", argv[i - 1]);
        btdLOD =
            (unsigned char) parseInteger(argv[i], 10, "invalid level of detail",
                                         0, 4);
      }
      else if (std::strcmp(argv[i], "-deftxt") == 0)
      {
        if (++i >= argc)
          throw errorMessage("missing argument for %s", argv[i - 1]);
        defTxtID =
            (unsigned int) parseInteger(argv[i], 0, "invalid default texture",
                                        0L, 0x0FFFFFFFL);
      }
      else if (std::strcmp(argv[i], "-no-vclr") == 0)
      {
        disableVCLR = true;
      }
      else if (std::strcmp(argv[i], "-no-gcvr") == 0)
      {
        disableGCVR = true;
      }
      else
      {
        throw errorMessage("invalid command line option: %s", argv[i]);
      }
    }
    if (args.size() != (checkNameExtension(args[0], ".esm") ? 2 : 4))
    {
      for (size_t i = 0; usageStrings[i]; i++)
        std::fprintf(stderr, "%s\n", usageStrings[i]);
      return err;
    }
    if (rgbScale > 256)
    {
      defaultColor = blendRGBA32(0, defaultColor,
                                 int((65536U + (rgbScale >> 1)) / rgbScale));
    }
    else
    {
      defaultColor = blendRGBA32(0, defaultColor << 1,
                                 int((32768U + (rgbScale >> 1)) / rgbScale));
    }
    threads.resize(size_t(threadCnt), (std::thread *) 0);

    if (archivePath)
      ba2File = new BA2File(archivePath, ".dds\t.bgsm", "/lod/\t/actors/");
    int     width = 0;
    int     height = 0;
    if (!checkNameExtension(args[0], ".esm"))
    {
      // DDS input files
      int     pixelFormat = 0;
      inFile = new DDSInputFile(args[0], width, height, pixelFormat, hdrBuf);
      if (pixelFormat == DDSInputFile::pixelFormatA16 ||
          pixelFormat == DDSInputFile::pixelFormatGRAY16 ||
          pixelFormat == DDSInputFile::pixelFormatRGBA16 ||
          pixelFormat == (DDSInputFile::pixelFormatUnknown + 16))
      {
        isFO76 = true;
      }
      else if (!(pixelFormat == DDSInputFile::pixelFormatA32 ||
                 pixelFormat == DDSInputFile::pixelFormatRGBA32 ||
                 pixelFormat == (DDSInputFile::pixelFormatUnknown + 32)))
      {
        throw errorMessage("invalid input file pixel format");
      }
      int     tmpWidth = 0;
      int     tmpHeight = 0;
      int     tmpPixelFormat = 0;
      txtSetFile = new DDSInputFile(args[2],
                                    tmpWidth, tmpHeight, tmpPixelFormat);
      if (!(tmpPixelFormat == DDSInputFile::pixelFormatR8 ||
            tmpPixelFormat == DDSInputFile::pixelFormatGRAY8))
      {
        throw errorMessage("invalid texture set file pixel format");
      }
      while (!((width << 4) == (tmpWidth << txtSetMip) &&
               height == (tmpHeight << txtSetMip)))
      {
        if (++txtSetMip >= 8)
          throw errorMessage("texture set dimensions do not match input file");
      }
      if (vclrFileName && !disableVCLR)
      {
        vclrFile = new DDSInputFile(vclrFileName,
                                    tmpWidth, tmpHeight, tmpPixelFormat);
        if (!((tmpPixelFormat == DDSInputFile::pixelFormatRGBA16 && isFO76) ||
              (tmpPixelFormat == DDSInputFile::pixelFormatRGB24 && !isFO76)))
        {
          throw errorMessage("invalid vertex color file pixel format");
        }
        while (fo76VClrMip < 4 &&
               !(width == (tmpWidth << fo76VClrMip) &&
                 height == (tmpHeight << fo76VClrMip)))
        {
          fo76VClrMip++;
        }
        if (fo76VClrMip >= (unsigned char) (!isFO76 ? 1 : 4))
          throw errorMessage("vertex color dimensions do not match input file");
      }
      if (gcvrFileName && !disableGCVR)
      {
        gcvrFile = new DDSInputFile(gcvrFileName,
                                    tmpWidth, tmpHeight, tmpPixelFormat);
        if (tmpPixelFormat != DDSInputFile::pixelFormatA8)
          throw errorMessage("invalid ground cover file pixel format");
        if (tmpWidth != width || tmpHeight != height)
          throw errorMessage("ground cover dimensions do not match input file");
      }
      loadTextures(landTextures, args[3], (LandscapeData *) 0, verboseMode,
                   int(mipLevel), ba2File);
    }
    else
    {
      // ESM/BTD input file(s)
      esmFile = new ESMFile(args[0]);
      unsigned int  formatMask = 0x1A;
      if (disableVCLR)
        formatMask &= ~8U;
      if (disableGCVR)
        formatMask &= ~16U;
      landData = new LandscapeData(esmFile, btdFileName, ba2File,
                                   formatMask, worldFormID, defTxtID,
                                   btdLOD, xMin, yMin, xMax, yMax);
      isFO76 = (btdFileName && *btdFileName);
      while ((2 << txtSetMip) < landData->getCellResolution())
        txtSetMip++;
      while ((32 << fo76VClrMip) < landData->getCellResolution())
        fo76VClrMip++;
      width = landData->getImageWidth();
      height = landData->getImageHeight();
      hdrBuf[0] = (!isFO76 ? 0x5F344F46U : 0x36374F46U);        // "FO4_"/"FO76"
      hdrBuf[1] = 0x444E414CU;                                  // "LAND"
      hdrBuf[2] = (unsigned int) landData->getXMin();
      hdrBuf[3] = (unsigned int) landData->getYMin();
      hdrBuf[4] = (unsigned int) roundFloat(landData->getZMin());
      hdrBuf[5] = (unsigned int) landData->getXMax();
      hdrBuf[6] = (unsigned int) landData->getYMax();
      hdrBuf[7] = (unsigned int) roundFloat(landData->getZMax());
      hdrBuf[8] = (unsigned int) roundFloat(landData->getWaterLevel());
      hdrBuf[9] = (unsigned int) landData->getCellResolution();
      hdrBuf[10] = 0U;
      loadTextures(landTextures, (char *) 0, landData, verboseMode,
                   int(mipLevel), ba2File);
    }
    mipLevel = mipLevel - float(int(mipLevel));

    width = width << xyScale;
    height = height << xyScale;
    hdrBuf[9] = hdrBuf[9] << xyScale;
    DDSOutputFile outFile(args[1],
                          width, height, DDSInputFile::pixelFormatRGB24,
                          hdrBuf, 0);
    float   txtScale = 1.0f;
    bool    integerMip = (float(int(mipLevel)) == mipLevel);
    if (!integerMip)
      txtScale = float(std::pow(2.0, mipLevel - float(int(mipLevel))));
    std::vector< RenderThread > threadData(threads.size());
    for (size_t i = 0; i < threadData.size(); i++)
    {
      RenderThread& t = threadData[i];
      if (!landData)
      {
        t.txtSetData = txtSetFile->getDataPtr();
        t.ltexData32 = (unsigned char *) 0;
        t.vclrData24 = (unsigned char *) 0;
        t.ltexData16 = (unsigned char *) 0;
        t.vclrData16 = (unsigned char *) 0;
        t.gcvrData = (unsigned char *) 0;
        if (!isFO76)
        {
          t.ltexData32 = inFile->getDataPtr();
          if (vclrFile)
            t.vclrData24 = vclrFile->getDataPtr();
        }
        else
        {
          t.ltexData16 = inFile->getDataPtr();
          if (vclrFile)
            t.vclrData16 = vclrFile->getDataPtr();
          if (gcvrFile)
            t.gcvrData = gcvrFile->getDataPtr();
        }
      }
      else
      {
        t.txtSetData = landData->getCellTextureSets();
        t.ltexData32 = reinterpret_cast< const unsigned char * >(
                           landData->getLandTexture32());
        t.vclrData24 = landData->getVertexColor24();
        t.ltexData16 = reinterpret_cast< const unsigned char * >(
                           landData->getLandTexture16());
        t.vclrData16 = reinterpret_cast< const unsigned char * >(
                           landData->getVertexColor16());
        t.gcvrData = landData->getGroundCover();
      }
      t.landTextures = &(landTextures.front());
      t.landTextureCnt = landTextures.size();
      t.mipLevel = mipLevel;
      t.rgbScale = rgbScale;
      t.defaultColor = defaultColor;
      t.width = width;
      t.height = height;
      t.xyScale = xyScale;
      t.txtScale = txtScale;
      t.integerMip = integerMip;
      t.isFO76 = isFO76;
      t.txtSetMip = txtSetMip;
      t.fo76VClrMip = fo76VClrMip;
    }
    for (int y = 0; y < height; )
    {
      for (size_t i = 0; i < threads.size() && y < height; i++)
      {
        int     nextY = y + 16;
        if (nextY > height)
          nextY = height;
        threads[i] = new std::thread(RenderThread::runThread,
                                     &(threadData.front()) + i, y, nextY);
        y = nextY;
      }
      for (size_t i = 0; i < threads.size() && threads[i]; i++)
      {
        threads[i]->join();
        delete threads[i];
        threads[i] = (std::thread *) 0;
        outFile.writeData(&(threadData[i].outBuf.front()),
                          sizeof(unsigned char) * threadData[i].outBuf.size());
      }
    }
    err = 0;
  }
  catch (std::exception& e)
  {
    std::fprintf(stderr, "landtxt: %s\n", e.what());
    err = 1;
  }
  for (size_t i = 0; i < threads.size(); i++)
  {
    if (threads[i])
    {
      threads[i]->join();
      delete threads[i];
    }
  }
  for (size_t i = 0; i < landTextures.size(); i++)
  {
    if (landTextures[i])
      delete landTextures[i];
  }
  if (landData)
    delete landData;
  if (esmFile)
    delete esmFile;
  if (gcvrFile)
    delete gcvrFile;
  if (vclrFile)
    delete vclrFile;
  if (txtSetFile)
    delete txtSetFile;
  if (inFile)
    delete inFile;
  if (ba2File)
    delete ba2File;
  return err;
}

