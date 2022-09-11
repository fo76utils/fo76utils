
#ifndef TERRMESH_HPP_INCLUDED
#define TERRMESH_HPP_INCLUDED

#include "common.hpp"
#include "filebuf.hpp"
#include "ddstxt.hpp"
#include "landdata.hpp"
#include "nif_file.hpp"

class TerrainMesh : public NIFFile::NIFTriShape
{
 protected:
  std::vector< NIFFile::NIFVertex >   vertexDataBuf;
  std::vector< NIFFile::NIFTriangle > triangleDataBuf;
  std::vector< unsigned char >  textureBuf;
  std::vector< unsigned char >  textureBuf2;
  std::vector< std::uint16_t >  hmapBuf;
  DDSTexture  *landTexture[2];
 public:
  TerrainMesh();
  virtual ~TerrainMesh();
  // texture resolution is 2^textureScale per height map vertex
  void createMesh(const std::uint16_t *hmapData, int hmapWidth, int hmapHeight,
                  const unsigned char *ltexData, const unsigned char *ltexDataN,
                  int ltexWidth, int ltexHeight, int textureScale,
                  int x0, int y0, int x1, int y1, int cellResolution,
                  float xOffset, float yOffset, float zMin, float zMax);
  void createMesh(const LandscapeData& landData,
                  int textureScale, int x0, int y0, int x1, int y1,
                  const DDSTexture * const *landTextures,
                  const DDSTexture * const *landTexturesN,
                  size_t landTextureCnt, float textureMip = 0.0f,
                  float textureRGBScale = 1.0f,
                  std::uint32_t textureDefaultColor = 0x003F3F3FU);
  inline unsigned int getTextureMask() const
  {
    return (landTexture[1] ? 3U : 1U);
  }
  inline const DDSTexture * const *getTextures() const
  {
    return landTexture;
  }
};

#endif

