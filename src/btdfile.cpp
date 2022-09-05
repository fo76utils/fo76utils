
// Appalachia.btd file format:
//
// 0x00000000-0x00000003: "BTDB"
// 0x00000004-0x00000007: (int32) 6, format version
// 0x00000008-0x0000000B: (float) -700.0, mimimum height
// 0x0000000C-0x0000000F: (float) 38209.996, maximum height
// 0x00000010-0x00000013: (int32) 25728 (201 * 128), total resolution X
// 0x00000014-0x00000017: (int32) 25728 (201 * 128), total resolution Y
// 0x00000018-0x0000001B: (int32) -100, minimum cell X (west)
// 0x0000001C-0x0000001F: (int32) -100, minimum cell Y (south)
// 0x00000020-0x00000023: (int32) 100, maximum cell X (east)
// 0x00000024-0x00000027: (int32) 100, maximum cell Y (north)
// 0x00000028-0x0000002B: (int32) 43, number of land textures (LTEX records)
// 0x0000002C-0x000000D7: 43 * int32, LTEX form IDs
// 0x000000D8-0x0004EF5F: 201 * 201 * float[2], min, max height for each cell
// 0x0004EF60-0x0018A97F: 201 * 2 * 201 * 2 * byte[8], cell quadrant land
//                        textures (43 - LTEX_table_index, or zero if none)
// 0x0018A980-0x0018A983: (int32) 30, number of ground covers (GCVR records)
// 0x0018A984-0x0018A9FB: 30 * int32, GCVR form IDs
// 0x0018A9FC-0x002C641B: 201 * 2 * 201 * 2 * byte[8], cell quadrant ground
//                        covers (GCVR_table_index, or 0xFF if none)
// 0x002C641C-0x007B4C9B: 201 * 8 * 201 * 8 * int16, LOD4 height map,
//                        0 to 65535 maps to minimum height to maximum height
// 0x007B4C9C-0x00CA351B: 201 * 8 * 201 * 8 * int16, LOD4 land textures,
//                        bits[N * 3 to N * 3 + 2] = opacity of texture N,
//                        texture 6 is the base texture
// 0x00CA351C-0x01191D9B: 201 * 8 * 201 * 8 * int16, LOD4 terrain color
//                        type in packed 16-bit format, 5 bits per channel
// 0x01191D9C-0x01250643: 97557 * (int32, int32), pairs of zlib compressed data
//                        offset (relative to 0x01250644) and size
// 0x01250644-EOF:        zlib compressed data blocks
// zlib compressed blocks     0 to  1351 (26 * 26 * 2): LOD3, 8x8 cell groups
//                         1352 to  6553 (51 * 51 * 2): LOD2, 4x4 cell groups
//                         6554 to 16754   (101 * 101): LOD1, 2x2 cell groups
//                        16755 to 57155   (201 * 201): LOD0 height map, LTEX
//                        57156 to 97556   (201 * 201): LOD0 ground cover mask
// LOD3, LOD2: pairs of height map, LTEX and terrain color blocks
// LOD1: height map, land textures block only
// height map, LTEX block format:
//   0x0000-0x5FFF: 64 * (64, 128) int16, height map, only the samples
//                  not present at lower levels of detail
//   0x6000-0xBFFF: 64 * (64, 128) int16, land texture opacities
// terrain color block format (LOD3 and LOD2 only):
//   0x0000-0x5FFF: 64 * (64, 128) int16
//   0x6000-0x7FFF: unused
// ground cover block format (LOD0 only):
//   0x0000-0x3FFF: 128 * 128 bytes, bit N is set if ground cover N is enabled

#include "common.hpp"
#include "zlib.hpp"
#include "btdfile.hpp"

#include <thread>

void BTDFile::loadBlockLines_8(unsigned char *dst, const unsigned char *src)
{
  for (size_t i = 0; i < 128; i++, dst++, src++)
    *dst = *src;
  dst = dst + (1024 - 128);
  for (size_t i = 0; i < 128; i++, dst++, src++)
    *dst = *src;
}

void BTDFile::loadBlockLines_16(std::uint16_t *dst, const unsigned char *src,
                                size_t xd, size_t yd)
{
  for (size_t i = 0; i < 64; i++, dst = dst + xd, src = src + 2)
  {
    dst = dst + xd;
    *dst = FileBuffer::readUInt16Fast(src);
  }
  dst = dst + yd;
  for (size_t i = 0; i < 128; i++, dst = dst + xd, src = src + 2)
    *dst = FileBuffer::readUInt16Fast(src);
}

void BTDFile::loadBlock(TileData& tileData, size_t dataOffs,
                        size_t n, unsigned char l, unsigned char b,
                        std::vector< std::uint16_t >& zlibBuf)
{
  if (l >= 2)
    n = (n << 1) + b;
  else if (l == 0 && b != 0)
    n = n + (nCellsY * nCellsX);
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
    throw errorMessage("end of input file");
  unsigned char *p = reinterpret_cast< unsigned char * >(&(zlibBuf.front()));
  if (ZLibDecompressor::decompressData(p,
                                       zlibBuf.size() * sizeof(std::uint16_t),
                                       fileBuf + offs, compressedSize)
      != ((l == 0 && b != 0) ? 16384 : ((l >= 2 && b != 0) ? 32768 : 49152)))
  {
    throw errorMessage("error in compressed landscape data");
  }
  size_t  xd = 1 << (b == 0 || l == 0 ? l : (l - 2));
  size_t  yd = (b == 0 || l == 0 ? (1024 - 128) : ((256 - 128) >> 2)) << l;
  for (size_t y = 0; y < 128; y = y + 2)
  {
    if (b == 0)                 // vertex height
    {
      loadBlockLines_16(&(tileData.hmapData.front())
                        + dataOffs + (y << (l + 10)), p, xd, yd);
      p = p + 384;
    }
    else if (l == 0)            // ground cover
    {
      loadBlockLines_8(&(tileData.gcvrData.front()) + dataOffs + (y << 10), p);
      p = p + 256;
    }
    else                        // vertex color
    {
      loadBlockLines_16(&(tileData.vclrData.front())
                        + dataOffs + (y << (l + 6)), p, xd, yd);
      p = p + 384;
    }
  }
  if (b != 0)
    return;
  for (size_t y = 0; y < 128; y = y + 2)
  {
    // landscape textures
    loadBlockLines_16(&(tileData.ltexData.front())
                      + dataOffs + (y << (l + 10)), p, xd, yd);
    p = p + 384;
  }
}

void BTDFile::loadBlocks(TileData& tileData, size_t x, size_t y,
                         size_t threadIndex, size_t threadCnt,
                         unsigned int blockMask)
{
  std::vector< std::uint16_t >  zlibBuf(0x6000, 0);
  size_t  t = 0;
  // LOD3..LOD0
  for (unsigned char l = 4; l-- > 0; )
  {
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
          if ((blockMask & 0x55) & (1 << (l + l)))
            loadBlock(tileData, ((yy << 10) + xx) << (l + 7), n, l, 0, zlibBuf);
          if ((blockMask & 0xA0) & (1 << (l + l + 1)))
            loadBlock(tileData, ((yy << 8) + xx) << (l + 5), n, l, 1, zlibBuf);
          if ((blockMask & 0x02) & (1 << (l + l + 1)))
            loadBlock(tileData, ((yy << 10) + xx) << (l + 7), n, l, 1, zlibBuf);
        }
        if (++t >= threadCnt)
          t = 0;
      }
    }
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
  blockMask = (blockMask & ~(tileData.blockMask)) & 0x03F7;
  if (!blockMask)
    return tileData;

  if (blockMask & 0x0155)
  {
    tileData.hmapData.resize(0x00100000);
    tileData.ltexData.resize(0x00100000);
  }
  if (blockMask & 0x0002)
    tileData.gcvrData.resize(0x00100000);
  if (blockMask & 0x02A0)
    tileData.vclrData.resize(0x00010000);

  // LOD4
  if (blockMask & 0x0300)
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
        if (blockMask & 0x0155)
        {
          tileData.hmapData[((yy << 10) + xx) << 4] =
              readUInt16(heightMapLOD4 + (offs << 1));
          tileData.ltexData[((yy << 10) + xx) << 4] =
              readUInt16(landTexturesLOD4 + (offs << 1));
        }
        if (blockMask & 0x02A0)
        {
          tileData.vclrData[((yy << 8) + xx) << 2] =
              readUInt16(vertexColorLOD4 + (offs << 1));
        }
      }
    }
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
                                   this, &(errMsgs.front()) + i, &tileData,
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
      throw errorMessage("%s", errMsgs[i].c_str());
  }
  tileData.blockMask = tileData.blockMask | blockMask;
  return tileData;
}

BTDFile::BTDFile(const char *fileName)
  : FileBuffer(fileName)
{
  if (!checkType(readUInt32(), "BTDB"))
    throw errorMessage("input file format is not BTD");
  if (readUInt32() != 6U)
    throw errorMessage("unsupported BTD format version");
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
  gcvrCnt = readUInt32();
  gcvrOffs = filePos;
  for (size_t i = 0; i < gcvrCnt; i++)
    (void) readUInt32();
  gcvrMapOffs = filePos;
  filePos = filePos + ((nCellsY * nCellsX) << 5);
  heightMapLOD4 = filePos;
  filePos = filePos + ((nCellsY * nCellsX) << 7);
  landTexturesLOD4 = filePos;
  filePos = filePos + ((nCellsY * nCellsX) << 7);
  vertexColorLOD4 = filePos;
  filePos = filePos + ((nCellsY * nCellsX) << 7);
  zlibBlocksTableOffs = filePos;
  zlibBlkTableOffsLOD3 = filePos;
  filePos = filePos + (((nCellsY + 7) >> 3) * ((nCellsX + 7) >> 3) * 2 * 8);
  zlibBlkTableOffsLOD2 = filePos;
  filePos = filePos + (((nCellsY + 3) >> 2) * ((nCellsX + 3) >> 2) * 2 * 8);
  zlibBlkTableOffsLOD1 = filePos;
  filePos = filePos + (((nCellsY + 1) >> 1) * ((nCellsX + 1) >> 1) * 8);
  zlibBlkTableOffsLOD0 = filePos;
  filePos = filePos + (nCellsY * nCellsX * 2 * 8);
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
    TileData  *t = &(tileCache.front()) + i;
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

unsigned int BTDFile::getGroundCover(size_t n) const
{
  if (n >= gcvrCnt)
    return 0U;
  return readUInt32(gcvrOffs + (n << 2));
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
      unsigned int  tmp =
          tileData.ltexData[((y0 + (yc << l)) << 10) + x0 + (xc << l)];
      tmp = ((tmp & 0x7E00) >> 9) | (tmp & 0x01C0) | ((tmp & 0x003F) << 9);
      tmp = ((tmp & 0x7038) >> 3) | (tmp & 0x01C0) | ((tmp & 0x0E07) << 3);
      buf[(yc << m) | xc] = (std::uint16_t) tmp;
    }
  }
}

void BTDFile::getCellGroundCover(unsigned char *buf, int cellX, int cellY,
                                 unsigned char l)
{
  const TileData& tileData = loadTile(cellX, cellY, 0x0002);
  size_t  x = size_t(cellX - cellMinX);
  size_t  y = size_t(cellY - cellMinY);
  size_t  x0 = (x & 7) << 7;
  size_t  y0 = (y & 7) << 7;
  size_t  n = 128 >> l;
  unsigned char m = 7 - l;
  unsigned int  gcvrMask = 0;
  for (size_t yc = 0; yc < n; yc++)
  {
    for (size_t xc = 0; xc < n; xc++)
    {
      if (!(xc & ((n >> 1) - 1)))
      {
        size_t  offs = ((((y << 1) | (yc >> (m - 1))) * (nCellsX << 1)
                         + ((x << 1) | (xc >> (m - 1)))) << 3) + gcvrMapOffs;
        for (unsigned char i = 8; i-- > 0; offs++)
          gcvrMask |= ((unsigned int) (readUInt8(offs) < gcvrCnt) << i);
      }
      unsigned int  tmp =
          tileData.gcvrData[((y0 + (yc << l)) << 10) + x0 + (xc << l)];
      tmp = ((tmp & 0xF0) >> 4) | ((tmp & 0x0F) << 4);
      tmp = ((tmp & 0xCC) >> 2) | ((tmp & 0x33) << 2);
      tmp = (((tmp & 0xAA) >> 1) | ((tmp & 0x55) << 1)) & gcvrMask;
      buf[(yc << m) | xc] = (unsigned char) tmp;
    }
  }
}

void BTDFile::getCellTerrainColor(std::uint16_t *buf, int cellX, int cellY,
                                  unsigned char l)
{
  const TileData& tileData = loadTile(cellX, cellY, (~0U << (l + l)) & 0x02A0U);
  size_t  x0 = size_t((cellX - cellMinX) & 7) << 5;
  size_t  y0 = size_t((cellY - cellMinY) & 7) << 5;
  size_t  n = 128 >> l;
  unsigned char m = 7 - l;
  for (size_t yc = 0; yc < n; yc++)
  {
    size_t  yy = (l >= 2 ? (yc << (l - 2)) : (yc >> (2 - l)));
    for (size_t xc = 0; xc < n; xc++)
    {
      size_t  xx = (l >= 2 ? (xc << (l - 2)) : (xc >> (2 - l)));
      buf[(yc << m) | xc] = tileData.vclrData[((y0 + yy) << 8) + x0 + xx];
    }
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
    for (size_t i = 0; i < 8; i++)
    {
      unsigned char tmp = readUInt8(ltexMapOffs + offs + i);
      if (tmp == 0 || tmp > ltexCnt)
        tmp = 0xFF;
      else
        tmp = (unsigned char) (ltexCnt - tmp);
      if (i < 5)
        t[5 - i] = tmp;
      if (tmp != 0xFF)
        t[0] = tmp;             // default texture
    }
    t[6] = 0xFF;
    t[7] = 0xFF;
    unsigned char *g = buf + ((q << 4) + 8);
    for (size_t i = 0; i < 8; i++)
    {
      unsigned char tmp = readUInt8(gcvrMapOffs + offs + i);
      if (tmp >= gcvrCnt)
        tmp = 0xFF;
      g[7 - i] = tmp;
    }
  }
}

