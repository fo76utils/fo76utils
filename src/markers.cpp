
#include "common.hpp"
#include "filebuf.hpp"
#include "esmfile.hpp"
#include "ddstxt.hpp"

static void createIcon256(std::vector< unsigned char >& buf, unsigned int c)
{
  static const unsigned char  alphaTable[16] =
  {
      8,  10,  13,  16,  20,  25,  32,  40,
     51,  64,  80, 101, 128, 161, 202, 255
  };
  buf.clear();
  buf.resize(256 * 256 + 148, 0);
  buf[0] = 0x44;                // 'D'
  buf[1] = 0x44;                // 'D'
  buf[2] = 0x53;                // 'S'
  buf[3] = 0x20;                // ' '
  buf[4] = 124;                 // size of DDS_HEADER
  buf[8] = 0x07;                // DDSD_CAPS, DDSD_HEIGHT, DDSD_WIDTH
  buf[9] = 0x10;                // DDSD_PIXELFORMAT
  buf[10] = 0x0A;               // DDSD_MIPMAPCOUNT, DDSD_LINEARSIZE
  buf[13] = 1;                  // height / 256
  buf[17] = 1;                  // width / 256
  buf[22] = 1;                  // linear size / 65536
  buf[28] = 1;                  // mipmap count
  buf[76] = 32;                 // size of DDS_PIXELFORMAT
  buf[80] = 0x04;               // DDPF_FOURCC
  buf[84] = 0x44;               // 'D'
  buf[85] = 0x58;               // 'X'
  buf[86] = 0x31;               // '1'
  buf[87] = 0x30;               // '0'
  buf[108] = 0x08;              // DDSCAPS_COMPLEX
  buf[109] = 0x10;              // DDSCAPS_TEXTURE
  buf[110] = 0x40;              // DDSCAPS_MIPMAP
  buf[128] = 0x4D;              // DXGI_FORMAT_BC3_UNORM
  buf[132] = 3;                 // D3D10_RESOURCE_DIMENSION_TEXTURE2D
  unsigned int  r = (c >> 16) & 0xFF;
  unsigned int  g = (c >> 8) & 0xFF;
  unsigned int  b = c & 0xFF;
  unsigned int  c0 = 0;
  if (((r * 299) + (g * 587) + (b * 114)) < 127500)
    c0 = 0xFFFF;
  r = (r * 31 + 127) / 255;
  g = (g * 63 + 127) / 255;
  b = (b * 31 + 127) / 255;
  unsigned int  c1 = (r << 11) | (g << 5) | b;
  unsigned char t = (unsigned char) ((c >> 24) & 0xFF);
  if (t < 0x10 || t >= 0xD0)
    t = 0x4F;
  t = t - 0x10;
  for (size_t i = 0; i < 4096; i++)
  {
    unsigned long long  ba = alphaTable[t & 0x0F];
    unsigned long long  bc = (c1 << 16) | c0;
    for (unsigned char j = 0; j < 16; j++)
    {
      int     x = int(((i & 0x3F) << 2) | (j & 3)) - 128;
      int     y = int(((i >> 6) << 2) | (j >> 2)) - 128;
      int     d = std::abs(x) + std::abs(y);    // diamond
      if ((t & 0xC0) == 0x00)                   // circle
        d = int(std::sqrt(double((x * x) + (y * y))) + 0.5);
      else if ((t & 0xC0) == 0x40)              // square
        d = (std::abs(x) > std::abs(y) ? std::abs(x) : std::abs(y));
      unsigned char aTmp = (d < 128 ? 112 : 0);
      unsigned char cTmp = 48;
      switch (t & 0x30)
      {
        case 0x00:                              // soft edges
          aTmp = (unsigned char) (d < 16 ? 112 : (d < 128 ? (128 - d) : 0));
          break;
        case 0x10:                              // outline only
          if (d < 108)
            aTmp = (unsigned char) (d < 92 ? 0 : ((d - 92) * 7));
          break;
        case 0x20:                              // hard edges
          break;
        case 0x30:                              // filled black or white outline
          if (d >= 92)
            cTmp = (unsigned char) (d < 108 ? ((108 - d) * 3) : 0);
          break;
      }
      aTmp = blendDithered(aTmp >> 4, (aTmp >> 4) + 1, (aTmp & 15) << 4, x, y);
      cTmp = blendDithered(cTmp >> 4, (cTmp >> 4) + 1, (cTmp & 15) << 4, x, y);
      aTmp = (aTmp < 1 ? 1 : (aTmp >= 7 ? 0 : (8 - aTmp)));
      cTmp = (cTmp == 3 ? 1 : (cTmp == 2 ? 3 : (cTmp == 1 ? 2 : 0)));
      ba = ba | ((unsigned long long) aTmp << (j * 3 + 16));
      bc = bc | ((unsigned long long) cTmp << (j * 2 + 32));
    }
    for (size_t j = 0; j < 8; j++)
    {
      buf[(i << 4) + j + 148] = (unsigned char) (ba & 0xFF);
      buf[(i << 4) + j + 156] = (unsigned char) (bc & 0xFF);
      ba = ba >> 8;
      bc = bc >> 8;
    }
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
    bool    isMapMarker;
    bool    isDoor;
    float   x;
    float   y;
    float   z;
    REFRRecord(unsigned int n = 0U)
      : formID(n), name(0U), xtel(0U), tnam(0xFFFFU),
        isInterior(false), isMapMarker(false), isDoor(false),
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
  float   xOffset;
  float   yOffset;
  float   xyScale;
  unsigned int  priorityMask;
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
  std::map< unsigned int, CellOffset >  interiorCellOffsets;
  void setInteriorCellOffset(const REFRRecord& refr);
 public:
  MapImage(const char *esmFileName, const std::vector< MarkerDef >& m,
           int w, int h, int xMin, int yMax, int cellSize, bool isFO76);
  virtual ~MapImage();
  const ESMRecord *getParentCell(unsigned int formID) const;
  bool getREFRRecord(REFRRecord& r, unsigned int formID);
  void drawIcon(size_t n, float x, float y);
  void findMarkers(unsigned int worldFormID = 0U);
  inline const std::vector< unsigned int >& getImageData() const
  {
    return buf;
  }
};

void MapImage::setInteriorCellOffset(const REFRRecord& refr)
{
  if (!refr.isDoor || refr.isInterior)
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
      interiorCellOffsets.find(cellFormID);
  if (i == interiorCellOffsets.end())
  {
    i = interiorCellOffsets.insert(std::pair< unsigned int, CellOffset >(
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

MapImage::MapImage(const char *esmFileName, const std::vector< MarkerDef >& m,
                   int w, int h, int xMin, int yMax, int cellSize, bool isFO76)
  : ESMFile(esmFileName),
    markerDefs(m)
{
  imageWidth = size_t(w);
  imageHeight = size_t(h);
  buf.resize(imageWidth * imageHeight, 0U);
  int     x0 = xMin * 4096;
  int     y1 = (yMax + 1) * 4096;
  if (isFO76)
  {
    x0 = x0 - 2048;
    y1 = y1 - 2048;
  }
  xyScale = float(cellSize) / 4096.0f;
  xOffset = float(-x0) * xyScale;
  yOffset = float(y1) * xyScale - 1.0f;
  priorityMask = 0;
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
  ESMField  f;
  const ESMRecord *p = getRecordPtr(formID);
  if (!p || !(*p == "REFR" || *p == "ACHR") || !getFirstField(f, *p))
    return false;
  r.formID = formID;
  do
  {
    if (f == "NAME" && f.size >= 4)
    {
      r.name = FileBuffer(f.data, f.size).readUInt32Fast();
    }
    else if (f == "DATA" && f.size >= 12)
    {
      FileBuffer  tmpBuf(f.data, f.size);
      r.x = tmpBuf.readFloat();
      r.y = tmpBuf.readFloat();
      r.z = tmpBuf.readFloat();
    }
    else if (f == "XTEL" && f.size >= 4)
    {
      r.xtel = FileBuffer(f.data, f.size).readUInt32Fast();
    }
    else if (f == "TNAM" && f.size >= 2)
    {
      r.tnam = FileBuffer(f.data, f.size).readUInt16Fast();
    }
  }
  while (f.next());
  r.isInterior = false;
  while (p->parent)
  {
    p = getRecordPtr(p->parent);
    if (!p)
      break;
    if (*p == "GRUP" && (p->formID == 2 || p->formID == 3))
    {
      r.isInterior = true;
      break;
    }
  }
  r.isMapMarker = (r.name == 0x00000010);       // MapMarker
  if (r.xtel)
  {
    p = getRecordPtr(r.name);
    if (p && *p == "DOOR" && getFirstField(f, *p))
    {
      r.isDoor = true;
      do
      {
        if (f == "FNAM" && f.size >= 1)
        {
          // ignore doors with the minimal use flag
          r.isDoor = !(f.data[0] & 0x08);
          break;
        }
      }
      while (f.next());
    }
  }
  return true;
}

void MapImage::drawIcon(size_t n, float x, float y)
{
  if (n >= markerDefs.size())
    return;
  const MarkerDef&  m = markerDefs[n];
  int     mipLevelI = int(m.mipLevel);
  float   mipMult = float(std::pow(2.0, m.mipLevel - float(mipLevelI)));
  float   xf = x * xyScale + xOffset - m.xOffs;
  float   yf = (-y) * xyScale + yOffset - m.yOffs;
  int     xi = int(xf) - int(xf < 0.0f);
  int     yi = int(yf) - int(yf < 0.0f);
  xf = float(xi) - xf;
  yf = float(yi) - yf;
  float   txtW = float(m.texture->getWidth() >> mipLevelI);
  float   txtH = float(m.texture->getHeight() >> mipLevelI);
  txtW = (txtW > 1.0f ? (txtW - 1.0f) : 0.0f);
  txtH = (txtH > 1.0f ? (txtH - 1.0f) : 0.0f);
  bool    integerMip = (m.mipLevel == float(mipLevelI));
  for (int yy = 0; true; yy++)
  {
    float   txtY = (yf + float(yy)) * mipMult;
    int     ay = 256;
    if (!yy)
    {
      ay = int((txtY + 1.0f) * 256.0f + 0.5f);
      if (ay <= 0)
        continue;
      txtY = 0.0f;
    }
    else if (txtY > txtH)
    {
      ay = int((txtH + 1.0f - txtY) * 256.0f + 0.5f);
      if (ay <= 0)
        break;
      txtY = txtH;
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
      int     ax = ay << 8;
      if (!xx)
      {
        ax = ay * int((txtX + 1.0f) * 256.0f + 0.5f);
        if (ax <= 0)
          continue;
        txtX = 0.0f;
      }
      else if (txtX > txtW)
      {
        ax = ay * int((txtW + 1.0f - txtX) * 256.0f + 0.5f);
        if (ax <= 0)
          break;
        txtX = txtW;
      }
      unsigned int  c;
      if (integerMip)
        c = m.texture->getPixelB(txtX, txtY, mipLevelI);
      else
        c = m.texture->getPixelT(txtX, txtY, m.mipLevel);
      unsigned int  a = c >> 24;
      if (!a)
        continue;
      a = (a * (unsigned int) ax + 32768U) >> 16;
      unsigned int  c0 = *p;
      unsigned int  a0 = c0 >> 24;
      if (a0)
      {
        c = blendRGBA32(c0, c, int(a + 1U));
        a = 255U - (((255U - a0) * (255U - a) + 127U) / 255U);
      }
      *p = (c & 0x00FFFFFFU) | (a << 24);
    }
  }
}

void MapImage::findMarkers(unsigned int worldFormID)
{
  std::set< REFRRecord >  objectsFound;
  const ESMRecord *r = getRecordPtr(0U);
  ESMField  f;
  while (r)
  {
    bool    searchChildRecords = bool(r->children);
    if (*r == "GRUP")
    {
      if (r->formID == 7 || r->formID >= 10 ||
          (worldFormID != 0U && r->formID == 1 && r->flags != worldFormID))
      {
        searchChildRecords = false;
      }
    }
    else if ((*r == "REFR" || *r == "ACHR") && getFirstField(f, *r))
    {
      bool    matchFlag = (formIDs.find(r->formID) != formIDs.end());
      do
      {
        if (f == "NAME" && f.size >= 4)
        {
          unsigned int  n = FileBuffer(f.data, f.size).readUInt32Fast();
          if (formIDs.find(n) != formIDs.end())
            matchFlag = true;
        }
        else if (BRANCH_EXPECT(f == "XTEL", false) && f.size >= 4)
        {
          REFRRecord  refr;
          if (getREFRRecord(refr, r->formID))
            setInteriorCellOffset(refr);
        }
      }
      while (f.next());
      if (matchFlag)
      {
        REFRRecord  refr;
        if (getREFRRecord(refr, r->formID))
          objectsFound.insert(refr);
      }
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
      if (refr.isInterior)
      {
        const ESMRecord *c = getParentCell(formID);
        if (!c)
          continue;
        std::map< unsigned int, CellOffset >::const_iterator  k =
            interiorCellOffsets.find(c->formID);
        if (k == interiorCellOffsets.end())
          continue;
        refr.x = refr.x + k->second.x;
        refr.y = refr.y + k->second.y;
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
              if (bool(m->flags & 0x0001) != refr.isInterior)
                continue;
            }
          }
          else if (refr.tnam != (unsigned int) m->flags)
          {
            continue;
          }
        }
        drawIcon(size_t(m - markerDefs.begin()), refr.x, refr.y);
      }
    }
  }
}

static const char *usageStrings[] =
{
  "Usage: markers INFILE.ESM[,...] OUTFILE.DDS LANDFILE.DDS ICONLIST.TXT "
  "[worldID]",
  "",
  "LANDFILE.DDS is the output of fo4land or btddump. The format of "
  "ICONLIST.TXT",
  "is a tab separated list of form ID, marker type, icon file, icon mip level",
  "(0.0 = full size, 1.0 = half size, etc.), and an optional priority (0-15).",
  "",
  "Form ID is the NAME field of the reference. Marker type is the TNAM field",
  "of the reference if it is a map marker (form ID = 0x00000010), or -1 for",
  "any type of map marker. For other object types, if not 0 or -1, it can be",
  "1 or 2 to show references from interior or exterior cells only.",
  "",
  "The icon file name must be either a BC3/DXT5 compressed DDS texture, or a",
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
    unsigned int  hdrBuf[11];
    int     imageWidth = 0;
    int     imageHeight = 0;
    int     pixelFormat = 0;
    DDSInputFile  landFile(argv[3],
                           imageWidth, imageHeight, pixelFormat, hdrBuf);
    // "FO", "LAND"
    if ((hdrBuf[0] & 0xFFFF) != 0x4F46 || hdrBuf[1] != 0x444E414C)
      throw errorMessage("invalid landscape image file");
    int     xMin = uint32ToSigned(hdrBuf[2]);
    int     yMax = uint32ToSigned(hdrBuf[6]);
    int     cellSize = int(hdrBuf[9]);
    bool    isFO76 = (hdrBuf[0] == 0x36374F46);
    unsigned int  worldFormID = (!isFO76 ? 0x0000003C : 0x0025DA15);
    if (argc > 5)
    {
      worldFormID =
          (unsigned int) parseInteger(argv[5], 0, "invalid world form ID",
                                      0L, 0x0FFFFFFFL);
    }
    loadTextures(markerDefs, argv[4]);

    MapImage  mapImage(argv[1], markerDefs, imageWidth, imageHeight,
                       xMin, yMax, cellSize, isFO76);
    mapImage.findMarkers(worldFormID);

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

