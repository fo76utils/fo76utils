
#include "common.hpp"
#include "filebuf.hpp"
#include "esmfile.hpp"
#include "ddstxt.hpp"
#include "nif_file.hpp"
#include "fp32vec4.hpp"

static void createIcon256(std::vector< unsigned char >& buf, unsigned int c)
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
          cTmp += ((c0 - cTmp) * FloatVector4((d - 92.0f) * (1.0f / 16.0f)));
        break;
    }
    cTmp[3] = (cTmp[3] > 0.0f ? (cTmp[3] < 255.0f ? cTmp[3] : 255.0f) : 0.0f);
    cTmp[3] = cTmp[3] * a0;
    unsigned int  n = (unsigned int) cTmp;
    unsigned char *p = &(buf.front()) + ((i << 2) + 128);
    p[0] = (unsigned char) (n & 0xFFU);
    p[1] = (unsigned char) ((n >> 8) & 0xFFU);
    p[2] = (unsigned char) ((n >> 16) & 0xFFU);
    p[3] = (unsigned char) ((n >> 24) & 0xFFU);
  }
}

struct MarkerDef
{
  DDSTexture    *texture;
  unsigned int  formID;
  int           flags;
  float         mipLevel;
  float         xOffs;
  float         yOffs;
  bool          uniqueTexture;
  unsigned char priority;
  MarkerDef()
    : texture((DDSTexture *) 0), formID(0U), flags(-1), mipLevel(0.0f),
      xOffs(0.0f), yOffs(0.0f), uniqueTexture(false), priority(0)
  {
  }
};

void loadTextures(std::vector< MarkerDef >& textures, const char *listFileName)
{
  textures.clear();
  FileBuffer  inFile(listFileName);
  try
  {
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
          textures.resize(textures.size() + 1);
          MarkerDef&  m = textures[textures.size() - 1];
          m.formID =
              (unsigned int) parseInteger(v[0].c_str(), 0, "invalid form ID",
                                          0L, 0x0FFFFFFFL);
          m.flags = int(parseInteger(v[1].c_str(), 0, "invalid marker flags",
                                     -1, 0xFFFF));
          m.mipLevel = float(parseFloat(v[3].c_str(), "invalid mip level",
                                        0.0f, 16.0f));
          if (v.size() > 4)
          {
            m.priority =
                (unsigned char) parseInteger(v[4].c_str(), 0,
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
              m.texture = new DDSTexture(&(iconBuf.front()), iconBuf.size());
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
          throw errorMessage("invalid marker definition file format at line %d",
                             lineNumber);
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
  catch (...)
  {
    for (size_t i = 0; i < textures.size(); i++)
    {
      if (textures[i].uniqueTexture)
        delete textures[i].texture;
      textures[i].uniqueTexture = false;
      textures[i].texture = (DDSTexture *) 0;
    }
    throw;
  }
}

class MapImage : public ESMFile
{
 public:
  struct REFRRecord
  {
    unsigned int  formID;
    unsigned int  name;
    unsigned int  xtel;
    unsigned int  tnam;
    bool    isInterior;
    bool    isChildWorld;
    bool    isMapMarker;
    bool    isDoor;
    float   x;
    float   y;
    float   z;
    REFRRecord(unsigned int n = 0U)
      : formID(n), name(0U), xtel(0U), tnam(0xFFFFU),
        isInterior(false), isChildWorld(false),
        isMapMarker(false), isDoor(false),
        x(0.0f), y(0.0f), z(0.0f)
    {
    }
    inline bool operator<(const REFRRecord& r) const
    {
      return (formID < r.formID);
    }
  };
 protected:
  const std::vector< MarkerDef >& markerDefs;
  std::vector< unsigned int > buf;
  size_t  imageWidth;
  size_t  imageHeight;
  NIFFile::NIFVertexTransform viewTransform;
  unsigned int  priorityMask;
  unsigned int  worldFormID;
  bool    isInteriorMap;
  std::set< unsigned int >  formIDs;
  struct CellOffset
  {
    float   x;
    float   y;
    float   z;
    int     weight;
    CellOffset()
      : x(0.0f), y(0.0f), z(0.0f), weight(0)
    {
    }
  };
  std::map< unsigned int, CellOffset >  cellOffsets;
  std::set< unsigned int >  disabledMarkers;
  bool checkParentWorld(const ESMRecord& r);
  void setWorldCellOffsets(unsigned int formID, float x, float y, float z);
  void setInteriorCellOffset(const REFRRecord& refr);
  void findDisabledMarkers(const ESMRecord& r);
 public:
  MapImage(const char *esmFileName, const std::vector< MarkerDef >& m,
           int w, int h, const NIFFile::NIFVertexTransform& vt);
  virtual ~MapImage();
  const ESMRecord *getParentCell(unsigned int formID) const;
  bool getREFRRecord(REFRRecord& r, unsigned int formID);
  void drawIcon(size_t n, float x, float y, float z);
  void findMarkers(unsigned int worldID = 0U);
  inline const std::vector< unsigned int >& getImageData() const
  {
    return buf;
  }
};

bool MapImage::checkParentWorld(const ESMRecord& r)
{
  const ESMRecord *r2 = findRecord(r.flags);
  if (!r2)
    return false;
  float   onamS = 1.0f;
  float   onamX = 0.0f;
  float   onamY = 0.0f;
  float   onamZ = 0.0f;
  unsigned int  parentWorld = 0U;
  unsigned char worldFlags = 0;
  ESMField  f(*this, *r2);
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
    const ESMRecord *r = getRecordPtr(formID);
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
  const ESMRecord *p = getParentCell(refr2.formID);
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

void MapImage::findDisabledMarkers(const ESMRecord& r)
{
  ESMField  f(*this, r);
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

MapImage::MapImage(const char *esmFileName, const std::vector< MarkerDef >& m,
                   int w, int h, const NIFFile::NIFVertexTransform& vt)
  : ESMFile(esmFileName),
    markerDefs(m),
    buf(size_t(w) * size_t(h), 0U),
    imageWidth(size_t(w)),
    imageHeight(size_t(h)),
    viewTransform(vt),
    priorityMask(0U),
    worldFormID(0U),
    isInteriorMap(false)
{
  for (size_t i = 0; i < markerDefs.size(); i++)
  {
    if (formIDs.find(markerDefs[i].formID) == formIDs.end())
      formIDs.insert(markerDefs[i].formID);
    priorityMask = priorityMask | (1U << markerDefs[i].priority);
  }
}

MapImage::~MapImage()
{
}

const ESMFile::ESMRecord * MapImage::getParentCell(unsigned int formID) const
{
  const ESMRecord *r;
  while (true)
  {
    if (!formID || !(r = getRecordPtr(formID)))
      return (ESMRecord *) 0;
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
  const ESMRecord *p = getRecordPtr(formID);
  if (!p || !(*p == "REFR" || *p == "ACHR"))
    return false;
  ESMField  f(*this, *p);
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
    p = getRecordPtr(p->parent);
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
    p = getRecordPtr(r.name);
    if (p && *p == "DOOR")
    {
      r.isDoor = true;
      ESMField  f2(*this, *p);
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
    unsigned int  *p = &(buf.front()) + (size_t(yi + yy) * imageWidth);
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
      FloatVector4  c0(*p);
      float   a0 = c0[3];
      if (a0 > 0.0f)
      {
        a *= (1.0f / 255.0f);
        a0 *= (1.0f / 255.0f);
        c = (c0 * FloatVector4(a0 * (1.0f - a)))
            + (c * FloatVector4(1.0f - (a0 * (1.0f - a))));
        a = (1.0f - ((1.0f - a0) * (1.0f - a))) * 255.0f;
      }
      c[3] = a;
      *p = (unsigned int) c;
    }
  }
}

void MapImage::findMarkers(unsigned int worldID)
{
  worldFormID = worldID;
  isInteriorMap = false;
  std::set< REFRRecord >  objectsFound;
  const ESMRecord *r = getRecordPtr(worldID);
  if (worldID)
  {
    if (r && *r == "CELL")
    {
      ESMField  f(*this, *r);
      while (f.next())
      {
        if (f == "DATA" && f.size() >= 1)
          isInteriorMap = bool(f.readUInt8Fast() & 0x01);
      }
    }
    if (isInteriorMap)
    {
      if (!(r && r->next))
        r = (ESMRecord *) 0;
      else
        r = getRecordPtr(r->next);
      if (!(r && *r == "GRUP" && r->formID >= 6U && r->children))
        r = (ESMRecord *) 0;
      else
        r = getRecordPtr(r->children);
    }
    else
    {
      r = getRecordPtr(0U);
    }
  }
  while (r)
  {
    bool    searchChildRecords = bool(r->children);
    if (BRANCH_LIKELY(*r == "REFR" || *r == "ACHR"))
    {
      bool    matchFlag = (formIDs.find(r->formID) != formIDs.end());
      ESMField  f(*this, *r);
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
        r = &(getRecord(r->parent));
      nextFormID = r->next;
    }
    if (!nextFormID)
      break;
    r = getRecordPtr(nextFormID);
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
        const ESMRecord *c = getParentCell(formID);
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

static const char *usageStrings[] =
{
  "Usage:",
  "    markers INFILE.ESM[,...] OUTFILE.DDS LANDFILE.DDS ICONLIST.TXT "
  "[worldID]",
  "    markers INFILE.ESM[,...] OUTFILE.DDS VIEWTRANSFORM ICONLIST.TXT "
  "[worldID]",
  "",
  "LANDFILE.DDS is the output of fo4land or btddump, to be used as reference",
  "for image size and mapping coordinates. Alternatively, a comma separated",
  "list of image width, height, view scale, X, Y, Z rotation, and X, Y, Z",
  "offsets can be specified, these parameters are used similarly to render.",
  "",
  "The format of ICONLIST.TXT is a tab separated list of form ID, marker",
  "type, icon file, icon mip level (0.0 = full size, 1.0 = half size, etc.),",
  "and an optional priority (0-15).",
  "",
  "Form ID is the NAME field of the reference. Marker type is the TNAM field",
  "of the reference if it is a map marker (form ID = 0x00000010), or -1 for",
  "any type of map marker. For other object types, if not 0 or -1, it can be",
  "1 or 2 to show references from interior or exterior cells only.",
  "",
  "The icon file must be either a DDS texture in a supported format, or a",
  "32-bit integer defining a basic shape and color for a 256x256 icon in",
  "0xTARRGGBB format. A is the opacity, T is the type as shape + edges:",
  "  shape: 1: circle, 5: square, 9: diamond",
  "  edges: 0: soft, 1: outline only, 2: hard, 3: filled black/white outline",
  "Any invalid type defaults to TA = 0x4F.",
  (char *) 0
};

int main(int argc, char **argv)
{
  if (argc < 5 || argc > 6)
  {
    for (size_t i = 0; usageStrings[i]; i++)
      std::fprintf(stderr, "%s\n", usageStrings[i]);
    return 1;
  }
  std::vector< MarkerDef >  markerDefs;
  try
  {
    int     imageWidth = 0;
    int     imageHeight = 0;
    float   viewScale = 1.0f;
    float   viewRotationX = float(std::atan(1.0) * 4.0);
    float   viewRotationY = 0.0f;
    float   viewRotationZ = 0.0f;
    float   viewOffsX = 0.0f;
    float   viewOffsY = 0.0f;
    float   viewOffsZ = 0.0f;
    {
      const char  *s = argv[3];
      size_t  n = std::strlen(s);
      if ((n >= 5 && (FileBuffer::readUInt32Fast(s + (n - 4)) | 0x20202000U)
                     == 0x7364642EU) ||         // ".dds"
          !std::strchr(s, ','))
      {
        unsigned int  hdrBuf[11];
        int     pixelFormat = 0;
        DDSInputFile  landFile(s, imageWidth, imageHeight, pixelFormat, hdrBuf);
        // "FO", "LAND"
        if ((hdrBuf[0] & 0xFFFF) != 0x4F46 || hdrBuf[1] != 0x444E414C)
          throw errorMessage("invalid landscape image file");
        int     xMin = uint32ToSigned(hdrBuf[2]);
        int     yMax = uint32ToSigned(hdrBuf[6]);
        int     cellSize = int(hdrBuf[9]);
        bool    isFO76 = (hdrBuf[0] == 0x36374F46);
        int     x0 = xMin * 4096;
        int     y1 = (yMax + 1) * 4096;
        if (isFO76)
        {
          x0 = x0 - 2048;
          y1 = y1 - 2048;
        }
        viewScale = float(cellSize) / 4096.0f;
        viewOffsX = float(-x0) * viewScale;
        viewOffsY = float(y1) * viewScale - 1.0f;
      }
      else
      {
        for (int i = 0; i < 9; i++)
        {
          long    tmp1 = 0L;
          double  tmp2 = 0.0;
          char    *endp = (char *) 0;
          if (i < 2)
            tmp1 = std::strtol(s, &endp, 0);
          else
            tmp2 = std::strtod(s, &endp);
          if (!endp || endp == s || *endp != (i < 8 ? ',' : '\0'))
          {
            throw errorMessage("invalid view transform, "
                               "must be 9 comma separated numbers");
          }
          if (i < 2 && (tmp1 < 2L || tmp1 > 65536L))
            throw errorMessage("invalid image dimensions");
          if (i == 2 && !(tmp2 >= (1.0 / 512.0) && tmp2 <= 16.0))
            throw errorMessage("invalid view scale");
          if (i >= 3 && i < 6)
          {
            if (!(tmp2 >= -360.0 && tmp2 <= 360.0))
              throw errorMessage("invalid view rotation");
            tmp2 = tmp2 * (std::atan(1.0) / 45.0);
          }
          if (i >= 6 && !(tmp2 >= -1048576.0 && tmp2 <= 1048576.0))
            throw errorMessage("invalid view offset");
          if (i < 8)
            s = endp + 1;
          if (i == 0)
            imageWidth = int(tmp1);
          else if (i == 1)
            imageHeight = int(tmp1);
          else if (i == 2)
            viewScale = float(tmp2);
          else if (i == 3)
            viewRotationX = float(tmp2);
          else if (i == 4)
            viewRotationY = float(tmp2);
          else if (i == 5)
            viewRotationZ = float(tmp2);
          else if (i == 6)
            viewOffsX = float(tmp2) + (float(imageWidth) * 0.5f);
          else if (i == 7)
            viewOffsY = float(tmp2) + (float(imageHeight - 2) * 0.5f);
          else if (i == 8)
            viewOffsZ = float(tmp2);
        }
      }
    }
    loadTextures(markerDefs, argv[4]);

    MapImage  mapImage(argv[1], markerDefs, imageWidth, imageHeight,
                       NIFFile::NIFVertexTransform(
                           viewScale,
                           viewRotationX, viewRotationY, viewRotationZ,
                           viewOffsX, viewOffsY, viewOffsZ));
    unsigned int  worldID = 0U;
    if (argc > 5)
    {
      worldID = (unsigned int) parseInteger(argv[5], 0, "invalid world form ID",
                                            0L, 0x0FFFFFFFL);
    }
    if (!worldID)
      worldID = (mapImage.getESMVersion() < 0xC0U ? 0x0000003C : 0x0025DA15);
    mapImage.findMarkers(worldID);

    DDSOutputFile outFile(argv[2], imageWidth, imageHeight,
                          DDSInputFile::pixelFormatRGBA32);
    const std::vector< unsigned int >&  buf = mapImage.getImageData();
    for (size_t i = 0; i < buf.size(); i++)
    {
      outFile.writeByte((unsigned char) ((buf[i] >> 16) & 0xFF));
      outFile.writeByte((unsigned char) ((buf[i] >> 8) & 0xFF));
      outFile.writeByte((unsigned char) (buf[i] & 0xFF));
      outFile.writeByte((unsigned char) ((buf[i] >> 24) & 0xFF));
    }
    outFile.flush();

    for (size_t i = 0; i < markerDefs.size(); i++)
    {
      if (markerDefs[i].uniqueTexture)
        delete markerDefs[i].texture;
      markerDefs[i].uniqueTexture = false;
      markerDefs[i].texture = (DDSTexture *) 0;
    }
  }
  catch (std::exception& e)
  {
    for (size_t i = 0; i < markerDefs.size(); i++)
    {
      if (markerDefs[i].uniqueTexture)
        delete markerDefs[i].texture;
      markerDefs[i].uniqueTexture = false;
      markerDefs[i].texture = (DDSTexture *) 0;
    }
    std::fprintf(stderr, "markers: %s\n", e.what());
    return 1;
  }
  return 0;
}

