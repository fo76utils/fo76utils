
#ifndef ESMFILE_HPP_INCLUDED
#define ESMFILE_HPP_INCLUDED

#include "common.hpp"
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
  class ESMField : public FileBuffer
  {
   public:
    unsigned int  type;
    unsigned int  dataRemaining;
    ESMField(ESMFile& f, const ESMRecord& r);
    ESMField(ESMFile& f, unsigned int formID);
    // constructor that accepts a const ESMFile,
    // but does not support compressed records
    ESMField(const ESMRecord& r, const ESMFile& f);
    bool next();
    inline bool operator==(const char *s) const
    {
      return FileBuffer::checkType(type, s);
    }
  };
  struct ESMVCInfo
  {
    unsigned int  year;
    unsigned int  month;
    unsigned int  day;
    unsigned int  userID1;
    unsigned int  userID2;
    unsigned int  formVersion;
    unsigned int  vcInfo2;
  };
 protected:
  unsigned int  recordCnt;
  unsigned char recordHdrSize;  // 20 for Oblivion, 24 for Fallout 3 and newer
  // 0x00: Oblivion
  // 0x02: Fallout 3/New Vegas
  // 0x0F: Fallout 3/New Vegas DLC
  // 0x28: Skyrim (2011)
  // 0x2B: Skyrim DLC
  // 0x2C: Skyrim Special Edition (2016)
  // 0x83: Fallout 4
  // 0xC0: Fallout 76 (Wild Appalachia?)
  // 0xC1: Fallout 76 (Nuclear Winter?)
  // 0xC2: Fallout 76 (Wastelanders)
  // 0xC3: Fallout 76 (Steel Dawn and newer)
  // 0xC5: Fallout 76 (The Pitt and newer?)
  unsigned char esmVersion;
  unsigned short  esmFlags;     // 0x80: localized strings
  unsigned int  zlibBufIndex;
  unsigned int  zlibBufRecord;
  std::vector< ESMRecord >    recordBuf;
  std::vector< unsigned int > formIDMap;
  std::vector< std::vector< unsigned char > > zlibBuf;
  std::vector< FileBuffer * > esmFiles;
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
  inline const ESMRecord *findRecord(unsigned int n) const
  {
    if (BRANCH_UNLIKELY(n >= 0x80000000U))
    {
      n = (n & 0x7FFFFFFFU) + recordCnt;
    }
    else
    {
      std::vector< unsigned int >::const_iterator i = formIDMap.begin() + n;
      if (BRANCH_UNLIKELY(i >= formIDMap.end()))
        return (ESMRecord *) 0;
      n = *i;
    }
    std::vector< ESMRecord >::const_iterator  i = recordBuf.begin() + n;
    if (BRANCH_UNLIKELY(i >= recordBuf.end()))
      return (ESMRecord *) 0;
    return &(*i);
  }
  inline ESMRecord *findRecord(unsigned int n)
  {
    return const_cast< ESMRecord * >(((const ESMFile *) this)->findRecord(n));
  }
  inline const ESMRecord *getRecordPtr(unsigned int formID) const
  {
    return findRecord(formID);
  }
  // encoding is (year - 2000) * 512 + (month * 32) + day for FO4 and newer
  unsigned short getRecordTimestamp(unsigned int formID) const;
  unsigned short getRecordUserID(unsigned int formID) const;
  inline unsigned short getRecordFormVersion(const ESMRecord& r) const
  {
    if (!esmVersion)
      return 0;
    return FileBuffer::readUInt16Fast(r.fileData + 20);
  }
  inline unsigned int getESMVersion() const
  {
    return esmVersion;
  }
  inline unsigned int getESMFlags() const
  {
    return esmFlags;
  }
  inline unsigned int getRecordHeaderSize() const
  {
    return recordHdrSize;
  }
  inline bool getIsLocalized() const
  {
    return bool(esmFlags & 0x80);
  }
  void getVersionControlInfo(ESMVCInfo& f, const ESMRecord& r) const;
};

#endif

