
#include "common.hpp"
#include "ba2file.hpp"
#include "stringdb.hpp"

StringDB::StringDB()
  : FileBuffer((unsigned char *) 0, 0)
{
}

StringDB::~StringDB()
{
}

void StringDB::clear()
{
  strings.clear();
}

bool StringDB::loadFile(const BA2File& ba2File, const char *stringsPrefix)
{
  strings.clear();
  std::vector< std::string >  fileNames;
  std::string tmpName;
  if (!stringsPrefix)
    stringsPrefix = "strings/seventysix_en";
  for (int k = 0; k < 3; k++)
  {
    static const char *stringsSuffixTable[3] =
    {
      ".strings", ".dlstrings", ".ilstrings"
    };
    tmpName = stringsPrefix;
    tmpName += stringsSuffixTable[k];
    fileNames.push_back(tmpName);
  }
  std::vector< unsigned char >  buf;
  std::vector< std::string >    namesFound;
  ba2File.getFileList(namesFound);
  for (int k = 0; k < 3; k++)
  {
    tmpName.clear();
    for (size_t i = 0; i < namesFound.size(); i++)
    {
      if (namesFound[i].find(fileNames[k]) != std::string::npos)
      {
        tmpName = namesFound[i];
        break;
      }
    }
    if (tmpName.empty())
      continue;
    ba2File.extractFile(buf, tmpName);
    fileBuf = buf.data();
    fileBufSize = buf.size();
    filePos = 0;
    size_t  stringCnt = readUInt32();
    size_t  dataSize = readUInt32();
    if (fileBufSize < dataSize || ((fileBufSize - dataSize) >> 3) <= stringCnt)
      errorMessage("invalid strings file");
    size_t  dataOffs = (stringCnt << 3) + 8;
    std::string s;
    for (size_t i = 0; i < stringCnt; i++)
    {
      unsigned int  id = readUInt32();
      size_t  offs = dataOffs + readUInt32();
      if (offs >= fileBufSize)
        errorMessage("invalid offset in strings file");
      size_t  savedPos = filePos;
      filePos = offs;
      s.clear();
      size_t  len = fileBufSize - filePos;
      if (k != 0)
        len = readUInt32();
      for ( ; len > 0; len--)
      {
        unsigned char c = readUInt8();
        if (!c)
          break;
        if (c >= 0x20)
          s += char(c);
        else if (c == 0x0A)
          s += "<br>";
        else if (c != 0x0D)
          s += ' ';
      }
      filePos = savedPos;
      if (strings.find(id) == strings.end())
      {
        strings.insert(std::pair< unsigned int, std::string >(id, s));
      }
      else
      {
        std::fprintf(stderr, "warning: string 0x%08X redefined\n", id);
        strings[id] = s;
      }
    }
    fileBuf = (unsigned char *) 0;
    fileBufSize = 0;
    filePos = 0;
  }
  return (!strings.empty());
}

bool StringDB::loadFile(const char *archivePath, const char *stringsPrefix)
{
  strings.clear();
  std::vector< std::string >  fileNames;
  std::string tmpName;
  if (!stringsPrefix)
    stringsPrefix = "strings/seventysix_en";
  for (int k = 0; k < 3; k++)
  {
    static const char *stringsSuffixTable[3] =
    {
      ".strings", ".dlstrings", ".ilstrings"
    };
    tmpName = stringsPrefix;
    tmpName += stringsSuffixTable[k];
    fileNames.push_back(tmpName);
  }
  BA2File ba2File(archivePath, &fileNames);
  return loadFile(ba2File, stringsPrefix);
}

bool StringDB::findString(std::string& s, unsigned int id) const
{
  s.clear();
  std::map< unsigned int, std::string >::const_iterator i = strings.find(id);
  if (i == strings.end())
    return false;
  s = i->second;
  return true;
}

std::string StringDB::operator[](size_t id) const
{
  std::map< unsigned int, std::string >::const_iterator i =
    strings.find((unsigned int) id);
  if (i == strings.end())
  {
    char    tmp[16];
    std::sprintf(tmp, "[0x%08x]", (unsigned int) id);
    return std::string(tmp);
  }
  return std::string(i->second);
}

