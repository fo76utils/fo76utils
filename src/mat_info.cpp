
#include "common.hpp"
#include "ba2file.hpp"
#include "material.hpp"
#include "sdlvideo.hpp"

#ifdef HAVE_SDL2
static void viewMaterials(
    const BA2File& ba2File, const CE2MaterialDB& materials,
    int displayWidth, int displayHeight, const std::vector< std::string >& args)
{
  (void) ba2File;
  (void) materials;
  (void) displayWidth;
  (void) displayHeight;
  (void) args;
}
#endif

static const char *usageString =
    "Usage: mat_info [OPTIONS...] ARCHIVEPATH [FILE1.MAT [FILE2.MAT...]]\n"
    "\n"
    "Options:\n"
    "    --help          print usage\n"
    "    --              remaining options are file names\n"
    "    -cdb FILENAME   set material database file name\n"
    "    -o FILENAME     set output file name (default: stdout)\n"
#ifdef HAVE_SDL2
    "    -view[WIDTHxHEIGHT] run in interactive mode\n"
#endif
    "    -list FILENAME  read list of material paths from FILENAME\n";

int main(int argc, char **argv)
{
  const char  *outFileName = (char *) 0;
  std::FILE *outFile = (std::FILE *) 0;
  bool    consoleFlag = true;
  try
  {
    std::vector< std::string >  args;
    const char  *cdbFileName = (char *) 0;
    const char  *listFileName = (char *) 0;
#ifdef HAVE_SDL2
    int     displayWidth = 0;
    int     displayHeight = 0;
#endif
    for (int i = 1; i < argc; i++)
    {
      if (argv[i][0] != '-')
      {
        args.emplace_back(argv[i]);
        continue;
      }
      if (std::strcmp(argv[i], "--") == 0)
      {
        while (++i < argc)
          args.emplace_back(argv[i]);
        break;
      }
      if (std::strcmp(argv[i], "-h") == 0 ||
          std::strcmp(argv[i], "--help") == 0)
      {
        std::printf("%s", usageString);
        return 0;
      }
      if (std::strcmp(argv[i], "-cdb") == 0)
      {
        if (++i >= argc)
          throw FO76UtilsError("missing argument for %s", argv[i - 1]);
        cdbFileName = argv[i];
      }
      if (std::strcmp(argv[i], "-o") == 0)
      {
        if (++i >= argc)
          throw FO76UtilsError("missing argument for %s", argv[i - 1]);
        outFileName = argv[i];
      }
      else if (std::strncmp(argv[i], "-view", 5) == 0)
      {
#ifdef HAVE_SDL2
        displayWidth = 1792;
        displayHeight = 896;
        std::string tmp(argv[i] + 5);
        size_t  n = tmp.find('x');
        if (n != std::string::npos)
        {
          displayHeight = int(parseInteger(tmp.c_str() + (n + 1), 10,
                                           "invalid image height", 8, 16384));
          tmp.resize(n);
          displayWidth = int(parseInteger(tmp.c_str(), 10,
                                          "invalid image width", 8, 16384));
        }
        else if (!tmp.empty())
        {
          errorMessage("invalid image dimensions");
        }
#else
        errorMessage("interactive mode requires SDL 2");
#endif
      }
      else if (std::strcmp(argv[i], "-list") == 0)
      {
        if (++i >= argc)
          throw FO76UtilsError("missing argument for %s", argv[i - 1]);
        listFileName = argv[i];
      }
      else
      {
        throw FO76UtilsError("invalid option: %s", argv[i]);
      }
    }
    if (args.size() < 1)
    {
      consoleFlag = false;
      SDLDisplay::enableConsole();
      std::fprintf(stderr, "%s", usageString);
      return 1;
    }
    if (listFileName)
    {
      FileBuffer  listFile(listFileName);
      size_t  startPos = 0;
      for (size_t i = 0; i < listFile.size(); i++)
      {
        char    c = '\0';
        if (listFile[i] >= 0x20)
          c = char(listFile[i]);
        if (c == '\0')
          startPos = i + 1;
        else if (i > startPos)
          args.back() += c;
        else
          args.emplace_back(&c, 1);
      }
    }
    const char  *fileNameFilter = ".cdb";
#ifdef HAVE_SDL2
    if (displayWidth && displayHeight)
      fileNameFilter = ".cdb\t.dds";
#endif
    BA2File ba2File(args[0].c_str(), fileNameFilter);
    CE2MaterialDB materials(ba2File, cdbFileName);
#ifdef HAVE_SDL2
    if (displayWidth && displayHeight)
    {
      viewMaterials(ba2File, materials, displayWidth, displayHeight, args);
      return 0;
    }
#endif
    consoleFlag = false;
    SDLDisplay::enableConsole();
    if (outFileName)
    {
      outFile = std::fopen(outFileName, "w");
      if (!outFile)
        errorMessage("error opening output file");
    }
    else
    {
      outFile = stdout;
    }
    std::string stringBuf;
    for (size_t i = 1; i < args.size(); i++)
    {
      const CE2Material *o = materials.findMaterial(args[i]);
      if (!o)
      {
        std::fprintf(stderr, "Material '%s' not found in the database\n",
                     args[i].c_str());
        continue;
      }
      std::fprintf(outFile, "==== %s ====\n", args[i].c_str());
      stringBuf.clear();
      o->printObjectInfo(stringBuf, 0, false);
      std::fwrite(stringBuf.c_str(), sizeof(char), stringBuf.length(), outFile);
    }
  }
  catch (std::exception& e)
  {
    if (consoleFlag)
      SDLDisplay::enableConsole();
    if (outFileName && outFile)
      std::fclose(outFile);
    std::fprintf(stderr, "mat_info: %s\n", e.what());
    return 1;
  }
  if (outFileName && outFile)
    std::fclose(outFile);
  return 0;
}

