
#ifndef BTDFILE_HPP_INCLUDED
#define BTDFILE_HPP_INCLUDED

#include "common.hpp"
#include "filebuf.hpp"

class BTDFile : public FileBuffer
{
 protected:
  size_t  heightMapResX;        // landscape total X resolution (nCellsX * 128)
  size_t  heightMapResY;        // landscape total Y resolution (nCellsY * 128)
  int     cellMinX;             // X coordinate of cell in SW corner
  int     cellMinY;             // Y coordinate of cell in SW corner
  int     cellMaxX;             // X coordinate of cell in NE corner
  int     cellMaxY;             // Y coordinate of cell in NE corner
  size_t  nCellsX;              // world map X size in cells
  size_t  nCellsY;              // world map Y size in cells
  float   worldHeightMin;       // world map minimum height
  float   worldHeightMax;       // world map maximum height
  size_t  cellHeightMinMaxMapOffs;      // minimum, maximum height for each cell
  size_t  ltexCnt;              // number of land textures
  size_t  ltexOffs;             // table of LTEX form IDs
  size_t  ltexMapOffs;          // 8 x (land texture + 1) for each cell quadrant
  size_t  heightMapLOD4;        // 1/8 cell resolution
  size_t  landTexturesLOD4;     // 1/8 cell resolution
  size_t  nCompressedBlocks;    // number of ZLib compressed blocks
  size_t  zlibBlocksTableOffs;  // table of compressed block offsets and sizes
  size_t  zlibBlocksDataOffs;   // ZLib compressed data
  size_t  zlibBlkTableOffsLOD3;
  size_t  zlibBlkTableOffsLOD2;
  size_t  zlibBlkTableOffsLOD1;
  size_t  zlibBlkTableOffsLOD0;
  struct TileData
  {
    unsigned short  x0;
    unsigned short  y0;
    // bit 0: LOD0 vertex height and land textures are loaded
    // bit 2: LOD1 vertex height and land textures are loaded
    // bit 4: LOD2 vertex height and land textures are loaded
    // bit 6: LOD3 vertex height and land textures are loaded
    // bit 8: LOD4 vertex height and land textures are loaded
    unsigned int  blockMask;
    std::vector< std::uint16_t >  hmapData;     // vertex height, 1024 * 1024
    std::vector< std::uint16_t >  ltexData;     // land textures, 1024 * 1024
  };
  std::map< unsigned int, TileData * >  tileCacheMap;
  std::vector< TileData > tileCache;
  size_t  tileCacheIndex;
  // ----------------
  void loadBlock(TileData& tileData, size_t dataOffs,
                 size_t n, unsigned char l,
                 std::vector< std::uint16_t >& zlibBuf);
  void loadBlocks(TileData& tileData, size_t x, size_t y,
                  size_t threadIndex, size_t threadCnt, unsigned int blockMask);
  static void loadBlocksThread(BTDFile *p, std::string *errMsg,
                               TileData *tileData, size_t x, size_t y,
                               size_t threadIndex, size_t threadCnt,
                               unsigned int blockMask);
  const TileData& loadTile(int cellX, int cellY, unsigned int blockMask);
 public:
  BTDFile(const unsigned char *fileData, size_t fileSize);
  virtual ~BTDFile();
  // set the number of 8x8 cell (5.125 MiB) tiles to keep decompressed
  void setTileCacheSize(size_t n);
  inline int getCellMinX() const
  {
    return cellMinX;
  }
  inline int getCellMinY() const
  {
    return cellMinY;
  }
  inline int getCellMaxX() const
  {
    return cellMaxX;
  }
  inline int getCellMaxY() const
  {
    return cellMaxY;
  }
  inline float getMinHeight() const
  {
    return worldHeightMin;
  }
  inline float getMaxHeight() const
  {
    return worldHeightMax;
  }
  inline size_t getLandTextureCount() const
  {
    return ltexCnt;
  }
  unsigned int getLandTexture(size_t n) const;
  // N = 128 >> l, buffer size = N * N
  // height map in pixelFormatGRAY16 format
  void getCellHeightMap(std::uint16_t *buf, int cellX, int cellY,
                        unsigned char l = 0);
  // land texture opacities in pixelFormatA16 format (see filebuf.hpp)
  void getCellLandTexture(std::uint16_t *buf, int cellX, int cellY,
                          unsigned char l = 0);
  // buf[0..6]   = SW quadrant land texture IDs (0xFF: no texture),
  //               getLandTexture() returns the corresponding form IDs
  // buf[16..31] = SE quadrant land texture IDs
  // buf[32..63] = NW and NE quadrants
  void getCellTextureSet(unsigned char *buf, int cellX, int cellY) const;
};

#endif

