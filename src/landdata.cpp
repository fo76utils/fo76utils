
#include "common.hpp"
#include "landdata.hpp"

void LandscapeData::allocateDataBuf()
{
  size_t  vertexCnt = size_t(getImageWidth()) * size_t(getImageHeight());
  size_t  hmapDataSize = vertexCnt * sizeof(std::uint16_t);
  size_t  ltexData16Size = vertexCnt * sizeof(std::uint16_t);
  size_t  txtSetDataSize =
      size_t(cellMaxX + 1 - cellMinX) * size_t(cellMaxY + 1 - cellMinY) * 64;
  size_t  totalDataSize = (hmapDataSize + ltexData16Size + txtSetDataSize
                           + sizeof(std::uint32_t) - 1) / sizeof(std::uint32_t);
  if (totalDataSize < 1)
    return;
  dataBuf.resize(totalDataSize);
  unsigned char *p = reinterpret_cast< unsigned char * >(dataBuf.data());
  if (hmapDataSize)
  {
    hmapData = reinterpret_cast< std::uint16_t * >(p);
    p = p + hmapDataSize;
  }
  if (ltexData16Size)
  {
    ltexData16 = reinterpret_cast< std::uint16_t * >(p);
    p = p + ltexData16Size;
  }
  if (txtSetDataSize)
  {
    txtSetData = p;
    for (size_t i = 0; i < txtSetDataSize; i++)
      txtSetData[i] = 0xFF;
  }
}

void LandscapeData::loadBTDFile(
    const unsigned char *btdFileData, size_t btdFileSize,
    unsigned char mipLevel)
{
  if (mipLevel > 4)
    errorMessage("LandscapeData: invalid mip level");
  cellResolution = 128 >> mipLevel;
  BTDFile btdFile(btdFileData, btdFileSize);
  if (btdFile.getCellMinX() > cellMinX)
    cellMinX = btdFile.getCellMinX();
  if (btdFile.getCellMinY() > cellMinY)
    cellMinY = btdFile.getCellMinY();
  if (btdFile.getCellMaxX() < cellMaxX)
    cellMaxX = btdFile.getCellMaxX();
  if (btdFile.getCellMaxY() < cellMaxY)
    cellMaxY = btdFile.getCellMaxY();
  zMin = btdFile.getMinHeight();
  zMax = btdFile.getMaxHeight();
  size_t  ltexCnt = btdFile.getLandTextureCount();
  ltexFormIDs.resize(ltexCnt);
  for (size_t i = 0; i < ltexCnt; i++)
    ltexFormIDs[i] = btdFile.getLandTexture(i);
  ltexEDIDs.resize(ltexCnt);
  materialPaths.resize(ltexCnt);
  ltexMaterials.resize(ltexCnt, (CE2Material *) 0);
  allocateDataBuf();
  size_t  n = size_t(cellResolution);
  unsigned char m = 7 - mipLevel;
  std::vector< std::uint16_t >  tmpBuf(n * n);
  int     x0 = cellMinX;
  int     y0 = cellMaxY;
  int     x = cellMinX;
  int     y = cellMaxY;
  while (true)
  {
    size_t  w = size_t(cellMaxX + 1 - cellMinX);
    // height map
    std::uint16_t *bufp16 = tmpBuf.data();
    btdFile.getCellHeightMap(bufp16, x, y, mipLevel);
    for (size_t yy = 0; yy < n; yy++)
    {
      size_t  offs = size_t((cellMaxY - y) << m) | (~yy & (n - 1));
      offs = ((offs * w) + size_t(x - cellMinX)) << m;
      for (size_t xx = 0; xx < n; xx++, bufp16++, offs++)
        hmapData[offs] = *bufp16;
    }
    // land texture
    bufp16 = tmpBuf.data();
    btdFile.getCellLandTexture(bufp16, x, y, mipLevel);
    for (size_t yy = 0; yy < n; yy++)
    {
      size_t  offs = size_t((cellMaxY - y) << m) | (~yy & (n - 1));
      offs = ((offs * w) + size_t(x - cellMinX)) << m;
      for (size_t xx = 0; xx < n; xx++, bufp16++, offs++)
        ltexData16[offs] = *bufp16;
    }
    // texture set
    unsigned char *bufp8 = reinterpret_cast< unsigned char * >(tmpBuf.data());
    btdFile.getCellTextureSet(bufp8, x, y);
    for (size_t yy = 0; yy < 2; yy++)
    {
      size_t  offs = size_t((cellMaxY - y) << 1) | (~yy & 1);
      offs = ((offs * w) + size_t(x - cellMinX)) << 5;
      for (size_t xx = 0; xx < 32; xx++, bufp8++, offs++)
        txtSetData[offs] = *bufp8;
    }
    x++;
    if (x > cellMaxX || ((x - btdFile.getCellMinX()) & 7) == 0)
    {
      y--;
      if (y < cellMinY || ((y - btdFile.getCellMinY()) & 7) == 7)
      {
        if (x > cellMaxX)
        {
          if (y < cellMinY)
            break;
          x = cellMinX;
          y0 = y;
        }
        x0 = x;
        y = y0;
      }
      x = x0;
    }
  }
}

void LandscapeData::loadTextureInfo(
    ESMFile& esmFile, const CE2MaterialDB *materials, size_t n)
{
  const ESMFile::ESMRecord  *r = esmFile.findRecord(ltexFormIDs[n]);
  if (!r)
    return;
  std::string stringBuf;
  ESMFile::ESMField f(esmFile, *r);
  while (f.next())
  {
    if (f == "EDID" && ltexEDIDs[n].empty())
    {
      f.readString(ltexEDIDs[n]);
    }
    else if (f == "BNAM" && *r == "LTEX")
    {
      f.readPath(stringBuf, std::string::npos, "materials/", ".mat");
      materialPaths[n] = stringBuf;
    }
  }
  if (materials && !materialPaths[n].empty())
    ltexMaterials[n] = materials->findMaterial(materialPaths[n]);
}

unsigned int LandscapeData::loadWorldInfo(ESMFile& esmFile,
                                          unsigned int worldID)
{
  const ESMFile::ESMRecord  *r = esmFile.findRecord(worldID);
  if (!(r && *r == "WRLD"))
    errorMessage("LandscapeData: world not found in ESM file");
  unsigned int  parentID = 0U;
  ESMFile::ESMField f(esmFile, *r);
  while (f.next())
  {
    if (f == "DNAM" && f.size() >= 8)
    {
      landLevel = f.readFloat();
      waterLevel = f.readFloat();
    }
    else if (f == "NAM3" && f.size() >= 4)
    {
      waterFormID = f.readUInt32Fast();
    }
    else if (f == "WNAM" && f.size() >= 4)
    {
      unsigned int  tmp = f.readUInt32Fast();
      if (tmp && tmp < worldID)
      {
        parentID = tmp;
        loadWorldInfo(esmFile, parentID);
      }
    }
  }
  return parentID;
}

LandscapeData::LandscapeData(
    ESMFile *esmFile, const char *btdFileName, const BA2File *ba2File,
    const CE2MaterialDB *materials, unsigned int worldID, int mipLevel,
    int xMin, int yMin, int xMax, int yMax)
  : cellMinX(xMin),
    cellMinY(yMin),
    cellMaxX(xMax),
    cellMaxY(yMax),
    cellResolution(32),
    hmapData((std::uint16_t *) 0),
    ltexData16((std::uint16_t *) 0),
    txtSetData((unsigned char *) 0),
    zMin(1.0e9f),
    zMax(-1.0e9f),
    waterLevel(0.0f),
    landLevel(1024.0f),
    waterFormID(0x00000018U)
{
  unsigned char l = (unsigned char) mipLevel;
  if (esmFile && esmFile->getESMVersion() <= 0xFFU)
    errorMessage("LandscapeData: unsupported ESM file version");
  if (!btdFileName)
    btdFileName = "";
  if (!worldID)
    worldID = 0x0001251BU;              // "NewAtlantis"
  if (esmFile)
    (void) loadWorldInfo(*esmFile, worldID);
  std::string btdFullName;
  const char  *s = btdFileName;
  size_t  n = std::strlen(s);
  if (!(n > 4 &&                        // ".BTD"
        (FileBuffer::readUInt32Fast(s + (n - 4)) & 0xDFDFDFFFU) == 0x4454422EU))
  {
    if (!ba2File)
    {
      btdFullName = btdFileName;
      if (!btdFullName.empty())
      {
        if (btdFullName.back() != '/'
#if defined(_WIN32) || defined(_WIN64)
            && btdFullName.back() != '\\' && btdFullName.back() != ':'
#endif
            )
        {
          btdFullName += '/';
        }
      }
      btdFullName += "Terrain/";
    }
    else
    {
      btdFullName = "terrain/";
    }
    bool    nameFound = false;
    if (esmFile)
    {
      const ESMFile::ESMRecord  *r = esmFile->findRecord(worldID);
      if (r && *r == "WRLD")
      {
        ESMFile::ESMField f(*esmFile, *r);
        while (f.next())
        {
          if (f == "EDID" && f.size() > 0)
          {
            for (size_t i = f.size(); i > 0; i--)
            {
              char    c = char(f.readUInt8Fast());
              if (!c)
                break;
              if (!((unsigned char) c >= 0x20 && (unsigned char) c < 0x7F))
                c = '_';
              if (ba2File && c >= 'A' && c <= 'Z')
                c = c + ('a' - 'A');
              btdFullName += c;
            }
            nameFound = true;
            break;
          }
        }
      }
    }
    if (!nameFound)
      errorMessage("LandscapeData: no BTD file name");
    else
      btdFullName += ".btd";
    s = btdFullName.c_str();
  }
  if (ba2File)
  {
    std::vector< unsigned char >  btdBuf;
    const unsigned char *btdFileData = (unsigned char *) 0;
    size_t  btdFileSize =
        ba2File->extractFile(btdFileData, btdBuf, std::string(s));
    loadBTDFile(btdFileData, btdFileSize, l);
  }
  else
  {
    FileBuffer  btdBuf(s);
    loadBTDFile(btdBuf.data(), btdBuf.size(), l);
  }
  if (esmFile)
  {
    for (size_t i = 0; i < ltexFormIDs.size(); i++)
      loadTextureInfo(*esmFile, materials, i);
  }
}

LandscapeData::~LandscapeData()
{
}

