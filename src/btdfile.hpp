
#ifndef BTDFILE_HPP_INCLUDED
#define BTDFILE_HPP_INCLUDED

#include "common.hpp"
#include "zlib.hpp"
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
  size_t  gcvrCnt;              // number of ground covers
  size_t  gcvrOffs;             // table of GCVR form IDs
  size_t  gcvrMapOffs;          // 8 x ground cover for each cell quadrant
  size_t  heightMapLOD4;        // 1/8 cell resolution
  size_t  landTexturesLOD4;     // 1/8 cell resolution
  size_t  unknownLOD4;          // 1/8 cell resolution
  size_t  nCompressedBlocks;    // number of ZLib compressed blocks
  size_t  zlibBlocksTableOffs;  // table of compressed block offsets and sizes
  size_t  zlibBlocksDataOffs;   // ZLib compressed data
  size_t  zlibBlkTableOffsLOD3;
  size_t  zlibBlkTableOffsLOD2;
  size_t  zlibBlkTableOffsLOD1;
  size_t  zlibBlkTableOffsLOD0;
  // current 8x8 group of cells loaded
  int     curTileX;
  int     curTileY;
  // previous 8x8 group of cells (cached)
  int     prvTileX;
  int     prvTileY;
  // 8x8 cells:
  //   curTileData[y * 1024 + x + 0x00000000] = vertex height
  //   curTileData[y * 1024 + x + 0x00100000] = landscape textures
  //   curTileData[y * 1024 + x + 0x00200000] = ground cover
  //   curTileData[y * 1024 + x + 0x00300000] = unknown map type
  unsigned short  *curTileData;
  unsigned short  *prvTileData;
  std::vector< unsigned short > tileBuf;
  // ----------------
  static void loadBlockLines_8(unsigned short *dst, const unsigned char *src,
                               unsigned char l);
  static void loadBlockLines_16(unsigned short *dst, const unsigned char *src,
                                unsigned char l);
  void loadBlock(unsigned short *tileData, size_t n,
                 unsigned char l, unsigned char b,
                 ZLibDecompressor& zlibDecompressor,
                 std::vector< unsigned short >& zlibBuf);
  void loadBlocks(unsigned short *tileData, size_t x, size_t y,
                  size_t threadIndex, size_t threadCnt);
  static void loadBlocksThread(BTDFile *p, std::string *errMsg,
                               unsigned short *tileData, size_t x, size_t y,
                               size_t threadIndex, size_t threadCnt);
  void loadTile(int cellX, int cellY);
 public:
  BTDFile(const char *fileName);
  virtual ~BTDFile();
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
  unsigned int getLandTexture(size_t n);
  unsigned int getGroundCover(size_t n);
  // buf[128 * 128]
  void getCellHeightMap(unsigned short *buf, int cellX, int cellY);
  // buf[128 * 128 * 16], for each vertex, the data format is:
  //   (texture, opacity) * 6, reserved * 4
  //   texture = 0xFF or opacity = 0: no texture
  void getCellLandTexture(unsigned char *buf, int cellX, int cellY);
  // buf[128 * 128 * 8], bit 7 is transparency (no ground cover if set)
  void getCellGroundCover(unsigned char *buf, int cellX, int cellY);
  // buf[128 * 128], unknown 3-channel map type in packed 16-bit format
  void getCellData4(unsigned short *buf, int cellX, int cellY);
};

#endif

