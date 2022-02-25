
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

bool StringDB::loadFile(const char *fileName, const char *stringsPrefix)
{
  strings.clear();
  BA2File ba2File(fileName);
  std::vector< unsigned char >  buf;
  std::vector< std::string >    fileNames;
  ba2File.getFileList(fileNames);
  std::string tmpName;
  for (int k = 0; k < 3; k++)
  {
    tmpName = (stringsPrefix ? stringsPrefix : "strings/seventysix_en");
    if (k == 0)
      tmpName += ".strings";
    else if (k == 1)
      tmpName += ".dlstrings";
    else
      tmpName += ".ilstrings";
    for (size_t i = 0; i < fileNames.size(); i++)
    {
      if (fileNames[i].find(tmpName) != std::string::npos)
      {
        tmpName = fileNames[i];
        break;
      }
      if ((i + 1) >= fileNames.size())
      {
        tmpName.clear();
        break;
      }
    }
    if (tmpName.empty())
      continue;
    ba2File.extractFile(buf, tmpName);
    fileBuf = &(buf.front());
    fileBufSize = buf.size();
    filePos = 0;
    size_t  stringCnt = readUInt32();
    size_t  dataSize = readUInt32();
    if (fileBufSize < dataSize || ((fileBufSize - dataSize) >> 3) <= stringCnt)
      throw errorMessage("invalid strings file");
    size_t  dataOffs = (stringCnt << 3) + 8;
    std::string s;
    for (size_t i = 0; i < stringCnt; i++)
    {
      unsigned int  id = readUInt32();
      size_t  offs = dataOffs + readUInt32();
      if (offs >= fileBufSize)
        throw errorMessage("invalid offset in strings file");
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

