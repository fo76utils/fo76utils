
#include "common.hpp"
#include "ba2file.hpp"
#include "material.hpp"
#include "sdlvideo.hpp"
#include "nif_view.hpp"
#include "bsmatcdb.hpp"
#include "zlib.hpp"

#if defined(_WIN32) || defined(_WIN64)
#  include <direct.h>
#else
#  include <sys/stat.h>
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
    "    -list FILENAME  read list of material paths from FILENAME\n"
    "    -all            use built-in list of all known material paths\n"
    "    -dump_list      print list of .mat paths (useful with -all)\n"
    "    -dump_db        dump all reflection data\n"
    "    -json           write JSON format .mat file(s)\n";

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

static void writeFileWithPath(const char *fileName, const std::string& buf)
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
    f->writeData(buf.c_str(), sizeof(char) * buf.length());
  }
  catch (...)
  {
    delete f;
    throw;
  }
  delete f;
}

static void loadCDBFiles(std::vector< std::vector< unsigned char > >& bufs,
                         const BA2File& ba2File, const char *fileName)
{
  if (!fileName || fileName[0] == '\0')
    fileName = BSReflStream::getDefaultMaterialDBPath();
  while (true)
  {
    size_t  len = 0;
    while (fileName[len] != ',' && fileName[len] != '\0')
      len++;
    bufs.emplace_back();
    ba2File.extractFile(bufs.back(), std::string(fileName, len));
    if (!fileName[len])
      break;
    fileName = fileName + (len + 1);
  }
}

#include "mat_dirs.cpp"

struct MatFileObject
{
  const unsigned char *objectInfo;
  std::string baseName;
};

static void loadMaterialPaths(
    std::vector< std::string >& args,
    const std::vector< std::vector< unsigned char > >& cdbBufs,
    const std::vector< std::string >& includePatterns,
    const std::vector< std::string >& excludePatterns)
{
  std::set< std::string > matPaths;
  for (size_t i = 1; i < args.size(); i++)
    matPaths.insert(args[i]);
  args.resize(1);
  std::map< std::uint32_t, std::string >  dirNameMap;
  std::string tmp;
  {
    std::vector< unsigned char >  tmpBuf(matDirNamesSize + 1, 0);
    (void) ZLibDecompressor::decompressData(tmpBuf.data(), matDirNamesSize,
                                            matDirNamesZLib,
                                            sizeof(matDirNamesZLib));
    for (size_t i = 0; i < tmpBuf.size(); i++)
    {
      tmp = reinterpret_cast< char * >(tmpBuf.data() + i);
      std::uint32_t h = 0U;
      for ( ; tmpBuf[i]; i++)
      {
        unsigned char c = tmpBuf[i];
        if (c == '/')
          c = '\\';
        hashFunctionCRC32(h, c);
      }
      tmp += '/';
      dirNameMap[h] = tmp;
    }
  }
  for (size_t i = 0; i < cdbBufs.size(); i++)
  {
    BSReflStream  cdbFile(cdbBufs[i].data(), cdbBufs[i].size());
    std::map< std::uint32_t, MatFileObject >  matFileObjects;
    const unsigned char *componentInfoPtr = nullptr;
    size_t  componentCnt = 0;
    size_t  componentID = 0;
    BSReflStream::Chunk chunkBuf;
    unsigned int  chunkType;
    while ((chunkType = cdbFile.readChunk(chunkBuf))
           != BSReflStream::ChunkType_NONE)
    {
      size_t  className = BSReflStream::String_Unknown;
      if (chunkBuf.size() >= 4) [[likely]]
        className = cdbFile.findString(chunkBuf.readUInt32Fast());
      if ((chunkType == BSReflStream::ChunkType_DIFF ||
           chunkType == BSReflStream::ChunkType_OBJT) &&
          componentID < componentCnt) [[likely]]
      {
        if (className == BSReflStream::String_BSComponentDB_CTName)
        {
          std::uint32_t objectID =
              FileBuffer::readUInt32Fast(componentInfoPtr + (componentID << 3));
          std::map< std::uint32_t, MatFileObject >::iterator  j =
              matFileObjects.find(objectID);
          if (j != matFileObjects.end())
          {
            bool    isDiff = (chunkType == BSReflStream::ChunkType_DIFF);
            for (unsigned int n = 0U - 1U;
                 chunkBuf.getFieldNumber(n, 0U, isDiff); )
            {
              if (!chunkBuf.readString(tmp))
                break;
              if (!tmp.empty())
              {
                for (size_t k = 0; k < tmp.length(); k++)
                  tmp[k] = convertNameCharacter((unsigned char) tmp[k]);
                j->second.baseName = tmp;
              }
            }
          }
        }
        componentID++;
        continue;
      }
      if (chunkType != BSReflStream::ChunkType_LIST)
        continue;
      unsigned int  n = 0U;
      if (chunkBuf.size() >= 8)
        n = chunkBuf.readUInt32Fast();
      switch (className)
      {
        case BSReflStream::String_BSComponentDB2_DBFileIndex_ObjectInfo:
          while (n--)
          {
            if ((chunkBuf.getPosition() + 21ULL) > chunkBuf.size())
              break;
            const unsigned char *p = chunkBuf.data() + chunkBuf.getPosition();
            chunkBuf.setPosition(chunkBuf.getPosition() + 21);
            if (FileBuffer::readUInt32Fast(p + 4) == 0x0074616DU && p[20])
            {
              // PersistentID.File = "mat\0" and HasData = true
              matFileObjects[FileBuffer::readUInt32Fast(p + 12)].objectInfo = p;
            }
          }
          break;
        case BSReflStream::String_BSComponentDB2_DBFileIndex_ComponentInfo:
          componentInfoPtr = chunkBuf.data() + chunkBuf.getPosition();
          componentCnt =
              std::min(size_t(n),
                       (chunkBuf.size() - chunkBuf.getPosition()) >> 3);
          componentID = 0;
          break;
        case BSReflStream::String_BSComponentDB2_DBFileIndex_EdgeInfo:
          while (n--)
          {
            if ((chunkBuf.getPosition() + 12ULL) > chunkBuf.size())
              break;
            std::uint32_t objectID = chunkBuf.readUInt32Fast();
            chunkBuf.setPosition(chunkBuf.getPosition() + 8);
            std::map< std::uint32_t, MatFileObject >::iterator  j =
                matFileObjects.find(objectID);
            if (j != matFileObjects.end())
              matFileObjects.erase(j);  // remove LOD materials
          }
          break;
      }
    }
    for (std::map< std::uint32_t, MatFileObject >::const_iterator
             j = matFileObjects.begin(); j != matFileObjects.end(); j++)
    {
      const std::string *baseName = &(j->second.baseName);
      std::map< std::uint32_t, MatFileObject >::const_iterator  k = j;
      while (baseName->empty())
      {
        std::uint32_t parentID =
            FileBuffer::readUInt32Fast(k->second.objectInfo + 16);
        if (!parentID)
          break;
        k = matFileObjects.find(parentID);
        if (k == matFileObjects.end())
          break;
        baseName = &(k->second.baseName);
      }
      if (baseName->empty())
        continue;
      std::map< std::uint32_t, std::string >::const_iterator  l =
          dirNameMap.find(FileBuffer::readUInt32Fast(j->second.objectInfo + 8));
      if (l == dirNameMap.end())
        continue;
      tmp = *baseName;
      std::uint32_t h = 0U;
      for (size_t m = 0; m < tmp.length(); m++)
        hashFunctionCRC32(h, (unsigned char) tmp[m]);
      if (h == FileBuffer::readUInt32Fast(j->second.objectInfo))
      {
        tmp.insert(0, l->second);
        tmp += ".mat";
        matPaths.insert(tmp);
      }
    }
  }
  for (std::set< std::string >::const_iterator
           i = matPaths.begin(); i != matPaths.end(); i++)
  {
    if (includePatterns.begin() != includePatterns.end())
    {
      bool    foundMatch = false;
      for (std::vector< std::string >::const_iterator
               j = includePatterns.begin(); j != includePatterns.end(); j++)
      {
        if (i->find(*j) != std::string::npos)
        {
          foundMatch = true;
          break;
        }
      }
      if (!foundMatch)
        continue;
    }
    bool    foundMatch = true;
    for (std::vector< std::string >::const_iterator
             j = excludePatterns.begin(); j != excludePatterns.end(); j++)
    {
      if (i->find(*j) != std::string::npos)
      {
        foundMatch = false;
        break;
      }
    }
    if (foundMatch)
      args.push_back(*i);
  }
}

int main(int argc, char **argv)
{
  const char  *outFileName = nullptr;
  std::FILE *outFile = nullptr;
#ifdef HAVE_SDL2
  NIF_View  *renderer = nullptr;
#endif
  bool    consoleFlag = true;
  bool    extractMatList = false;
  // -1: view, 0: default, 1: dump_db, 2: JSON, 3: dump_list
  signed char dumpMode = 0;
  try
  {
    std::vector< std::string >  args;
    std::vector< std::string >  includePatterns;
    std::vector< std::string >  excludePatterns;
    const char  *cdbFileName = nullptr;
    const char  *listFileName = nullptr;
#ifdef HAVE_SDL2
    int     displayWidth = 0;
    int     displayHeight = 0;
#endif
    bool    noOptionsFlag = false;
    for (int i = 1; i < argc; i++)
    {
      if (noOptionsFlag || argv[i][0] != '-' || std::strcmp(argv[i], "--") == 0)
      {
        if (!noOptionsFlag)
        {
          noOptionsFlag = true;
          if (std::strcmp(argv[i], "--") == 0)
            continue;
        }
        std::string s(argv[i]);
        if (args.size() > 0)
        {
          for (size_t j = 0; j < s.length(); j++)
            s[j] = convertNameCharacter((unsigned char) s[j]);
        }
        if (args.size() < 1 || !extractMatList)
        {
          args.push_back(s);
          continue;
        }
        if (!s.starts_with("-x:"))
          includePatterns.push_back(s);
        else if (s.length() > 3)
          excludePatterns.emplace_back(s, 3);
        continue;
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
        dumpMode = -1;
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
      else if (std::strcmp(argv[i], "-all") == 0)
      {
        extractMatList = true;
      }
      else if (std::strcmp(argv[i], "-dump_list") == 0)
      {
        dumpMode = 3;
      }
      else if (std::strcmp(argv[i], "-dump_db") == 0)
      {
        dumpMode = 1;
      }
      else if (std::strcmp(argv[i], "-json") == 0)
      {
        dumpMode = 2;
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
          c = convertNameCharacter(listFile[i]);
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
    if (dumpMode < 0)
      fileNameFilter = ".cdb\t.dds\t.mesh\t.nif";
#endif
    BA2File ba2File(args[0].c_str(), fileNameFilter);
    std::vector< std::vector< unsigned char > > cdbBufs;
    if (!(dumpMode < 0 && !extractMatList))
      loadCDBFiles(cdbBufs, ba2File, cdbFileName);
    if (extractMatList)
      loadMaterialPaths(args, cdbBufs, includePatterns, excludePatterns);
    if (dumpMode > 0)
    {
      consoleFlag = false;
      SDLDisplay::enableConsole();
      std::string stringBuf;
      std::string errMsgBuf;
      if (dumpMode == 1)
      {
        for (size_t i = 0; i < cdbBufs.size(); i++)
        {
          BSReflDump  cdbFile(cdbBufs[i].data(), cdbBufs[i].size());
          try
          {
            cdbFile.readAllChunks(stringBuf, 0, true);
          }
          catch (std::exception& e)
          {
            errMsgBuf = e.what();
            break;
          }
        }
      }
      else if (dumpMode == 2 && cdbBufs.size() > 0)
      {
        BSMaterialsCDB  cdbFile(cdbBufs[0].data(), cdbBufs[0].size());
        std::string tmpBuf;
        for (size_t i = 1; i < args.size(); i++)
        {
          if (args.size() > 2)
            std::printf("%s\n", args[i].c_str());
          cdbFile.getJSONMaterial(tmpBuf, args[i]);
          if (tmpBuf.empty())
          {
            std::fprintf(stderr, "Warning: %s: material not found\n",
                         args[i].c_str());
          }
          else
          {
            tmpBuf += '\n';
            if (args.size() <= 2)
              stringBuf += tmpBuf;
            else
              writeFileWithPath(args[i].c_str(), tmpBuf);
          }
        }
      }
      else if (dumpMode == 3)
      {
        for (size_t i = 1; i < args.size(); i++)
          printToString(stringBuf, "%s\n", args[i].c_str());
      }
      if (!(dumpMode == 2 && args.size() > 2))
      {
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
      }
      if (!errMsgBuf.empty())
      {
        std::fprintf(stderr, "mat_info: %s\n", errMsgBuf.c_str());
        return 1;
      }
      return 0;
    }
#ifdef HAVE_SDL2
    if (dumpMode < 0 && args.size() > 1)
    {
      args.erase(args.begin(), args.begin() + 1);
      std::sort(args.begin(), args.end());
      renderer = new NIF_View(ba2File, nullptr, cdbFileName, true);
      SDLDisplay  display(displayWidth, displayHeight, "mat_info", 4U, 56);
      display.setDefaultTextColor(0x00, 0xC0);
      renderer->viewModels(display, args, 0);
      delete renderer;
      return 0;
    }
#endif
    CE2MaterialDB materials;
    for (size_t i = 0; i < cdbBufs.size(); i++)
      materials.loadCDBFile(cdbBufs[i].data(), cdbBufs[i].size());
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

