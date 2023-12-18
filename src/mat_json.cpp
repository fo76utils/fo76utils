
#include "common.hpp"
#include "mat_json.hpp"

void CDBMaterialToJSON::copyBaseObject(MaterialObject& o)
{
  if (isRootObject(o.baseObject))
    return;
  std::map< std::uint32_t, MaterialObject >::iterator p =
      objects.find(o.baseObject);
  if (p == objects.end())
  {
    o.baseObject = 0U;
    return;
  }
  if (!isRootObject(p->second.baseObject))
    copyBaseObject(p->second);
  o.components = p->second.components;
  o.baseObject = p->second.baseObject;
}

void CDBMaterialToJSON::loadItem(CDBObject& o, CDBChunk& chunkBuf, bool isDiff,
                                 std::uint32_t itemType)
{
  o.type = itemType;
  o.value.clear();
  if (itemType == String_Unknown)
  {
    chunkBuf.setPosition(chunkBuf.size());
    return;
  }
  if (itemType > String_Unknown)
  {
    std::map< std::uint32_t, CDBClassDef >::const_iterator
        i = classes.find(itemType);
    if (i == classes.end())
    {
      chunkBuf.setPosition(chunkBuf.size());
      return;
    }
    unsigned int  nMax = (unsigned int) i->second.fields.size();
    if (i->second.isUser)
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
          loadItem(o.children[std::uint32_t(n)], userBuf, isDiff,
                   i->second.fields[n].second);
        }
      }
      else
      {
        for (unsigned int n = 0U; className2 < String_Unknown; )
        {
          loadItem(o.children[std::uint32_t(n)], userBuf, isDiff,
                   className2);
          className2 = std::uint32_t(findString(userBuf.readUInt32()));
          if (++n >= nMax)
            break;
        }
      }
    }
    else
    {
      nMax--;
      unsigned int  n = 0U - 1U;
      if (itemType == String_BSComponentDB2_ID) [[unlikely]]
      {
        o.type = String_String;
        o.children.clear();
        while (chunkBuf.getFieldNumber(n, nMax, isDiff))
        {
          std::uint32_t dbID = chunkBuf.readUInt32();
          std::map< std::uint32_t, MaterialObject >::const_iterator i =
              objects.find(dbID);
          if (i != objects.end())
          {
            o.value.clear();
            printToString(o.value, "res:%08X:%08X:%08X",
                          (unsigned int) i->second.persistentID.ext,
                          (unsigned int) i->second.persistentID.dir,
                          (unsigned int) i->second.persistentID.file);
          }
        }
      }
      else
      {
        while (chunkBuf.getFieldNumber(n, nMax, isDiff))
        {
          loadItem(o.children[std::uint32_t(n)], chunkBuf, isDiff,
                   i->second.fields[n].second);
        }
      }
    }
    return;
  }
  FileBuffer& buf2 = *(static_cast< FileBuffer * >(&chunkBuf));
  switch (itemType)
  {
    case String_None:
      o.value = "null";
      break;
    case String_String:
      {
        unsigned int  len = buf2.readUInt16();
        bool    endOfString = false;
        while (len--)
        {
          unsigned char c = buf2.readUInt8();
          if (!endOfString)
          {
            if (!c)
              endOfString = true;
            else if (c == '\\')
              o.value += "\\\\";
            else if (c >= 0x20)
              o.value += char(c);
            else if (c == '\t')
              o.value += "\\t";
            else if (c == '\n')
              o.value += "\\n";
            else if (c == '\r')
              o.value += "\\r";
            else
              o.value += ' ';
          }
        }
      }
      break;
    case String_List:
      {
        CDBChunk  listBuf;
        unsigned int  chunkType = readChunk(listBuf);
        if (chunkType != ChunkType_LIST)
        {
          throw FO76UtilsError("unexpected chunk type in CDB file at 0x%08x",
                               (unsigned int) getPosition());
        }
        std::uint32_t listClassName =
            std::uint32_t(findString(listBuf.readUInt32()));
        std::uint32_t listSize = 0U;
        if ((listBuf.getPosition() + 4ULL) <= listBuf.size())
          listSize = listBuf.readUInt32();
        for (std::uint32_t n = 0U; n < listSize; n++)
          loadItem(o.children[n], listBuf, isDiff, listClassName);
        return;
      }
      break;
    case String_Map:
      {
        CDBChunk  mapBuf;
        unsigned int  chunkType = readChunk(mapBuf);
        if (chunkType != ChunkType_MAPC)
        {
          throw FO76UtilsError("unexpected chunk type in CDB file at 0x%08x",
                               (unsigned int) getPosition());
        }
        std::uint32_t keyClassName =
            std::uint32_t(findString(mapBuf.readUInt32()));
        std::uint32_t valueClassName =
            std::uint32_t(findString(mapBuf.readUInt32()));
        std::uint32_t mapSize = 0U;
        if ((mapBuf.getPosition() + 4ULL) <= mapBuf.size())
          mapSize = mapBuf.readUInt32();
        for (std::uint32_t n = 0U; n < mapSize; n++)
        {
          loadItem(o.children[n << 1], mapBuf, false, keyClassName);
          loadItem(o.children[(n << 1) + 1U], mapBuf, isDiff, valueClassName);
        }
        return;
      }
      break;
    case String_Ref:
      {
        std::uint32_t refType = std::uint32_t(findString(buf2.readUInt32()));
        loadItem(o.children[0U], chunkBuf, isDiff, refType);
        return;
      }
      break;
    case String_Int8:
      printToString(o.value, "%d", int(std::int8_t(buf2.readUInt8())));
      break;
    case String_UInt8:
      printToString(o.value, "%u", (unsigned int) buf2.readUInt8());
      break;
    case String_Int16:
      printToString(o.value, "%d", int(std::int16_t(buf2.readUInt16())));
      break;
    case String_UInt16:
      printToString(o.value, "%u", (unsigned int) buf2.readUInt16());
      break;
    case String_Int32:
      printToString(o.value, "%d", int(buf2.readInt32()));
      break;
    case String_UInt32:
      printToString(o.value, "%u", (unsigned int) buf2.readUInt32());
      break;
    case String_Int64:
      printToString(o.value,
                    "%lld", (long long) std::int64_t(buf2.readUInt64()));
      break;
    case String_UInt64:
      printToString(o.value, "%llu", (unsigned long long) buf2.readUInt64());
      break;
    case String_Bool:
      printToString(o.value, "%s", (!buf2.readUInt8() ? "false" : "true"));
      break;
    case String_Float:
      printToString(o.value, "%.7g", buf2.readFloat());
      break;
    case String_Double:
      // FIXME: implement this in a portable way
      printToString(o.value, "%.14g",
                    std::bit_cast< double, std::uint64_t >(buf2.readUInt64()));
      break;
    default:
      chunkBuf.setPosition(chunkBuf.size());
      break;
  }
}

void CDBMaterialToJSON::readAllChunks()
{
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
        std::map< std::uint32_t, MaterialObject >::iterator i =
            objects.find(dbID);
        if (i != objects.end() && className > String_Unknown)
        {
          if (!isRootObject(i->second.baseObject))
            copyBaseObject(i->second);
          std::uint64_t k = (std::uint64_t(className) << 32) | componentIndex;
          loadItem(i->second.components[k], chunkBuf, isDiff, className);
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
      while (n--)
      {
        BSResourceID  persistentID;
        persistentID.dir = chunkBuf.readUInt32();
        persistentID.file = chunkBuf.readUInt32();
        persistentID.ext = chunkBuf.readUInt32();
        std::uint32_t dbID = chunkBuf.readUInt32();
        MaterialObject& o = objects[dbID];
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
        std::map< std::uint32_t, MaterialObject >::iterator o =
            objects.find(sourceID);
        std::map< std::uint32_t, MaterialObject >::iterator p =
            objects.find(targetID);
        if (o != objects.end() && p != objects.end())
          o->second.parent = &(p->second);
      }
    }
  }
  for (std::map< std::uint32_t, MaterialObject >::iterator
           i = objects.begin(); i != objects.end(); i++)
  {
    MaterialObject  *p = &(i->second);
    if (!isRootObject(p->baseObject))
      copyBaseObject(*p);
    while (p->parent)
      p = const_cast< MaterialObject * >(p->parent);
    if (p->persistentID.file == 0x0074616DU)    // "mat\0"
      matFileObjectMap[p->persistentID].push_back(i->second.dbID);
  }
}

void CDBMaterialToJSON::dumpObject(
    std::string& s, const CDBObject& o, int indentCnt) const
{
  if (o.type > String_Unknown)
  {
    // structure
    printToString(s, "{\n%*s\"Data\": {\n", indentCnt + 2, "");
    for (std::map< std::uint32_t, CDBObject >::const_iterator
             i = o.children.begin(); i != o.children.end(); )
    {
      std::uint32_t fieldNum = i->first;
      std::map< std::uint32_t, CDBClassDef >::const_iterator  j =
          classes.find(o.type);
      const char  *fieldNameStr = "null";
      if (j != classes.end() && fieldNum < j->second.fields.size())
        fieldNameStr = stringTable[j->second.fields[fieldNum].first];
      printToString(s, "%*s\"%s\": ", indentCnt + 4, "", fieldNameStr);
      dumpObject(s, i->second, indentCnt + 4);
      std::map< std::uint32_t, CDBObject >::const_iterator  k = i;
      k++;
      if (k != o.children.end())
        printToString(s, ",\n");
      else
        printToString(s, "\n");
      i = k;
    }
    printToString(s, "%*s},\n", indentCnt + 2, "");
    printToString(s, "%*s\"Type\": \"%s\"\n",
                  indentCnt + 2, "", stringTable[o.type]);
    printToString(s, "%*s}", indentCnt, "");
    return;
  }
  if (o.type == String_String ||
      (o.type >= String_Int8 && o.type <= String_Double))
  {
    printToString(s, "\"%s\"", o.value.c_str());
    return;
  }
  if (o.type == String_List)
  {
    printToString(s, "{\n%*s\"Data\": [\n", indentCnt + 2, "");
    for (std::map< std::uint32_t, CDBObject >::const_iterator
             i = o.children.begin(); i != o.children.end(); )
    {
      printToString(s, "%*s", indentCnt + 4, "");
      dumpObject(s, i->second, indentCnt + 4);
      std::map< std::uint32_t, CDBObject >::const_iterator  k = i;
      k++;
      if (k != o.children.end())
        printToString(s, ",\n");
      else
        printToString(s, "\n");
      i = k;
    }
    printToString(s, "%*s],\n", indentCnt + 2, "");
    if (o.children.begin() != o.children.end())
    {
      printToString(s, "%*s\"ElementType\": \"%s\",\n",
                    indentCnt + 2, "",
                    stringTable[o.children.begin()->second.type]);
    }
    printToString(s, "%*s\"Type\": \"<collection>\"\n", indentCnt + 2, "");
    printToString(s, "%*s}", indentCnt, "");
    return;
  }
  // TODO: map and ref
  if (o.type == String_None)
  {
    s += "\"null\"";
    return;
  }
  if (o.type == String_Unknown)
  {
    s += "\"<unknown>\"";
    return;
  }
}

static std::uint32_t calculateHash(
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

void CDBMaterialToJSON::dumpMaterial(
    std::string& jsonBuf, const std::string& materialPath) const
{
  jsonBuf.clear();
  if (materialPath.empty())
    return;
  std::uint64_t h = 0U;
  std::uint32_t e = calculateHash(h, materialPath);
  const std::vector< std::uint32_t >  *objectList;
  {
    BSResourceID  tmp;
    tmp.dir = std::uint32_t(h & 0xFFFFFFFFU);
    tmp.file = e;
    tmp.ext = std::uint32_t(h >> 32);
    std::map< BSResourceID, std::vector< std::uint32_t > >::const_iterator  i =
        matFileObjectMap.find(tmp);
    if (i == matFileObjectMap.end())
      return;
    objectList = &(i->second);
  }
  jsonBuf = "{\n  \"Objects\": [\n";
  for (size_t n = 0; n < objectList->size(); n++)
  {
    std::map< std::uint32_t, MaterialObject >::const_iterator i =
        objects.find((*objectList)[n]);
    if (i == objects.end())
      continue;
    printToString(jsonBuf, "    {\n      \"Components\": [\n");
    for (std::map< std::uint64_t, CDBObject >::const_iterator
             j = i->second.components.begin(); j != i->second.components.end();
             j++)
    {
      jsonBuf += "        ";
      dumpObject(jsonBuf, j->second, 8);
      if (jsonBuf.ends_with("\n        }"))
      {
        jsonBuf.resize(jsonBuf.length() - 10);
        printToString(jsonBuf, ",\n          \"Index\": \"%u\"\n",
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
                  (unsigned int) i->second.persistentID.ext,
                  (unsigned int) i->second.persistentID.dir,
                  (unsigned int) i->second.persistentID.file);
    const char  *parentStr = "";
    std::map< std::uint32_t, MaterialObject >::const_iterator j;
    if (i->second.baseObject &&
        (j = objects.find(i->second.baseObject)) != objects.end())
    {
      if (j->second.persistentID.dir == 0x7EA3660CU)
        parentStr = "materials\\\\layered\\\\root\\\\layeredmaterials.mat";
      else if (j->second.persistentID.dir == 0x8EBE84FFU)
        parentStr = "materials\\\\layered\\\\root\\\\blenders.mat";
      else if (j->second.persistentID.dir == 0x574A4CF3U)
        parentStr = "materials\\\\layered\\\\root\\\\layers.mat";
      else if (j->second.persistentID.dir == 0x7D1E021BU)
        parentStr = "materials\\\\layered\\\\root\\\\materials.mat";
      else if (j->second.persistentID.dir == 0x06F52154U)
        parentStr = "materials\\\\layered\\\\root\\\\texturesets.mat";
      else if (j->second.persistentID.dir == 0x4298BB09U)
        parentStr = "materials\\\\layered\\\\root\\\\uvstreams.mat";
    }
    printToString(jsonBuf, "      \"Parent\": \"%s\"\n    },\n", parentStr);
  }
  jsonBuf.resize(jsonBuf.length() - 2);
  jsonBuf += "\n  ],\n  \"Version\": \"1\"\n}";
}

