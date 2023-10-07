
#ifndef CDB_FILE_HPP_INCLUDED
#define CDB_FILE_HPP_INCLUDED

#include "common.hpp"
#include "filebuf.hpp"

class CDBFile : public FileBuffer
{
 public:
  enum
  {
    String_None = 0,
    String_String = 1,
    String_List = 2,
    String_Map = 3,
    String_Ref = 4,
    String_Int8 = 7,
    String_UInt8 = 8,
    String_Int16 = 9,
    String_UInt16 = 10,
    String_Int32 = 11,
    String_UInt32 = 12,
    String_Int64 = 13,
    String_UInt64 = 14,
    String_Bool = 15,
    String_Float = 16,
    String_Double = 17,
    String_Unknown = 18,
    String_BSComponentDB2_DBFileIndex_ComponentInfo = 149,
    String_BSComponentDB2_DBFileIndex_ComponentTypeInfo = 150,
    String_BSComponentDB2_DBFileIndex_EdgeInfo = 151,
    String_BSComponentDB2_DBFileIndex_ObjectInfo = 152,
    String_BSResource_ID = 227
  };
  static const char *stringTable[1141];
  static int findString(const char *s);
 protected:
  std::vector< std::int16_t > stringMap;
  void readStringTable();
 public:
  CDBFile(const unsigned char *fileData, size_t fileSize)
    : FileBuffer(fileData, fileSize)
  {
    readStringTable();
  }
  CDBFile(const char *fileName)
    : FileBuffer(fileName)
  {
    readStringTable();
  }
  size_t findString(unsigned int strtOffs) const;
};

#endif

