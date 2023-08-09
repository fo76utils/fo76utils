
#include "common.hpp"
#include "filebuf.hpp"
#include "ba2file.hpp"

#if defined(_WIN32) || defined(_WIN64)
#  include <direct.h>
#else
#  include <sys/stat.h>
#endif

static inline char convertNameCharacter(unsigned char c)
{
  if (c >= 'A' && c <= 'Z')
    return char(c + ('a' - 'A'));
  if (c == '\\')
    return '/';
  if (c <= (unsigned char) ' ')
    return ' ';
  return char(c);
}

static void loadFileList(std::set< std::string >& fileNames,
                         const char *listFileName)
{
  FileBuffer  inFile(listFileName);
  std::string s;
  while (true)
  {
    unsigned char c = '\n';
    if (inFile.getPosition() < inFile.size())
      c = inFile.readUInt8();
    if (c == '\t' || c == '\r' || c == '\n')
    {
      while (s.length() > 0 && s.back() == ' ')
        s.resize(s.length() - 1);
      if (!s.empty())
      {
        if (s.find('/') != std::string::npos ||
            s.find('.') != std::string::npos)
        {
          fileNames.insert(s);
        }
        s.clear();
      }
    }
    if (inFile.getPosition() >= inFile.size())
      break;
    if (c > (unsigned char) ' ' || (c == ' ' && !s.empty()))
      s += convertNameCharacter(c);
  }
}

static void writeFileWithPath(const char *fileName,
                              const std::vector< unsigned char >& buf)
{
  OutputFile  *f = (OutputFile *) 0;
  try
  {
    f = new OutputFile(fileName, 0);
  }
  catch (...)
  {
    std::string pathName;
    size_t  pathOffs = 0;
    while (true)
    {
      pathName = fileName;
      pathOffs = pathName.find('/', pathOffs);
      if (pathOffs == size_t(std::string::npos))
        break;
      pathName.resize(pathOffs);
      pathOffs++;
#if defined(_WIN32) || defined(_WIN64)
      (void) _mkdir(pathName.c_str());
#else
      (void) mkdir(pathName.c_str(), 0755);
#endif
    }
    f = new OutputFile(fileName, 0);
  }
  try
  {
    f->writeData(buf.data(), sizeof(unsigned char) * buf.size());
  }
  catch (...)
  {
    delete f;
    throw;
  }
  delete f;
}

int main(int argc, char **argv)
{
  try
  {
    std::set< std::string > fileNames;
    std::set< std::string > namesFound;
    std::vector< std::string >  includePatterns;
    std::vector< std::string >  excludePatterns;
    int     archiveCnt = argc - 1;
    bool    extractingFiles = false;
    bool    listPackedSizes = false;
    for (int i = 1; i < argc; i++)
    {
      if (std::strcmp(argv[i], "--") == 0 ||
          std::strcmp(argv[i], "--list") == 0 ||
          std::strcmp(argv[i], "--list-packed") == 0)
      {
        archiveCnt = i - 1;
        extractingFiles = (argv[i][2] == '\0');
        if (!extractingFiles)
          listPackedSizes = (argv[i][6] != '\0');
        while (++i < argc)
        {
          if (argv[i][0] == '@' && argv[i][1] != '\0')
          {
            loadFileList(fileNames, argv[i] + 1);
            continue;
          }
          std::string s;
          if (argv[i][0] == '-' && argv[i][1] == 'x' && argv[i][2] == ':')
          {
            for (size_t j = 3; argv[i][j] != '\0'; j++)
              s += convertNameCharacter((unsigned char) argv[i][j]);
            if (!s.empty())
              excludePatterns.push_back(s);
          }
          else if (argv[i][0] == '*' && argv[i][1] == '\0')
          {
            includePatterns.clear();
            includePatterns.push_back(std::string());
          }
          else
          {
            for (size_t j = 0; argv[i][j] != '\0'; j++)
              s += convertNameCharacter((unsigned char) argv[i][j]);
            if (!s.empty())
              includePatterns.push_back(s);
          }
        }
        break;
      }
    }
    if (archiveCnt < 1)
    {
      std::fprintf(stderr, "Usage:\n\n");
      std::fprintf(stderr, "%s ARCHIVES...\n", argv[0]);
      std::fprintf(stderr, "    List the contents of .ba2 or .bsa files, "
                           "or archive directories\n");
      std::fprintf(stderr, "%s ARCHIVES... --list [PATTERNS...]\n", argv[0]);
      std::fprintf(stderr, "%s ARCHIVES... --list @LISTFILE\n", argv[0]);
      std::fprintf(stderr, "    List archive contents, filtered by name "
                           "patterns or list file.\n");
      std::fprintf(stderr, "    Using --list-packed instead of --list "
                           "prints compressed file sizes\n");
      std::fprintf(stderr, "%s ARCHIVES... -- [PATTERNS...]\n", argv[0]);
      std::fprintf(stderr, "    Extract files with a name including "
                           "any of the patterns, archives\n");
      std::fprintf(stderr, "    listed first have higher priority. "
                           "Empty pattern list or \"*\"\n");
      std::fprintf(stderr, "    extracts all files, patterns beginning "
                           "with -x: exclude files\n");
      std::fprintf(stderr, "%s ARCHIVES... -- @LISTFILE\n", argv[0]);
      std::fprintf(stderr, "    Extract all valid file names specified "
                           "in the list file. Names\n");
      std::fprintf(stderr, "    are separated by tabs or new lines, "
                           "any string not including\n");
      std::fprintf(stderr, "    at least one /, \\, or . character "
                           "is ignored\n");
      return 1;
    }

    if (!extractingFiles)
    {
      for (int i = 1; i <= archiveCnt; i++)
      {
        BA2File ba2File(argv[i],
                        &includePatterns, &excludePatterns, &fileNames);
        std::vector< std::string >  fileList;
        ba2File.getFileList(fileList);
        if (fileList.size() > 0)
          std::printf("%s:\n", argv[i]);
        for (size_t j = 0; j < fileList.size(); j++)
        {
          if (fileNames.find(fileList[j]) != fileNames.end())
            namesFound.insert(fileList[j]);
          std::printf("%s\t%8u bytes\n",
                      fileList[j].c_str(),
                      (unsigned int) ba2File.getFileSize(fileList[j],
                                                         listPackedSizes));
        }
      }
    }
    else
    {
      std::vector< std::string >  archivePaths;
      for (int i = archiveCnt; i >= 1; i--)
        archivePaths.push_back(std::string(argv[i]));
      BA2File ba2File(archivePaths,
                      &includePatterns, &excludePatterns, &fileNames);
      std::vector< std::string >  fileList;
      ba2File.getFileList(fileList);
      std::vector< unsigned char >  outBuf;
      for (size_t i = 0; i < fileList.size(); i++)
      {
        if (fileNames.find(fileList[i]) != fileNames.end())
          namesFound.insert(fileList[i]);
        std::printf("%s\t%8u bytes\n",
                    fileList[i].c_str(),
                    (unsigned int) ba2File.getFileSize(fileList[i]));
        ba2File.extractFile(outBuf, fileList[i]);
        writeFileWithPath(fileList[i].c_str(), outBuf);
      }
    }
    if (namesFound.size() != fileNames.size())
    {
      std::fprintf(stderr, "\n%s: file(s) not found in archives:\n", argv[0]);
      for (std::set< std::string >::iterator i = fileNames.begin();
           i != fileNames.end(); i++)
      {
        if (namesFound.find(*i) == namesFound.end())
          std::fprintf(stderr, "  %s\n", i->c_str());
      }
      return 1;
    }
  }
  catch (std::exception& e)
  {
    std::fprintf(stderr, "%s: %s\n", argv[0], e.what());
    return 1;
  }
  return 0;
}

