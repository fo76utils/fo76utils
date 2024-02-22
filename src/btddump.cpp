
#include "common.hpp"
#include "filebuf.hpp"
#include "ba2file.hpp"
#include "btdfile.hpp"
#include "fp32vec4.hpp"

// from terrmesh.cpp

static inline FloatVector4 calculateNormal(FloatVector4 v1, FloatVector4 v2)
{
  // cross product of v1 and v2
  FloatVector4  tmp((v1[1] * v2[2]) - (v1[2] * v2[1]),
                    (v1[2] * v2[0]) - (v1[0] * v2[2]),
                    (v1[0] * v2[1]) - (v1[1] * v2[0]), 0.0f);
  return tmp.normalize3Fast();
}

void vertexNormals(DDSOutputFile& outFile, BTDFile& btdFile,
                   unsigned char l, int xMin, int yMin, int xMax, int yMax)
{
  btdFile.setTileCacheSize(size_t((((xMax + 1 - xMin) + 7) >> 3) + 1));
  int     cellResolution = 128 >> l;
  unsigned char m = 7 - l;
  int     w = (xMax + 1 - xMin) << m;
  int     h = (yMax + 1 - yMin) << m;
  std::vector< std::uint16_t >  buf((size_t(w) * size_t(h)) << 1, 0);
  std::vector< std::uint16_t >  cellBuf(size_t(16384 >> (l + l)), 0);
  float   zMin = btdFile.getMinHeight();
  float   xyScale = 100.0f / float(cellResolution);
  float   xyScale2 = xyScale * 65535.0f / (btdFile.getMaxHeight() - zMin);
  for (int y = 0; y < h; y++)
  {
    if (!y || !((y + (cellResolution >> 1)) & (cellResolution - 1)))
    {
      int     cellY = yMax - ((y >> m) + (!y ? 0 : 1));
      if (cellY >= yMin)
      {
        for (int cellX = xMin; cellX <= xMax; cellX++)
        {
          std::uint16_t *srcPtr = cellBuf.data();
          btdFile.getCellHeightMap(srcPtr, cellX, cellY, l);
          for (int yc = 0; yc < cellResolution; yc++)
          {
            std::uint16_t *dstPtr = buf.data();
            dstPtr = dstPtr + (size_t((cellResolution << ((yMax - cellY) & 1))
                                      - (yc + 1)) * size_t(w));
            dstPtr = dstPtr + (size_t(cellX - xMin) << m);
            for (int xc = 0; xc < cellResolution; xc++, srcPtr++, dstPtr++)
              *dstPtr = *srcPtr;
          }
        }
      }
    }
    FloatVector4  prvNormalSE(0.0f);
    FloatVector4  prvNormalNE(0.0f);
    for (int x = 0; x < w; x++)
    {
      // 0, N, S, E, NW, SW, SE, NE, W
      static const int  xOffsTable[9] = { 0, 0, 0, 1, -1, -1, 1, 1, -1 };
      static const int  yOffsTable[9] = { 0, -1, 1, 0, -1, 1, 1, -1, 0 };
      float   z[9];
      for (int i = 0; i < 9; i++)
      {
        int     xc = x + xOffsTable[i];
        int     yc = y + yOffsTable[i];
        xc = (xc > 0 ? (xc < (w - 1) ? xc : (w - 1)) : 0);
        yc = (yc > 0 ? (yc < (h - 1) ? yc : (h - 1)) : 0);
        yc = yc & ((cellResolution << 1) - 1);
        z[i] = float(int(buf[size_t(yc) * size_t(w) + size_t(xc)]));
      }
      FloatVector4  v_n(0.0f, xyScale2, z[1] - z[0], 0.0f);
      FloatVector4  v_s(0.0f, -xyScale2, z[2] - z[0], 0.0f);
      FloatVector4  v_e(xyScale2, 0.0f, z[3] - z[0], 0.0f);
      FloatVector4  normal;
      if ((x ^ y) & 1)
      {
        //    0 1 2
        // -1 +-+-+
        //    |\|/|
        //  0 +-+-+
        //    |/|\|
        //  1 +-+-+
        FloatVector4  v_nw(-xyScale2, xyScale2, z[4] - z[0], 0.0f);
        FloatVector4  v_sw(-xyScale2, -xyScale2, z[5] - z[0], 0.0f);
        FloatVector4  v_se(xyScale2, -xyScale2, z[6] - z[0], 0.0f);
        FloatVector4  v_ne(xyScale2, xyScale2, z[7] - z[0], 0.0f);
        normal = calculateNormal(v_ne, v_n);
        normal += calculateNormal(v_n, v_nw);
        if (!x)
        {
          FloatVector4  v_w(-xyScale2, 0.0f, z[8] - z[0], 0.0f);
          prvNormalNE = calculateNormal(v_nw, v_w);
          prvNormalSE = calculateNormal(v_w, v_sw);
        }
        normal += prvNormalNE;
        normal += prvNormalSE;
        normal += calculateNormal(v_sw, v_s);
        normal += calculateNormal(v_s, v_se);
        prvNormalSE = calculateNormal(v_se, v_e);
        normal += prvNormalSE;
        prvNormalNE = calculateNormal(v_e, v_ne);
        normal += prvNormalNE;
      }
      else
      {
        if (!x)
        {
          FloatVector4  v_w(-xyScale2, 0.0f, z[8] - z[0], 0.0f);
          prvNormalNE = calculateNormal(v_n, v_w);
          prvNormalSE = calculateNormal(v_w, v_s);
        }
        normal = prvNormalNE;
        normal += prvNormalSE;
        prvNormalSE = calculateNormal(v_s, v_e);
        normal += prvNormalSE;
        prvNormalNE = calculateNormal(v_e, v_n);
        normal += prvNormalNE;
      }
      normal.normalize();
      normal += 1.0f;
      normal *= 127.5f;
      unsigned int  c = (unsigned int) normal;
      outFile.writeByte((unsigned char) ((c >> 16) & 0xFFU));   // B
      outFile.writeByte((unsigned char) ((c >> 8) & 0xFFU));    // G
      outFile.writeByte((unsigned char) (c & 0xFFU));           // R
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
  const unsigned char *p = inFile.data();
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

static bool archiveFilterFunction(void *p, const std::string& s)
{
  (void) p;
  return s.ends_with(".btd");
}

int main(int argc, char **argv)
{
  if (argc < 5 || argc > 7)
  {
    std::fprintf(stderr, "Usage: btddump INFILE.BTD OUTFILE FMT ARCHIVEPATH "
                         "[LOD [PALETTE.GPL]]\n");
    std::fprintf(stderr, "    FMT = 0: raw 16-bit height map\n");
    std::fprintf(stderr, "    FMT = 1: vertex normals in 24-bit RGB format\n");
    std::fprintf(stderr, "    FMT = 2: raw landscape textures "
                         "(2 bytes / vertex)\n");
    std::fprintf(stderr, "    FMT = 3: dithered 8-bit landscape texture\n");
    std::fprintf(stderr, "    FMT = 8: cell texture sets as 8-bit IDs\n");
    std::fprintf(stderr, "    FMT = 9: cell texture sets as 32-bit form IDs\n");
    std::fprintf(stderr, "Using a palette file changes format 3 "
                         "from dithered 8-bit to RGB\n");
    return 1;
  }
  try
  {
    BA2File ba2File(argv[4], &archiveFilterFunction);
    std::vector< unsigned char >  btdBuf;
    const unsigned char *btdFileData = (unsigned char *) 0;
    size_t  btdFileSize =
        ba2File.extractFile(btdFileData, btdBuf, std::string(argv[1]));
    BTDFile btdFile(btdFileData, btdFileSize);
    int     outFmt =
        int(parseInteger(argv[3], 10, "invalid output format", 0, 9));
    if (outFmt & 4)
      errorMessage("invalid output format");
    int     xMin = btdFile.getCellMinX();
    int     yMin = btdFile.getCellMinY();
    int     xMax = btdFile.getCellMaxX();
    int     yMax = btdFile.getCellMaxY();
    std::printf("Cell range: %d, %d (SW) to %d, %d (NE)\n",
                xMin, yMin, xMax, yMax);
    if (outFmt == 0 || outFmt == 1)
    {
      std::printf("Minimum height = %f, maximum height = %f\n",
                  btdFile.getMinHeight(), btdFile.getMaxHeight());
    }
    if (outFmt == 3 || outFmt == 8)
    {
      for (size_t i = 0; i < btdFile.getLandTextureCount(); i++)
      {
        unsigned int  n = btdFile.getLandTexture(i);
        std::printf("Land texture %d: 0x%08X\n", int(i), n);
      }
    }
    unsigned char l = 0;
    if (argc > 5)
    {
      l = (unsigned char) parseInteger(argv[5], 10, "invalid level of detail",
                                       0, 4);
    }
    if (outFmt == 8 || outFmt == 9)
      l = 6;
    unsigned char m = 7 - l;
    std::vector< unsigned char >  outBuf;
    std::vector< unsigned long long > ltexPalette;
    if (argc > 6 && outFmt == 3)
    {
      outFmt = 10;                      // LTEX RGB
      ltexPalette.resize(256);
      loadPalette(ltexPalette.data(), argv[6]);
    }
    std::vector< std::uint16_t >  heightBuf;
    std::vector< std::uint16_t >  ltexBuf;
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
    hdrBuf[4] = (unsigned int) roundFloat(btdFile.getMinHeight());
    hdrBuf[5] = (unsigned int) xMax;
    hdrBuf[6] = (unsigned int) yMax;
    hdrBuf[7] = (unsigned int) roundFloat(btdFile.getMaxHeight());
    hdrBuf[8] = 0;                      // water level
    hdrBuf[9] = 1U << m;
    hdrBuf[10] = (outFmt >= 2 ? 0U : 1024U);    // default land level
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
          btdFile.getCellHeightMap(heightBuf.data(), x, y, l);
          break;
        case 2:
        case 3:
        case 10:
          ltexBuf.resize(cellBufSize);
          btdFile.getCellLandTexture(ltexBuf.data(), x, y, l);
          break;
      }
      if (outFmt == 3 || outFmt >= 8)
        btdFile.getCellTextureSet(txtSetBuf.data(), x, y);
      for (size_t yy = 0; yy < (1U << m); yy++)
      {
        size_t  offs = size_t((y0 - y) << m) | ((127U >> l) - yy);
        offs = (offs * w + size_t(x - xMin)) << m;
        unsigned char *p = outBuf.data() + (offs * outFmtDataSizes[outFmt]);
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
                  txtSetBuf.data()
                  + (((yy >> (m - 1)) << 5) + ((xx >> (m - 1)) << 4));
              unsigned char tmp = txtSetPtr[6];
              unsigned int  a =
                  std::uint32_t(std::int16_t(ltexBuf[(yy << m) | xx]));
              for (size_t i = 6; i-- > 0; a = a << 3)
              {
                tmp = blendDithered(tmp, txtSetPtr[i],
                                    (unsigned char)
                                        ((((a >> 15) & 7) * 73U) >> 1),
                                    int(xx), int(127U - yy));
              }
              p[0] = tmp;
            }
            break;
          case 8:               // texture set as 8-bit IDs
            for (size_t xx = 0; xx < (1U << m); xx++, p = p + 16)
            {
              for (size_t i = 0; i < 16; i++)
                p[i] = txtSetBuf[(((yy << m) | xx) << 4) | i];
            }
            break;
          case 9:               // texture set as 32-bit form IDs
            for (size_t xx = 0; xx < (1U << m); xx++, p = p + 64)
            {
              for (size_t i = 0; i < 16; i++)
              {
                unsigned int  tmp = txtSetBuf[(((yy << m) | xx) << 4) | i];
                if (tmp == 0xFF || i >= 8)
                  tmp = 0U;
                else
                  tmp = btdFile.getLandTexture(tmp);
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
                  txtSetBuf.data()
                  + (((yy >> (m - 1)) << 5) + ((xx >> (m - 1)) << 4));
              unsigned long long  tmp = ltexPalette[txtSetPtr[6]];
              unsigned int  a =
                  std::uint32_t(std::int16_t(ltexBuf[(yy << m) | xx]));
              for (size_t i = 6; i-- > 0; a = a << 3)
              {
                tmp = blendRBG64(tmp, ltexPalette[txtSetPtr[i]],
                                 (((a >> 15) & 7) * 73U) >> 1);
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
            outFile.writeData(outBuf.data(),
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

