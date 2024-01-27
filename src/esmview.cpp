
#include "common.hpp"
#include "esmdbase.hpp"
#include "sdlvideo.hpp"

#ifdef HAVE_SDL2
#  include "nif_view.hpp"
#endif

class ESMView : public ESMDump, public SDLDisplay
{
 protected:
  BA2File   *ba2File;
#ifdef HAVE_SDL2
  NIF_View  *renderer;
#endif
  static void printID(char *bufp, unsigned int id);
  void printID(unsigned int id);
 public:
  ESMView(const char *fileName, const char *archivePath,
          int w = 1152, int h = 648, int l = 36, int downsampLevel = 1,
          unsigned char bgColor = 0xE6, unsigned char fgColor = 0x00);
  virtual ~ESMView();
  void loadStrings(const char *stringsPrefix);
  const char *parseFormID(unsigned int& n,
                          const char *s, size_t len = 0x7FFFFFFF) const;
  unsigned int findPreviousRecord(unsigned int formID) const;
  unsigned int findNextRecord(unsigned int formID) const;
  unsigned int findNextGroup(unsigned int formID, const char *pattern) const;
  unsigned int findNextRef(unsigned int formID, const char *pattern);
  unsigned int findNextRecord(unsigned int formID, const char *pattern) const;
  void printFormID(unsigned int formID);
  void printRecordHdr(unsigned int formID);
  void dumpRecord(unsigned int formID = 0U, bool noUnknownFields = false);
#ifdef HAVE_SDL2
  // the following functions return false if the window is closed
  bool viewModel(unsigned int formID,
                 unsigned int mswpFormID = 0U, float gradientMapV = -1.0f);
  bool browseRecord(unsigned int& formID);
#endif
};

void ESMView::loadStrings(const char *stringsPrefix)
{
  if (ba2File)
    ESMDump::loadStrings(*ba2File, stringsPrefix);
}

const char * ESMView::parseFormID(
    unsigned int& n, const char *s, size_t len) const
{
  n = 0xFFFFFFFFU;
  std::string tmp;
  bool    isHexValue = true;
  bool    isDecValue = false;
  for ( ; len > 0 && s && (unsigned char) *s > 0x20; s++, len--)
  {
    char    c = *s;
    if (c >= 'A' && c <= 'Z')
      c = c + ('a' - 'A');
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
          (c == 'x' && tmp.length() == 1 && tmp[0] == '0')))
    {
      bool    skipCharacter =
          ((c == '#' || c == '$') && tmp.empty() && isHexValue);
      isHexValue = false;
      if (skipCharacter)
      {
        isDecValue = (c == '#');
        continue;
      }
    }
    tmp += c;
  }
  if (tmp.empty())
    return s;
  if (!(isHexValue || isDecValue))
  {
    for (std::map< unsigned int, std::string >::const_iterator
             i = edidDB.begin(); i != edidDB.end(); i++)
    {
      if (i->second.length() != tmp.length())
        continue;
      for (size_t j = 0; j < tmp.length(); j++)
      {
        char    c = i->second[j];
        if (c >= 'A' && c <= 'Z')
          c = c + ('a' - 'A');
        if (c != tmp[j])
          break;
        if ((j + 1) >= tmp.length())
        {
          n = i->first;
          return s;
        }
      }
    }
  }
  else
  {
    n = 0U;
    for (size_t i = 0; i < tmp.length(); i++)
    {
      unsigned char c = (unsigned char) tmp[i];
      if (c >= 0x30 && c <= 0x39)       // '0' to '9'
        n = ((n * (isDecValue ? 10U : 16U)) & 0xFFFFFFFFU) + (c - 0x30U);
      else if (c >= 0x61 && c <= 0x66)  // 'a' to 'f'
        n = ((n * (isDecValue ? 10U : 16U)) & 0xFFFFFFFFU) + (c - 0x57U);
    }
  }
  return s;
}

void ESMView::printID(char *bufp, unsigned int id)
{
  id = id & 0x7F7F7F7FU;
  for (int i = 0; i < 4; i++, id = id >> 8)
  {
    unsigned char c = (unsigned char) (id & 0xFFU);
    bufp[i] = ((c >= 0x20 && c < 0x7F) ? char(c) : '?');
  }
}

void ESMView::printID(unsigned int id)
{
  char    tmpBuf[8];
  printID(tmpBuf, id);
  tmpBuf[4] = '\0';
  consolePrint("%s", tmpBuf);
}

ESMView::ESMView(
    const char *fileName, const char *archivePath, int w, int h, int l,
    int downsampLevel, unsigned char bgColor, unsigned char fgColor)
  : ESMDump(fileName, (std::FILE *) 0),
#ifdef HAVE_SDL2
    SDLDisplay(w, h, "esmview", 4U, l),
    ba2File((BA2File *) 0),
    renderer((NIF_View *) 0)
#else
    SDLDisplay(640, 360, "esmview", 0U, 30),
    ba2File((BA2File *) 0)
#endif
{
#ifdef HAVE_SDL2
  setDownsampleLevel(downsampLevel);
  setDefaultTextColor(bgColor, fgColor);
#else
  (void) w;
  (void) h;
  (void) l;
  (void) downsampLevel;
  (void) bgColor;
  (void) fgColor;
#endif
  verboseMode = true;
  if (archivePath)
  {
    ba2File = new BA2File(archivePath,
#ifdef HAVE_SDL2
                          ".bgem\t.bgsm\t.bto\t.btr\t.dds\t.nif\t"
#endif
                          ".dlstrings\t.ilstrings\t.strings");
  }
}

ESMView::~ESMView()
{
#ifdef HAVE_SDL2
  if (renderer)
    delete renderer;
#endif
  if (ba2File)
    delete ba2File;
}

unsigned int ESMView::findPreviousRecord(unsigned int formID) const
{
  const ESMRecord *r = findRecord(formID);
  if (r)
  {
    unsigned int  prvFormID = r->parent;
    r = findRecord(prvFormID);
    if (r)
    {
      prvFormID = (r->children ? r->children : r->next);
      for ( ; prvFormID; prvFormID = r->next)
      {
        r = findRecord(prvFormID);
        if (!r)
          break;
        if (r->next == formID)
          return prvFormID;
      }
    }
  }
  return 0xFFFFFFFFU;
}

unsigned int ESMView::findNextRecord(unsigned int formID) const
{
  const ESMRecord *r = findRecord(formID);
  if (!r)
    return 0xFFFFFFFFU;
  if (r->children)
    return r->children;
  while (!r->next)
  {
    if (!r->parent)
      break;
    r = findRecord(r->parent);
  }
  return r->next;
}

unsigned int ESMView::findNextGroup(unsigned int formID,
                                    const char *pattern) const
{
  unsigned int  groupType = 0;
  unsigned int  groupLabel = 0;
  int     x = 0;
  int     y = 0;
  bool    haveLabel = false;
  bool    r0Flag = false;
  for (size_t i = 0; pattern[i] != '\0'; i++)
  {
    char    c = pattern[i];
    if (c == '\r' || c == '\n')
      break;
    if ((unsigned char) c <= ' ')
      continue;
    if (c >= 'a' && c <= 'z')
      c = c - ('a' - 'A');
    if (c == ':')
    {
      haveLabel = true;
      continue;
    }
    if (!haveLabel && c >= '0' && c <= '9')
    {
      groupType = groupType * 10U + (unsigned int) (c - '0');
      continue;
    }
    haveLabel = true;
    if (groupType == 0)
    {
      groupLabel = (groupLabel >> 8) | (((unsigned int) c & 0xFFU) << 24);
    }
    else if (groupType >= 2 && groupType <= 5)
    {
      if (c == '-')
      {
        y = -1;
      }
      else if (c == ',')
      {
        x = y;
        y = 0;
      }
      else if (c >= '0' && c <= '9')
      {
        y = y * 10 + (y >= 0 ? int(c - '0') : int('9' - c));
      }
    }
    else
    {
      parseFormID(groupLabel, pattern + i);
      break;
    }
  }
  if (groupType >= 2 && groupType <= 5)
  {
    groupLabel = (unsigned int) (y + int(y < 0));
    if (groupType >= 4)
    {
      groupLabel = groupLabel & 0xFFFFU;
      groupLabel = groupLabel | ((unsigned int) (x + int(x < 0)) << 16);
    }
    groupLabel = groupLabel & 0xFFFFFFFFU;
  }
  while (true)
  {
    formID = findNextRecord(formID);
    if (formID == 0xFFFFFFFFU)
      break;
    if (!formID)
    {
      if (r0Flag)
        return 0xFFFFFFFFU;
      r0Flag = true;
      continue;
    }
    const ESMRecord&  r = getRecord(formID);
    if (r == "GRUP" && r.formID == groupType &&
        (!haveLabel || r.flags == groupLabel))
    {
      break;
    }
  }
  return formID;
}

unsigned int ESMView::findNextRef(unsigned int formID, const char *pattern)
{
  size_t  formIDOffs = 0;
  unsigned int  fieldType = 0U;
  while (pattern[0] && (unsigned char) pattern[0] <= 0x20)
    pattern++;
  for (size_t i = 0; pattern[i] != '\0'; i++)
  {
    char    c = pattern[i];
    if (c == ':')
    {
      formIDOffs = i + 1;
      for (size_t j = 0; j < i; j++)
      {
        c = pattern[j];
        if (c >= 'a' && c <= 'z')
          c = c - ('a' - 'A');
        fieldType = (fieldType >> 8) | (((unsigned int) c & 0xFFU) << 24);
      }
      break;
    }
  }
  if (!formIDOffs)
    fieldType = 0x2A000000;             // "*"
  unsigned int  n = 0U;
  parseFormID(n, pattern + formIDOffs);
  bool    r0Flag = false;
  while (true)
  {
    formID = findNextRecord(formID);
    if (formID == 0xFFFFFFFFU)
      break;
    if (!formID)
    {
      if (r0Flag)
        return 0xFFFFFFFFU;
      r0Flag = true;
      continue;
    }
    const ESMRecord&  r = getRecord(formID);
    if (r == "GRUP" || r == "NAVM" ||
        (r == "LAND" && (fieldType & 0xFFFFFF00U) != 0x54585400))   // "*TXT"
    {
      continue;
    }
    ESMField  f(*this, r);
    while (f.next())
    {
      if (f.type == fieldType || fieldType == 0x2A000000)       // "*"
      {
        if (f.type == 0x4F4C564C && f.size() >= 8)              // "LVLO"
          (void) f.readUInt32Fast();
        while ((f.getPosition() + 4) <= f.size())
        {
          if (f.readUInt32Fast() == n &&
              !((f == "LCSR" && (f.getPosition() % 16U) == 0) ||
                (f == "LCEP" && (f.getPosition() % 12U) == 0)))
          {
            return formID;
          }
          if (!(f == "KWDA" || f == "MCQP" || f == "LCSR" || f == "LCEP"))
            break;
        }
      }
    }
  }
  return formID;
}

unsigned int ESMView::findNextRecord(unsigned int formID,
                                     const char *pattern) const
{
  while (pattern[0] == '\t' || pattern[0] == ' ')
    pattern++;
  size_t  patternLen = 0;
  while ((unsigned char) pattern[patternLen] >= ' ')
    patternLen++;
  bool    r0Flag = false;
  while (true)
  {
    formID = findNextRecord(formID);
    if (formID == 0xFFFFFFFFU)
      break;
    if (!formID)
    {
      if (r0Flag)
        return 0xFFFFFFFFU;
      r0Flag = true;
      continue;
    }
    const ESMRecord&  r = getRecord(formID);
    if (r == "GRUP")
      continue;
    if (!patternLen)
      break;
    std::map< unsigned int, std::string >::const_iterator i =
        edidDB.find(formID);
    if (i == edidDB.end())
      continue;
    for (size_t j = 0; (j + patternLen) <= i->second.length(); j++)
    {
      for (size_t k = 0; true; k++)
      {
        if (k >= patternLen)
          return formID;
        char    c0 = i->second[j + k];
        char    c1 = pattern[k];
        if (c0 >= 'A' && c0 <= 'Z')
          c0 = c0 + ('a' - 'A');
        if (c1 >= 'A' && c1 <= 'Z')
          c1 = c1 + ('a' - 'A');
        if (c0 != c1)
          break;
      }
    }
  }
  return formID;
}

void ESMView::printFormID(unsigned int formID)
{
  std::map< unsigned int, std::string >::const_iterator i = edidDB.find(formID);
  if (i != edidDB.end())
    consolePrint("\033[4m0x%08X\033[24m (%s)", formID, i->second.c_str());
  else
    consolePrint("\033[4m0x%08X\033[24m", formID);
}

void ESMView::printRecordHdr(unsigned int formID)
{
  const ESMRecord&  r = getRecord(formID);
  unsigned int  recordType = r.type;
  printID(recordType);
  consolePrint(":\t");
  printFormID(formID);
  if (!(r == "GRUP") && r.flags != 0)
    consolePrint("\tFlags: 0x%08X", r.flags);
  if (r == "GRUP")
  {
    static const char *groupTypes[11] =
    {
      "top",
      "world children",
      "interior cell block",
      "interior cell sub-block",
      "exterior cell block",
      "exterior cell sub-block",
      "cell children",
      "topic children",
      "cell persistent children",
      "cell temporary children",
      "unknown"
    };
    consolePrint(
        "\t%u (%s)\t", r.formID, groupTypes[(r.formID <= 9U ? r.formID : 10U)]);
    switch (r.formID)
    {
      case 0:
        printID(r.flags);
        break;
      case 2:
      case 3:
        consolePrint("%d", uint32ToSigned(r.flags));
        break;
      case 4:
      case 5:
        consolePrint("%d\t%d", uint16ToSigned((unsigned short) (r.flags >> 16)),
                     uint16ToSigned((unsigned short) (r.flags & 0xFFFF)));
        break;
      default:
        printFormID(r.flags);
        break;
    }
  }
  consolePrint("\033[m\n");
}

void ESMView::dumpRecord(unsigned int formID, bool noUnknownFields)
{
  consolePrint("\033[31m\033[1m");
  printRecordHdr(formID);
  const ESMRecord&  r = getRecord(formID);
  consolePrint("Parent:\t");
  printID(getRecord(r.parent).type);
  consolePrint(" ");
  printFormID(r.parent);
  if (r.children)
  {
    consolePrint("\nChild:\t");
    printID(getRecord(r.children).type);
    consolePrint(" ");
    printFormID(r.children);
  }
  if (r.next)
  {
    consolePrint("\nNext:\t");
    printID(getRecord(r.next).type);
    consolePrint(" ");
    printFormID(r.next);
  }
  consolePrint("\n");

  if (r == "GRUP")
  {
    return;
  }

  std::vector< Field >  fields;
  ESMField  f(*this, r);
  while (f.next())
  {
    Field   tmpField;
    tmpField.type = f.type;
    if (!convertField(tmpField.data, r, f) && f.size() > 0 && !noUnknownFields)
    {
      size_t  bytesPerLine = 8;
      if (getTextColumns() >= 144)
        bytesPerLine = 32;
      else if (getTextColumns() >= 78)
        bytesPerLine = 16;
      FileBuffer::printHexData(tmpField.data, f.data(), f.size(), bytesPerLine);
    }
    if (!tmpField.data.empty())
      fields.push_back(tmpField);
  }
  if (fields.size() < 1)
    return;

  consolePrint("\n");
  for (size_t i = 0; i < fields.size(); i++)
  {
    consolePrint("  ");
    printID(fields[i].type);
    if (!fields[i].data.ends_with('\n'))
    {
      consolePrint(":%s\n", fields[i].data.c_str());
    }
    else
    {
      consolePrint(":\n");
      const char  *s = fields[i].data.c_str();
      size_t  n = fields[i].data.find('\n');
      do
      {
        if (s[0] == '0' && s[1] == '0' && s[2] == '0' && s[3] == '0')
          s = s + 4;
        fields[i].data[n] = '\0';
        consolePrint("    %s\n", s);
        fields[i].data[n] = '\n';
        s = fields[i].data.c_str() + (n + 1);
        if (!*s)
          break;
        n = fields[i].data.find('\n', n + 1);
      }
      while (n != std::string::npos);
    }
  }
}

#ifdef HAVE_SDL2
bool ESMView::viewModel(unsigned int formID,
                        unsigned int mswpFormID, float gradientMapV)
{
  if (!renderer)
  {
    if (!ba2File)
      errorMessage("viewing meshes requires specifying an archive path");
    renderer = new NIF_View(*ba2File, this);
  }
  const ESMRecord *r;
  if (!formID || !(r = findRecord(formID)))
    errorMessage("invalid form ID");
  renderer->clearMaterialSwaps();
  unsigned int  refrMSWPFormID = 0U;
  if (*r == "REFR")
  {
    unsigned int  refrName = 0U;
    ESMField  f(*r, *this);
    while (f.next())
    {
      if (f == "NAME" && f.size() >= 4)
        refrName = f.readUInt32Fast();
      else if (f == "XMSP" && f.size() >= 4)
        refrMSWPFormID = f.readUInt32Fast();
    }
    if (!refrName || !(r = findRecord(refrName)))
      errorMessage("invalid record type");
  }
  std::vector< std::string >  nifFileNames(1, std::string());
  std::string&  modelPath = nifFileNames[0];
  ESMField  f(*this, *r);
  while (f.next())
  {
    if ((f == "MODL" || f == "MOD2") && f.size() > 4)
    {
      f.readPath(modelPath, std::string::npos, "meshes/", ".nif");
      if (*r == "SCOL" && getESMVersion() >= 0xC0)
      {
        if (modelPath.find(".esm/", 0, 5) == std::string::npos)
        {
          // fix invalid SCOL model paths in SeventySix.esm
          modelPath = "meshes/scol/seventysix.esm/cm00000000.nif";
          unsigned int  n = r->formID;
          for (int j = 36; n; j--, n = n >> 4)
            modelPath[j] = char(n & 15U) + ((n & 15U) < 10U ? '0' : 'W');
        }
      }
    }
    else if ((f == "MODS" || f == "MO2S") && f.size() >= 4)
    {
      renderer->addMaterialSwap(f.readUInt32Fast() & 0x7FFFFFFFU);
    }
    else if (f == "MODC" && f.size() >= 4)
    {
      renderer->addColorSwap(f.readFloat());
    }
    else if (f == "WNAM" && *r == "ACTI" && f.size() >= 4)
    {
      renderer->setWaterColor(f.readUInt32Fast());
    }
  }
  if (modelPath.empty())
    errorMessage("model path not found in record");
  if (refrMSWPFormID)
    renderer->addMaterialSwap(refrMSWPFormID);
  if (mswpFormID)
    renderer->addMaterialSwap(mswpFormID);
  if (gradientMapV >= 0.0f)
    renderer->addColorSwap(gradientMapV);
  clearTextBuffer();
  std::uint32_t savedTextColor = defaultTextColor;
  bool    noQuitFlag = true;
  try
  {
    setDefaultTextColor(0x00, 0xC0);
    noQuitFlag = renderer->viewModels(*this, nifFileNames);
    defaultTextColor = savedTextColor;
    textColor = savedTextColor;
  }
  catch (...)
  {
    defaultTextColor = savedTextColor;
    textColor = savedTextColor;
    throw;
  }
  return noQuitFlag;
}

bool ESMView::browseRecord(unsigned int& formID)
{
  const ESMRecord *r;
  if (!(r = findRecord(formID)))
    return true;
  if (!(*r == "GRUP" && r->children) && formID)
  {
    if (!r->parent || !(r = findRecord(r->parent)))
      return true;
  }
  std::vector< std::string >  recordList;
  std::vector< unsigned int > formIDList;
  std::string s;
  char    tmpBuf[64];
  bool    noQuitFlag = true;
  while (true)
  {
    recordList.clear();
    formIDList.clear();
    int     itemSelected = 0;
    unsigned int  nextFormID = r->children;
    if (*r == "TES4")
    {
      nextFormID = r->next;
    }
    else
    {
      std::sprintf(tmpBuf, "0x%08X            Parent group", r->parent);
      s = tmpBuf;
      recordList.push_back(s);
      formIDList.push_back(r->parent);
    }
    while (nextFormID && (r = findRecord(nextFormID)) != (ESMRecord *) 0)
    {
      std::sprintf(tmpBuf, "0x%08X    ????", nextFormID);
      printID(&(tmpBuf[14]), r->type);
      s = tmpBuf;
      std::map< unsigned int, std::string >::const_iterator i;
      i = edidDB.find(nextFormID);
      bool    haveEDID = (i != edidDB.end());
      if (*r == "REFR" || *r == "ACHR")
      {
        unsigned int  refrName = 0U;
        ESMField  f(*r, *this);
        while (f.next())
        {
          if (f == "NAME" && f.size() >= 4)
            refrName = f.readUInt32Fast();
        }
        const ESMRecord *r2;
        if (refrName && (r2 = findRecord(refrName)) != (ESMRecord *) 0)
        {
          std::sprintf(tmpBuf, "    ????: 0x%08X", refrName);
          printID(&(tmpBuf[4]), r2->type);
          s += tmpBuf;
          if (!haveEDID)
            i = edidDB.find(refrName);
        }
      }
      else if (*r == "CELL")
      {
        int     xPos = 0;
        int     yPos = 0;
        bool    isInterior = false;
        ESMField  f(*this, *r);
        while (f.next())
        {
          if (f == "DATA" && f.size() >= 1)
          {
            isInterior = bool(f.readUInt8Fast() & 0x01);
          }
          else if (f == "XCLC" && f.size() >= 8)
          {
            xPos = f.readInt32();
            yPos = f.readInt32();
          }
        }
        if (isInterior)
          std::sprintf(tmpBuf, "    Interior        ");
        else
          std::sprintf(tmpBuf, "    X, Y: %4d, %4d", xPos, yPos);
        s += tmpBuf;
      }
      else if (*r == "GRUP")
      {
        static const char *groupTypes[10] =
        {
          "Top  ", "World", "ICBlk", "ICSub", "ECBlk",
          "ECSub", "Cell ", "Topic", "CPers", "CTemp"
        };
        if (r->formID <= 9U)
          std::sprintf(tmpBuf, "    %s", groupTypes[r->formID]);
        else
          std::sprintf(tmpBuf, "    %05u", r->formID & 0xFFFFU);
        s += tmpBuf;
        if (r->formID == 2U || r->formID == 3U)
        {
          std::sprintf(tmpBuf, " %4d      ", uint32ToSigned(r->flags));
        }
        else if (r->formID == 4U || r->formID == 5U)
        {
          std::sprintf(tmpBuf, " %4d, %4d",
                       uint16ToSigned((unsigned short) (r->flags >> 16)),
                       uint16ToSigned((unsigned short) (r->flags & 0xFFFF)));
        }
        else if (r->formID)
        {
          std::sprintf(tmpBuf, " 0x%08X", r->flags);
        }
        else
        {
          std::sprintf(tmpBuf, " ????");
          printID(&(tmpBuf[1]), r->flags);
        }
        s += tmpBuf;
        if ((r->formID == 1U || r->formID >= 6U) && !haveEDID)
          i = edidDB.find(r->flags);
      }
      else if (haveEDID)
      {
        s += "                    ";
      }
      if (haveEDID)
      {
        s += "    \"";
        s += i->second;
        s += '"';
      }
      else if (i != edidDB.end())
      {
        s += "    [";
        s += i->second;
        s += ']';
      }
      recordList.push_back(s);
      formIDList.push_back(nextFormID);
      if (nextFormID == formID)
        itemSelected = int(recordList.size() - 1);
      nextFormID = r->next;
    }
    if (recordList.size() < 1)
      break;
    clearSurface(ansiColor256Table[(defaultTextColor >> 16) & 0xFFU]);
    itemSelected = browseList(recordList, "Select record", itemSelected,
                              0x0B080F04FFFFULL);
    if (itemSelected < 0)
    {
      noQuitFlag = (itemSelected >= -1);
      break;
    }
    if ((r = findRecord(formIDList[itemSelected])) != (ESMRecord *) 0 &&
        !(*r == "GRUP" || *r == "TES4"))
    {
      formID = formIDList[itemSelected];
      break;
    }
  }
  return noQuitFlag;
}
#endif

static void printUsage()
{
  std::fprintf(stderr,
               "esmview FILENAME.ESM[,...] [ARCHIVEPATH "
               "[STRINGS_PREFIX]] [OPTIONS...]\n\n");
  std::fprintf(stderr, "Options:\n");
  std::fprintf(stderr, "    -h      print usage\n");
  std::fprintf(stderr, "    --      remaining options are file names\n");
  std::fprintf(stderr, "    -F FILE read field definitions from FILE\n");
#ifdef HAVE_SDL2
  std::fprintf(stderr,
               "    -w COLUMNS,ROWS,FONT_HEIGHT,DOWNSAMPLE,BGCOLOR,FGCOLOR\n");
  std::fprintf(stderr, "            set SDL console dimensions and colors\n");
#endif
}

static const char *usageString =
    "$EDID:          select record by editor ID\n"
    "0xxxxxx:        hexadecimal form ID\n"
    "#xxxxxx:        decimal form ID\n"
    "B:              print history of records viewed\n"
    "C:              first child record\n"
    "D r:f:name:data define field(s)\n"
    "F xx xx xx xx:  convert binary floating point value(s)\n"
    "G cccc:         find next top level group of record type cccc\n"
    "G d:xxxxxxxx:   find group of type d with form ID label\n"
    "G d:d:          find group of type 2 or 3 with int32 label\n"
    "G d:d,d:        find group of type 4 or 5 with X,Y label\n"
    "I:              print short info and all parent groups\n"
    "L:              back to previously shown record\n"
    "N:              next record in current group\n"
    "P:              parent group\n"
    "R cccc:xxxxxxxx find next reference to form ID in field cccc\n"
    "R *:xxxxxxxx    find next reference to form ID\n"
    "S pattern:      find next record with EDID matching pattern\n"
#ifdef HAVE_SDL2
    "S:              select record interactively\n"
#endif
    "U:              toggle hexadecimal display of unknown field types\n"
    "V:              previous record in current group\n"
#ifdef HAVE_SDL2
    "W [MODS [MODC]] view model (MODL) of current record, if present\n"
    "Ctrl-A:         copy the contents of the text buffer to the clipboard\n"
#endif
    "Q or Ctrl-D:    quit\n\n"
    "C, N, P, and V can be grouped and used as a single command\n\n"
    "Mouse controls:\n\n"
    "Double click:   copy word\n"
    "Triple click:   copy line\n"
    "Middle button:  paste\n"
    "Right button:   copy and paste word\n\n";

int main(int argc, char **argv)
{
  int     consoleWidth = 1152;
  int     consoleHeight = 648;
  int     consoleRows = 36;
  unsigned char consoleDownsampling = 1;
  unsigned char consoleBGColor = 0xE6;
  unsigned char consoleFGColor = 0x00;

  std::vector< unsigned int > prvRecords;
  std::map< size_t, std::string > cmdHistory1;
  std::map< std::string, size_t > cmdHistory2;
  std::string cmdBuf;
  unsigned int  formID = 0U;
  bool    helpFlag = false;
  bool    noUnknownFields = false;
  // 0: print usage and return 0, 1: error, 2: print usage, 3: usage + error
  unsigned char errorFlag = 1;
  ESMView *esmFilePtr = (ESMView *) 0;
  do
  {
    try
    {
      if (!esmFilePtr)
      {
        if (argc < 2)
        {
          errorFlag = 2;
          errorMessage("");
        }
        const char  *inputFileName = 0;
        const char  *archivePath = 0;
        const char  *stringsPrefix = 0;
        const char  *fldDefFileName = 0;
        bool    noOptionsFlag = false;
        for (int i = 1; i < argc; i++)
        {
          if (!noOptionsFlag && argv[i][0] == '-')
          {
            if (std::strcmp(argv[i], "--help") == 0)
            {
              errorFlag = 0;
              errorMessage("");
            }
            if (argv[i][1] != '\0' && argv[i][2] == '\0')
            {
              switch (argv[i][1])
              {
                case '-':
                  noOptionsFlag = true;
                  continue;
                case 'h':
                  errorFlag = 0;
                  errorMessage("");
                  break;
                case 'F':
                  if (++i >= argc)
                    errorMessage("-F: missing file name");
                  fldDefFileName = argv[i];
                  continue;
#ifdef HAVE_SDL2
                case 'w':
                  if (++i >= argc)
                  {
                    errorMessage("-w: missing argument");
                  }
                  else
                  {
                    const char  *s = argv[i];
                    int     consoleParams[6];
                    int     n = 0;
                    consoleParams[0] = 96;      // number of columns
                    consoleParams[1] = 36;      // number of rows
                    consoleParams[2] = 18;      // font height in pixels
                    consoleParams[3] = 1;       // downsampling enabled
                    consoleParams[4] = 0xE6;    // background color
                    consoleParams[5] = 0x00;    // foreground color
                    while (true)
                    {
                      while (*s && (unsigned char) *s <= 0x20)
                        s++;
                      if (*s == '\0')
                        break;
                      if (n >= 6)
                        errorMessage("-w: too many arguments");
                      if (*s != ',')
                      {
                        char    *endp = (char *) 0;
                        long    tmp = std::strtol(s, &endp, 0);
                        if (!endp || endp == s)
                          errorMessage("-w: invalid integer argument");
                        s = endp;
                        while (*s && (unsigned char) *s <= 0x20)
                          s++;
                        if (*s && *s != ',')
                          errorMessage("-w: invalid integer argument");
                        if (n == 0 && !(tmp >= 32L && tmp <= 384L))
                          errorMessage("-w: invalid number of columns");
                        if (n == 1 && !(tmp >= 8L && tmp <= 128L))
                          errorMessage("-w: invalid number of rows");
                        if (n == 2 && !(tmp >= 9L && tmp <= 120L))
                          errorMessage("-w: invalid font height");
                        if (n == 3 && !(tmp >= 0L && tmp <= 2L))
                          errorMessage("-w: invalid downsample level");
                        if (n == 4 && (tmp & ~0xFFL))
                          errorMessage("-w: invalid background color");
                        if (n == 5 && (tmp & ~0xFFL))
                          errorMessage("-w: invalid text color");
                        consoleParams[n] = int(tmp);
                      }
                      if (*s == ',')
                        s++;
                      n++;
                    }
                    consoleWidth = consoleParams[0] * consoleParams[2];
                    consoleWidth = (consoleWidth + consoleWidth + 1) / 3;
                    consoleHeight = consoleParams[1] * consoleParams[2];
                    if (consoleWidth < 256 || consoleWidth > 16384 ||
                        consoleHeight < 64 || consoleHeight > 8192)
                    {
                      errorMessage("invalid console dimensions");
                    }
                    consoleRows = consoleParams[1];
                    consoleDownsampling = (unsigned char) consoleParams[3];
                    consoleBGColor = (unsigned char) consoleParams[4];
                    consoleFGColor = (unsigned char) consoleParams[5];
                  }
                  continue;
#endif
                default:
                  break;
              }
            }
            errorFlag = 3;
            throw FO76UtilsError("invalid option: %s", argv[i]);
          }
          if (!inputFileName)
          {
            inputFileName = argv[i];
          }
          else if (!archivePath)
          {
            archivePath = argv[i];
          }
          else if (!stringsPrefix)
          {
            stringsPrefix = argv[i];
          }
          else
          {
            errorMessage("too many file names");
          }
        }

        esmFilePtr =
            new ESMView(inputFileName, archivePath,
                        consoleWidth, consoleHeight, consoleRows,
                        consoleDownsampling, consoleBGColor, consoleFGColor);
        if (archivePath)
        {
          if (!stringsPrefix)
            stringsPrefix = "strings/seventysix_en";
          esmFilePtr->loadStrings(stringsPrefix);
        }
        esmFilePtr->findEDIDs();
        if (fldDefFileName)
          esmFilePtr->loadFieldDefFile(fldDefFileName);
      }

      ESMView&  esmFile = *esmFilePtr;
      while (true)
      {
        if (!helpFlag)
        {
          esmFile.dumpRecord(formID, noUnknownFields);
          esmFile.consolePrint("\n");
        }
        helpFlag = false;
        if (!esmFile.consoleInput(cmdBuf, cmdHistory1, cmdHistory2))
        {
          delete esmFilePtr;
          esmFilePtr = (ESMView *) 0;
          break;
        }
        for (size_t i = 0; i < cmdBuf.length(); )
        {
          if (cmdBuf[0] == 'd' && i > 1)
            break;
          if (cmdBuf[i] >= 'A' && cmdBuf[i] <= 'Z')
            cmdBuf[i] = cmdBuf[i] + ('a' - 'A');
          if ((unsigned char) cmdBuf[i] < 0x20 || cmdBuf[i] == '\177')
            cmdBuf.erase(i, 1);
          else
            i++;
        }
        if (cmdBuf.empty())
        {
          helpFlag = true;
          continue;
        }
        if (cmdBuf == "q" || cmdBuf == "quit" || cmdBuf == "exit")
        {
          delete esmFilePtr;
          esmFilePtr = (ESMView *) 0;
          break;
        }
        if (cmdBuf == "l")
        {
          if (prvRecords.size() > 0)
          {
            formID = prvRecords.back();
            prvRecords.pop_back();
          }
        }
        else if (cmdBuf == "b")
        {
          helpFlag = true;
          for (size_t i = 0; i < prvRecords.size(); i++)
            esmFile.printRecordHdr(prvRecords[i]);
        }
        else if (cmdBuf == "i")
        {
          helpFlag = true;
          std::vector< unsigned int > tmpBuf;
          const ESMFile::ESMRecord  *r = esmFile.findRecord(formID);
          while (r && r->parent)
          {
            tmpBuf.push_back(r->parent);
            r = esmFile.findRecord(r->parent);
          }
          for (size_t i = tmpBuf.size(); i-- > 0; )
            esmFile.printRecordHdr(tmpBuf[i]);
          esmFile.consolePrint(
              "%s\033[31m\033[1m", (tmpBuf.size() > 0 ? "\n" : ""));
          esmFile.printRecordHdr(formID);
          r = esmFile.findRecord(formID);
          if (!r)
            continue;
          size_t  fileOffs =
              size_t(r->fileData - esmFile.findRecord(0U)->fileData);
          ESMFile::ESMVCInfo  vcInfo;
          esmFile.getVersionControlInfo(vcInfo, *r);
          esmFile.consolePrint(
              "Timestamp:  %04u-%02u-%02u\tUser ID:  0x%04X, 0x%02X\t"
              "File pointer:  0x%08X\n\n",
              vcInfo.year, vcInfo.month, vcInfo.day,
              vcInfo.userID1, vcInfo.userID2,
              (unsigned int) fileOffs & 0xFFFFFFFFU);
          if (r->children)
          {
            esmFile.consolePrint("Child:\t");
            esmFile.printRecordHdr(r->children);
          }
          if (r->next)
          {
            esmFile.consolePrint("Next:\t");
            esmFile.printRecordHdr(r->next);
          }
        }
        else if (cmdBuf[0] == 'd' && cmdBuf.length() > 1)
        {
          helpFlag = true;
          std::string tmp(cmdBuf.c_str() + 1);
          for (size_t i = 0; i < tmp.length(); i++)
          {
            if (tmp[i] == ':')
              tmp[i] = '\t';
          }
          FileBuffer  tmpBuf(reinterpret_cast< const unsigned char * >(
                                 tmp.c_str()), tmp.length());
          try
          {
            esmFile.loadFieldDefFile(tmpBuf);
          }
          catch (std::exception& e)
          {
            esmFile.consolePrint("Invalid field definition\n");
            continue;
          }
        }
        else if (cmdBuf[0] == 'f' && cmdBuf.length() > 1)
        {
          helpFlag = true;
          try
          {
            std::uint64_t tmp = 0ULL;
            unsigned char tmpBuf[4];
            unsigned char nBits = 0;
            unsigned char n = 0;
            for (size_t i = 1; i < cmdBuf.length(); i++)
            {
              char    c = cmdBuf[i];
              if (c >= '0' && c <= '9')
              {
                tmp = (tmp << 4) | std::uint64_t(c - '0');
                nBits = nBits + 4;
              }
              else if (c >= 'a' && c <= 'f')
              {
                tmp = (tmp << 4) | std::uint64_t(c - 'W');
                nBits = nBits + 4;
              }
              else if (!(c == ' ' || (c == 'x' && nBits <= 4)))
              {
                errorMessage("Invalid hexadecimal floating point value");
              }
              if (nBits > 64 || tmp > 0xFFFFFFFFU)
                errorMessage("Invalid hexadecimal floating point value");
              if (!((c == ' ' || (i + 1) >= cmdBuf.length()) && nBits))
                continue;
              int     nBytes = 1;
              if (nBits > 16 || tmp > 0xFFU || (i + 1) >= cmdBuf.length())
                nBytes = 4;
              nBits = 0;
              do
              {
                if (n >= 4)
                {
                  if (!tmp)
                    break;
                  errorMessage("Invalid hexadecimal floating point value");
                }
                tmpBuf[n++] = (unsigned char) (tmp & 0xFFU);
                tmp = tmp >> 8;
              }
              while (--nBytes);
              if (n < 4)
                continue;
              n = 0;
              FileBuffer  buf(tmpBuf, 4);
              double  x = buf.readFloat();
              esmFile.consolePrint(
                  ((x > -1.0e7 && x < 1.0e8) ? "%f\n" : "%g\n"), x);
            }
          }
          catch (std::exception& e)
          {
            esmFile.consolePrint("%s\n", e.what());
            continue;
          }
        }
        else if (cmdBuf[0] == 'g' && cmdBuf.length() > 1)
        {
          unsigned int  tmp = esmFile.findNextGroup(formID, cmdBuf.c_str() + 1);
          if (tmp == 0xFFFFFFFFU)
          {
            helpFlag = true;
            esmFile.consolePrint("Group not found\n");
          }
          else if (tmp != formID)
          {
            prvRecords.push_back(formID);
            formID = tmp;
          }
        }
        else if (cmdBuf[0] == 'r' && cmdBuf.length() > 1)
        {
          unsigned int  tmp = esmFile.findNextRef(formID, cmdBuf.c_str() + 1);
          if (tmp == 0xFFFFFFFFU)
          {
            helpFlag = true;
            esmFile.consolePrint("Record not found\n");
          }
          else if (tmp != formID)
          {
            prvRecords.push_back(formID);
            formID = tmp;
          }
        }
        else if (cmdBuf[0] == 's' && cmdBuf.length() > 1)
        {
          unsigned int  tmp =
              esmFile.findNextRecord(formID, cmdBuf.c_str() + 1);
          if (tmp == 0xFFFFFFFFU)
          {
            helpFlag = true;
            esmFile.consolePrint("Record not found\n");
          }
          else if (tmp != formID)
          {
            prvRecords.push_back(formID);
            formID = tmp;
          }
        }
#ifdef HAVE_SDL2
        else if (cmdBuf == "s")
        {
          if (!esmFile.browseRecord(formID))
          {
            delete esmFilePtr;
            esmFilePtr = (ESMView *) 0;
            break;
          }
        }
#endif
        else if (cmdBuf == "u")
        {
          helpFlag = true;
          noUnknownFields = !noUnknownFields;
          esmFile.consolePrint("Printing unknown fields: %s\n",
                               (!noUnknownFields ? "on" : "off"));
        }
        else if ((cmdBuf[0] >= '0' && cmdBuf[0] <= '9') ||
                 cmdBuf[0] == '#' || cmdBuf[0] == '$')
        {
          unsigned int  newFormID = 0U;
          esmFile.parseFormID(newFormID, cmdBuf.c_str(), cmdBuf.length());
          try
          {
            (void) esmFile.getRecordTimestamp(newFormID);
          }
          catch (...)
          {
            esmFile.consolePrint("Invalid form ID\n\n");
            newFormID = formID;
            helpFlag = true;
          }
          if (newFormID != formID)
          {
            prvRecords.push_back(formID);
            formID = newFormID;
          }
        }
        else if (std::strchr("cnpv", cmdBuf[0]))
        {
          unsigned int  prvFormID = 0xFFFFFFFFU;
          for (size_t i = 0; i < cmdBuf.length(); i++)
          {
            unsigned int  newFormID = 0xFFFFFFFFU;
            switch (cmdBuf[i])
            {
              case 'c':
                newFormID = esmFile.getRecord(formID).children;
                break;
              case 'n':
                newFormID = esmFile.getRecord(formID).next;
                break;
              case 'p':
                if (formID)
                  newFormID = esmFile.getRecord(formID).parent;
                break;
              case 'v':
                newFormID = esmFile.findPreviousRecord(formID);
                break;
              default:
                if ((unsigned char) cmdBuf[i] > 0x20)
                {
                  esmFile.consolePrint("Invalid command: %c\n", cmdBuf[i]);
                  if (prvFormID != 0xFFFFFFFFU)
                    formID = prvFormID;
                  helpFlag = true;
                  i = cmdBuf.length() - 1;
                }
                break;
            }
            if (newFormID != (cmdBuf[i] < 'p' ? 0U : 0xFFFFFFFFU))
            {
              prvFormID = (prvFormID == 0xFFFFFFFFU ? formID : prvFormID);
              formID = newFormID;
            }
          }
          if (prvFormID != 0xFFFFFFFFU)
            prvRecords.push_back(prvFormID);
        }
#ifdef HAVE_SDL2
        else if (cmdBuf[0] == 'w')
        {
          unsigned int  mswpFormID = 0U;
          float   gradientMapV = -1.0f;
          const char  *s = cmdBuf.c_str() + 1;
          while (*s == '\t' || *s == '\r' || *s == ' ')
            s++;
          s = esmFile.parseFormID(mswpFormID, s);
          if (mswpFormID == 0xFFFFFFFFU)
            mswpFormID = 0U;
          while (*s == '\t' || *s == '\r' || *s == ' ')
            s++;
          if (*s != '\0')
          {
            char    *endp = (char *) 0;
            float   tmp = float(std::strtod(s, &endp));
            if (endp && endp != s && tmp >= 0.0f && tmp <= 1.0f)
              gradientMapV = tmp;
          }
          if (!esmFile.viewModel(formID, mswpFormID, gradientMapV))
          {
            delete esmFilePtr;
            esmFilePtr = (ESMView *) 0;
            break;
          }
        }
#endif
        else
        {
          esmFile.consolePrint("%s", usageString);
          helpFlag = true;
        }
      }
    }
    catch (std::exception& e)
    {
      if (esmFilePtr)
      {
        esmFilePtr->consolePrint("\033[41m\033[33m\033[1mError: %s\033[m\n",
                                 e.what());
      }
      else
      {
        SDLDisplay::enableConsole();
        if (errorFlag != 1)
          printUsage();
        if (errorFlag & 1)
          std::fprintf(stderr, "esmview: %s\n", e.what());
        return int(bool(errorFlag));
      }
    }
  }
  while (esmFilePtr);
  return 0;
}

