
#include "common.hpp"
#include "filebuf.hpp"

#include <thread>

static int          landWidth = 0;
static int          landHeight = 0;
static unsigned int renderWidth = 0;
static unsigned int renderHeight = 0;
static unsigned int imageWidth = 0;
static unsigned int imageHeight = 0;
static int          renderMode = 0;     // 0: SE, 1: SW, 2: NW, 3: NE, 4: 2D
static int          xOffset = 0;
static int          yOffset = 0;
static int          renderScale = 8;
static unsigned int heightScale = 9728;
static unsigned int waterLevel = 0;
static unsigned int waterColor = 0x60004080U;
static int          lightOffsX = 256;
static int          lightOffsY = 256;

static unsigned int   textureWidth = 0;
static unsigned int   textureHeight = 0;
static unsigned char  textureScale = 0;
static bool           ltexRGBFormat = false;
static unsigned long long ltexPalette[256];

static const unsigned char  *hmapBuf = (unsigned char *) 0;
static const unsigned char  *ltexBuf = (unsigned char *) 0;
static const unsigned char  *wmapBuf = (unsigned char *) 0;
static std::vector< unsigned short >  zDiffColorMult;
static std::vector< unsigned char >   outBuf;

static void loadLTexPalette(const char *fileName)
{
  for (size_t i = 0; i < 256; i++)
    ltexPalette[i] = 0x0000010000100001ULL * (unsigned int) i;
  if (!fileName)
    return;
  FileBuffer    inFile(fileName);
  unsigned long long  tmp1 = 0x01;
  int     tmp2 = -1;
  const unsigned char *p = inFile.getDataPtr();
  size_t  k = 0;
  for (size_t i = 0; i < inFile.size(); i++)
  {
    char    c = char(p[i]);
    if (c >= '0' && c <= '9')
    {
      if (tmp2 < 0)
        tmp2 = 0;
      else
        tmp2 = tmp2 * 10;
      tmp2 = tmp2 + (c - '0');
    }
    if ((i + 1) >= inFile.size())
      c = '\n';
    if (tmp2 >= 0 && !(c >= '0' && c <= '9'))
    {
      if (tmp1 < 0x1000000000000000ULL)
        tmp1 = (tmp1 << 20) | (unsigned int) tmp2;
      tmp2 = -1;
    }
    if (c == '\n')
    {
      if (tmp1 >= 0x1000000000000000ULL && k < 256)
      {
        ltexPalette[k] = tmp1 & 0x0FFFFFFFFFFFFFFFULL;
        k++;
      }
      tmp1 = 0x01;
    }
  }
}

static inline bool checkLandXY(int x, int y)
{
  return ((unsigned int) x < (unsigned int) (landWidth << 8) &&
          (unsigned int) y < (unsigned int) (landHeight << 8));
}

static inline unsigned int getVertexHeight(int x, int y,
                                           const unsigned char *buf)
{
  unsigned int  w = (unsigned int) landWidth;
  unsigned int  x0 = (unsigned int) x >> 8;
  unsigned int  x1 = x0 + (unsigned int) ((x0 + 1) < w);
  unsigned int  y0 = (unsigned int) y >> 8;
  unsigned int  y1 = y0 + (unsigned int) ((y0 + 1) < (unsigned int) landHeight);
  unsigned int  z0 = FileBuffer::readUInt16Fast(buf + ((y0 * w + x0) << 1));
  unsigned int  z1 = FileBuffer::readUInt16Fast(buf + ((y0 * w + x1) << 1));
  unsigned int  z2 = FileBuffer::readUInt16Fast(buf + ((y1 * w + x0) << 1));
  unsigned int  z3 = FileBuffer::readUInt16Fast(buf + ((y1 * w + x1) << 1));
  unsigned int  xf = x & 0xFF;
  z0 = (z0 * (256U - xf)) + (z1 * xf);
  z2 = (z2 * (256U - xf)) + (z3 * xf);
  unsigned int  yf = y & 0xFF;
  // return 18-bit height
  return (((z0 * (256U - yf)) + (z2 * yf) + 8192U) >> 14);
}

static inline unsigned long long getLTexRGBPixel(unsigned int x, unsigned int y)
{
  size_t  offs = (y * textureWidth + x) * 3U;
  return ((unsigned long long) ltexBuf[offs]
          | ((unsigned long long) ltexBuf[offs + 1] << 20)
          | ((unsigned long long) ltexBuf[offs + 2] << 40));
}

static inline unsigned long long getVertexColor(int x, int y)
{
  if (!ltexBuf)
    return 0x0000FF000FF000FFULL;
  x = x << textureScale;
  y = y << textureScale;
  unsigned int  x0 = (unsigned int) x >> 8;
  unsigned int  x1 = x0 + (unsigned int) ((x0 + 1) < textureWidth);
  unsigned int  y0 = (unsigned int) y >> 8;
  unsigned int  y1 = y0 + (unsigned int) ((y0 + 1) < textureHeight);
  unsigned long long  c0, c1, c2, c3;
  if (ltexRGBFormat)
  {
    c0 = getLTexRGBPixel(x0, y0);
    c1 = getLTexRGBPixel(x1, y0);
    c2 = getLTexRGBPixel(x0, y1);
    c3 = getLTexRGBPixel(x1, y1);
  }
  else
  {
    c0 = ltexPalette[ltexBuf[y0 * textureWidth + x0]];
    c1 = ltexPalette[ltexBuf[y0 * textureWidth + x1]];
    c2 = ltexPalette[ltexBuf[y1 * textureWidth + x0]];
    c3 = ltexPalette[ltexBuf[y1 * textureWidth + x1]];
  }
  unsigned int  xf = x & 0xFF;
  c0 = ((c0 * (256U - xf)) + (c1 * xf) + 0x0000080000800008ULL)
       & 0x00FFF00FFF00FFF0ULL;
  c2 = ((c2 * (256U - xf)) + (c3 * xf) + 0x0000080000800008ULL)
       & 0x00FFF00FFF00FFF0ULL;
  unsigned int  yf = y & 0xFF;
  return ((((c0 * (256U - yf)) + (c2 * yf) + 0x0080000800008000ULL) >> 16)
          & 0x0000FF000FF000FFULL);
}

static inline void renderPixels(unsigned long long *lineBufRGB,
                                unsigned int y0, unsigned int y1,
                                unsigned long long c)
{
  unsigned char s = (unsigned char) (renderScale + 8);
  unsigned int  f0 = (y0 >> (s - 8)) & 0xFF;
  y0 = y0 >> s;
  unsigned int  f1 = (y1 >> (s - 8)) & 0xFF;
  y1 = y1 >> s;
  if (y0 == y1)
  {
    lineBufRGB[y0] = lineBufRGB[y0] + (c * (f1 - f0));
    return;
  }
  lineBufRGB[y0] = lineBufRGB[y0] + (c * (256U - f0));
  while (++y0 < y1)
    lineBufRGB[y0] = lineBufRGB[y0] + (c << 8);
  lineBufRGB[y0] = lineBufRGB[y0] + (c * f1);
}

void renderLine2D(unsigned long long *lineBufRGB, int x0, int y0)
{
  unsigned int  waterAlpha = (waterColor >> 24) << 1;
  unsigned long long  waterRGB =
      ((unsigned long long) (waterColor & 0x00FF0000) << 24)
      | ((unsigned long long) (waterColor & 0x0000FF00) << 12)
      | (unsigned long long) (waterColor & 0x000000FF);
  waterRGB = waterRGB * ((waterAlpha * zDiffColorMult[65536] + 128U) >> 8)
             + 0x0000800008000080ULL;
  waterAlpha = 256 - waterAlpha;
  int     d = (renderScale >= 7 ? 128 : (1 << renderScale));
  for (int offs = 0; offs < int(renderHeight << 6);
       offs = offs + d, y0 = y0 - d)
  {
    unsigned int  z = 0;
    unsigned int  w = waterLevel;
    unsigned long long  c = 0;
    if (checkLandXY(x0, y0))
    {
      z = getVertexHeight(x0, y0, hmapBuf);
      if (wmapBuf)
        w = getVertexHeight(x0, y0, wmapBuf);
      c = getVertexColor(x0, y0);
    }
    int     zDiff = 0;
    if (checkLandXY(x0 + lightOffsX, y0 + lightOffsY))
      zDiff = int(getVertexHeight(x0 + lightOffsX, y0 + lightOffsY, hmapBuf));
    zDiff = int(z) + 65536 - zDiff;
    zDiff = (zDiff >= 0 ? (zDiff <= 131071 ? zDiff : 131071) : 0);
    c = c * (unsigned int) zDiffColorMult[zDiff];
    c = ((c + 0x0000800008000080ULL) >> 8) & 0x0003FF003FF003FFULL;
    if (z < w)
    {
      unsigned long long  c2 = waterRGB;
      if (checkLandXY(x0 + lightOffsX, y0 + lightOffsY))
      {
        zDiff = int(getVertexHeight(x0 + lightOffsX, y0 + lightOffsY, wmapBuf));
        if (zDiff != int(w) && zDiff > int(waterLevel))
        {
          zDiff = int(w) - zDiff;
          zDiff = (zDiff >= -65536 ? (zDiff <= 65535 ? zDiff : 65535) : -65536);
          c2 = (c2 >> 8) & 0x0003FF003FF003FFULL;
          c2 = c2 * (unsigned int) zDiffColorMult[zDiff + 196608];
          c2 = c2 + 0x0000800008000080ULL;
        }
      }
      c = ((c * waterAlpha + c2) >> 8) & 0x0003FF003FF003FFULL;
    }
    renderPixels(lineBufRGB,
                 (unsigned int) offs << 8, (unsigned int) (offs + d) << 8, c);
  }
}

void renderLineIsometric(unsigned long long *lineBufRGB, int x0, int y0)
{
  unsigned int  waterAlpha = (waterColor >> 24) << 1;
  unsigned long long  waterRGB =
      ((unsigned long long) (waterColor & 0x00FF0000) << 24)
      | ((unsigned long long) (waterColor & 0x0000FF00) << 12)
      | (unsigned long long) (waterColor & 0x000000FF);
  waterRGB = waterRGB * ((waterAlpha * zDiffColorMult[65536] + 128U) >> 8)
             + 0x0000800008000080ULL;
  waterAlpha = 256 - waterAlpha;
  int     d = 4730;
  if (renderScale < 7)
    d = ((d >> (6 - renderScale)) + 1) >> 1;
  int     prvLineOffs = 0;
  bool    wasLand = false;
  for (int offs = -(int((heightScale + 1) << 12));
       prvLineOffs < int(renderHeight << 13); offs = offs + d)
  {
    int     lineOffs = offs;
    int     landOffs = int(((unsigned int) offs + 0x80000040U) >> 7)
                       - 0x01000000;
    int     x = x0 + (!((renderMode + 1) & 2) ? -landOffs : landOffs);
    int     y = y0 + (!(renderMode & 2) ? -landOffs : landOffs);
    bool    isLand = checkLandXY(x, y);
    unsigned int  z = 0;
    unsigned int  w = waterLevel;
    unsigned long long  c = 0;
    if (!isLand)
    {
      if (!wasLand || w == 0)
      {
        prvLineOffs = (lineOffs > prvLineOffs ? lineOffs : prvLineOffs);
        wasLand = isLand;
        continue;
      }
      lineOffs += int((w * heightScale + 0x040000) >> 6);
    }
    else
    {
      z = getVertexHeight(x, y, hmapBuf);
      if (wmapBuf)
        w = getVertexHeight(x, y, wmapBuf);
      c = getVertexColor(x, y);
      lineOffs += int((z * heightScale + 0x040000) >> 6);
      if (!wasLand && lineOffs > d)
        prvLineOffs = lineOffs - d;
    }
    if (lineOffs <= prvLineOffs)
    {
      wasLand = isLand;
      continue;
    }
    int     zDiff = 0;
    if (checkLandXY(x + lightOffsX, y + lightOffsY))
      zDiff = int(getVertexHeight(x + lightOffsX, y + lightOffsY, hmapBuf));
    zDiff = int(z) + 65536 - zDiff;
    zDiff = (zDiff >= 0 ? (zDiff <= 131071 ? zDiff : 131071) : 0);
    c = c * (unsigned int) zDiffColorMult[zDiff];
    c = ((c + 0x0000800008000080ULL) >> 8) & 0x0003FF003FF003FFULL;
    if (z < w)
    {
      unsigned long long  c2 = waterRGB;
      if (checkLandXY(x + lightOffsX, y + lightOffsY))
      {
        zDiff = int(getVertexHeight(x + lightOffsX, y + lightOffsY, wmapBuf));
        if (zDiff != int(w) && zDiff > int(waterLevel))
        {
          zDiff = int(w) - zDiff;
          zDiff = (zDiff >= -65536 ? (zDiff <= 65535 ? zDiff : 65535) : -65536);
          c2 = (c2 >> 8) & 0x0003FF003FF003FFULL;
          c2 = c2 * (unsigned int) zDiffColorMult[zDiff + 196608];
          c2 = c2 + 0x0000800008000080ULL;
        }
      }
      c = ((c * waterAlpha + c2) >> 8) & 0x0003FF003FF003FFULL;
    }
    if (lineOffs > int(renderHeight << 13))
      lineOffs = int(renderHeight << 13);
    renderPixels(lineBufRGB,
                 (unsigned int) prvLineOffs << 1, (unsigned int) lineOffs << 1,
                 c);
    prvLineOffs = lineOffs;
    wasLand = isLand;
  }
}

void renderThread(size_t xMin, size_t xMax)
{
  std::vector< unsigned long long > lineBufRGB(imageHeight + 1, 0);
  int     x0 = xOffset * 256 + (landWidth << 7);
  int     y0 = yOffset * 256 + (landHeight << 7);
  if (renderMode == 4)
  {
    int     offs = (1 << renderScale) >> (textureScale + 2);
    x0 = x0 - int(renderWidth << 5) - offs;
    y0 = y0 + int(renderHeight << 5) + offs;
  }
  else
  {
    int     w = int((renderWidth * 2365U + 64) >> 7);
    int     h = int(renderHeight << 5);
    x0 = x0 + (!((renderMode + 1) & 2) ? h : -h) + (!(renderMode & 2) ? -w : w);
    y0 = y0 + (!(renderMode & 2) ? h : -h) + (!((renderMode + 1) & 2) ? w : -w);
  }
  for (size_t x = xMin; x < xMax; )
  {
    for (size_t i = 0; i < 2; i++, x = x + size_t(1 << (renderScale - 1)))
    {
      if (renderMode == 4)
      {
        renderLine2D(&(lineBufRGB.front()), x0 + int(x), y0);
      }
      else
      {
        int     offs = int(((unsigned int) x * 2365ULL + 2048U) >> 12);
        renderLineIsometric(&(lineBufRGB.front()),
                            x0 + (!(renderMode & 2) ? offs : -offs),
                            y0 + (!((renderMode + 1) & 2) ? -offs : offs));
      }
    }
    for (size_t i = 0; i < imageHeight; i++)
    {
      unsigned long long  c = lineBufRGB[i];
      lineBufRGB[i] = 0;
      unsigned int  r = (((unsigned int) (c >> 48) & 0x0FFF) + 1) >> 1;
      unsigned int  g = (((unsigned int) (c >> 28) & 0x0FFF) + 1) >> 1;
      unsigned int  b = (((unsigned int) (c >> 8) & 0x0FFF) + 1) >> 1;
      unsigned char *p =
          &(outBuf.front()) + (((imageHeight - (i + 1)) * imageWidth
                                + ((x - 1) >> renderScale)) * 3);
      p[0] = (unsigned char) (b < 255 ? b : 255);
      p[1] = (unsigned char) (g < 255 ? g : 255);
      p[2] = (unsigned char) (r < 255 ? r : 255);
    }
  }
}

int main(int argc, char **argv)
{
  int     lightMultL = 88;
  int     lightMultZ = 100;
  int     lightPow = 35;
  int     threadCnt = int(std::thread::hardware_concurrency());
  threadCnt = (threadCnt > 1 ? (threadCnt < 256 ? threadCnt : 256) : 1);
  if (argc < 5)
  {
    std::fprintf(stderr,
                 "Usage: %s HMAPFILE.DDS OUTFILE.DDS W H [OPTIONS...]\n",
                 argv[0]);
    std::fprintf(stderr, "Options:\n");
    std::fprintf(stderr, "  -iso[_se|_sw|_nw|_ne] | -2d\n");
    std::fprintf(stderr, "  -ltex FILENAME\n");
    std::fprintf(stderr, "  -ltxpal FILENAME\n");
    std::fprintf(stderr, "  -lod INT (%d)\n", renderScale - 8);
    std::fprintf(stderr, "  -xoffs INT (%d)\n", xOffset);
    std::fprintf(stderr, "  -yoffs INT (%d)\n", yOffset);
    std::fprintf(stderr,
                 "  -zrange INT (ZMAX - ZMIN, multiplied by 4 for FO76) (%u)\n",
                 heightScale << 4);
    std::fprintf(stderr, "  -waterlevel UINT16 (%u)\n", waterLevel >> 2);
    std::fprintf(stderr, "  -watercolor UINT32_A7R8G8B8 (0x%08X)\n",
                 waterColor);
    std::fprintf(stderr, "  -wmap FILENAME\n");
    std::fprintf(stderr,
                 "  -light[_nw|_w|_sw|_s|_se|_e|_ne|_n] "
                 "LMULTx100 ZMULTx100 POWx100 (%d %d %d)\n",
                 lightMultL, lightMultZ, lightPow);
    std::fprintf(stderr, "  -threads INT (%d)\n", threadCnt);
    return 1;
  }
  DDSInputFile  *ltexFile = (DDSInputFile *) 0;
  DDSInputFile  *wmapFile = (DDSInputFile *) 0;
  const char    *ltexFileName = (char *) 0;
  const char    *ltexPalFileName = (char *) 0;
  const char    *wmapFileName = (char *) 0;
  try
  {
    imageWidth = (unsigned int) parseInteger(argv[3], 0, (char *) 0, 1, 32768);
    imageHeight = (unsigned int) parseInteger(argv[4], 0, (char *) 0, 1, 32768);
    unsigned int  hdrBuf[11];
    int           pixelFormat = 0;
    DDSInputFile  hmapFile(argv[1], landWidth, landHeight, pixelFormat, hdrBuf);
    if (pixelFormat != DDSInputFile::pixelFormatGRAY16)
      errorMessage("invalid height map file pixel format, must be L16");
    // "FO", "LAND"
    if ((hdrBuf[0] & 0xFFFF) == 0x4F46 && hdrBuf[1] == 0x444E414C)
    {
      int     zMin = uint32ToSigned(hdrBuf[4]);
      int     zMax = uint32ToSigned(hdrBuf[7]);
      int     cellSize = int(hdrBuf[9]);
      heightScale = (unsigned int) ((zMax - zMin) * cellSize + 256) >> 9;
      int     wLvl = uint32ToSigned(hdrBuf[8]);
      if (wLvl > zMin)
      {
        waterLevel = (unsigned int) roundFloat(float(wLvl - zMin) * 262140.0f
                                               / float(zMax - zMin));
      }
    }
    hmapBuf = hmapFile.getDataPtr();

    for (int i = 5; i < argc; i++)
    {
      if (std::strcmp(argv[i], "-iso") == 0 ||
          std::strcmp(argv[i], "-iso_se") == 0)
      {
        renderMode = 0;
      }
      else if (std::strcmp(argv[i], "-iso_sw") == 0)
      {
        renderMode = 1;
      }
      else if (std::strcmp(argv[i], "-iso_nw") == 0)
      {
        renderMode = 2;
      }
      else if (std::strcmp(argv[i], "-iso_ne") == 0)
      {
        renderMode = 3;
      }
      else if (std::strcmp(argv[i], "-2d") == 0)
      {
        renderMode = 4;
      }
      else if (std::strcmp(argv[i], "-ltex") == 0)
      {
        if (++i >= argc)
          errorMessage("missing file name");
        ltexFileName = argv[i];
      }
      else if (std::strcmp(argv[i], "-ltxpal") == 0)
      {
        if (++i >= argc)
          errorMessage("missing file name");
        ltexPalFileName = argv[i];
      }
      else if (std::strcmp(argv[i], "-lod") == 0)
      {
        if (++i >= argc)
          errorMessage("missing integer argument");
        renderScale = int(parseInteger(argv[i], 0, (char *) 0, -5, 4)) + 8;
      }
      else if (std::strcmp(argv[i], "-xoffs") == 0)
      {
        if (++i >= argc)
          errorMessage("missing integer argument");
        xOffset = int(parseInteger(argv[i], 0, "invalid X offset",
                                   1 - landWidth, landWidth - 1));
      }
      else if (std::strcmp(argv[i], "-yoffs") == 0)
      {
        if (++i >= argc)
          errorMessage("missing integer argument");
        yOffset = int(parseInteger(argv[i], 0, "invalid Y offset",
                                   1 - landHeight, landHeight - 1));
      }
      else if (std::strcmp(argv[i], "-zrange") == 0)
      {
        if (++i >= argc)
          errorMessage("missing integer argument");
        unsigned int  tmp =
            (unsigned int) parseInteger(argv[i], 0, "invalid Z range",
                                        0, 1000000);
        heightScale = (tmp + 8U) >> 4;
      }
      else if (std::strcmp(argv[i], "-waterlevel") == 0)
      {
        if (++i >= argc)
          errorMessage("missing integer argument");
        waterLevel =
            (unsigned int) parseInteger(argv[i], 0, "invalid water level",
                                        0, 65536) << 2;
      }
      else if (std::strcmp(argv[i], "-watercolor") == 0)
      {
        if (++i >= argc)
          errorMessage("missing integer argument");
        waterColor =
            (unsigned int) parseInteger(argv[i], 0, "invalid water color",
                                        0, 0x7FFFFFFF);
      }
      else if (std::strcmp(argv[i], "-wmap") == 0)
      {
        if (++i >= argc)
          errorMessage("missing file name");
        wmapFileName = argv[i];
      }
      else if (std::strncmp(argv[i], "-light", 6) == 0)
      {
        lightOffsX = 0;
        lightOffsY = 0;
        for (const char *p = argv[i] + 6; *p != '\0'; p++)
        {
          if (*p == 'N' || *p == 'n')
            lightOffsY = 256;
          else if (*p == 'W' || *p == 'w')
            lightOffsX = 256;
          else if (*p == 'S' || *p == 's')
            lightOffsY = -256;
          else if (*p == 'E' || *p == 'e')
            lightOffsX = -256;
          else if (*p != '_')
            errorMessage("invalid light direction");
        }
        if (lightOffsX == 0 && lightOffsY == 0)
        {
          lightOffsX = 256;
          lightOffsY = 256;
        }
        if (++i >= argc)
          errorMessage("missing integer argument");
        lightMultL = int(parseInteger(argv[i], 0, (char *) 0, 25, 400));
        if (++i >= argc)
          errorMessage("missing integer argument");
        lightMultZ = int(parseInteger(argv[i], 0, (char *) 0, -4096, 4096));
        if (++i >= argc)
          errorMessage("missing integer argument");
        lightPow = int(parseInteger(argv[i], 0, (char *) 0, 6, 100));
      }
      else if (std::strcmp(argv[i], "-threads") == 0)
      {
        if (++i >= argc)
          errorMessage("missing integer argument");
        threadCnt =
            int(parseInteger(argv[i], 0, "invalid thread count", 1, 64));
      }
      else
      {
        throw FO76UtilsError("invalid command line option: \"%s\"", argv[i]);
      }
    }

    if (ltexFileName)
    {
      int     tmpWidth = 0;
      int     tmpHeight = 0;
      int     tmpPixelFormat = 0;
      ltexFile = new DDSInputFile(ltexFileName,
                                  tmpWidth, tmpHeight, tmpPixelFormat);
      textureScale = 0;
      while ((landWidth << textureScale) < tmpWidth && textureScale < 8)
        textureScale++;
      textureWidth = (unsigned int) landWidth << textureScale;
      textureHeight = (unsigned int) landHeight << textureScale;
      if (tmpWidth != int(textureWidth) || tmpHeight != int(textureHeight))
        errorMessage("land texture dimensions do not match input file");
      if (tmpPixelFormat == DDSInputFile::pixelFormatRGB24)
      {
        ltexRGBFormat = true;
      }
      else if (tmpPixelFormat == DDSInputFile::pixelFormatGRAY8 ||
               tmpPixelFormat == (DDSInputFile::pixelFormatUnknown + 8))
      {
        ltexRGBFormat = false;
      }
      else
      {
        errorMessage("invalid land texture file pixel format");
      }
      ltexBuf = ltexFile->getDataPtr();
    }
    loadLTexPalette(ltexPalFileName);
    if (wmapFileName)
    {
      int     tmpWidth = 0;
      int     tmpHeight = 0;
      int     tmpPixelFormat = 0;
      wmapFile = new DDSInputFile(wmapFileName,
                                  tmpWidth, tmpHeight, tmpPixelFormat);
      if (tmpWidth != landWidth || tmpHeight != landHeight)
        errorMessage("water height map dimensions do not match input file");
      if (tmpPixelFormat != DDSInputFile::pixelFormatGRAY16)
        errorMessage("invalid water height map file pixel format");
      wmapBuf = wmapFile->getDataPtr();
    }
    renderWidth = (imageWidth << renderScale) >> 6;
    renderHeight = (imageHeight << renderScale) >> 6;
    if (((renderWidth << 6) >> renderScale) != imageWidth ||
        ((renderHeight << 6) >> renderScale) != imageHeight)
    {
      errorMessage("invalid output image dimensions");
    }

    zDiffColorMult.resize(262144);
    for (int i = 0; i < 131072; i++)
    {
      double  x = double(i - 65536) * double(lightMultZ * int(heightScale));
      if (lightOffsX == 0 || lightOffsY == 0)
        x = x * (1.0 / 11863283.2);
      else
        x = x * (1.0 / 16777216.0);
      if (x < 0.0)
        x = (x * 2.0 - 1.0) / (x - 1.0);
      else
        x = 1.0 / (x + 1.0);
      x = std::pow(x, double(lightPow) / 100.0);
      int     tmp = int(x * 256.0 * (double(lightMultL) / 100.0) + 0.5);
      zDiffColorMult[i] = (unsigned short) (tmp < 1023 ? tmp : 1023);
      tmp = int(x * 256.0 + 0.5);
      zDiffColorMult[i + 131072] = (unsigned short) (tmp < 1023 ? tmp : 1023);
    }
    outBuf.resize(size_t(imageWidth) * imageHeight * 3);

    if (threadCnt > int(imageWidth))
      threadCnt = int(imageWidth);
    std::vector< std::thread * >  threads(size_t(threadCnt), (std::thread *) 0);
    try
    {
      size_t  prvX = 0;
      for (size_t i = 0; i < threads.size(); i++)
      {
        size_t  x = ((i + 1) * imageWidth / threads.size()) << renderScale;
        threads[i] = new std::thread(renderThread, prvX, x);
        prvX = x;
      }
    }
    catch (...)
    {
      for (size_t i = 0; i < threads.size(); i++)
      {
        if (threads[i])
        {
          threads[i]->join();
          delete threads[i];
        }
      }
      throw;
    }
    for (size_t i = 0; i < threads.size(); i++)
    {
      threads[i]->join();
      delete threads[i];
    }

    DDSOutputFile outFile(argv[2], int(imageWidth), int(imageHeight),
                          DDSInputFile::pixelFormatRGB24,
                          (unsigned int *) 0, 0);
    outFile.writeData(&(outBuf.front()), sizeof(unsigned char) * outBuf.size());
    if (ltexFile)
      delete ltexFile;
    if (wmapFile)
      delete wmapFile;
  }
  catch (std::exception& e)
  {
    if (ltexFile)
      delete ltexFile;
    if (wmapFile)
      delete wmapFile;
    std::fprintf(stderr, "%s: %s\n", argv[0], e.what());
    return 1;
  }
  return 0;
}

