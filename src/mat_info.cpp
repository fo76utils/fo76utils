
#include "common.hpp"
#include "ba2file.hpp"
#include "material.hpp"

int main(int argc, char **argv)
{
  try
  {
    if (argc < 3)
    {
      std::fprintf(stderr,
                   "Usage: mat_info ARCHIVEPATH FILE1.MAT [FILE2.MAT...]\n");
      return 1;
    }
    BA2File ba2File(argv[1], ".cdb");
    CE2MaterialDB materials(ba2File);
  }
  catch (std::exception& e)
  {
    std::fprintf(stderr, "mat_info: %s\n", e.what());
    return 1;
  }
  return 0;
}

