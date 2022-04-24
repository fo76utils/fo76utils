
#include "common.hpp"
#include "filebuf.hpp"
#include "btdfile.hpp"

static void calculateNormal(float& normalX, float& normalY, float& normalZ,
                            float dx, float dy, float dz1, float dz2)
{
  // from terrmesh.cpp
  float   tmp1 = 1.0f + (dz1 * dz1);
  float   tmp2 = 1.0f + (dz2 * dz2);
  float   tmp3 = float(std::sqrt(tmp2 / tmp1));
  float   dz = (dz1 * tmp3 + dz2) / (tmp3 + 1.0f);
  normalZ += 0.5f;
  normalX -= (dz * dx);
  normalY -= (dz * dy);
}

void vertexNormals(DDSOutputFile& outFile, BTDFile& btdFile,
                   unsigned char l, int xMin, int yMin, int xMax, int yMax)
{
  btdFile.setTileCacheSize(size_t((((xMax + 1 - xMin) + 7) >> 3) + 1));
  int     cellResolution = 128 >> l;
  unsigned char m = 7 - l;
  int     w = (xMax + 1 - xMin) << m;
  int     h = (yMax + 1 - yMin) << m;
  std::vector< unsigned short > buf((size_t(w) * size_t(h)) << 1, 0);
  std::vector< unsigned short > cellBuf(size_t(16384 >> (l + l)), 0);
  float   zMin = btdFile.getMinHeight();
  float   zScale = (btdFile.getMaxHeight() - zMin) / 65535.0f;
  float   normalScale1 = float(cellResolution) * (1.0f / 4096.0f);
  float   normalScale2 = float(cellResolution) * (0.7071068f / 4096.0f);
  for (int y = 0; y < h; y++)
  {
    if (!y || !((y + (cellResolution >> 1)) & (cellResolution - 1)))
    {
      int     cellY = yMax - ((y >> m) + (!y ? 0 : 1));
      if (cellY >= yMin)
      {
        for (int cellX = xMin; cellX <= xMax; cellX++)
        {
          unsigned short  *srcPtr = &(cellBuf.front());
          btdFile.getCellHeightMap(srcPtr, cellX, cellY, l);
          for (int yc = 0; yc < cellResolution; yc++)
          {
            unsigned short  *dstPtr = &(buf.front());
            dstPtr = dstPtr + (size_t((cellResolution << ((yMax - cellY) & 1))
                                      - (yc + 1)) * size_t(w));
            dstPtr = dstPtr + (size_t(cellX - xMin) << m);
            for (int xc = 0; xc < cellResolution; xc++, srcPtr++, dstPtr++)
              *dstPtr = *srcPtr;
          }
        }
      }
    }
    for (int x = 0; x < w; x++)
    {
      static const int  xOffsTable[9] = { -1, 0, 1, -1, 0, 1, -1, 0, 1 };
      static const int  yOffsTable[9] = { -1, -1, -1, 0, 0, 0, 1, 1, 1 };
      float   z[9];
      for (int i = 0; i < 9; i++)
      {
        int     xc = x + xOffsTable[i];
        int     yc = y + yOffsTable[i];
        xc = (xc > 0 ? (xc < (w - 1) ? xc : (w - 1)) : 0);
        yc = (yc > 0 ? (yc < (h - 1) ? yc : (h - 1)) : 0);
        yc = yc & ((cellResolution << 1) - 1);
        z[i] = float(int(buf[size_t(yc) * size_t(w) + size_t(xc)]))
               * zScale + zMin;
      }
      float   normalX = 0.0f;
      float   normalY = 0.0f;
      float   normalZ = 0.0f;
      calculateNormal(normalX, normalY, normalZ, 1.0f, 0.0f,
                      (z[4] - z[3]) * normalScale1,     // -W
                      (z[5] - z[4]) * normalScale1);    // +E
      calculateNormal(normalX, normalY, normalZ, 0.0f, 1.0f,
                      (z[4] - z[7]) * normalScale1,     // -S
                      (z[1] - z[4]) * normalScale1);    // +N
      if ((x ^ y) & 1)
      {
        //    0 1 2
        // -1 +-+-+
        //    |\|/|
        //  0 +-+-+
        //    |/|\|
        //  1 +-+-+
        calculateNormal(normalX, normalY, normalZ, -0.7071068f, 0.7071068f,
                        (z[4] - z[8]) * normalScale2,   // -SE
                        (z[0] - z[4]) * normalScale2);  // +NW
        calculateNormal(normalX, normalY, normalZ, 0.7071068f, 0.7071068f,
                        (z[4] - z[6]) * normalScale2,   // -SW
                        (z[2] - z[4]) * normalScale2);  // +NE
      }
      float   tmp =
          (normalX * normalX) + (normalY * normalY) + (normalZ * normalZ);
      tmp = 1.0f / float(std::sqrt(tmp));
      int     r = roundFloat(normalX * tmp * 127.5f + 127.5f);
      int     g = roundFloat(normalY * tmp * 127.5f + 127.5f);
      int     b = roundFloat(normalZ * tmp * 127.5f + 127.5f);
      outFile.writeByte((unsigned char) (b > 0 ? (b < 255 ? b : 255) : 0));
      outFile.writeByte((unsigned char) (g > 0 ? (g < 255 ? g : 255) : 0));
      outFile.writeByte((unsigned char) (r > 0 ? (r < 255 ? r : 255) : 0));
    }
  }
}

static void loadPalette(unsigned long long *buf, const char *fileName)
{
  for (size_t i = 0; i < 256; i++)
    buf[i] = 0x0000100001000010ULL * (unsigned int) i;
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
        buf[k] = (tmp1 & 0x0FFFFFFFFFFFFFFFULL) << 4;
        k++;
      }
      tmp1 = 0x01;
    }
  }
}

int main(int argc, char **argv)
{
  if (argc < 8 || argc > 10)
  {
    std::fprintf(stderr, "Usage: btddump INFILE.BTD OUTFILE FMT "
                         "XMIN YMIN XMAX YMAX [LOD [PALETTE.GPL]]\n");
    std::fprintf(stderr, "    FMT = 0: raw 16-bit height map\n");
    std::fprintf(stderr, "    FMT = 1: vertex normals in 24-bit RGB format\n");
    std::fprintf(stderr, "    FMT = 2: raw landscape textures "
                         "(2 bytes / vertex)\n");
    std::fprintf(stderr, "    FMT = 3: dithered 8-bit landscape texture\n");
    std::fprintf(stderr, "    FMT = 4: raw ground cover (1 byte / vertex)\n");
    std::fprintf(stderr, "    FMT = 5: dithered 8-bit ground cover\n");
    std::fprintf(stderr, "    FMT = 6: raw 16-bit terrain color\n");
    std::fprintf(stderr, "    FMT = 7: terrain color in 24-bit RGB format\n");
    std::fprintf(stderr, "    FMT = 8: cell texture sets as 8-bit IDs\n");
    std::fprintf(stderr, "    FMT = 9: cell texture sets as 32-bit form IDs\n");
    std::fprintf(stderr, "Using a palette file changes formats 3 and 5 "
                         "from dithered 8-bit to RGB\n");
    return 1;
  }
  try
  {
    BTDFile btdFile(argv[1]);
    int     outFmt =
        int(parseInteger(argv[3], 10, "invalid output format", 0, 9));
    int     xMin =
        int(parseInteger(argv[4], 10, "invalid X range",
                         btdFile.getCellMinX(), btdFile.getCellMaxX()));
    int     yMin =
        int(parseInteger(argv[5], 10, "invalid Y range",
                         btdFile.getCellMinY(), btdFile.getCellMaxY()));
    int     xMax =
        int(parseInteger(argv[6], 10, "invalid X range",
                         xMin, btdFile.getCellMaxX()));
    int     yMax =
        int(parseInteger(argv[7], 10, "invalid Y range",
                         yMin, btdFile.getCellMaxY()));
    if (outFmt == 0 || outFmt == 1)
    {
      std::printf("Minimum height = %f, maximum height = %f\n",
                  btdFile.getMinHeight(), btdFile.getMaxHeight());
    }
    unsigned int  gcvrOffset = 0;
    if (outFmt == 3 || outFmt == 5 || outFmt == 8)
    {
      for (size_t i = 0; i < btdFile.getLandTextureCount(); i++)
      {
        unsigned int  n = btdFile.getLandTexture(i);
        std::printf("Land texture %d: 0x%08X\n", int(i), n);
      }
      gcvrOffset = (unsigned int) btdFile.getLandTextureCount();
      for (size_t i = 0; i < btdFile.getGroundCoverCount(); i++)
      {
        unsigned int  n = btdFile.getGroundCover(i);
        std::printf("Ground cover %d: 0x%08X\n", int(i + gcvrOffset), n);
      }
    }
    unsigned char l = (outFmt < 6 ? 0 : 2);
    if (argc > 8)
    {
      l = (unsigned char) parseInteger(argv[8], 10, "invalid level of detail",
                                       ((outFmt & 14) != 6 ? 0 : 2), 4);
    }
    if (outFmt == 8 || outFmt == 9)
      l = 6;
    unsigned char m = 7 - l;
    std::vector< unsigned char >  outBuf;
    std::vector< unsigned long long > ltexPalette;
    if (argc > 9 && (outFmt == 3 || outFmt == 5))
    {
      outFmt = (outFmt >> 1) + 9;       // 10: LTEX RGB, 11: GCVR RGB
      ltexPalette.resize(256);
      loadPalette(&(ltexPalette.front()), argv[9]);
    }
    std::vector< unsigned short > heightBuf;
    std::vector< unsigned short > ltexBuf;
    std::vector< unsigned char >  gcvrBuf;
    std::vector< unsigned short > vclrBuf;
    std::vector< unsigned char >  txtSetBuf(64, 0xFF);
    static const unsigned int outFmtDataSizes[12] =
    {
      2, 3, 2, 1, 1, 1, 2, 3, 16, 64, 3, 3
    };
    static const int  outFmtPixelFormats[12] =
    {
      DDSInputFile::pixelFormatGRAY16,  DDSInputFile::pixelFormatRGB24,
      DDSInputFile::pixelFormatA16,     DDSInputFile::pixelFormatGRAY8,
      DDSInputFile::pixelFormatA8,      DDSInputFile::pixelFormatGRAY8,
      DDSInputFile::pixelFormatRGBA16,  DDSInputFile::pixelFormatRGB24,
      DDSInputFile::pixelFormatR8,      DDSInputFile::pixelFormatR32,
      DDSInputFile::pixelFormatRGB24,   DDSInputFile::pixelFormatRGB24
    };
    unsigned int  hdrBuf[11];
    hdrBuf[0] = 0x36374F46;             // "FO76"
    hdrBuf[1] = 0x444E414C;             // "LAND"
    hdrBuf[2] = (unsigned int) xMin;
    hdrBuf[3] = (unsigned int) yMin;
    hdrBuf[4] = (unsigned int) int(btdFile.getMinHeight() - 0.5f);
    hdrBuf[5] = (unsigned int) xMax;
    hdrBuf[6] = (unsigned int) yMax;
    hdrBuf[7] = (unsigned int) int(btdFile.getMaxHeight() + 0.5f);
    hdrBuf[8] = 0;                      // water level
    hdrBuf[9] = 1U << m;
    hdrBuf[10] = (outFmt >= 2 ? gcvrOffset : 1024U);    // default land level
    DDSOutputFile outFile(argv[2],
                          (xMax + 1 - xMin) << ((outFmt & 14) != 8 ? m : 5),
                          (yMax + 1 - yMin) << m,
                          outFmtPixelFormats[outFmt], hdrBuf,
                          (outFmt != 1 ? 0 : 16384));
    if (outFmt == 1)
    {
      vertexNormals(outFile, btdFile, l, xMin, yMin, xMax, yMax);
      return 0;
    }
    int     x0 = xMin;
    int     y0 = yMax;
    int     x = xMin;
    int     y = yMax;
    while (true)
    {
      size_t  w = size_t(xMax + 1 - xMin);
      if (!outBuf.size())
      {
        size_t  h = size_t(((y0 - btdFile.getCellMinY()) & 7) + 1);
        if (h > size_t(y0 + 1 - yMin))
          h = size_t(y0 + 1 - yMin);
        outBuf.resize(w * h * (outFmtDataSizes[outFmt] << (m << 1)), 0);
      }
      size_t  cellBufSize = 16384 >> (l + l);
      switch (outFmt)
      {
        case 0:
          heightBuf.resize(cellBufSize);
          btdFile.getCellHeightMap(&(heightBuf.front()), x, y, l);
          break;
        case 2:
        case 3:
        case 10:
          ltexBuf.resize(cellBufSize);
          btdFile.getCellLandTexture(&(ltexBuf.front()), x, y, l);
          break;
        case 4:
        case 5:
        case 11:
          gcvrBuf.resize(cellBufSize);
          btdFile.getCellGroundCover(&(gcvrBuf.front()), x, y, l);
          break;
        case 6:
        case 7:
          vclrBuf.resize(cellBufSize);
          btdFile.getCellTerrainColor(&(vclrBuf.front()), x, y, l);
          break;
      }
      if (outFmt == 3 || outFmt == 5 || outFmt >= 8)
        btdFile.getCellTextureSet(&(txtSetBuf.front()), x, y);
      for (size_t yy = 0; yy < (1U << m); yy++)
      {
        size_t  offs = size_t((y0 - y) << m) | ((127U >> l) - yy);
        offs = (offs * w + size_t(x - xMin)) << m;
        unsigned char *p = &(outBuf.front()) + (offs * outFmtDataSizes[outFmt]);
        switch (outFmt)
        {
          case 0:               // raw height map
            for (size_t xx = 0; xx < (1U << m); xx++, p = p + 2)
            {
              unsigned int  tmp = heightBuf[(yy << m) | xx];
              p[0] = (unsigned char) (tmp & 0xFF);
              p[1] = (unsigned char) (tmp >> 8);
            }
            break;
          case 2:               // raw land textures
            for (size_t xx = 0; xx < (1U << m); xx++, p = p + 2)
            {
              unsigned int  tmp = ltexBuf[(yy << m) | xx];
              p[0] = (unsigned char) (tmp & 0xFF);
              p[1] = (unsigned char) ((tmp >> 8) & 0xFF);
            }
            break;
          case 3:               // dithered 8-bit land texture
            for (size_t xx = 0; xx < (1U << m); xx++, p++)
            {
              const unsigned char *txtSetPtr =
                  &(txtSetBuf.front())
                  + (((yy >> (m - 1)) << 5) + ((xx >> (m - 1)) << 4));
              unsigned char tmp = txtSetPtr[0];
              unsigned int  a = ltexBuf[(yy << m) | xx];
              for (size_t i = 1; i < 6; i++, a = a >> 3)
              {
                tmp = blendDithered(tmp, txtSetPtr[i],
                                    (unsigned char) (((a & 7) * 73U) >> 1),
                                    int(xx), int(127U - yy));
              }
              p[0] = tmp;
            }
            break;
          case 4:               // raw ground cover
            for (size_t xx = 0; xx < (1U << m); xx++, p++)
              p[0] = gcvrBuf[(yy << m) | xx];
            break;
          case 5:               // dithered 8-bit ground cover
            for (size_t xx = 0; xx < (1U << m); xx++, p++)
            {
              const unsigned long long  ditherGCVR = 0x0110028041145AFFULL;
              const unsigned char *txtSetPtr =
                  &(txtSetBuf.front())
                  + (((yy >> (m - 1)) << 5) + ((xx >> (m - 1)) << 4) + 8);
              unsigned char tmp = 0xFF;
              unsigned char dx = (unsigned char) (xx & 3);
              unsigned char dy = (unsigned char) ((127U - yy) & 1);
              unsigned char n = (dy << 2) | dx;
              for (unsigned int i = 0; i < 8; i++)
              {
                if (gcvrBuf[(yy << m) | xx] & (1U << i))
                {
                  if (ditherGCVR & (1ULL << n))
                    tmp = txtSetPtr[i];
                  n = n + 8;
                }
              }
              if (tmp != 0xFF)
                tmp = (unsigned char) (gcvrOffset + tmp);
              p[0] = tmp;
            }
            break;
          case 6:               // raw vertex colors
            for (size_t xx = 0; xx < (1U << m); xx++, p = p + 2)
            {
              unsigned int  tmp = vclrBuf[(yy << m) | xx];
              p[0] = (unsigned char) (tmp & 0xFF);
              p[1] = (unsigned char) ((tmp >> 8) | 0x80);
            }
            break;
          case 7:               // vertex colors in 24-bit RGB format
            for (size_t xx = 0; xx < (1U << m); xx++, p = p + 3)
            {
              unsigned int  tmp = vclrBuf[(yy << m) | xx];
              p[0] = (unsigned char) (((tmp & 0x001F) * 1053U + 64) >> 7);
              p[1] = (unsigned char) (((tmp & 0x03E0) * 1053U + 2048) >> 12);
              p[2] = (unsigned char) (((tmp & 0x7C00) * 1053U + 65536) >> 17);
            }
            break;
          case 8:               // texture set as 8-bit IDs
            for (size_t xx = 0; xx < (1U << m); xx++, p = p + 16)
            {
              for (size_t i = 0; i < 16; i++)
              {
                unsigned int  tmp = txtSetBuf[(((yy << m) | xx) << 4) | i];
                if (i >= 8 && tmp != 0xFF)
                  tmp += gcvrOffset;
                p[i] = (unsigned char) tmp;
              }
            }
            break;
          case 9:               // texture set as 32-bit form IDs
            for (size_t xx = 0; xx < (1U << m); xx++, p = p + 64)
            {
              for (size_t i = 0; i < 16; i++)
              {
                unsigned int  tmp = txtSetBuf[(((yy << m) | xx) << 4) | i];
                if (tmp == 0xFF)
                  tmp = 0U;
                else if (i < 8)
                  tmp = btdFile.getLandTexture(tmp);
                else
                  tmp = btdFile.getGroundCover(tmp);
                p[i << 2] = (unsigned char) (tmp & 0xFF);
                p[(i << 2) + 1] = (unsigned char) ((tmp >> 8) & 0xFF);
                p[(i << 2) + 2] = (unsigned char) ((tmp >> 16) & 0xFF);
                p[(i << 2) + 3] = (unsigned char) ((tmp >> 24) & 0xFF);
              }
            }
            break;
          case 10:              // RGB land texture
            for (size_t xx = 0; xx < (1U << m); xx++, p = p + 3)
            {
              const unsigned char *txtSetPtr =
                  &(txtSetBuf.front())
                  + (((yy >> (m - 1)) << 5) + ((xx >> (m - 1)) << 4));
              unsigned long long  tmp = ltexPalette[txtSetPtr[0]];
              unsigned int  a = ltexBuf[(yy << m) | xx];
              for (size_t i = 1; i < 6; i++, a = a >> 3)
              {
                tmp = blendRGB64(tmp, ltexPalette[txtSetPtr[i]],
                                 ((a & 7) * 73U) >> 1);
              }
              tmp = tmp + 0x0000080000800008ULL;
              p[0] = (unsigned char) ((tmp >> 4) & 0xFF);
              p[1] = (unsigned char) ((tmp >> 24) & 0xFF);
              p[2] = (unsigned char) ((tmp >> 44) & 0xFF);
            }
            break;
          case 11:              // RGB ground cover
            for (size_t xx = 0; xx < (1U << m); xx++, p = p + 3)
            {
              const unsigned char *txtSetPtr =
                  &(txtSetBuf.front())
                  + (((yy >> (m - 1)) << 5) + ((xx >> (m - 1)) << 4) + 8);
              unsigned long long  tmp = ltexPalette[0xFF];
              unsigned int  n = 0;
              for (unsigned int i = 0; i < 8; i++)
              {
                unsigned int  g = txtSetPtr[i];
                if (g == 0xFF || !(gcvrBuf[(yy << m) | xx] & (1U << i)))
                  continue;
                g = (gcvrOffset + g) & 0xFFU;
                n++;
                tmp = blendRGB64(tmp, ltexPalette[g], (256U + (n >> 1)) / n);
              }
              tmp = tmp + 0x0000080000800008ULL;
              p[0] = (unsigned char) ((tmp >> 4) & 0xFF);
              p[1] = (unsigned char) ((tmp >> 24) & 0xFF);
              p[2] = (unsigned char) ((tmp >> 44) & 0xFF);
            }
            break;
        }
      }
      x++;
      if (x > xMax || ((x - btdFile.getCellMinX()) & 7) == 0)
      {
        y--;
        if (y < yMin || ((y - btdFile.getCellMinY()) & 7) == 7)
        {
          if (x > xMax)
          {
            outFile.writeData(&(outBuf.front()),
                              sizeof(unsigned char) * outBuf.size());
            outBuf.clear();
            x = xMin;
            if (y < yMin)
              break;
            y0 = y;
          }
          y = y0;
          x0 = x;
        }
        x = x0;
      }
    }
  }
  catch (std::exception& e)
  {
    std::fprintf(stderr, "btddump: %s\n", e.what());
    return 1;
  }
  return 0;
}

