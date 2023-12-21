
#ifndef MAT_JSON_HPP_INCLUDED
#define MAT_JSON_HPP_INCLUDED

#include "common.hpp"
#include "filebuf.hpp"
#include "cdb_file.hpp"

class CDBMaterialToJSON : public CDBFile
{
 protected:
  struct CDBClassDef
  {
    std::vector< std::pair< std::uint32_t, std::uint32_t > >  fields;
    bool    isUser = false;
  };
  struct CDBObject
  {
    std::uint32_t type;
    std::string value;
    std::map< std::uint32_t, CDBObject >  children;
  };
  struct BSResourceID
  {
    std::uint32_t dir;
    std::uint32_t file;
    std::uint32_t ext;
    inline BSResourceID()
    {
    }
    BSResourceID(const std::string& fileName);
    inline bool operator<(const BSResourceID& r) const
    {
      return (dir < r.dir || (dir == r.dir && file < r.file) ||
              (dir == r.dir && file == r.file && ext < r.ext));
    }
  };
  struct MaterialObject
  {
    BSResourceID  persistentID;
    std::uint32_t dbID;
    std::uint32_t baseObject;
    const MaterialObject  *parent;
    // key = (type << 32) | index
    std::map< std::uint64_t, CDBObject >  components;
  };
  std::map< std::uint32_t, CDBClassDef >  classes;
  std::vector< std::uint32_t >  objectsHashMap;
  std::vector< MaterialObject > objects;
  std::vector< std::pair< std::uint32_t, std::uint32_t > >  componentInfo;
  std::map< BSResourceID, std::vector< std::uint32_t > >  matFileObjectMap;
  static inline bool isRootObject(std::uint32_t dbID)
  {
    return (dbID <= 7U);
  }
  inline MaterialObject *findObject(std::uint32_t dbID);
  inline const MaterialObject *findObject(std::uint32_t dbID) const;
  void copyBaseObject(MaterialObject& o);
  void loadItem(CDBObject& o, CDBChunk& chunkBuf, bool isDiff,
                std::uint32_t itemType);
  void readAllChunks();
  void dumpObject(std::string& s, const CDBObject& o, int indentCnt) const;
 public:
  CDBMaterialToJSON(const unsigned char *fileData, size_t fileSize)
    : CDBFile(fileData, fileSize)
  {
    readAllChunks();
  }
  CDBMaterialToJSON(const char *fileName)
    : CDBFile(fileName)
  {
    readAllChunks();
  }
  void dumpMaterial(std::string& jsonBuf,
                    const std::string& materialPath) const;
};

#endif

