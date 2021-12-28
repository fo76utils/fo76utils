
#include "common.hpp"
#include "esmfile.hpp"

const unsigned char * ESMFile::uncompressRecord(ESMRecord& r)
{
  if (r.formID == zlibBufRecord)
    return &(zlibBuf[zlibBufIndex].front());
  unsigned int  compressedSize;
  {
    FileBuffer  buf(r.fileData + 4, 4);
    compressedSize = buf.readUInt32();
  }
  if (compressedSize < 10)
    throw errorMessage("invalid compressed record size");
  FileBuffer  buf(r.fileData, compressedSize + 24);
  compressedSize = compressedSize - 4;
  buf.setPosition(24);
  unsigned int  uncompressedSize = buf.readUInt32();
  zlibBufRecord = 0xFFFFFFFFU;
  zlibBuf[zlibBufIndex].resize(size_t(24) + uncompressedSize);
  unsigned char *p = &(zlibBuf[zlibBufIndex].front());
  std::memcpy(p, buf.getDataPtr(), 24);
  p[4] = (unsigned char) (uncompressedSize & 0xFF);
  p[5] = (unsigned char) ((uncompressedSize >> 8) & 0xFF);
  p[6] = (unsigned char) ((uncompressedSize >> 16) & 0xFF);
  p[7] = (unsigned char) ((uncompressedSize >> 24) & 0xFF);
  p[10] = p[10] & 0xFB;         // clear compressed record flag
  unsigned int  recordSize =
      (unsigned int) zlibDecompressor.decompressData(
                         p + 24, uncompressedSize,
                         buf.getDataPtr() + 28, compressedSize);
  if (recordSize != uncompressedSize)
    throw errorMessage("invalid compressed record size");
  if ((zlibBufIndex + 1) < zlibBuf.size())
  {
    zlibBufIndex++;
    r.flags = r.flags & ~0x00040000U;
    r.fileData = p;
  }
  zlibBufRecord = r.formID;
  return p;
}

unsigned int ESMFile::loadRecords(size_t& groupCnt, FileBuffer& buf,
                                  size_t endPos, unsigned int parent)
{
  ESMRecord *prv = (ESMRecord *) 0;
  unsigned int  r = 0U;
  while (buf.getPosition() < endPos)
  {
    if ((buf.getPosition() + 24) > endPos)
      throw errorMessage("end of group in ESM input file");
    const unsigned char *p = buf.getDataPtr() + buf.getPosition();
    unsigned int  recordType = buf.readUInt32Fast();
    unsigned int  recordSize = buf.readUInt32Fast();
    unsigned int  flags = buf.readUInt32Fast();
    unsigned int  formID = buf.readUInt32Fast();
    unsigned int  n = formID;
    (void) buf.readUInt32Fast();        // timestamp, version control info
    (void) buf.readUInt32Fast();        // record version, unknown
    if (FileBuffer::checkType(recordType, "GRUP"))
    {
      n = (unsigned int) groupCnt | 0x80000000U;
      groupCnt++;
    }
    ESMRecord *esmRecord = findRecord(n);
    if (!esmRecord)
      throw errorMessage("internal error: invalid form ID");
    if (FileBuffer::checkType(recordType, "GRUP"))
    {
      esmRecord->children = loadRecords(groupCnt, buf,
                                        buf.getPosition() + recordSize - 24, n);
    }
    else if (recordSize > 0)
    {
      if (recordSize < 6 || (buf.getPosition() + recordSize) > endPos)
        throw errorMessage("invalid ESM record size");
      buf.setPosition(buf.getPosition() + recordSize);
    }
    if (!esmRecord->fileData)
    {
      esmRecord->parent = parent;
      if (prv)
        prv->next = n;
      prv = esmRecord;
      if (!r)
        r = n;
    }
    if (n != 0U || !esmRecord->fileData)
    {
      esmRecord->type = recordType;
      esmRecord->flags = flags;
      esmRecord->formID = formID;
      esmRecord->fileData = p;
    }
  }
  return r;
}

ESMFile::ESMFile(const char *fileNames, bool enableZLibCache)
  : recordCnt(0),
    groupOffs(0),
    zlibBufIndex(0),
    zlibBufRecord(0xFFFFFFFFU)
{
  try
  {
    std::vector< std::string >  tmpFileNames;
    std::string fileName;
    for (size_t i = 0; fileNames[i] != '\0'; i++)
    {
      if (fileName.length() > 4)
      {
        if (fileNames[i] == ',' && fileNames[i - 4] == '.' &&
            (fileNames[i - 3] == 'E' || fileNames[i - 3] == 'e') &&
            (fileNames[i - 2] == 'S' || fileNames[i - 2] == 's') &&
            (fileNames[i - 1] == 'M' || fileNames[i - 1] == 'm'))
        {
          tmpFileNames.push_back(fileName);
          fileName.clear();
          continue;
        }
      }
      fileName += fileNames[i];
    }
    if (!fileName.empty())
      tmpFileNames.push_back(fileName);
    if (tmpFileNames.size() < 1)
      throw errorMessage("ESMFile: no input files");
    esmFiles.resize(tmpFileNames.size(), (FileBuffer *) 0);
    for (size_t i = 0; i < tmpFileNames.size(); i++)
    {
      const char  *fName = tmpFileNames[i].c_str();
      esmFiles[i] = new FileBuffer(fName);
      if (!FileBuffer::checkType(esmFiles[i]->readUInt32(), "TES4"))
        throw errorMessage("input file %s is not in ESM format", fName);
      (void) esmFiles[i]->readUInt64();
      if (esmFiles[i]->readUInt32() != 0U)
        throw errorMessage("%s: invalid ESM file header", fName);
    }

    size_t  compressedCnt = 0;
    size_t  groupCnt = 0;
    unsigned int  maxFormID = 0;
    for (size_t i = 0; i < esmFiles.size(); i++)
    {
      FileBuffer& buf = *(esmFiles[i]);
      buf.setPosition(0);
      while (buf.getPosition() < buf.size())
      {
        if ((buf.getPosition() + 24) > buf.size())
          throw errorMessage("end of input file %s", tmpFileNames[i].c_str());
        unsigned int  recordType = buf.readUInt32Fast();
        unsigned int  recordSize = buf.readUInt32Fast();
        unsigned int  flags = buf.readUInt32Fast();
        unsigned int  formID = buf.readUInt32Fast();
        (void) buf.readUInt32Fast();    // timestamp, version control info
        (void) buf.readUInt32Fast();    // record version, unknown
        if (FileBuffer::checkType(recordType, "GRUP"))
        {
          if (recordSize < 24 ||
              (buf.getPosition() + (recordSize - 24)) > buf.size())
          {
            throw errorMessage("%s: invalid group size",
                               tmpFileNames[i].c_str());
          }
          groupCnt++;
        }
        else
        {
          if (formID > 0x0FFFFFFFU)
            throw errorMessage("%s: invalid form ID", tmpFileNames[i].c_str());
          maxFormID = (formID > maxFormID ? formID : maxFormID);
          if ((recordSize < 10 && (flags & 0x00040000) != 0) ||
              (buf.getPosition() + recordSize) > buf.size())
          {
            throw errorMessage("%s: invalid record size",
                               tmpFileNames[i].c_str());
          }
          recordCnt++;
          if (flags & 0x00040000)
            compressedCnt++;
          buf.setPosition(buf.getPosition() + recordSize);
        }
      }
    }
    groupOffs = recordCnt;
    recordBuf.resize(recordCnt + groupCnt);
    formIDMap.resize(maxFormID + 1U, (unsigned int) recordBuf.size());
    if (compressedCnt)
    {
      if (!enableZLibCache)
        compressedCnt = 1;
      zlibBuf.resize(compressedCnt);
    }

    unsigned int  n = 0;
    for (size_t i = 0; i < esmFiles.size(); i++)
    {
      FileBuffer& buf = *(esmFiles[i]);
      buf.setPosition(0);
      while (buf.getPosition() < buf.size())
      {
        unsigned int  recordType = buf.readUInt32Fast();
        unsigned int  recordSize = buf.readUInt32Fast();
        (void) buf.readUInt32Fast();    // flags
        unsigned int  formID = buf.readUInt32Fast();
        (void) buf.readUInt32Fast();    // timestamp, version control info
        (void) buf.readUInt32Fast();    // record version, unknown
        if (!FileBuffer::checkType(recordType, "GRUP") && formID <= maxFormID)
        {
          formIDMap[formID] = n;
          n++;
          buf.setPosition(buf.getPosition() + recordSize);
        }
      }
    }

    groupCnt = 0;
    for (size_t i = 0; i < esmFiles.size(); i++)
    {
      FileBuffer& buf = *(esmFiles[i]);
      buf.setPosition(0);
      n = loadRecords(groupCnt, buf, buf.size(), 0U);
      ESMRecord *r = findRecord(0U);
      if (n != 0U && r && r->next != n)
      {
        while (r->next)
          r = findRecord(r->next);
        r->next = n;
      }
    }
  }
  catch (...)
  {
    for (size_t i = 0; i < esmFiles.size(); i++)
    {
      if (esmFiles[i])
        delete esmFiles[i];
    }
    throw;
  }
}

ESMFile::~ESMFile()
{
  for (size_t i = 0; i < esmFiles.size(); i++)
  {
    if (esmFiles[i])
      delete esmFiles[i];
  }
}

const ESMFile::ESMRecord& ESMFile::getRecord(unsigned int formID) const
{
  const ESMRecord *r = findRecord(formID);
  if (!r)
    throw errorMessage("invalid form ID");
  return *r;
}

ESMFile::ESMField::ESMField(ESMFile& f, const ESMRecord& r)
  : FileBuffer(),
    type(0),
    dataRemaining(0)
{
  if (r.type == 0x50555247)             // "GRUP"
    return;
  if (r.flags & 0x00040000)             // compressed record
    fileBuf = f.uncompressRecord(*(f.findRecord(r.formID)));
  else
    fileBuf = r.fileData;
  fileBufSize = 24;
  filePos = 4;
  dataRemaining = readUInt32Fast();
}

ESMFile::ESMField::ESMField(ESMFile& f, unsigned int formID)
  : FileBuffer(),
    type(0),
    dataRemaining(0)
{
  const ESMRecord *r = f.findRecord(formID);
  if (!r)
    throw errorMessage("invalid form ID");
  if (r->type == 0x50555247)            // "GRUP"
    return;
  if (r->flags & 0x00040000)            // compressed record
    fileBuf = f.uncompressRecord(*(f.findRecord(r->formID)));
  else
    fileBuf = r->fileData;
  fileBufSize = 24;
  filePos = 4;
  dataRemaining = readUInt32Fast();
}

bool ESMFile::ESMField::next()
{
  fileBuf = fileBuf + fileBufSize;
  fileBufSize = dataRemaining;
  filePos = 0;
  if (!dataRemaining)
    return false;
  if (dataRemaining < 6)
    throw errorMessage("end of record data");
  type = readUInt32Fast();
  size_t  n = readUInt16Fast();
  if (type == 0x58585858 && n == 4)     // "XXXX"
  {
    if (dataRemaining < 16)
      throw errorMessage("end of record data");
    n = readUInt32Fast();
    type = readUInt32Fast();
    (void) readUInt16Fast();
  }
  if ((filePos + n) > dataRemaining)
    throw errorMessage("end of record data");
  dataRemaining = dataRemaining - (unsigned int) (filePos + n);
  if (!(n + dataRemaining))
    fileBuf = (unsigned char *) 0;
  else
    fileBuf = fileBuf + filePos;
  fileBufSize = n;
  filePos = 0;
  return true;
}

unsigned short ESMFile::getRecordTimestamp(unsigned int formID) const
{
  const ESMRecord *r = findRecord(formID);
  if (!r)
    throw errorMessage("invalid form ID");
  const unsigned char *p = r->fileData;
  return (((unsigned short) p[17] << 8) | p[16]);
}

unsigned short ESMFile::getRecordUserID(unsigned int formID) const
{
  const ESMRecord *r = findRecord(formID);
  if (!r)
    throw errorMessage("invalid form ID");
  const unsigned char *p = r->fileData;
  return (((unsigned short) p[19] << 8) | p[18]);
}

