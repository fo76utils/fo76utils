
#include "common.hpp"
#include "filebuf.hpp"
#include "ddstxt.hpp"

#include <thread>

static bool isDDSFileName(const std::string& fileName)
{
  if (fileName.length() <= 4)
    return false;
  const char  *s = fileName.c_str() + (fileName.length() - 4);
  return (s[0] == '.' && (s[1] == 'D' || s[1] == 'd') &&
          (s[2] == 'D' || s[2] == 'd') && (s[3] == 'S' || s[3] == 's'));
}

static void loadTextures(std::vector< DDSTexture * >& textures,
                         const char *listFileName, bool verboseMode)
{
  std::vector< std::string >  fileNames;
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
        if (!isDDSFileName(s))
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
  if (fileNames.size() < 1)
    throw errorMessage("texture file list is empty");
  textures.resize(fileNames.size(), (DDSTexture *) 0);
  for (size_t i = 0; i < fileNames.size(); i++)
  {
    if (verboseMode)
    {
      std::fprintf(stderr, "\rLoading texture %3d: %-58s",
                   int(i), fileNames[i].c_str());
    }
    try
    {
      textures[i] = new DDSTexture(fileNames[i].c_str());
    }
    catch (...)
    {
      if (isDDSFileName(fileNames[i]))
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
  const DDSInputFile  *inFile;
  const DDSInputFile  *vclrFile;
  const DDSInputFile  *gcvrFile;
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
  static inline unsigned long long rgb24ToRGB64(unsigned int c)
  {
    return ((unsigned long long) (c & 0x000000FF)
            | ((unsigned long long) (c & 0x0000FF00) << 12)
            | ((unsigned long long) (c & 0x00FF0000) << 24));
  }
  unsigned int renderPixel(int x, int y, int txtX, int txtY);
  void renderLines(int y0, int y1);
  static void runThread(RenderThread *p, int y0, int y1);
};

unsigned int RenderThread::renderPixel(int x, int y, int txtX, int txtY)
{
  x = (x < width ? x : (width - 1)) >> xyScale;
  y = (y < height ? y : (height - 1)) >> xyScale;
  size_t  offs = size_t(y) * size_t(width >> xyScale) + size_t(x);
  const unsigned char *p0 = inFile->getDataPtr() + ((offs >> 3) << 5);
  const unsigned char *p = inFile->getDataPtr() + (offs << 2);
  unsigned int  a =
      ((unsigned int) p[3] << 16) | ((unsigned int) p[2] << 8) | p[1];
  unsigned long long  c = rgb24ToRGB64(defaultColor);
  unsigned char prvTexture = 0xFF;
  if (!integerMip)
  {
    for ( ; a; a = a >> 3, p0 = p0 + 4)
    {
      if (!(a & 7))
        continue;
      unsigned char t = *p0;
      if (t >= landTextureCnt || t == prvTexture || !landTextures[t])
        continue;
      unsigned long long  cTmp =
          rgb24ToRGB64(landTextures[t]->getPixelT(float(txtX) * txtScale,
                                                  float(txtY) * txtScale,
                                                  mipLevel));
      prvTexture = ((a & 7) == 7 ? t : (unsigned char) 0xFF);
      if ((a & 7) != 7)
        cTmp = blendRGB64(c, cTmp, (((a & 7) << 8) + 3) / 7U);
      c = cTmp;
    }
    if (gcvrFile)
    {
      p0 = gcvrFile->getDataPtr() + ((offs >> 3) << 4);
      a = (*gcvrFile)[(offs << 1) + 1];
      for ( ; a; a = a >> 1, p0 = p0 + 2)
      {
        if (!(a & 1))
          continue;
        unsigned char t = *p0;
        if (t >= landTextureCnt || !landTextures[t])
          continue;
        unsigned int  cTmp =
            landTextures[t]->getPixelT(float(txtX) * txtScale,
                                       float(txtY) * txtScale, mipLevel);
        c = blendRGB64(c, rgb24ToRGB64(cTmp), cTmp >> 24);
      }
    }
  }
  else
  {
    for ( ; a; a = a >> 3, p0 = p0 + 4)
    {
      if (!(a & 7))
        continue;
      unsigned char t = *p0;
      if (t >= landTextureCnt || t == prvTexture || !landTextures[t])
        continue;
      unsigned long long  cTmp =
          rgb24ToRGB64(landTextures[t]->getPixelN(txtX, txtY, int(mipLevel)));
      prvTexture = ((a & 7) == 7 ? t : (unsigned char) 0xFF);
      if ((a & 7) != 7)
        cTmp = blendRGB64(c, cTmp, (((a & 7) << 8) + 3) / 7U);
      c = cTmp;
    }
    if (gcvrFile)
    {
      p0 = gcvrFile->getDataPtr() + ((offs >> 3) << 4);
      a = (*gcvrFile)[(offs << 1) + 1];
      for ( ; a; a = a >> 1, p0 = p0 + 2)
      {
        if (!(a & 1))
          continue;
        unsigned char t = *p0;
        if (t >= landTextureCnt || !landTextures[t])
          continue;
        unsigned int  cTmp =
            landTextures[t]->getPixelN(txtX, txtY, int(mipLevel));
        c = blendRGB64(c, rgb24ToRGB64(cTmp), cTmp >> 24);
      }
    }
  }
  c = c * rgbScale;
  unsigned int  r = (unsigned int) (c & 0x000FFFFF);
  unsigned int  g = (unsigned int) ((c >> 20) & 0x000FFFFF);
  unsigned int  b = (unsigned int) ((c >> 40) & 0x000FFFFF);
  if (vclrFile)
  {
    b = (b * (*vclrFile)[offs * 3] + 127U) / 255U;
    g = (g * (*vclrFile)[offs * 3 + 1] + 127U) / 255U;
    r = (r * (*vclrFile)[offs * 3 + 2] + 127U) / 255U;
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

int main(int argc, char **argv)
{
  if (argc < 4)
  {
    std::fprintf(stderr, "Usage: landtxt INFILE.DDS OUTFILE.DDS "
                         "LTEXLIST.TXT [OPTIONS...]\n\n");
    std::fprintf(stderr, "Input file is raw 4 bytes per vertex "
                         "(btddump or fo4land format 2)\n\n");
    std::fprintf(stderr, "Options:\n");
    std::fprintf(stderr, "    -vclr FILENAME.DDS  vertex color file name\n");
    std::fprintf(stderr, "    -gcvr FILENAME.DDS  ground cover file name\n");
    std::fprintf(stderr, "    -mip FLOAT          mip level\n");
    std::fprintf(stderr, "    -mult FLOAT         multiply RGB output "
                         "with the specified value\n");
    std::fprintf(stderr, "    -defclr 0x00RRGGBB  default color "
                         "for untextured areas\n");
    std::fprintf(stderr, "    -scale INT          scale output resolution "
                         "by 2^N\n");
    std::fprintf(stderr, "    -threads INT        set the number of threads "
                         "to use\n");
    std::fprintf(stderr, "    -q                  do not print "
                         "texture file names\n");
    return 1;
  }
  std::vector< DDSTexture * >   landTextures;
  std::vector< std::thread * >  threads;
  DDSInputFile  *vclrFile = (DDSInputFile *) 0;
  DDSInputFile  *gcvrFile = (DDSInputFile *) 0;
  try
  {
    const char    *vclrFileName = (char *) 0;
    const char    *gcvrFileName = (char *) 0;
    float         mipLevel = 0.0f;
    unsigned int  rgbScale = 256;
    unsigned int  defaultColor = 0x003F3F3F;
    int           xyScale = 0;
    int           threadCnt = int(std::thread::hardware_concurrency());
    threadCnt = (threadCnt > 1 ? (threadCnt < 256 ? threadCnt : 256) : 1);
    bool          verboseMode = true;
    for (int i = 4; i < argc; i++)
    {
      if (std::strcmp(argv[i], "-vclr") == 0)
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
      else
      {
        throw errorMessage("invalid command line option: %s", argv[i]);
      }
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

    int     width = 0;
    int     height = 0;
    int     pixelFormat = 0;
    unsigned int  hdrBuf[11];
    DDSInputFile  inFile(argv[1], width, height, pixelFormat, hdrBuf);
    if (pixelFormat != DDSInputFile::pixelFormatL8A24 &&
        pixelFormat != DDSInputFile::pixelFormatRGBA32 &&
        pixelFormat != (DDSInputFile::pixelFormatUnknown + 32))
    {
      throw errorMessage("invalid input file pixel format");
    }
    if (vclrFileName)
    {
      int     tmpWidth = 0;
      int     tmpHeight = 0;
      int     tmpPixelFormat = 0;
      vclrFile = new DDSInputFile(vclrFileName,
                                  tmpWidth, tmpHeight, tmpPixelFormat);
      if (tmpPixelFormat != DDSInputFile::pixelFormatRGB24)
        throw errorMessage("invalid vertex color file pixel format");
      if (tmpWidth != width || tmpHeight != height)
        throw errorMessage("vertex color dimensions do not match input file");
    }
    if (gcvrFileName)
    {
      int     tmpWidth = 0;
      int     tmpHeight = 0;
      int     tmpPixelFormat = 0;
      gcvrFile = new DDSInputFile(gcvrFileName,
                                  tmpWidth, tmpHeight, tmpPixelFormat);
      if (tmpPixelFormat != DDSInputFile::pixelFormatL8A8)
        throw errorMessage("invalid ground cover file pixel format");
      if (tmpWidth != width || tmpHeight != height)
        throw errorMessage("ground cover dimensions do not match input file");
    }
    loadTextures(landTextures, argv[3], verboseMode);

    width = width << xyScale;
    height = height << xyScale;
    hdrBuf[9] = hdrBuf[9] << xyScale;
    DDSOutputFile outFile(argv[2],
                          width, height, DDSInputFile::pixelFormatRGB24,
                          hdrBuf, 0);
    float   txtScale = 1.0f;
    bool    integerMip = (float(int(mipLevel)) == mipLevel);
    if (!integerMip)
      txtScale = float(std::pow(2.0, mipLevel - float(int(mipLevel))));
    std::vector< RenderThread > threadData(threads.size());
    for (size_t i = 0; i < threadData.size(); i++)
    {
      threadData[i].inFile = &inFile;
      threadData[i].vclrFile = vclrFile;
      threadData[i].gcvrFile = gcvrFile;
      threadData[i].landTextures = &(landTextures.front());
      threadData[i].landTextureCnt = landTextures.size();
      threadData[i].mipLevel = mipLevel;
      threadData[i].rgbScale = rgbScale;
      threadData[i].defaultColor = defaultColor;
      threadData[i].width = width;
      threadData[i].height = height;
      threadData[i].xyScale = xyScale;
      threadData[i].txtScale = txtScale;
      threadData[i].integerMip = integerMip;
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

    for (size_t i = 0; i < landTextures.size(); i++)
    {
      if (landTextures[i])
        delete landTextures[i];
    }
    if (vclrFile)
      delete vclrFile;
    if (gcvrFile)
      delete gcvrFile;
  }
  catch (std::exception& e)
  {
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
    if (vclrFile)
      delete vclrFile;
    if (gcvrFile)
      delete gcvrFile;
    std::fprintf(stderr, "landtxt: %s\n", e.what());
    return 1;
  }
  return 0;
}

