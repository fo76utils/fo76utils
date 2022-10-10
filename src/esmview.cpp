
#include "common.hpp"
#include "esmdbase.hpp"
#include "sdlvideo.hpp"
#include "nif_view.hpp"

class ESMView : public ESMDump, public SDLDisplay
{
 protected:
  Renderer  *renderer;
  void printID(unsigned int id);
 public:
  ESMView(const char *fileName);
  virtual ~ESMView();
  unsigned int findPreviousRecord(unsigned int formID) const;
  unsigned int findNextRecord(unsigned int formID) const;
  unsigned int findNextGroup(unsigned int formID, const char *pattern) const;
  unsigned int findNextRef(unsigned int formID, const char *pattern);
  unsigned int findNextRecord(unsigned int formID, const char *pattern) const;
  void printFormID(unsigned int formID);
  void printRecordHdr(unsigned int formID);
  void dumpRecord(unsigned int formID = 0U, bool noUnknownFields = false);
};

void ESMView::printID(unsigned int id)
{
  char    tmpBuf[8];
  for (int i = 0; i < 4; i++)
  {
    unsigned char c = (unsigned char) (id & 0x7F);
    id = id >> 8;
    if (c < 0x20 || c >= 0x7F)
      c = 0x3F;
    tmpBuf[i] = char(c);
  }
  tmpBuf[4] = '\0';
  consolePrint("%s", tmpBuf);
}

ESMView::ESMView(const char *fileName)
  : ESMDump(fileName, (std::FILE *) 0),
    SDLDisplay(1152, 648, "esmview", 4U, 36),
    renderer((Renderer *) 0)
{
  setEnableDownsample(true);
  setDefaultTextColor(0xE6, 0x10);
  verboseMode = true;
}

ESMView::~ESMView()
{
  if (renderer)
    delete renderer;
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
      if (c >= '0' && c <= '9')
        groupLabel = (groupLabel << 4) | (unsigned int) (c - '0');
      else if (c >= 'A' && c <= 'F')
        groupLabel = (groupLabel << 4) | (unsigned int) (c - ('A' - 10));
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
  unsigned int  fieldType = 0;
  unsigned int  n = 0;
  bool    haveFieldType = false;
  for (size_t i = 0; pattern[i] != '\0'; i++)
  {
    char    c = pattern[i];
    if ((unsigned char) c <= ' ')
      continue;
    if (c == ':')
    {
      haveFieldType = true;
      n = 0;
      continue;
    }
    if (c >= 'a' && c <= 'z')
      c = c - ('a' - 'A');
    if (!haveFieldType)
      fieldType = (fieldType >> 8) | (((unsigned int) c & 0xFFU) << 24);
    else if (c >= '0' && c <= '9')
      n = (n << 4) | (unsigned int) (c - '0');
    else if (c >= 'A' && c <= 'F')
      n = (n << 4) | (unsigned int) (c - ('A' - 10));
  }
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
    convertField(tmpField.data, r, f);
    if (tmpField.data.empty() && f.size() > 0 && !noUnknownFields)
    {
      char    tmpBuf[32];
      std::sprintf(tmpBuf, "\t[%5u]", (unsigned int) f.size());
      tmpField.data = tmpBuf;
#ifdef HAVE_SDL2
      size_t  maxElements = size_t(getTextColumns());
      if (maxElements < 32)
        maxElements = 32;
      maxElements = ((maxElements - 24) * 39) >> 7;
#else
      size_t  maxElements = 17;
#endif
      for (size_t i = 0; i < f.size(); i++)
      {
        if (i >= maxElements)
        {
          tmpField.data += " ...";
          break;
        }
        std::sprintf(tmpBuf, (!(i & 3) ? "  %02X" : " %02X"),
                     (unsigned int) f.getDataPtr()[i]);
        tmpField.data += tmpBuf;
      }
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
    consolePrint(":%s\n", fields[i].data.c_str());
  }
}

static void printUsage()
{
  std::fprintf(stderr,
               "esmview FILENAME.ESM[,...] [LOCALIZATION.BA2 "
               "[STRINGS_PREFIX]] [OPTIONS...]\n\n");
  std::fprintf(stderr, "Options:\n");
  std::fprintf(stderr, "    -h      print usage\n");
  std::fprintf(stderr, "    --      remaining options are file names\n");
  std::fprintf(stderr, "    -F FILE read field definitions from FILE\n");
}

static const char *usageString =
    "0xxxxxx:        hexadecimal form ID\n"
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
    "U:              toggle hexadecimal display of unknown field types\n"
    "Q:              quit\n"
    "V:              previous record in current group\n\n"
    "C, N, P, and V can be grouped and used as a single command\n\n";

int main(int argc, char **argv)
{
  SDLDisplay::enableConsole();
  if (argc < 2)
  {
    printUsage();
    return 1;
  }
  try
  {
    const char  *inputFileName = 0;
    const char  *stringsFileName = 0;
    const char  *stringsPrefix = 0;
    const char  *fldDefFileName = 0;
    bool    noOptionsFlag = false;
    for (int i = 1; i < argc; i++)
    {
      if (!noOptionsFlag && argv[i][0] == '-')
      {
        if (std::strcmp(argv[i], "--help") == 0)
        {
          printUsage();
          return 0;
        }
        if (argv[i][1] != '\0' && argv[i][2] == '\0')
        {
          switch (argv[i][1])
          {
            case '-':
              noOptionsFlag = true;
              continue;
            case 'h':
              printUsage();
              return 0;
            case 'F':
              if (++i >= argc)
                errorMessage("-F: missing file name");
              fldDefFileName = argv[i];
              continue;
            default:
              break;
          }
        }
        printUsage();
        throw FO76UtilsError("\ninvalid option: %s", argv[i]);
      }
      if (!inputFileName)
      {
        inputFileName = argv[i];
      }
      else if (!stringsFileName)
      {
        stringsFileName = argv[i];
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

    ESMView esmFile(inputFileName);
    if (stringsFileName)
    {
      if (!stringsPrefix)
        stringsPrefix = "strings/seventysix_en";
      esmFile.loadStrings(stringsFileName, stringsPrefix);
    }
    esmFile.findEDIDs();
    if (fldDefFileName)
      esmFile.loadFieldDefFile(fldDefFileName);

    std::vector< unsigned int > prvRecords;
    std::map< size_t, std::string > cmdHistory1;
    std::map< std::string, size_t > cmdHistory2;
    std::string cmdBuf;
    unsigned int  formID = 0U;
    bool    helpFlag = false;
    bool    noUnknownFields = false;
    while (true)
    {
      if (!helpFlag)
      {
        esmFile.dumpRecord(formID, noUnknownFields);
        esmFile.consolePrint("\n");
      }
      helpFlag = false;
      if (!esmFile.consoleInput(cmdBuf, cmdHistory1, cmdHistory2))
        break;
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
        break;
      if (cmdBuf == "l")
      {
        if (prvRecords.size() > 0)
        {
          formID = prvRecords[prvRecords.size() - 1];
          prvRecords.resize(prvRecords.size() - 1);
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
        const ESMFile::ESMRecord  *r = esmFile.getRecordPtr(formID);
        while (r && r->parent)
        {
          tmpBuf.push_back(r->parent);
          r = esmFile.getRecordPtr(r->parent);
        }
        for (size_t i = tmpBuf.size(); i-- > 0; )
          esmFile.printRecordHdr(tmpBuf[i]);
        esmFile.consolePrint(
            "%s\033[31m\033[1m", (tmpBuf.size() > 0 ? "\n" : ""));
        esmFile.printRecordHdr(formID);
        r = esmFile.getRecordPtr(formID);
        if (!r)
          continue;
        size_t  fileOffs =
            size_t(r->fileData - esmFile.getRecordPtr(0U)->fileData);
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
          unsigned char tmpBuf[4];
          unsigned char j = 0;
          for (size_t i = 1; i < cmdBuf.length(); i++)
          {
            char    c = cmdBuf[i];
            if (c >= '0' && c <= '9')
              c = c - '0';
            else if (c >= 'a' && c <= 'f')
              c = c - 'W';
            else
              errorMessage("Invalid hexadecimal floating point value");
            if (!(j & 1))
              tmpBuf[j >> 1] = (unsigned char) c << 4;
            else
              tmpBuf[j >> 1] |= (unsigned char) c;
            if (++j >= 8)
            {
              j = 0;
              FileBuffer  buf(tmpBuf, 4);
              double  x = buf.readFloat();
              esmFile.consolePrint(
                  ((x > -1.0e7 && x < 1.0e8) ? "%f\n" : "%g\n"), x);
            }
          }
          if (j != 0)
            errorMessage("Invalid hexadecimal floating point value");
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
        unsigned int  tmp = esmFile.findNextRecord(formID, cmdBuf.c_str() + 1);
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
      else if (cmdBuf == "u")
      {
        helpFlag = true;
        noUnknownFields = !noUnknownFields;
        esmFile.consolePrint("Printing unknown fields: %s\n",
                             (!noUnknownFields ? "on" : "off"));
      }
      else if (cmdBuf[0] >= '0' && cmdBuf[0] <= '9')
      {
        unsigned int  newFormID = 0U;
        for (size_t i = 0; i < cmdBuf.length(); i++)
        {
          if (cmdBuf[i] >= '0' && cmdBuf[i] <= '9')
            newFormID = (newFormID << 4) | (unsigned int) (cmdBuf[i] - '0');
          else if (cmdBuf[i] >= 'a' && cmdBuf[i] <= 'f')
            newFormID = (newFormID << 4) | (unsigned int) (cmdBuf[i] - 'W');
        }
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
      else
      {
        esmFile.consolePrint("%s", usageString);
        helpFlag = true;
      }
    }
  }
  catch (std::exception& e)
  {
    std::fprintf(stderr, "esmview: %s\n", e.what());
    return 1;
  }
  return 0;
}

