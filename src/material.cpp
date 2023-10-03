
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
  const std::string *emptyString = stringBuffers.front().data();
  switch (o->type)
  {
    case 1:
      {
        CE2Material *p = static_cast< CE2Material * >(o);
        p->flags = 0U;
        p->layerMask = 0U;
        for (size_t i = 0; i < CE2Material::maxLayers; i++)
          p->layers[i] = (CE2Material::Layer *) 0;
        p->alphaThreshold = 1.0f / 3.0f;
        p->alphaSourceLayer = 0;        // "MATERIAL_LAYER_0"
        p->alphaBlendMode = 0;          // "Linear"
        p->alphaVertexColorChannel = 0; // "Red"
        p->alphaHeightBlendThreshold = 0.0f;
        p->alphaHeightBlendFactor = 0.05f;
        p->alphaPosition = 0.5f;
        p->alphaContrast = 0.0f;
        p->alphaUVStream = (CE2Material::UVStream *) 0;
        p->opacityLayer1 = 0;           // "MATERIAL_LAYER_0"
        p->opacityLayer2 = 1;           // "MATERIAL_LAYER_1"
        p->opacityBlender1 = 0;         // "BLEND_LAYER_0"
        p->opacityBlender1Mode = 0;     // "Lerp"
        p->opacityLayer3 = 2;           // "MATERIAL_LAYER_2"
        p->opacityBlender2 = 1;         // "BLEND_LAYER_1"
        p->opacityBlender2Mode = 0;     // "Lerp"
        p->specularOpacityOverride = 0.0f;
        for (size_t i = 0; i < CE2Material::maxBlenders; i++)
          p->blenders[i] = (CE2Material::Blender *) 0;
        for (size_t i = 0; i < CE2Material::maxLODMaterials; i++)
          p->lodMaterials[i] = (CE2Material *) 0;
        p->effectSettings = (CE2Material::EffectSettings *) 0;
        p->emissiveSettings = (CE2Material::EmissiveSettings *) 0;
        p->translucencySettings = (CE2Material::TranslucencySettings *) 0;
        p->decalSettings = (CE2Material::DecalSettings *) 0;
        p->waterSettings = (CE2Material::WaterSettings *) 0;
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
        p->textureReplacementMask = 0U;
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

void * CE2MaterialDB::allocateSpace(
    size_t nBytes, const void *copySrc, size_t alignBytes)
{
  std::uintptr_t  addr0 =
      reinterpret_cast< std::uintptr_t >(objectBuffers.back().data());
  std::uintptr_t  endAddr = addr0 + objectBuffers.back().capacity();
  std::uintptr_t  addr =
      reinterpret_cast< std::uintptr_t >(&(*(objectBuffers.back().end())));
  std::uintptr_t  alignMask = std::uintptr_t(alignBytes - 1);
  addr = (addr + alignMask) & ~alignMask;
  std::uintptr_t  bytesRequired = (nBytes + alignMask) & ~alignMask;
  if (BRANCH_UNLIKELY((endAddr - addr) < bytesRequired))
  {
    std::uintptr_t  bufBytes = 65536U;
    if (bytesRequired > bufBytes)
      throw std::bad_alloc();
    objectBuffers.emplace_back();
    objectBuffers.back().reserve(size_t(bufBytes));
    addr0 = reinterpret_cast< std::uintptr_t >(objectBuffers.back().data());
    addr = (addr0 + alignMask) & ~alignMask;
  }
  objectBuffers.back().resize(size_t((addr + bytesRequired) - addr0));
  unsigned char *p = reinterpret_cast< unsigned char * >(addr);
  if (BRANCH_UNLIKELY(copySrc))
    std::memcpy(p, copySrc, nBytes);
  return p;
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
  static const size_t objectSizeTable[8] =
  {
    sizeof(CE2MaterialObject), sizeof(CE2Material),
    sizeof(CE2Material::Blender), sizeof(CE2Material::Layer),
    sizeof(CE2Material::Material), sizeof(CE2Material::TextureSet),
    sizeof(CE2Material::UVStream), sizeof(CE2MaterialObject)
  };
  o = reinterpret_cast< CE2MaterialObject * >(
          allocateSpace(objectSizeTable[type]));
  objectTable[objectID] = o;
  o->type = type | std::int32_t(~baseObjID << 8);
  o->e = 0U;
  o->h = 0U;
  o->name = stringBuffers.front().data();
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
    return stringBuffers.front().data();
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

#if ENABLE_CDB_DEBUG
static const char *predefinedCDBStrings[16] =
{
  "String",       "",             "",             "",           // -254 to -251
  "",             "",             "",             "Int8",       // -250 to -247
  "",             "Int16",        "",             "Int32",      // -246 to -243
  "",             "",             "Bool",         "Float"       // -242 to -239
};
#endif

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
    if (chunkType == 0x53414C43U)               // "CLAS"
    {
      buf2.setPosition(buf2.getPosition() + 6);
      while ((buf2.getPosition() + 12ULL) <= buf2.size())
      {
        unsigned int  unknown1 = buf2.readUInt16Fast();
        const char    *fieldName = "???";
        int     tmp = findString(buf2.readUInt32Fast());
        if (tmp >= 0)
          fieldName = stringTable[tmp];
        const char    *fieldType = "???";
        tmp = std::int32_t(buf2.readUInt32Fast());
        char    tmpBuf[32];
        std::sprintf(tmpBuf, "%d", tmp);
        if (tmp >= -254 && tmp <= -239)
          fieldType = predefinedCDBStrings[tmp + 254];
        else if (tmp < 0)
          fieldType = tmpBuf;
        else if ((tmp = findString((unsigned int) tmp)) >= 0)
          fieldType = stringTable[tmp];
        unsigned int  dataOffset = buf2.readUInt16Fast();
        std::printf("    0x%04X, %s, %s, 0x%04X\n",
                    unknown1, fieldName, fieldType, dataOffset);
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
            if (!((objectNameMap[h]->type ^ o->type) & 0xFF))
              break;
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
  ComponentInfo componentInfo(*this, objectTable);
  for (size_t componentID = 0; componentID < componentCnt; )
  {
    if ((buf.getPosition() + 8ULL) > buf.size())
      errorMessage("unexpected end of material database file");
    unsigned int  chunkType = buf.readUInt32Fast();
    unsigned int  chunkSize = buf.readUInt32Fast();
    if ((buf.getPosition() + std::uint64_t(chunkSize)) > buf.size())
      errorMessage("unexpected end of material database file");
    (void) new(&(componentInfo.buf))
        FileBuffer(buf.data() + buf.getPosition(), chunkSize, 4);
    buf.setPosition(buf.getPosition() + chunkSize);
    if (BRANCH_UNLIKELY(chunkType != 0x46464944U && chunkType != 0x544A424FU))
      continue;                                 // not "DIFF" or "OBJT"
    bool    isDiff = (chunkType != 0x544A424FU);
    const unsigned char *cmpInfoPtr = componentInfoPtr + (componentID << 3);
    unsigned int  objectID = FileBuffer::readUInt32Fast(cmpInfoPtr);
    componentInfo.componentIndex = FileBuffer::readUInt16Fast(cmpInfoPtr + 4);
    componentInfo.componentType = FileBuffer::readUInt16Fast(cmpInfoPtr + 6);
    componentID++;
    componentInfo.o =
        const_cast< CE2MaterialObject * >(findObject(objectTable, objectID));
    if (BRANCH_UNLIKELY(!componentInfo.o))
      continue;
    if (BRANCH_UNLIKELY(componentInfo.o->type < 0))
      initializeObject(componentInfo.o, objectTable);
    if (BRANCH_UNLIKELY(chunkSize < 4U))
      continue;
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
                chunkSize, objectID, componentInfo.o->type);
    int     t =
        findString(FileBuffer::readUInt32Fast(componentInfo.buf.data()));
    if (t < 0)
    {
      std::printf("component 0x%04X slot %3u\n",
                  componentInfo.componentType, componentInfo.componentIndex);
    }
    else
    {
      std::printf("component 0x%04X (%s) slot %3u\n",
                  componentInfo.componentType, stringTable[t],
                  componentInfo.componentIndex);
    }
#  if ENABLE_CDB_DEBUG > 1
    printHexData(componentInfo.buf.data() + 4, componentInfo.buf.size() - 4);
#  endif
#endif
    if (!componentInfo.o)
    {
#if ENABLE_CDB_DEBUG
      std::printf("Warning: invalid object ID in material database\n");
#endif
      continue;
    }
    unsigned int  n = componentInfo.componentType - 0x0060U;
    if ((n & ~0x3FU) || !ComponentInfo::readFunctionTable[n])
    {
#if ENABLE_CDB_DEBUG
      std::printf("Warning: unrecognized component type 0x%04X\n", n + 0x0060U);
#endif
      continue;
    }
    ComponentInfo::readFunctionTable[n](componentInfo, isDiff);
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
  while (true)
  {
    size_t  len = 0;
    while (fileName[len] != ',' && fileName[len] != '\0')
      len++;
    const unsigned char *buf = (unsigned char *) 0;
    size_t  bufSize =
        ba2File.extractFile(buf, fileBuf, std::string(fileName, len));
    FileBuffer  tmpBuf(buf, bufSize);
    loadCDBFile(tmpBuf);
    if (!fileName[len])
      break;
    fileName = fileName + (len + 1);
  }
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

