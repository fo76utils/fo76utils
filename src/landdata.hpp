
#ifndef LANDDATA_HPP_INCLUDED
#define LANDDATA_HPP_INCLUDED

#include "common.hpp"
#include "filebuf.hpp"
#include "esmfile.hpp"
#include "btdfile.hpp"
#include "ba2file.hpp"

class LandscapeData
{
 protected:
  int     cellMinX;
  int     cellMinY;
  int     cellMaxX;
  int     cellMaxY;
  int     cellResolution;
  int     cellOffset;
  unsigned short  *hmapData;
  unsigned int    *ltexData;
  unsigned char   *vnmlData;
  unsigned char   *vclrData24;
  unsigned short  *gcvrData;
  unsigned short  *vclrData16;
  float   zMin;
  float   zMax;
  float   waterLevel;
  float   landLevel;
  unsigned int  waterFormID;
  std::vector< unsigned int > ltexFormIDs;
  std::vector< std::string >  ltexEDIDs;
  std::vector< std::string >  ltexBGSMPaths;
  std::vector< std::string >  ltexDPaths;
  std::vector< std::string >  ltexNPaths;
  std::vector< unsigned int > dataBuf;
  void allocateDataBuf(unsigned int formatMask, bool isFO76);
  void loadBTDFile(const char *btdFileName,
                   unsigned int formatMask, unsigned char mipLevel);
  // ((y + 32768) << 48) | ((x + 32768) << 32) | land_formID
  void findESMLand(ESMFile& esmFile,
                   std::vector< unsigned long long >& landList,
                   unsigned int formID, int d = 0, int x = 0, int y = 0);
  unsigned char findTextureID(unsigned int formID);
  void loadESMFile(ESMFile& esmFile,
                   unsigned int formatMask, unsigned int worldID,
                   unsigned int defTxtID, unsigned char mipLevel);
  static void readString(std::string& s, size_t n, FileBuffer& buf);
  static void readPath(std::string& s, size_t n, FileBuffer& buf,
                       const char *prefix = 0, const char *suffix = 0);
  void loadTextureInfo(ESMFile& esmFile, const BA2File *ba2File, size_t n);
  void loadWorldInfo(ESMFile& esmFile, unsigned int worldID);
 public:
  // formatMask & 1:  set to load height map (pixelFormatGRAY16)
  // formatMask & 2:  set to load land textures (pixelFormatL8A24)
  // formatMask & 4:  set to load vertex normals (pixelFormatRGB24)
  // formatMask & 8:  set to load vertex colors (pixelFormatRGB24 or RGBA16)
  // formatMask & 16: set to load ground cover (pixelFormatL8A8)
  LandscapeData(ESMFile *esmFile, const char *btdFileName,
                const BA2File *ba2File, unsigned int formatMask = 31U,
                unsigned int worldID = 0U, unsigned int defTxtID = 0U,
                int mipLevel = 0, int xMin = -32768, int yMin = -32768,
                int xMax = 32767, int yMax = 32767);
  virtual ~LandscapeData();
  inline int getXMin() const
  {
    return cellMinX;
  }
  inline int getYMin() const
  {
    return cellMinY;
  }
  inline float getZMin() const
  {
    return zMin;
  }
  inline int getXMax() const
  {
    return cellMaxX;
  }
  inline int getYMax() const
  {
    return cellMaxY;
  }
  inline float getZMax() const
  {
    return zMax;
  }
  inline int getCellResolution() const
  {
    return cellResolution;
  }
  inline float getWaterLevel() const
  {
    return waterLevel;
  }
  inline float getDefaultLandLevel() const
  {
    return landLevel;
  }
  inline unsigned int getWaterFormID() const
  {
    return waterFormID;
  }
  inline int getImageWidth() const
  {
    return ((cellMaxX + 1 - cellMinX) * cellResolution);
  }
  inline int getImageHeight() const
  {
    return ((cellMaxY + 1 - cellMinY) * cellResolution);
  }
  // return the image X/Y position corresponding to 0.0,0.0 world coordinates
  inline int getOriginX() const
  {
    return ((-cellMinX) * cellResolution + cellOffset);
  }
  inline int getOriginY() const
  {
    return ((cellMaxY + 1) * cellResolution - (cellOffset + 1));
  }
  inline size_t getTextureCount() const
  {
    return ltexFormIDs.size();
  }
  inline unsigned int getTextureFormID(size_t n) const
  {
    return ltexFormIDs[n];
  }
  inline const std::string& getTextureEDID(size_t n) const
  {
    return ltexEDIDs[n];
  }
  inline const std::string& getTextureMaterial(size_t n) const
  {
    return ltexBGSMPaths[n];
  }
  inline const std::string& getTextureDiffuse(size_t n) const
  {
    return ltexDPaths[n];
  }
  inline const std::string& getTextureNormal(size_t n) const
  {
    return ltexNPaths[n];
  }
  inline const unsigned short *getHeightMap() const
  {
    return hmapData;
  }
  inline const unsigned int *getLandTexture() const
  {
    return ltexData;
  }
  // for Fallout 4 and older
  inline const unsigned char *getVertexNormals() const
  {
    return vnmlData;
  }
  inline const unsigned char *getVertexColor24() const
  {
    return vclrData24;
  }
  // for Fallout 76
  inline const unsigned short *getGroundCover() const
  {
    return gcvrData;
  }
  inline const unsigned short *getVertexColor16() const
  {
    // the maximum cell resolution for this data is 32
    return vclrData16;
  }
};

#endif

