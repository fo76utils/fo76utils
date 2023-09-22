
#include "common.hpp"
#include "ba2file.hpp"
#include "material.hpp"

static const char *usageString =
    "Usage: mat_info [OPTIONS...] ARCHIVEPATH [FILE1.MAT [FILE2.MAT...]]\n"
    "\n"
    "Options:\n"
    "    --help          print usage\n"
    "    --              remaining options are file names\n"
    "    -cdb FILENAME   set material database file name\n";

int main(int argc, char **argv)
{
  try
  {
    std::vector< const char * > args;
    const char  *cdbFileName = (char *) 0;
    for (int i = 1; i < argc; i++)
    {
      if (argv[i][0] != '-')
      {
        args.push_back(argv[i]);
        continue;
      }
      if (std::strcmp(argv[i], "--") == 0)
      {
        while (++i < argc)
          args.push_back(argv[i]);
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
      else
      {
        throw FO76UtilsError("invalid option: %s", argv[i]);
      }
    }
    if (args.size() < 1)
    {
      std::fprintf(stderr, "%s", usageString);
      return 1;
    }
    BA2File ba2File(args[0], ".cdb");
    CE2MaterialDB materials(ba2File, cdbFileName);
    std::string stringBuf;
    for (size_t i = 1; i < args.size(); i++)
    {
      const CE2Material *o = materials.findMaterial(std::string(args[i]));
      if (!o)
      {
        std::fprintf(stderr, "Material '%s' not found in the database\n",
                     args[i]);
        continue;
      }
      std::printf("==== %s ====\n", args[i]);
      stringBuf.clear();
      o->printObjectInfo(stringBuf, 0, false);
      std::fwrite(stringBuf.c_str(), sizeof(char), stringBuf.length(), stdout);
    }
  }
  catch (std::exception& e)
  {
    std::fprintf(stderr, "mat_info: %s\n", e.what());
    return 1;
  }
  return 0;
}

