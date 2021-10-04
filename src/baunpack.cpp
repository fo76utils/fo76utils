
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
      while (s.length() > 0 && s[s.length() - 1] == ' ')
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
    f->writeData(&(buf.front()), sizeof(unsigned char) * buf.size());
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
    std::set< std::string > filesExtracted;
    std::vector< std::string >  includePatterns;
    std::vector< std::string >  excludePatterns;
    int     archiveCnt = argc - 1;
    bool    extractingFiles = false;
    for (int i = 1; i < argc; i++)
    {
      if (std::strcmp(argv[i], "--") == 0)
      {
        archiveCnt = i - 1;
        extractingFiles = true;
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
            includePatterns.push_back(s);
          }
          else
          {
            for (size_t j = 0; argv[i][j] != '\0'; j++)
              s += convertNameCharacter((unsigned char) argv[i][j]);
            if (!s.empty())
              includePatterns.push_back(s);
          }
        }
        if (includePatterns.size() < 1 && fileNames.begin() == fileNames.end())
          includePatterns.push_back(std::string());
        break;
      }
    }
    if (archiveCnt < 1)
    {
      std::fprintf(stderr, "Usage:\n\n");
      std::fprintf(stderr, "%s ARCHIVES...\n", argv[0]);
      std::fprintf(stderr, "    List the contents of archives\n");
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

    std::vector< unsigned char >  outBuf;
    for (int i = 1; i <= archiveCnt; i++)
    {
      BA2File ba2File(argv[i]);
      std::vector< std::string >  fileList;
      ba2File.getFileList(fileList);
      bool    printArchiveFlag = true;
      for (size_t j = 0; j < fileList.size(); j++)
      {
        if (extractingFiles)
        {
          if (filesExtracted.find(fileList[j]) != filesExtracted.end())
            continue;
          bool    nameMatches = false;
          if (fileNames.find(fileList[j]) != fileNames.end())
          {
            fileNames.erase(fileList[j]);
            nameMatches = true;
          }
          else
          {
            for (size_t k = 0; k < includePatterns.size(); k++)
            {
              if (includePatterns[k].empty() ||
                  fileList[j].find(includePatterns[k]) != std::string::npos)
              {
                nameMatches = true;
                break;
              }
            }
          }
          for (size_t k = 0; k < excludePatterns.size(); k++)
          {
            if (fileList[j].find(excludePatterns[k]) != std::string::npos)
            {
              nameMatches = false;
              break;
            }
          }
          if (!nameMatches)
            continue;
        }
        if (printArchiveFlag)
        {
          printArchiveFlag = false;
          std::printf("%s:\n", argv[i]);
        }
        std::printf("%s\t%8u bytes\n",
                    fileList[j].c_str(),
                    (unsigned int) ba2File.getFileSize(fileList[j].c_str()));
        if (extractingFiles)
        {
          ba2File.extractFile(outBuf, fileList[j].c_str());
          writeFileWithPath(fileList[j].c_str(), outBuf);
          filesExtracted.insert(fileList[j]);
        }
      }
    }
    if (fileNames.begin() != fileNames.end())
    {
      std::fprintf(stderr, "\n%s: file(s) not found in archives:\n", argv[0]);
      for (std::set< std::string >::iterator i = fileNames.begin();
           i != fileNames.end(); i++)
      {
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

