
#include "common.hpp"
#include "filebuf.hpp"
#include "btdfile.hpp"

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
    std::fprintf(stderr, "    FMT = 1: dithered 8-bit height map\n");
    std::fprintf(stderr, "    FMT = 2: raw landscape textures "
                         "(4 bytes / vertex)\n");
    std::fprintf(stderr, "    FMT = 3: dithered 8-bit landscape texture\n");
    std::fprintf(stderr, "    FMT = 4: raw ground cover (2 bytes / vertex)\n");
    std::fprintf(stderr, "    FMT = 5: dithered 8-bit ground cover\n");
    std::fprintf(stderr, "    FMT = 6: raw 16-bit terrain color\n");
    std::fprintf(stderr, "    FMT = 7: terrain color in 24-bit RGB format\n");
    std::fprintf(stderr, "Using a palette file changes formats 3 and 5 "
                         "from dithered 8-bit to RGB\n");
    return 1;
  }
  try
  {
    BTDFile btdFile(argv[1]);
    int     outFmt =
        int(parseInteger(argv[3], 10, "invalid output format", 0, 7));
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
    if (outFmt == 2 || outFmt == 3)
    {
      for (size_t i = 0; i < btdFile.getLandTextureCount(); i++)
      {
        unsigned int  n = btdFile.getLandTexture(i);
        std::printf("Land texture %d: 0x%08X\n", int(i), n);
      }
    }
    unsigned int  gcvrOffset = 0;
    if (outFmt == 4 || outFmt == 5)
    {
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
                                       (outFmt < 6 ? 0 : 2),
                                       (outFmt >= 2 && outFmt <= 5 ? 3 : 4));
    }
    unsigned char m = 7 - l;
    std::vector< unsigned char >  outBuf;
    std::vector< unsigned long long > ltexPalette;
    if (argc > 9 && (outFmt == 3 || outFmt == 5))
    {
      outFmt = (outFmt >> 1) + 7;       // 8: LTEX RGB, 9: GCVR RGB
      ltexPalette.resize(256);
      loadPalette(&(ltexPalette.front()), argv[9]);
    }
    std::vector< unsigned short > heightBuf;
    std::vector< unsigned int >   ltexBuf;
    std::vector< unsigned short > gcvrBuf;
    std::vector< unsigned short > vclrBuf;
    static const unsigned int outFmtDataSizes[10] =
    {
      2, 1, 4, 1, 2, 1, 2, 3, 3, 3
    };
    static const int  outFmtPixelFormats[10] =
    {
      DDSInputFile::pixelFormatGRAY16,  DDSInputFile::pixelFormatGRAY8,
      DDSInputFile::pixelFormatL8A24,   DDSInputFile::pixelFormatGRAY8,
      DDSInputFile::pixelFormatL8A8,    DDSInputFile::pixelFormatGRAY8,
      DDSInputFile::pixelFormatRGBA16,  DDSInputFile::pixelFormatRGB24,
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
    hdrBuf[10] = gcvrOffset;
    DDSOutputFile outFile(argv[2],
                          (xMax + 1 - xMin) << m, (yMax + 1 - yMin) << m,
                          outFmtPixelFormats[outFmt], hdrBuf, 0);
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
        outBuf.resize(w * h * (outFmtDataSizes[outFmt] << (14 - (l << 1))), 0);
      }
      size_t  cellBufSize = 16384 >> (l + l);
      switch (outFmt)
      {
        case 0:
        case 1:
          heightBuf.resize(cellBufSize);
          btdFile.getCellHeightMap(&(heightBuf.front()), x, y, l);
          break;
        case 2:
        case 3:
        case 8:
          ltexBuf.resize(cellBufSize);
          btdFile.getCellLandTexture(&(ltexBuf.front()), x, y, l);
          break;
        case 4:
        case 5:
        case 9:
          gcvrBuf.resize(cellBufSize);
          btdFile.getCellGroundCover(&(gcvrBuf.front()), x, y, l);
          break;
        case 6:
        case 7:
          vclrBuf.resize(cellBufSize);
          btdFile.getCellTerrainColor(&(vclrBuf.front()), x, y, l);
          break;
      }
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
          case 1:               // dithered 8-bit height map
            for (size_t xx = 0; xx < (1U << m); xx++, p++)
            {
              unsigned int  tmp = heightBuf[(yy << m) | xx];
              tmp = (tmp * 0xFF01U) >> 16;
              tmp = blendDithered((unsigned char) (tmp >> 8),
                                  (unsigned char) ((tmp + 255) >> 8),
                                  (unsigned char) (tmp & 0xFF),
                                  int(xx), int(127U - yy));
              p[0] = (unsigned char) tmp;
            }
            break;
          case 2:               // raw land textures
            for (size_t xx = 0; xx < (1U << m); xx++, p = p + 4)
            {
              unsigned int  tmp = ltexBuf[(yy << m) | xx];
              p[0] = (unsigned char) (tmp & 0xFF);
              p[1] = (unsigned char) ((tmp >> 8) & 0xFF);
              p[2] = (unsigned char) ((tmp >> 16) & 0xFF);
              p[3] = (unsigned char) ((tmp >> 24) & 0xFF);
            }
            break;
          case 3:               // dithered 8-bit land texture
            for (size_t xx = 0; xx < (1U << m); xx++, p++)
            {
              const unsigned int  *ltexPtr =
                  &(ltexBuf.front()) + ((yy << m) | (xx & ~7U));
              unsigned int  tmp = ltexPtr[0] & 0xFFU;
              unsigned int  a = ltexBuf[(yy << m) | xx] >> 11;
              for (size_t i = 1; i < 6; i++, a = a >> 3)
              {
                tmp = blendDithered((unsigned char) tmp,
                                    (unsigned char) (ltexPtr[i] & 0xFFU),
                                    (unsigned char) (((a & 7) * 73U) >> 1),
                                    int(xx), int(127U - yy));
              }
              p[0] = (unsigned char) tmp;
            }
            break;
          case 4:               // raw ground cover
            for (size_t xx = 0; xx < (1U << m); xx++, p = p + 2)
            {
              unsigned int  tmp = gcvrBuf[(yy << m) | xx];
              if (!(~tmp & 0xFF))
                p[0] = 0xFF;
              else
                p[0] = (unsigned char) ((tmp + gcvrOffset) & 0xFF);
              p[1] = (unsigned char) ((tmp >> 8) & 0xFF);
            }
            break;
          case 5:               // dithered 8-bit ground cover
            for (size_t xx = 0; xx < (1U << m); xx++, p++)
            {
              const unsigned long long  ditherGCVR = 0x0110028041145AFFULL;
              unsigned int  tmp = 0xFF;
              unsigned char dx = (unsigned char) (xx & 3);
              unsigned char dy = (unsigned char) ((127U - yy) & 1);
              unsigned char n = (dy << 2) | dx;
              for (unsigned int i = 0; i < 8; i++)
              {
                if (gcvrBuf[(yy << m) | xx] & (0x0100U << i))
                {
                  if (ditherGCVR & (1ULL << n))
                    tmp = gcvrBuf[(yy << m) | (xx & ~7U) | i] & 0xFFU;
                  n = n + 8;
                }
              }
              if (tmp != 0xFF)
                tmp = gcvrOffset + tmp;
              p[0] = (unsigned char) tmp;
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
          case 8:               // RGB land texture
            for (size_t xx = 0; xx < (1U << m); xx++, p = p + 3)
            {
              const unsigned int  *ltexPtr =
                  &(ltexBuf.front()) + ((yy << m) | (xx & ~7U));
              unsigned long long  tmp = ltexPalette[ltexPtr[0] & 0xFFU];
              unsigned int  a = ltexBuf[(yy << m) | xx] >> 11;
              for (size_t i = 1; i < 6; i++, a = a >> 3)
              {
                tmp = blendRGB64(tmp, ltexPalette[ltexPtr[i] & 0xFFU],
                                 ((a & 7) * 73U) >> 1);
              }
              tmp = tmp + 0x0000080000800008ULL;
              p[0] = (unsigned char) ((tmp >> 4) & 0xFF);
              p[1] = (unsigned char) ((tmp >> 24) & 0xFF);
              p[2] = (unsigned char) ((tmp >> 44) & 0xFF);
            }
            break;
          case 9:               // RGB ground cover
            for (size_t xx = 0; xx < (1U << m); xx++, p = p + 3)
            {
              unsigned long long  tmp = ltexPalette[0xFF];
              unsigned int  n = 0;
              for (unsigned int i = 0; i < 8; i++)
              {
                if (!(gcvrBuf[(yy << m) | xx] & (0x0100U << i)))
                  continue;
                unsigned int  g = gcvrBuf[(yy << m) | (xx & ~7U) | i];
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

