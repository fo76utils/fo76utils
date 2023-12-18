
#include "common.hpp"
#include "ba2file.hpp"
#include "material.hpp"
#include "sdlvideo.hpp"
#include "nif_view.hpp"
#include "esmdbase.hpp"
#include "mat_json.hpp"

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
    "    -list FILENAME  read list of material paths from FILENAME\n"
    "    -dump_db        dump all reflection data\n"
    "    -json           write JSON format .mat file\n";

int main(int argc, char **argv)
{
  const char  *outFileName = nullptr;
  std::FILE *outFile = nullptr;
#ifdef HAVE_SDL2
  NIF_View  *renderer = nullptr;
#endif
  bool    consoleFlag = true;
  bool    dumpReflData = false;
  bool    dumpJSONData = false;
  try
  {
    std::vector< std::string >  args;
    const char  *cdbFileName = nullptr;
    const char  *listFileName = nullptr;
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
                                           "invalid image height", 16, 16384));
          tmp.resize(n);
          displayWidth = int(parseInteger(tmp.c_str(), 10,
                                          "invalid image width", 16, 16384));
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
      else if (std::strcmp(argv[i], "-dump_db") == 0)
      {
        dumpReflData = true;
        dumpJSONData = false;
      }
      else if (std::strcmp(argv[i], "-json") == 0)
      {
        dumpReflData = false;
        dumpJSONData = true;
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
      fileNameFilter = ".cdb\t.dds\t.mesh\t.nif";
#endif
    BA2File ba2File(args[0].c_str(), fileNameFilter);
    if (dumpReflData || dumpJSONData)
    {
      if (!(cdbFileName && *cdbFileName))
        cdbFileName = "materials/materialsbeta.cdb";
      std::vector< unsigned char >  cdbBuf;
      ba2File.extractFile(cdbBuf, std::string(cdbFileName));
      std::string stringBuf;
      std::string errMsgBuf;
      if (dumpReflData)
      {
        CDBDump cdbFile(cdbBuf.data(), cdbBuf.size());
        try
        {
          cdbFile.readAllChunks(stringBuf, 0, true);
        }
        catch (std::exception& e)
        {
          errMsgBuf = e.what();
        }
      }
      else
      {
        CDBMaterialToJSON cdbFile(cdbBuf.data(), cdbBuf.size());
        std::string tmpBuf;
        for (size_t i = 1; i < args.size(); i++)
        {
          if (args.size() > 2)
            printToString(stringBuf, "// %s\n", args[i].c_str());
          cdbFile.dumpMaterial(tmpBuf, args[i]);
          stringBuf += tmpBuf;
          stringBuf += '\n';
        }
      }
      if (!outFileName)
      {
        std::fwrite(stringBuf.c_str(), sizeof(char), stringBuf.length(),
                    stdout);
      }
      else
      {
        outFile = std::fopen(outFileName, "w");
        if (!outFile)
          errorMessage("error opening output file");
        if (std::fwrite(stringBuf.c_str(), sizeof(char), stringBuf.length(),
                        outFile) != stringBuf.length())
        {
          errorMessage("error writing output file");
        }
        std::fclose(outFile);
        outFile = nullptr;
      }
      if (!errMsgBuf.empty())
      {
        std::fprintf(stderr, "mat_info: %s\n", errMsgBuf.c_str());
        return 1;
      }
      return 0;
    }
#ifdef HAVE_SDL2
    if (displayWidth && displayHeight && args.size() > 1)
    {
      args.erase(args.begin(), args.begin() + 1);
      std::sort(args.begin(), args.end());
      renderer = new NIF_View(ba2File, nullptr, cdbFileName);
      SDLDisplay  display(displayWidth, displayHeight, "mat_info", 4U, 48);
      display.setDefaultTextColor(0x00, 0xC0);
      renderer->viewModels(display, args, 0);
      delete renderer;
      return 0;
    }
#endif
    CE2MaterialDB materials(ba2File, cdbFileName);
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
    if (renderer)
      delete renderer;
    std::fprintf(stderr, "mat_info: %s\n", e.what());
    return 1;
  }
  if (outFileName && outFile)
    std::fclose(outFile);
  if (renderer)
    delete renderer;
  return 0;
}

