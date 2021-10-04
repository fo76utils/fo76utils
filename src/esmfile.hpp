
#ifndef ESMFILE_HPP_INCLUDED
#define ESMFILE_HPP_INCLUDED

#include "common.hpp"
#include "zlib.hpp"
#include "filebuf.hpp"

class ESMFile
{
 public:
  struct ESMRecord
  {
    unsigned int  type;         // 0x50555247 ("GRUP") if group
    unsigned int  flags;        // group label if group
    unsigned int  formID;       // group type if group
    unsigned int  parent;       // form ID of parent group or record
    unsigned int  children;     // form ID of first child record or group
    unsigned int  next;         // form ID of next record in this group
    const unsigned char *fileData;      // pointer to record in file buffer
    ESMRecord()
      : type(0xFFFFFFFFU), flags(0), formID(0U), parent(0),
        children(0), next(0), fileData((unsigned char *) 0)
    {
    }
    inline bool operator==(const char *s) const
    {
      return FileBuffer::checkType(type, s);
    }
  };
  struct ESMField
  {
    unsigned int  type;
    unsigned int  size;
    const unsigned char *data;
    unsigned int  dataRemaining;
    ESMField()
      : type(0), size(0), data((unsigned char *) 0), dataRemaining(0)
    {
    }
    bool next();
    inline bool operator==(const char *s) const
    {
      return FileBuffer::checkType(type, s);
    }
  };
 protected:
  unsigned int  recordCnt;
  unsigned int  groupOffs;
  unsigned int  zlibBufIndex;
  unsigned int  zlibBufRecord;
  std::vector< ESMRecord >    recordBuf;
  std::vector< unsigned int > formIDMap;
  std::vector< std::vector< unsigned char > > zlibBuf;
  ZLibDecompressor  zlibDecompressor;
  std::vector< FileBuffer * > esmFiles;
  inline const ESMRecord *findRecord(unsigned int n) const
  {
    size_t  offs = recordBuf.size();
    if (n & 0x80000000U)
      offs = (n & 0x7FFFFFFFU) + groupOffs;
    else if (n < formIDMap.size())
      offs = formIDMap[n];
    if (offs >= recordBuf.size())
      return (ESMRecord *) 0;
    return (&(recordBuf.front()) + offs);
  }
  inline ESMRecord *findRecord(unsigned int n)
  {
    return const_cast< ESMRecord * >(((const ESMFile *) this)->findRecord(n));
  }
  const unsigned char *uncompressRecord(ESMRecord& r);
  unsigned int loadRecords(size_t& groupCnt, FileBuffer& buf,
                           size_t endPos, unsigned int parent);
 public:
  // fileNames can be a single ESM file, or a comma separated list
  ESMFile(const char *fileNames, bool enableZLibCache = false);
  virtual ~ESMFile();
  inline const ESMRecord& operator[](size_t n) const
  {
    return *(findRecord(n));
  }
  const ESMRecord& getRecord(unsigned int formID) const;
  // returns NULL if the record does not exist
  inline const ESMRecord *getRecordPtr(unsigned int formID) const
  {
    return findRecord(formID);
  }
  bool getFirstField(ESMField& f, const ESMRecord& r);
  bool getFirstField(ESMField& f, unsigned int formID);
  // encoding is (year - 2000) * 512 + (month * 32) + day for FO4 and newer
  unsigned short getRecordTimestamp(unsigned int formID) const;
  unsigned short getRecordUserID(unsigned int formID) const;
};

#endif

