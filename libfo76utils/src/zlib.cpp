
#include "common.hpp"
#include "zlib.hpp"
#include "fp32vec4.hpp"

std::uint32_t ZLibDecompressor::readU32LE()
{
  std::uint32_t w = readU16LE();
  w = w | ((std::uint32_t) readU16LE() << 16);
  return w;
}

std::uint64_t ZLibDecompressor::readU64LE()
{
  std::uint64_t w = readU32LE();
  w = w | ((std::uint64_t) readU32LE() << 32);
  return w;
}

std::uint16_t ZLibDecompressor::readU16BE()
{
  std::uint16_t w = readU8();
  w = (w << 8) | readU8();
  return w;
}

std::uint32_t ZLibDecompressor::readU32BE()
{
  std::uint32_t w = readU16BE();
  w = (w << 16) | readU16BE();
  return w;
}

inline void ZLibDecompressor::srLoad(unsigned long long& sr)
{
  if (sr < 0x00010000ULL) [[unlikely]]
  {
    unsigned int  bitCnt = (unsigned int) std::bit_width(sr) - 1U;
#if defined(__i386__) || defined(__x86_64__) || defined(__x86_64)
    if ((inPtr + 6) <= inBufEnd) [[likely]]
    {
      // inPtr - 2 is always valid because of the 2 bytes long zlib header
      const std::uint64_t *p =
          reinterpret_cast< const std::uint64_t * >(inPtr - 2);
      sr = (*p >> (16U - bitCnt)) | (1ULL << (bitCnt + 48U));
      inPtr = inPtr + 6;
    }
    else if ((inPtr + 2) <= inBufEnd) [[likely]]
    {
      const std::uint32_t *p =
          reinterpret_cast< const std::uint32_t * >(inPtr - 2);
      sr = (unsigned long long) *p | (1ULL << 32);
      sr = sr >> (16U - bitCnt);
      inPtr = inPtr + 2;
    }
    else
    {
      errorMessage("end of ZLib compressed data");
    }
#else
    if ((inPtr + 2) > inBufEnd) [[unlikely]]
      errorMessage("end of ZLib compressed data");
    sr &= ~(1ULL << bitCnt);
    do
    {
      sr = sr | ((unsigned long long) *(inPtr++) << bitCnt);
      bitCnt = bitCnt + 8;
    }
    while (bitCnt < 56 && inPtr < inBufEnd);
    sr |= (1ULL << bitCnt);
#endif
  }
}

inline void ZLibDecompressor::srReset(unsigned long long& sr)
{
  unsigned long long  tmp = sr;
  while (tmp >= 0x0100ULL)
  {
    tmp = tmp >> 8;
    inPtr--;
  }
  sr = 1;
}

inline unsigned int ZLibDecompressor::readBitsRR(
    unsigned long long& sr, unsigned char nBits, unsigned int prefix)
{
  srLoad(sr);
  unsigned int  w =
      ((unsigned int) sr & ((1U << nBits) - 1U)) | (prefix << nBits);
  sr = sr >> nBits;
  return w;
}

size_t ZLibDecompressor::decompressLZ4(unsigned char *buf,
                                       size_t uncompressedSize)
{
  if (readU16BE() != 0x4D18)
    errorMessage("invalid LZ4 frame header");
  unsigned char flags = readU8();
  if ((flags & 0xC3) != 0x40)
    errorMessage("unsupported LZ4 version or format flags");
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
        errorMessage("uncompressed LZ4 data larger than output buffer");
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
            errorMessage("invalid or corrupt LZ4 compressed data");
          c = readU8();
          litLen = litLen + c;
        }
        while (c == 0xFF);
      }
      if (litLen > blockSize)
        errorMessage("invalid or corrupt LZ4 compressed data");
      if (litLen > bytesLeft)
        errorMessage("uncompressed LZ4 data larger than output buffer");
      blockSize = blockSize - litLen;
      bytesLeft = bytesLeft - litLen;
      for ( ; litLen; wp++, litLen--)
        *wp = readU8();
      size_t  lzLen = t & 0x0F;
      if (!blockSize && !lzLen)
        break;
      if (blockSize < 2)
        errorMessage("invalid or corrupt LZ4 compressed data");
      blockSize = blockSize - 2;
      size_t  offs = readU16LE();
      if (offs < 1 || offs > size_t(wp - buf))
        errorMessage("invalid or corrupt LZ4 compressed data");
      if (lzLen == 15)
      {
        unsigned char c;
        do
        {
          if (!(blockSize--))
            errorMessage("invalid or corrupt LZ4 compressed data");
          c = readU8();
          lzLen = lzLen + c;
        }
        while (c == 0xFF);
      }
      lzLen = lzLen + 4;
      if (lzLen > bytesLeft)
        errorMessage("uncompressed LZ4 data larger than output buffer");
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
    errorMessage("invalid or corrupt LZ4 compressed data");

  return uncompressedSize;
}

unsigned long long ZLibDecompressor::huffmanInit(
    unsigned long long sr, bool useFixedEncoding)
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
    return sr;
  }

  // dynamic Huffman code
  unsigned int  *huffTableC = getHuffTable(0);  // code length codes
  unsigned char *lenTbl = reinterpret_cast< unsigned char * >(huffTableC);
  size_t  hLit = readBitsRR(sr, 5) + 257;
  size_t  hDist = readBitsRR(sr, 5) + 1;
  size_t  hCLen = readBitsRR(sr, 4) + 4;
  for (size_t i = 0; i < 19; i++)
    lenTbl[i] = 0;
  for (size_t i = 0; i < hCLen; i++)
  {
    size_t  j = (i < 3 ? (i + 16) : ((i & 1) == 0 ?
                                     ((i >> 1) + 6) : (((19 - i) >> 1) & 7)));
    lenTbl[j] = (unsigned char) readBitsRR(sr, 3);
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
      unsigned char c = (unsigned char) huffmanDecode(sr, huffTableC);
      if (c < 16)
      {
        lenTbl[i] = c;
        rleCode = c;
      }
      else if (c == 16)
      {
        lenTbl[i] = rleCode;
        rleLen = (unsigned char) readBitsRR(sr, 2) + 2;
      }
      else
      {
        lenTbl[i] = 0;
        rleCode = 0;
        if (c == 17)
          rleLen = (unsigned char) readBitsRR(sr, 3) + 2;
        else
          rleLen = (unsigned char) readBitsRR(sr, 7) + 10;
      }
    }
    huffmanBuildDecodeTable((!t ? huffTableL : huffTableD), lenTbl, n);
  }
  return sr;
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
      n = std::rotl(std::uint8_t(n), 4);
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
    unsigned long long& sr, const unsigned int *huffTable)
{
  srLoad(sr);
  unsigned int  b = huffTable[sr & 0xFF];
  unsigned char nBits = (unsigned char) (b & 0xFF);
  b = b >> 8;
  if (nBits < 9) [[likely]]
  {
    sr = sr >> nBits;
    return b;
  }
  if (nBits > 16)
  {
    errorMessage("invalid Huffman code in ZLib compressed data");
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
    errorMessage("invalid Huffman code in ZLib compressed data");
  }
  return huffTable[b];
}

unsigned char * ZLibDecompressor::decompressZLibBlock(
    unsigned long long& srRef, unsigned char *wp,
    unsigned char *buf, unsigned char *bufEnd)
{
  unsigned int  *huffTableL = getHuffTable(2);  // literals and length codes
  unsigned int  *huffTableD = getHuffTable(1);  // distance codes
  unsigned long long  sr = srRef;
  while (true)
  {
    unsigned int  c = huffmanDecode(sr, huffTableL);
    if (c < 256)                    // literal byte
    {
      if (wp >= bufEnd)
        errorMessage("uncompressed ZLib data larger than output buffer");
      *(wp++) = (unsigned char) c;
      continue;
    }
    size_t  lzLen = c - 254;
    if (!(lzLen >= 3 && lzLen <= 10)) [[unlikely]]
    {
      if (!(lzLen >= 11 && lzLen <= 30)) [[unlikely]]
      {
        if (lzLen == 31)
          lzLen = 258;
        else if (c == 256)
          break;
        else
          errorMessage("invalid or corrupt ZLib compressed data");
      }
      else
      {
        lzLen = lzLen - 7;
        unsigned char nBits = (unsigned char) (lzLen >> 2);
        lzLen = readBitsRR(sr, nBits, (unsigned int) ((lzLen & 3) | 4)) + 3;
      }
    }
    size_t  offs = huffmanDecode(sr, huffTableD);
    if (offs >= 4)
    {
      if (offs >= 30)
      {
        errorMessage("invalid or corrupt ZLib compressed data");
      }
      offs = offs - 2;
      unsigned char nBits = (unsigned char) (offs >> 1);
      offs = readBitsRR(sr, nBits, (unsigned int) ((offs & 1) | 2));
    }
    offs++;
    if (offs > size_t(wp - buf))
    {
      errorMessage("invalid LZ77 offset in ZLib compressed data");
    }
    const unsigned char *rp = wp - offs;
    if ((wp + lzLen) > bufEnd)
    {
      errorMessage("uncompressed ZLib data larger than output buffer");
    }
    // copy LZ77 sequence
    *(wp++) = *(rp++);
    *(wp++) = *(rp++);
    *(wp++) = *(rp++);
    if (lzLen == 3) [[likely]]
      continue;
    *(wp++) = *(rp++);
    if (lzLen == 4) [[likely]]
      continue;
    switch (lzLen & 3)
    {
      case 3:
        *(wp++) = *(rp++);
        [[fallthrough]];
      case 2:
        *(wp++) = *(rp++);
        [[fallthrough]];
      case 1:
        *(wp++) = *(rp++);
    }
    for ( ; lzLen >= 8; lzLen = lzLen - 4)
    {
      *(wp++) = *(rp++);
      *(wp++) = *(rp++);
      *(wp++) = *(rp++);
      *(wp++) = *(rp++);
    }
  }
  srRef = sr;
  return wp;
}

size_t ZLibDecompressor::decompressZLib(unsigned char *buf,
                                        size_t uncompressedSize)
{
  unsigned char *wp = buf;
  unsigned char *bufEnd = buf + uncompressedSize;
  unsigned long long  sr = 1;
  unsigned int  a = 1U;                 // Adler-32 checksum
  while (true)
  {
    unsigned char *wpPrv = wp;
    // read Deflate block header
    unsigned char bhdr = (unsigned char) readBitsRR(sr, 3);
    if (!(bhdr & 6))                    // no compression
    {
      srReset(sr);
      size_t  len = readU16LE();
      if ((len ^ readU16LE()) != 0xFFFF)
      {
        errorMessage("invalid or corrupt ZLib compressed data");
      }
      if ((wp + len) > bufEnd)
      {
        errorMessage("uncompressed ZLib data larger than output buffer");
      }
      for ( ; len; wp++, len--)
      {
        *wp = readU8();
      }
    }
    else if ((bhdr & 6) == 6)           // reserved (invalid)
    {
      errorMessage("invalid Deflate block type in ZLib compressed data");
    }
    else                                // compressed block
    {
      sr = huffmanInit(sr, !(bhdr & 4));
      wp = decompressZLibBlock(sr, wp, buf, bufEnd);
    }
    // update Adler-32 checksum
    a = calculateAdler32(wpPrv, size_t(wp - wpPrv), a);
    if (bhdr & 1)
    {
      // final block
      break;
    }
  }
  srReset(sr);
  // verify Adler-32 checksum
  if (readU32BE() != a)
  {
    errorMessage("checksum error in ZLib compressed data");
  }

  return size_t(wp - buf);
}

unsigned int ZLibDecompressor::calculateAdler32(
    const unsigned char *buf, size_t bufSize, unsigned int a)
{
  unsigned int  s1 = a & 0xFFFFU;
  unsigned int  s2 = a >> 16;
  size_t  i = 0;
#if ENABLE_X86_64_SIMD >= 2
  while ((i + 16) <= bufSize)
  {
    static const XMM_UInt16 mulTbl1 = { 16, 15, 14, 13, 12, 11, 10, 9 };
    static const XMM_UInt16 mulTbl2 = { 8, 7, 6, 5, 4, 3, 2, 1 };
    XMM_UInt32  aTmp = { s1, 0U, s2, 0U };
    size_t  n = std::min((bufSize - i) >> 4, size_t(256));
    do
    {
      XMM_UInt16  tmp1, tmp2, tmp3;
      const std::uint64_t *p =
          reinterpret_cast< const std::uint64_t * >(buf + i);
      __asm__ ("vpmovzxbw %1, %0" : "=x" (tmp1) : "m" (*p));
      __asm__ ("vpmovzxbw %1, %0" : "=x" (tmp2) : "m" (p[1]));
      __asm__ ("vpslldq $0x08, %1, %0" : "=x" (tmp3) : "x" (aTmp));
      __asm__ ("vpslld $0x04, %0, %0" : "+x" (tmp3));
      __asm__ ("vpaddd %1, %0, %0" : "+x" (aTmp) : "x" (tmp3));
      __asm__ ("vpmullw %2, %1, %0" : "=x" (tmp3) : "x" (tmp1), "xm" (mulTbl1));
      __asm__ ("vpaddw %1, %0, %0" : "+x" (tmp1) : "x" (tmp2));
      __asm__ ("vpmullw %1, %0, %0" : "+x" (tmp2) : "xm" (mulTbl2));
      __asm__ ("vpaddw %1, %0, %0" : "+x" (tmp2) : "x" (tmp3));
      __asm__ ("vpshufd $0x4e, %1, %0" : "=x" (tmp3) : "x" (tmp1));
      __asm__ ("vpaddw %1, %0, %0" : "+x" (tmp1) : "x" (tmp3));
      __asm__ ("vpshufd $0x4e, %1, %0" : "=x" (tmp3) : "x" (tmp2));
      __asm__ ("vpaddw %1, %0, %0" : "+x" (tmp2) : "x" (tmp3));
      __asm__ ("vpblendw $0xf0, %1, %0, %0" : "+x" (tmp1) : "x" (tmp2));
      __asm__ ("vpshufd $0xb1, %1, %0" : "=x" (tmp3) : "x" (tmp1));
      __asm__ ("vpaddw %1, %0, %0" : "+x" (tmp1) : "x" (tmp3));
      __asm__ ("vpslld $0x10, %1, %0" : "=x" (tmp3) : "x" (tmp1));
      __asm__ ("vpaddd %1, %0, %0" : "+x" (tmp1) : "x" (tmp3));
      __asm__ ("vpsrld $0x10, %0, %0" : "+x" (tmp1));
      __asm__ ("vpaddd %1, %0, %0" : "+x" (aTmp) : "x" (tmp1));
      i = i + 16;
    }
    while (--n);
    s1 = aTmp[0] % 65521U;
    s2 = aTmp[2] % 65521U;
  }
#else
  while ((i + 4) <= bufSize)
  {
    size_t  n = std::min((bufSize - i) >> 2, size_t(256));
    do
    {
      s1 = s1 + buf[i];
      s2 = s2 + s1;
      s1 = s1 + buf[i + 1];
      s2 = s2 + s1;
      s1 = s1 + buf[i + 2];
      s2 = s2 + s1;
      s1 = s1 + buf[i + 3];
      s2 = s2 + s1;
      i = i + 4;
    }
    while (--n);
    s1 = s1 % 65521U;
    s2 = s2 % 65521U;
  }
#endif
  for ( ; i < bufSize; i++)
  {
    s1 = s1 + buf[i];
    s2 = s2 + s1;
  }
  return ((s1 % 65521U) | ((s2 % 65521U) << 16));
}

size_t ZLibDecompressor::decompressData(
    unsigned char *buf, size_t uncompressedSize,
    const unsigned char *inBuf, size_t compressedSize)
{
  ZLibDecompressor  zlibDecompressor(inBuf, compressedSize);
  // CMF, FLG
  std::uint16_t h = zlibDecompressor.readU16BE();
  if (h == 0x0422)
    return zlibDecompressor.decompressLZ4(buf, uncompressedSize);
  if ((h & 0x8F20) != 0x0800 || (h % 31) != 0)
    errorMessage("invalid or unsupported ZLib compression method");
  return zlibDecompressor.decompressZLib(buf, uncompressedSize);
}

size_t ZLibDecompressor::decompressLZ4Raw(
    unsigned char *buf, size_t uncompressedSize,
    const unsigned char *inBuf, size_t compressedSize)
{
  unsigned char *wp = buf;
  size_t  bytesLeft = uncompressedSize;
  const unsigned char *inBufEnd = inBuf + compressedSize;
  while (inBuf < inBufEnd)
  {
    unsigned char t = *(inBuf++);       // token
    size_t  litLen = t >> 4;
    if (litLen == 15)
    {
      unsigned char c;
      do
      {
        if (inBuf >= inBufEnd)
          errorMessage("invalid or corrupt LZ4 compressed data");
        c = *(inBuf++);
        litLen = litLen + c;
      }
      while (c == 0xFF);
    }
    if ((inBuf + litLen) > inBufEnd)
      errorMessage("invalid or corrupt LZ4 compressed data");
    if (litLen > bytesLeft)
      errorMessage("uncompressed LZ4 data larger than output buffer");
    bytesLeft = bytesLeft - litLen;
    for ( ; litLen; wp++, litLen--)
      *wp = *(inBuf++);
    size_t  lzLen = t & 0x0F;
    if (inBuf >= inBufEnd && !lzLen)
      break;
    if ((inBuf + 2) > inBufEnd)
      errorMessage("invalid or corrupt LZ4 compressed data");
    size_t  offs = size_t(inBuf[0]) | (size_t(inBuf[1]) << 8);
    inBuf = inBuf + 2;
    if (offs < 1 || offs > size_t(wp - buf))
      errorMessage("invalid or corrupt LZ4 compressed data");
    if (lzLen == 15)
    {
      unsigned char c;
      do
      {
        if (inBuf >= inBufEnd)
          errorMessage("invalid or corrupt LZ4 compressed data");
        c = *(inBuf++);
        lzLen = lzLen + c;
      }
      while (c == 0xFF);
    }
    lzLen = lzLen + 4;
    if (lzLen > bytesLeft)
      errorMessage("uncompressed LZ4 data larger than output buffer");
    bytesLeft = bytesLeft - lzLen;
    const unsigned char *rp = wp - offs;
    for ( ; lzLen; rp++, wp++, lzLen--)
      *wp = *rp;
  }
  if (bytesLeft)
    uncompressedSize = uncompressedSize - bytesLeft;

  return uncompressedSize;
}

