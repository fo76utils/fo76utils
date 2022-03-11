
#include "common.hpp"
#include "landdata.hpp"

void LandscapeData::allocateDataBuf(unsigned int formatMask, bool isFO76)
{
  size_t  hmapDataSize = 0;
  size_t  ltexDataSize = 0;
  size_t  vnmlDataSize = 0;
  size_t  vclrData24Size = 0;
  size_t  vclrData16Size = 0;
  size_t  gcvrDataSize = 0;
  size_t  vertexCnt = size_t(getImageWidth()) * size_t(getImageHeight());
  if (formatMask & 0x01)
    hmapDataSize = vertexCnt * sizeof(unsigned short);
  if (formatMask & 0x02)
    ltexDataSize = vertexCnt * sizeof(unsigned int);
  if (!isFO76)
  {
    if (formatMask & 0x04)
      vnmlDataSize = vertexCnt * 3;
    if (formatMask & 0x08)
      vclrData24Size = vertexCnt * 3;
  }
  else
  {
    if (formatMask & 0x08)
    {
      vclrData16Size = vertexCnt * sizeof(unsigned short);
      if (cellResolution >= 128)
        vclrData16Size = vclrData16Size >> 4;
      else if (cellResolution >= 64)
        vclrData16Size = vclrData16Size >> 2;
    }
    if (formatMask & 0x10)
      gcvrDataSize = vertexCnt * sizeof(unsigned short);
  }
  size_t  totalDataSize = (hmapDataSize + ltexDataSize) / sizeof(unsigned int);
  totalDataSize += ((vnmlDataSize + vclrData24Size) / sizeof(unsigned int));
  totalDataSize += ((vclrData16Size + gcvrDataSize) / sizeof(unsigned int));
  if (totalDataSize < 1)
    return;
  dataBuf.resize(totalDataSize);
  unsigned char *p = reinterpret_cast< unsigned char * >(&(dataBuf.front()));
  if (hmapDataSize)
  {
    hmapData = reinterpret_cast< unsigned short * >(p);
    p = p + hmapDataSize;
  }
  if (ltexDataSize)
  {
    ltexData = reinterpret_cast< unsigned int * >(p);
    p = p + ltexDataSize;
  }
  if (vnmlDataSize)
  {
    vnmlData = p;
    p = p + vnmlDataSize;
  }
  if (vclrData24Size)
  {
    vclrData24 = p;
    p = p + vclrData24Size;
  }
  if (vclrData16Size)
  {
    vclrData16 = reinterpret_cast< unsigned short * >(p);
    p = p + vclrData16Size;
  }
  if (gcvrDataSize)
    gcvrData = reinterpret_cast< unsigned short * >(p);
}

void LandscapeData::loadBTDFile(const char *btdFileName,
                                unsigned int formatMask, unsigned char mipLevel)
{
  if (mipLevel < ((formatMask & 0x1B) == 0x08 ? 2 : 0) ||
      mipLevel > (!(formatMask & 0x12) ? 4 : 3))
  {
    throw errorMessage("LandscapeData: invalid mip level");
  }
  cellResolution = 128 >> mipLevel;
  cellOffset = 64 >> mipLevel;
  BTDFile btdFile(btdFileName);
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
  size_t  ltexCnt = 0;
  size_t  gcvrCnt = 0;
  if (formatMask & 0x02)
    ltexCnt = btdFile.getLandTextureCount();
  if (formatMask & 0x10)
    gcvrCnt = btdFile.getGroundCoverCount();
  ltexFormIDs.resize(ltexCnt + gcvrCnt);
  for (size_t i = 0; i < ltexCnt; i++)
    ltexFormIDs[i] = btdFile.getLandTexture(i);
  for (size_t i = 0; i < gcvrCnt; i++)
    ltexFormIDs[ltexCnt + i] = btdFile.getGroundCover(i);
  ltexEDIDs.resize(ltexCnt + gcvrCnt);
  ltexBGSMPaths.resize(ltexCnt + gcvrCnt);
  ltexDPaths.resize(ltexCnt + gcvrCnt);
  ltexNPaths.resize(ltexCnt + gcvrCnt);
  allocateDataBuf(formatMask, true);
  size_t  n = size_t(cellResolution);
  unsigned char m = 7 - mipLevel;
  std::vector< unsigned int > tmpBuf(n * n);
  int     x0 = cellMinX;
  int     y0 = cellMaxY;
  int     x = cellMinX;
  int     y = cellMaxY;
  while (true)
  {
    unsigned int    *bufp32 = &(tmpBuf.front());
    unsigned char   *bufp8 = reinterpret_cast< unsigned char * >(bufp32);
    unsigned short  *bufp16 = reinterpret_cast< unsigned short * >(bufp8);
    size_t  w = size_t(cellMaxX + 1 - cellMinX);
    if (formatMask & 0x01)              // height map
    {
      btdFile.getCellHeightMap(bufp16, x, y, mipLevel);
      for (size_t yy = 0; yy < n; yy++)
      {
        size_t  offs = size_t((cellMaxY - y) << m) | (~yy & (n - 1));
        offs = ((offs * w) + size_t(x - cellMinX)) << m;
        for (size_t xx = 0; xx < n; xx++, bufp16++, offs++)
          hmapData[offs] = *bufp16;
      }
    }
    if (formatMask & 0x02)              // land texture
    {
      btdFile.getCellLandTexture(bufp32, x, y, mipLevel);
      for (size_t yy = 0; yy < n; yy++)
      {
        size_t  offs = size_t((cellMaxY - y) << m) | (~yy & (n - 1));
        offs = ((offs * w) + size_t(x - cellMinX)) << m;
        for (size_t xx = 0; xx < n; xx++, bufp32++, offs++)
          ltexData[offs] = *bufp32;
      }
    }
    if (formatMask & 0x08)              // terrain color
    {
      if (mipLevel > 2)
      {
        btdFile.getCellTerrainColor(bufp16, x, y, mipLevel);
        for (size_t yy = 0; yy < n; yy++)
        {
          size_t  offs = size_t((cellMaxY - y) << m) | (~yy & (n - 1));
          offs = ((offs * w) + size_t(x - cellMinX)) << m;
          for (size_t xx = 0; xx < n; xx++, bufp16++, offs++)
            vclrData16[offs] = *bufp16;
        }
      }
      else
      {
        btdFile.getCellTerrainColor(bufp16, x, y, 2);
        for (size_t yy = 0; yy < 32; yy++)
        {
          size_t  offs = size_t((cellMaxY - y) << 5) | (31U - yy);
          offs = ((offs * w) + size_t(x - cellMinX)) << 5;
          for (size_t xx = 0; xx < 32; xx++, bufp16++, offs++)
            vclrData16[offs] = *bufp16;
        }
      }
    }
    if (formatMask & 0x10)              // ground cover
    {
      btdFile.getCellGroundCover(bufp16, x, y, mipLevel);
      for (size_t yy = 0; yy < n; yy++)
      {
        size_t  offs = size_t((cellMaxY - y) << m) | (~yy & (n - 1));
        offs = ((offs * w) + size_t(x - cellMinX)) << m;
        for (size_t xx = 0; xx < n; xx++, bufp16++, offs++)
        {
          unsigned int  tmp = *bufp16;
          if (~tmp & 0xFFU)
            tmp += (unsigned int) ltexCnt;
          gcvrData[offs] = (unsigned short) tmp;
        }
      }
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

void LandscapeData::findESMLand(ESMFile& esmFile,
                                std::vector< unsigned long long >& landList,
                                unsigned int formID, int d, int x, int y)
{
  const ESMFile::ESMRecord  *r = (ESMFile::ESMRecord *) 0;
  for ( ; formID; formID = r->next)
  {
    r = esmFile.getRecordPtr(formID);
    if (!r)
      break;
    if (*r == "GRUP")
    {
      if (!(r->children && r->formID >= 1 && r->formID <= 9))
        continue;
      if (r->formID != 2 && r->formID != 3 && r->formID != 7 && d < 8)
        findESMLand(esmFile, landList, r->children, d + 1, x, y);
    }
    else if (*r == "CELL")
    {
      ESMFile::ESMField f(esmFile, *r);
      while (f.next())
      {
        if (f == "XCLC" && f.size() >= 8)
        {
          x = f.readInt32();
          y = f.readInt32();
        }
      }
    }
    else if (*r == "LAND" &&
             x >= cellMinX && x <= cellMaxX && y >= cellMinY && y <= cellMaxY)
    {
      landList.push_back(((unsigned long long) (y + 32768) << 48)
                         | ((unsigned long long) (x + 32768) << 32)
                         | (unsigned long long) formID);
    }
  }
}

unsigned char LandscapeData::findTextureID(unsigned int formID)
{
  if (!formID)
    return 0xFF;
  for (size_t i = 0; i < ltexFormIDs.size(); i++)
  {
    if (ltexFormIDs[i] == formID)
      return (unsigned char) i;
  }
  if (ltexFormIDs.size() >= 255)
    throw errorMessage("LandscapeData: number of unique land textures > 255");
  ltexFormIDs.push_back(formID);
  return (unsigned char) (ltexFormIDs.size() - 1);
}

void LandscapeData::loadESMFile(ESMFile& esmFile,
                                unsigned int formatMask, unsigned int worldID,
                                unsigned int defTxtID, unsigned char mipLevel)
{
  if (mipLevel != 2)
    throw errorMessage("LandscapeData: invalid mip level");
  formatMask = formatMask & 0x0F;
  std::vector< unsigned long long > landList;
  {
    const ESMFile::ESMRecord  *r = esmFile.getRecordPtr(worldID);
    if (r && *r == "WRLD" && r->next)
    {
      r = esmFile.getRecordPtr(r->next);
      if (r && *r == "GRUP" && r->formID == 1 && r->children)
        findESMLand(esmFile, landList, r->children);
    }
  }
  cellMinX = 32767;
  cellMinY = 32767;
  cellMaxX = -32768;
  cellMaxY = -32768;
  for (size_t i = 0; i < landList.size(); i++)
  {
    int     x = int((landList[i] >> 32) & 0xFFFFU) - 32768;
    int     y = int((landList[i] >> 48) & 0xFFFFU) - 32768;
    cellMinX = (x < cellMinX ? x : cellMinX);
    cellMinY = (y < cellMinY ? y : cellMinY);
    cellMaxX = (x > cellMaxX ? x : cellMaxX);
    cellMaxY = (y > cellMaxY ? y : cellMaxY);
  }
  if (!(cellMaxX >= cellMinX && cellMaxY >= cellMinY))
    throw errorMessage("LandscapeData: world not found in ESM file");
  allocateDataBuf(formatMask, false);
  std::map< unsigned int, unsigned int >  emptyCells;
  std::map< unsigned int, float > cellHeightOffsets;
  for (int y = cellMinY; y <= cellMaxY; y++)
  {
    for (int x = cellMinX; x <= cellMaxX; x++)
    {
      unsigned int  k =
          ((unsigned int) (y + 32768) << 16) | (unsigned int) (x + 32768);
      emptyCells.insert(std::pair< unsigned int, unsigned int >(k, formatMask));
      cellHeightOffsets.insert(std::pair< unsigned int, float >(k, landLevel));
    }
  }
  zMin = landLevel;
  zMax = landLevel;
  std::vector< unsigned int > cellTextures(1024, 0xFFU);
  for (size_t i = 0; i < landList.size(); i++)
  {
    unsigned int  k = (unsigned int) ((landList[i] >> 32) & 0xFFFFFFFFU);
    std::map< unsigned int, unsigned int >::iterator  j = emptyCells.find(k);
    if (j == emptyCells.end())
      continue;
    const ESMFile::ESMRecord  *r =
        esmFile.getRecordPtr((unsigned int) (landList[i] & 0xFFFFFFFFU));
    if (!(r && *r == "LAND"))
      continue;
    int     x = (int(k & 0xFFFFU) - (cellMinX + 32768)) << 5;
    int     y = ((cellMaxY + 32768 - int((k >> 16) & 0xFFFFU)) << 5) + 31;
    size_t  w = size_t(cellMaxX + 1 - cellMinX) << 5;
    size_t  dataOffs = size_t(y) * w + size_t(x);
    unsigned int  cellDataValidMask = 0;
    unsigned int  textureQuadrant = 0;
    unsigned int  textureLayer = 0;
    if (formatMask & 0x02)
    {
      for (size_t l = 0; l < cellTextures.size(); l++)
        cellTextures[l] = 0xFFU;
    }
    ESMFile::ESMField f(esmFile, *r);
    while (f.next())
    {
      if (f == "VHGT" && (formatMask & 0x01) && f.size() >= 1093)
      {
        // vertex heights
        float   zOffs = f.readFloat() * 8.0f;
        cellHeightOffsets[k] = zOffs;
        int     z = 0;
        unsigned short  *p = hmapData + dataOffs;
        for (int yy = 0; yy < 32; yy++, p = p - (w + 32))
        {
          int     tmp = f.readUInt8Fast();
          z += (!(tmp & 0x80) ? tmp : (tmp - 256));
          *(p++) = (unsigned short) (z + 32768);
          float   zf = zOffs + float(z << 3);
          zMin = (zf < zMin ? zf : zMin);
          zMax = (zf > zMax ? zf : zMax);
          int     z0 = z;
          for (int xx = 1; xx < 32; xx++)
          {
            tmp = f.readUInt8Fast();
            z += (!(tmp & 0x80) ? tmp : (tmp - 256));
            *(p++) = (unsigned short) (z + 32768);
            zf = zOffs + float(z << 3);
            zMin = (zf < zMin ? zf : zMin);
            zMax = (zf > zMax ? zf : zMax);
          }
          z = z0;
          (void) f.readUInt8Fast();
        }
        cellDataValidMask |= 1U;
      }
      else if (f == "VNML" && (formatMask & 0x04) && f.size() >= 3267)
      {
        // vertex normals
        unsigned char *p = vnmlData + (dataOffs * 3);
        for (int yy = 0; yy < 32; yy++, p = p - ((w + 32) * 3))
        {
          for (int xx = 0; xx < 32; xx++, p = p + 3)
          {
            p[2] = (unsigned char) (f.readUInt8Fast() ^ 0x80);
            p[1] = (unsigned char) (f.readUInt8Fast() ^ 0x80);
            p[0] = (unsigned char) (f.readUInt8Fast() ^ 0x80);
          }
          f.setPosition(f.getPosition() + 3);
        }
        cellDataValidMask |= 4U;
      }
      else if (f == "VCLR" && (formatMask & 0x08) && f.size() >= 3267)
      {
        // vertex colors
        unsigned char *p = vclrData24 + (dataOffs * 3);
        for (int yy = 0; yy < 32; yy++, p = p - ((w + 32) * 3))
        {
          for (int xx = 0; xx < 32; xx++, p = p + 3)
          {
            p[2] = f.readUInt8Fast();
            p[1] = f.readUInt8Fast();
            p[0] = f.readUInt8Fast();
          }
          f.setPosition(f.getPosition() + 3);
        }
        cellDataValidMask |= 8U;
      }
      else if ((formatMask & 0x02) && f.size() >= 8)
      {
        // vertex textures
        if (f == "BTXT" || f == "ATXT")
        {
          unsigned int  textureID = findTextureID(f.readUInt32Fast());
          textureQuadrant = f.readUInt16Fast() & 3U;
          textureLayer = 0;
          if (f == "ATXT")
            textureLayer = (f.readUInt16Fast() & 7U) + 1;
          for (unsigned int yy = 0; yy < 16; yy++)
          {
            size_t  offs = ((31U - (yy + ((textureQuadrant & 2U) << 3))) << 5)
                           | ((textureQuadrant & 1U) << 4) | textureLayer;
            cellTextures[offs] = (cellTextures[offs] & ~0xFFU) | textureID;
          }
          cellDataValidMask |= 2U;
        }
        else if (f == "VTXT" && textureLayer)
        {
          while ((f.getPosition() + 8) <= f.size())
          {
            unsigned int  n = f.readUInt32Fast() & 0xFFFFU;
            int     a = int(f.readFloat() * 7.0f + 0.5f);
            a = (a >= 0 ? (a <= 7 ? a : 7) : 0);
            unsigned int  xx = n % 17U;
            unsigned int  yy = (n / 17U) % 17U;
            if ((xx | yy) & 16)
              continue;
            size_t  offs = ((31U - (yy + ((textureQuadrant & 2U) << 3))) << 5)
                           | (xx + ((textureQuadrant & 1U) << 4));
            unsigned int  m = 0x00E0U << (textureLayer * 3U);
            cellTextures[offs] = (cellTextures[offs] & ~m)
                                 | (((unsigned int) a * 0x24924900U) & m);
          }
        }
      }
    }
    if (cellDataValidMask & 0x02)
    {
      // convert texture set from 9 to 8 layers
      unsigned int  *p = ltexData + dataOffs;
      for (int yy = 31; yy >= 0; yy--, p = p - (w + 32))
      {
        for (int xx = 0; xx < 32; xx = xx + 8)
        {
          unsigned char blockLayerAlphas[8];
          for (int l = 0; l < 8; l++)
            blockLayerAlphas[l] = 0;
          for (int n = 0; n < 8; n++)
          {
            unsigned int  m = cellTextures[(yy << 5) | xx | n] >> 8;
            for (int l = 0; l < 8; l++, m = m >> 3)
              blockLayerAlphas[l] += (unsigned char) (m & 7U);
          }
          // find the least visible layer, and remove it
          int     removedLayer = 7;
          unsigned char removedLayerAlpha = 255;
          for (int l = 0; l < 8; l++)
          {
            if (blockLayerAlphas[l] <= removedLayerAlpha)
            {
              removedLayer = l;
              removedLayerAlpha = blockLayerAlphas[l];
            }
          }
          if (removedLayerAlpha > 1)
          {
            std::fprintf(stderr,
                         "LandscapeData: warning: too many layers at %d,%d\n",
                         x + xx, y + yy);
          }
          for (int n = 0; n < 8; n++)
          {
            unsigned int  tmp = cellTextures[(yy << 5) | xx | n];
            unsigned int  m = 0xE0000000U;
            for (int l = 0; l < 8; l++, tmp = tmp >> 3)
            {
              if (l != removedLayer)
                m = (m >> 3) | ((tmp & 0x0700U) << 21);
            }
            if (n <= removedLayer)
              m = m | (cellTextures[(yy << 5) | (xx & 16) | n] & 0xFFU);
            else
              m = m | (cellTextures[(yy << 5) | (xx & 16) | (n + 1)] & 0xFFU);
            *(p++) = m;
          }
        }
      }
    }
    j->second = j->second & ~cellDataValidMask;
  }
  for (std::map< unsigned int, unsigned int >::iterator i = emptyCells.begin();
       i != emptyCells.end(); i++)
  {
    if (i->second == 0U)
      continue;
    // fill empty cells with default data
    int     x = int(i->first & 0xFFFFU) - (cellMinX + 32768);
    int     y = (cellMaxY + 32768) - int((i->first >> 16) & 0xFFFFU);
    size_t  w = size_t(cellMaxX + 1 - cellMinX) << 5;
    size_t  dataOffs = size_t((y << 5) + 31) * w + size_t(x << 5);
    for (int yy = 0; yy < 32; yy++, dataOffs = dataOffs - (w + 32))
    {
      for (int xx = 0; xx < 32; xx++, dataOffs++)
      {
        if (i->second & 0x01)
          hmapData[dataOffs] = 0x8000;
        if (i->second & 0x02)
          ltexData[dataOffs] = 0x07FFU;
        if (i->second & 0x04)
        {
          vnmlData[dataOffs * 3] = 0xFF;
          vnmlData[dataOffs * 3 + 1] = 0x80;
          vnmlData[dataOffs * 3 + 2] = 0x80;
        }
        if (i->second & 0x08)
        {
          vclrData24[dataOffs * 3] = 0xFF;
          vclrData24[dataOffs * 3 + 1] = 0xFF;
          vclrData24[dataOffs * 3 + 2] = 0xFF;
        }
      }
    }
  }
  if (formatMask & 0x01)
  {
    // convert height map to normalized 16-bit unsigned integer format
    double  zScale = 0.0;
    if (zMax > zMin)
      zScale = 65535.0 / (double(zMax) - double(zMin));
    for (std::map< unsigned int, float >::iterator
             i = cellHeightOffsets.begin(); i != cellHeightOffsets.end(); i++)
    {
      int     x = int(i->first & 0xFFFFU) - (cellMinX + 32768);
      int     y = (cellMaxY + 32768) - int((i->first >> 16) & 0xFFFFU);
      size_t  w = size_t(cellMaxX + 1 - cellMinX) << 5;
      size_t  dataOffs = size_t((y << 5) + 31) * w + size_t(x << 5);
      double  zOffset = (double(i->second) - double(zMin)) * zScale + 0.5;
      for (int yy = 0; yy < 32; yy++, dataOffs = dataOffs - (w + 32))
      {
        for (int xx = 0; xx < 32; xx++, dataOffs++)
        {
          double  z = double((int(hmapData[dataOffs]) - 32768) << 3);
          int     tmp = int(z * zScale + zOffset);
          tmp = (tmp >= 0 ? (tmp <= 65535 ? tmp : 65535) : 0);
          hmapData[dataOffs] = (unsigned short) tmp;
        }
      }
    }
  }
  if (formatMask & 0x02)
  {
    if (defTxtID)
    {
      // replace missing textures with default texture ID
      unsigned int  defaultTexture = ~0xFFU | findTextureID(defTxtID);
      size_t  n = size_t(getImageWidth()) * size_t(getImageHeight());
      for (size_t i = 0; i < n; i++)
      {
        if (!(~(ltexData[i]) & 0xFFU))
          ltexData[i] = ltexData[i] & defaultTexture;
      }
    }
    ltexEDIDs.resize(ltexFormIDs.size());
    ltexBGSMPaths.resize(ltexFormIDs.size());
    ltexDPaths.resize(ltexFormIDs.size());
    ltexNPaths.resize(ltexFormIDs.size());
  }
}

void LandscapeData::readString(std::string& s, size_t n, FileBuffer& buf)
{
  s.clear();
  if ((buf.getPosition() + n) > buf.size())
  {
    if (buf.getPosition() >= buf.size())
      return;
    n = buf.size() - buf.getPosition();
  }
  for (size_t i = 0; i < n; i++)
  {
    char    c = char(buf.readUInt8Fast());
    if (!c)
      break;
    if ((unsigned char) c < 0x20)
      c = ' ';
    s += c;
  }
}

void LandscapeData::readPath(std::string& s, size_t n, FileBuffer& buf,
                             const char *prefix, const char *suffix)
{
  readString(s, n, buf);
  if (s.empty())
    return;
  for (size_t i = 0; i < s.length(); i++)
  {
    if (s[i] >= 'A' && s[i] <= 'Z')
      s[i] = s[i] + ('a' - 'A');
    else if (s[i] == '\\')
      s[i] = '/';
  }
  if (prefix && prefix[0] != '\0')
  {
    if (std::strncmp(s.c_str(), prefix, std::strlen(prefix)) != 0)
      s.insert(0, prefix);
  }
  if (suffix && suffix[0] != '\0')
  {
    size_t  suffixLen = std::strlen(suffix);
    if (s.length() < suffixLen ||
        std::strcmp(s.c_str() + (s.length() - suffixLen), suffix) != 0)
    {
      s += suffix;
    }
  }
}

void LandscapeData::loadTextureInfo(ESMFile& esmFile, const BA2File *ba2File,
                                    size_t n)
{
  const ESMFile::ESMRecord  *r = esmFile.getRecordPtr(ltexFormIDs[n]);
  if (!r)
    return;
  ESMFile::ESMField f(esmFile, *r);
  while (f.next())
  {
    if (f == "EDID" && ltexEDIDs[n].empty())
    {
      readString(ltexEDIDs[n], f.size(), f);
    }
    else if ((f == "BNAM" && *r == "LTEX") ||
             (f == "MNAM" && *r == "TXST"))
    {
      readPath(ltexBGSMPaths[n], f.size(), f, "materials/", ".bgsm");
    }
    else if (f.size() >= 4 &&
             ltexBGSMPaths[n].empty() && ltexDPaths[n].empty() &&
             ((f == "TNAM" && *r == "LTEX") ||
              (f == "LNAM" && *r == "GCVR")))
    {
      unsigned int  tmp = f.readUInt32Fast();
      unsigned int  savedFormID = ltexFormIDs[n];
      const ESMFile::ESMRecord  *r2 = esmFile.getRecordPtr(tmp);
      if (tmp && r2 &&
          ((*r == "LTEX" && *r2 == "TXST") || (*r == "GCVR" && *r2 == "LTEX")))
      {
        ltexFormIDs[n] = tmp;
        loadTextureInfo(esmFile, ba2File, n);
        ltexFormIDs[n] = savedFormID;
      }
    }
    else if (f == "TX00" && *r == "TXST")
    {
      readPath(ltexDPaths[n], f.size(), f, "textures/", ".dds");
    }
    else if (f == "TX01" && *r == "TXST")
    {
      readPath(ltexNPaths[n], f.size(), f, "textures/", ".dds");
    }
    else if (f == "ICON" && *r == "LTEX")
    {
      readPath(ltexDPaths[n], f.size(), f, "textures/landscape/", ".dds");
      if (ba2File && !ltexDPaths[n].empty())
      {
        ltexNPaths[n] = ltexDPaths[n];
        ltexNPaths[n].insert(ltexNPaths[n].length() - 4, "_n");
        if (ba2File->getFileSize(ltexNPaths[n]) < 0L)
          ltexNPaths[n].clear();
      }
    }
  }
  if (ltexDPaths[n].empty() && ba2File && !ltexBGSMPaths[n].empty())
  {
    if (ba2File->getFileSize(ltexBGSMPaths[n]) < 68)
      return;
    std::vector< unsigned char >  tmpBuf;
    ba2File->extractFile(tmpBuf, ltexBGSMPaths[n]);
    FileBuffer  bgsmFile(&(tmpBuf.front()), tmpBuf.size());
    if (bgsmFile.readUInt32Fast() != 0x4D534742U)       // "BGSM"
      return;
    unsigned int  bgsmVersion = bgsmFile.readUInt32Fast();
    if (bgsmVersion == 2)               // Fallout 4
      bgsmFile.setPosition(63);
    else if (bgsmVersion == 20)         // Fallout 76
      bgsmFile.setPosition(60);
    else
      return;
    for (int i = 0; i < 2; i++)
    {
      if ((bgsmFile.getPosition() + 4) > bgsmFile.size())
        break;
      size_t  nameLen = bgsmFile.readUInt32Fast();
      size_t  nextPos = bgsmFile.getPosition() + nameLen;
      if (nextPos > bgsmFile.size())
        break;
      if (!i)
        readPath(ltexDPaths[n], nameLen, bgsmFile, "textures/", ".dds");
      else
        readPath(ltexNPaths[n], nameLen, bgsmFile, "textures/", ".dds");
      bgsmFile.setPosition(nextPos);
    }
  }
}

void LandscapeData::loadWorldInfo(ESMFile& esmFile, unsigned int worldID)
{
  const ESMFile::ESMRecord  *r = esmFile.getRecordPtr(worldID);
  if (!(r && *r == "WRLD"))
    throw errorMessage("LandscapeData: world not found in ESM file");
  ESMFile::ESMField f(esmFile, *r);
  while (f.next())
  {
    if (f == "DNAM" && f.size() >= 8)
    {
      landLevel = f.readFloat();
      waterLevel = f.readFloat();
    }
    else if (f == "NAM2" && f.size() >= 4)
    {
      waterFormID = f.readUInt32Fast();
    }
  }
}

LandscapeData::LandscapeData(
    ESMFile *esmFile, const char *btdFileName, const BA2File *ba2File,
    unsigned int formatMask, unsigned int worldID, unsigned int defTxtID,
    int mipLevel, int xMin, int yMin, int xMax, int yMax)
  : cellMinX(xMin),
    cellMinY(yMin),
    cellMaxX(xMax),
    cellMaxY(yMax),
    cellResolution(32),
    cellOffset(0),
    hmapData((unsigned short *) 0),
    ltexData((unsigned int *) 0),
    vnmlData((unsigned char *) 0),
    vclrData24((unsigned char *) 0),
    gcvrData((unsigned short *) 0),
    vclrData16((unsigned short *) 0),
    zMin(1.0e9f),
    zMax(-1.0e9f),
    waterLevel(0.0f),
    landLevel(1024.0f),
    waterFormID(0x00000018U)
{
  unsigned char l = (unsigned char) mipLevel;
  if (!worldID)
    worldID = (btdFileName && *btdFileName ? 0x0025DA15U : 0x0000003CU);
  if (esmFile)
  {
    waterLevel = -8192.0f;      // defaults for Oblivion
    landLevel = -8192.0f;
    loadWorldInfo(*esmFile, worldID);
  }
  if (btdFileName && *btdFileName)
    loadBTDFile(btdFileName, formatMask, l);
  else if (esmFile)
    loadESMFile(*esmFile, formatMask, worldID, defTxtID, l);
  else
    throw errorMessage("LandscapeData: no input file");
  if (esmFile)
  {
    for (size_t i = 0; i < ltexFormIDs.size(); i++)
      loadTextureInfo(*esmFile, ba2File, i);
  }
}

LandscapeData::~LandscapeData()
{
}

