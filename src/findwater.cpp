
#include "filebuf.hpp"
#include "esmfile.hpp"
#include "ba2file.hpp"
#include "nif_file.hpp"
#include "plot3d.hpp"

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

static BA2File  *meshArchiveFile = (BA2File *) 0;
static std::vector< NIFFile * > nifFiles;
static std::map< unsigned int, std::vector< NIFFile::NIFTriShape > >
    waterMeshes;
static std::vector< std::vector< NIFFile::NIFVertex > > obndVertexBuf;
static std::vector< NIFFile::NIFTriangle >  obndTriangleBuf;

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

struct WaterHeightMap
{
  static inline void drawPixel(int x, int y, float z)
  {
    if (x < 0 || x >= landWidth || y < 0 || y >= landHeight)
      return;
    size_t  offs = (unsigned int) y * (unsigned int) landWidth + size_t(x);
    unsigned short  zi = (unsigned short) convertZ(z);
    if (zi > waterHeightMap[offs])
      waterHeightMap[offs] = zi;
  }
};

static void fillWaterCell(int x0, int y0, int z0)
{
  int     x1 = convertX(float(x0 + 4096)) - 1;
  int     y1 = convertY(float(y0 + 4096)) + 1;
  x0 = convertX(float(x0));
  y0 = convertY(float(y0));
  if (x0 > x1 || x1 < 0 || x0 >= landWidth ||
      y0 < y1 || y0 < 0 || y1 >= landHeight)
  {
    return;
  }
  Plot3D< WaterHeightMap, float > plot3d;
  plot3d.drawRectangle(x0, y0, x1, y1, float(z0));
}

static void fillWaterMesh(const NIFFile::NIFTriShape *meshData, size_t meshCnt,
                          const NIFFile::NIFVertexTransform& vertexTransform)
{
  for (size_t i = 0; i < meshCnt; i++)
  {
#if 0
    if (!(meshData[i].flags & 0x02))
      continue;
#endif
    NIFFile::NIFVertexTransform t = meshData[i].vertexTransform;
    t *= vertexTransform;
    for (size_t j = 0; j < meshData[i].triangleCnt; j++)
    {
      NIFFile::NIFVertex  v0 =
          meshData[i].vertexData[meshData[i].triangleData[j].v0];
      NIFFile::NIFVertex  v1 =
          meshData[i].vertexData[meshData[i].triangleData[j].v1];
      NIFFile::NIFVertex  v2 =
          meshData[i].vertexData[meshData[i].triangleData[j].v2];
      t.transformXYZ(v0.x, v0.y, v0.z);
      t.transformXYZ(v1.x, v1.y, v1.z);
      t.transformXYZ(v2.x, v2.y, v2.z);
      int     x0 = convertX(v0.x);
      int     y0 = convertY(v0.y);
      int     x1 = convertX(v1.x);
      int     y1 = convertY(v1.y);
      int     x2 = convertX(v2.x);
      int     y2 = convertY(v2.y);
      if (!((x0 < 0 && x1 < 0 && x2 < 0) || (y0 < 0 && y1 < 0 && y2 < 0) ||
            (x0 >= landWidth && x1 >= landWidth && x2 >= landWidth) ||
            (y0 >= landHeight && y1 >= landHeight && y2 >= landHeight)))
      {
        Plot3D< WaterHeightMap, float > plot3d;
        plot3d.drawTriangle(x0, y0, v0.z, x1, y1, v1.z, x2, y2, v2.z);
      }
    }
  }
}

static void createMeshFromOBND(std::vector< NIFFile::NIFTriShape >& meshData,
                               int x1, int y1, int z1, int x2, int y2, int z2)
{
  meshData.resize(1);
  obndVertexBuf.push_back(std::vector< NIFFile::NIFVertex >());
  std::vector< NIFFile::NIFVertex >&  vertexData = obndVertexBuf.back();
  vertexData.resize(8);
  for (unsigned char i = 0; i < 8; i++)
  {
    vertexData[i].x = float(!(i & 1) ? x1 : x2);
    vertexData[i].y = float(!(i & 2) ? y1 : y2);
    vertexData[i].z = float(!(i & 4) ? z1 : z2);
  }
  if (obndTriangleBuf.size() != 12)
  {
    obndTriangleBuf.resize(12);
    for (unsigned char i = 0; i < 12; i++)
    {
      obndTriangleBuf[i].v0 =
          (unsigned short) ((0x0000442211000000ULL >> (i << 2)) & 7U);
      obndTriangleBuf[i].v1 =
          (unsigned short) ((0x0000656353424121ULL >> (i << 2)) & 7U);
      obndTriangleBuf[i].v2 =
          (unsigned short) ((0x0000777777665533ULL >> (i << 2)) & 7U);
    }
  }
  meshData[0].vertexCnt = (unsigned int) vertexData.size();
  meshData[0].triangleCnt = (unsigned int) obndTriangleBuf.size();
  meshData[0].vertexData = vertexData.data();
  meshData[0].triangleData = obndTriangleBuf.data();
  meshData[0].m.flags = BGSMFile::Flag_TSWater | BGSMFile::Flag_TSAlphaBlending
                        | BGSMFile::Flag_TwoSided;
}

static std::vector< NIFFile::NIFTriShape >& getMeshData(ESMFile& esmFile,
                                                        unsigned int formID)
{
  std::map< unsigned int, std::vector< NIFFile::NIFTriShape > >::iterator i =
      waterMeshes.find(formID);
  if (i != waterMeshes.end())
    return i->second;
  i = waterMeshes.insert(
          std::pair< unsigned int, std::vector< NIFFile::NIFTriShape > >(
              formID, std::vector< NIFFile::NIFTriShape >())).first;
  const ESMFile::ESMRecord& r = esmFile.getRecord(formID);
  if (!(r == "ACTI" || r == "MSTT" || r == "PWAT" || r == "STAT"))
    return i->second;
  ESMFile::ESMField f(esmFile, r);
  int     x1 = 0;
  int     y1 = 0;
  int     z1 = 0;
  int     x2 = 0;
  int     y2 = 0;
  int     z2 = 0;
  std::string modelPath;
  bool    isWater = (r == "PWAT");
  while (f.next())
  {
    if (f == "OBND" && f.size() >= 12)
    {
      x1 = uint16ToSigned(f.readUInt16Fast());
      y1 = uint16ToSigned(f.readUInt16Fast());
      z1 = uint16ToSigned(f.readUInt16Fast());
      x2 = uint16ToSigned(f.readUInt16Fast());
      y2 = uint16ToSigned(f.readUInt16Fast());
      z2 = uint16ToSigned(f.readUInt16Fast());
    }
    else if (f == "MODL" && f.size() >= 1)
    {
      modelPath.clear();
      for (size_t j = 0; j < f.size(); j++)
      {
        char    c = char(f.readUInt8Fast());
        if (!c)
          break;
        if ((unsigned char) c < 0x20)
          c = ' ';
        else if (c >= 'A' && c <= 'Z')
          c = c + ('a' - 'A');
        else if (c == '\\')
          c = '/';
        modelPath += c;
      }
    }
    else if (f == "WNAM" && f.size() >= 4)
    {
      isWater = true;
    }
  }
  if (modelPath.length() < 8 ||
      std::strncmp(modelPath.c_str(), "meshes/", 7) != 0)
  {
    modelPath.insert(0, "meshes/");
  }
  if (meshArchiveFile && modelPath.length() >= 18 &&
      (std::strncmp(modelPath.c_str(), "meshes/water/", 13) == 0 ||
       modelPath.find("/fxwaterfall") != std::string::npos) &&
      std::strcmp(modelPath.c_str() + (modelPath.length() - 4), ".nif") == 0)
  {
    isWater = true;
  }
  else
  {
    modelPath.clear();
  }
  if (meshArchiveFile && !modelPath.empty())
  {
    NIFFile *nifFile = nullptr;
    try
    {
      BA2File::UCharArray fileBuf;
      meshArchiveFile->extractFile(fileBuf, modelPath);
      nifFile = new NIFFile(fileBuf.data, fileBuf.size);
      nifFile->getMesh(i->second);
      nifFiles.push_back(nifFile);
      return i->second;
    }
    catch (...)
    {
      if (nifFile)
        delete nifFile;
      i->second.clear();
      return i->second;
    }
  }
  if (isWater && (x1 | y1 | z1 | x2 | y2 | z2) != 0)
    createMeshFromOBND(i->second, x1, y1, z1, x2, y2, z2);
  return i->second;
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
      float   rx = 0.0f;
      float   ry = 0.0f;
      float   rz = 0.0f;
      float   scale = 1.0f;
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
      const ESMFile::ESMRecord  *r2 = esmFile.findRecord(n);
      if (r2 &&
          (*r2 == "ACTI" || *r2 == "MSTT" || *r2 == "PWAT" || *r2 == "STAT"))
      {
        std::vector< NIFFile::NIFTriShape >&  meshData =
            getMeshData(esmFile, n);
        if (meshData.size() > 0)
        {
          NIFFile::NIFVertexTransform t(scale, rx, ry, rz, x, y, z);
          fillWaterMesh(meshData.data(), meshData.size(), t);
        }
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

static bool archiveFilterFunction(void *p, const std::string_view& s)
{
  (void) p;
  return (s.find("water") != std::string_view::npos);
}

int main(int argc, char **argv)
{
  if (argc < 3)
  {
    std::fprintf(stderr,
                 "Usage: findwater INFILE.ESM[,...] OUTFILE.DDS "
                 "[OPTIONS...]\n");
    std::fprintf(stderr, "Options:\n");
    std::fprintf(stderr, "    HMAPFILE.DDS    Height map input file\n");
    std::fprintf(stderr, "    ARCHIVEPATH     Path to water mesh archive(s)\n");
    std::fprintf(stderr, "    worldID         Form ID of world\n");
    return 1;
  }
  try
  {
    std::string   hmapFileName;
    std::vector< std::string >  meshArchivePaths;
    unsigned int  worldID = 0U;
    for (int i = 3; i < argc; i++)
    {
      try
      {
        worldID = (unsigned int) parseInteger(argv[i]);
      }
      catch (...)
      {
        std::string tmp(argv[i]);
        std::string s;
        size_t  n = tmp.rfind('.');
        if (n != std::string::npos)
        {
          for ( ; n < tmp.length(); n++)
          {
            char    c = tmp[n];
            if (c >= 'A' && c <= 'Z')
              c = c + ('a' - 'A');
            s += c;
          }
        }
        if (s == ".ba2" || s == ".bsa" || s.empty() || s.length() > 5)
          meshArchivePaths.push_back(tmp);
        else
          hmapFileName = tmp;
      }
    }
    if (!meshArchivePaths.empty())
    {
      meshArchiveFile = new BA2File();
      for (std::vector< std::string >::const_iterator
               i = meshArchivePaths.end(); i != meshArchivePaths.begin(); )
      {
        i--;
        meshArchiveFile->loadArchivePath(i->c_str(), &archiveFilterFunction);
      }
    }
    unsigned int  hdrBuf[11];
    hdrBuf[0] = 0x36374F46;             // "FO76"
    hdrBuf[10] = 0;
    if (!hmapFileName.empty())
    {
      int     pixelFormat = 0;
      DDSInputFile  inFile(hmapFileName.c_str(),
                           landWidth, landHeight, pixelFormat, hdrBuf);
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
    if (worldID)
    {
      if (worldID & ~0x0FFFFFFFU)
        errorMessage("invalid world form ID");
      worldFormID = worldID;
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
    for (size_t i = 0; i < nifFiles.size(); i++)
    {
      if (nifFiles[i])
        delete nifFiles[i];
    }
    if (meshArchiveFile)
      delete meshArchiveFile;
    std::fprintf(stderr, "findwater: %s\n", e.what());
    return 1;
  }
  for (size_t i = 0; i < nifFiles.size(); i++)
  {
    if (nifFiles[i])
      delete nifFiles[i];
  }
  if (meshArchiveFile)
    delete meshArchiveFile;
  return 0;
}

