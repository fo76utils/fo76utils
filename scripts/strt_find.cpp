
#include "common.hpp"
#include "filebuf.hpp"
#include "ba2file.hpp"
#include "esmfile.hpp"

static void loadStrings(std::set< std::string >& cdbStrings, FileBuffer& buf)
{
  if (buf.size() < 24)
    return;
  if (buf.readUInt64() != 0x0000000848544542ULL)        // "BETH", 8
    return;
  if (buf.readUInt32() != 4U)
    return;
  (void) buf.readUInt32();
  if (buf.readUInt32() != 0x54525453U)                  // "STRT"
    return;
  std::string s;
  size_t  n = buf.readUInt32();
  while (n--)
  {
    char    c = char(buf.readUInt8());
    if (c)
    {
      s += c;
    }
    else if (!s.empty())
    {
      cdbStrings.insert(s);
      s.clear();
    }
  }
  if (!s.empty())
    cdbStrings.insert(s);
}

static const char *predefinedCDBStrings[19] =
{
  "None",         "String",       "List",         "Map",        // -255 to -252
  "Ref",          "Unknown_-250", "Unknown_-249", "Int8",       // -251 to -248
  "UInt8",        "Int16",        "UInt16",       "Int32",      // -247 to -244
  "UInt32",       "Int64",        "UInt64",       "Bool",       // -243 to -240
  "Float",        "Double",       "Unknown"                     // -239 to -237
};

int main()
{
  std::set< std::string > cdbStrings;
  BA2File ba2File("Starfield/Data");
  {
    std::vector< unsigned char >  tmpBuf;
    ba2File.extractFile(tmpBuf, std::string("materials/materialsbeta.cdb"));
    FileBuffer  buf(tmpBuf.data(), tmpBuf.size());
    loadStrings(cdbStrings, buf);
  }
  ESMFile esmFile("Starfield/Data/Starfield.esm");
  for (unsigned int i = 0U; i <= 0x0FFFFFFFU; i++)
  {
    const ESMFile::ESMRecord  *r = esmFile.findRecord(i);
    if (!r)
      continue;
    ESMFile::ESMField f(esmFile, *r);
    while (f.next())
      loadStrings(cdbStrings, f);
  }
  std::set< std::string >::iterator i = cdbStrings.begin();
  for (unsigned int n = 0U; i != cdbStrings.end(); n++)
  {
    const char  *s;
    if (n < (unsigned int) (sizeof(predefinedCDBStrings) / sizeof(char *)))
    {
      s = predefinedCDBStrings[n];
    }
    else
    {
      s = i->c_str();
      i++;
    }
    std::printf("  \"%s\", ", s);
    for (size_t j = std::strlen(s); j < 50; j++)
      std::fputc(' ', stdout);
    std::printf("// %4u\n", n);
  }
  return 0;
}

