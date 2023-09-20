
#include "common.hpp"
#include "material.hpp"
#include "filebuf.hpp"

#include <new>

#ifndef ENABLE_CDB_DEBUG
#  define ENABLE_CDB_DEBUG  0
#endif
#if ENABLE_CDB_DEBUG > 1
static void printHexData(const unsigned char *p, size_t n);
#endif

std::uint32_t CE2MaterialDB::calculateHash(
    std::uint64_t& h, const std::string& fileName)
{
  size_t  baseNamePos = fileName.rfind('/');
  size_t  baseNamePos2 = fileName.rfind('\\');
  size_t  extPos = fileName.rfind('.');
  if (baseNamePos == std::string::npos ||
      (baseNamePos2 != std::string::npos && baseNamePos2 > baseNamePos))
  {
    baseNamePos = baseNamePos2;
  }
  if (extPos == std::string::npos ||
      (baseNamePos != std::string::npos && extPos < baseNamePos))
  {
    extPos = fileName.length();
  }
  size_t  i = 0;
  unsigned int  crcValue = 0U;
  if (baseNamePos != std::string::npos)
  {
    for ( ; i < baseNamePos; i++)
    {
      unsigned char c = (unsigned char) fileName.c_str()[i];
      if (c >= 0x41 && c <= 0x5A)       // 'A' - 'Z'
        c = c | 0x20;                   // convert to lower case
      else if (c == 0x2F)               // '/'
        c = 0x5C;                       // convert to '\\'
      crcValue = (crcValue >> 8) ^ crc32Table[(crcValue ^ c) & 0xFFU];
    }
    i++;
  }
  h = std::uint64_t(crcValue) << 32;
  crcValue = 0U;
  for ( ; i < extPos; i++)
  {
    unsigned char c = (unsigned char) fileName.c_str()[i];
    if (c >= 0x41 && c <= 0x5A)         // 'A' - 'Z'
      c = c | 0x20;                     // convert to lower case
    else if (c == 0x2F)                 // '/'
      c = 0x5C;                         // convert to '\\'
    crcValue = (crcValue >> 8) ^ crc32Table[(crcValue ^ c) & 0xFFU];
  }
  h = h | crcValue;
  i++;
  if ((i + 3) <= fileName.length())
    return FileBuffer::readUInt32Fast(fileName.c_str() + i);
  if ((i + 2) <= fileName.length())
    return FileBuffer::readUInt16Fast(fileName.c_str() + i);
  if (i < fileName.length())
    return (unsigned char) fileName.c_str()[i];
  return 0U;
}

int CE2MaterialDB::findString(const char *s)
{
  size_t  n0 = 0;
  size_t  n2 = sizeof(stringTable) / sizeof(char *);
  while (n2 > (n0 + 1))
  {
    size_t  n1 = (n0 + n2) >> 1;
    if (std::strcmp(s, stringTable[n1]) < 0)
      n2 = n1;
    else
      n0 = n1;
  }
  if (n2 > n0 && std::strcmp(s, stringTable[n0]) == 0)
    return int(n0);
  return -1;
}

int CE2MaterialDB::findString(unsigned int strtOffs) const
{
  size_t  n0 = 0;
  size_t  n2 = stringMap.size();
  while (n2 > (n0 + 1))
  {
    size_t  n1 = (n0 + n2) >> 1;
    if (strtOffs < (stringMap[n1] & 0xFFFFFFFFU))
      n2 = n1;
    else
      n0 = n1;
  }
  if (n2 > n0 && strtOffs == (stringMap[n0] & 0xFFFFFFFFU))
    return int(stringMap[n0] >> 32);
  return -1;
}

inline const CE2MaterialObject * CE2MaterialDB::findObject(
    const std::vector< CE2MaterialObject * >& t, unsigned int objectID) const
{
  if (!objectID || objectID >= t.size())
    return (CE2MaterialObject *) 0;
  return t[objectID];
}

static const std::uint32_t
    defaultTextureRepl[CE2Material::TextureSet::maxTexturePaths] =
{
  // color, normal, opacity, rough
  0xFF000000U, 0xFFFF8080U, 0xFFFFFFFFU, 0xFF000000U,
  // metal, ao, height, emissive
  0xFF000000U, 0xFFFFFFFFU, 0xFF000000U, 0xFF000000U,
  // transmissive, curvature, mask
  0xFF000000U, 0xFF808080U, 0xFF000000U, 0xFF808080U,
  0xFF000000U, 0xFF000000U, 0xFF808080U, 0xFF808080U,
  0xFF808080U, 0xFF000000U, 0xFF000000U, 0xFFFFFFFFU,
  0xFF808080U
};

void CE2MaterialDB::initializeObject(
    CE2MaterialObject *o, const std::vector< CE2MaterialObject * >& objectTable)
{
  std::uint32_t baseObjID = ~(std::uint32_t(o->type)) >> 8;
  o->type = o->type & 0xFF;
  if (BRANCH_LIKELY(baseObjID))
  {
    // initialize from base object
    const CE2MaterialObject *t = findObject(objectTable, baseObjID);
    if (!t)
      errorMessage("invalid base object in material database");
    if (BRANCH_UNLIKELY(t->type < 0))
      initializeObject(const_cast< CE2MaterialObject * >(t), objectTable);
    if (o->type != t->type)
      errorMessage("invalid base object in material database");
    o->name = t->name;
    size_t  objectSize = sizeof(CE2MaterialObject);
    switch (o->type)
    {
      case 1:
        objectSize = sizeof(CE2Material);
        break;
      case 2:
        objectSize = sizeof(CE2Material::Blender);
        break;
      case 3:
        objectSize = sizeof(CE2Material::Layer);
        break;
      case 4:
        objectSize = sizeof(CE2Material::Material);
        break;
      case 5:
        objectSize = sizeof(CE2Material::TextureSet);
        break;
      case 6:
        objectSize = sizeof(CE2Material::UVStream);
        break;
      default:
        return;
    }
    unsigned char *dstPtr = reinterpret_cast< unsigned char * >(o);
    const unsigned char *srcPtr = reinterpret_cast< const unsigned char * >(t);
    dstPtr = dstPtr + sizeof(CE2MaterialObject);
    srcPtr = srcPtr + sizeof(CE2MaterialObject);
    std::memcpy(dstPtr, srcPtr, objectSize - sizeof(CE2MaterialObject));
    return;
  }
  // initialize with defaults
  const std::string *emptyString = stringBuffers[0].data();
  switch (o->type)
  {
    case 1:
      {
        CE2Material *p = static_cast< CE2Material * >(o);
        p->flags = 0U;
        p->alphaThreshold = 1.0f / 3.0f;
        p->opacityMode1 = 0;            // "Lerp"
        p->opacityMode2 = 0;
        p->layerMask = 0U;
        for (size_t i = 0; i < CE2Material::maxLayers; i++)
          p->layers[i] = (CE2Material::Layer *) 0;
        for (size_t i = 0; i < CE2Material::maxBlenders; i++)
          p->blenders[i] = (CE2Material::Blender *) 0;
        for (size_t i = 0; i < CE2Material::maxLODMaterials; i++)
          p->lodMaterials[i] = (CE2Material *) 0;
        p->emissiveColor = FloatVector4(1.0f);
      }
      break;
    case 2:
      {
        CE2Material::Blender  *p = static_cast< CE2Material::Blender * >(o);
        p->uvStream = (CE2Material::UVStream *) 0;
        p->texturePath = emptyString;
        p->textureReplacement = 0xFFFFFFFFU;
        for (size_t i = 0; i < CE2Material::Blender::maxFloatParams; i++)
          p->floatParams[i] = 0.5f;
        for (size_t i = 0; i < CE2Material::Blender::maxBoolParams; i++)
          p->boolParams[i] = false;
      }
      break;
    case 3:
      {
        CE2Material::Layer  *p = static_cast< CE2Material::Layer * >(o);
        p->material = (CE2Material::Material *) 0;
        p->uvStream = (CE2Material::UVStream *) 0;
      }
      break;
    case 4:
      {
        CE2Material::Material *p = static_cast< CE2Material::Material * >(o);
        p->color = FloatVector4(1.0f);
        p->colorMode = 0;               // "Multiply"
        p->textureSet = (CE2Material::TextureSet *) 0;
      }
      break;
    case 5:
      {
        CE2Material::TextureSet *p =
            static_cast< CE2Material::TextureSet * >(o);
        p->texturePathMask = 0U;
        p->floatParam = 0.5f;
        for (size_t i = 0; i < CE2Material::TextureSet::maxTexturePaths; i++)
          p->texturePaths[i] = emptyString;
        for (size_t i = 0; i < CE2Material::TextureSet::maxTexturePaths; i++)
          p->textureReplacements[i] = defaultTextureRepl[i];
      }
      break;
    case 6:
      {
        CE2Material::UVStream *p = static_cast< CE2Material::UVStream * >(o);
        p->scaleAndOffset = FloatVector4(1.0f, 1.0f, 0.0f, 0.0f);
        p->textureAddressMode = 0;      // "Wrap"
      }
      break;
  }
}

CE2MaterialObject * CE2MaterialDB::allocateObject(
    std::vector< CE2MaterialObject * >& objectTable,
    std::uint32_t objectID, std::uint32_t baseObjID)
{
  if (objectID >= objectTable.size() || baseObjID >= objectTable.size())
    errorMessage("CE2MaterialDB: internal error: object ID is out of range");
  CE2MaterialObject *o = objectTable[objectID];
  if (BRANCH_UNLIKELY(o))
  {
    throw FO76UtilsError("object 0x%08X is defined more than once "
                         "in material database", (unsigned int) objectID);
  }
  std::int32_t  type;
  if (BRANCH_UNLIKELY(!baseObjID))
  {
    if (objectID >= 2U && objectID <= 7U)
      type = std::int32_t((0x13652400U >> (objectID << 2)) & 15U);
    else
      errorMessage("invalid root object ID in material database");
  }
  else
  {
    if (objectTable[baseObjID])
      type = objectTable[baseObjID]->type & 0xFF;
    else
      errorMessage("invalid base object ID in material database");
  }
  std::uintptr_t  addr0 =
      reinterpret_cast< std::uintptr_t >(objectBuffers.back().data());
  std::uintptr_t  endAddr = addr0 + objectBuffers.back().capacity();
  std::uintptr_t  addr = addr0 + objectBuffers.back().size();
  addr = (addr + 15U) & ~(std::uintptr_t(15));
  std::uintptr_t  bytesRequired = std::uintptr_t(sizeof(CE2MaterialObject));
  switch (type)
  {
    case 1:
      bytesRequired = std::uintptr_t(sizeof(CE2Material));
      break;
    case 2:
      bytesRequired = std::uintptr_t(sizeof(CE2Material::Blender));
      break;
    case 3:
      bytesRequired = std::uintptr_t(sizeof(CE2Material::Layer));
      break;
    case 4:
      bytesRequired = std::uintptr_t(sizeof(CE2Material::Material));
      break;
    case 5:
      bytesRequired = std::uintptr_t(sizeof(CE2Material::TextureSet));
      break;
    case 6:
      bytesRequired = std::uintptr_t(sizeof(CE2Material::UVStream));
      break;
  }
  bytesRequired = (bytesRequired + 15U) & ~(std::uintptr_t(15));
  if (addr >= endAddr || (addr + bytesRequired) > endAddr)
  {
    objectBuffers.emplace_back();
    objectBuffers.back().reserve(65536);
    addr0 = reinterpret_cast< std::uintptr_t >(objectBuffers.back().data());
    endAddr = addr0 + 65536;
    addr = (addr0 + 15U) & ~(std::uintptr_t(15));
    if (addr >= endAddr || (addr + bytesRequired) > endAddr)
      throw std::bad_alloc();
  }
  objectBuffers.back().resize(size_t((addr + bytesRequired) - addr0));
  unsigned char *p = reinterpret_cast< unsigned char * >(addr);
  o = reinterpret_cast< CE2MaterialObject * >(p);
  objectTable[objectID] = o;
  o->type = type | std::int32_t(~baseObjID << 8);
  o->e = 0U;
  o->h = 0U;
  o->name = stringBuffers[0].data();
  o->parent = (CE2MaterialObject *) 0;
  if (BRANCH_UNLIKELY(!baseObjID))
    initializeObject(o, objectTable);
  return o;
}

const std::string * CE2MaterialDB::readStringParam(
    std::string& stringBuf, FileBuffer& buf, size_t len, int type)
{
  switch (type)
  {
    case 1:
      buf.readPath(stringBuf, len, "textures/", ".dds");
      break;
    default:
      buf.readString(stringBuf, len);
      break;
  }
  if (stringBuf.empty())
    return stringBuffers[0].data();
  std::uint64_t h = 0xFFFFFFFFU;
  const char  *p = stringBuf.c_str();
  const char  *endp = stringBuf.c_str() + stringBuf.length();
  // *endp is valid and always '\0'
  for ( ; (p + 7) <= endp; p = p + 8)
    hashFunctionUInt64(h, FileBuffer::readUInt64Fast(p));
  if (p < endp)
  {
    std::uint64_t tmp;
    if ((p + 3) <= endp)
    {
      tmp = FileBuffer::readUInt32Fast(p);
      if ((p + 5) <= endp)
        tmp = tmp | (std::uint64_t(FileBuffer::readUInt16Fast(p + 4)) << 32);
    }
    else
    {
      tmp = FileBuffer::readUInt16Fast(p);
    }
    hashFunctionUInt64(h, tmp);
  }
  size_t  n = size_t(h & stringHashMask);
  size_t  collisionCnt = 0;
  std::uint32_t m = storedStringParams[n];
  while (m)
  {
    const std::string *s =
        stringBuffers[m >> stringBufShift].data() + (m & stringBufMask);
    if (stringBuf == *s)
      return s;
    if (++collisionCnt >= 1024)
      errorMessage("CE2MaterialDB: internal error: stringHashMask is too low");
    n = (n + 1) & stringHashMask;
    m = storedStringParams[n];
  }
  if (stringBuffers.back().size() > stringBufMask)
  {
    stringBuffers.emplace_back();
    stringBuffers.back().reserve(stringBufMask + 1);
  }
  storedStringParams[n] =
      std::uint32_t(((stringBuffers.size() - 1) << stringBufShift)
                    | stringBuffers.back().size());
  stringBuffers.back().push_back(stringBuf);
  return &(stringBuffers.back().back());
}

size_t CE2MaterialDB::readTables(
    const unsigned char*& componentInfoPtr,
    std::vector< CE2MaterialObject * >& objectTable, FileBuffer& buf)
{
  componentInfoPtr = (unsigned char *) 0;
  size_t  componentCnt = 0;
  size_t  componentDataPos = 0;
  stringMap.clear();
  buf.setPosition(12);
  for (unsigned int chunkCnt = buf.readUInt32(); chunkCnt > 1U; chunkCnt--)
  {
    if ((buf.getPosition() + 8ULL) > buf.size())
      errorMessage("unexpected end of material database file");
    unsigned int  chunkType = buf.readUInt32Fast();
    unsigned int  chunkSize = buf.readUInt32Fast();
#if ENABLE_CDB_DEBUG
    char    chunkTypeStr[8];
    chunkTypeStr[0] = char(chunkType & 0x7FU);
    chunkTypeStr[1] = char((chunkType >> 8) & 0x7FU);
    chunkTypeStr[2] = char((chunkType >> 16) & 0x7FU);
    chunkTypeStr[3] = char((chunkType >> 24) & 0x7FU);
    chunkTypeStr[4] = '\0';
    std::printf("Chunk type = %s, offset = 0x%08X, size = %8u bytes\n",
                chunkTypeStr, (unsigned int) buf.getPosition() - 8U, chunkSize);
#endif
    if ((buf.getPosition() + std::uint64_t(chunkSize)) > buf.size())
      errorMessage("unexpected end of material database file");
    FileBuffer  buf2(buf.data() + buf.getPosition(), chunkSize, 0);
    buf.setPosition(buf.getPosition() + chunkSize);
    if (BRANCH_LIKELY(chunkType == 0x46464944U || chunkType == 0x544A424FU))
      continue;                                 // "DIFF" or "OBJT"
    if (chunkType == 0x54525453U)               // "STRT" (string table)
    {
      if (stringMap.size() > 0)
        errorMessage("duplicate string table in material database");
      // create string table
      while (buf2.getPosition() < buf2.size())
      {
        unsigned int  strtOffs = (unsigned int) buf2.getPosition();
        const char  *s =
            reinterpret_cast< const char * >(buf2.data()) + strtOffs;
        while (buf2.readUInt8Fast())
        {
          if (buf2.getPosition() >= buf2.size())
            errorMessage("string table is not terminated in material database");
        }
        int     stringNum = findString(s);
        if (stringNum >= 0)
        {
          stringMap.push_back((std::uint64_t(stringNum) << 32) | strtOffs);
        }
#if ENABLE_CDB_DEBUG
        else
        {
          std::printf("Warning: unrecognized string in material database: %s\n",
                      s);
        }
#endif
      }
      continue;
    }
    int     t = -1;
    if (chunkSize >= 4U)
      t = findString(buf2.readUInt32Fast());
#if ENABLE_CDB_DEBUG
    if (t >= 0)
      std::printf("    %s\n", stringTable[t]);
    if (chunkType == 0x4350414DU)               // "MAPC"
    {
      if (t == 107)                             // "BSResource::ID"
      {
        (void) buf2.readUInt32();
        for (unsigned int n = buf2.readUInt32(); n; n--)
        {
          // hash of the base name without extension
          std::printf("    0x%08X", buf2.readUInt32());
          // extension ("mat\0")
          std::printf(", 0x%08X", buf2.readUInt32());
          // hash of the directory name
          std::printf(", 0x%08X", buf2.readUInt32());
          // unknown hash
          std::printf(", 0x%08X", buf2.readUInt32());
          // unknown hash
          std::printf(", 0x%08X\n", buf2.readUInt32());
        }
      }
      continue;
    }
#endif
    if (chunkType != 0x5453494CU)               // "LIST"
      continue;
    if (t == 37)                // "BSComponentDB2::DBFileIndex::ObjectInfo"
    {
      unsigned int  n = buf2.readUInt32();
      if ((buf2.getPosition() + (std::uint64_t(n) * 21UL)) > buf2.size())
        errorMessage("unexpected end of LIST chunk in material database");
      if (objectTable.begin() != objectTable.end())
        errorMessage("duplicate ObjectInfo list in material database");
      const unsigned char *objectInfoPtr = buf2.data() + buf2.getPosition();
      unsigned int  objectCnt = n;
      std::uint32_t maxObjID = 0U;
      for (const unsigned char *p = objectInfoPtr; n; n--, p = p + 21)
        maxObjID = std::max(maxObjID, FileBuffer::readUInt32Fast(p + 12));
      if (maxObjID > 0x007FFFFFU)
        errorMessage("object ID is out of range in material database");
      objectTable.resize(size_t(maxObjID) + 1, (CE2MaterialObject *) 0);
      n = objectCnt;
      for (const unsigned char *p = objectInfoPtr; n; n--, p = p + 21)
      {
        std::uint32_t nameHash = FileBuffer::readUInt32Fast(p);
        std::uint32_t nameExt = FileBuffer::readUInt32Fast(p + 4);
        std::uint32_t dirHash = FileBuffer::readUInt32Fast(p + 8);
        // root objects:
        //     0x00000002 = Material (object type 4)
        //     0x00000003 = Blender (object type 2)
        //     0x00000004 = TextureSet (object type 5)
        //     0x00000005 = UVStream (object type 6)
        //     0x00000006 = Layer (object type 3)
        //     0x00000007 = MaterialFile (object type 1)
        std::uint32_t objectID = FileBuffer::readUInt32Fast(p + 12);
        // initialize using defaults from this object
        std::uint32_t baseObjID = FileBuffer::readUInt32Fast(p + 16);
        // true if baseObjID is valid
        bool          hasBaseObject = bool(p[20]);
#if ENABLE_CDB_DEBUG
        std::printf("    0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X, %d\n",
                    (unsigned int) nameHash, (unsigned int) nameExt,
                    (unsigned int) dirHash, (unsigned int) objectID,
                    (unsigned int) baseObjID, int(hasBaseObject));
#endif
        if (BRANCH_UNLIKELY(!objectID))
          continue;
        if (!hasBaseObject)
          baseObjID = 0U;
        CE2MaterialObject *o = allocateObject(objectTable, objectID, baseObjID);
        o->h = std::uint64_t(nameHash) | (std::uint64_t(dirHash) << 32);
        o->e = nameExt;
        if (nameExt != 0x0074616DU)             // "mat\0"
          continue;
        std::uint64_t h = 0xFFFFFFFFU;
        hashFunctionUInt64(h, o->h);
        hashFunctionUInt64(h, o->e);
        h = h & objectNameHashMask;
        size_t  collisionCnt = 0;
        for ( ; objectNameMap[h]; h = (h + 1U) & objectNameHashMask)
        {
          if (BRANCH_UNLIKELY(objectNameMap[h]->h == o->h &&
                              objectNameMap[h]->e == o->e))
          {
            if (objectNameMap[h]->type == o->type)
            {
              o = objectNameMap[h];
              break;
            }
            errorMessage("duplicate object name hash in material database");
          }
          if (++collisionCnt >= 1024)
          {
            errorMessage("CE2MaterialDB: internal error: "
                         "objectNameHashMask is too low");
          }
        }
        objectNameMap[h] = o;
      }
    }
    else if (t == 34)           // "BSComponentDB2::DBFileIndex::ComponentInfo"
    {
      unsigned int  n = buf2.readUInt32();
      if ((buf2.getPosition() + (std::uint64_t(n) << 3)) > buf2.size())
        errorMessage("unexpected end of LIST chunk in material database");
      if (componentInfoPtr)
        errorMessage("duplicate ComponentInfo list in material database");
      componentInfoPtr = buf2.data() + buf2.getPosition();
      componentCnt = n;
      componentDataPos = buf.getPosition();
#if ENABLE_CDB_DEBUG
      while (n--)
      {
        unsigned int  objectID = buf2.readUInt32Fast();
        unsigned int  componentIndex = buf2.readUInt16Fast();
        unsigned int  componentType = buf2.readUInt16Fast();
        std::printf("    0x%08X, 0x%04X, 0x%04X\n",
                    objectID, componentIndex, componentType);
      }
#endif
    }
    else if (t == 36)           // "BSComponentDB2::DBFileIndex::EdgeInfo"
    {
      unsigned int  n = buf2.readUInt32();
      if ((buf2.getPosition() + (std::uint64_t(n) * 12UL)) > buf2.size())
        errorMessage("unexpected end of LIST chunk in material database");
      const unsigned char *edgeInfoPtr = buf2.data() + buf2.getPosition();
      for (const unsigned char *p = edgeInfoPtr; n; n--, p = p + 12)
      {
        std::uint32_t childObjectID = FileBuffer::readUInt32Fast(p);
        std::uint32_t parentObjectID = FileBuffer::readUInt32Fast(p + 4);
        unsigned int  edgeType = FileBuffer::readUInt16Fast(p + 10);    // 0x64
#if ENABLE_CDB_DEBUG
        unsigned int  edgeIndex = FileBuffer::readUInt16Fast(p + 8);    // 0
        std::printf("    0x%08X, 0x%08X, 0x%04X, 0x%04X\n",
                    (unsigned int) childObjectID, (unsigned int) parentObjectID,
                    edgeIndex, edgeType);
#endif
        if (BRANCH_UNLIKELY(edgeType != 0x0064U))
          continue;             // not "BSComponentDB2::OuterEdge"
        const CE2MaterialObject *o = findObject(objectTable, childObjectID);
        if (!o)
          errorMessage("invalid object ID in EdgeInfo list in CDB file");
#if ENABLE_CDB_DEBUG
        if (o->parent)
        {
          std::printf("Warning: parent of object 0x%08X redefined\n",
                      (unsigned int) childObjectID);
        }
#endif
        const_cast< CE2MaterialObject * >(o)->parent =
            findObject(objectTable, parentObjectID);
      }
    }
  }
  buf.setPosition(componentDataPos);
  return componentCnt;
}

void CE2MaterialDB::readComponents(
    FileBuffer& buf, const unsigned char *componentInfoPtr, size_t componentCnt,
    const std::vector< CE2MaterialObject * >& objectTable)
{
  std::string stringBuf;
  for (size_t componentID = 0; componentID < componentCnt; )
  {
    if ((buf.getPosition() + 8ULL) > buf.size())
      errorMessage("unexpected end of material database file");
    unsigned int  chunkType = buf.readUInt32Fast();
    unsigned int  chunkSize = buf.readUInt32Fast();
    if ((buf.getPosition() + std::uint64_t(chunkSize)) > buf.size())
      errorMessage("unexpected end of material database file");
    FileBuffer  buf2(buf.data() + buf.getPosition(), chunkSize, 0);
    buf.setPosition(buf.getPosition() + chunkSize);
    if (BRANCH_UNLIKELY(chunkType != 0x46464944U && chunkType != 0x544A424FU))
      continue;                                 // not "DIFF" or "OBJT"
    bool    isDiff = (chunkType != 0x544A424FU);
    const unsigned char *cmpInfoPtr = componentInfoPtr + (componentID << 3);
    unsigned int  objectID = FileBuffer::readUInt32Fast(cmpInfoPtr);
    unsigned int  componentIndex = FileBuffer::readUInt16Fast(cmpInfoPtr + 4);
    unsigned int  componentType = FileBuffer::readUInt16Fast(cmpInfoPtr + 6);
    componentID++;
    CE2MaterialObject *o =
        const_cast< CE2MaterialObject * >(findObject(objectTable, objectID));
    if (BRANCH_UNLIKELY(!o))
      continue;
    if (BRANCH_UNLIKELY(o->type < 0))
      initializeObject(o, objectTable);
    if (BRANCH_UNLIKELY(chunkSize < 4U))
      continue;
    int     objectType = o->type;
#if ENABLE_CDB_DEBUG
    char    chunkTypeStr[8];
    chunkTypeStr[0] = char(chunkType & 0x7FU);
    chunkTypeStr[1] = char((chunkType >> 8) & 0x7FU);
    chunkTypeStr[2] = char((chunkType >> 16) & 0x7FU);
    chunkTypeStr[3] = char((chunkType >> 24) & 0x7FU);
    chunkTypeStr[4] = '\0';
    std::printf("%s at 0x%08X, size =%5u, object = 0x%08X:%d, ",
                chunkTypeStr,
                (unsigned int) (buf.getPosition() - (chunkSize + 8U)),
                chunkSize, objectID, objectType);
    int     t = findString(buf2.readUInt32Fast());
    if (t < 0)
    {
      std::printf("component 0x%04X slot %3u: ", componentType, componentIndex);
    }
    else
    {
      std::printf("component 0x%04X (%s) slot %3u: ",
                  componentType, stringTable[t], componentIndex);
    }
#endif
    if (!o)
    {
#if ENABLE_CDB_DEBUG
      std::printf("\nWarning: invalid object ID in material database\n");
#endif
      continue;
    }
    {
      size_t  tmp = 2;
      if (componentType >= 0x0067U && componentType <= 0x006CU)
        tmp = 4;
      if (!isDiff)
        buf2.setPosition(4);
      else
        buf2.setPosition(tmp + 4);
      if ((buf2.getPosition() + tmp) > buf2.size())
      {
#if ENABLE_CDB_DEBUG
        std::fputc('\n', stdout);
#  if ENABLE_CDB_DEBUG > 1
        printHexData(buf2.data() + 4, buf2.size() - 4);
#  endif
#endif
        continue;
      }
    }
    switch (componentType)
    {
      case 0x0061:                      // "BSComponentDB::CTName"
        {
          unsigned int  len = buf2.readUInt16Fast();
          o->name = readStringParam(stringBuf, buf2, len, 0);
#if ENABLE_CDB_DEBUG
          std::printf("%s", stringBuf.c_str());
#endif
        }
        break;
      case 0x0067:                      // "BSMaterial::BlenderID"
      case 0x0068:                      // "BSMaterial::LayerID"
      case 0x0069:                      // "BSMaterial::MaterialID"
      case 0x006A:                      // "BSMaterial::TextureSetID"
      case 0x006B:                      // "BSMaterial::UVStreamID"
      case 0x006C:                      // "BSMaterial::LODMaterialID"
        {
          std::uint32_t objectID2 = buf2.readUInt32Fast();
#if ENABLE_CDB_DEBUG
          std::printf("0x%08X", (unsigned int) objectID2);
#endif
          const CE2MaterialObject *p = findObject(objectTable, objectID2);
#if ENABLE_CDB_DEBUG
          if (objectID2 && !p)
            std::printf("\nWarning: invalid object ID in material database");
#endif
          switch (componentType)
          {
            case 0x0067:                // "BSMaterial::BlenderID"
              if (objectType == 1 && componentIndex < CE2Material::maxBlenders)
              {
                static_cast< CE2Material * >(o)->blenders[componentIndex] =
                    static_cast< const CE2Material::Blender * >(p);
              }
#if ENABLE_CDB_DEBUG
              else
              {
                std::printf("\nWarning: invalid blender link in CDB file");
              }
#endif
              break;
            case 0x0068:                // "BSMaterial::LayerID"
              if (objectType == 1 && componentIndex < CE2Material::maxLayers)
              {
                CE2Material *m = static_cast< CE2Material * >(o);
                m->layers[componentIndex] =
                    static_cast< const CE2Material::Layer * >(p);
                if (!p)
                  m->layerMask &= ~(1U << componentIndex);
                else
                  m->layerMask |= (1U << componentIndex);
              }
#if ENABLE_CDB_DEBUG
              else
              {
                std::printf("\nWarning: invalid layer link in CDB file");
              }
#endif
              break;
            case 0x0069:                // "BSMaterial::MaterialID"
              if (objectType == 3)
              {
                static_cast< CE2Material::Layer * >(o)->material =
                    static_cast< const CE2Material::Material * >(p);
              }
#if ENABLE_CDB_DEBUG
              else
              {
                std::printf("\nWarning: invalid material link in CDB file");
              }
#endif
              break;
            case 0x006A:                // "BSMaterial::TextureSetID"
              if (objectType == 4)
              {
                static_cast< CE2Material::Material * >(o)->textureSet =
                    static_cast< const CE2Material::TextureSet * >(p);
              }
#if ENABLE_CDB_DEBUG
              else
              {
                std::printf("\nWarning: invalid texture set link in CDB file");
              }
#endif
              break;
            case 0x006B:                // "BSMaterial::UVStreamID"
              {
                const CE2Material::UVStream *uvStream =
                    static_cast< const CE2Material::UVStream * >(p);
                if (objectType == 2)
                  static_cast< CE2Material::Blender * >(o)->uvStream = uvStream;
                else if (objectType == 3)
                  static_cast< CE2Material::Layer * >(o)->uvStream = uvStream;
#if ENABLE_CDB_DEBUG
                else
                  std::printf("\nWarning: invalid UV stream link in CDB file");
#endif
              }
              break;
            case 0x006C:                // "BSMaterial::LODMaterialID"
              if (objectType == 1 &&
                  componentIndex < CE2Material::maxLODMaterials)
              {
                static_cast< CE2Material * >(o)->lodMaterials[componentIndex] =
                    static_cast< const CE2Material * >(p);
              }
#if ENABLE_CDB_DEBUG
              else
              {
                std::printf("\nWarning: invalid LOD material link in CDB file");
              }
#endif
              break;
          }
        }
        break;
      case 0x006E:      // "BSMaterial::TextureReplacement"
        if (objectType == 2 ||
            (objectType == 5 &&
             componentIndex < CE2Material::TextureSet::maxTexturePaths))
        {
          if (isDiff)
            buf2.setPosition(buf2.getPosition() - 2);
          std::uint32_t *c;
          if (objectType == 2)
          {
            c = &(static_cast< CE2Material::Blender * >(o)->textureReplacement);
          }
          else
          {
            c = &(static_cast< CE2Material::TextureSet * >(
                      o)->textureReplacements[componentIndex]);
          }
          FloatVector4  tmp(c);
          tmp *= (1.0f / 255.0f);
          bool    readingStruct = false;
          for (size_t n = 0; (buf2.getPosition() + 4ULL) <= buf2.size(); n++)
          {
            if (isDiff)
            {
              n = buf2.readUInt16Fast();
              if (n && !readingStruct)
              {
                if (n > 1)
                  break;
                buf2.setPosition(buf2.getPosition() + 2);
                readingStruct = true;
                continue;
              }
              if (readingStruct)
                n++;
            }
            if (n > 4)
            {
              if (!readingStruct)
                break;
              buf2.setPosition(buf2.getPosition() + 2);
              readingStruct = false;
              continue;
            }
            if (n == 0)
              buf2.setPosition(buf2.getPosition() + 1); // unknown boolean
            else if ((buf2.getPosition() + 4ULL) <= buf2.size())
              tmp[n - 1] = std::min(std::max(buf2.readFloat(), 0.0f), 1.0f);
          }
#if ENABLE_CDB_DEBUG
          std::printf("%.3f, %.3f, %.3f, %.3f", tmp[0], tmp[1], tmp[2], tmp[3]);
#endif
          *c = std::uint32_t(tmp * 255.0f);
        }
        break;
      case 0x006F:      // "BSMaterial::MaterialOverrideColorTypeComponent"
        {
          unsigned int  len = buf2.readUInt16Fast();
          buf2.readString(stringBuf, len);
#if ENABLE_CDB_DEBUG
          std::printf("%s", stringBuf.c_str());
#endif
          if (objectType == 4)
          {
            static_cast< CE2Material::Material * >(o)->colorMode =
                (unsigned char) (stringBuf == "Lerp" ? 1 : 0);
          }
        }
        break;
      case 0x0075:      // "BSMaterial::AlphaSettingsComponent"
        if (objectType == 1 && isDiff)
        {
          CE2Material *m = static_cast< CE2Material * >(o);
          buf2.setPosition(buf2.getPosition() - 2);
          bool    readingStruct = false;
          while ((buf2.getPosition() + 4ULL) <= buf2.size())
          {
            int     fieldNum = buf2.readUInt16Fast();
            if (fieldNum > 8)
            {
              if (!readingStruct)
                break;
              readingStruct = false;
              buf2.setPosition(buf2.getPosition() + 2);
              continue;
            }
            if (readingStruct)
              fieldNum = fieldNum | 0x30;
#if ENABLE_CDB_DEBUG
            std::printf("  %02x: ", (unsigned int) fieldNum);
#endif
            switch (fieldNum)
            {
              case 3:
                readingStruct = true;
                break;
              case 0:                   // alpha testing enabled
              case 4:                   // unknown booleans
              case 0x31:
              case 0x32:
                {
                  bool    tmp = bool(buf2.readUInt8Fast());
                  if (fieldNum == 0)
                    m->setFlags(CE2Material::Flag_AlphaTesting, tmp);
#if ENABLE_CDB_DEBUG
                  std::printf("%d", int(tmp));
#endif
                }
                break;
              case 1:                   // alpha threshold (default: 0.333333)
              case 6:                   // unknown, default: 0.0
              case 7:
              case 8:
              case 0x36:                // unknown, default: 0.05
              case 0x38:                // unknown, default: 0.5
                if ((buf2.getPosition() + 4ULL) <= buf2.size())
                {
                  float   tmp = buf2.readFloat();
                  tmp = std::min(std::max(tmp, 0.0f), 1.0f);
                  if (fieldNum == 1)
                    m->alphaThreshold = tmp;
#if ENABLE_CDB_DEBUG
                  std::printf("%f", tmp);
#endif
                }
                break;
              case 2:                   // unknown, default: "MATERIAL_LAYER_0"
              case 0x30:                // unknown, "Linear" (default),
                                        // "Additive" or "PositionContrast"
              case 0x33:                // unknown, "Red" (default) or "Alpha"
                {
                  unsigned int  len = buf2.readUInt16Fast();
                  buf2.readString(stringBuf, len);
#if ENABLE_CDB_DEBUG
                  std::printf("%s", stringBuf.c_str());
#endif
                }
                break;
              case 0x34:
                if ((buf2.getPosition() + 8ULL) <= buf2.size())
                {
#if ENABLE_CDB_DEBUG
                  // unknown, always 0
                  unsigned int  tmp1 = buf2.readUInt32Fast();
                  // UVStream object ID
                  unsigned int  tmp2 = buf2.readUInt32Fast();
                  std::printf("%u, 0x%08X", tmp1, tmp2);
#else
                  buf2.setPosition(buf2.getPosition() + 8);
#endif
                }
                break;
              default:
                buf2.setPosition(buf2.size());
                break;
            }
          }
        }
        break;
      case 0x0076:      // "BSMaterial::DecalSettingsComponent"
        if (objectType == 1)
        {
          static_cast< CE2Material * >(
              o)->setFlags(CE2Material::Flag_IsDecal, true);
        }
        break;
      case 0x0078:      // "BSMaterial::EmissiveSettingsComponent"
        if (objectType == 1 && isDiff)
        {
          CE2Material *m = static_cast< CE2Material * >(o);
          buf2.setPosition(buf2.getPosition() - 2);
          std::uint32_t structID = 0U;
          while ((buf2.getPosition() + 4ULL) <= buf2.size())
          {
            unsigned int  n = buf2.readUInt16Fast();
            if (n & 0x8000U)
            {
              if (!structID)
                break;
              structID = structID >> 8;
              buf2.setPosition(buf2.getPosition() + 2);
              continue;
            }
            n = n | (structID << 8);
#if ENABLE_CDB_DEBUG
            std::printf("  %02x: ", n);
#endif
            switch (n)
            {
              case 0:                   // 0: emissive enabled, default: false
              case 0x8104:              // 1:4: unknown boolean, default: false
              case 0x8107:              // 1:7
                {
                  bool    tmp = bool(buf2.readUInt8Fast());
                  if (n == 0)
                    m->setFlags(CE2Material::Flag_Glow, tmp);
#if ENABLE_CDB_DEBUG
                  std::printf("%d", int(tmp));
#endif
                }
                break;
              case 1:                   // structures
              case 0x8101:
                structID = (structID << 8) | ((n & 0xFFU) | 0x80U);
                break;
              case 0x00818100:          // 1:1:0: red
              case 0x00818101:          // 1:1:1: green
              case 0x00818102:          // 1:1:2: blue
              case 0x00818103:          // 1:1:3: emissive scale
              case 0x8103:              // 1:3: unknown float, default: 0.0
              case 0x8105:              // 1:5: unknown float, default: 432.0
              case 0x8106:              // 1:6: unknown float, default: 0.0
              case 0x8108:              // 1:8: unknown float, default: 9999.0
              case 0x8109:              // 1:9: unknown float, default: 0.0
                if ((buf2.getPosition() + 4ULL) <= buf2.size())
                {
                  float   tmp = buf2.readFloat();
                  n = n - 0x00818100U;
                  if (!(n & ~3U))
                    m->emissiveColor[n] = std::min(std::max(tmp, 0.0f), 1.0f);
#if ENABLE_CDB_DEBUG
                  std::printf("%f", tmp);
#endif
                }
                break;
              case 0x8100:              // unknown, default: "MATERIAL_LAYER_0"
              case 0x8102:              // unknown, can be "None" or "Blender1"
                {
                  unsigned int  len = buf2.readUInt16Fast();
                  buf2.readString(stringBuf, len);
#if ENABLE_CDB_DEBUG
                  std::printf("%s", stringBuf.c_str());
#endif
                }
                break;
              default:                  // unrecognized data field
                buf2.setPosition(buf2.size());
                break;
            }
          }
        }
        break;
      case 0x007C:      // "BSMaterial::EffectSettingsComponent"
        if (objectType == 1)
        {
          static_cast< CE2Material * >(
              o)->setFlags(CE2Material::Flag_IsEffect, true);
        }
        break;
      case 0x007D:      // "BSMaterial::OpacityComponent"
        if (objectType == 1 && isDiff)
        {
          CE2Material *m = static_cast< CE2Material * >(o);
          buf2.setPosition(buf2.getPosition() - 2);
          while ((buf2.getPosition() + 4ULL) <= buf2.size())
          {
            int     fieldNum = buf2.readUInt16Fast();
            if (fieldNum > 9)
              break;
#if ENABLE_CDB_DEBUG
            std::printf("  %02x: ", (unsigned int) fieldNum);
#endif
            switch (fieldNum)
            {
              case 0:                   // default: "MATERIAL_LAYER_0"
              case 2:                   // default: "MATERIAL_LAYER_1"
              case 3:                   // default: "BLEND_LAYER_0"
              case 4:                   // default: "Lerp"
              case 6:                   // default: "MATERIAL_LAYER_2"
              case 7:                   // default: "BLEND_LAYER_1"
              case 8:                   // default: "Lerp"
                {
                  unsigned int  len = buf2.readUInt16Fast();
                  buf2.readString(stringBuf, len);
                  if (fieldNum == 4 || fieldNum == 8)
                  {
                    unsigned char opacityMode = 0;
                    if (stringBuf == "Additive")
                      opacityMode = 1;
                    else if (stringBuf == "Subtractive")
                      opacityMode = 2;
                    else if (stringBuf == "Multiplicative")
                      opacityMode = 3;
                    if (fieldNum == 4)
                      m->opacityMode1 = opacityMode;
                    else
                      m->opacityMode2 = opacityMode;
                  }
#if ENABLE_CDB_DEBUG
                  std::printf("%s", stringBuf.c_str());
#endif
                }
                break;
              case 1:                   // unknown booleans
              case 5:
                {
#if ENABLE_CDB_DEBUG
                  bool    tmp = bool(buf2.readUInt8Fast());
                  std::printf("%d", int(tmp));
#else
                  buf2.setPosition(buf2.getPosition() + 1);
#endif
                }
                break;
              case 9:                   // unknown, default: 0.0
                if ((buf2.getPosition() + 4ULL) <= buf2.size())
                {
#if ENABLE_CDB_DEBUG
                  float   tmp = buf2.readFloat();
                  std::printf("%f", tmp);
#else
                  buf2.setPosition(buf2.getPosition() + 4);
#endif
                }
                break;
            }
          }
        }
        break;
      case 0x0081:      // "BSMaterial::WaterSettingsComponent"
        if (objectType == 1)
        {
          static_cast< CE2Material * >(o)->setFlags(
              CE2Material::Flag_IsWater | CE2Material::Flag_TwoSided
              | CE2Material::Flag_AlphaBlending, true);
        }
        break;
      case 0x0086:      // "BSMaterial::TerrainSettingsComponent"
        if (objectType == 1)
        {
          static_cast< CE2Material * >(
              o)->setFlags(CE2Material::Flag_IsTerrain, true);
        }
        break;
      case 0x0089:      // "BSMaterial::VegetationSettingsComponent"
        if (objectType == 1)
        {
          static_cast< CE2Material * >(o)->setFlags(
              CE2Material::Flag_IsVegetation | CE2Material::Flag_TwoSided,
              true);
        }
        break;
      case 0x0091:      // "BSMaterial::Color"
        if (objectType == 4)
        {
          CE2Material::Material *m = static_cast< CE2Material::Material * >(o);
          for (size_t n = 0; (buf2.getPosition() + 4ULL) <= buf2.size(); n++)
          {
            if (isDiff)
              n = buf2.readUInt16Fast();
            if (n > 3)
              break;
            m->color[n] = std::min(std::max(buf2.readFloat(), 0.0f), 1.0f);
          }
#if ENABLE_CDB_DEBUG
          std::printf("%f, %f, %f, %f",
                      m->color[0], m->color[1], m->color[2], m->color[3]);
#endif
        }
        break;
      case 0x0093:      // "BSMaterial::Scale"
      case 0x0094:      // "BSMaterial::Offset"
        if (objectType == 6)
        {
          CE2Material::UVStream *s = static_cast< CE2Material::UVStream * >(o);
          size_t  offs = ~componentType & 2U;
          for (size_t n = 0; (buf2.getPosition() + 4ULL) <= buf2.size(); n++)
          {
            if (isDiff)
              n = buf2.readUInt16Fast();
            if (n > 1)
              break;
            s->scaleAndOffset[n + offs] = buf2.readFloat();
          }
#if ENABLE_CDB_DEBUG
          std::printf("%f, %f",
                      s->scaleAndOffset[offs], s->scaleAndOffset[offs + 1]);
#endif
        }
        break;
      case 0x0096:      // "BSMaterial::TextureFile"
      case 0x0097:      // "BSMaterial::MRTextureFile"
        {
          unsigned int  len = buf2.readUInt16Fast();
          const std::string *txtPath = readStringParam(stringBuf, buf2, len, 1);
#if ENABLE_CDB_DEBUG
          std::printf("%s", txtPath->c_str());
#endif
          if (objectType == 5 &&
              componentIndex < CE2Material::TextureSet::maxTexturePaths)
          {
            CE2Material::TextureSet *txtSet =
                static_cast< CE2Material::TextureSet * >(o);
            txtSet->texturePaths[componentIndex] = txtPath;
            if (txtPath->empty())
              txtSet->texturePathMask &= ~(1U << componentIndex);
            else
              txtSet->texturePathMask |= (1U << componentIndex);
          }
          else if (objectType == 2)
          {
            static_cast< CE2Material::Blender * >(o)->texturePath = txtPath;
          }
#if ENABLE_CDB_DEBUG
          else
          {
            std::printf("\nWarning: invalid texture file link in CDB file");
          }
#endif
        }
        break;
      case 0x0098:      // "BSMaterial::MaterialParamFloat"
        if ((buf2.getPosition() + 4ULL) <= buf2.size())
        {
          float   tmp = std::min(std::max(buf2.readFloat(), 0.0f), 1.0f);
          if (objectType == 2 &&
              componentIndex < CE2Material::Blender::maxFloatParams)
          {
            static_cast< CE2Material::Blender * >(
                o)->floatParams[componentIndex] = tmp;
          }
          else if (objectType == 5)
          {
            static_cast< CE2Material::TextureSet * >(o)->floatParam = tmp;
          }
#if ENABLE_CDB_DEBUG
          std::printf("%f", tmp);
#endif
        }
        break;
      case 0x009A:      // "BSMaterial::ParamBool"
        if (buf2.getPosition() < buf2.size())
        {
          bool    tmp = bool(buf2.readUInt8Fast());
          if (objectType == 2 &&
              componentIndex < CE2Material::Blender::maxBoolParams)
          {
            static_cast< CE2Material::Blender * >(
                o)->boolParams[componentIndex] = tmp;
          }
#if ENABLE_CDB_DEBUG
          std::printf("%d", int(tmp));
#endif
        }
        break;
      case 0x009C:      // "BSMaterial::TextureAddressModeComponent"
        {
          unsigned int  len = buf2.readUInt16Fast();
          buf2.readString(stringBuf, len);
#if ENABLE_CDB_DEBUG
          std::printf("%s", stringBuf.c_str());
#endif
          if (objectType == 6)
          {
            unsigned char m = 0;
            if (stringBuf == "Clamp")
              m = 1;
            else if (stringBuf == "Mirror")
              m = 2;
            else if (stringBuf == "Border")
              m = 3;
            static_cast< CE2Material::UVStream * >(o)->textureAddressMode = m;
          }
        }
        break;
    }
#if ENABLE_CDB_DEBUG
    std::fputc('\n', stdout);
#  if ENABLE_CDB_DEBUG > 1
    printHexData(buf2.data() + 4, buf2.size() - 4);
#  endif
#endif
  }
  for (size_t i = 0; i < objectTable.size(); i++)
  {
    if (objectTable[i] && objectTable[i]->type < 0)
      initializeObject(objectTable[i], objectTable);
  }
}

CE2MaterialDB::CE2MaterialDB()
{
  objectNameMap.resize(objectNameHashMask + 1, (CE2MaterialObject *) 0);
  objectBuffers.emplace_back();
  objectBuffers.back().reserve(65536);
  storedStringParams.resize(stringHashMask + 1, 0U);
  stringBuffers.emplace_back();
  stringBuffers.back().reserve(stringBufMask + 1);
  stringBuffers.back().emplace_back("");
}

CE2MaterialDB::CE2MaterialDB(const BA2File& ba2File, const char *fileName)
{
  objectNameMap.resize(objectNameHashMask + 1, (CE2MaterialObject *) 0);
  objectBuffers.emplace_back();
  objectBuffers.back().reserve(65536);
  storedStringParams.resize(stringHashMask + 1, 0U);
  stringBuffers.emplace_back();
  stringBuffers.back().reserve(stringBufMask + 1);
  stringBuffers.back().emplace_back("");
  loadCDBFile(ba2File, fileName);
}

CE2MaterialDB::~CE2MaterialDB()
{
}

void CE2MaterialDB::loadCDBFile(FileBuffer& buf)
{
  if (buf.readUInt64() != 0x0000000848544542ULL)        // "BETH", 8
    errorMessage("invalid or unsupported material database file");
  (void) buf.readUInt32();              // unknown, possibly version number (4)
  // first pass: create tables
  std::vector< CE2MaterialObject * >  objectTable;
  const unsigned char *componentInfoPtr = (unsigned char *) 0;
  size_t  componentCnt = readTables(componentInfoPtr, objectTable, buf);
  // second pass: load components
  if (componentInfoPtr && componentCnt)
    readComponents(buf, componentInfoPtr, componentCnt, objectTable);
}

void CE2MaterialDB::loadCDBFile(const unsigned char *buf, size_t bufSize)
{
  FileBuffer  tmpBuf(buf, bufSize);
  loadCDBFile(tmpBuf);
}

void CE2MaterialDB::loadCDBFile(const BA2File& ba2File, const char *fileName)
{
  std::vector< unsigned char >  fileBuf;
  if (!fileName || fileName[0] == '\0')
    fileName = "materials/materialsbeta.cdb";
  const unsigned char *buf = (unsigned char *) 0;
  size_t  bufSize = ba2File.extractFile(buf, fileBuf, std::string(fileName));
  FileBuffer  tmpBuf(buf, bufSize);
  loadCDBFile(tmpBuf);
}

const CE2Material * CE2MaterialDB::findMaterial(
    const std::string& fileName) const
{
  std::uint64_t tmp = 0U;
  std::uint32_t e = calculateHash(tmp, fileName);
  std::uint64_t h = 0xFFFFFFFFU;
  hashFunctionUInt64(h, tmp);
  hashFunctionUInt64(h, e);
  h = h & objectNameHashMask;
  for ( ; objectNameMap[h]; h = (h + 1U) & objectNameHashMask)
  {
    if (objectNameMap[h]->h == tmp && objectNameMap[h]->e == e)
      break;
  }
  const CE2MaterialObject *o = objectNameMap[h];
  if (o && o->type == 1)
    return static_cast< const CE2Material * >(o);
  return (CE2Material *) 0;
}

#if 0
void CE2MaterialDB::printTables(const BA2File& ba2File, const char *fileName)
{
  std::printf("const std::uint32_t CE2MaterialDB::crc32Table[256] =\n");
  std::printf("{\n");
  for (size_t i = 0; i < 256; i++)
  {
    unsigned int  crcValue = (unsigned int) i;
    for (size_t j = 0; j < 8; j++)
      crcValue = (crcValue >> 1) ^ (0xEDB88320U & (0U - (crcValue & 1U)));
    if ((i % 6) == 0)
      std::fputc(' ', stdout);
    std::printf(" 0x%08X", crcValue);
    if (i == 255)
      std::fputc('\n', stdout);
    else if ((i % 6) != 5)
      std::fputc(',', stdout);
    else
      std::printf(",\n");
  }
  std::printf("};\n\n");

  std::vector< unsigned char >  fileBuf;
  if (!fileName || fileName[0] == '\0')
    fileName = "materials/materialsbeta.cdb";
  ba2File.extractFile(fileBuf, std::string(fileName));
  FileBuffer  buf(fileBuf.data(), fileBuf.size());
  if (buf.readUInt64() != 0x0000000848544542ULL)        // "BETH", 8
    errorMessage("invalid or unsupported material database file");
  (void) buf.readUInt32();              // unknown, 4
  std::vector< std::string >  stringTable;
  for (unsigned int chunkCnt = buf.readUInt32(); chunkCnt > 1U; chunkCnt--)
  {
    unsigned int  chunkType = buf.readUInt32();
    unsigned int  chunkSize = buf.readUInt32();
    if ((buf.getPosition() + std::uint64_t(chunkSize)) > buf.size())
      errorMessage("unexpected end of material database file");
    FileBuffer  buf2(buf.data() + buf.getPosition(), chunkSize);
    buf.setPosition(buf.getPosition() + chunkSize);
    if (chunkType == 0x54525453U)               // "STRT" (string table)
    {
      while (buf2.getPosition() < buf2.size())
      {
        stringTable.emplace_back();
        buf2.readString(stringTable.back());
      }
    }
  }
  std::sort(stringTable.begin(), stringTable.end());
  std::printf("const char * CE2MaterialDB::stringTable[%d] =\n",
              int(stringTable.size()));
  std::printf("{\n");
  for (size_t i = 0; i < stringTable.size(); i++)
  {
    std::printf("  \"%s\"%c",
                stringTable[i].c_str(),
                ((i + 1) < stringTable.size() ? ',' : ' '));
    for (size_t j = stringTable[i].length(); j < 51; j++)
      std::fputc(' ', stdout);
    std::printf("// %3d\n", int(i));
  }
  std::printf("};\n");
}
#endif

const std::uint32_t CE2MaterialDB::crc32Table[256] =
{
  0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F,
  0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
  0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2,
  0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
  0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9,
  0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
  0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C,
  0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
  0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423,
  0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
  0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106,
  0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
  0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D,
  0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
  0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950,
  0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
  0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7,
  0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
  0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA,
  0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
  0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81,
  0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
  0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84,
  0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
  0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB,
  0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
  0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E,
  0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
  0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55,
  0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
  0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28,
  0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
  0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F,
  0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
  0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242,
  0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
  0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69,
  0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
  0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC,
  0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
  0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693,
  0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
  0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

const char * CE2MaterialDB::stringTable[451] =
{
  "AOVertexColorChannel",                               //   0
  "ActiveLayersMask",                                   //   1
  "AdaptiveEmittance",                                  //   2
  "Address",                                            //   3
  "AlbedoPaletteTex",                                   //   4
  "AlbedoTint",                                         //   5
  "AlbedoTintColor",                                    //   6
  "AlphaBias",                                          //   7
  "AlphaChannel",                                       //   8
  "AlphaDistance",                                      //   9
  "AlphaPaletteTex",                                    //  10
  "AlphaTestThreshold",                                 //  11
  "AlphaTint",                                          //  12
  "AnimatedDecalIgnoresTAA",                            //  13
  "ApplyFlowOnANMR",                                    //  14
  "ApplyFlowOnEmissivity",                              //  15
  "ApplyFlowOnOpacity",                                 //  16
  "BSBind::Address",                                    //  17
  "BSBind::ColorCurveController",                       //  18
  "BSBind::ComponentProperty",                          //  19
  "BSBind::ControllerComponent",                        //  20
  "BSBind::Controllers",                                //  21
  "BSBind::Controllers::Mapping",                       //  22
  "BSBind::Directory",                                  //  23
  "BSBind::DirectoryComponent",                         //  24
  "BSBind::Float2DCurveController",                     //  25
  "BSBind::Float2DLerpController",                      //  26
  "BSBind::Float3DCurveController",                     //  27
  "BSBind::FloatCurveController",                       //  28
  "BSBind::FloatLerpController",                        //  29
  "BSBind::Multiplex",                                  //  30
  "BSBind::TimerController",                            //  31
  "BSColorCurve",                                       //  32
  "BSComponentDB2::DBFileIndex",                        //  33
  "BSComponentDB2::DBFileIndex::ComponentInfo",         //  34
  "BSComponentDB2::DBFileIndex::ComponentTypeInfo",     //  35
  "BSComponentDB2::DBFileIndex::EdgeInfo",              //  36
  "BSComponentDB2::DBFileIndex::ObjectInfo",            //  37
  "BSComponentDB2::ID",                                 //  38
  "BSComponentDB::CTName",                              //  39
  "BSFloat2DCurve",                                     //  40
  "BSFloat3DCurve",                                     //  41
  "BSFloatCurve",                                       //  42
  "BSFloatCurve::Control",                              //  43
  "BSMaterial::AlphaBlenderSettings",                   //  44
  "BSMaterial::AlphaSettingsComponent",                 //  45
  "BSMaterial::BlendModeComponent",                     //  46
  "BSMaterial::BlendParamFloat",                        //  47
  "BSMaterial::BlenderID",                              //  48
  "BSMaterial::Channel",                                //  49
  "BSMaterial::CollisionComponent",                     //  50
  "BSMaterial::Color",                                  //  51
  "BSMaterial::ColorChannelTypeComponent",              //  52
  "BSMaterial::ColorRemapSettingsComponent",            //  53
  "BSMaterial::DecalSettingsComponent",                 //  54
  "BSMaterial::DetailBlenderSettings",                  //  55
  "BSMaterial::DetailBlenderSettingsComponent",         //  56
  "BSMaterial::DistortionComponent",                    //  57
  "BSMaterial::EffectSettingsComponent",                //  58
  "BSMaterial::EmissiveSettingsComponent",              //  59
  "BSMaterial::EmittanceSettings",                      //  60
  "BSMaterial::EyeSettingsComponent",                   //  61
  "BSMaterial::FlipbookComponent",                      //  62
  "BSMaterial::FlowSettingsComponent",                  //  63
  "BSMaterial::GlobalLayerDataComponent",               //  64
  "BSMaterial::GlobalLayerNoiseSettings",               //  65
  "BSMaterial::HairSettingsComponent",                  //  66
  "BSMaterial::Internal::CompiledDB",                   //  67
  "BSMaterial::Internal::CompiledDB::FilePair",         //  68
  "BSMaterial::LODMaterialID",                          //  69
  "BSMaterial::LayerID",                                //  70
  "BSMaterial::LayeredEdgeFalloffComponent",            //  71
  "BSMaterial::LayeredEmissivityComponent",             //  72
  "BSMaterial::LevelOfDetailSettings",                  //  73
  "BSMaterial::MRTextureFile",                          //  74
  "BSMaterial::MaterialID",                             //  75
  "BSMaterial::MaterialOverrideColorTypeComponent",     //  76
  "BSMaterial::MaterialParamFloat",                     //  77
  "BSMaterial::MouthSettingsComponent",                 //  78
  "BSMaterial::Offset",                                 //  79
  "BSMaterial::OpacityComponent",                       //  80
  "BSMaterial::ParamBool",                              //  81
  "BSMaterial::PhysicsMaterialType",                    //  82
  "BSMaterial::ProjectedDecalSettings",                 //  83
  "BSMaterial::Scale",                                  //  84
  "BSMaterial::ShaderModelComponent",                   //  85
  "BSMaterial::ShaderRouteComponent",                   //  86
  "BSMaterial::SourceTextureWithReplacement",           //  87
  "BSMaterial::StarmapBodyEffectComponent",             //  88
  "BSMaterial::TerrainSettingsComponent",               //  89
  "BSMaterial::TerrainTintSettingsComponent",           //  90
  "BSMaterial::TextureAddressModeComponent",            //  91
  "BSMaterial::TextureFile",                            //  92
  "BSMaterial::TextureReplacement",                     //  93
  "BSMaterial::TextureResolutionSetting",               //  94
  "BSMaterial::TextureSetID",                           //  95
  "BSMaterial::TextureSetKindComponent",                //  96
  "BSMaterial::TranslucencySettings",                   //  97
  "BSMaterial::TranslucencySettingsComponent",          //  98
  "BSMaterial::UVStreamID",                             //  99
  "BSMaterial::UVStreamParamBool",                      // 100
  "BSMaterial::VegetationSettingsComponent",            // 101
  "BSMaterial::WaterFoamSettingsComponent",             // 102
  "BSMaterial::WaterGrimeSettingsComponent",            // 103
  "BSMaterial::WaterSettingsComponent",                 // 104
  "BSMaterialBinding::MaterialPropertyNode",            // 105
  "BSMaterialBinding::MaterialUVStreamPropertyNode",    // 106
  "BSResource::ID",                                     // 107
  "BackLightingEnable",                                 // 108
  "BackLightingTintColor",                              // 109
  "BacklightingScale",                                  // 110
  "BacklightingSharpness",                              // 111
  "BacklightingTransparencyFactor",                     // 112
  "BackscatterStrength",                                // 113
  "BackscatterWrap",                                    // 114
  "BaseFrequency",                                      // 115
  "Bias",                                               // 116
  "Binding",                                            // 117
  "BindingType",                                        // 118
  "BlendContrast",                                      // 119
  "BlendMaskContrast",                                  // 120
  "BlendMaskPosition",                                  // 121
  "BlendMode",                                          // 122
  "BlendNormalsAdditively",                             // 123
  "BlendPosition",                                      // 124
  "BlendSoftness",                                      // 125
  "Blender",                                            // 126
  "BlendingMode",                                       // 127
  "BlueChannel",                                        // 128
  "BlurStrength",                                       // 129
  "BranchFlexibility",                                  // 130
  "BuildVersion",                                       // 131
  "CameraDistanceFade",                                 // 132
  "Children",                                           // 133
  "Circular",                                           // 134
  "Class",                                              // 135
  "ClassReference",                                     // 136
  "Collisions",                                         // 137
  "Color",                                              // 138
  "Columns",                                            // 139
  "ComponentIndex",                                     // 140
  "ComponentType",                                      // 141
  "ComponentTypes",                                     // 142
  "Components",                                         // 143
  "ContactShadowSoftening",                             // 144
  "Contrast",                                           // 145
  "Controller",                                         // 146
  "Controls",                                           // 147
  "CorneaEyeRoughness",                                 // 148
  "CorneaSpecularity",                                  // 149
  "Curve",                                              // 150
  "DBID",                                               // 151
  "DEPRECATEDTerrainBlendGradientFactor",               // 152
  "DEPRECATEDTerrainBlendStrength",                     // 153
  "DefaultValue",                                       // 154
  "DepthBiasInUlp",                                     // 155
  "DepthMVFixup",                                       // 156
  "DepthMVFixupEdgesOnly",                              // 157
  "DepthOffsetMaskVertexColorChannel",                  // 158
  "DepthScale",                                         // 159
  "DetailBlendMask",                                    // 160
  "DetailBlendMaskUVStream",                            // 161
  "DetailBlenderSettings",                              // 162
  "DiffuseTransmissionScale",                           // 163
  "Dir",                                                // 164
  "DirectTransmissionScale",                            // 165
  "DirectionalityIntensity",                            // 166
  "DirectionalitySaturation",                           // 167
  "DirectionalityScale",                                // 168
  "DisplacementMidpoint",                               // 169
  "DitherDistanceMax",                                  // 170
  "DitherDistanceMin",                                  // 171
  "DitherScale",                                        // 172
  "Duration",                                           // 173
  "Easing",                                             // 174
  "Edge",                                               // 175
  "EdgeMaskContrast",                                   // 176
  "EdgeMaskDistanceMax",                                // 177
  "EdgeMaskDistanceMin",                                // 178
  "EdgeMaskMin",                                        // 179
  "Edges",                                              // 180
  "EmissiveClipThreshold",                              // 181
  "EmissiveMaskSourceBlender",                          // 182
  "EmissiveOnlyAutomaticallyApplied",                   // 183
  "EmissiveOnlyEffect",                                 // 184
  "EmissivePaletteTex",                                 // 185
  "EmissiveSourceLayer",                                // 186
  "EmissiveTint",                                       // 187
  "EnableAdaptiveLimits",                               // 188
  "Enabled",                                            // 189
  "End",                                                // 190
  "ExposureOffset",                                     // 191
  "Ext",                                                // 192
  "FPS",                                                // 193
  "FalloffStartAngle",                                  // 194
  "FalloffStartAngles",                                 // 195
  "FalloffStartOpacities",                              // 196
  "FalloffStartOpacity",                                // 197
  "FalloffStopAngle",                                   // 198
  "FalloffStopAngles",                                  // 199
  "FalloffStopOpacities",                               // 200
  "FalloffStopOpacity",                                 // 201
  "FarFadeValue",                                       // 202
  "File",                                               // 203
  "FileName",                                           // 204
  "First",                                              // 205
  "FirstBlenderIndex",                                  // 206
  "FirstBlenderMode",                                   // 207
  "FirstLayerIndex",                                    // 208
  "FirstLayerMaskIndex",                                // 209
  "FirstLayerTint",                                     // 210
  "FlipBackFaceNormalsInViewSpace",                     // 211
  "FlowExtent",                                         // 212
  "FlowIsAnimated",                                     // 213
  "FlowMap",                                            // 214
  "FlowMapAndTexturesAreFlipbooks",                     // 215
  "FlowSourceUVChannel",                                // 216
  "FlowSpeed",                                          // 217
  "FlowUVOffset",                                       // 218
  "FlowUVScale",                                        // 219
  "FoamTextureDistortion",                              // 220
  "FoamTextureScrollSpeed",                             // 221
  "ForceRenderBeforeOIT",                               // 222
  "FrequencyMultiplier",                                // 223
  "Frosting",                                           // 224
  "FrostingBlurBias",                                   // 225
  "FrostingUnblurredBackgroundAlphaBlend",              // 226
  "GlobalLayerNoiseData",                               // 227
  "GreenChannel",                                       // 228
  "HasData",                                            // 229
  "HasOpacity",                                         // 230
  "HashMap",                                            // 231
  "HeightBlendFactor",                                  // 232
  "HeightBlendThreshold",                               // 233
  "HurstExponent",                                      // 234
  "ID",                                                 // 235
  "Index",                                              // 236
  "IndirectSpecRoughness",                              // 237
  "IndirectSpecularScale",                              // 238
  "IndirectSpecularTransmissionScale",                  // 239
  "Input",                                              // 240
  "InputDistance",                                      // 241
  "IrisDepthPosition",                                  // 242
  "IrisDepthTransitionRatio",                           // 243
  "IrisSpecularity",                                    // 244
  "IrisTotalDepth",                                     // 245
  "IrisUVSize",                                         // 246
  "IsAFlipbook",                                        // 247
  "IsAlphaTested",                                      // 248
  "IsDecal",                                            // 249
  "IsDetailBlendMaskSupported",                         // 250
  "IsEmpty",                                            // 251
  "IsGlass",                                            // 252
  "IsPlanet",                                           // 253
  "IsProjected",                                        // 254
  "IsSampleInterpolating",                              // 255
  "IsSpikyHair",                                        // 256
  "IsTeeth",                                            // 257
  "LayerIndex",                                         // 258
  "LeafAmplitude",                                      // 259
  "LeafFrequency",                                      // 260
  "LightingPower",                                      // 261
  "LightingWrap",                                       // 262
  "Loop",                                               // 263
  "Loops",                                              // 264
  "LowLOD",                                             // 265
  "LowLODRootMaterial",                                 // 266
  "LuminousEmittance",                                  // 267
  "MappingsA",                                          // 268
  "Mask",                                               // 269
  "MaskDistanceFromShoreEnd",                           // 270
  "MaskDistanceFromShoreStart",                         // 271
  "MaskDistanceRampWidth",                              // 272
  "MaskIntensityMax",                                   // 273
  "MaskIntensityMin",                                   // 274
  "MaskNoiseAmp",                                       // 275
  "MaskNoiseAnimSpeed",                                 // 276
  "MaskNoiseBias",                                      // 277
  "MaskNoiseFreq",                                      // 278
  "MaskNoiseGlobalScale",                               // 279
  "MaskWaveParallax",                                   // 280
  "MaterialMaskIntensityScale",                         // 281
  "MaterialOverallAlpha",                               // 282
  "MaterialTypeOverride",                               // 283
  "MaxConcentrationPlankton",                           // 284
  "MaxConcentrationSediment",                           // 285
  "MaxConcentrationYellowMatter",                       // 286
  "MaxDepthOffset",                                     // 287
  "MaxDisplacement",                                    // 288
  "MaxInput",                                           // 289
  "MaxOffsetEmittance",                                 // 290
  "MaxParralaxOcclusionSteps",                          // 291
  "MaxValue",                                           // 292
  "MediumLODRootMaterial",                              // 293
  "MinInput",                                           // 294
  "MinOffsetEmittance",                                 // 295
  "MinValue",                                           // 296
  "MipBase",                                            // 297
  "Mode",                                               // 298
  "MostSignificantLayer",                               // 299
  "Name",                                               // 300
  "NearFadeValue",                                      // 301
  "NoHalfResOptimization",                              // 302
  "Nodes",                                              // 303
  "NoiseMaskTexture",                                   // 304
  "NormalAffectsStrength",                              // 305
  "NormalOverride",                                     // 306
  "NumLODMaterials",                                    // 307
  "Object",                                             // 308
  "ObjectID",                                           // 309
  "Objects",                                            // 310
  "OpacitySourceLayer",                                 // 311
  "OpacityUVStream",                                    // 312
  "Optimized",                                          // 313
  "ParallaxOcclusionScale",                             // 314
  "ParallaxOcclusionShadows",                           // 315
  "Parent",                                             // 316
  "Path",                                               // 317
  "PathStr",                                            // 318
  "PersistentID",                                       // 319
  "PhytoplanktonReflectanceColorB",                     // 320
  "PhytoplanktonReflectanceColorG",                     // 321
  "PhytoplanktonReflectanceColorR",                     // 322
  "PlacedWater",                                        // 323
  "Position",                                           // 324
  "ProjectedDecalSetting",                              // 325
  "ReceiveDirectionalShadows",                          // 326
  "ReceiveNonDirectionalShadows",                       // 327
  "RedChannel",                                         // 328
  "ReflectanceB",                                       // 329
  "ReflectanceG",                                       // 330
  "ReflectanceR",                                       // 331
  "RemapAlbedo",                                        // 332
  "RemapEmissive",                                      // 333
  "RemapOpacity",                                       // 334
  "RenderLayer",                                        // 335
  "Replacement",                                        // 336
  "ResolutionHint",                                     // 337
  "RotationAngle",                                      // 338
  "Roughness",                                          // 339
  "Route",                                              // 340
  "Rows",                                               // 341
  "SSSStrength",                                        // 342
  "SSSWidth",                                           // 343
  "ScleraEyeRoughness",                                 // 344
  "ScleraSpecularity",                                  // 345
  "Second",                                             // 346
  "SecondBlenderIndex",                                 // 347
  "SecondBlenderMode",                                  // 348
  "SecondLayerActive",                                  // 349
  "SecondLayerIndex",                                   // 350
  "SecondLayerMaskIndex",                               // 351
  "SecondLayerTint",                                    // 352
  "SecondMostSignificantLayer",                         // 353
  "SedimentReflectanceColorB",                          // 354
  "SedimentReflectanceColorG",                          // 355
  "SedimentReflectanceColorR",                          // 356
  "Settings",                                           // 357
  "SoftEffect",                                         // 358
  "SoftFalloffDepth",                                   // 359
  "SourceDirection",                                    // 360
  "SourceDirectoryHash",                                // 361
  "SourceID",                                           // 362
  "SpecLobe0RoughnessScale",                            // 363
  "SpecLobe1RoughnessScale",                            // 364
  "SpecScale",                                          // 365
  "SpecularOpacityOverride",                            // 366
  "SpecularTransmissionScale",                          // 367
  "Start",                                              // 368
  "StreamID",                                           // 369
  "Strength",                                           // 370
  "SurfaceHeightMap",                                   // 371
  "Tangent",                                            // 372
  "TangentBend",                                        // 373
  "TargetID",                                           // 374
  "TargetUVStream",                                     // 375
  "TerrainBlendGradientFactor",                         // 376
  "TerrainBlendStrength",                               // 377
  "TexcoordScaleAndBias",                               // 378
  "TexcoordScale_XY",                                   // 379
  "TexcoordScale_XZ",                                   // 380
  "TexcoordScale_YZ",                                   // 381
  "Texture",                                            // 382
  "TextureMappingType",                                 // 383
  "Thin",                                               // 384
  "ThirdLayerActive",                                   // 385
  "ThirdLayerIndex",                                    // 386
  "ThirdLayerMaskIndex",                                // 387
  "ThirdLayerTint",                                     // 388
  "TilingDistance",                                     // 389
  "TransmissiveScale",                                  // 390
  "TransmittanceSourceLayer",                           // 391
  "TransmittanceWidth",                                 // 392
  "TrunkFlexibility",                                   // 393
  "Type",                                               // 394
  "UVStreamTargetBlender",                              // 395
  "UVStreamTargetLayer",                                // 396
  "UseDetailBlendMask",                                 // 397
  "UseDitheredTransparency",                            // 398
  "UseFallOff",                                         // 399
  "UseGBufferNormals",                                  // 400
  "UseNoiseMaskTexture",                                // 401
  "UseParallaxOcclusionMapping",                        // 402
  "UseRGBFallOff",                                      // 403
  "UseRandomOffset",                                    // 404
  "UseSSS",                                             // 405
  "UseVertexAlpha",                                     // 406
  "UseVertexColor",                                     // 407
  "UsesDirectionality",                                 // 408
  "Value",                                              // 409
  "VariationStrength",                                  // 410
  "Version",                                            // 411
  "VertexColorBlend",                                   // 412
  "VertexColorChannel",                                 // 413
  "VeryLowLODRootMaterial",                             // 414
  "WaterDepthBlur",                                     // 415
  "WaterEdgeFalloff",                                   // 416
  "WaterEdgeNormalFalloff",                             // 417
  "WaterRefractionMagnitude",                           // 418
  "WaterWetnessMaxDepth",                               // 419
  "WaveAmplitude",                                      // 420
  "WaveDistortionAmount",                               // 421
  "WaveFlipWaveDirection",                              // 422
  "WaveParallaxFalloffBias",                            // 423
  "WaveParallaxFalloffScale",                           // 424
  "WaveParallaxInnerStrength",                          // 425
  "WaveParallaxOuterStrength",                          // 426
  "WaveScale",                                          // 427
  "WaveShoreFadeInnerDistance",                         // 428
  "WaveShoreFadeOuterDistance",                         // 429
  "WaveSpawnFadeInDistance",                            // 430
  "WaveSpeed",                                          // 431
  "WorldspaceScaleFactor",                              // 432
  "WriteMask",                                          // 433
  "XCurve",                                             // 434
  "XMFLOAT2",                                           // 435
  "XMFLOAT3",                                           // 436
  "XMFLOAT4",                                           // 437
  "YCurve",                                             // 438
  "YellowMatterReflectanceColorB",                      // 439
  "YellowMatterReflectanceColorG",                      // 440
  "YellowMatterReflectanceColorR",                      // 441
  "ZCurve",                                             // 442
  "ZTest",                                              // 443
  "ZWrite",                                             // 444
  "upControllers",                                      // 445
  "upDir",                                              // 446
  "w",                                                  // 447
  "x",                                                  // 448
  "y",                                                  // 449
  "z"                                                   // 450
};

#if ENABLE_CDB_DEBUG > 1
static void printHexData(const unsigned char *p, size_t n)
{
  for (size_t i = 0; i < n; i++)
  {
    if (!(i & 15))
      std::printf("%08x", (unsigned int) i);
    std::printf("%s%02x", (!(i & 7) ? "  " : " "), (unsigned int) p[i]);
    if ((i & 15) != 15 && (i + 1) < n)
      continue;
    while ((i & 15) != 15)
    {
      i++;
      std::printf("%s", (!(i & 7) ? "    " : "   "));
    }
    std::printf("  |");
    i = i - 16;
    do
    {
      i++;
      unsigned char c = p[i];
      if (c < 0x20 || c >= 0x7F)
        c = 0x2E;
      std::fputc(c, stdout);
    }
    while ((i & 15) != 15 && (i + 1) < n);
    std::printf("|\n");
  }
}
#endif

