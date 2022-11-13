
#include "common.hpp"
#include "fp32vec4.hpp"
#include "filebuf.hpp"
#include "markers.hpp"

void MapImage::createIcon256(std::vector< unsigned char >& buf, unsigned int c)
{
  buf.clear();
  buf.resize(256 * 256 * 4 + 128, 0);
  buf[0] = 0x44;        // 'D'
  buf[1] = 0x44;        // 'D'
  buf[2] = 0x53;        // 'S'
  buf[3] = 0x20;        // ' '
  buf[4] = 124;         // size of DDS_HEADER
  buf[8] = 0x0F;        // DDSD_CAPS, DDSD_HEIGHT, DDSD_WIDTH, DDSD_PITCH
  buf[9] = 0x10;        // DDSD_PIXELFORMAT
  buf[13] = 1;          // height / 256
  buf[17] = 1;          // width / 256
  buf[21] = 4;          // pitch / 256
  buf[76] = 32;         // size of DDS_PIXELFORMAT
  buf[80] = 0x41;       // DDPF_ALPHAPIXELS, DDPF_RGB
  buf[88] = 32;         // bit count
  buf[92] = 0xFF;       // red mask byte 0
  buf[97] = 0xFF;       // green mask byte 1
  buf[102] = 0xFF;      // blue mask byte 2
  buf[107] = 0xFF;      // alpha mask byte 3
  buf[109] = 0x10;      // DDSCAPS_TEXTURE
  FloatVector4  c0(0.0f, 0.0f, 0.0f, 255.0f);
  FloatVector4  c1(((c & 0xFFU) << 16) | (c & 0xFF00U) | ((c >> 16) & 0xFFU)
                   | 0xFF000000U);
  if (c1.dotProduct3(FloatVector4(0.299f, 0.587f, 0.114f, 0.0f)) < 127.5f)
    c0 = FloatVector4(255.0f);
  unsigned char t = (unsigned char) ((c >> 24) & 0xFF);
  if (t < 0x10 || t >= 0xD0)
    t = 0x4F;
  t = t - 0x10;
  float   a0 = FloatVector4::exp2Fast(float(int(t & 0x0F) - 15) / 3.0f);
  for (int i = 0; i < 65536; i++)
  {
    float   x = float(i & 255) - 127.5f;
    float   y = float(i >> 8) - 127.5f;
    float   d;
    if ((t & 0xC0) == 0x00)                     // circle
      d = float(std::sqrt((x * x) + (y * y)));
    else if ((t & 0xC0) == 0x40)                // square
      d = float(std::fabs(x) > std::fabs(y) ? std::fabs(x) : std::fabs(y));
    else
      d = float(std::fabs(x) + std::fabs(y));   // diamond
    FloatVector4  cTmp(c1);
    switch (t & 0x30)
    {
      case 0x00:                                // soft edges
        cTmp[3] = (128.0f - d) * (255.0f / 112.0f);
        break;
      case 0x10:                                // outline only
        if (d < 92.0f || d > 128.0f)
          cTmp[3] = 0.0f;
        else if (d < 108.0f)
          cTmp[3] = (d - 92.0f) * (255.0f / 16.0f);
        break;
      case 0x20:                                // hard edges
        cTmp[3] = (d > 128.0f ? 0.0f : 255.0f);
        break;
      case 0x30:                                // filled black or white outline
        if (d > 128.0f)
          cTmp[3] = 0.0f;
        else if (d > 108.0f)
          cTmp = c0;
        else if (d > 92.0f)
          cTmp += ((c0 - cTmp) * ((d - 92.0f) * (1.0f / 16.0f)));
        break;
    }
    cTmp[3] = (cTmp[3] > 0.0f ? (cTmp[3] < 255.0f ? cTmp[3] : 255.0f) : 0.0f);
    cTmp[3] = cTmp[3] * a0;
    std::uint32_t n = std::uint32_t(cTmp);
    unsigned char *p = buf.data() + ((i << 2) + 128);
    p[0] = (unsigned char) (n & 0xFFU);
    p[1] = (unsigned char) ((n >> 8) & 0xFFU);
    p[2] = (unsigned char) ((n >> 16) & 0xFFU);
    p[3] = (unsigned char) ((n >> 24) & 0xFFU);
  }
}

void MapImage::loadTextures(const char *listFileName)
{
  markerDefs.clear();
  FileBuffer  inFile(listFileName);
  std::map< std::string, DDSTexture * > textureFiles;
  std::vector< unsigned char >  iconBuf;
  std::vector< std::string >  v;
  std::string s;
  int     lineNumber = 1;
  bool    commentFlag = false;
  bool    eofFlag = false;
  do
  {
    char    c = '\n';
    if (inFile.getPosition() >= inFile.size())
      eofFlag = true;
    else
      c = char(inFile.readUInt8Fast());
    if (c == '\0' || c == '\r' || c == '\n')
    {
      while (s.length() > 0 && (unsigned char) s[s.length() - 1] <= ' ')
        s.resize(s.length() - 1);
      if (!s.empty())
        v.push_back(s);
      if (v.size() == 4 || v.size() == 5)
      {
        markerDefs.resize(markerDefs.size() + 1);
        MarkerDef&  m = markerDefs.back();
        m.formID =
            (unsigned int) parseInteger(v[0].c_str(), 0, "invalid form ID",
                                        0L, 0x0FFFFFFFL);
        m.flags = int(parseInteger(v[1].c_str(), 0, "invalid marker flags",
                                   -1, 0xFFFF));
        m.mipLevel = float(parseFloat(v[3].c_str(), "invalid mip level",
                                      0.0f, 16.0f));
        if (v.size() > 4)
        {
          m.priority = (unsigned char) parseInteger(v[4].c_str(), 0,
                                                    "invalid priority", 0, 15);
        }
        std::map< std::string, DDSTexture * >::iterator i =
            textureFiles.find(v[2]);
        if (i != textureFiles.end())
        {
          m.texture = i->second;
        }
        else
        {
          unsigned int  iconColor = 0;
          try
          {
            iconColor = (unsigned int) parseInteger(v[2].c_str(), 0);
          }
          catch (...)
          {
            m.texture = new DDSTexture(v[2].c_str());
          }
          if (!m.texture)
          {
            createIcon256(iconBuf, iconColor);
            m.texture = new DDSTexture(iconBuf.data(), iconBuf.size());
          }
          m.uniqueTexture = true;
          textureFiles.insert(std::pair< std::string, DDSTexture * >(
                                  v[2], m.texture));
        }
        double  mipMult = std::pow(0.5, m.mipLevel);
        double  alphaSum = 0.0;
        for (int y = 0; y < m.texture->getHeight(); y++)
        {
          for (int x = 0; x < m.texture->getWidth(); x++)
          {
            int     a = int(m.texture->getPixelN(x, y, 0) >> 24);
            m.xOffs = m.xOffs + float(x * a);
            m.yOffs = m.yOffs + float(y * a);
            alphaSum = alphaSum + double(a);
          }
        }
        if (alphaSum > 0.0)
        {
          m.xOffs = m.xOffs * float(mipMult / alphaSum);
          m.yOffs = m.yOffs * float(mipMult / alphaSum);
        }
      }
      else if (v.size() > 0)
      {
        throw FO76UtilsError("invalid marker definition file format "
                             "at line %d", lineNumber);
      }
      v.clear();
      s.clear();
      lineNumber += int(c == '\n');
      commentFlag = false;
    }
    else if (c == '#' || c == ';' ||
             (c == '/' && inFile.getPosition() < inFile.size() &&
              inFile[inFile.getPosition()] == '/'))
    {
      commentFlag = true;
    }
    else if (!commentFlag)
    {
      if (c == '\t')
      {
        while (s.length() > 0 && (unsigned char) s[s.length() - 1] <= ' ')
          s.resize(s.length() - 1);
        if (!s.empty())
          v.push_back(s);
        s.clear();
      }
      else if (!s.empty() || (unsigned char) c > ' ')
      {
        s += c;
      }
    }
  }
  while (!eofFlag);
}

bool MapImage::checkParentWorld(const ESMFile::ESMRecord& r)
{
  const ESMFile::ESMRecord  *r2 = esmFile.getRecordPtr(r.flags);
  if (!r2)
    return false;
  float   onamS = 1.0f;
  float   onamX = 0.0f;
  float   onamY = 0.0f;
  float   onamZ = 0.0f;
  unsigned int  parentWorld = 0U;
  unsigned char worldFlags = 0;
  ESMFile::ESMField f(esmFile, *r2);
  while (f.next())
  {
    if (f == "WNAM" && f.size() >= 4)
    {
      parentWorld = f.readUInt32Fast();
    }
    else if (f == "DATA" && f.size() >= 1)
    {
      worldFlags = f.readUInt8Fast();
    }
    else if (f == "ONAM" && f.size() >= 16)
    {
      onamS = f.readFloat();
      onamX = f.readFloat();
      onamY = f.readFloat();
      onamZ = f.readFloat();
    }
  }
  if (!(parentWorld == worldFormID && r.children &&
        onamS >= 0.0f && !(worldFlags & 0x20)))
  {
    return false;
  }
  if (!(onamX == 0.0f && onamY == 0.0f && onamZ == 0.0f))
    setWorldCellOffsets(r.children, onamX, onamY, onamZ);
  return true;
}

void MapImage::setWorldCellOffsets(unsigned int formID,
                                   float x, float y, float z)
{
  while (formID)
  {
    const ESMFile::ESMRecord  *r = esmFile.getRecordPtr(formID);
    if (!r)
      break;
    if (*r == "GRUP")
    {
      if (r->formID == 4 || r->formID == 5)
        setWorldCellOffsets(r->children, x, y, z);
    }
    else if (*r == "CELL")
    {
      std::map< unsigned int, CellOffset >::iterator  i =
          cellOffsets.find(formID);
      if (i == cellOffsets.end())
      {
        i = cellOffsets.insert(std::pair< unsigned int, CellOffset >(
                                   formID, CellOffset())).first;
      }
      i->second.weight++;
      i->second.x = x;
      i->second.y = y;
      i->second.z = z;
    }
    formID = r->next;
  }
}

void MapImage::setInteriorCellOffset(const REFRRecord& refr)
{
  if (!refr.isDoor || refr.isInterior || isInteriorMap)
    return;
  REFRRecord  refr2;
  if (!getREFRRecord(refr2, refr.xtel))
    return;
  if (!refr2.isInterior)
    return;
  const ESMFile::ESMRecord  *p = getParentCell(refr2.formID);
  if (!p)
    return;
  unsigned int  cellFormID = p->formID;
  float   xOffs = refr.x - refr2.x;
  float   yOffs = refr.y - refr2.y;
  float   zOffs = refr.z - refr2.z;
  std::map< unsigned int, CellOffset >::iterator  i =
      cellOffsets.find(cellFormID);
  if (i == cellOffsets.end())
  {
    i = cellOffsets.insert(std::pair< unsigned int, CellOffset >(
                               cellFormID, CellOffset())).first;
  }
  // use average offset if there are multiple entrances
  i->second.weight++;
  float   w0 = float(i->second.weight - 1);
  float   w1 = 1.0f / float(i->second.weight);
  i->second.x = (i->second.x * w0 + xOffs) * w1;
  i->second.y = (i->second.y * w0 + yOffs) * w1;
  i->second.z = (i->second.z * w0 + zOffs) * w1;
}

void MapImage::findDisabledMarkers(const ESMFile::ESMRecord& r)
{
  ESMFile::ESMField f(esmFile, r);
  while (f.next())
  {
    if (f == "LCEP")
    {
      while ((f.getPosition() + 12) <= f.size())
      {
        unsigned int  formID = f.readUInt32Fast();
        if (f.readUInt32Fast() == 0x001C26C2)   // ZCellEnableMarker
        {
          if (disabledMarkers.find(formID) == disabledMarkers.end())
            disabledMarkers.insert(formID);
        }
        (void) f.readUInt32Fast();
      }
    }
  }
}

MapImage::MapImage(ESMFile& esmFiles, const char *listFileName,
                   std::uint32_t *outBufRGBA, int w, int h,
                   const NIFFile::NIFVertexTransform& vt)
  : esmFile(esmFiles),
    buf(outBufRGBA),
    imageWidth(size_t(w)),
    imageHeight(size_t(h)),
    viewTransform(vt),
    priorityMask(0U),
    worldFormID(0U),
    isInteriorMap(false)
{
  try
  {
    loadTextures(listFileName);
    for (size_t i = 0; i < markerDefs.size(); i++)
    {
      if (formIDs.find(markerDefs[i].formID) == formIDs.end())
        formIDs.insert(markerDefs[i].formID);
      priorityMask = priorityMask | (1U << markerDefs[i].priority);
    }
  }
  catch (...)
  {
    for (size_t i = 0; i < markerDefs.size(); i++)
    {
      if (markerDefs[i].uniqueTexture)
        delete markerDefs[i].texture;
      markerDefs[i].uniqueTexture = false;
      markerDefs[i].texture = (DDSTexture *) 0;
    }
    throw;
  }
}

MapImage::~MapImage()
{
  for (size_t i = 0; i < markerDefs.size(); i++)
  {
    if (markerDefs[i].uniqueTexture)
      delete markerDefs[i].texture;
    markerDefs[i].uniqueTexture = false;
    markerDefs[i].texture = (DDSTexture *) 0;
  }
}

const ESMFile::ESMRecord * MapImage::getParentCell(unsigned int formID) const
{
  const ESMFile::ESMRecord  *r;
  while (true)
  {
    if (!formID || !(r = esmFile.getRecordPtr(formID)))
      return (ESMFile::ESMRecord *) 0;
    if (*r == "CELL")
      break;
    if (*r == "GRUP" && (r->formID == 6 || r->formID == 8 || r->formID == 9))
      formID = r->flags;
    else
      formID = r->parent;
  }
  return r;
}

bool MapImage::getREFRRecord(REFRRecord& r, unsigned int formID)
{
  const ESMFile::ESMRecord  *p = esmFile.getRecordPtr(formID);
  if (!p || !(*p == "REFR" || *p == "ACHR"))
    return false;
  ESMFile::ESMField f(esmFile, *p);
  if (!f.next())
    return false;
  r.formID = formID;
  do
  {
    if (f == "NAME" && f.size() >= 4)
    {
      r.name = f.readUInt32Fast();
    }
    else if (f == "DATA" && f.size() >= 12)
    {
      r.x = f.readFloat();
      r.y = f.readFloat();
      r.z = f.readFloat();
    }
    else if (f == "XTEL" && f.size() >= 4)
    {
      r.xtel = f.readUInt32Fast();
    }
    else if (f == "TNAM" && f.size() >= 2)
    {
      r.tnam = f.readUInt16Fast();
    }
  }
  while (f.next());
  r.isInterior = false;
  r.isChildWorld = false;
  while (p->parent)
  {
    p = esmFile.getRecordPtr(p->parent);
    if (!p)
      break;
    if (*p == "GRUP")
    {
      if (p->formID == 2 || p->formID == 3)
      {
        r.isInterior = true;
        break;
      }
      else if (p->formID == 1)
      {
        r.isChildWorld = (worldFormID && p->flags != worldFormID);
        break;
      }
    }
  }
  r.isMapMarker = (r.name == 0x00000010);       // MapMarker
  if (r.isChildWorld && !r.isMapMarker)
    return false;
  if (r.xtel)
  {
    p = esmFile.getRecordPtr(r.name);
    if (p && *p == "DOOR")
    {
      r.isDoor = true;
      ESMFile::ESMField f2(esmFile, *p);
      while (f2.next())
      {
        if (f2 == "FNAM" && f2.size() >= 1)
        {
          // ignore doors with the minimal use flag
          r.isDoor = !(f2.getDataPtr()[0] & 0x08);
          break;
        }
      }
    }
  }
  return true;
}

void MapImage::drawIcon(size_t n, float x, float y, float z)
{
  if (n >= markerDefs.size())
    return;
  const MarkerDef&  m = markerDefs[n];
  int     mipLevelI = int(m.mipLevel);
  float   mipMult = float(std::pow(2.0, m.mipLevel - float(mipLevelI)));
  viewTransform.transformXYZ(x, y, z);
  float   xf = x - m.xOffs;
  float   yf = y - m.yOffs;
  int     xi = int(xf) - int(xf < 0.0f);
  int     yi = int(yf) - int(yf < 0.0f);
  xf = float(xi) - xf;
  yf = float(yi) - yf;
  float   txtW = float(m.texture->getWidth() >> mipLevelI);
  float   txtH = float(m.texture->getHeight() >> mipLevelI);
  txtW = (txtW > 1.0f ? (txtW - 1.0f) : 0.0f);
  txtH = (txtH > 1.0f ? (txtH - 1.0f) : 0.0f);
  float   txtSclX = 1.0f / (txtW > 1.0f ? txtW : 1.0f);
  float   txtSclY = 1.0f / (txtH > 1.0f ? txtH : 1.0f);
  bool    integerMip = (m.mipLevel == float(mipLevelI));
  for (int yy = 0; true; yy++)
  {
    float   txtY = (yf + float(yy)) * mipMult;
    float   ay = 1.0f;
    if (txtY < 0.0f)
    {
      ay = txtY + 1.0f;
      if (ay <= 0.0f)
        continue;
    }
    else if (txtY > txtH)
    {
      ay = txtH + 1.0f - txtY;
      if (ay <= 0.0f)
        break;
    }
    if ((yi + yy) < 0 || (yi + yy) >= int(imageHeight))
      continue;
    std::uint32_t *p = buf + (size_t(yi + yy) * imageWidth);
    int     xx = 0;
    if (xi < 0)
      xx = -xi;
    else
      p = p + xi;
    for ( ; (xi + xx) < int(imageWidth); xx++, p++)
    {
      float   txtX = (xf + float(xx)) * mipMult;
      float   a = ay;
      if (txtX < 0.0f)
      {
        if (txtX <= -1.0f)
          continue;
        a *= (txtX + 1.0f);
      }
      else if (txtX > txtW)
      {
        if (txtX >= (txtW + 1.0f))
          break;
        a *= (txtW + 1.0f - txtX);
      }
      FloatVector4  c(0.0f);
      if (integerMip)
        c = m.texture->getPixelBC(txtX * txtSclX, txtY * txtSclY, mipLevelI);
      else
        c = m.texture->getPixelTC(txtX * txtSclX, txtY * txtSclY, m.mipLevel);
      a *= c[3];
      if (!(a > 0.0f))
        continue;
      FloatVector4  c0(FloatVector4::convertRGBA32(*p));
      float   a0 = c0[3];
      if (a0 > 0.0f)
      {
        a *= (1.0f / 255.0f);
        a0 *= (1.0f / 255.0f);
        c = (c0 * (a0 * (1.0f - a))) + (c * (1.0f - (a0 * (1.0f - a))));
        a = (1.0f - ((1.0f - a0) * (1.0f - a))) * 255.0f;
      }
      c[3] = a;
      *p = c.convertToRGBA32(false, true);
    }
  }
}

void MapImage::findMarkers(unsigned int worldID)
{
  worldFormID = worldID;
  isInteriorMap = false;
  std::set< REFRRecord >  objectsFound;
  const ESMFile::ESMRecord  *r = esmFile.getRecordPtr(worldID);
  if (worldID)
  {
    if (r && *r == "CELL")
    {
      ESMFile::ESMField f(esmFile, *r);
      while (f.next())
      {
        if (f == "DATA" && f.size() >= 1)
          isInteriorMap = bool(f.readUInt8Fast() & 0x01);
      }
    }
    if (isInteriorMap)
    {
      if (!(r && r->next))
        r = (ESMFile::ESMRecord *) 0;
      else
        r = esmFile.getRecordPtr(r->next);
      if (!(r && *r == "GRUP" && r->formID >= 6U && r->children))
        r = (ESMFile::ESMRecord *) 0;
      else
        r = esmFile.getRecordPtr(r->children);
    }
    else
    {
      r = esmFile.getRecordPtr(0U);
    }
  }
  while (r)
  {
    bool    searchChildRecords = bool(r->children);
    if (BRANCH_LIKELY(*r == "REFR" || *r == "ACHR"))
    {
      bool    matchFlag = (formIDs.find(r->formID) != formIDs.end());
      ESMFile::ESMField f(esmFile, *r);
      while (f.next())
      {
        if (f == "NAME" && f.size() >= 4)
        {
          unsigned int  n = f.readUInt32Fast();
          if (formIDs.find(n) != formIDs.end())
            matchFlag = true;
        }
        else if (BRANCH_UNLIKELY(f == "XTEL") && f.size() >= 4)
        {
          REFRRecord  refr;
          if (getREFRRecord(refr, r->formID))
            setInteriorCellOffset(refr);
        }
      }
      if (matchFlag)
      {
        REFRRecord  refr;
        if (getREFRRecord(refr, r->formID))
          objectsFound.insert(refr);
      }
    }
    else if (*r == "GRUP")
    {
      if (r->formID == 7 || r->formID >= 10 ||
          (worldID != 0U && r->formID == 1 && r->flags != worldID))
      {
        searchChildRecords = (r->formID == 1 && checkParentWorld(*r));
      }
    }
    else if (*r == "LCTN")
    {
      findDisabledMarkers(*r);
    }
    else if (*r == "CELL" && isInteriorMap)
    {
      break;
    }
    unsigned int  nextFormID = 0U;
    if (searchChildRecords)
    {
      nextFormID = r->children;
    }
    else
    {
      while (!r->next && r->parent)
        r = &(esmFile.getRecord(r->parent));
      nextFormID = r->next;
    }
    if (!nextFormID)
      break;
    r = esmFile.getRecordPtr(nextFormID);
  }
  for (unsigned char p = 0; p <= 15; p++)
  {
    if (!(priorityMask & (1U << p)))
      continue;
    for (std::set< REFRRecord >::const_iterator i = objectsFound.begin();
         i != objectsFound.end(); i++)
    {
      REFRRecord    refr = *i;
      unsigned int  formID = refr.formID;
      unsigned int  n = refr.name;
      if (refr.isInterior || refr.isChildWorld)
      {
        const ESMFile::ESMRecord  *c = getParentCell(formID);
        if (!c)
          continue;
        std::map< unsigned int, CellOffset >::const_iterator  k =
            cellOffsets.find(c->formID);
        if (k != cellOffsets.end())
        {
          refr.x = refr.x + k->second.x;
          refr.y = refr.y + k->second.y;
          refr.z = refr.z + k->second.z;
        }
        else if (refr.isInterior && !isInteriorMap)
        {
          continue;
        }
      }
      for (std::vector< MarkerDef >::const_iterator m = markerDefs.begin();
           m != markerDefs.end(); m++)
      {
        if ((m->formID != n && m->formID != formID) || m->priority != p)
          continue;
        if (m->flags >= 0)
        {
          if (!refr.isMapMarker)
          {
            if ((m->flags ^ (m->flags >> 1)) & 0x0001)
            {
              if (bool(m->flags & 0x0001) != refr.isInterior && !isInteriorMap)
                continue;
            }
          }
          else if (refr.tnam != (unsigned int) m->flags ||
                   disabledMarkers.find(refr.formID) != disabledMarkers.end())
          {
            continue;
          }
        }
        drawIcon(size_t(m - markerDefs.begin()), refr.x, refr.y, refr.z);
      }
    }
  }
}

