
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
// 0x00CA351C-0x01191D9B: 201 * 8 * 201 * 8 * int16, LOD4 unknown 3-channel map
//                        type in packed 16-bit format, 5 bits per channel
// 0x01191D9C-0x01250643: 97557 * (int32, int32), pairs of zlib compressed data
//                        offset (relative to 0x01250644) and size
// 0x01250644-EOF:        zlib compressed data blocks
// zlib compressed blocks     0 to  1351 (26 * 26 * 2): LOD3, 8x8 cell groups
//                         1352 to  6553 (51 * 51 * 2): LOD2, 4x4 cell groups
//                         6554 to 16754   (101 * 101): LOD1, 2x2 cell groups
//                        16755 to 57155   (201 * 201): LOD0 height map, LTEX
//                        57156 to 97556   (201 * 201): LOD0 ground cover mask
// LOD3, LOD2: pairs of height map, LTEX and unknown map blocks
// LOD1: height map, land textures block only
// height map, LTEX block format:
//   0x0000-0x5FFF: 64 * (64, 128) int16, height map, only the samples
//                  not present at lower levels of detail
//   0x6000-0xBFFF: 64 * (64, 128) int16, land texture opacities
// unknown map block format (LOD3 and LOD2 only):
//   0x0000-0x5FFF: 64 * (64, 128) int16
//   0x6000-0x7FFF: unused
// ground cover block format (LOD0 only):
//   0x0000-0x3FFF: 128 * 128 bytes, bit N is set if ground cover N is enabled

#include "common.hpp"
#include "btdfile.hpp"

#include <thread>

void BTDFile::loadBlockLines_8(unsigned short *dst, const unsigned char *src,
                               unsigned char l)
{
  size_t  xd = 1 << l;
  for (size_t i = 0; i < 128; i++, dst = dst + xd, src++)
    *dst = *src;
  dst = dst + ((1024 - 128) << l);
  for (size_t i = 0; i < 128; i++, dst = dst + xd, src++)
    *dst = *src;
}

void BTDFile::loadBlockLines_16(unsigned short *dst, const unsigned char *src,
                                unsigned char l)
{
  size_t  xd = 1 << l;
  unsigned short  tmp = 1;
  if (sizeof(unsigned short) == 2 &&
      *(reinterpret_cast< unsigned char * >(&tmp)) == 1)
  {
    for (size_t i = 0; i < 64; i++, dst = dst + xd, src = src + 2)
    {
      dst = dst + xd;
      *dst = *(reinterpret_cast< const unsigned short * >(src));
    }
    dst = dst + ((1024 - 128) << l);
    for (size_t i = 0; i < 128; i++, dst = dst + xd, src = src + 2)
      *dst = *(reinterpret_cast< const unsigned short * >(src));
  }
  else
  {
    for (size_t i = 0; i < 64; i++, dst = dst + xd, src = src + 2)
    {
      dst = dst + xd;
      *dst = (unsigned short) src[0] | ((unsigned short) src[1] << 8);
    }
    dst = dst + ((1024 - 128) << l);
    for (size_t i = 0; i < 128; i++, dst = dst + xd, src = src + 2)
      *dst = (unsigned short) src[0] | ((unsigned short) src[1] << 8);
  }
}

void BTDFile::loadBlock(unsigned short *tileData, size_t n,
                        unsigned char l, unsigned char b,
                        ZLibDecompressor& zlibDecompressor,
                        std::vector< unsigned short >& zlibBuf)
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
  if (zlibDecompressor.decompressData(p,
                                      zlibBuf.size() * sizeof(unsigned short),
                                      fileBuf + offs, compressedSize)
      != ((l == 0 && b != 0) ? 16384 : ((l >= 2 && b != 0) ? 32768 : 49152)))
  {
    throw errorMessage("error in compressed landscape data");
  }
  for (size_t y = 0; y < 128; y = y + 2)
  {
    if (b == 0)                 // vertex height
    {
      loadBlockLines_16(tileData + (y << (l + 10)), p, l);
      p = p + 384;
    }
    else if (l == 0)            // ground cover
    {
      loadBlockLines_8(tileData + (y << (l + 10)) + 0x00200000, p, l);
      p = p + 256;
    }
    else                        // unknown
    {
      loadBlockLines_16(tileData + (y << (l + 10)) + 0x00300000, p, l);
      p = p + 384;
    }
  }
  if (b != 0)
    return;
  for (size_t y = 0; y < 128; y = y + 2)
  {
    // landscape textures
    loadBlockLines_16(tileData + (y << (l + 10)) + 0x00100000, p, l);
    p = p + 384;
  }
}

void BTDFile::loadBlocks(unsigned short *tileData, size_t x, size_t y,
                         size_t threadIndex, size_t threadCnt)
{
  ZLibDecompressor  zlibDecompressor;
  std::vector< unsigned short > zlibBuf(0x6000, 0);
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
          unsigned short  *p =
              tileData + (((yy << l) << 17) + ((xx << l) << 7));
          loadBlock(p, n, l, 0, zlibDecompressor, zlibBuf);
          if (l != 1)
            loadBlock(p, n, l, 1, zlibDecompressor, zlibBuf);
        }
        if (++t >= threadCnt)
          t = 0;
      }
    }
  }
}

void BTDFile::loadBlocksThread(BTDFile *p, std::string *errMsg,
                               unsigned short *tileData, size_t x, size_t y,
                               size_t threadIndex, size_t threadCnt)
{
  try
  {
    p->loadBlocks(tileData, x, y, threadIndex, threadCnt);
  }
  catch (std::exception& e)
  {
    *errMsg = std::string(e.what());
  }
}

void BTDFile::loadTile(int cellX, int cellY)
{
  if (cellX >= curTileX && cellX <= (curTileX + 7) &&
      cellY >= curTileY && cellY <= (curTileY + 7))
  {
    return;
  }
  {
    int     tmp = curTileX;
    curTileX = prvTileX;
    prvTileX = tmp;
    tmp = curTileY;
    curTileY = prvTileY;
    prvTileY = tmp;
    unsigned short  *tmp2 = curTileData;
    curTileData = prvTileData;
    prvTileData = tmp2;
  }
  if (cellX >= curTileX && cellX <= (curTileX + 7) &&
      cellY >= curTileY && cellY <= (curTileY + 7))
  {
    return;
  }
  size_t  x = size_t(cellX - cellMinX) & ~7U;
  size_t  y = size_t(cellY - cellMinY) & ~7U;
  curTileX = int(x) + cellMinX;
  curTileY = int(y) + cellMinY;
  // LOD4
  for (size_t yy = 0; yy < 64; yy++)
  {
    if ((curTileY + int(yy >> 3)) > cellMaxY)
      break;
    for (size_t xx = 0; xx < 64; xx++)
    {
      if ((curTileX + int(xx >> 3)) > cellMaxX)
        break;
      filePos = heightMapLOD4
                + (((((y << 3) + yy) * (nCellsX << 3)) + ((x << 3) + xx)) << 1);
      curTileData[(((yy << 10) + xx) << 4) + 0x00000000] = readUInt16();
      filePos = landTexturesLOD4
                + (((((y << 3) + yy) * (nCellsX << 3)) + ((x << 3) + xx)) << 1);
      curTileData[(((yy << 10) + xx) << 4) + 0x00100000] = readUInt16();
      filePos = unknownLOD4
                + (((((y << 3) + yy) * (nCellsX << 3)) + ((x << 3) + xx)) << 1);
      curTileData[(((yy << 10) + xx) << 4) + 0x00300000] = readUInt16();
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
                                   this, &(errMsgs.front()) + i, curTileData,
                                   x, y, i, threadCnt);
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
}

BTDFile::BTDFile(const char *fileName)
  : FileBuffer(fileName)
{
  if (!checkType(readUInt32(), "BTDB"))
    throw errorMessage("input file format is not BTD");
  if (readUInt32() != 6U)
    throw errorMessage("unsupported BTD format version");
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
  unknownLOD4 = filePos;
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
  curTileX = 0x7FFFFFF0;
  curTileY = 0x7FFFFFF0;
  prvTileX = 0x7FFFFFF0;
  prvTileY = 0x7FFFFFF0;
  tileBuf.resize(128 * 128 * 4 * 8 * 8 * 2, 0);
  curTileData = &(tileBuf.front());
  prvTileData = curTileData + (128 * 128 * 4 * 8 * 8);
}

BTDFile::~BTDFile()
{
}

unsigned int BTDFile::getLandTexture(size_t n)
{
  if (n >= ltexCnt)
    return 0U;
  filePos = ltexOffs + (n << 2);
  return readUInt32();
}

unsigned int BTDFile::getGroundCover(size_t n)
{
  if (n >= gcvrCnt)
    return 0U;
  filePos = gcvrOffs + (n << 2);
  return readUInt32();
}

void BTDFile::getCellHeightMap(unsigned short *buf, int cellX, int cellY)
{
  loadTile(cellX, cellY);
  size_t  x0 = size_t((cellX - cellMinX) & 7) << 7;
  size_t  y0 = size_t((cellY - cellMinY) & 7) << 7;
  for (size_t yc = 0; yc < 128; yc++)
  {
    for (size_t xc = 0; xc < 128; xc++)
      buf[(yc << 7) + xc] = curTileData[((y0 + yc) << 10) + x0 + xc];
  }
}

void BTDFile::getCellLandTexture(unsigned char *buf, int cellX, int cellY)
{
  loadTile(cellX, cellY);
  size_t  x = size_t(cellX - cellMinX);
  size_t  y = size_t(cellY - cellMinY);
  size_t  x0 = (x & 7) << 7;
  size_t  y0 = (y & 7) << 7;
  unsigned char l[8];
  for (size_t i = 0; i < 8; i++)
    l[i] = 0xFF;
  for (size_t q = 0; q < 4; q++)
  {
    unsigned char defaultTexture = 0xFF;
    size_t  offs =
        ((y << 1) | (q >> 1)) * (nCellsX << 1) + ((x << 1) | (q & 1));
    filePos = (offs << 3) + ltexMapOffs;
    for (size_t i = 0; i < 8; i++)
    {
      unsigned char tmp = readUInt8();
      if (tmp == 0 || tmp > ltexCnt)
        tmp = 0xFF;
      else
        tmp = (unsigned char) (ltexCnt - tmp);
      l[i] = tmp;
      if (tmp != 0xFF)
        defaultTexture = tmp;
    }
    for (size_t yy = 0; yy < 64; yy++)
    {
      for (size_t xx = 0; xx < 64; xx++)
      {
        size_t  xc = ((q & 1) << 6) | xx;
        size_t  yc = ((q & 2) << 5) | yy;
        unsigned char *bufp = buf + ((((yc << 7) | xc) << 4) + 12);
        for (size_t i = 0; i < 4; i++)
          bufp[i] = (unsigned char) (((i & 1) - 1) & 0xFF);
        unsigned int  tmp =
            curTileData[((y0 + yc) << 10) + x0 + xc + 0x00100000];
        for (int i = 0; i < 5; i++, tmp = tmp >> 3)
        {
          bufp = bufp - 2;
          bufp[0] = l[i];
          bufp[1] = (unsigned char) (((tmp & 7) * 73) >> 1);
        }
        *(bufp - 2) = defaultTexture;
        *(bufp - 1) = 0xFF;
      }
    }
  }
}

void BTDFile::getCellGroundCover(unsigned char *buf, int cellX, int cellY)
{
  loadTile(cellX, cellY);
  size_t  x = size_t(cellX - cellMinX);
  size_t  y = size_t(cellY - cellMinY);
  size_t  x0 = (x & 7) << 7;
  size_t  y0 = (y & 7) << 7;
  unsigned char g[8];
  for (size_t i = 0; i < 8; i++)
    g[i] = 0xFF;
  for (size_t q = 0; q < 4; q++)
  {
    size_t  offs =
        ((y << 1) | (q >> 1)) * (nCellsX << 1) + ((x << 1) | (q & 1));
    filePos = (offs << 3) + gcvrMapOffs;
    for (size_t i = 0; i < 8; i++)
    {
      unsigned char tmp = readUInt8();
      if (tmp >= gcvrCnt)
        tmp = 0xFF;
      g[i] = tmp;
    }
    for (size_t yy = 0; yy < 64; yy++)
    {
      for (size_t xx = 0; xx < 64; xx++)
      {
        size_t  xc = ((q & 1) << 6) | xx;
        size_t  yc = ((q & 2) << 5) | yy;
        unsigned char *bufp = buf + (((yc << 7) | xc) << 3);
        unsigned int  tmp =
            curTileData[((y0 + yc) << 10) + x0 + xc + 0x00200000];
        for (int i = 0; i < 8; i++)
          bufp[7 - i] = g[i] | (unsigned char) (~(tmp << (7 - i)) & 0x80);
      }
    }
  }
}

void BTDFile::getCellData4(unsigned short *buf, int cellX, int cellY)
{
  loadTile(cellX, cellY);
  size_t  x = size_t(cellX - cellMinX);
  size_t  y = size_t(cellY - cellMinY);
  size_t  x0 = (x & 7) << 7;
  size_t  y0 = (y & 7) << 7;
  for (size_t yc = 0; yc < 128; yc = yc + 4)
  {
    for (size_t xc = 0; xc < 128; xc = xc + 4)
    {
      unsigned short  *bufp = buf + ((yc << 7) | xc);
      unsigned short  tmp =
          curTileData[((y0 + yc) << 10) + x0 + xc + 0x00300000];
      for (int i = 0; i < 16; i++)
        bufp[((i >> 2) << 7) | (i & 3)] = tmp;
    }
  }
}

