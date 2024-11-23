
#include "common.hpp"
#include "zlib.hpp"
#include "esmfile.hpp"

inline ESMFile::ESMRecord & ESMFile::insertFormID(unsigned int formID)
{
  const std::uint32_t *p = pluginMap + (((formID >> 24) & 0xFFU) * 3U);
  if (formID < p[0] || formID > p[1])
    errorMessage("internal error: invalid form ID");
  std::uint32_t&  n = formIDMap[(formID + p[2]) & 0xFFFFFFFFU];
  if (n & 0x80000000U) [[likely]]
  {
    n = std::uint32_t(recordBufSize);
    return recordBuf[recordBufSize++];
  }
  if (n >= recordBufSize)
    errorMessage("internal error: invalid record index");
  return recordBuf[n];
}

const unsigned char * ESMFile::uncompressRecord(ESMRecord& r)
{
  if (r.formID == zlibBufRecord)
    return zlibBuf.back().data();
  unsigned int  compressedSize;
  {
    FileBuffer  buf(r.fileData + 4, 4);
    compressedSize = buf.readUInt32();
  }
  if (compressedSize < 10)
    errorMessage("invalid compressed record size");
  unsigned int  offs = recordHdrSize;
  FileBuffer  buf(r.fileData, compressedSize + offs);
  compressedSize = compressedSize - 4;
  buf.setPosition(offs);
  unsigned int  uncompressedSize = buf.readUInt32();
  zlibBufRecord = 0xFFFFFFFFU;
  zlibBuf.back().resize(size_t(offs) + uncompressedSize);
  unsigned char *p = zlibBuf.back().data();
  std::memcpy(p, buf.data(), offs);
  p[4] = (unsigned char) (uncompressedSize & 0xFF);
  p[5] = (unsigned char) ((uncompressedSize >> 8) & 0xFF);
  p[6] = (unsigned char) ((uncompressedSize >> 16) & 0xFF);
  p[7] = (unsigned char) ((uncompressedSize >> 24) & 0xFF);
  p[10] = p[10] & 0xFB;         // clear compressed record flag
  unsigned int  recordSize =
      (unsigned int) ZLibDecompressor::decompressData(
                         p + offs, uncompressedSize,
                         buf.data() + (offs + 4), compressedSize);
  if (recordSize != uncompressedSize)
    errorMessage("invalid compressed record size");
  if (zlibBuf.size() < zlibBuf.capacity())
  {
    zlibBuf.emplace_back();
    r.flags = r.flags & ~0x00040000U;
    r.fileData = p;
  }
  zlibBufRecord = r.formID;
  return p;
}

unsigned int ESMFile::loadRecords(
    size_t& groupCnt, FileBuffer& buf, size_t endPos, unsigned int parent)
{
  ESMRecord *prv = nullptr;
  unsigned int  r = 0U;
  while (buf.getPosition() < endPos)
  {
    if ((buf.getPosition() + recordHdrSize) > endPos)
      errorMessage("end of group in ESM input file");
    const unsigned char *p = buf.getReadPtr();
    unsigned int  recordType = buf.readUInt32Fast();
    unsigned int  recordSize = buf.readUInt32Fast();
    unsigned int  flags = buf.readUInt32Fast();
    unsigned int  formID = buf.readUInt32Fast();
    unsigned int  n = formID;
    // skip version control info
    buf.setPosition(buf.getPosition() + (recordHdrSize - 16));
    if (FileBuffer::checkType(recordType, "GRUP"))
    {
      n = (unsigned int) groupCnt | 0x80000000U;
      groupCnt++;
    }
    ESMRecord&  esmRecord = insertFormID(n);
    if (!esmRecord.fileData) [[likely]]
    {
      esmRecord.parent = parent;
      if (prv)
        prv->next = n;
      prv = &esmRecord;
      if (!r)
        r = n;
    }
    if (n != 0U || !esmRecord.fileData) [[likely]]
    {
      esmRecord.type = recordType;
      esmRecord.flags = flags;
      esmRecord.formID = formID;
      esmRecord.fileData = p;
    }
    if (FileBuffer::checkType(recordType, "GRUP"))
    {
      esmRecord.children =
          loadRecords(groupCnt, buf,
                      buf.getPosition() + recordSize - recordHdrSize, n);
    }
    else if (recordSize > 0)
    {
      if (recordSize < 6 || (buf.getPosition() + recordSize) > endPos)
        errorMessage("invalid ESM record size");
      buf.setPosition(buf.getPosition() + recordSize);
    }
  }
  return r;
}

ESMFile::ESMFile(const char *fileNames, bool enableZLibCache)
  : recordHdrSize(0),
    esmVersion(0),
    esmFlags(0),
    zlibBufRecord(0xFFFFFFFFU),
    pluginMap(nullptr),
    formIDMap(nullptr),
    recordBuf(nullptr),
    recordBufSize(0)
{
  try
  {
    pluginMap = new std::uint32_t[0x0300];
    for (size_t i = 0; i < 0x0300; i = i + 3)
    {
      pluginMap[i] = 0xFFFFFFFFU;       // minimum form ID
      pluginMap[i + 1] = 0U;            // maximum form ID
      pluginMap[i + 2] = 0U;            // formIDMap offset
    }
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
      errorMessage("ESMFile: no input files");
    esmFiles.resize(tmpFileNames.size(), nullptr);
    for (size_t i = 0; i < tmpFileNames.size(); i++)
    {
      const char  *fName = tmpFileNames[i].c_str();
      esmFiles[i] = new FileBuffer(fName);
      if (!FileBuffer::checkType(esmFiles[i]->readUInt32(), "TES4"))
        throw FO76UtilsError("input file %s is not in ESM format", fName);
      (void) esmFiles[i]->readUInt32();
      esmFlags = esmFiles[i]->readUInt32();
      if (esmFiles[i]->readUInt32() != 0U)
        throw FO76UtilsError("%s: invalid ESM file header", fName);
      (void) esmFiles[i]->readUInt32();
      unsigned int  tmp = esmFiles[i]->readUInt16();
      if (tmp < 0x1000)
      {
        recordHdrSize = 24;
        esmVersion = tmp;
      }
      else if (tmp == 0x4548)           // "HE"
      {
        recordHdrSize = 20;
        esmVersion = 0x00;
      }
      else
      {
        throw FO76UtilsError("%s: invalid ESM file header", fName);
      }
    }

    size_t  compressedCnt = 0;
    size_t  recordCnt = 0;
    size_t  groupCnt = 0;
    for (size_t i = 0; i < esmFiles.size(); i++)
    {
      FileBuffer& buf = *(esmFiles[i]);
      buf.setPosition(0);
      while (buf.getPosition() < buf.size())
      {
        if ((buf.getPosition() + recordHdrSize) > buf.size())
          throw FO76UtilsError("end of input file %s", tmpFileNames[i].c_str());
        unsigned int  recordType = buf.readUInt32Fast();
        unsigned int  recordSize = buf.readUInt32Fast();
        unsigned int  flags = buf.readUInt32Fast();
        unsigned int  formID = buf.readUInt32Fast();
        std::uint32_t n = std::uint32_t(formID);
        // skip version control info
        buf.setPosition(buf.getPosition() + (recordHdrSize - 16));
        if (FileBuffer::checkType(recordType, "GRUP"))
        {
          if (recordSize < recordHdrSize ||
              (buf.getPosition() + (recordSize - recordHdrSize)) > buf.size())
          {
            throw FO76UtilsError("%s: invalid group size",
                                 tmpFileNames[i].c_str());
          }
          n = std::uint32_t(0x80000000U | groupCnt);
          groupCnt++;
        }
        else
        {
          if (formID > 0x0FFFFFFFU && ((formID + 0x03000000U) & 0xFE000000U))
          {
            throw FO76UtilsError("%s: invalid form ID",
                                 tmpFileNames[i].c_str());
          }
          if ((recordSize < 10 && (flags & 0x00040000) != 0) ||
              (buf.getPosition() + recordSize) > buf.size())
          {
            throw FO76UtilsError("%s: invalid record size",
                                 tmpFileNames[i].c_str());
          }
          recordCnt++;
          if (flags & 0x00040000)
            compressedCnt++;
          buf.setPosition(buf.getPosition() + recordSize);
        }
        std::uint32_t *p = pluginMap + (((n >> 24) & 0xFFU) * 3U);
        p[0] = std::min(p[0], n);
        p[1] = std::max(p[1], n);
      }
    }
    size_t  formIDMapSize = 0;
    for (size_t i = 0; i < 0x0300; i = i + 3)
    {
      if (pluginMap[i] <= pluginMap[i + 1])
      {
        pluginMap[i + 2] = std::uint32_t(formIDMapSize - pluginMap[i]);
        formIDMapSize = formIDMapSize + size_t(pluginMap[i + 1]) + 1 - size_t(pluginMap[i]);
      }
    }
    formIDMap = new std::uint32_t[formIDMapSize];
    memsetUInt32(formIDMap, 0xFFFFFFFFU, formIDMapSize);
    recordBuf = new ESMRecord[recordCnt + groupCnt];
    if (compressedCnt)
    {
      if (!enableZLibCache)
        compressedCnt = 1;
      zlibBuf.reserve(compressedCnt);
      zlibBuf.emplace_back();
    }

    groupCnt = 0;
    for (size_t i = 0; i < esmFiles.size(); i++)
    {
      FileBuffer& buf = *(esmFiles[i]);
      buf.setPosition(0);
      unsigned int  n = loadRecords(groupCnt, buf, buf.size(), 0U);
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
    delete[] recordBuf;
    delete[] formIDMap;
    delete[] pluginMap;
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
  delete[] recordBuf;
  delete[] formIDMap;
  delete[] pluginMap;
}

const ESMFile::ESMRecord& ESMFile::getRecord(unsigned int formID) const
{
  const ESMRecord *r = findRecord(formID);
  if (!r)
    errorMessage("invalid form ID");
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
  fileBufSize = f.recordHdrSize;
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
    errorMessage("invalid form ID");
  if (r->type == 0x50555247)            // "GRUP"
    return;
  if (r->flags & 0x00040000)            // compressed record
    fileBuf = f.uncompressRecord(*(const_cast< ESMRecord * >(r)));
  else
    fileBuf = r->fileData;
  fileBufSize = f.recordHdrSize;
  filePos = 4;
  dataRemaining = readUInt32Fast();
}

ESMFile::ESMField::ESMField(const ESMRecord& r, const ESMFile& f)
  : FileBuffer(r.fileData, f.recordHdrSize, 8),
    type(0),
    dataRemaining(FileBuffer::readUInt32Fast(r.fileData + 4))
{
  if (r.type == 0x50555247 || (r.flags & 0x00040000)) [[unlikely]]
  {
    // "GRUP" or compressed record
    dataRemaining = 0;
  }
}

bool ESMFile::ESMField::next()
{
  fileBuf = fileBuf + fileBufSize;
  fileBufSize = dataRemaining;
  filePos = 0;
  if (!dataRemaining)
    return false;
  if (dataRemaining < 6)
    errorMessage("end of record data");
  type = readUInt32Fast();
  size_t  n = readUInt16Fast();
  if (type == 0x58585858) [[unlikely]]          // "XXXX"
  {
    if (n == 4) [[likely]]
    {
      if (dataRemaining < 16)
        errorMessage("end of record data");
      n = readUInt32Fast();
      type = readUInt32Fast();
      (void) readUInt16Fast();
    }
  }
  if ((filePos + n) > dataRemaining)
    errorMessage("end of record data");
  dataRemaining = dataRemaining - (unsigned int) (filePos + n);
  fileBuf = fileBuf + filePos;
  fileBufSize = n;
  filePos = 0;
  return true;
}

ESMFile::CDBRecord::CDBRecord(ESMField& f)
  : FileBuffer(f.data(), f.size(), 0),
    stringTableSize(0U),
    stringTable(nullptr)
{
  if (f.size() >= 16) [[likely]]
  {
    dataRemaining = (unsigned int) (f.size() - 16);
    type = FileBuffer::readUInt32Fast(f.data());
    fileBuf = f.data() + 8;
    fileBufSize = FileBuffer::readUInt32Fast(f.data() + 4);
    if (type == 0x48544542U && fileBufSize == 8)        // "BETH"
    {
      if (FileBuffer::readUInt32Fast(f.data() + 8) == 4U) [[likely]]
      {
        recordsRemaining = FileBuffer::readUInt32Fast(f.data() + 12);
        recordsRemaining -= (unsigned int) (recordsRemaining > 0U);
        return;
      }
    }
  }
  fileBufSize = 0;
  type = 0U;
  recordsRemaining = 0U;
  dataRemaining = 0U;
}

bool ESMFile::CDBRecord::next()
{
  fileBuf = fileBuf + fileBufSize;
  fileBufSize = dataRemaining;
  filePos = 0;
  if (!recordsRemaining)
    return false;
  recordsRemaining--;
  if (dataRemaining < 8U)
    errorMessage("unexpected end of CDB data");
  type = readUInt32Fast();
  size_t  n = readUInt32Fast();
  if (n > (dataRemaining - 8U))
    errorMessage("unexpected end of CDB data");
  dataRemaining = dataRemaining - (unsigned int) (n + 8);
  fileBuf = fileBuf + 8;
  fileBufSize = n;
  filePos = 0;
  if (type == 0x54525453U) [[unlikely]]         // "STRT"
  {
    for ( ; n > 0; n--)
    {
      if (!fileBuf[n - 1])
      {
        stringTableSize = (unsigned int) n;
        stringTable = reinterpret_cast< const char * >(fileBuf);
        break;
      }
    }
  }
  return true;
}

unsigned short ESMFile::getRecordTimestamp(unsigned int formID) const
{
  const ESMRecord *r = findRecord(formID);
  if (!r)
    errorMessage("invalid form ID");
  const unsigned char *p = r->fileData;
  return (((unsigned short) p[17] << 8) | p[16]);
}

unsigned short ESMFile::getRecordUserID(unsigned int formID) const
{
  const ESMRecord *r = findRecord(formID);
  if (!r)
    errorMessage("invalid form ID");
  const unsigned char *p = r->fileData;
  return (((unsigned short) p[19] << 8) | p[18]);
}

void ESMFile::getVersionControlInfo(ESMVCInfo& f, const ESMRecord& r) const
{
  unsigned int  tmp = ((unsigned int) r.fileData[17] << 8) | r.fileData[16];
  f.day = tmp & 0x1F;
  if (esmVersion < 0x80)
  {
    // Oblivion, Fallout 3, New Vegas, Skyrim
    f.month = (tmp >> 8) + 11;
    f.year = 2002U + (f.month / 12U);
    f.month = (f.month % 12U) + 1;
    if (f.year == 2003 && esmVersion != 0x00)
      f.year = 2011;
  }
  else
  {
    // Fallout 4, 76
    f.month = (tmp >> 5) & 0x0F;
    f.year = 2000U + (tmp >> 9);
  }
  f.userID1 = r.fileData[18];
  f.userID2 = r.fileData[19];
  if (f.userID2 != 0 && (esmVersion >= 0xC0 && tmp >= 0x26C0))  // June 2019
  {
    // Fallout 76 uses 16-bit user IDs since mid-2019
    f.userID1 = f.userID1 | (f.userID2 << 8);
    f.userID2 = 0;
  }
  if (!esmVersion)
  {
    f.formVersion = 0;
    f.vcInfo2 = 0;
  }
  else
  {
    f.formVersion = ((unsigned int) r.fileData[21] << 8) | r.fileData[20];
    f.vcInfo2 = ((unsigned int) r.fileData[23] << 8) | r.fileData[22];
  }
}

