
#ifndef ESMDBASE_HPP_INCLUDED
#define ESMDBASE_HPP_INCLUDED

#include "common.hpp"
#include "esmfile.hpp"
#include "stringdb.hpp"

class ESMDump : public ESMFile
{
 protected:
  struct Field
  {
    unsigned int  type;
    std::string   data;
  };
  StringDB  strings;
  std::map< unsigned long long, int >   recordStats;
  std::FILE *outputFile;
  std::set< unsigned int >  fieldsExcluded;
  std::set< unsigned int >  recordsIncluded;
  std::set< unsigned int >  recordsExcluded;
  unsigned int  flagsIncluded;
  unsigned int  flagsExcluded;
  bool    tsvFormat;
  bool    statsOnly;
  bool    haveStrings;
  bool    verboseMode;
  std::map< unsigned int, std::string > edidDB;
  // custom field definitions, key = (record << 32) | field
  // value format is name (optional) + "\t" + data types:
  //   <: remaining data is an array, and can be repeated any number of times
  //   b: boolean
  //   c: 8-bit unsigned integer (hexadecimal)
  //   h: 16-bit unsigned integer (hexadecimal)
  //   s: 16-bit signed integer
  //   x: 32-bit unsigned integer (hexadecimal)
  //   u: 32-bit unsigned integer
  //   i: 32-bit signed integer
  //   d: form ID
  //   f: 32-bit float
  //   l: localized string
  //   z: string
  //   n: file name
  //   .: ignore byte
  //   *: ignore any remaining data
  std::map< unsigned long long, std::string > fieldDefDB;
  void updateStats(unsigned int recordType, unsigned int fieldType);
  void printID(unsigned int id);
  static void printInteger(std::string& s, long long n);
  static void printHexValue(std::string& s, unsigned int n, unsigned int w);
  static void printBoolean(std::string& s, FileBuffer& buf);
  void printFormID(std::string& s, FileBuffer& buf);
  static void printFloat(std::string& s, FileBuffer& buf);
  void printLString(std::string& s, FileBuffer& buf);
  static void printZString(std::string& s, FileBuffer& buf);
  static void printFileName(std::string& s, FileBuffer& buf);
  void convertField(std::string& s, ESMField& f, const std::string& fldDef);
  void convertField(std::string& s, const ESMRecord& r, ESMField& f);
 public:
  ESMDump(const char *fileName, std::FILE *outFile = 0);
  virtual ~ESMDump();
  void setRecordFlagsMask(unsigned int includeMask, unsigned int excludeMask);
  void includeRecordType(const char *s);
  void excludeRecordType(const char *s);
  void excludeFieldType(const char *s);
  void setTSVFormat(bool isEnabled);
  void loadStrings(const char *fileName, const char *stringsPrefix);
  void dumpRecord(unsigned int formID = 0U,
                  const ESMRecord *parentGroup = (ESMRecord *) 0);
  void dumpVersionInfo(unsigned int formID = 0U,
                       const ESMRecord *parentGroup = (ESMRecord *) 0);
  void printStats();
  void setStatsOnlyMode(bool isEnabled)
  {
    statsOnly = isEnabled;
  }
  void setVerboseMode(bool isEnabled)
  {
    verboseMode = isEnabled;
  }
  void findEDIDs(unsigned int formID = 0U);
  void loadFieldDefFile(const char *fileName);
  void loadFieldDefFile(FileBuffer& inFile);
};

#endif

