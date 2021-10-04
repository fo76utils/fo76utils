
#include "common.hpp"
#include "esmdbase.hpp"

class ESMView : public ESMDump
{
 public:
  ESMView(const char *fileName, std::FILE *outFile = 0);
  virtual ~ESMView();
  unsigned int findNextRecord(unsigned int formID) const;
  unsigned int findNextGroup(unsigned int formID, const char *pattern) const;
  unsigned int findNextRef(unsigned int formID, const char *pattern);
  unsigned int findNextRecord(unsigned int formID, const char *pattern) const;
  void printFormID(unsigned int formID);
  void printRecordHdr(unsigned int formID);
  void dumpRecord(unsigned int formID = 0U, bool noUnknownFields = false);
};

ESMView::ESMView(const char *fileName, std::FILE *outFile)
  : ESMDump(fileName, outFile)
{
  verboseMode = true;
}

ESMView::~ESMView()
{
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
    ESMField  f;
    if (!getFirstField(f, r))
      continue;
    do
    {
      if (f.type == fieldType || fieldType == 0x2A000000)       // "*"
      {
        FileBuffer  buf(f.data, f.size);
        if (f.type == 0x4F4C564C && f.size >= 8)                // "LVLO"
          (void) buf.readUInt32Fast();
        while ((buf.getPosition() + 4) <= buf.size())
        {
          if (buf.readUInt32Fast() == n)
            return formID;
          if (f.type != 0x4144574B && f.type != 0x5051434D) // "KWDA", "MCQP"
            break;
        }
      }
    }
    while (f.next());
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
  {
    std::fprintf(outputFile, "\033[4m0x%08X\033[24m (%s)",
                 formID, i->second.c_str());
  }
  else
  {
    std::fprintf(outputFile, "\033[4m0x%08X\033[24m", formID);
  }
}

void ESMView::printRecordHdr(unsigned int formID)
{
  const ESMRecord&  r = getRecord(formID);
  unsigned int  recordType = r.type;
  printID(recordType);
  std::fprintf(outputFile, ":\t");
  printFormID(formID);
  if (!(r == "GRUP") && r.flags != 0)
    std::fprintf(outputFile, "\tFlags: 0x%08X", r.flags);
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
    std::fprintf(outputFile, "\t%u (%s)\t",
                 r.formID, groupTypes[(r.formID <= 9U ? r.formID : 10U)]);
    switch (r.formID)
    {
      case 0:
        printID(r.flags);
        break;
      case 2:
      case 3:
        std::fprintf(outputFile, "%d", uint32ToSigned(r.flags));
        break;
      case 4:
      case 5:
        std::fprintf(outputFile, "%d\t%d",
                     uint16ToSigned((unsigned short) (r.flags >> 16)),
                     uint16ToSigned((unsigned short) (r.flags & 0xFFFF)));
        break;
      default:
        printFormID(r.flags);
        break;
    }
  }
  std::fprintf(outputFile, "\033[m\n");
}

void ESMView::dumpRecord(unsigned int formID, bool noUnknownFields)
{
  std::fprintf(outputFile, "\033[1m");
  printRecordHdr(formID);
  const ESMRecord&  r = getRecord(formID);
  std::fprintf(outputFile, "Parent:\t");
  printID(getRecord(r.parent).type);
  std::fputc(' ', outputFile);
  printFormID(r.parent);
  if (r.children)
  {
    std::fprintf(outputFile, "\nChild:\t");
    printID(getRecord(r.children).type);
    std::fputc(' ', outputFile);
    printFormID(r.children);
  }
  if (r.next)
  {
    std::fprintf(outputFile, "\nNext:\t");
    printID(getRecord(r.next).type);
    std::fputc(' ', outputFile);
    printFormID(r.next);
  }
  std::fputc('\n', outputFile);

  if (r == "GRUP")
  {
    return;
  }

  std::vector< Field >  fields;
  ESMField  f;
  if (getFirstField(f, r))
  {
    do
    {
      Field   tmpField;
      tmpField.type = f.type;
      convertField(tmpField.data, r, f);
      if (tmpField.data.empty() && f.size > 0 && !noUnknownFields)
      {
        char    tmpBuf[32];
        std::sprintf(tmpBuf, "\t[%5u]", (unsigned int) f.size);
        tmpField.data = tmpBuf;
        for (size_t i = 0; i < f.size; i++)
        {
          if (i >= 17)
          {
            tmpField.data += " ...";
            break;
          }
          std::sprintf(tmpBuf, (!(i & 3) ? "  %02X" : " %02X"),
                       (unsigned int) f.data[i]);
          tmpField.data += tmpBuf;
        }
      }
      if (!tmpField.data.empty())
        fields.push_back(tmpField);
    }
    while (f.next());
  }
  if (fields.size() < 1)
    return;

  std::fputc('\n', outputFile);
  for (size_t i = 0; i < fields.size(); i++)
  {
    std::fprintf(outputFile, "  ");
    printID(fields[i].type);
    std::fprintf(outputFile, ":%s\n", fields[i].data.c_str());
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

int main(int argc, char **argv)
{
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
                throw errorMessage("-F: missing file name");
              fldDefFileName = argv[i];
              continue;
            default:
              break;
          }
        }
        printUsage();
        throw errorMessage("\ninvalid option: %s", argv[i]);
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
        throw errorMessage("too many file names");
      }
    }

    ESMView esmFile(inputFileName, stdout);
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
    std::string cmdBuf;
    std::string prvCmd;
    unsigned int  formID = 0U;
    bool    helpFlag = false;
    bool    noUnknownFields = false;
    while (true)
    {
      if (!cmdBuf.empty() && !helpFlag)
        prvCmd = cmdBuf;
      cmdBuf.clear();
      if (!helpFlag)
      {
        esmFile.dumpRecord(formID, noUnknownFields);
        std::fputc('\n', stdout);
      }
      helpFlag = false;
      std::printf("> ");
      std::fflush(stdout);
      while (true)
      {
        int     c = std::fgetc(stdin);
        if (c == EOF)
        {
          cmdBuf = "q";
          break;
        }
        c = c & 0xFF;
        if (c == '\n')
          break;
        else if (*(cmdBuf.c_str()) == 'd')
          cmdBuf += char(c);
        else if (c >= 'A' && c <= 'Z')
          cmdBuf += (char(c) + ('a' - 'A'));
        else if (c > 0x20 && c < 0x7F)
          cmdBuf += char(c);
      }
      if (cmdBuf.empty())
        cmdBuf = prvCmd;
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
        std::printf("%s\033[1m", (tmpBuf.size() > 0 ? "\n" : ""));
        esmFile.printRecordHdr(formID);
        r = esmFile.getRecordPtr(formID);
        if (!r)
          continue;
        size_t  fileOffs =
            size_t(r->fileData - esmFile.getRecordPtr(0U)->fileData);
        std::printf("Timestamp:  0x%04X\tUser ID:  0x%04X\t"
                    "File pointer:  0x%08X\n\n",
                    (unsigned int) esmFile.getRecordTimestamp(formID),
                    (unsigned int) esmFile.getRecordUserID(formID),
                    (unsigned int) fileOffs & 0xFFFFFFFFU);
        if (r->children)
        {
          std::printf("Child:\t");
          esmFile.printRecordHdr(r->children);
        }
        if (r->next)
        {
          std::printf("Next:\t");
          esmFile.printRecordHdr(r->next);
        }
      }
      else if (cmdBuf == "c")
      {
        unsigned int  newFormID = esmFile.getRecord(formID).children;
        if (newFormID)
        {
          prvRecords.push_back(formID);
          formID = newFormID;
        }
      }
      else if (cmdBuf == "n")
      {
        unsigned int  newFormID = esmFile.getRecord(formID).next;
        if (newFormID)
        {
          prvRecords.push_back(formID);
          formID = newFormID;
        }
      }
      else if (cmdBuf == "p")
      {
        unsigned int  newFormID = esmFile.getRecord(formID).parent;
        if (formID)
        {
          prvRecords.push_back(formID);
          formID = newFormID;
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
          std::printf("Invalid field definition\n");
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
              throw errorMessage("Invalid hexadecimal floating point value");
            if (!(j & 1))
              tmpBuf[j >> 1] = (unsigned char) c << 4;
            else
              tmpBuf[j >> 1] |= (unsigned char) c;
            if (++j >= 8)
            {
              j = 0;
              FileBuffer  buf(tmpBuf, 4);
              double  x = buf.readFloat();
              std::printf(((x > -1.0e7 && x < 1.0e8) ? "%f\n" : "%g\n"), x);
            }
          }
          if (j != 0)
            throw errorMessage("Invalid hexadecimal floating point value");
        }
        catch (std::exception& e)
        {
          std::printf("%s\n", e.what());
          continue;
        }
      }
      else if (cmdBuf[0] == 'g' && cmdBuf.length() > 1)
      {
        unsigned int  tmp = esmFile.findNextGroup(formID, cmdBuf.c_str() + 1);
        if (tmp == 0xFFFFFFFFU)
        {
          helpFlag = true;
          std::printf("Group not found\n");
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
          std::printf("Record not found\n");
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
          std::printf("Record not found\n");
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
        std::printf("Printing unknown fields: %s\n",
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
          std::printf("Invalid form ID\n\n");
          newFormID = formID;
          helpFlag = true;
        }
        if (newFormID != formID)
        {
          prvRecords.push_back(formID);
          formID = newFormID;
        }
      }
      else
      {
        std::printf("0xxxxxx:        hexadecimal form ID\n");
        std::printf("B:              print history of records viewed\n");
        std::printf("C:              first child record\n");
        std::printf("D r:f:name:data define field(s)\n");
        std::printf("F xx xx xx xx:  convert binary floating point value(s)\n");
        std::printf("G cccc:         find next top level group of record type "
                    "cccc\n");
        std::printf("G d:xxxxxxxx:   find group of type d with form ID "
                    "label\n");
        std::printf("G d:d:          find group of type 2 or 3 with int32 "
                    "label\n");
        std::printf("G d:d,d:        find group of type 4 or 5 with X,Y "
                    "label\n");
        std::printf("I:              print short info and all parent groups\n");
        std::printf("L:              back to previously shown record\n");
        std::printf("N:              next record in current group\n");
        std::printf("P:              parent group\n");
        std::printf("R cccc:xxxxxxxx find next reference to form ID in field "
                    "cccc\n");
        std::printf("R *:xxxxxxxx    find next reference to form ID\n");
        std::printf("S pattern:      find next record with EDID matching "
                    "pattern\n");
        std::printf("U:              toggle hexadecimal display of unknown "
                    "field types\n");
        std::printf("Q:              quit\n\n");
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

