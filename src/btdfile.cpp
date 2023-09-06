
// NewAtlantis.btd file format:
//
// 0x00000000-0x00000003: "BTDB"
// 0x00000004-0x00000007: (uint32) 5 or 6, format version?
// 0x00000008-0x0000000B: (float) 0.0, mimimum height in meters
// 0x0000000C-0x0000000F: (float) 260.502045, maximum height in meters
// 0x00000010-0x00000013: (uint32) 1792 (14 * 128), total resolution X
// 0x00000014-0x00000017: (uint32) 1792 (14 * 128), total resolution Y
// 0x00000018-0x0000001B: (int32) 0, unused (was minimum cell X (west) in FO76)
// 0x0000001C-0x0000001F: (int32) 0, unused (was minimum cell Y (south) in FO76)
// 0x00000020-0x00000023: (int32) 0, unused (was maximum cell X (east) in FO76)
// 0x00000024-0x00000027: (int32) 0, unused (was maximum cell Y (north) in FO76)
// 0x00000028-0x0000002B: (uint32) 7, number of land textures (LTEX records)
// 0x0000002C-0x00000047: 7 * uint32, LTEX form IDs
// 0x00000048-0x00000667: 14 * 14 * float[2], min, max height for each cell
// 0x00000668-0x00001EE7: 14 * 2 * 14 * 2 * byte[8], cell quadrant land
//                        textures (LTEX_table_index - 1, or zero if none)
// 0x00001EE8-0x000080E7: 14 * 8 * 14 * 8 * uint16, LOD4 height map,
//                        0 to 65535 maps to minimum height to maximum height
// 0x000080E8-0x0000E2E7: 14 * 8 * 14 * 8 * uint16, LOD4 land textures,
//                        bits[N * 3 to N * 3 + 2] = texture N alpha (0 to 7),
//                        texture 6 is the base texture
// 0x0000E2E8-0x0000EB2F: 265 * (uint32, uint32), pairs of zlib compressed data
//                        offset (relative to 0x0000EB30) and size
// 0x0000EB30-EOF:        zlib compressed data blocks
// zlib compressed blocks     0 to   3 ( 2 *  2): LOD3, 8x8 cell groups
//                            4 to  19 ( 4 *  4): LOD2, 4x4 cell groups
//                           20 to  68 ( 7 *  7): LOD1, 2x2 cell groups
//                           69 to 264 (14 * 14): LOD0 height map, LTEX
// height map, LTEX block format:
//   0x0000-0x7FFF: 128 * 128 uint16 height map
//   0x8000-0xFFFF: 128 * 128 uint16 land texture alphas

#include "common.hpp"
#include "zlib.hpp"
#include "btdfile.hpp"

#include <thread>

void BTDFile::loadBlock(TileData& tileData, size_t dataOffs,
                        size_t n, unsigned char l,
                        std::vector< std::uint16_t >& zlibBuf)
{
  size_t  zlibBlkTableOffs = zlibBlkTableOffsLOD0 + (n << 3);
  if (l == 1)
    zlibBlkTableOffs = zlibBlkTableOffsLOD1 + (n << 3);
  else if (l == 2)
    zlibBlkTableOffs = zlibBlkTableOffsLOD2 + (n << 3);
  else if (l == 3)
    zlibBlkTableOffs = zlibBlkTableOffsLOD3 + (n << 3);
  FileBuffer  blockBuf(fileBuf + zlibBlkTableOffs,
                       fileBufSize - zlibBlkTableOffs);
  size_t  offs = blockBuf.readUInt32() + zlibBlocksDataOffs;
  size_t  compressedSize = blockBuf.readUInt32();
  if ((offs + compressedSize) > fileBufSize)
    errorMessage("end of input file");
  unsigned char *p = reinterpret_cast< unsigned char * >(zlibBuf.data());
  if (ZLibDecompressor::decompressData(p,
                                       zlibBuf.size() * sizeof(std::uint16_t),
                                       fileBuf + offs, compressedSize) != 65536)
  {
    errorMessage("error in compressed landscape data");
  }
  size_t  xd = 1 << l;
  for (size_t y = 0; y < 128; y++)      // vertex height
  {
    std::uint16_t *dst =
        tileData.hmapData.data() + (dataOffs + (y << (l + 10)));
    for (size_t i = 0; i < 128; i++, dst = dst + xd, p = p + 2)
      *dst = FileBuffer::readUInt16Fast(p);
  }
  for (size_t y = 0; y < 128; y++)      // landscape textures
  {
    std::uint16_t *dst =
        tileData.ltexData.data() + (dataOffs + (y << (l + 10)));
    for (size_t i = 0; i < 128; i++, dst = dst + xd, p = p + 2)
      *dst = FileBuffer::readUInt16Fast(p);
  }
}

void BTDFile::loadBlocks(TileData& tileData, size_t x, size_t y,
                         size_t threadIndex, size_t threadCnt,
                         unsigned int blockMask)
{
  std::vector< std::uint16_t >  zlibBuf(0x8000, 0);
  size_t  t = 0;
  // LOD0..LOD3
  for (unsigned char l = 0; l < 4; l++)
  {
    if (!(blockMask & (1U << (l + l))))
      continue;
    for (size_t yy = 0; yy < size_t(8 >> l); yy++)
    {
      size_t  yc = (y >> l) + yy;
      if (yc >= ((nCellsY + (1 << l) - 1) >> l))
        break;
      for (size_t xx = 0; xx < size_t(8 >> l); xx++)
      {
        size_t  xc = (x >> l) + xx;
        if (xc >= ((nCellsX + (1 << l) - 1) >> l))
          break;
        if (t == threadIndex)
        {
          size_t  n = yc * ((nCellsX + (1 << l) - 1) >> l) + xc;
          loadBlock(tileData, ((yy << 10) + xx) << (l + 7), n, l, zlibBuf);
        }
        if (++t >= threadCnt)
          t = 0;
      }
    }
    break;
  }
}

void BTDFile::loadBlocksThread(BTDFile *p, std::string *errMsg,
                               TileData *tileData, size_t x, size_t y,
                               size_t threadIndex, size_t threadCnt,
                               unsigned int blockMask)
{
  try
  {
    p->loadBlocks(*tileData, x, y, threadIndex, threadCnt, blockMask);
  }
  catch (std::exception& e)
  {
    *errMsg = std::string(e.what());
  }
}

const BTDFile::TileData& BTDFile::loadTile(int cellX, int cellY,
                                           unsigned int blockMask)
{
  unsigned int  x0 = (unsigned int) (cellX - cellMinX) & 0xFFF8U;
  unsigned int  y0 = (unsigned int) (cellY - cellMinY) & 0xFFF8U;
  unsigned int  cacheKey = (y0 << 16) | x0;
  std::map< unsigned int, TileData * >::iterator  t =
      tileCacheMap.find(cacheKey);
  if (t == tileCacheMap.end())
  {
    TileData& tmp = tileCache[tileCacheIndex];
    t = tileCacheMap.find(((unsigned int) tmp.y0 << 16) | tmp.x0);
    if (t != tileCacheMap.end())
      tileCacheMap.erase(t);
    tmp.x0 = (unsigned short) x0;
    tmp.y0 = (unsigned short) y0;
    tmp.blockMask = 0U;
    t = tileCacheMap.insert(std::pair< unsigned int, TileData * >(
                                cacheKey, &tmp)).first;
    tileCacheIndex++;
    if (tileCacheIndex >= tileCache.size())
      tileCacheIndex = 0;
  }
  TileData& tileData = *(t->second);
  blockMask = (blockMask & ~(tileData.blockMask)) & 0x0155U;
  if (!blockMask)
    return tileData;

  tileData.hmapData.resize(0x00100000);
  tileData.ltexData.resize(0x00100000);

  // LOD4
  if (blockMask == 0x0100)
  {
    for (size_t yy = 0; yy < 64; yy++)
    {
      if ((cellMinY + int((yy >> 3) + y0)) > cellMaxY)
        break;
      for (size_t xx = 0; xx < 64; xx++)
      {
        if ((cellMinX + int((xx >> 3) + x0)) > cellMaxX)
          break;
        size_t  offs = (yy + (y0 << 3)) * (nCellsX << 3) + (xx + (x0 << 3));
        tileData.hmapData[((yy << 10) + xx) << 4] =
            readUInt16(heightMapLOD4 + (offs << 1));
        tileData.ltexData[((yy << 10) + xx) << 4] =
            readUInt16(landTexturesLOD4 + (offs << 1));
      }
    }
    tileData.blockMask = tileData.blockMask | blockMask;
    return tileData;
  }

  // LOD3..LOD0
  size_t  threadCnt = size_t(std::thread::hardware_concurrency());
  if (threadCnt < 1)
    threadCnt = 1;
  else if (threadCnt > 16)
    threadCnt = 16;
  std::vector< std::thread * >  threads(threadCnt, (std::thread *) 0);
  std::vector< std::string >    errMsgs(threadCnt);
  for (size_t i = 0; i < threadCnt; i++)
  {
    try
    {
      threads[i] = new std::thread(loadBlocksThread,
                                   this, errMsgs.data() + i, &tileData,
                                   x0, y0, i, threadCnt, blockMask);
    }
    catch (std::exception& e)
    {
      errMsgs[i] = std::string(e.what());
      break;
    }
  }
  for (size_t i = 0; i < threadCnt; i++)
  {
    if (threads[i])
    {
      threads[i]->join();
      delete threads[i];
    }
  }
  for (size_t i = 0; i < threadCnt; i++)
  {
    if (!errMsgs[i].empty())
      throw FO76UtilsError(1, errMsgs[i].c_str());
  }
  tileData.blockMask = tileData.blockMask | blockMask;
  return tileData;
}

BTDFile::BTDFile(const char *fileName)
  : FileBuffer(fileName)
{
  if (!checkType(readUInt32(), "BTDB"))
    errorMessage("input file format is not BTD");
  if ((readUInt32() - 5U) & ~1U)        // must be 5 or 6
    errorMessage("unsupported BTD format version");
  tileCacheIndex = 0;
  // TODO: check header data for errors
  worldHeightMin = readFloat();
  worldHeightMax = readFloat();
  heightMapResX = readUInt32();
  heightMapResY = readUInt32();
  cellMinX = readInt32();
  cellMinY = readInt32();
  cellMaxX = readInt32();
  cellMaxY = readInt32();
  if (!(cellMinX | cellMinY | cellMaxX | cellMaxY))
  {
    cellMinX = -(int(heightMapResX >> 8));
    cellMinY = -(int(heightMapResY >> 8));
    cellMaxX = cellMinX + int(heightMapResX >> 7) - 1;
    cellMaxY = cellMinY + int(heightMapResY >> 7) - 1;
  }
  nCellsX = size_t(cellMaxX + 1 - cellMinX);
  nCellsY = size_t(cellMaxY + 1 - cellMinY);
  ltexCnt = readUInt32();
  ltexOffs = filePos;
  for (size_t i = 0; i < ltexCnt; i++)
    (void) readUInt32();
  cellHeightMinMaxMapOffs = filePos;
  filePos = filePos + ((nCellsY * nCellsX) << 3);
  ltexMapOffs = filePos;
  filePos = filePos + ((nCellsY * nCellsX) << 5);
  heightMapLOD4 = filePos;
  filePos = filePos + ((nCellsY * nCellsX) << 7);
  landTexturesLOD4 = filePos;
  filePos = filePos + ((nCellsY * nCellsX) << 7);
  zlibBlocksTableOffs = filePos;
  zlibBlkTableOffsLOD3 = filePos;
  filePos = filePos + (((nCellsY + 7) >> 3) * ((nCellsX + 7) >> 3) * 8);
  zlibBlkTableOffsLOD2 = filePos;
  filePos = filePos + (((nCellsY + 3) >> 2) * ((nCellsX + 3) >> 2) * 8);
  zlibBlkTableOffsLOD1 = filePos;
  filePos = filePos + (((nCellsY + 1) >> 1) * ((nCellsX + 1) >> 1) * 8);
  zlibBlkTableOffsLOD0 = filePos;
  filePos = filePos + (nCellsY * nCellsX * 8);
  zlibBlocksDataOffs = filePos;
  nCompressedBlocks = (filePos - zlibBlocksTableOffs) >> 3;
  (void) readUInt8();
  setTileCacheSize(2);
}

BTDFile::~BTDFile()
{
}

void BTDFile::setTileCacheSize(size_t n)
{
  size_t  prvSize = tileCache.size();
  if (n <= prvSize)
    return;
  tileCacheMap.clear();
  tileCache.resize(n);
  for (size_t i = prvSize; i < n; i++)
  {
    tileCache[i].x0 = 0x8000;
    tileCache[i].y0 = 0x8000;
    tileCache[i].blockMask = 0U;
  }
  for (size_t i = 0; i < prvSize; i++)
  {
    TileData  *t = tileCache.data() + i;
    unsigned int  cacheKey = ((unsigned int) t->y0 << 16) | t->x0;
    if (bool(cacheKey & 0x80008000U) || !(t->blockMask))
    {
      t->x0 = 0x8000;
      t->y0 = 0x8000;
      t->blockMask = 0U;
      continue;
    }
    tileCacheMap.insert(std::pair< unsigned int, TileData * >(cacheKey, t));
  }
}

unsigned int BTDFile::getLandTexture(size_t n) const
{
  if (n >= ltexCnt)
    return 0U;
  return readUInt32(ltexOffs + (n << 2));
}

void BTDFile::getCellHeightMap(std::uint16_t *buf, int cellX, int cellY,
                               unsigned char l)
{
  const TileData& tileData = loadTile(cellX, cellY, (~0U << (l + l)) & 0x0155U);
  size_t  x0 = size_t((cellX - cellMinX) & 7) << 7;
  size_t  y0 = size_t((cellY - cellMinY) & 7) << 7;
  size_t  n = 128 >> l;
  unsigned char m = 7 - l;
  for (size_t yc = 0; yc < n; yc++)
  {
    for (size_t xc = 0; xc < n; xc++)
    {
      buf[(yc << m) | xc] =
          tileData.hmapData[((y0 + (yc << l)) << 10) + x0 + (xc << l)];
    }
  }
}

void BTDFile::getCellLandTexture(std::uint16_t *buf, int cellX, int cellY,
                                 unsigned char l)
{
  const TileData& tileData = loadTile(cellX, cellY, (~0U << (l + l)) & 0x0155U);
  size_t  x0 = size_t((cellX - cellMinX) & 7) << 7;
  size_t  y0 = size_t((cellY - cellMinY) & 7) << 7;
  size_t  n = 128 >> l;
  unsigned char m = 7 - l;
  for (size_t yc = 0; yc < n; yc++)
  {
    for (size_t xc = 0; xc < n; xc++)
    {
      buf[(yc << m) | xc] =
          tileData.ltexData[((y0 + (yc << l)) << 10) + x0 + (xc << l)];
    }
  }
}

void BTDFile::getCellGroundCover(unsigned char *buf, int cellX, int cellY,
                                 unsigned char l)
{
  size_t  n = 128 >> l;
  unsigned char m = 7 - l;
  for (size_t yc = 0; yc < n; yc++)
  {
    for (size_t xc = 0; xc < n; xc++)
      buf[(yc << m) | xc] = 0x00;
  }
}

void BTDFile::getCellTerrainColor(std::uint16_t *buf, int cellX, int cellY,
                                  unsigned char l)
{
  size_t  n = 128 >> l;
  unsigned char m = 7 - l;
  for (size_t yc = 0; yc < n; yc++)
  {
    for (size_t xc = 0; xc < n; xc++)
      buf[(yc << m) | xc] = 0xFFFF;
  }
}

void BTDFile::getCellTextureSet(unsigned char *buf, int cellX, int cellY) const
{
  size_t  x = size_t(cellX - cellMinX);
  size_t  y = size_t(cellY - cellMinY);
  for (size_t q = 0; q < 4; q++)
  {
    size_t  offs =
        (((y << 1) | (q >> 1)) * (nCellsX << 1) + ((x << 1) | (q & 1))) << 3;
    unsigned char *t = buf + (q << 4);
    unsigned char baseTxt = 0xFF;
    for (size_t i = 0; i < 8; i++)
    {
      unsigned char tmp = readUInt8(ltexMapOffs + offs + i);
      if (tmp == 0 || tmp > ltexCnt)
        tmp = 0xFF;
      else
        baseTxt = --tmp;
      t[i] = tmp;
    }
    if (t[6] == 0xFF)
      t[6] = baseTxt;
    unsigned char *g = buf + ((q << 4) + 8);
    for (size_t i = 0; i < 8; i++)
      g[i] = 0xFF;
  }
}

