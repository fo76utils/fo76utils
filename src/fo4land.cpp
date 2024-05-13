
#include "common.hpp"
#include "esmfile.hpp"
#include "ba2file.hpp"
#include "landdata.hpp"

static unsigned int worldFormID = 0x0000003CU;
static unsigned int defaultTexture = 0U;

static bool archiveFilterFunction(void *p, const std::string_view& s)
{
  (void) p;
  if (!(s.ends_with(".dds") || s.ends_with(".bgsm")))
    return false;
  return (s.find("/lod/") == std::string_view::npos &&
          s.find("/actors/") == std::string_view::npos);
}

int main(int argc, char **argv)
{
  BA2File *ba2File = (BA2File *) 0;
  try
  {
    std::vector< const char * > args;
    const char  *ba2Path = (char *) 0;
    for (int i = 1; i < argc; i++)
    {
      if (std::strcmp(argv[i], "--") == 0)
      {
        while (++i < argc)
          args.push_back(argv[i]);
        break;
      }
      if (std::strcmp(argv[i], "-d") == 0)
      {
        if (++i >= argc)
          errorMessage("missing argument for -d");
        ba2Path = argv[i];
      }
      else
      {
        args.push_back(argv[i]);
      }
    }
    if (args.size() < 2 || args.size() > 9 || args.size() == 6)
    {
      std::fprintf(stderr, "Usage: fo4land INFILE.ESM[,...] OUTFILE.DDS "
                           "[OPTIONS...]\n");
      std::fprintf(stderr, "Options: [-d ARCHIVEPATH] FMT "
                           "[XMIN YMIN XMAX YMAX] [worldID [defTxtID]]\n\n");
      std::fprintf(stderr, "FMT = 0: height map (16-bit grayscale)\n");
      std::fprintf(stderr, "FMT = 1: vertex normals (24-bit RGB)\n");
      std::fprintf(stderr, "FMT = 2: raw land textures (4 bytes per vertex)\n");
      std::fprintf(stderr, "FMT = 3: land textures (8-bit dithered)\n");
      std::fprintf(stderr, "FMT = 4: vertex colors (24-bit RGB)\n");
      std::fprintf(stderr, "FMT = 5: cell texture sets as 8-bit IDs\n");
      std::fprintf(stderr, "FMT = 6: cell texture sets as 32-bit form IDs\n\n");
      std::fprintf(stderr, "worldID defaults to 0x0000003C, "
                           "use 0x000DA726 for Fallout: New Vegas\n");
      return 1;
    }
    int     outFmt = 0;
    unsigned int  formatMask = 1;
    int     xMin = -32768;
    int     xMax = 32767;
    int     yMin = -32768;
    int     yMax = 32767;
    if (args.size() >= 7)
    {
      xMin = int(parseInteger(args[3], 10, "invalid xMin", -32768, 32767));
      yMin = int(parseInteger(args[4], 10, "invalid yMin", -32768, 32767));
      xMax = int(parseInteger(args[5], 10, "invalid xMax", xMin, 32767));
      yMax = int(parseInteger(args[6], 10, "invalid yMax", yMin, 32767));
      args.erase(args.begin() + 3, args.begin() + 7);
    }
    if (args.size() > 2)
    {
      outFmt = int(parseInteger(args[2], 10, "invalid output format", 0, 6));
      if (outFmt == 0)                  // height map
        formatMask = 1;
      else if (outFmt == 1)             // vertex normals
        formatMask = 4;
      else if (outFmt == 4)             // vertex colors
        formatMask = 8;
      else                              // textures
        formatMask = 2;
    }
    if (args.size() > 3)
    {
      worldFormID =
          (unsigned int) parseInteger(args[3], 0,
                                      "invalid world form ID", 0L, 0x0FFFFFFFL);
    }
    if (args.size() > 4)
    {
      defaultTexture =
          (unsigned int) parseInteger(args[4], 0,
                                      "invalid default texture form ID",
                                      0L, 0x0FFFFFFFL);
    }
    if (ba2Path && *ba2Path)
      ba2File = new BA2File(ba2Path, &archiveFilterFunction);
    ESMFile   esmFile(args[0]);
    LandscapeData landData(&esmFile, (char *) 0, ba2File,
                           formatMask, worldFormID, defaultTexture, 2,
                           xMin, yMin, xMax, yMax);
    if (ba2File)
    {
      delete ba2File;
      ba2File = (BA2File *) 0;
    }
    xMin = landData.getXMin();
    yMin = landData.getYMin();
    xMax = landData.getXMax();
    yMax = landData.getYMax();
    float   zMin = landData.getZMin();
    float   zMax = landData.getZMax();
    static const int  outFmtPixelFormats[7] =
    {
      DDSInputFile::pixelFormatGRAY16,  DDSInputFile::pixelFormatRGB24,
      DDSInputFile::pixelFormatA32,     DDSInputFile::pixelFormatGRAY8,
      DDSInputFile::pixelFormatRGB24,   DDSInputFile::pixelFormatR8,
      DDSInputFile::pixelFormatR32
    };
    unsigned int  hdrBuf[11];
    hdrBuf[0] = 0x5F344F46;             // "FO4_"
    hdrBuf[1] = 0x444E414C;             // "LAND"
    hdrBuf[2] = (unsigned int) xMin;
    hdrBuf[3] = (unsigned int) yMin;
    hdrBuf[4] = (unsigned int) roundFloat(zMin);
    hdrBuf[5] = (unsigned int) xMax;
    hdrBuf[6] = (unsigned int) yMax;
    hdrBuf[7] = (unsigned int) roundFloat(zMax);
    hdrBuf[8] = (unsigned int) roundFloat(landData.getWaterLevel());
    hdrBuf[9] = (outFmt < 5 ? 32U : 2U);
    hdrBuf[10] = 0;
    if (outFmt == 0)
      hdrBuf[10] = (unsigned int) roundFloat(landData.getDefaultLandLevel());
    std::printf("Image size = %dx%d, "
                "cell X range = %d to %d, Y range = %d to %d\n",
                (xMax + 1 - xMin) << 5, (yMax + 1 - yMin) * int(hdrBuf[9]),
                xMin, xMax, yMin, yMax);
    DDSOutputFile outFile(args[1],
                          (xMax + 1 - xMin) << 5,
                          (yMax + 1 - yMin) * int(hdrBuf[9]),
                          outFmtPixelFormats[outFmt], hdrBuf, 16384);
    if (outFmt == 0)
    {
      float   waterLevel = landData.getWaterLevel();
      std::printf("Minimum height = %f, maximum height = %f\n", zMin, zMax);
      std::printf("Water level = %f (%d)\n",
                  waterLevel,
                  int(((waterLevel - zMin) / (zMax - zMin)) * 65535.0f + 0.5f));
    }
    else if (formatMask & 0x02)
    {
      for (size_t i = 0; i < landData.getTextureCount(); i++)
      {
        std::printf("Land texture %d:\t0x%08X",
                    int(i), landData.getTextureFormID(i));
        if (!landData.getTextureEDID(i).empty())
          std::printf("\t%s", landData.getTextureEDID(i).c_str());
        if (!landData.getMaterialPath(i).empty())
          std::printf("\t%s", landData.getMaterialPath(i).c_str());
        if (!landData.getTextureDiffuse(i).empty())
          std::printf("\t%s", landData.getTextureDiffuse(i).c_str());
        std::fputc('\n', stdout);
      }
    }
    size_t  vertexCnt =
        size_t(landData.getImageWidth()) * size_t(landData.getImageHeight());
    if (outFmt == 0 && landData.getHeightMap())
    {
      outFile.writeData(landData.getHeightMap(),
                        vertexCnt * sizeof(unsigned short));
    }
    if (outFmt == 1 && landData.getVertexNormals())
      outFile.writeData(landData.getVertexNormals(), vertexCnt * 3);
    if (outFmt == 2 && landData.getLandTexture32())
    {
      outFile.writeData(landData.getLandTexture32(),
                        vertexCnt * sizeof(unsigned int));
    }
    if (outFmt == 3 &&
        landData.getLandTexture32() && landData.getCellTextureSets())
    {
      const unsigned int  *ltexPtr = landData.getLandTexture32();
      int     w = landData.getImageWidth();
      int     h = landData.getImageHeight();
      for (int y = 0; y < h; y++)
      {
        for (int x = 0; x < w; x++)
        {
          const unsigned char *txtSetPtr = landData.getCellTextureSets();
          txtSetPtr = txtSetPtr + (((y >> 4) * w) + ((x >> 4) << 4));
          unsigned char tmp = txtSetPtr[0];
          unsigned int  a = ltexPtr[y * w + x];
          for (size_t i = 1; i < 9; i++, a = a >> 4)
          {
            unsigned char t = txtSetPtr[i];
            if (t < landData.getTextureCount() && landData.getTextureFormID(t))
            {
              tmp = blendDithered(tmp, t,
                                  (unsigned char) ((a & 15U) * 17U), x, ~y);
            }
          }
          outFile.writeByte(tmp);
        }
      }
    }
    if (outFmt == 4 && landData.getVertexColor24())
      outFile.writeData(landData.getVertexColor24(), vertexCnt * 3);
    if (outFmt == 5 && landData.getCellTextureSets())
    {
      size_t  n = size_t(xMax + 1 - xMin) * size_t(yMax + 1 - yMin) * 64;
      outFile.writeData(landData.getCellTextureSets(), n);
    }
    if (outFmt == 6 && landData.getCellTextureSets())
    {
      const unsigned char *txtSetPtr = landData.getCellTextureSets();
      size_t  n = size_t(xMax + 1 - xMin) * size_t(yMax + 1 - yMin) * 64;
      for (size_t i = 0; i < n; i++)
      {
        unsigned int  tmp = txtSetPtr[i];
        if (tmp >= landData.getTextureCount())
          tmp = 0U;
        else
          tmp = landData.getTextureFormID(tmp);
        outFile.writeByte((unsigned char) (tmp & 0xFF));
        outFile.writeByte((unsigned char) ((tmp >> 8) & 0xFF));
        outFile.writeByte((unsigned char) ((tmp >> 16) & 0xFF));
        outFile.writeByte((unsigned char) ((tmp >> 24) & 0xFF));
      }
    }
    outFile.flush();
  }
  catch (std::exception& e)
  {
    if (ba2File)
      delete ba2File;
    std::fprintf(stderr, "fo4land: %s\n", e.what());
    return 1;
  }
  return 0;
}

