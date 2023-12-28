
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <set>

static const char *compressCmd = "zopfli --zlib --i100 matdirs.tmp";

int main(int argc, char **argv)
{
  if (argc < 3)
  {
    std::fprintf(stderr, "Usage: %s OUTFILE INFILE1 [INFILE2...]\n", argv[0]);
    return 1;
  }
  std::set< std::string > dirNames;
  std::string s;
  for (int i = 2; i < argc; i++)
  {
    std::FILE *f = std::fopen(argv[i], "rb");
    if (!f)
    {
      std::fprintf(stderr, "%s: error opening file %s\n", argv[0], argv[i]);
      return 1;
    }
    bool    eofFlag = false;
    while (!eofFlag)
    {
      int     c = std::fgetc(f);
      if (c == EOF)
      {
        eofFlag = true;
        c = '\0';
      }
      c = c & 0xFF;
      if (c >= 0x20)
      {
        if (c >= 'A' && c <= 'Z')
          c = c + ('a' - 'A');
        else if (c == '\\')
          c = '/';
        s += char(c);
        continue;
      }
      if (!s.empty())
      {
        if (s.ends_with(".mat") || s.ends_with(".bgsm") || s.ends_with(".bgem"))
        {
          size_t  n = s.rfind('/');
          if (n == std::string::npos)
            s.clear();
          else
            s.resize(n);
        }
        else if (s.ends_with('/'))
        {
          s.resize(s.length() - 1);
        }
        if (!s.empty())
        {
          dirNames.insert(s);
          s.clear();
        }
      }
    }
    std::fclose(f);
  }
  std::FILE *f = std::fopen("matdirs.tmp", "wb");
  if (!f)
  {
    std::fprintf(stderr, "%s: error opening file matdirs.tmp\n", argv[0]);
    return 1;
  }
  size_t  uncompressedSize = 0;
  for (std::set< std::string >::const_iterator
           i = dirNames.begin(); i != dirNames.end(); i++)
  {
    std::fwrite(i->c_str(), sizeof(char), i->length(), f);
    std::fputc('\0', f);
    uncompressedSize += (i->length() + 1);
  }
  std::fclose(f);
  if (std::system(compressCmd) != 0)
  {
    std::fprintf(stderr, "%s: error compressing matdirs.tmp\n", argv[0]);
    return 1;
  }
  std::remove("matdirs.tmp");
  f = std::fopen("matdirs.tmp.zlib", "rb");
  if (!f)
  {
    std::fprintf(stderr, "%s: error opening file matdirs.tmp.zlib\n", argv[0]);
    return 1;
  }
  std::vector< unsigned char >  outBuf;
  while (true)
  {
    int     c = std::fgetc(f);
    if (c == EOF)
      break;
    outBuf.push_back((unsigned char) (c & 0xFF));
  }
  std::fclose(f);
  std::remove("matdirs.tmp.zlib");
  f = std::fopen(argv[1], "w");
  if (!f)
  {
    std::fprintf(stderr, "%s: error opening file %s\n", argv[0], argv[1]);
    return 1;
  }
  std::fprintf(f, "\n#ifndef MAT_DIRS_CPP_INCLUDED\n");
  std::fprintf(f, "#define MAT_DIRS_CPP_INCLUDED\n\n");
  std::fprintf(f, "static const size_t matDirNamesSize = %u;\n\n",
               (unsigned int) uncompressedSize);
  std::fprintf(f, "static const unsigned char  matDirNamesZLib[%u] =\n{\n",
               (unsigned int) outBuf.size());
  for (size_t i = 0; i < outBuf.size(); i++)
  {
    if ((i % 12) == 0)
      std::fprintf(f, "  0x%02X", (unsigned int) outBuf[i]);
    else
      std::fprintf(f, ", 0x%02X", (unsigned int) outBuf[i]);
    if ((i + 1) >= outBuf.size())
      std::fputc('\n', f);
    else if (((i + 1) % 12) == 0)
      std::fprintf(f, ",\n");
  }
  std::fprintf(f, "};\n\n#endif\n\n");
  std::fclose(f);
  return 0;
}

