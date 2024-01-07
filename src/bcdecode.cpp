
#include "common.hpp"
#include "filebuf.hpp"
#include "fp32vec4.hpp"
#include "ddstxt.hpp"
#include "sfcube.hpp"

bool convertHDRToDDS(std::vector< unsigned char >& outBuf, FileBuffer& inBuf,
                     int cubeWidth, bool invertCoord, float maxLevel)
{
  // file should begin with "#?RADIANCE\n"
  if (inBuf.size() < 11 ||
      FileBuffer::readUInt64Fast(inBuf.data()) != 0x4E41494441523F23ULL ||
      FileBuffer::readUInt32Fast(inBuf.data() + 7) != 0x0A45434EU)
  {
    return false;
  }
  inBuf.setPosition(11);
  std::string lineBuf;
  int     w = 0;
  int     h = 0;
  while (true)
  {
    unsigned char c = inBuf.readUInt8();
    if (c < 0x08)
      return false;
    if (c != 0x0A)
    {
      lineBuf += char(c);
      continue;
    }
    if (lineBuf.starts_with("-Y"))
    {
      const char  *s = lineBuf.c_str() + 2;
      while (*s == '\t' || *s == ' ')
        s++;
      char    *endp = nullptr;
      long    n = std::strtol(s, &endp, 10);
      if (!endp || endp == s || n < 8 || n > 32768)
        return false;
      h = int(n);
      s = endp;
      while (*s == '\t' || *s == ' ')
        s++;
      if (!(s[0] == '+' && s[1] == 'X'))
        return false;
      s = s + 2;
      while (*s == '\t' || *s == ' ')
        s++;
      n = std::strtol(s, &endp, 10);
      if (!endp || endp == s || n < 8 || n > 32768)
        return false;
      w = int(n);
      break;
    }
    lineBuf.clear();
  }
  std::vector< std::uint32_t >  tmpBuf(size_t(w * h), 0U);
  for (int y = 0; y < h; y++)
  {
    if ((inBuf.getPosition() + 4ULL) > inBuf.size())
      return false;
    std::uint32_t *p = tmpBuf.data() + size_t(y * w);
    std::uint32_t tmp =
        FileBuffer::readUInt32Fast(inBuf.data() + inBuf.getPosition());
    if (tmp != ((std::uint32_t(w & 0xFF) << 24) | (std::uint32_t(w >> 8) << 16)
                | 0x0202U))
    {
      // old RLE format
      unsigned char lenShift = 0;
      for (int x = 0; x < w; )
      {
        std::uint32_t c = inBuf.readUInt32();
        if ((c & 0x00FFFFFFU) != 0x00010101U || x < 1)
        {
          lenShift = 0;
          p[x] = c;
          x++;
        }
        else
        {
          size_t  l = (c >> 24) << lenShift;
          lenShift = 8;
          for ( ; l; l--, x++)
          {
            if (x >= w)
              return false;
            p[x] = p[x - 1];
          }
        }
      }
    }
    else
    {
      // new RLE format
      inBuf.setPosition(inBuf.getPosition() + 4);
      for (unsigned char c = 0; c < 32; c = c + 8)
      {
        for (int x = 0; x < w; )
        {
          if (inBuf.getPosition() >= inBuf.size())
            return false;
          unsigned char l = inBuf.readUInt8Fast();
          if (l <= 0x80)
          {
            // copy literals
            for ( ; l; l--, x++)
            {
              if (x >= w || inBuf.getPosition() >= inBuf.size())
                return false;
              p[x] |= (std::uint32_t(inBuf.readUInt8Fast()) << c);
            }
          }
          else
          {
            // RLE
            if (inBuf.getPosition() >= inBuf.size())
              return false;
            std::uint32_t b = std::uint32_t(inBuf.readUInt8Fast()) << c;
            for ( ; l > 0x80; l--, x++)
            {
              if (x >= w)
                return false;
              p[x] |= b;
            }
          }
        }
      }
    }
  }
  std::vector< FloatVector4 > tmpBuf2(tmpBuf.size(), FloatVector4(0.0f));
  for (size_t i = 0; i < tmpBuf.size(); i++)
  {
    std::uint32_t b = tmpBuf[i];
    FloatVector4  c(b);
    int     e = int(b >> 24);
    if (e < 136)
    {
      if (e < 106)
        c = FloatVector4(0.0f);
      else
        c /= float(1 << (136 - e));
    }
    else if (e > 136)
    {
      if (e <= 166)
        c *= float(1 << (e - 136));
      else
        c = FloatVector4(float(64.0 * 65536.0 * 65536.0));
    }
    c[3] = 1.0f;
    tmpBuf2[i] = c;
  }
  outBuf.resize(size_t(cubeWidth * cubeWidth) * 6 * sizeof(std::uint64_t) + 148,
                0);
  unsigned char *p = outBuf.data();
  for (size_t i = 6; i < 37; i++)
    FileBuffer::writeUInt32Fast(p + (i << 2), 0U);
  FileBuffer::writeUInt32Fast(p, 0x20534444U);          // "DDS "
  p[4] = 124;                           // size of header
  FileBuffer::writeUInt32Fast(p + 8, 0x0002100FU);      // flags
  // height, width, pitch
  FileBuffer::writeUInt32Fast(p + 12, std::uint32_t(cubeWidth));
  FileBuffer::writeUInt32Fast(p + 16, std::uint32_t(cubeWidth));
  FileBuffer::writeUInt32Fast(p + 20,
                              std::uint32_t(cubeWidth * sizeof(std::uint64_t)));
  p[28] = 1;                            // number of mipmaps
  p[76] = 32;                           // size of pixel format
  p[80] = 0x04;                         // DDPF_FOURCC
  FileBuffer::writeUInt32Fast(p + 84, 0x30315844U);     // "DX10"
  FileBuffer::writeUInt32Fast(p + 108, 0x00401008U);    // dwCaps
  p[113] = 0xFE;                        // dwCaps2 (DDSCAPS2_CUBEMAP*)
  p[128] = 0x0A;                        // DXGI_FORMAT_R16G16B16A16_FLOAT
  p[132] = 3;                           // DDS_DIMENSION_TEXTURE2D
  p = p + 148;
  for (int n = 0; n < 6; n++)
  {
    for (int y = 0; y < cubeWidth; y++)
    {
      FloatVector4  coordMult(1.0f, -1.0f, -1.0f, 1.0f);
      if (invertCoord)
        coordMult = FloatVector4(1.0f);
      for (int x = 0; x < cubeWidth; x++, p = p + sizeof(std::uint64_t))
      {
        FloatVector4  v(SFCubeMapFilter::convertCoord(x, y, cubeWidth, n));
        v *= coordMult;
        // convert to equirectangular coordinates
        float   xf = float(std::atan2(v[1], v[0])) * 0.15915494f + 0.5f;
        float   yf = float(std::asin(std::min(std::max(v[2], -1.0f), 1.0f)))
                     * 0.31830989f + 0.5f;
        xf *= float(w - 1);
        yf *= float(h - 1);
        int     x0 = std::min< int >(std::max< int >(int(xf), 0), w - 1);
        int     y0 = std::min< int >(std::max< int >(int(yf), 0), h - 1);
        xf = xf - float(std::floor(xf));
        yf = yf - float(std::floor(yf));
        int     x1 = x0 + int(x0 < (w - 1));
        int     y1 = y0 + int(y0 < (h - 1));
        // bilinear interpolation
        FloatVector4  c0(tmpBuf2[y0 * w + x0]);
        FloatVector4  c1(tmpBuf2[y0 * w + x1]);
        FloatVector4  c2(tmpBuf2[y1 * w + x0]);
        FloatVector4  c3(tmpBuf2[y1 * w + x1]);
        c0 = c0 + ((c1 - c0) * xf);
        c2 = c2 + ((c3 - c2) * xf);
        c0 = c0 + ((c2 - c0) * yf);
        c0.maxValues(FloatVector4(0.0f));
        if (maxLevel < 0.0f)
          c0 = c0 * FloatVector4(maxLevel) / (FloatVector4(maxLevel) - c0);
        else
          c0.minValues(FloatVector4(maxLevel));
        c0[3] = 1.0f;
        FileBuffer::writeUInt64Fast(p, c0.convertToFloat16());
      }
    }
  }
  return true;
}

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    std::fprintf(stderr, "Usage:\n");
    std::fprintf(stderr,
                 "    bcdecode INFILE.DDS "
                 "[MULT | OUTFILE.RGBA | OUTFILE.DDS [FACE [FLAGS]]]\n");
    std::fprintf(stderr,
                 "    bcdecode INFILE.DDS OUTFILE.DDS -cube_filter [WIDTH]\n");
    std::fprintf(stderr,
                 "    bcdecode INFILE.HDR "
                 "OUTFILE.DDS -cube [WIDTH [MAXLEVEL]]\n\n");
    std::fprintf(stderr, "    FLAGS & 1 = ignore alpha channel\n");
    std::fprintf(stderr, "    FLAGS & 2 = calculate normal map blue channel\n");
    return 1;
  }
  OutputFile  *outFile = nullptr;
  try
  {
    if (argc > 3 && std::strcmp(argv[3], "-cube") == 0)
    {
      int     w = 2048;
      bool    invertCoord = false;
      float   maxLevel = 65504.0f;
      if (argc > 4)
      {
        w = int(parseInteger(argv[4], 10, "invalid output image dimensions",
                             -16384, 16384));
        if (w < 0)
        {
          w = -w;
          invertCoord = true;
        }
        if (w < 32 || (w & (w - 1)))
          errorMessage("invalid output image dimensions");
        if (argc > 5)
        {
          maxLevel = float(parseFloat(argv[5], "invalid maximum output level",
                                      -65504.0, 65504.0));
          if (maxLevel > -0.125f && maxLevel < 0.125f)
            errorMessage("invalid maximum output level");
        }
      }
      std::vector< unsigned char >  outBuf;
      {
        FileBuffer  inFile(argv[1]);
        if (!convertHDRToDDS(outBuf, inFile, w, invertCoord, maxLevel))
          errorMessage("invalid or unsupported input file");
      }
      outFile = new OutputFile(argv[2], 16384);
      outFile->writeData(outBuf.data(), outBuf.size());
      outFile->flush();
      delete outFile;
      return 0;
    }
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

