
#include "common.hpp"
#include "zlib.hpp"
#include "fp32vec4.hpp"

unsigned int ZLibDecompressor::readU32LE()
{
  unsigned int  w = readU16LE();
  w = w | ((unsigned int) readU16LE() << 16);
  return w;
}

unsigned long long ZLibDecompressor::readU64LE()
{
  unsigned long long  w = readU32LE();
  w = w | ((unsigned long long) readU32LE() << 32);
  return w;
}

unsigned short ZLibDecompressor::readU16BE()
{
  unsigned short  w = readU8();
  w = (w << 8) | readU8();
  return w;
}

unsigned int ZLibDecompressor::readU32BE()
{
  unsigned int  w = readU16BE();
  w = (w << 16) | readU16BE();
  return w;
}

inline void ZLibDecompressor::srLoad()
{
  if (BRANCH_EXPECT(sr < 0x00010000ULL, false))
  {
#if defined(__i386__) || defined(__x86_64__) || defined(__x86_64)
    int     bitCnt = FloatVector4::log2Int(int(sr)) + 127;
    if (BRANCH_EXPECT((inPtr + 6) <= inBufEnd, true))
    {
      // inPtr - 2 is always valid because of the 2 bytes long zlib header
      const unsigned long long  *p =
          reinterpret_cast< const unsigned long long * >(inPtr - 2);
      sr = (*p >> 1) | (1ULL << 63);
      sr = sr >> (unsigned int) (142 - bitCnt);
      inPtr = inPtr + 6;
    }
    else if (BRANCH_EXPECT((inPtr + 2) <= inBufEnd, true))
    {
      const unsigned int  *p =
          reinterpret_cast< const unsigned int * >(inPtr - 2);
      sr = (unsigned long long) *p | (1ULL << 32);
      sr = sr >> (unsigned int) (143 - bitCnt);
      inPtr = inPtr + 2;
    }
    else
    {
      throw errorMessage("end of ZLib compressed data");
    }
#else
    if (BRANCH_EXPECT((inPtr + 2) > inBufEnd, false))
      throw errorMessage("end of ZLib compressed data");
    unsigned int  srBitCnt = (unsigned int) FloatVector4::log2Int(int(sr));
    sr &= ~(1ULL << srBitCnt);
    do
    {
      sr = sr | ((unsigned long long) *(inPtr++) << srBitCnt);
      srBitCnt = srBitCnt + 8;
    }
    while (srBitCnt < 56 && inPtr < inBufEnd);
    sr |= (1ULL << srBitCnt);
#endif
  }
}

inline unsigned int ZLibDecompressor::readBitsRR(unsigned char nBits)
{
  srLoad();
  unsigned int  w = (unsigned int) sr & ((1U << nBits) - 1U);
  sr = sr >> nBits;
  return w;
}

size_t ZLibDecompressor::decompressLZ4(unsigned char *buf,
                                       size_t uncompressedSize)
{
  if (readU16BE() != 0x4D18)
    throw errorMessage("invalid LZ4 frame header");
  unsigned char flags = readU8();
  if ((flags & 0xC3) != 0x40)
    throw errorMessage("unsupported LZ4 version or format flags");
  (void) readU8();                      // block maximum size
  size_t  contentSize = uncompressedSize;
  if (flags & 0x08)
  {
    contentSize = size_t(readU64LE());
  }
  // FIXME: all LZ4 checksums are ignored for now
  (void) readU8();                      // header checksum

  unsigned char *wp = buf;
  size_t  bytesLeft = uncompressedSize;
  while (true)
  {
    size_t  blockSize = readU32LE();
    if (!blockSize)                     // EndMark
      break;
    if (blockSize & 0x80000000U)        // uncompressed block
    {
      blockSize = blockSize & 0x7FFFFFFF;
      if (blockSize > bytesLeft)
      {
        throw errorMessage("uncompressed LZ4 data larger than output buffer");
      }
      bytesLeft = bytesLeft - blockSize;
      for ( ; blockSize; wp++, blockSize--)
        *wp = readU8();
      if (flags & 0x10)
      {
        (void) readU32LE();             // block checksum
      }
      continue;
    }
    while (blockSize)                   // compressed block
    {
      unsigned char t = readU8();       // token
      blockSize--;
      size_t  litLen = t >> 4;
      if (litLen == 15)
      {
        unsigned char c;
        do
        {
          if (!(blockSize--))
            throw errorMessage("invalid or corrupt LZ4 compressed data");
          c = readU8();
          litLen = litLen + c;
        }
        while (c == 0xFF);
      }
      if (litLen > blockSize)
        throw errorMessage("invalid or corrupt LZ4 compressed data");
      if (litLen > bytesLeft)
        throw errorMessage("uncompressed LZ4 data larger than output buffer");
      blockSize = blockSize - litLen;
      bytesLeft = bytesLeft - litLen;
      for ( ; litLen; wp++, litLen--)
        *wp = readU8();
      size_t  lzLen = t & 0x0F;
      if (!blockSize && !lzLen)
        break;
      if (blockSize < 2)
        throw errorMessage("invalid or corrupt LZ4 compressed data");
      blockSize = blockSize - 2;
      size_t  offs = readU16LE();
      if (offs < 1 || offs > size_t(wp - buf))
        throw errorMessage("invalid or corrupt LZ4 compressed data");
      if (lzLen == 15)
      {
        unsigned char c;
        do
        {
          if (!(blockSize--))
            throw errorMessage("invalid or corrupt LZ4 compressed data");
          c = readU8();
          lzLen = lzLen + c;
        }
        while (c == 0xFF);
      }
      lzLen = lzLen + 4;
      if (lzLen > bytesLeft)
        throw errorMessage("uncompressed LZ4 data larger than output buffer");
      bytesLeft = bytesLeft - lzLen;
      const unsigned char *rp = wp - offs;
      for ( ; lzLen; rp++, wp++, lzLen--)
        *wp = *rp;
    }
    if (flags & 0x10)
    {
      (void) readU32LE();               // block checksum
    }
  }

  if (flags & 0x04)
  {
    (void) readU32LE();                 // content checksum
  }
  if (bytesLeft)
    uncompressedSize = uncompressedSize - bytesLeft;
  if ((flags & 0x08) != 0 && uncompressedSize != contentSize)
    throw errorMessage("invalid or corrupt LZ4 compressed data");

  return uncompressedSize;
}

void ZLibDecompressor::huffmanInit(bool useFixedEncoding)
{
  unsigned int  *huffTableL = getHuffTable(2);  // literals and length codes
  unsigned int  *huffTableD = getHuffTable(1);  // distance codes

  if (useFixedEncoding)
  {
    // fixed Huffman code
    unsigned char *lenTbl = reinterpret_cast< unsigned char * >(huffTableL);
    for (size_t i = 0; i < 288; i++)
      lenTbl[i] = (i < 144 ? 8 : (i < 256 ? 9 : (i < 280 ? 7 : 8)));
    huffmanBuildDecodeTable(huffTableL, lenTbl, 288);
    lenTbl = reinterpret_cast< unsigned char * >(huffTableD);
    for (size_t i = 0; i < 32; i++)
      lenTbl[i] = 5;
    huffmanBuildDecodeTable(huffTableD, lenTbl, 32);
    return;
  }

  // dynamic Huffman code
  unsigned int  *huffTableC = getHuffTable(0);  // code length codes
  unsigned char *lenTbl = reinterpret_cast< unsigned char * >(huffTableC);
  size_t  hLit = readBitsRR(5) + 257;
  size_t  hDist = readBitsRR(5) + 1;
  size_t  hCLen = readBitsRR(4) + 4;
  for (size_t i = 0; i < 19; i++)
    lenTbl[i] = 0;
  for (size_t i = 0; i < hCLen; i++)
  {
    size_t  j = (i < 3 ? (i + 16) : ((i & 1) == 0 ?
                                     ((i >> 1) + 6) : (((19 - i) >> 1) & 7)));
    lenTbl[j] = (unsigned char) readBitsRR(3);
  }
  huffmanBuildDecodeTable(huffTableC, lenTbl, 19);
  unsigned char rleCode = 0;
  unsigned char rleLen = 0;
  for (size_t t = 0; t < 2; t++)
  {
    lenTbl = reinterpret_cast< unsigned char * >(!t ? huffTableL : huffTableD);
    size_t  n = (t == 0 ? hLit : hDist);
    for (size_t i = 0; i < n; i++)
    {
      if (rleLen)
      {
        lenTbl[i] = rleCode;
        rleLen--;
        continue;
      }
      unsigned char c = (unsigned char) huffmanDecode(huffTableC);
      if (c < 16)
      {
        lenTbl[i] = c;
        rleCode = c;
      }
      else if (c == 16)
      {
        lenTbl[i] = rleCode;
        rleLen = (unsigned char) readBitsRR(2) + 2;
      }
      else
      {
        lenTbl[i] = 0;
        rleCode = 0;
        if (c == 17)
          rleLen = (unsigned char) readBitsRR(3) + 2;
        else
          rleLen = (unsigned char) readBitsRR(7) + 10;
      }
    }
    huffmanBuildDecodeTable((!t ? huffTableL : huffTableD), lenTbl, n);
  }
}

void ZLibDecompressor::huffmanBuildDecodeTable(
    unsigned int *huffTable, const unsigned char *lenTbl, size_t lenTblSize)
{
  for (size_t i = 256; i < 288; i++)
    huffTable[i] = 0;
  unsigned int  maxLen = 0;
  for (size_t i = 0; i < lenTblSize; i++)
  {
    unsigned int  len = lenTbl[i];
    if (!len)
      continue;
    huffTable[len + 255] = huffTable[len + 255] + 1;
    if (len > maxLen)
      maxLen = len;
  }
  huffTable[288] = 320;
  for (size_t i = 1; i < 32; i++)
  {
    huffTable[i + 288] = huffTable[i + 287] + huffTable[i + 255];
    huffTable[i + 255] = 0;
  }
  for (size_t i = 0; i < lenTblSize; i++)
  {
    unsigned int  len = lenTbl[i];
    if (len)
    {
      huffTable[huffTable[len + 287] + huffTable[len + 255]] = (unsigned int) i;
      huffTable[len + 255] = huffTable[len + 255] + 1;
    }
  }
  for (size_t i = 0; i < 256; i++)
    huffTable[i] = 0xFFFFFFFFU;
  for (unsigned int l = 1; l <= maxLen; l++)
  {
    unsigned int  r = 0;
    if (l > 1)
    {
      r = huffTable[l + 254] << 1;
      huffTable[l + 255] = huffTable[l + 255] + r;
      huffTable[l + 287] = huffTable[l + 287] - r;
    }
    // build fast decode table
    for ( ; r < huffTable[l + 255]; r++)
    {
      unsigned int  c = (r << 8) >> l;
      unsigned int  n = ((c & 0x55) << 1) | ((c & 0xAA) >> 1);
      n = ((n & 0x33) << 2) | ((n & 0xCC) >> 2);
      n = ((n & 0x0F) << 4) | ((n & 0xF0) >> 4);
      if (l <= 8)
        c = huffTable[huffTable[l + 287] + r];
      c = (c << 8) | l;
      do
      {
        huffTable[n] = c;
        n = n + (1U << l);
      }
      while (n < 256);
    }
  }
  for (unsigned int l = maxLen + 1; l <= 32; l++)
  {
    // invalid lengths
    huffTable[l + 255] = 0xFFFFFFFFU;
    huffTable[l + 287] = 0x80000000U;
  }
}

// huffTable[N+256] = limit (last valid code + 1) for code length N+1
// huffTable[N+288] = base index in huffTable for code length N+1

inline unsigned int ZLibDecompressor::huffmanDecode(
    const unsigned int *huffTable)
{
  srLoad();
  unsigned int  b = huffTable[sr & 0xFF];
  unsigned char nBits = (unsigned char) (b & 0xFF);
  b = b >> 8;
  if (BRANCH_EXPECT(nBits < 9, true))
  {
    sr = sr >> nBits;
    return b;
  }
  if (nBits > 16)
  {
    throw errorMessage("invalid Huffman code in ZLib compressed data");
  }
  sr = sr >> 8;
  const unsigned int  *t = huffTable + (256 + 8 - 1);
  do
  {
    t++;
    b = (b << 1) | ((unsigned int) sr & 1U);
    sr = sr >> 1;
  }
  while (b >= *t);
  b = b + t[32];
  if (b & 0x80000000U)
  {
    throw errorMessage("invalid Huffman code in ZLib compressed data");
  }
  return huffTable[b];
}

unsigned char * ZLibDecompressor::decompressZLibBlock(
    unsigned char *wp, unsigned char *buf, unsigned char *bufEnd,
    unsigned int& a1, unsigned int& a2)
{
  unsigned int  *huffTableL = getHuffTable(2);  // literals and length codes
  unsigned int  *huffTableD = getHuffTable(1);  // distance codes
  unsigned int  s1 = a1;
  unsigned int  s2 = a2;
  while (true)
  {
    if (BRANCH_EXPECT(s2 & 0x80000000U, false))
    {
      // update Adler-32 checksum
      s1 = s1 % 65521U;
      s2 = s2 % 65521U;
    }
    unsigned int  c = huffmanDecode(huffTableL);
    if (c < 256)                    // literal byte
    {
      if (wp >= bufEnd)
      {
        throw errorMessage("uncompressed ZLib data larger than output buffer");
      }
      *wp = (unsigned char) c;
      s1 = s1 + c;
      s2 = s2 + s1;
      wp++;
      continue;
    }
    size_t  lzLen = c - 254;
    if (BRANCH_EXPECT(!(lzLen >= 3 && lzLen <= 10), false))
    {
      if (BRANCH_EXPECT(!(lzLen >= 11 && lzLen <= 30), false))
      {
        if (lzLen == 31)
          lzLen = 258;
        else if (c == 256)
          break;
        else
          throw errorMessage("invalid or corrupt ZLib compressed data");
      }
      else
      {
        unsigned char nBits = (unsigned char) ((lzLen - 7) >> 2);
        lzLen = ((((lzLen - 7) & 3) | 4) << nBits) + readBitsRR(nBits) + 3;
      }
    }
    size_t  offs = huffmanDecode(huffTableD);
    if (offs >= 4)
    {
      if (offs >= 30)
      {
        throw errorMessage("invalid or corrupt ZLib compressed data");
      }
      unsigned char nBits = (unsigned char) ((offs - 2) >> 1);
      offs = ((((offs - 2) & 1) | 2) << nBits) | readBitsRR(nBits);
    }
    offs++;
    if (offs > size_t(wp - buf))
    {
      throw errorMessage("invalid LZ77 offset in ZLib compressed data");
    }
    const unsigned char *rp = wp - offs;
    if ((wp + lzLen) > bufEnd)
    {
      throw errorMessage("uncompressed ZLib data larger than output buffer");
    }
    // copy LZ77 sequence
    s1 = s1 + *rp;
    s2 = s2 + s1;
    *(wp++) = *(rp++);
    s1 = s1 + *rp;
    s2 = s2 + s1;
    *(wp++) = *(rp++);
    s1 = s1 + *rp;
    s2 = s2 + s1;
    *(wp++) = *(rp++);
    while (lzLen-- > 3)
    {
      s1 = s1 + *rp;
      s2 = s2 + s1;
      *(wp++) = *(rp++);
    }
  }
  a1 = s1;
  a2 = s2;
  return wp;
}

size_t ZLibDecompressor::decompressZLib(unsigned char *buf,
                                        size_t uncompressedSize)
{
  unsigned char *wp = buf;
  unsigned char *bufEnd = buf + uncompressedSize;
  unsigned int  s1 = 1;                 // Adler-32 checksum
  unsigned int  s2 = 0;
  while (true)
  {
    // read Deflate block header
    unsigned char bhdr = (unsigned char) readBitsRR(3);
    if (!(bhdr & 6))                    // no compression
    {
      srReset();
      size_t  len = readU16LE();
      if ((len ^ readU16LE()) != 0xFFFF)
      {
        throw errorMessage("invalid or corrupt ZLib compressed data");
      }
      if ((wp + len) > bufEnd)
      {
        throw errorMessage("uncompressed ZLib data larger than output buffer");
      }
      for ( ; len; wp++, len--)
      {
        *wp = readU8();
        // update Adler-32 checksum
        s1 = s1 + *wp;
        s2 = s2 + s1;
        if (BRANCH_EXPECT(s2 & 0x80000000U, false))
        {
          s1 = s1 % 65521U;
          s2 = s2 % 65521U;
        }
      }
    }
    else if ((bhdr & 6) == 6)           // reserved (invalid)
    {
      throw errorMessage("invalid Deflate block type in ZLib compressed data");
    }
    else                                // compressed block
    {
      huffmanInit(!(bhdr & 4));
      wp = decompressZLibBlock(wp, buf, bufEnd, s1, s2);
    }
    s1 = s1 % 65521U;
    s2 = s2 % 65521U;
    if (bhdr & 1)
    {
      // final block
      break;
    }
  }
  srReset();
  // verify Adler-32 checksum
  if (readU32BE() != ((s2 << 16) | s1))
  {
    throw errorMessage("checksum error in ZLib compressed data");
  }

  return size_t(wp - buf);
}

size_t ZLibDecompressor::decompressData(unsigned char *buf,
                                        size_t uncompressedSize,
                                        const unsigned char *inBuf,
                                        size_t compressedSize)
{
  ZLibDecompressor  zlibDecompressor(inBuf, compressedSize);
  // CMF, FLG
  unsigned short  h = zlibDecompressor.readU16BE();
  if (h == 0x0422)
    return zlibDecompressor.decompressLZ4(buf, uncompressedSize);
  if ((h & 0x8F20) != 0x0800 || (h % 31) != 0)
    throw errorMessage("invalid or unsupported ZLib compression method");
  return zlibDecompressor.decompressZLib(buf, uncompressedSize);
}

