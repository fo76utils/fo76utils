
#include "filebuf.hpp"
#include "esmfile.hpp"

static int    landWidth = 25728;
static int    landHeight = 25728;
static int    cellMinX = -100;
static int    cellMinY = -100;
static int    cellMaxX = 100;
static int    cellMaxY = 100;
static float  zMin = -700.0f;
static float  zMax = 38210.0f;
static float  zRangeOffs = 700.0f;
static float  zRangeScale = 1.6842714f;
static float  defaultWaterLevel = 0.0f;
static float  xyRangeScale = 0.03125f;
static int    cellSize = 128;
static int    cellOffsetX = 12864;
static int    cellOffsetY = 12863;
static unsigned int worldFormID = 0x0025DA15;

static std::vector< unsigned short >  waterHeightMap;

static inline int convertX(float x)
{
  return (roundFloat(x * xyRangeScale) + cellOffsetX);
}

static inline int convertY(float y)
{
  return (roundFloat(-y * xyRangeScale) + cellOffsetY);
}

static inline unsigned int convertZ(float z)
{
  int     tmp = roundFloat((z + zRangeOffs) * zRangeScale);
  tmp = (tmp > 0 ? (tmp < 65535 ? tmp : 65535) : 0);
  return (unsigned int) tmp;
}

static void fillWaterCell(int x0, int y0, int z0)
{
  unsigned int  zz = convertZ(float(z0));
  int     d = 4096 / cellSize;
  for (int y = 0; y < 4096; y = y + d)
  {
    for (int x = 0; x < 4096; x = x + d)
    {
      int     xx = convertX(float(x0 + x));
      int     yy = convertY(float(y0 + y));
      if (xx >= 0 && xx < landWidth && yy >= 0 && yy < landHeight)
      {
        if (zz > waterHeightMap[yy * landWidth + xx])
          waterHeightMap[yy * landWidth + xx] = (unsigned short) zz;
      }
    }
  }
}

static void fillPolygon(float x1, float y1, float z1,
                        float x2, float y2, float z2,
                        float x3, float y3, float z3,
                        float x4, float y4, float z4)
{
  float   d1 = ((x3 - x1) * (x3 - x1)) + ((y3 - y1) * (y3 - y1));
  float   d2 = ((x4 - x2) * (x4 - x2)) + ((y4 - y2) * (y4 - y2));
  int     n = int(std::sqrt(d1 > d2 ? d1 : d2) * xyRangeScale + 1.5);
  float   f = 1.0f / float(n);
  for (int i = 0; i <= n; i++)
  {
    for (int j = 0; j <= n; j++)
    {
      float   m1 = (float(n - i) * f) * (float(n - j) * f);
      float   m2 = (float(i) * f) * (float(n - j) * f);
      float   m3 = (float(i) * f) * (float(j) * f);
      float   m4 = (float(n - i) * f) * (float(j) * f);
      int     x = convertX((x1 * m1) + (x2 * m2) + (x3 * m3) + (x4 * m4));
      int     y = convertY((y1 * m1) + (y2 * m2) + (y3 * m3) + (y4 * m4));
      unsigned int  z = convertZ((z1 * m1) + (z2 * m2) + (z3 * m3) + (z4 * m4));
      if (x >= 0 && x < landWidth && y >= 0 && y < landHeight)
      {
        if (z > waterHeightMap[y * landWidth + x])
          waterHeightMap[y * landWidth + x] = (unsigned short) z;
      }
    }
  }
}

static void fillWater(int x0, int y0, int z0, int x1, int y1, int z1,
                      int x2, int y2, int z2, float rx, float ry, float rz)
{
  float   rx_s = float(std::sin(rx));
  float   rx_c = float(std::cos(rx));
  float   ry_s = float(std::sin(ry));
  float   ry_c = float(std::cos(ry));
  float   rz_s = float(std::sin(rz));
  float   rz_c = float(std::cos(rz));
  float   r_xx = ry_c * rz_c;
  float   r_xy = ry_c * rz_s;
  float   r_xz = -ry_s;
  float   r_yx = (rx_s * ry_s * rz_c) - (rx_c * rz_s);
  float   r_yy = (rx_s * ry_s * rz_s) + (rx_c * rz_c);
  float   r_yz = rx_s * ry_c;
  float   r_zx = (rx_c * ry_s * rz_c) + (rx_s * rz_s);
  float   r_zy = (rx_c * ry_s * rz_s) - (rx_s * rz_c);
  float   r_zz = rx_c * ry_c;
  float   x[8];
  float   y[8];
  float   z[8];
  for (int i = 0; i < 8; i++)
  {
    float   xf = float(!(i & 1) ? x1 : x2);
    float   yf = float(!(i & 2) ? y1 : y2);
    float   zf = float(!(i & 4) ? z1 : z2);
    x[i] = float(x0) + (xf * r_xx) + (yf * r_xy) + (zf * r_xz);
    y[i] = float(y0) + (xf * r_yx) + (yf * r_yy) + (zf * r_yz);
    z[i] = float(z0) + (xf * r_zx) + (yf * r_zy) + (zf * r_zz);
  }
  static const unsigned char v[24] =
  {
    0, 1, 3, 2,  0, 1, 5, 4,  0, 2, 6, 4,  1, 3, 7, 5,  2, 3, 7, 6,  4, 5, 7, 6
  };
  for (int i = 0; i < 6; i++)
  {
    unsigned char c1 = v[i << 2];
    unsigned char c2 = v[(i << 2) + 1];
    unsigned char c3 = v[(i << 2) + 2];
    unsigned char c4 = v[(i << 2) + 3];
    fillPolygon(x[c1], y[c1], z[c1], x[c2], y[c2], z[c2],
                x[c3], y[c3], z[c3], x[c4], y[c4], z[c4]);
  }
}

void findWater(ESMFile& esmFile, unsigned int formID)
{
  do
  {
    const ESMFile::ESMRecord& r = esmFile.getRecord(formID);
    if (r.children)
    {
      if (r == "GRUP" &&
          (r.formID == 2 || r.formID == 3 ||
           (r.formID == 1 && r.flags != worldFormID)))
      {
        formID = r.next;
        continue;
      }
      findWater(esmFile, r.children);
    }
    if (r == "REFR")
    {
      unsigned int  n = 0;
      float   x = 0.0f;
      float   y = 0.0f;
      float   z = 0.0f;
      int     x1 = 0;
      int     y1 = 0;
      int     z1 = 0;
      int     x2 = 0;
      int     y2 = 0;
      int     z2 = 0;
      float   rx = 0.0f;
      float   ry = 0.0f;
      float   rz = 0.0f;
      float   scale = 1.0f;
      bool    isWater = false;
      ESMFile::ESMField f(esmFile, r);
      while (f.next())
      {
        if (f == "NAME" && f.size() >= 4)
        {
          n = f.readUInt32();
        }
        else if (f == "DATA" && f.size() >= 24)
        {
          x = f.readFloat();
          y = f.readFloat();
          z = f.readFloat();
          rx = f.readFloat();
          ry = f.readFloat();
          rz = f.readFloat();
        }
        else if (f == "XSCL" && f.size() == 4)
        {
          scale = f.readFloat();
        }
      }
      const ESMFile::ESMRecord  *r2 = esmFile.getRecordPtr(n);
      if (r2 && (*r2 == "ACTI" || *r2 == "PWAT"))
      {
        isWater = (*r2 == "PWAT");
        ESMFile::ESMField f2(esmFile, *r2);
        while (f2.next())
        {
          if (f2 == "WNAM" && f2.size() >= 4)
          {
            isWater = true;
          }
          else if (f2 == "OBND" && f2.size() >= 12)
          {
            x1 = f2.readInt16();
            y1 = f2.readInt16();
            z1 = f2.readInt16();
            x2 = f2.readInt16();
            y2 = f2.readInt16();
            z2 = f2.readInt16();
          }
        }
      }
      if (isWater)
      {
        x1 = roundFloat(float(x1) * scale);
        y1 = roundFloat(float(y1) * scale);
        z1 = roundFloat(float(z1) * scale);
        x2 = roundFloat(float(x2) * scale);
        y2 = roundFloat(float(y2) * scale);
        z2 = roundFloat(float(z2) * scale);
        fillWater(int(x), int(y), int(z), x1, y1, z1, x2, y2, z2, rx, ry, rz);
      }
    }
    if (r == "CELL")
    {
      unsigned int  flags = 0;
      float   x = 0.0f;
      float   y = 0.0f;
      float   z = 0.0f;
      ESMFile::ESMField f(esmFile, r);
      while (f.next())
      {
        if (f == "DATA" && f.size() >= 1)
        {
          flags = f.readUInt8();
        }
        else if (f == "XCLC" && f.size() >= 8)
        {
          x = float(f.readInt32()) * 4096.0f;
          y = float(f.readInt32()) * 4096.0f;
        }
        else if (f == "XCLW" && f.size() >= 4)
        {
          z = f.readFloat();
        }
      }
      if (((flags & 3) == 2) && z >= -1000000.0f && z <= 1000000.0f)
      {
        fillWaterCell(int(x), int(y), int(z));
      }
    }
    formID = r.next;
  }
  while (formID);
}

int main(int argc, char **argv)
{
  if (argc < 3 || argc > 5)
  {
    std::fprintf(stderr,
                 "Usage: findwater INFILE.ESM[,...] OUTFILE.DDS "
                 "[HMAPFILE.DDS [worldID]]\n");
    return 1;
  }
  try
  {
    unsigned int  hdrBuf[11];
    hdrBuf[0] = 0x36374F46;             // "FO76"
    hdrBuf[10] = 0;
    if (argc > 3)
    {
      int     pixelFormat = 0;
      DDSInputFile  inFile(argv[3], landWidth, landHeight, pixelFormat, hdrBuf);
      // "FO", "LAND"
      if ((hdrBuf[0] & 0xFFFF) == 0x4F46 && hdrBuf[1] == 0x444E414C)
      {
        cellMinX = uint32ToSigned(hdrBuf[2]);
        cellMinY = uint32ToSigned(hdrBuf[3]);
        zMin = float(uint32ToSigned(hdrBuf[4]));
        cellMaxX = uint32ToSigned(hdrBuf[5]);
        cellMaxY = uint32ToSigned(hdrBuf[6]);
        zMax = float(uint32ToSigned(hdrBuf[7]));
        defaultWaterLevel = float(uint32ToSigned(hdrBuf[8]));
        cellSize = int(hdrBuf[9]);
        if (hdrBuf[0] != 0x36374F46)
          worldFormID = 0x0000003C;
      }
      else
      {
        hdrBuf[0] = 0x36374F46;         // default to "FO76"
      }
    }
    if (argc > 4)
    {
      worldFormID = (unsigned int) parseInteger(argv[4], 0,
                                                "invalid world form ID",
                                                0L, 0x0FFFFFFFL);
    }
    hdrBuf[1] = 0x444E414C;             // "LAND"
    hdrBuf[2] = (unsigned int) cellMinX;
    hdrBuf[3] = (unsigned int) cellMinY;
    hdrBuf[4] = (unsigned int) roundFloat(zMin);
    hdrBuf[5] = (unsigned int) cellMaxX;
    hdrBuf[6] = (unsigned int) cellMaxY;
    hdrBuf[7] = (unsigned int) roundFloat(zMax);
    hdrBuf[8] = (unsigned int) roundFloat(defaultWaterLevel);
    hdrBuf[9] = (unsigned int) cellSize;
    landWidth = (cellMaxX + 1 - cellMinX) * cellSize;
    landHeight = (cellMaxY + 1 - cellMinY) * cellSize;
    zRangeOffs = -zMin;
    zRangeScale = 65535.0f / (zMax - zMin);
    xyRangeScale = float(cellSize) / 4096.0f;
    cellOffsetX = -(cellMinX * cellSize);
    cellOffsetY = (cellMaxY + 1) * cellSize - 1;
    if (hdrBuf[0] == 0x36374F46)
    {
      // half cell offset for BTD landscape format
      cellOffsetX = cellOffsetX + (cellSize >> 1);
      cellOffsetY = cellOffsetY - (cellSize >> 1);
    }
    waterHeightMap.resize(size_t(landWidth) * size_t(landHeight),
                          (unsigned short) convertZ(defaultWaterLevel));

    ESMFile esmFile(argv[1]);
    findWater(esmFile, 0);

    DDSOutputFile outFile(argv[2], landWidth, landHeight,
                          DDSInputFile::pixelFormatGRAY16, hdrBuf);
    for (size_t i = 0; i < waterHeightMap.size(); i++)
    {
      outFile.writeByte((unsigned char) (waterHeightMap[i] & 0xFF));
      outFile.writeByte((unsigned char) (waterHeightMap[i] >> 8));
    }
    outFile.flush();
  }
  catch (std::exception& e)
  {
    std::fprintf(stderr, "findwater: %s\n", e.what());
    return 1;
  }
  return 0;
}

