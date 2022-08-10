
#ifndef ZLIB_HPP_INCLUDED
#define ZLIB_HPP_INCLUDED

#include "common.hpp"

class ZLibDecompressor
{
 protected:
  const unsigned char *inPtr;
  const unsigned char *inBufEnd;
  unsigned long long  sr;
  unsigned int  tableBuf[1312];         // 320 * 3 + 32 + 32 + 288
  // huffTable[N] (0 <= N <= 255):
  //     fast decode table for code lengths <= 8, contains length | (C << 8),
  //     where C is the decoded symbol, or N bit reversed if length > 8
  // huffTable[len + 255] (1 <= len <= 16):
  //     limit table, len bits of input is valid if less than the table element
  // huffTable[len + 287] (1 <= len <= 16):
  //     table of signed offset to be added to the input for symbol table offset
  // huffTable[C + 320] (0 <= C <= 31, or 287 for literals):
  //     table of decoded symbols
  inline unsigned int *getHuffTable(size_t n)
  {
    // n = 0: code lengths
    // n = 1: distances
    // n = 2: literals / lengths
    return (tableBuf + (n * (256 + 64 + 32)));
  }
  inline unsigned char readU8()
  {
    if (inPtr >= inBufEnd)
      throw errorMessage("end of ZLib compressed data");
    unsigned char c = *(inPtr++);
    return c;
  }
  inline unsigned short readU16LE()
  {
    if ((inPtr + 1) >= inBufEnd)
      throw errorMessage("end of ZLib compressed data");
    unsigned short  w = *(inPtr++);
    w = w | ((unsigned short) *(inPtr++) << 8);
    return w;
  }
  unsigned int readU32LE();
  unsigned long long readU64LE();
  unsigned short readU16BE();
  unsigned int readU32BE();
  inline void srLoad();
  inline void srReset()
  {
    unsigned long long  tmp = sr;
    while (tmp >= 0x0100ULL)
    {
      tmp = tmp >> 8;
      inPtr--;
    }
    sr = 1;
  }
  // read bits packed in Deflate order (LSB first - LSB first)
  // nBits must be in the range 1 to 16
  inline unsigned int readBitsRR(unsigned char nBits);
  size_t decompressLZ4(unsigned char *buf, size_t uncompressedSize);
  void huffmanInit(bool useFixedEncoding);
  // lenTbl[c] = length of symbol 'c' (0: not used)
  static void huffmanBuildDecodeTable(
      unsigned int *huffTable, const unsigned char *lenTbl, size_t lenTblSize);
  inline unsigned int huffmanDecode(const unsigned int *huffTable);
  unsigned char *decompressZLibBlock(unsigned char *wp,
                                     unsigned char *buf, unsigned char *bufEnd,
                                     unsigned int& a1, unsigned int& a2);
  size_t decompressZLib(unsigned char *buf, size_t uncompressedSize);
  ZLibDecompressor(const unsigned char *inBuf, size_t compressedSize)
    : inPtr(inBuf),
      inBufEnd(inBuf + compressedSize),
      sr(1)
  {
  }
 public:
  static size_t decompressData(unsigned char *buf, size_t uncompressedSize,
                               const unsigned char *inBuf,
                               size_t compressedSize);
};

#endif

