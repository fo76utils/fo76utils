
#include "common.hpp"
#include "filebuf.hpp"
#include "ddstxt.hpp"
#include "sfcube.hpp"

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    std::fprintf(stderr, "Usage:\n");
    std::fprintf(stderr,
                 "    bcdecode INFILE.DDS "
                 "[MULT | OUTFILE.RGBA | OUTFILE.DDS [FACE [FLAGS]]]\n");
    std::fprintf(stderr,
                 "    bcdecode INFILE.DDS "
                 "OUTFILE.DDS -cube_filter [WIDTH]\n\n");
    std::fprintf(stderr, "    FLAGS & 1 = ignore alpha channel\n");
    std::fprintf(stderr, "    FLAGS & 2 = calculate normal map blue channel\n");
    return 1;
  }
  OutputFile  *outFile = nullptr;
  try
  {
    if (argc > 3 && std::strcmp(argv[3], "-cube_filter") == 0)
    {
      size_t  w = 256;
      if (argc > 4)
      {
        w = size_t(parseInteger(argv[4], 10, "invalid output image dimensions",
                                128, 2048));
      }
      SFCubeMapFilter cubeFilter(w);
      std::vector< unsigned char >  outBuf;
      {
        FileBuffer  inFile(argv[1]);
        if (!(inFile.size() > 148 &&
              FileBuffer::readUInt32Fast(inFile.data()) == 0x20534444U))
        {
          errorMessage("invalid input file");
        }
        size_t  bufCapacity = w * w * 8 * 4 + 148;
        if (inFile.size() > bufCapacity)
          bufCapacity = inFile.size();
        outBuf.resize(bufCapacity);
        std::memcpy(outBuf.data(), inFile.data(), inFile.size());
        size_t  newSize = cubeFilter.convertImage(outBuf.data(), inFile.size(),
                                                  true, bufCapacity);
        if (outBuf[128] != 0x43 || !newSize)
          errorMessage("failed to convert texture");
        outBuf.resize(newSize);
      }
      outFile = new OutputFile(argv[2], 16384);
      outFile->writeData(outBuf.data(), outBuf.size());
      outFile->flush();
      delete outFile;
      return 0;
    }
    DDSTexture  t(argv[1]);
    unsigned int  w = (unsigned int) t.getWidth();
    unsigned int  h = (unsigned int) t.getHeight();
    double  rgbScale = 255.0;
    int     textureNum = 0;
    unsigned int  alphaMask = 0U;
    bool    ddsOutFmt = false;
    bool    calculateNormalZ = false;
    if (argc > 2)
    {
      // TES5: 1.983
      // FO4:  1.698
      // FO76: 1.608
      try
      {
        rgbScale = parseFloat(argv[2], nullptr, 0.001, 1000.0) * 255.0;
      }
      catch (...)
      {
        rgbScale = 255.0;
        size_t  n = std::strlen(argv[2]);
        ddsOutFmt = true;
        if (n > 5)
        {
          const char  *s = argv[2] + (n - 5);
          if (std::strcmp(s, ".data") == 0 || std::strcmp(s, ".DATA") == 0 ||
              std::strcmp(s, ".rgba") == 0 || std::strcmp(s, ".RGBA") == 0)
          {
            ddsOutFmt = false;
          }
        }
        if (!ddsOutFmt)
        {
          outFile = new OutputFile(argv[2], 16384);
        }
        else
        {
          outFile = new DDSOutputFile(argv[2], int(w), int(h),
                                      DDSInputFile::pixelFormatRGBA32);
        }
      }
      if (argc > 3)
      {
        textureNum = int(parseInteger(argv[3], 10, "invalid texture number",
                                      0, int(t.getTextureCount()) - 1));
      }
      if (argc > 4)
      {
        int     tmp = int(parseInteger(argv[4], 0, "invalid flags", 0, 3));
        if (tmp & 1)
          alphaMask = 0xFF000000U;
        calculateNormalZ = bool(tmp & 2);
      }
    }
    const double  gamma = 2.2;
    std::vector< double > gammaTable(256, 0.0);
    for (int i = 0; i < 256; i++)
      gammaTable[i] = std::pow(double(i) / 255.0, gamma);

    double  r2Sum = 0.0;
    double  g2Sum = 0.0;
    double  b2Sum = 0.0;
    double  aSum = 0.0;
    for (size_t y = 0; y < h; y++)
    {
      for (size_t x = 0; x < w; x++)
      {
        unsigned int  c = t.getPixelN(x, y, 0, textureNum) | alphaMask;
        unsigned char r = (unsigned char) (c & 0xFF);
        unsigned char g = (unsigned char) ((c >> 8) & 0xFF);
        unsigned char b = (unsigned char) ((c >> 16) & 0xFF);
        unsigned char a = (unsigned char) ((c >> 24) & 0xFF);
        if (BRANCH_UNLIKELY(calculateNormalZ))
        {
          float   normalX = float(int(r)) * (1.0f / 127.5f) - 1.0f;
          float   normalY = float(int(g)) * (1.0f / 127.5f) - 1.0f;
          float   normalZ = 1.0f - ((normalX * normalX) + (normalY * normalY));
          normalZ = float(std::sqrt(std::max(normalZ, 0.0f)));
          b = (unsigned char) roundFloat(normalZ * 127.5f + 127.5f);
        }
        if (outFile)
        {
          if (!ddsOutFmt)
          {
            outFile->writeByte(r);
            outFile->writeByte(g);
            outFile->writeByte(b);
          }
          else
          {
            outFile->writeByte(b);
            outFile->writeByte(g);
            outFile->writeByte(r);
          }
          outFile->writeByte(a);
        }
        else
        {
          double  f = 1.0;      // double(int(a)) / 255.0;
          r2Sum = r2Sum + (gammaTable[r] * f);
          g2Sum = g2Sum + (gammaTable[g] * f);
          b2Sum = b2Sum + (gammaTable[b] * f);
          aSum = aSum + f;
        }
      }
    }
    if (!outFile)
    {
      r2Sum = std::pow(r2Sum / aSum, 1.0 / gamma) * rgbScale + 0.5;
      g2Sum = std::pow(g2Sum / aSum, 1.0 / gamma) * rgbScale + 0.5;
      b2Sum = std::pow(b2Sum / aSum, 1.0 / gamma) * rgbScale + 0.5;
      std::printf("%3d%4d%4d\n", int(r2Sum), int(g2Sum), int(b2Sum));
    }
    else
    {
      outFile->flush();
      delete outFile;
    }
  }
  catch (std::exception& e)
  {
    if (outFile)
      delete outFile;
    std::fprintf(stderr, "bcdecode: %s\n", e.what());
    return 1;
  }
  return 0;
}

