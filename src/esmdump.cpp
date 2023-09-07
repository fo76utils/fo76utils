
#include "common.hpp"
#include "esmdbase.hpp"

static void printUsage()
{
  std::fprintf(stderr,
               "esmdump FILENAME.ESM[,...] [LOCALIZATION.BA2 "
               "[STRINGS_PREFIX]] [OPTIONS...]\n\n");
  std::fprintf(stderr, "Options:\n");
  std::fprintf(stderr, "    -h      print usage\n");
  std::fprintf(stderr, "    --      remaining options are file names\n");
  std::fprintf(stderr, "    -o FN   set output file name to FN\n");
  std::fprintf(stderr, "    -F FILE read field definitions from FILE\n");
  std::fprintf(stderr, "    -f 0xN  only include records with flags&N != 0\n");
  std::fprintf(stderr, "    -i TYPE include groups and records of type TYPE\n");
  std::fprintf(stderr, "    -e 0xN  exclude records with flags&N != 0\n");
  std::fprintf(stderr, "    -x TYPE exclude groups and records of type TYPE\n");
  std::fprintf(stderr, "    -d TYPE exclude fields of type TYPE\n");
  std::fprintf(stderr, "    -n      do not print record and field stats\n");
  std::fprintf(stderr, "    -s      only print record and field stats\n");
  std::fprintf(stderr, "    -edid   print EDIDs of form ID fields\n");
  std::fprintf(stderr, "    -t      TSV format output\n");
  std::fprintf(stderr, "    -u      print TSV format version control info\n");
  std::fprintf(stderr, "    -v      verbose mode\n");
}

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    printUsage();
    return 1;
  }
  std::FILE *outputFile = 0;
  try
  {
    const char  *outputFileName = 0;
    const char  *inputFileName = 0;
    const char  *stringsFileName = 0;
    const char  *stringsPrefix = 0;
    const char  *fldDefFileName = 0;
    std::set< const char * >  fieldsExcluded;
    std::set< const char * >  recordsIncluded;
    std::set< const char * >  recordsExcluded;
    unsigned int  flagsIncluded = 0U;
    unsigned int  flagsExcluded = 0U;
    bool    noStats = false;
    bool    statsOnly = false;
    bool    tsvFormat = false;
    bool    noOptionsFlag = false;
    bool    verboseMode = false;
    bool    printEDIDs = false;
    bool    vcFormat = false;
    for (int i = 1; i < argc; i++)
    {
      if (!noOptionsFlag && argv[i][0] == '-')
      {
        if (std::strcmp(argv[i], "--help") == 0)
        {
          printUsage();
          return 0;
        }
        if (argv[i][1] != '\0' && argv[i][2] == '\0')
        {
          switch (argv[i][1])
          {
            case '-':
              noOptionsFlag = true;
              continue;
            case 'F':
              if (++i >= argc)
                errorMessage("-F: missing file name");
              fldDefFileName = argv[i];
              continue;
            case 'd':
              if (++i >= argc)
                errorMessage("-d: no field type");
              if (fieldsExcluded.find(argv[i]) == fieldsExcluded.end())
                fieldsExcluded.insert(argv[i]);
              continue;
            case 'e':
              if (++i >= argc)
                errorMessage("-e: missing flags mask");
              flagsExcluded =
                  (unsigned int) parseInteger(argv[i], 0,
                                              "-e: number required");
              continue;
            case 'f':
              if (++i >= argc)
                errorMessage("-f: missing flags mask");
              flagsIncluded =
                  (unsigned int) parseInteger(argv[i], 0,
                                              "-f: number required");
              continue;
            case 'h':
              printUsage();
              return 0;
            case 'i':
              if (++i >= argc)
                errorMessage("-i: no record type");
              if (recordsIncluded.find(argv[i]) == recordsIncluded.end())
                recordsIncluded.insert(argv[i]);
              continue;
            case 'n':
              noStats = true;
              statsOnly = false;
              continue;
            case 'o':
              if (++i >= argc)
                errorMessage("-o: missing file name");
              outputFileName = argv[i];
              continue;
            case 's':
              noStats = false;
              statsOnly = true;
              continue;
            case 't':
              tsvFormat = true;
              continue;
            case 'u':
              vcFormat = true;
              continue;
            case 'v':
              verboseMode = true;
              continue;
            case 'x':
              if (++i >= argc)
                errorMessage("-x: no record type");
              if (recordsExcluded.find(argv[i]) == recordsExcluded.end())
                recordsExcluded.insert(argv[i]);
              continue;
            default:
              break;
          }
        }
        if (std::strcmp(argv[i], "-edid") == 0)
        {
          printEDIDs = true;
          continue;
        }
        printUsage();
        throw FO76UtilsError("\ninvalid option: %s", argv[i]);
      }
      if (!inputFileName)
      {
        inputFileName = argv[i];
      }
      else if (!stringsFileName)
      {
        stringsFileName = argv[i];
      }
      else if (!stringsPrefix)
      {
        stringsPrefix = argv[i];
      }
      else
      {
        errorMessage("too many file names");
      }
    }

    if (outputFileName)
    {
      outputFile = std::fopen(outputFileName, "w");
      if (!outputFile)
        errorMessage("error opening output file");
    }

    ESMDump esmFile(inputFileName, outputFile);
    if (stringsFileName)
    {
      if (!stringsPrefix)
        stringsPrefix = "strings/starfield_en";
      esmFile.loadStrings(stringsFileName, stringsPrefix);
    }
    esmFile.setRecordFlagsMask(flagsIncluded, flagsExcluded);
    for (std::set< const char * >::iterator i = recordsIncluded.begin();
         i != recordsIncluded.end(); i++)
    {
      esmFile.includeRecordType(*i);
    }
    for (std::set< const char * >::iterator i = recordsExcluded.begin();
         i != recordsExcluded.end(); i++)
    {
      esmFile.excludeRecordType(*i);
    }
    for (std::set< const char * >::iterator i = fieldsExcluded.begin();
         i != fieldsExcluded.end(); i++)
    {
      esmFile.excludeFieldType(*i);
    }

    esmFile.setVerboseMode(verboseMode);
    if (printEDIDs)
      esmFile.findEDIDs();
    if (vcFormat)
    {
      esmFile.dumpVersionInfo();
    }
    else
    {
      esmFile.setTSVFormat(tsvFormat);
      esmFile.setStatsOnlyMode(statsOnly);
      if (fldDefFileName)
        esmFile.loadFieldDefFile(fldDefFileName);
      esmFile.dumpRecord();
      if (!noStats)
      {
        if (!statsOnly)
          std::fputc('\n', (outputFile ? outputFile : stdout));
        esmFile.printStats();
      }
    }

    if (outputFile)
    {
      std::fclose(outputFile);
      outputFile = 0;
    }
  }
  catch (std::exception& e)
  {
    if (outputFile)
      std::fclose(outputFile);
    std::fprintf(stderr, "esmdump: %s\n", e.what());
    return 1;
  }
  return 0;
}

