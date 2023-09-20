
#ifndef LANDDATA_HPP_INCLUDED
#define LANDDATA_HPP_INCLUDED

#include "common.hpp"
#include "filebuf.hpp"
#include "esmfile.hpp"
#include "btdfile.hpp"
#include "ba2file.hpp"
#include "material.hpp"

class LandscapeData
{
 protected:
  int     cellMinX;
  int     cellMinY;
  int     cellMaxX;
  int     cellMaxY;
  int     cellResolution;
  std::uint16_t *hmapData;
  std::uint16_t *ltexData16;
  unsigned char *txtSetData;
  float   zMin;
  float   zMax;
  float   waterLevel;
  float   landLevel;
  unsigned int  waterFormID;
  std::vector< unsigned int > ltexFormIDs;
  std::vector< std::string >  ltexEDIDs;
  std::vector< std::string >  materialPaths;
  std::vector< const CE2Material * >  ltexMaterials;
  std::vector< std::uint32_t >  dataBuf;
  void allocateDataBuf();
  void loadBTDFile(const unsigned char *btdFileData, size_t btdFileSize,
                   unsigned char mipLevel);
  void loadTextureInfo(ESMFile& esmFile,
                       const CE2MaterialDB *materials, size_t n);
  // returns the form ID of the parent world, or 0 if there is none
  unsigned int loadWorldInfo(ESMFile& esmFile, unsigned int worldID);
 public:
  LandscapeData(
      ESMFile *esmFile, const char *btdFileName,
      const BA2File *ba2File, const CE2MaterialDB *materials,
      unsigned int worldID = 0U, int mipLevel = 0,
      int xMin = -32768, int yMin = -32768, int xMax = 32767, int yMax = 32767);
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
    return ((-cellMinX) * cellResolution);
  }
  inline int getOriginY() const
  {
    return ((cellMaxY + 1) * cellResolution - 1);
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
  inline const CE2Material *getTextureMaterial(size_t n) const
  {
    return ltexMaterials[n];
  }
  inline const std::string& getMaterialPath(size_t n) const
  {
    return materialPaths[n];
  }
  // txtSetData[y * nCellsX * 32 + (x * 16) + n] =
  //     texture ID for layer n of cell quadrant x,y
  // Starfield uses layers 0 to 6 (0 = top layer, 6 = base layer)
  // with 5x3 bit additional layer opacities.
  inline const unsigned char *getCellTextureSets() const
  {
    return txtSetData;
  }
  inline const std::uint16_t *getHeightMap() const
  {
    return hmapData;
  }
  inline const std::uint16_t *getLandTexture16() const
  {
    return ltexData16;
  }
};

#endif

