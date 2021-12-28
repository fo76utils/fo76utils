
#include "common.hpp"
#include "esmfile.hpp"

static unsigned int worldFormID = 0x0000003CU;
static float  waterLevel = -1000000.0f;
static float  zMin = 1.0e9f;
static float  zMax = -1.0e9f;
static unsigned int defaultTexture = 0U;

struct CellData
{
  int     x;
  int     y;
  std::vector< float >  heightMap;
  std::vector< unsigned char >  vertexNormals;
  std::vector< unsigned char >  vertexColors;
  std::vector< unsigned char >  vertexTextures;
  CellData(int xc, int yc)
    : x(xc), y(yc)
  {
  }
};

static std::vector< unsigned int >  ltexFormIDs;
static std::map< unsigned int, CellData > worldCells;

unsigned char findTextureID(unsigned int formID)
{
  if (!formID)
    return 0xFF;
  for (size_t i = 0; i < ltexFormIDs.size(); i++)
  {
    if (ltexFormIDs[i] == formID)
      return (unsigned char) i;
  }
  if (ltexFormIDs.size() >= 255)
    throw errorMessage("number of unique land textures > 255");
  ltexFormIDs.push_back(formID);
  return (unsigned char) (ltexFormIDs.size() - 1);
}

// fldMask & 1: height map
// fldMask & 2: normals
// fldMask & 4: vertex colors
// fldMask & 8: textures

void dumpLandscape(ESMFile& esmFile, unsigned int formID, unsigned int fldMask)
{
  const ESMFile::ESMRecord  *landRecord = &(esmFile.getRecord(formID));
  const ESMFile::ESMRecord  *r = landRecord;
  while (true)
  {
    if (*r == "CELL")
      break;
    unsigned int  parent = r->parent;
    if (*r == "GRUP" && r->formID >= 6)
      parent = r->flags;
    if (!parent)
      return;
    r = &(esmFile.getRecord(parent));
  }
  ESMFile::ESMField f(esmFile, *r);
  if (!f.next())
    return;
  int     cellX = 0;
  int     cellY = 0;
  {
    while (!(f == "XCLC"))
    {
      if (!f.next())
        return;
    }
    cellX = f.readInt32();
    cellY = f.readInt32();
  }
  if (cellX < -32768 || cellX > 32767 || cellY < -32768 || cellY > 32767)
    return;
  ESMFile::ESMField f2(esmFile, *landRecord);
  if (!f2.next())
    return;
  unsigned int  key =
      ((unsigned int) (cellY + 32768) << 16) | (unsigned int) (cellX + 32768);
  if (worldCells.find(key) != worldCells.end())
    worldCells.erase(key);
  CellData& cell =
      worldCells.insert(std::pair< unsigned int, CellData >(
                            key, CellData(cellX, cellY))).first->second;
  unsigned char textureID = 0;
  unsigned char textureQuadrant = 0;
  unsigned char textureLayer = 0;
  do
  {
    if (f2 == "VHGT" && (fldMask & 1) != 0 && f2.size() >= 1093)
    {
      cell.heightMap.resize(1024, 0.0f);
      float   z = f2.readFloat() * 8.0f;
      for (int y = 0; y < 32; y++)
      {
        z += float(int(f2.readInt8()) << 3);
        cell.heightMap[y << 5] = z;
        zMin = (z < zMin ? z : zMin);
        zMax = (z > zMax ? z : zMax);
        float   z0 = z;
        for (int x = 1; x < 32; x++)
        {
          z += float(int(f2.readInt8()) << 3);
          cell.heightMap[(y << 5) | x] = z;
          zMin = (z < zMin ? z : zMin);
          zMax = (z > zMax ? z : zMax);
        }
        z = z0;
        (void) f2.readUInt8();
      }
    }
    else if (f2 == "VNML" && (fldMask & 2) != 0 && f2.size() >= 3267)
    {
      cell.vertexNormals.resize(3072);
      for (int y = 0; y < 32; y++)
      {
        for (int x = 0; x < 96; x++)
          cell.vertexNormals[y * 96 + x] = f2.readUInt8();
        (void) f2.readUInt8();
        (void) f2.readUInt8();
        (void) f2.readUInt8();
      }
    }
    else if (f2 == "VCLR" && (fldMask & 4) != 0 && f2.size() >= 3267)
    {
      cell.vertexColors.resize(3072);
      for (int y = 0; y < 32; y++)
      {
        for (int x = 0; x < 96; x++)
          cell.vertexColors[y * 96 + x] = f2.readUInt8();
        (void) f2.readUInt8();
        (void) f2.readUInt8();
        (void) f2.readUInt8();
      }
    }
    else if ((fldMask & 8) != 0 && f2.size() >= 8)
    {
      if (f2 == "BTXT" || f2 == "ATXT")
      {
        if (cell.vertexTextures.size() < 0x4800)
        {
          cell.vertexTextures.resize(0x4800, 0);
          for (size_t i = 0; i < cell.vertexTextures.size(); i = i + 18)
          {
            cell.vertexTextures[i] = 0xFF;
            cell.vertexTextures[i + 1] = 0xFF;
          }
        }
        textureID = findTextureID(f2.readUInt32());
        textureQuadrant = (unsigned char) (f2.readUInt16() & 3);
        textureLayer = 0;
        if (f2 == "ATXT")
          textureLayer = (unsigned char) ((f2.readUInt16() & 7) + 1);
        int     offs =
            int(((textureQuadrant & 2) << 4) | (textureQuadrant & 1)) * 0x0120
            + (textureLayer << 1);
        for (int y = 0; y < 16; y++)
        {
          for (int x = 0; x < 16; x++)
            cell.vertexTextures[((y << 5) | x) * 18 + offs] = textureID;
        }
      }
      else if (f2 == "VTXT" && textureLayer >= 1 && textureLayer <= 8)
      {
        for (size_t i = 0; (i + 8) <= f2.size(); i = i + 8)
        {
          unsigned int  p = f2.readUInt32() & 0xFFFF;
          float   a = f2.readFloat();
          int     x = int(p % 17U);
          int     y = int((p / 17U) % 17U);
          if (x >= 16 || y >= 16)
            continue;
          x = x + ((textureQuadrant & 1) << 4);
          y = y + ((textureQuadrant & 2) << 3);
          a = (a > 0.0f ? (a < 1.0f ? a : 1.0f) : 0.0f) * 255.0f + 0.5f;
          int     offs = ((y << 5) | x) * 18 + (textureLayer << 1);
          cell.vertexTextures[offs + 1] = (unsigned char) int(a);
        }
      }
    }
  }
  while (f2.next());
}

void findLandscape(ESMFile& esmFile, unsigned int formID, unsigned int fldMask)
{
  do
  {
    const ESMFile::ESMRecord& r = esmFile.getRecord(formID);
    if (r == "GRUP" && r.children)
    {
      if (r.formID != 1 || r.flags == worldFormID)
        findLandscape(esmFile, r.children, fldMask);
    }
    else if (r == "WRLD" && r.formID == worldFormID)
    {
      ESMFile::ESMField f(esmFile, r);
      while (f.next())
      {
        if (f == "DNAM" && f.size() >= 8)
        {
          (void) f.readFloat();
          waterLevel = f.readFloat();
        }
      }
    }
    else if (r == "LAND")
    {
      dumpLandscape(esmFile, formID, fldMask);
    }
    formID = r.next;
  }
  while (formID);
}

static bool printLTEXInfo(ESMFile& esmFile, unsigned int formID)
{
  bool    isValid = false;
  std::printf("\t0x%08X", formID);
  try
  {
    const ESMFile::ESMRecord& r = esmFile.getRecord(formID);
    if (r == "LTEX")
    {
      std::string   edid;
      unsigned int  txstFormID = 0U;
      ESMFile::ESMField f(esmFile, r);
      while (f.next())
      {
        if (f == "EDID" && edid.empty())
        {
          for (size_t i = 0; i < f.size(); i++)
          {
            if (f.getDataPtr()[i] == '\0')
              break;
            if (f.getDataPtr()[i] >= 0x20 && f.getDataPtr()[i] < 0x7F)
              edid += char(f.getDataPtr()[i]);
          }
        }
        else if (f == "TNAM" && f.size() == 4)
        {
          txstFormID = f.readUInt32();
        }
      }
      if (!edid.empty())
        std::printf("\t%s", edid.c_str());
      if (txstFormID)
      {
        ESMFile::ESMField f2(esmFile, txstFormID);
        while (f2.next())
        {
          if ((f2 == "TX00" || f2 == "MNAM") && f2.size() > 0)
          {
            std::fputc('\t', stdout);
            for (size_t i = 0; i < f2.size(); i++)
            {
              unsigned char c = f2.getDataPtr()[i];
              if (c == '\0')
                break;
              if (c < 0x20)
                continue;
              if (c >= 'A' && c <= 'Z')
                c = c + ('a' - 'A');
              else if (c == '\\')
                c = '/';
              std::fputc(c, stdout);
              isValid = true;
            }
            break;
          }
        }
      }
    }
  }
  catch (...)
  {
  }
  std::fputc('\n', stdout);
  return isValid;
}

int main(int argc, char **argv)
{
  if (argc < 3 || argc > 10 || argc == 7)
  {
    std::fprintf(stderr, "Usage: fo4land INFILE.ESM[,...] OUTFILE.DDS "
                         "[OPTIONS...]\n");
    std::fprintf(stderr, "Options: FMT [XMIN YMIN XMAX YMAX] "
                         "[worldID [defTxtID]]\n\n");
    std::fprintf(stderr, "FMT = 0: height map (16-bit grayscale)\n");
    std::fprintf(stderr, "FMT = 1: vertex normals (24-bit RGB)\n");
    std::fprintf(stderr, "FMT = 2: raw land textures (4 bytes per vertex)\n");
    std::fprintf(stderr, "FMT = 3: land textures (8-bit dithered)\n");
    std::fprintf(stderr, "FMT = 4: vertex colors (24-bit RGB)\n\n");
    std::fprintf(stderr, "worldID defaults to 0x0000003C, "
                         "use 0x000DA726 for Fallout: New Vegas\n");
    return 1;
  }
  try
  {
    int     outFmt = 0;
    unsigned int  fldMask = 1;
    int     xMin = 32768;
    int     xMax = -32768;
    int     yMin = 32768;
    int     yMax = -32768;
    int     argOffs = 4;
    if (argc >= 8)
    {
      argOffs = 8;
      xMin = int(parseInteger(argv[4], 10, "invalid xMin", -512, 512));
      yMin = int(parseInteger(argv[5], 10, "invalid yMin", -512, 512));
      xMax = int(parseInteger(argv[6], 10, "invalid xMax", xMin, 512));
      yMax = int(parseInteger(argv[7], 10, "invalid yMax", yMin, 512));
    }
    if (argc > 3)
    {
      outFmt = int(parseInteger(argv[3], 10, "invalid output format", 0, 4));
      fldMask = (outFmt == 2 ? 8U : (outFmt == 4 ? 4U : (1U << outFmt)));
    }
    if (argc > argOffs)
    {
      worldFormID =
          (unsigned int) parseInteger(argv[argOffs], 0,
                                      "invalid world form ID", 0L, 0x0FFFFFFFL);
    }
    if (argc > (argOffs + 1))
    {
      defaultTexture =
          (unsigned int) parseInteger(argv[argOffs + 1], 0,
                                      "invalid default texture form ID",
                                      0L, 0x0FFFFFFFL);
    }
    ESMFile   esmFile(argv[1]);
    findLandscape(esmFile, 0U, fldMask);
    if (worldCells.begin() == worldCells.end())
      throw errorMessage("no landscape records found");
    defaultTexture = findTextureID(defaultTexture);
    if (argc < 8)
    {
      for (std::map< unsigned int, CellData >::iterator i = worldCells.begin();
           i != worldCells.end(); i++)
      {
        CellData& cell = i->second;
        xMin = (cell.x < xMin ? cell.x : xMin);
        xMax = (cell.x > xMax ? cell.x : xMax);
        yMin = (cell.y < yMin ? cell.y : yMin);
        yMax = (cell.y > yMax ? cell.y : yMax);
      }
    }
    std::printf("Image size = %dx%d, "
                "cell X range = %d to %d, Y range = %d to %d\n",
                (xMax + 1 - xMin) * 32, (yMax + 1 - yMin) * 32,
                xMin, xMax, yMin, yMax);
    static const int  outFmtPixelFormats[5] =
    {
      DDSInputFile::pixelFormatGRAY16,  DDSInputFile::pixelFormatRGB24,
      DDSInputFile::pixelFormatL8A24,   DDSInputFile::pixelFormatGRAY8,
      DDSInputFile::pixelFormatRGB24
    };
    unsigned int  hdrBuf[11];
    hdrBuf[0] = 0x5F344F46;             // "FO4_"
    hdrBuf[1] = 0x444E414C;             // "LAND"
    hdrBuf[2] = (unsigned int) xMin;
    hdrBuf[3] = (unsigned int) yMin;
    hdrBuf[4] = (unsigned int) int(zMin + (zMin < 0.0f ? -0.5f : 0.5f));
    hdrBuf[5] = (unsigned int) xMax;
    hdrBuf[6] = (unsigned int) yMax;
    hdrBuf[7] = (unsigned int) int(zMax + (zMax < 0.0f ? -0.5f : 0.5f));
    hdrBuf[8] =
        (unsigned int) int(waterLevel + (waterLevel < 0.0f ? -0.5f : 0.5f));
    hdrBuf[9] = 32;
    hdrBuf[10] = 0;
    DDSOutputFile outFile(argv[2],
                          (xMax + 1 - xMin) << 5, (yMax + 1 - yMin) << 5,
                          outFmtPixelFormats[outFmt], hdrBuf, 16384);
    if (fldMask == 1)
    {
      std::printf("Minimum height = %f, maximum height = %f\n", zMin, zMax);
      std::printf("Water level = %f (%d)\n",
                  waterLevel,
                  int(((waterLevel - zMin) / (zMax - zMin)) * 65535.0f + 0.5f));
    }
    else if (fldMask == 8)
    {
      for (size_t i = 0; i < ltexFormIDs.size(); i++)
      {
        std::printf("Land texture %d:", int(i));
        if (!printLTEXInfo(esmFile, ltexFormIDs[i]) && outFmt != 2)
          ltexFormIDs[i] = 0U;
      }
    }
    for (int y = (yMax << 5) + 31; y >= (yMin << 5); y--)
    {
      for (int xc = xMin; xc <= xMax; xc++)
      {
        unsigned int  key = (unsigned int) (y + (32768 << 5)) >> 5;
        key = (key << 16) | (unsigned int) (xc + 32768);
        std::map< unsigned int, CellData >::iterator  i = worldCells.find(key);
        if (i == worldCells.end() ||
            ((fldMask & 1) != 0 && i->second.heightMap.size() < 1024) ||
            ((fldMask & 2) != 0 && i->second.vertexNormals.size() < 3072) ||
            ((fldMask & 4) != 0 && i->second.vertexColors.size() < 3072) ||
            ((fldMask & 8) != 0 && i->second.vertexTextures.size() < 0x4800))
        {
          static const unsigned long long dummyCellData[5] =
          {
            0x0000000000010000ULL,      // height map
            0x00000000018080FFULL,      // normals
            0x00000001000007FFULL,      // raw LTEX
            0x00000000000001FFULL,      // dithered LTEX
            0x0000000001FFFFFFULL       // vertex colors
          };
          for (int j = 0; j < 32; j++)
          {
            unsigned long long  tmp = dummyCellData[outFmt];
            if (outFmt == 3 || ((j & 7) == 0 && outFmt == 2))
              tmp = (tmp & ~0xFFULL) | defaultTexture;
            for ( ; tmp > 0x01; tmp = tmp >> 8)
              outFile.writeByte((unsigned char) (tmp & 0xFF));
          }
          continue;
        }
        CellData& cell = i->second;
        if (outFmt == 0)
        {
          for (int x = 0; x < 32; x++)
          {
            float   z = cell.heightMap[((y & 31) << 5) | x];
            int     n = int(((z - zMin) / (zMax - zMin)) * 65535.0f + 0.5f);
            outFile.writeByte((unsigned char) (n & 0xFF));
            outFile.writeByte((unsigned char) (n >> 8));
          }
        }
        else if (outFmt == 1)
        {
          for (int x = 0; x < 32; x++)
          {
            const unsigned char *p =
                &(cell.vertexNormals.front()) + ((((y & 31) << 5) | x) * 3);
            outFile.writeByte((unsigned char) ((p[2] + 128) & 0xFF));
            outFile.writeByte((unsigned char) ((p[1] + 128) & 0xFF));
            outFile.writeByte((unsigned char) ((p[0] + 128) & 0xFF));
          }
        }
        else if (outFmt == 2)
        {
          unsigned char blockTextures[8];
          unsigned int  blockLayerMask = 0;
          for (int x = 0; x < 32; x++)
          {
            const unsigned char *p =
                &(cell.vertexTextures.front()) + ((((y & 31) << 5) | x) * 18);
            if (!(x & 7))
            {
              blockTextures[0] =
                  (p[0] != 0xFF ? p[0] : (unsigned char) defaultTexture);
              blockLayerMask = 0x0001;
              for (unsigned int j = 1; j < 8; j++)
              {
                blockTextures[j] = p[j << 1];
                for (unsigned int k = 0; k < 18; k = k + 2)
                {
                  if (p[j * 18 + k] != 0xFF && p[j * 18 + k + 1] >= 19)
                    blockLayerMask = blockLayerMask | (1U << (k >> 1));
                }
              }
              if (blockLayerMask > 0xFF)
              {
                unsigned int  n = 1;
                for (unsigned int j = 2; j < 18; j = j + 2)
                {
                  bool    layerUsed = false;
                  for (unsigned int k = 0; k < 8; k++)
                  {
                    if (p[k * 18 + j] != 0xFF && p[k * 18 + j + 1] >= 19)
                    {
                      layerUsed = true;
                      break;
                    }
                  }
                  if (!layerUsed)
                    continue;
                  if (n >= 8)
                  {
                    std::fprintf(stderr, "Warning: too many layers at %d,%d\n",
                                 (xc << 5) + x, y);
                    blockLayerMask = 0x007F;
                    n--;
                  }
                  blockTextures[n] = p[j];
                  blockLayerMask = blockLayerMask | (1U << (j >> 1));
                  n++;
                }
              }
              else
              {
                blockLayerMask = 0xFF;
              }
            }
            outFile.writeByte(blockTextures[x & 7]);
            unsigned int  tmp = 0;
            unsigned char n = 0;
            for (unsigned int j = 0; j < 18; j = j + 2)
            {
              if (!(blockLayerMask & (1U << (j >> 1))))
                continue;
              unsigned int  a = p[j + 1];
              tmp = tmp | (((a * 7U + 127U) / 255U) << n);
              n = n + 3;
            }
            outFile.writeByte((unsigned char) ((tmp | 0x07) & 0xFF));
            outFile.writeByte((unsigned char) ((tmp >> 8) & 0xFF));
            outFile.writeByte((unsigned char) ((tmp >> 16) & 0xFF));
          }
        }
        else if (outFmt == 3)
        {
          for (int x = 0; x < 32; x++)
          {
            const unsigned char *p =
                &(cell.vertexTextures.front()) + ((((y & 31) << 5) | x) * 18);
            unsigned char tmp = p[0];
            for (size_t j = 1; j < 9; j++)
            {
              if (ltexFormIDs[p[j << 1]])
                tmp = blendDithered(tmp, p[j << 1], p[(j << 1) + 1], x, y);
            }
            if (tmp == 0xFF)
              tmp = (unsigned char) defaultTexture;
            outFile.writeByte(tmp);
          }
        }
        else if (outFmt == 4)
        {
          for (int x = 0; x < 32; x++)
          {
            const unsigned char *p =
                &(cell.vertexColors.front()) + ((((y & 31) << 5) | x) * 3);
            outFile.writeByte(p[2]);
            outFile.writeByte(p[1]);
            outFile.writeByte(p[0]);
          }
        }
      }
    }
    outFile.flush();
  }
  catch (std::exception& e)
  {
    std::fprintf(stderr, "fo4land: %s\n", e.what());
    return 1;
  }
  return 0;
}

