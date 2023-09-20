
#ifndef TERRMESH_HPP_INCLUDED
#define TERRMESH_HPP_INCLUDED

#include "common.hpp"
#include "filebuf.hpp"
#include "ddstxt.hpp"
#include "landdata.hpp"
#include "landtxt.hpp"
#include "nif_file.hpp"

class TerrainMesh : public NIFFile::NIFTriShape
{
 protected:
  std::vector< NIFFile::NIFVertex >   vertexDataBuf;
  std::vector< NIFFile::NIFTriangle > triangleDataBuf;
  std::vector< unsigned char >  textureBuf;
  std::vector< unsigned char >  textureBuf2;
  std::vector< std::uint16_t >  hmapBuf;
  DDSTexture    *landTexture[8];
  unsigned int  landTextureMask;
 public:
  TerrainMesh();
  virtual ~TerrainMesh();
  // texture resolution is 2^textureScale per height map vertex
  void createMesh(const std::uint16_t *hmapData, int hmapWidth, int hmapHeight,
                  const unsigned char * const *ltexData, unsigned int ltexMask,
                  int ltexWidth, int ltexHeight, int textureScale,
                  int x0, int y0, int x1, int y1, int cellResolution,
                  float xOffset, float yOffset, float zMin, float zMax);
  void createMesh(const LandscapeData& landData,
                  int textureScale, int x0, int y0, int x1, int y1,
                  const LandscapeTextureSet *landTextures,
                  size_t landTextureCnt, unsigned int ltexMask,
                  float textureMip = 0.0f, float textureRGBScale = 1.0f,
                  std::uint32_t textureDefaultColor = 0x003F3F3FU);
  inline unsigned int getTextureMask() const
  {
    return landTextureMask;
  }
  inline const DDSTexture * const *getTextures() const
  {
    return landTexture;
  }
};

#endif

