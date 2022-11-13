
#ifndef MARKERS_HPP_INCLUDED
#define MARKERS_HPP_INCLUDED

#include "common.hpp"
#include "esmfile.hpp"
#include "nif_file.hpp"
#include "ddstxt.hpp"

class MapImage
{
 public:
  struct REFRRecord
  {
    unsigned int  formID;
    unsigned int  name;
    unsigned int  xtel;
    unsigned int  tnam;
    bool    isInterior;
    bool    isChildWorld;
    bool    isMapMarker;
    bool    isDoor;
    float   x;
    float   y;
    float   z;
    REFRRecord(unsigned int n = 0U)
      : formID(n), name(0U), xtel(0U), tnam(0xFFFFU),
        isInterior(false), isChildWorld(false),
        isMapMarker(false), isDoor(false),
        x(0.0f), y(0.0f), z(0.0f)
    {
    }
    inline bool operator<(const REFRRecord& r) const
    {
      return (formID < r.formID);
    }
  };
 protected:
  struct MarkerDef
  {
    DDSTexture    *texture;
    unsigned int  formID;
    int           flags;
    float         mipLevel;
    float         xOffs;
    float         yOffs;
    bool          uniqueTexture;
    unsigned char priority;
    MarkerDef()
      : texture((DDSTexture *) 0), formID(0U), flags(-1), mipLevel(0.0f),
        xOffs(0.0f), yOffs(0.0f), uniqueTexture(false), priority(0)
    {
    }
  };
  ESMFile&  esmFile;
  std::vector< MarkerDef >  markerDefs;
  std::uint32_t *buf;
  size_t  imageWidth;
  size_t  imageHeight;
  NIFFile::NIFVertexTransform viewTransform;
  unsigned int  priorityMask;
  unsigned int  worldFormID;
  bool    isInteriorMap;
  std::set< unsigned int >  formIDs;
  struct CellOffset
  {
    float   x;
    float   y;
    float   z;
    int     weight;
    CellOffset()
      : x(0.0f), y(0.0f), z(0.0f), weight(0)
    {
    }
  };
  std::map< unsigned int, CellOffset >  cellOffsets;
  std::set< unsigned int >  disabledMarkers;
  static void createIcon256(std::vector< unsigned char >& buf, unsigned int c);
  void loadTextures(const char *listFileName);
  bool checkParentWorld(const ESMFile::ESMRecord& r);
  void setWorldCellOffsets(unsigned int formID, float x, float y, float z);
  void setInteriorCellOffset(const REFRRecord& refr);
  void findDisabledMarkers(const ESMFile::ESMRecord& r);
 public:
  MapImage(ESMFile& esmFiles, const char *listFileName,
           std::uint32_t *outBufRGBA, int w, int h,
           const NIFFile::NIFVertexTransform& vt);
  virtual ~MapImage();
  const ESMFile::ESMRecord *getParentCell(unsigned int formID) const;
  bool getREFRRecord(REFRRecord& r, unsigned int formID);
  void drawIcon(size_t n, float x, float y, float z);
  void findMarkers(unsigned int worldID = 0U);
};

#endif

