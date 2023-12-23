
#include "common.hpp"
#include "mat_json.hpp"

CDBMaterialToJSON::BSResourceID::BSResourceID(const std::string& fileName)
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
    // directory name
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
  ext = crcValue;
  crcValue = 0U;
  for ( ; i < extPos; i++)
  {
    // base name
    unsigned char c = (unsigned char) fileName.c_str()[i];
    if (c >= 0x41 && c <= 0x5A)         // 'A' - 'Z'
      c = c | 0x20;                     // convert to lower case
    hashFunctionCRC32(crcValue, c);
  }
  dir = crcValue;
  i++;
  if ((i + 3) <= fileName.length())
    file = FileBuffer::readUInt32Fast(fileName.c_str() + i);
  else if ((i + 2) <= fileName.length())
    file = FileBuffer::readUInt16Fast(fileName.c_str() + i);
  else if (i < fileName.length())
    file = (unsigned char) fileName.c_str()[i];
  else
    file = 0U;
  // convert extension to lower case
  file = file | ((file >> 1) & 0x20202020U);
}

inline const CDBMaterialToJSON::MaterialObject *
    CDBMaterialToJSON::findObject(std::uint32_t dbID) const
{
  if (!dbID) [[unlikely]]
    return nullptr;
  std::uint32_t h = 0xFFFFFFFFU;
  hashFunctionCRC32C< std::uint32_t >(h, dbID);
  size_t  m = objectsHashMap.size() - 2;
  size_t  k = h & m;
  while (true)
  {
    if (objectsHashMap[k] == dbID) [[likely]]
      return (objects.data() + objectsHashMap[k + 1]);
    if (!objectsHashMap[k])
      break;
    k = (k + 2) & m;
  }
  return nullptr;
}

inline CDBMaterialToJSON::MaterialObject *
    CDBMaterialToJSON::findObject(std::uint32_t dbID)
{
  const CDBMaterialToJSON *p = this;
  return const_cast< MaterialObject * >(p->findObject(dbID));
}

void * CDBMaterialToJSON::allocateSpace(size_t nBytes, size_t alignBytes)
{
  std::uintptr_t  addr0 =
      reinterpret_cast< std::uintptr_t >(objectBuffers.back().data());
  std::uintptr_t  endAddr = addr0 + objectBuffers.back().capacity();
  std::uintptr_t  addr =
      reinterpret_cast< std::uintptr_t >(&(*(objectBuffers.back().end())));
  std::uintptr_t  alignMask = std::uintptr_t(alignBytes - 1);
  addr = (addr + alignMask) & ~alignMask;
  std::uintptr_t  bytesRequired = (nBytes + alignMask) & ~alignMask;
  if ((endAddr - addr) < bytesRequired) [[unlikely]]
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
  return p;
}

CDBMaterialToJSON::CDBObject * CDBMaterialToJSON::allocateObject(
    std::uint32_t itemType, const CDBClassDef *classDef, size_t elementCnt)
{
  size_t  dataSize = sizeof(CDBObject);
  size_t  childCnt = 0;
  if (itemType == String_List || itemType == String_Map)
    childCnt = elementCnt;
  else if (itemType == String_Ref)
    childCnt = 1;
  else if (classDef)
    childCnt = classDef->fields.size();
  if (childCnt > 1)
    dataSize += ((childCnt - 1) * sizeof(CDBObject *));
  CDBObject *o =
      reinterpret_cast< CDBObject * >(allocateSpace(dataSize,
                                                    alignof(CDBObject)));
  o->type = itemType;
  o->childCnt = std::uint32_t(childCnt);
  if (childCnt)
  {
    for (size_t i = 0; i < childCnt; i++)
      o->data.children[i] = nullptr;
  }
  else if (itemType == String_String)
  {
    o->data.stringValue = "";
  }
  else
  {
    o->data.uint64Value = 0U;
  }
  return o;
}

void CDBMaterialToJSON::copyObject(CDBObject*& o)
{
  if (!o)
    return;
  size_t  dataSize = sizeof(CDBObject);
  if (o->childCnt > 1)
    dataSize += ((o->childCnt - 1) * sizeof(CDBObject *));
  CDBObject *p =
      reinterpret_cast< CDBObject * >(allocateSpace(dataSize,
                                                    alignof(CDBObject)));
  std::memcpy(p, o, dataSize);
  o = p;
  for (std::uint32_t i = 0U; i < p->childCnt; i++)
    copyObject(p->data.children[i]);
}

void CDBMaterialToJSON::copyBaseObject(MaterialObject& o)
{
  if (isRootObject(o.baseObject))
    return;
  MaterialObject  *p = findObject(o.baseObject);
  if (!p)
  {
    o.baseObject = 0U;
    return;
  }
  if (!isRootObject(p->baseObject))
    copyBaseObject(*p);
  o.components = p->components;
  o.baseObject = p->baseObject;
  for (std::map< std::uint64_t, CDBObject * >::iterator
           i = o.components.begin(); i != o.components.end(); i++)
  {
    copyObject(i->second);
  }
}

void CDBMaterialToJSON::loadItem(CDBObject*& o, CDBChunk& chunkBuf, bool isDiff,
                                 std::uint32_t itemType)
{
  const CDBClassDef *classDef = nullptr;
  if (itemType > String_Unknown)
  {
    std::map< std::uint32_t, CDBClassDef >::const_iterator
        i = classes.find(itemType);
    if (i != classes.end()) [[likely]]
      classDef = &(i->second);
    else
      itemType = String_Unknown;
  }
  if (!(o && o->type == itemType))
  {
    if (!(itemType == String_List || itemType == String_Map))
      o = allocateObject(itemType, classDef, 0);
  }
  if (itemType > String_Unknown)
  {
    unsigned int  nMax = (unsigned int) classDef->fields.size();
    if (classDef->isUser)
    {
      CDBChunk  userBuf;
      unsigned int  userChunkType = readChunk(userBuf);
      if (userChunkType != ChunkType_USER && userChunkType != ChunkType_USRD)
      {
        throw FO76UtilsError("unexpected chunk type in CDB file at 0x%08x",
                             (unsigned int) getPosition());
      }
      isDiff = (userChunkType == ChunkType_USRD);
      std::uint32_t className1 =
          std::uint32_t(findString(userBuf.readUInt32()));
      if (className1 != itemType)
      {
        throw FO76UtilsError("USER chunk has unexpected type at 0x%08x",
                             (unsigned int) getPosition());
      }
      std::uint32_t className2 =
          std::uint32_t(findString(userBuf.readUInt32()));
      if (className2 == className1)
      {
        // no type conversion
        nMax--;
        for (unsigned int n = 0U - 1U;
             userBuf.getFieldNumber(n, nMax, isDiff); )
        {
          loadItem(o->data.children[n], userBuf, isDiff,
                   classDef->fields[n].second);
        }
      }
      else if (className2 < String_Unknown)
      {
        if (!o->childCnt)
        {
          o->childCnt++;
          o->data.children[0] = nullptr;
        }
        unsigned int  n = 0U;
        do
        {
          loadItem(o->data.children[n], userBuf, isDiff, className2);
          className2 = std::uint32_t(findString(userBuf.readUInt32()));
          n++;
        }
        while (int(n) <= int(nMax) && className2 < String_Unknown);
      }
    }
    else
    {
      nMax--;
      for (unsigned int n = 0U - 1U; chunkBuf.getFieldNumber(n, nMax, isDiff); )
      {
        loadItem(o->data.children[n], chunkBuf, isDiff,
                 classDef->fields[n].second);
      }
    }
    return;
  }
  FileBuffer& buf2 = *(static_cast< FileBuffer * >(&chunkBuf));
  switch (itemType)
  {
    case String_String:
      {
        unsigned int  len = buf2.readUInt16();
        if ((buf2.getPosition() + std::uint64_t(len)) > buf2.size())
          errorMessage("unexpected end of CDB file");
        char    *s =
            reinterpret_cast< char * >(allocateSpace(len + 1U, sizeof(char)));
        std::memcpy(s, buf2.data() + buf2.getPosition(), len);
        s[len] = '\0';
        o->data.stringValue = s;
        buf2.setPosition(buf2.getPosition() + len);
      }
      break;
    case String_List:
    case String_Map:
      {
        CDBChunk  listBuf;
        unsigned int  chunkType = readChunk(listBuf);
        bool    isMap = (itemType == String_Map);
        if (chunkType != (!isMap ? ChunkType_LIST : ChunkType_MAPC))
        {
          throw FO76UtilsError("unexpected chunk type in CDB file at 0x%08x",
                               (unsigned int) getPosition());
        }
        std::uint32_t classNames[2];
        classNames[0] = std::uint32_t(findString(listBuf.readUInt32()));
        classNames[1] = classNames[0];
        if (isMap)
          classNames[1] = std::uint32_t(findString(listBuf.readUInt32()));
        std::uint32_t listSize = 0U;
        if ((listBuf.getPosition() + 4ULL) <= listBuf.size())
          listSize = listBuf.readUInt32();
        if (isMap)
        {
          if (listSize > 0x7FFFFFFFU)
            errorMessage("invalid map size in CDB file");
          listSize = listSize << 1;
        }
        std::uint32_t n = 0U;
        bool    appendingItems = false;
        if (isDiff && o && o->type == itemType && o->childCnt &&
            o->data.children[0]->type == classNames[0])
        {
          if (!isMap)
            appendingItems = (std::uint32_t(classNames[0] - String_Int8) > 11U);
          else
            appendingItems = (o->data.children[1]->type == classNames[1]);
        }
        if (appendingItems)
        {
          std::uint32_t prvSize = o->childCnt;
          if ((std::uint64_t(listSize) + prvSize) > 0xFFFFFFFFULL)
            errorMessage("invalid list size in CDB file");
          listSize = listSize + prvSize;
          CDBObject *p = allocateObject(itemType, classDef, listSize);
          for ( ; n < prvSize; n++)
            p->data.children[n] = o->data.children[n];
          o = p;
        }
        else if (!(o && o->type == itemType && o->childCnt >= listSize))
        {
          o = allocateObject(itemType, classDef, listSize);
        }
        o->childCnt = listSize;
        for ( ; n < listSize; n++)
        {
          loadItem(o->data.children[n], listBuf, isDiff, classNames[n & 1U]);
          if (isMap && !(n & 1U) && classNames[0] == String_String)
          {
            // check for duplicate keys
            for (std::uint32_t k = 0U; k < n; k = k + 2U)
            {
              if (std::strcmp(o->data.children[k]->data.stringValue,
                              o->data.children[n]->data.stringValue) == 0)
              {
                loadItem(o->data.children[k + 1U], listBuf, isDiff,
                         classNames[1]);
                listSize = listSize - 2U;
                o->childCnt = listSize;
                n--;
                break;
              }
            }
          }
        }
        return;
      }
      break;
    case String_Ref:
      {
        std::uint32_t refType = std::uint32_t(findString(buf2.readUInt32()));
        loadItem(o->data.children[0], chunkBuf, isDiff, refType);
        return;
      }
      break;
    case String_Int8:
      o->data.int8Value = std::int8_t(buf2.readUInt8());
      break;
    case String_UInt8:
      o->data.uint8Value = buf2.readUInt8();
      break;
    case String_Int16:
      o->data.int16Value = std::int16_t(buf2.readUInt16());
      break;
    case String_UInt16:
      o->data.uint16Value = buf2.readUInt16();
      break;
    case String_Int32:
      o->data.int32Value = buf2.readInt32();
      break;
    case String_UInt32:
      o->data.uint32Value = buf2.readUInt32();
      break;
    case String_Int64:
      o->data.int64Value = std::int64_t(buf2.readUInt64());
      break;
    case String_UInt64:
      o->data.uint64Value = buf2.readUInt64();
      break;
    case String_Bool:
      o->data.boolValue = bool(buf2.readUInt8());
      break;
    case String_Float:
      o->data.floatValue = buf2.readFloat();
      break;
    case String_Double:
      // FIXME: implement this in a portable way
      o->data.doubleValue =
          std::bit_cast< double, std::uint64_t >(buf2.readUInt64());
      break;
    default:
      chunkBuf.setPosition(chunkBuf.size());
      break;
  }
}

void CDBMaterialToJSON::readAllChunks()
{
  objectBuffers.emplace_back();
  objectBuffers.back().reserve(65536);
  CDBChunk  chunkBuf;
  unsigned int  chunkType;
  size_t  componentID = 0;
  size_t  componentCnt = 0;
  while ((chunkType = readChunk(chunkBuf)) != 0U)
  {
    if (chunkType == ChunkType_CLAS)
    {
      std::uint32_t className =
          std::uint32_t(findString(chunkBuf.readUInt32()));
      if (className < String_Unknown)
        errorMessage("invalid class ID in CDB file");
      if (className == String_Unknown)
        continue;
      (void) chunkBuf.readUInt32();     // classVersion
      unsigned int  classFlags = chunkBuf.readUInt16();
      (void) chunkBuf.readUInt16();     // fieldCnt
      CDBClassDef&  classDef = classes[className];
      classDef.isUser = bool(classFlags & 4U);
      while ((chunkBuf.getPosition() + 12ULL) <= chunkBuf.size())
      {
        std::uint32_t fieldName =
            std::uint32_t(findString(chunkBuf.readUInt32Fast()));
        if (fieldName < String_Unknown)
          errorMessage("invalid field name in class definition in CDB file");
        std::uint32_t fieldType =
            std::uint32_t(findString(chunkBuf.readUInt32Fast()));
        (void) chunkBuf.readUInt16Fast();       // dataOffset
        (void) chunkBuf.readUInt16Fast();       // dataSize
        classDef.fields.emplace_back(fieldName, fieldType);
      }
      continue;
    }
    std::uint32_t className = String_None;
    if (chunkType != ChunkType_TYPE && chunkBuf.size() >= 4) [[likely]]
      className = std::uint32_t(findString(chunkBuf.readUInt32Fast()));
    if (chunkType == ChunkType_DIFF || chunkType == ChunkType_OBJT) [[likely]]
    {
      bool    isDiff = (chunkType == ChunkType_DIFF);
      if (componentID < componentCnt) [[likely]]
      {
        std::uint32_t dbID = componentInfo[componentID].first;
        std::uint32_t componentIndex = componentInfo[componentID].second;
        MaterialObject  *i = findObject(dbID);
        if (i && className > String_Unknown)
        {
          if (!isRootObject(i->baseObject))
            copyBaseObject(*i);
          std::uint64_t k = (std::uint64_t(className) << 32) | componentIndex;
          loadItem(i->components[k], chunkBuf, isDiff, className);
        }
        componentID++;
      }
      continue;
    }
    if (chunkType != ChunkType_LIST)
      continue;
    std::uint32_t n = 0U;
    if (chunkBuf.size() >= 8)
      n = chunkBuf.readUInt32Fast();
    if (className == String_BSComponentDB2_DBFileIndex_ObjectInfo)
    {
      if (n > ((chunkBuf.size() - chunkBuf.getPosition()) / 21))
        errorMessage("unexpected end of LIST chunk in material database");
      size_t  m = 16;
      while (m < (size_t(n) << 2))
        m = m << 1;
      objectsHashMap.clear();
      objectsHashMap.resize(m, 0U);
      objects.clear();
      objects.resize(n);
      m = m - 2;
      for (std::uint32_t i = 0U; i < n; i++)
      {
        BSResourceID  persistentID;
        persistentID.dir = chunkBuf.readUInt32();
        persistentID.file = chunkBuf.readUInt32();
        persistentID.ext = chunkBuf.readUInt32();
        std::uint32_t dbID = chunkBuf.readUInt32();
        if (!dbID)
          errorMessage("invalid object ID in material database");
        std::uint32_t h = 0xFFFFFFFFU;
        hashFunctionCRC32C< std::uint32_t >(h, dbID);
        size_t  k = h & m;
        for ( ; objectsHashMap[k]; k = (k + 2) & m)
        {
          if (objectsHashMap[k] == dbID)
            errorMessage("duplicate object ID in material database");
        }
        objectsHashMap[k] = dbID;
        objectsHashMap[k + 1] = i;
        MaterialObject& o = objects[i];
        o.persistentID = persistentID;
        o.dbID = dbID;
        o.baseObject = chunkBuf.readUInt32();
        (void) static_cast< FileBuffer * >(&chunkBuf)->readUInt8(); // HasData
        o.parent = nullptr;
      }
    }
    else if (className == String_BSComponentDB2_DBFileIndex_ComponentInfo)
    {
      componentID = 0;
      componentCnt = 0;
      while (n--)
      {
        std::uint32_t dbID = chunkBuf.readUInt32();
        std::uint32_t componentIndex = chunkBuf.readUInt16();
        (void) chunkBuf.readUInt16();   // Type
        componentInfo.emplace_back(dbID, componentIndex);
        componentCnt++;
      }
    }
    else if (className == String_BSComponentDB2_DBFileIndex_EdgeInfo)
    {
      while (n--)
      {
        std::uint32_t sourceID = chunkBuf.readUInt32();
        std::uint32_t targetID = chunkBuf.readUInt32();
        (void) chunkBuf.readUInt16();   // Index
        (void) chunkBuf.readUInt16();   // Type
        MaterialObject  *o = findObject(sourceID);
        MaterialObject  *p = findObject(targetID);
        if (o && p)
          o->parent = p;
      }
    }
  }
  for (std::vector< MaterialObject >::iterator
           i = objects.begin(); i != objects.end(); i++)
  {
    MaterialObject  *p = &(*i);
    if (!isRootObject(p->baseObject))
      copyBaseObject(*p);
    while (p->parent)
      p = const_cast< MaterialObject * >(p->parent);
    if (p->persistentID.file == 0x0074616DU)    // "mat\0"
      matFileObjectMap[p->persistentID].push_back(i->dbID);
  }
  for (std::map< BSResourceID, std::vector< std::uint32_t > >::iterator
           i = matFileObjectMap.begin(); i != matFileObjectMap.end(); i++)
  {
    if (i->second.size() > 1)
      std::sort(i->second.begin(), i->second.end());
  }
}

void CDBMaterialToJSON::dumpObject(
    std::string& s, const CDBObject *o, int indentCnt) const
{
  if (!o)
  {
    printToString(s, "null");
    return;
  }
  if (o->type > String_Unknown)
  {
    // structure
    if (o->type == String_BSComponentDB2_ID) [[unlikely]]
    {
      const MaterialObject  *p = nullptr;
      if (o->childCnt && o->data.children[0] &&
          o->data.children[0]->type == String_UInt32)
      {
        p = findObject(o->data.children[0]->data.uint32Value);
      }
      if (!p)
      {
        printToString(s, "\"\"");
      }
      else
      {
        printToString(s, "\"res:%08X:%08X:%08X\"",
                      (unsigned int) p->persistentID.ext,
                      (unsigned int) p->persistentID.dir,
                      (unsigned int) p->persistentID.file);
      }
      return;
    }
    printToString(s, "{\n%*s\"Data\": {", indentCnt + 2, "");
    std::map< std::uint32_t, CDBClassDef >::const_iterator  i =
        classes.find(o->type);
    for (std::uint32_t fieldNum = 0U; fieldNum < o->childCnt; fieldNum++)
    {
      if (!o->data.children[fieldNum])
        continue;
      const char  *fieldNameStr = "null";
      if (i != classes.end() && fieldNum < i->second.fields.size())
        fieldNameStr = stringTable[i->second.fields[fieldNum].first];
      printToString(s, "\n%*s\"%s\": ", indentCnt + 4, "", fieldNameStr);
      dumpObject(s, o->data.children[fieldNum], indentCnt + 4);
      printToString(s, ",");
    }
    if (s.ends_with(','))
    {
      s[s.length() - 1] = '\n';
      printToString(s, "%*s", indentCnt + 2, "");
    }
    printToString(s, "},\n%*s\"Type\": \"%s\"\n",
                  indentCnt + 2, "", stringTable[o->type]);
    printToString(s, "%*s}", indentCnt, "");
    return;
  }
  switch (o->type)
  {
    case String_None:
      printToString(s, "null");
      break;
    case String_String:
      s += '"';
      for (size_t i = 0; o->data.stringValue[i]; i++)
      {
        char    c = o->data.stringValue[i];
        if ((unsigned char) c < 0x20 || c == '"' || c == '\\')
        {
          s += '\\';
          switch (c)
          {
            case '\b':
              c = 'b';
              break;
            case '\t':
              c = 't';
              break;
            case '\n':
              c = 'n';
              break;
            case '\f':
              c = 'f';
              break;
            case '\r':
              c = 'r';
              break;
            default:
              if ((unsigned char) c < 0x20)
              {
                printToString(s, "u%04X", (unsigned int) c);
                continue;
              }
              break;
          }
        }
        s += c;
      }
      s += '"';
      break;
    case String_List:
      printToString(s, "{\n%*s\"Data\": [", indentCnt + 2, "");
      for (std::uint32_t i = 0U; i < o->childCnt; i++)
      {
        printToString(s, "\n%*s", indentCnt + 4, "");
        dumpObject(s, o->data.children[i], indentCnt + 4);
        if ((i + 1U) < o->childCnt)
          printToString(s, ",");
        else
          printToString(s, "\n%*s", indentCnt + 2, "");
      }
      printToString(s, "],\n");
      if (o->childCnt)
      {
        const char  *elementType = "null";
        if (o->data.children[0])
        {
          if (o->data.children[0]->type == String_String)
            elementType = "BSFixedString";
          else if (o->data.children[0]->type == String_Float)
            elementType = "float";
          else
            elementType = stringTable[o->data.children[0]->type];
        }
        printToString(s, "%*s\"ElementType\": \"%s\",\n",
                      indentCnt + 2, "", elementType);
      }
      printToString(s, "%*s\"Type\": \"<collection>\"\n", indentCnt + 2, "");
      printToString(s, "%*s}", indentCnt, "");
      break;
    case String_Map:
      printToString(s, "{\n%*s\"Data\": [", indentCnt + 2, "");
      for (std::uint32_t i = 0U; i < o->childCnt; i++)
      {
        printToString(s, "\n%*s{", indentCnt + 4, "");
        printToString(s, "\n%*s\"Data\": {", indentCnt + 6, "");
        printToString(s, "\n%*s\"Key\": ", indentCnt + 8, "");
        dumpObject(s, o->data.children[i], indentCnt + 8);
        i++;
        printToString(s, ",\n%*s\"Value\": ", indentCnt + 8, "");
        dumpObject(s, o->data.children[i], indentCnt + 8);
        printToString(s, "\n%*s},", indentCnt + 6, "");
        printToString(s, "\n%*s\"Type\": \"StdMapType::Pair\"\n",
                      indentCnt + 6, "");
        printToString(s, "%*s}", indentCnt + 4, "");
        if ((i + 1U) < o->childCnt)
          printToString(s, ",");
        else
          printToString(s, "\n%*s", indentCnt + 2, "");
      }
      printToString(s, "],\n%*s\"ElementType\": \"StdMapType::Pair\",\n",
                    indentCnt + 2, "");
      printToString(s, "%*s\"Type\": \"<collection>\"\n", indentCnt + 2, "");
      printToString(s, "%*s}", indentCnt, "");
      break;
    case String_Ref:
      printToString(s, "{\n%*s\"Data\": ", indentCnt + 2, "");
      dumpObject(s, o->data.children[0], indentCnt + 2);
      printToString(s, ",\n%*s\"Type\": \"<ref>\"\n", indentCnt + 2, "");
      printToString(s, "%*s}", indentCnt, "");
      break;
    case String_Int8:
      printToString(s, "%d", int(o->data.int8Value));
      break;
    case String_UInt8:
      printToString(s, "%u", (unsigned int) o->data.uint8Value);
      break;
    case String_Int16:
      printToString(s, "%d", int(o->data.int16Value));
      break;
    case String_UInt16:
      printToString(s, "%u", (unsigned int) o->data.uint16Value);
      break;
    case String_Int32:
      printToString(s, "%d", int(o->data.int32Value));
      break;
    case String_UInt32:
      printToString(s, "%u", (unsigned int) o->data.uint32Value);
      break;
    case String_Int64:
      printToString(s, "\"%lld\"", (long long) o->data.int64Value);
      break;
    case String_UInt64:
      printToString(s, "\"%llu\"", (unsigned long long) o->data.uint64Value);
      break;
    case String_Bool:
      printToString(s, "%s", (!o->data.boolValue ? "false" : "true"));
      break;
    case String_Float:
      printToString(s, "%.7g", o->data.floatValue);
      break;
    case String_Double:
      printToString(s, "%.14g", o->data.doubleValue);
      break;
    default:
      printToString(s, "\"<unknown>\"");
      break;
  }
}

void CDBMaterialToJSON::dumpMaterial(
    std::string& jsonBuf, const std::string& materialPath) const
{
  jsonBuf.clear();
  if (materialPath.empty())
    return;
  const std::vector< std::uint32_t >  *objectList;
  {
    std::map< BSResourceID, std::vector< std::uint32_t > >::const_iterator  i =
        matFileObjectMap.find(BSResourceID(materialPath));
    if (i == matFileObjectMap.end())
      return;
    objectList = &(i->second);
  }
  jsonBuf = "{\n  \"Objects\": [\n";
  for (std::vector< std::uint32_t >::const_iterator
           k = objectList->begin(); k != objectList->end(); k++)
  {
    const MaterialObject  *i = findObject(*k);
    if (!i)
      continue;
    printToString(jsonBuf, "    {\n      \"Components\": [\n");
    for (std::map< std::uint64_t, CDBObject * >::const_iterator
             j = i->components.begin(); j != i->components.end(); j++)
    {
      jsonBuf += "        ";
      dumpObject(jsonBuf, j->second, 8);
      if (jsonBuf.ends_with("\n        }"))
      {
        jsonBuf.resize(jsonBuf.length() - 10);
        printToString(jsonBuf, ",\n          \"Index\": %u\n",
                      (unsigned int) (j->first & 0xFFFFFFFFU));
        printToString(jsonBuf, "        },\n");
      }
    }
    if (jsonBuf.ends_with(",\n"))
    {
      jsonBuf.resize(jsonBuf.length() - 1);
      jsonBuf[jsonBuf.length() - 1] = '\n';
    }
    printToString(jsonBuf, "      ],\n");
    printToString(jsonBuf, "      \"ID\": \"res:%08X:%08X:%08X\",\n",
                  (unsigned int) i->persistentID.ext,
                  (unsigned int) i->persistentID.dir,
                  (unsigned int) i->persistentID.file);
    const char  *parentStr = "";
    const MaterialObject  *j;
    if (i->baseObject && (j = findObject(i->baseObject)) != nullptr)
    {
      if (j->persistentID.dir == 0x7EA3660CU)
        parentStr = "materials\\\\layered\\\\root\\\\layeredmaterials.mat";
      else if (j->persistentID.dir == 0x8EBE84FFU)
        parentStr = "materials\\\\layered\\\\root\\\\blenders.mat";
      else if (j->persistentID.dir == 0x574A4CF3U)
        parentStr = "materials\\\\layered\\\\root\\\\layers.mat";
      else if (j->persistentID.dir == 0x7D1E021BU)
        parentStr = "materials\\\\layered\\\\root\\\\materials.mat";
      else if (j->persistentID.dir == 0x06F52154U)
        parentStr = "materials\\\\layered\\\\root\\\\texturesets.mat";
      else if (j->persistentID.dir == 0x4298BB09U)
        parentStr = "materials\\\\layered\\\\root\\\\uvstreams.mat";
    }
    printToString(jsonBuf, "      \"Parent\": \"%s\"\n    },\n", parentStr);
  }
  jsonBuf.resize(jsonBuf.length() - 2);
  jsonBuf += "\n  ],\n  \"Version\": 1\n}";
}

