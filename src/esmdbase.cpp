
#include "common.hpp"
#include "esmdbase.hpp"

void ESMDump::updateStats(unsigned int recordType, unsigned int fieldType)
{
  unsigned long long  key =
      ((unsigned long long) FileBuffer::swapUInt32(recordType) << 32)
      | FileBuffer::swapUInt32(fieldType);
  if (recordStats.find(key) == recordStats.end())
    recordStats.insert(std::pair< unsigned long long, int >(key, 1));
  else
    recordStats[key]++;
}

void ESMDump::printStats()
{
  for (std::map< unsigned long long, int >::iterator i = recordStats.begin();
       i != recordStats.end(); i++)
  {
    unsigned int  recordType =
        FileBuffer::swapUInt32((unsigned int) (i->first >> 32));
    unsigned int  fieldType = FileBuffer::swapUInt32((unsigned int) i->first);
    printID(recordType);
    if (fieldType)
    {
      std::fputc(':', outputFile);
      printID(fieldType);
    }
    std::fprintf(outputFile, ":\t%d\n", i->second);
  }
}

void ESMDump::findEDIDs(unsigned int formID)
{
  do
  {
    const ESMRecord&  r = getRecord(formID);
    if (r.children)
      findEDIDs(r.children);
    if (!(r == "GRUP" || r == "LAND" || r == "NAVM"))
    {
      ESMField  f;
      if (getFirstField(f, r) && f == "EDID")
      {
        std::string s;
        FileBuffer  buf(f.data, f.size);
        printZString(s, buf);
        if (s.empty())
          continue;
        std::map< unsigned int, std::string >::iterator i = edidDB.find(formID);
        if (i != edidDB.end())
          i->second = s;
        else
          edidDB.insert(std::pair< unsigned int, std::string >(formID, s));
      }
    }
    formID = r.next;
  }
  while (formID);
}

void ESMDump::loadFieldDefFile(const char *fileName)
{
  FileBuffer  inFile(fileName);
  loadFieldDefFile(inFile);
}

void ESMDump::loadFieldDefFile(FileBuffer& inFile)
{
  std::vector< unsigned int > recordTypes;
  std::vector< unsigned int > fieldTypes;
  std::string s;
  unsigned int  tabCnt = 0;
  unsigned int  tmp = 0;
  int     lineNumber = 1;
  bool    commentFlag = false;
  bool    eofFlag = false;
  do
  {
    bool    errorFlag = false;
    char    c = '\n';
    if (inFile.getPosition() >= inFile.size())
      eofFlag = true;
    else
      c = char(inFile.readUInt8Fast());
    if (c == '\0' || c == '\r' || c == '\n')
    {
      if (!(tabCnt == 3 || (tabCnt == 0 && recordTypes.size() == 0)))
      {
        throw errorMessage("invalid field definition file format at line %d",
                           lineNumber);
      }
      for (size_t i = 0; i < recordTypes.size(); i++)
      {
        for (size_t j = 0; j < fieldTypes.size(); j++)
        {
          unsigned long long  key =
              ((unsigned long long) recordTypes[i] << 32) | fieldTypes[j];
          std::map< unsigned long long, std::string >::iterator k =
              fieldDefDB.find(key);
          if (k != fieldDefDB.end())
          {
            k->second = s;
          }
          else
          {
            fieldDefDB.insert(std::pair< unsigned long long, std::string >(
                                  key, s));
          }
        }
      }
      recordTypes.clear();
      fieldTypes.clear();
      s.clear();
      tabCnt = 0;
      tmp = 0;
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
        tabCnt++;
        if (tabCnt > 3 || (tabCnt <= 2 && !(tmp & 0xFF)))
          errorFlag = true;
        else if (tabCnt == 1)
          recordTypes.push_back(tmp);
        else if (tabCnt == 2)
          fieldTypes.push_back(tmp);
        else
          s += '\t';
        tmp = 0;
      }
      else if (tabCnt < 2)
      {
        if (c == ',')
        {
          if (!(tmp & 0xFF))
            errorFlag = true;
          else if (tabCnt == 0)
            recordTypes.push_back(tmp);
          else if (tabCnt == 1)
            fieldTypes.push_back(tmp);
          tmp = 0;
        }
        else if ((unsigned char) c > ' ')
        {
          if (c >= 'a' && c <= 'z')
            c = c - ('a' - 'A');
          if (tmp & 0xFF)
            errorFlag = true;
          else
            tmp = (tmp >> 8) | (((unsigned int) c & 0xFFU) << 24);
        }
      }
      else if (tabCnt == 2 || (unsigned char) c > ' ')
      {
        if (tabCnt > 2 && c >= 'A' && c <= 'Z')
          c = c + ('a' - 'A');
        s += c;
      }
    }
    if (errorFlag)
    {
      throw errorMessage("invalid field definition file format at line %d",
                         lineNumber);
    }
  }
  while (!eofFlag);
}

void ESMDump::printID(unsigned int id)
{
  for (int i = 0; i < 4; i++)
  {
    unsigned char c = (unsigned char) (id & 0x7F);
    id = id >> 8;
    if (c < 0x20 || c >= 0x7F)
      c = 0x3F;
    std::fputc(c, outputFile);
  }
}

void ESMDump::printInteger(std::string& s, long long n)
{
  char    tmpBuf[32];
  bool    signFlag = (n < 0LL);
  if (signFlag)
    n = -n;
  tmpBuf[31] = '\0';
  size_t  i = 30;
  for ( ; i > 0 && (n != 0 || i >= 30); i--, n = n / 10)
    tmpBuf[i] = char(n % 10) + '0';
  if (signFlag)
    tmpBuf[i] = '-';
  else
    i++;
  s += &(tmpBuf[i]);
}

void ESMDump::printHexValue(std::string& s, unsigned int n, unsigned int w)
{
  char    tmpBuf[32];
  tmpBuf[0] = '0';
  tmpBuf[1] = 'x';
  tmpBuf[w + 2] = '\0';
  for (unsigned int i = 0; i < w; i++)
  {
    char    c = char((n >> (((w - 1U) - i) << 2)) & 0x0F) + '0';
    if (c > '9')
      c = c + ('A' - ('9' + 1));
    tmpBuf[i + 2] = c;
  }
  s += tmpBuf;
}

void ESMDump::printBoolean(std::string& s, FileBuffer& buf)
{
  if (buf.readUInt8() != 0)
    s += '1';
  else
    s += '0';
}

void ESMDump::printFormID(std::string& s, FileBuffer& buf)
{
  unsigned int  n = buf.readUInt32();
  printHexValue(s, n, 8);
  if (!edidDB.empty())
  {
    std::map< unsigned int, std::string >::iterator k = edidDB.find(n);
    if (k != edidDB.end())
    {
      s += " (";
      s += k->second;
      s += ')';
    }
  }
}

void ESMDump::printFloat(std::string& s, FileBuffer& buf)
{
  char    tmpBuf[256];
  double  x = buf.readFloat();
  std::sprintf(tmpBuf, ((x > -1.0e7 && x < 1.0e8) ? "%.6f" : "%g"), x);
  s += tmpBuf;
}

void ESMDump::printLString(std::string& s, FileBuffer& buf)
{
  unsigned int  n = buf.readUInt32();
  if (n)
    s += strings[n];
}

void ESMDump::printZString(std::string& s, FileBuffer& buf)
{
  while (buf.getPosition() < buf.size())
  {
    unsigned char c = buf.readUInt8Fast();
    if (!c)
      break;
    if (c >= 0x20)
      s += char(c);
  }
}

void ESMDump::printFileName(std::string& s, FileBuffer& buf)
{
  while (buf.getPosition() < buf.size())
  {
    unsigned char c = buf.readUInt8Fast();
    if (!c)
      break;
    if (c < 0x20)
      continue;
    if (c >= 'A' && c <= 'Z')
      c = c + ('a' - 'A');
    else if (c == '\\')
      c = '/';
    s += char(c);
  }
}

void ESMDump::convertField(std::string& s,
                           const ESMField& f, const std::string& fldDef)
{
  const char  *dataTypes = fldDef.c_str();
  size_t  n = fldDef.find('\t');
  if (n != std::string::npos)
  {
    dataTypes = fldDef.c_str() + (n + 1);
    if (n > 0 && !tsvFormat)
    {
      s = " ";
      s.insert(s.length(), fldDef, 0, n);
    }
  }
  FileBuffer  buf(f.data, f.size);
  size_t  arrayPos = std::string::npos;
  bool    haveData = false;
  for (size_t i = 0; dataTypes[i] != '\0'; i++)
  {
    char    c = dataTypes[i];
    if (c == '*')
      break;
    size_t  sizeRequired = 1;
    if (c == 'h' || c == 's')
      sizeRequired = 2;
    else if ((unsigned char) c > 'c' && c != 'z' && c != 'n')
      sizeRequired = 4;
    if ((buf.getPosition() + sizeRequired) > buf.size())
    {
      if (arrayPos == std::string::npos)
        haveData = false;
      break;
    }
    if (c != '.')
      s += '\t';
    size_t  prvLen = s.length();
    switch (c)
    {
      case '<':
        if (dataTypes[i + 1] != '\0' && dataTypes[i + 1] != '<')
          arrayPos = i;
        break;
      case 'b':
        printBoolean(s, buf);
        break;
      case 'c':
        printHexValue(s, buf.readUInt8(), 2);
        break;
      case 'h':
        printHexValue(s, buf.readUInt16(), 4);
        break;
      case 's':
        printInteger(s, buf.readInt16());
        break;
      case 'x':
        printHexValue(s, buf.readUInt32(), 8);
        break;
      case 'u':
        printInteger(s, buf.readUInt32());
        break;
      case 'i':
        printInteger(s, buf.readInt32());
        break;
      case 'd':
        printFormID(s, buf);
        break;
      case 'f':
        printFloat(s, buf);
        break;
      case 'l':
        printLString(s, buf);
        break;
      case 'z':
        printZString(s, buf);
        break;
      case 'n':
        printFileName(s, buf);
        break;
      case '.':
        (void) buf.readUInt8Fast();
        break;
      default:
        throw errorMessage("invalid data type in field definition");
    }
    if (s.length() != prvLen)
      haveData = true;
    if (dataTypes[i + 1] == '\0' && arrayPos != std::string::npos)
      i = arrayPos;
  }
  if (!haveData)
    s.clear();
  while (s.length() > 0 && s[s.length() - 1] == '\t')
    s.resize(s.length() - 1);
}

void ESMDump::convertField(std::string& s,
                           const ESMRecord& r, const ESMField& f)
{
  if (fieldDefDB.begin() != fieldDefDB.end())
  {
    unsigned long long  key = ((unsigned long long) r.type << 32) | f.type;
    std::map< unsigned long long, std::string >::const_iterator i =
        fieldDefDB.find(key);
    if (i != fieldDefDB.end())
    {
      convertField(s, f, i->second);
      return;
    }
  }
  //  1: boolean
  //  2: 16-bit integer flags
  //  3: 16-bit signed integer
  //  4: 32-bit unsigned integer
  //  5: 32-bit signed integer
  //  6: form ID
  //  7: float
  //  8: localized string
  //  9: string
  // 10: file name
  int     dataTypes[8];
  int     dataCnt = 1;
  int     arraySize = 1;
  int&    dataType = dataTypes[0];
  dataType = 0;
  bool    exactSizeRequired = true;
  switch (f.type)
  {
    case 0x4E444C41:            // "ALDN"
    case 0x52464C41:            // "ALFR"
    case 0x54524C41:            // "ALRT"
    case 0x4C544551:            // "QETL"
    case 0x4D544F51:            // "QOTM"
    case 0x4D4C5451:            // "QTLM"
      if (!tsvFormat && r == "QUST")
        dataType = 6;
      break;
    case 0x44494C41:            // "ALID"
    case 0x32534943:            // "CIS2"
    case 0x4C525544:            // "DURL"
    case 0x52544C46:            // "FLTR"
    case 0x52565451:            // "QTVR"
    case 0x4D434353:            // "SCCM"
    case 0x43464353:            // "SCFC"
      if (!tsvFormat && r == "QUST")
        dataType = 9;
      break;
    case 0x4D414E41:            // "ANAM"
      if (!tsvFormat)
        dataType = 6;
      break;
    case 0x4D414E42:            // "BNAM"
      if (r == "LTEX")
        dataType = 10;
      break;
    case 0x54585442:            // "BTXT"
    case 0x54585449:            // "ITXT"
      if (r == "TERM" && !tsvFormat)
        dataType = 8;
      break;
    case 0x45565243:            // "CRVE"
      dataType = 10;
      break;
    case 0x41544144:            // "DATA"
      if (r == "GMST")
      {
        ESMField  tmpField;
        if (getFirstField(tmpField, r))
        {
          if (tmpField == "EDID" && tmpField.size > 0)
          {
            switch (tmpField.data[0])
            {
              case 'b':
                dataType = 1;
                break;
              case 'f':
                dataType = 7;
                break;
              case 'i':
                dataType = 5;
                break;
              case 's':
                dataType = 8;
                break;
              case 'u':
                dataType = 4;
                break;
            }
          }
        }
      }
      else if (r == "CELL")
      {
        dataType = 2;
        exactSizeRequired = false;
      }
      else if (verboseMode && r == "REFR")
      {
        dataType = 7;
        arraySize = 6;
      }
      break;
    case 0x43534544:            // "DESC"
    case 0x4C4C5546:            // "FULL"
      dataType = 8;
      break;
    case 0x4D414E44:            // "DNAM"
      if (r == "KYWD" || r == "IDLE")
      {
        dataType = 9;
      }
      else if (r == "WRLD")
      {
        dataType = 7;
        arraySize = 2;
      }
      break;
    case 0x44494445:            // "EDID"
      dataType = 9;
      break;
    case 0x4C494345:            // "ECIL"
    case 0x49445445:            // "ETDI"
    case 0x50495445:            // "ETIP"
    case 0x32444F4D:            // "MOD2"
      dataType = 10;
      break;
    case 0x45435345:            // "ESCE"
    case 0x53435345:            // "ESCS"
      if (!tsvFormat)
      {
        dataType = 6;
      }
      break;
    case 0x56544C46:            // "FLTV"
      dataType = 7;
      break;
    case 0x4144574B:            // "KWDA"
      if (!tsvFormat)
      {
        dataType = 6;
        arraySize = int(f.size >> 2);
      }
      break;
    case 0x4F4C564C:            // "LVLO"
      if (r == "LVLI")
      {
        dataType = 6;
      }
      break;
    case 0x4C444F4D:            // "MODL"
      dataType = (!(r == "ARMO" || r == "ARMA") ? 10 : 6);
      break;
    case 0x4D414E4D:            // "MNAM"
      if (r == "TXST")
        dataType = 10;
      break;
    case 0x314D414E:            // "NAM1"
      dataType = ((r == "INFO" || r == "QUST") ? 8 : 6);
      break;
    case 0x324D414E:            // "NAM2"
      if (!tsvFormat && (r == "INFO" || r == "QUST"))
      {
        dataType = 9;
      }
      break;
    case 0x454D414E:            // "NAME"
      if (verboseMode && r == "REFR")
      {
        dataType = 6;
      }
      break;
    case 0x444E424F:            // "OBND"
      if (!tsvFormat)
      {
        dataType = 3;
        arraySize = 6;
      }
      break;
    case 0x49504550:            // "PEPI"
    case 0x4D414E53:            // "SNAM"
      if (!tsvFormat && r == "QUST")
        dataType = 10;
      break;
    case 0x4D414E52:            // "RNAM"
      dataType = ((r == "INFO" || r == "TERM") ? 8 : 6);
      break;
    case 0x544C4354:            // "TCLT"
      if (!tsvFormat && r == "INFO")
        dataType = 6;
      break;
    case 0x30305854:            // "TX00"
    case 0x31305854:            // "TX01"
    case 0x32305854:            // "TX02"
    case 0x33305854:            // "TX03"
    case 0x34305854:            // "TX04"
    case 0x35305854:            // "TX05"
    case 0x36305854:            // "TX06"
    case 0x37305854:            // "TX07"
      dataType = 10;
      break;
    case 0x434C4358:            // "XCLC"
      if (r == "CELL")
      {
        dataType = 5;
        arraySize = 2;
        exactSizeRequired = false;
      }
      break;
    case 0x574C4358:            // "XCLW"
      if (r == "CELL")
      {
        dataType = 7;
      }
      break;
    case 0x4C435358:            // "XSCL"
      if (verboseMode && r == "REFR")
      {
        dataType = 7;
      }
      break;
  }
  if (!dataType)
  {
    if (verboseMode && (f.type & 0xFFFFFF00U) == 0x4D414E00 && f.size == 4)
    {
      // "*NAM"
      FileBuffer    buf(f.data, f.size);
      unsigned int  n = buf.readUInt32();
      if (n >= 256U && n < 0x80000000U && getRecordPtr(n))
        dataType = 6;
    }
    if (!dataType)
      return;
  }
  size_t  dataSize = 0;
  for (int i = 0; i < dataCnt; i++)
  {
    if (dataTypes[i] >= 4 && dataTypes[i] <= 8)
    {
      dataSize = dataSize + 4;
    }
    else if (dataTypes[i] >= 2 && dataTypes[i] <= 3)
    {
      dataSize = dataSize + 2;
    }
    else
    {
      dataSize = dataSize + 1;
      exactSizeRequired = false;
    }
  }
  dataSize = dataSize * size_t(arraySize);
  if ((exactSizeRequired && f.size != dataSize) || f.size < dataSize)
    return;
  FileBuffer  buf(f.data, f.size);
  for (int i = 0; i < arraySize; i++)
  {
    for (int j = 0; j < dataCnt; j++)
    {
      s += '\t';
      switch (dataTypes[j])
      {
        case 1:                 // boolean
          printBoolean(s, buf);
          break;
        case 2:                 // 16-bit integer flags
          printHexValue(s, buf.readUInt16(), 4);
          break;
        case 3:                 // 16-bit signed integer
          printInteger(s, buf.readInt16());
          break;
        case 4:                 // 32-bit unsigned integer
          printInteger(s, buf.readUInt32());
          break;
        case 5:                 // 32-bit signed integer
          printInteger(s, buf.readInt32());
          break;
        case 6:                 // form ID
          printFormID(s, buf);
          break;
        case 7:                 // float
          printFloat(s, buf);
          break;
        case 8:                 // localized string
          printLString(s, buf);
          break;
        case 9:                 // string
          printZString(s, buf);
          break;
        case 10:                // file name
          printFileName(s, buf);
          break;
      }
    }
  }
  if (s.length() == 1)
    s.clear();
}

ESMDump::ESMDump(const char *fileName, std::FILE *outFile)
  : ESMFile(fileName),
    outputFile(outFile),
    flagsIncluded(0U),
    flagsExcluded(0U),
    tsvFormat(false),
    statsOnly(false),
    haveStrings(false),
    verboseMode(false)
{
  if (!outputFile)
    outputFile = stdout;
}

ESMDump::~ESMDump()
{
}

void ESMDump::setRecordFlagsMask(unsigned int includeMask,
                                 unsigned int excludeMask)
{
  flagsIncluded = includeMask;
  flagsExcluded = excludeMask;
}

static unsigned int convertRecordType(const char *s)
{
  unsigned int  tmp = 0U;
  if (s)
  {
    for (int i = 0; i < 4; i++)
    {
      char    c = s[i];
      if (!c)
        break;
      if (c >= 'a' && c <= 'z')
        c = c + 'A' - 'a';
      tmp = tmp | ((unsigned int) ((unsigned char) c) << (i << 3));
    }
  }
  return tmp;
}

void ESMDump::includeRecordType(const char *s)
{
  unsigned int  tmp = convertRecordType(s);
  if (tmp && recordsIncluded.find(tmp) == recordsIncluded.end())
    recordsIncluded.insert(tmp);
}

void ESMDump::excludeRecordType(const char *s)
{
  unsigned int  tmp = convertRecordType(s);
  if (tmp && recordsExcluded.find(tmp) == recordsExcluded.end())
    recordsExcluded.insert(tmp);
}

void ESMDump::excludeFieldType(const char *s)
{
  unsigned int  tmp = convertRecordType(s);
  if (tmp && fieldsExcluded.find(tmp) == fieldsExcluded.end())
    fieldsExcluded.insert(tmp);
}

void ESMDump::setTSVFormat(bool isEnabled)
{
  tsvFormat = isEnabled;
  if (tsvFormat)
  {
    std::fprintf(outputFile,
                 "Group\tRecord\tFormID\tEDID\tFULL\tDESC\tRNAM\tData...\n");
  }
}

void ESMDump::loadStrings(const char *fileName, const char *stringsPrefix)
{
  haveStrings = false;
  strings.clear();
  if (!fileName)
    return;
  haveStrings = strings.loadFile(fileName, stringsPrefix);
}

void ESMDump::dumpRecord(unsigned int formID, const ESMRecord *parentGroup)
{
  do
  {
    const ESMRecord&  r = getRecord(formID);
    unsigned int  recordType = r.type;
    if (r == "GRUP")
    {
      if (r.children)
      {
        if (verboseMode && r.formID <= 9 && !tsvFormat)
        {
          std::fprintf(outputFile, "GRP{:\t%u\t", r.formID);
          switch (r.formID)
          {
            case 0:
              printID(r.flags);
              std::fputc('\n', outputFile);
              break;
            case 1:
            case 6:
            case 7:
            case 8:
            case 9:
              std::fprintf(outputFile, "0x%08X\n", r.flags);
              break;
            case 2:
            case 3:
              std::fprintf(outputFile, "%d\n", uint32ToSigned(r.flags));
              break;
            case 4:
            case 5:
              std::fprintf(outputFile, "%d\t%d\n",
                           uint16ToSigned((unsigned short) (r.flags >> 16)),
                           uint16ToSigned((unsigned short) (r.flags & 0xFFFF)));
              break;
          }
        }
        dumpRecord(r.children, (r.formID ? parentGroup : &r));
        if (verboseMode && r.formID <= 9 && !tsvFormat)
          std::fprintf(outputFile, "}GRP:\n");
      }
      formID = r.next;
      continue;
    }

    unsigned int  parentGroupID = 0U;
    if (parentGroup)
      parentGroupID = parentGroup->flags;
    unsigned int  flags = r.flags;
    if ((flags & flagsExcluded) || (flagsIncluded && !(flags & flagsIncluded)))
    {
      formID = r.next;
      continue;
    }
    if (recordsIncluded.begin() != recordsIncluded.end())
    {
      if (recordsIncluded.find(parentGroupID) == recordsIncluded.end() &&
          recordsIncluded.find(recordType) == recordsIncluded.end())
      {
        formID = r.next;
        continue;
      }
    }
    if (recordsExcluded.begin() != recordsExcluded.end())
    {
      if (recordsExcluded.find(parentGroupID) != recordsExcluded.end() ||
          recordsExcluded.find(recordType) != recordsExcluded.end())
      {
        formID = r.next;
        continue;
      }
    }
    updateStats(recordType, 0);

    std::string edid;
    std::vector< Field >  fields;
    ESMField  f;
    if (getFirstField(f, r))
    {
      do
      {
        Field   tmpField;
        tmpField.type = f.type;
        updateStats(recordType, f.type);
        if (!statsOnly && fieldsExcluded.find(f.type) == fieldsExcluded.end())
        {
          if (f == "EDID")
          {
            convertField(edid, r, f);
          }
          else
          {
            convertField(tmpField.data, r, f);
            if (!tmpField.data.empty())
              fields.push_back(tmpField);
          }
        }
      }
      while (f.next());
    }
    if (edid.empty() && fields.size() < 1)
    {
      formID = r.next;
      continue;
    }

    if (tsvFormat)
    {
      std::vector< std::string >  tsvFields(4, std::string("\t"));
      if (!edid.empty())
        tsvFields[0] = edid;
      for (size_t i = 0; i < fields.size(); i++)
      {
        if (FileBuffer::checkType(fields[i].type, "FULL"))
        {
          if (tsvFields[1].length() < 2 || r == "NPC_")
            tsvFields[1] = fields[i].data;
        }
        else if (FileBuffer::checkType(fields[i].type, "DESC"))
        {
          tsvFields[2] = fields[i].data;
        }
        else if (FileBuffer::checkType(fields[i].type, "RNAM"))
        {
          tsvFields[3] = fields[i].data;
        }
        else
        {
          tsvFields.push_back(fields[i].data);
        }
      }
      if (parentGroup)
        printID(parentGroupID);
      std::fputc('\t', outputFile);
      printID(recordType);
      std::fprintf(outputFile, "\t0x%08X", formID);
      for (size_t i = 0; i < tsvFields.size(); i++)
        std::fprintf(outputFile, "%s", tsvFields[i].c_str());
      std::fputc('\n', outputFile);
    }
    else
    {
      if (parentGroup && parentGroupID != recordType)
      {
        printID(parentGroupID);
        std::fputc(':', outputFile);
      }
      printID(recordType);
      if (edid.empty())
        std::fprintf(outputFile, ":\t0x%08X\n", formID);
      else
        std::fprintf(outputFile, ":\t0x%08X%s\n", formID, edid.c_str());
      for (size_t i = 0; i < fields.size(); i++)
      {
        std::fputc(':', outputFile);
        printID(fields[i].type);
        std::fprintf(outputFile, "%s\n", fields[i].data.c_str());
      }
    }
    formID = r.next;
  }
  while (formID);
}

