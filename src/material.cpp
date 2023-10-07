
#include "common.hpp"
#include "material.hpp"

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
  std::uint32_t crcValue = 0U;
  if (baseNamePos != std::string::npos)
  {
    for ( ; i < baseNamePos; i++)
    {
      unsigned char c = (unsigned char) fileName.c_str()[i];
      if (c >= 0x41 && c <= 0x5A)       // 'A' - 'Z'
        c = c | 0x20;                   // convert to lower case
      else if (c == 0x2F)               // '/'
        c = 0x5C;                       // convert to '\\'
      hashFunctionCRC32(crcValue, c);
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
    hashFunctionCRC32(crcValue, c);
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
        p->shaderModel = 31;            // "BaseMaterial"
        p->alphaSourceLayer = 0;        // "MATERIAL_LAYER_0"
        p->alphaBlendMode = 0;          // "Linear"
        p->alphaVertexColorChannel = 0; // "Red"
        p->alphaHeightBlendThreshold = 0.0f;
        p->alphaHeightBlendFactor = 0.05f;
        p->alphaPosition = 0.5f;
        p->alphaContrast = 0.0f;
        p->alphaUVStream = (CE2Material::UVStream *) 0;
        p->shaderRoute = 0;             // "Deferred"
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
        p->layeredEmissiveSettings = (CE2Material::LayeredEmissiveSettings *) 0;
        p->translucencySettings = (CE2Material::TranslucencySettings *) 0;
        p->decalSettings = (CE2Material::DecalSettings *) 0;
        p->vegetationSettings = (CE2Material::VegetationSettings *) 0;
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

void CE2MaterialDB::loadCDBFile(CDBFile& buf)
{
  ComponentInfo componentInfo(*this, buf);
  const unsigned char *componentInfoPtr = (unsigned char *) 0;
  size_t  componentID = 0;
  size_t  componentCnt = 0;
  buf.setPosition(12);
  for (unsigned int chunkCnt = buf.readUInt32Fast(); chunkCnt > 1U; chunkCnt--)
  {
    if ((buf.getPosition() + 8ULL) > buf.size())
      errorMessage("unexpected end of material database file");
    unsigned int  chunkType = buf.readUInt32Fast();
    unsigned int  chunkSize = buf.readUInt32Fast();
    if ((buf.getPosition() + std::uint64_t(chunkSize)) > buf.size())
      errorMessage("unexpected end of material database file");
#if ENABLE_CDB_DEBUG
    char    chunkTypeStr[8];
    chunkTypeStr[0] = char(chunkType & 0x7FU);
    chunkTypeStr[1] = char((chunkType >> 8) & 0x7FU);
    chunkTypeStr[2] = char((chunkType >> 16) & 0x7FU);
    chunkTypeStr[3] = char((chunkType >> 24) & 0x7FU);
    chunkTypeStr[4] = '\0';
    size_t  t = CDBFile::String_Unknown;
    if (chunkSize >= 4U)
    {
      t = buf.findString(FileBuffer::readUInt32Fast(
                             buf.data() + buf.getPosition()));
    }
    std::printf("%s (%s) at 0x%08X, size = %u bytes",
                chunkTypeStr, CDBFile::stringTable[t],
                (unsigned int) buf.getPosition() - 8U, chunkSize);
#endif
    (void) new(&(componentInfo.buf)) FileBuffer(buf.data() + buf.getPosition(),
                                                chunkSize, 4);
    buf.setPosition(buf.getPosition() + chunkSize);
    if (BRANCH_LIKELY(chunkType == 0x46464944U || chunkType == 0x544A424FU))
    {                                           // "DIFF" or "OBJT"
      if (BRANCH_UNLIKELY(componentID >= componentCnt))
      {
#if ENABLE_CDB_DEBUG
        std::fputc('\n', stdout);
#endif
        continue;
      }
      // read component
      bool    isDiff = (chunkType != 0x544A424FU);
      const unsigned char *cmpInfoPtr = componentInfoPtr + (componentID << 3);
      unsigned int  objectID = FileBuffer::readUInt32Fast(cmpInfoPtr);
      componentInfo.componentIndex = FileBuffer::readUInt16Fast(cmpInfoPtr + 4);
      componentInfo.componentType = FileBuffer::readUInt16Fast(cmpInfoPtr + 6);
      componentID++;
      componentInfo.o = const_cast< CE2MaterialObject * >(
                            findObject(componentInfo.objectTable, objectID));
      if (BRANCH_UNLIKELY(!componentInfo.o))
      {
#if ENABLE_CDB_DEBUG
        std::fputc('\n', stdout);
#endif
        continue;
      }
      if (BRANCH_UNLIKELY(componentInfo.o->type < 0))
        initializeObject(componentInfo.o, componentInfo.objectTable);
      if (BRANCH_UNLIKELY(chunkSize < 4U))
      {
#if ENABLE_CDB_DEBUG
        std::fputc('\n', stdout);
#endif
        continue;
      }
#if ENABLE_CDB_DEBUG
      std::printf(", object = 0x%08X:%d, component 0x%04X[%u]\n",
                  objectID, componentInfo.o->type,
                  componentInfo.componentType, componentInfo.componentIndex);
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
        std::printf("Warning: unrecognized component type 0x%04X\n",
                    n + 0x0060U);
#endif
        continue;
      }
      ComponentInfo::readFunctionTable[n](componentInfo, isDiff);
      continue;
    }
#if ENABLE_CDB_DEBUG
    std::fputc('\n', stdout);
#endif
    FileBuffer& buf2 = componentInfo.buf;
    if (BRANCH_UNLIKELY(chunkType == 0x54525453U))      // "STRT" (string table)
    {
      if ((buf.getPosition() - chunkSize) != 24U)
        errorMessage("duplicate string table in material database");
      continue;
    }
#if !ENABLE_CDB_DEBUG
    size_t  t = CDBFile::String_Unknown;
    if (chunkSize >= 4U)
      t = buf.findString(FileBuffer::readUInt32Fast(buf2.data()));
#else
    if (chunkType == 0x4350414DU)               // "MAPC"
    {
      size_t  t2 = buf.findString(buf2.readUInt32());
      unsigned int  n = buf2.readUInt32();
      std::printf("  < %s, %s >[%u]\n",
                  CDBFile::stringTable[t], CDBFile::stringTable[t2], n);
      if (t == CDBFile::String_BSResource_ID)
      {
        for ( ; n; n--)
        {
          // hash of the base name without extension
          std::printf("    0x%08X", buf2.readUInt32());
          // extension ("mat\0")
          std::printf(", 0x%08X", buf2.readUInt32());
          // hash of the directory name
          std::printf(", 0x%08X", buf2.readUInt32());
          // unknown hash
          std::printf("  0x%016llX\n", (unsigned long long) buf2.readUInt64());
        }
      }
      continue;
    }
    if (chunkType == 0x53414C43U)               // "CLAS"
    {
      buf2.setPosition(buf2.getPosition() + 8);
      while ((buf2.getPosition() + 12ULL) <= buf2.size())
      {
        const char    *fieldName =
            CDBFile::stringTable[buf.findString(buf2.readUInt32Fast())];
        const char    *fieldType =
            CDBFile::stringTable[buf.findString(buf2.readUInt32Fast())];
        unsigned int  dataOffset = buf2.readUInt16Fast();
        unsigned int  dataSize = buf2.readUInt16Fast();
        std::printf("    %s, %s, 0x%04X, 0x%04X\n",
                    fieldName, fieldType, dataOffset, dataSize);
      }
      continue;
    }
#endif
    if (chunkType != 0x5453494CU)               // not "LIST"
    {
#if ENABLE_CDB_DEBUG
      if (chunkType == 0x52455355U || chunkType == 0x44525355U)
      {                                         // "USER" or "USRD"
        size_t  t2 = buf.findString(buf2.readUInt32());
        std::printf("  %s : %s\n",
                    CDBFile::stringTable[t], CDBFile::stringTable[t2]);
#  if ENABLE_CDB_DEBUG > 1
        printHexData(buf2.data() + buf2.getPosition(),
                     buf2.size() - buf2.getPosition());
#  endif
      }
#endif
      continue;
    }
    unsigned int  n = buf2.readUInt32();
#if ENABLE_CDB_DEBUG
    std::printf("  %s[%u]\n", CDBFile::stringTable[t], n);
#endif
    if (t == CDBFile::String_BSComponentDB2_DBFileIndex_ObjectInfo)
    {
      if ((buf2.getPosition() + (std::uint64_t(n) * 21UL)) > buf2.size())
        errorMessage("unexpected end of LIST chunk in material database");
      if (componentInfo.objectTable.begin() != componentInfo.objectTable.end())
        errorMessage("duplicate ObjectInfo list in material database");
      const unsigned char *objectInfoPtr = buf2.data() + buf2.getPosition();
      unsigned int  objectCnt = n;
      std::uint32_t maxObjID = 0U;
      for (const unsigned char *p = objectInfoPtr; n; n--, p = p + 21)
        maxObjID = std::max(maxObjID, FileBuffer::readUInt32Fast(p + 12));
      if (maxObjID > 0x007FFFFFU)
        errorMessage("object ID is out of range in material database");
      componentInfo.objectTable.resize(size_t(maxObjID) + 1,
                                       (CE2MaterialObject *) 0);
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
        CE2MaterialObject *o =
            allocateObject(componentInfo.objectTable, objectID, baseObjID);
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
    else if (t == CDBFile::String_BSComponentDB2_DBFileIndex_ComponentInfo)
    {
      if ((buf2.getPosition() + (std::uint64_t(n) << 3)) > buf2.size())
        errorMessage("unexpected end of LIST chunk in material database");
      if (componentInfoPtr)
        errorMessage("duplicate ComponentInfo list in material database");
      componentInfoPtr = buf2.data() + buf2.getPosition();
      componentCnt = n;
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
    else if (t == CDBFile::String_BSComponentDB2_DBFileIndex_EdgeInfo)
    {
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
        const CE2MaterialObject *o =
            findObject(componentInfo.objectTable, childObjectID);
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
            findObject(componentInfo.objectTable, parentObjectID);
      }
    }
  }
  for (std::vector< CE2MaterialObject * >::iterator
           i = componentInfo.objectTable.begin();
       i != componentInfo.objectTable.end(); i++)
  {
    if (*i && (*i)->type < 0)
      initializeObject(*i, componentInfo.objectTable);
  }
}

void CE2MaterialDB::loadCDBFile(const unsigned char *buf, size_t bufSize)
{
  CDBFile   tmpBuf(buf, bufSize);
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
    CDBFile   tmpBuf(buf, bufSize);
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

