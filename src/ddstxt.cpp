
#include "common.hpp"
#include "ddstxt.hpp"

void DDSTexture::decodeBlock_BC1(unsigned int *dst, const unsigned char *src)
{
  unsigned int  c[4];
  unsigned int  c0 = ((unsigned int) src[1] << 8) | src[0];
  unsigned int  c1 = ((unsigned int) src[3] << 8) | src[2];
  unsigned int  r0 = (((c0 >> 11) & 0x1F) * 255 + 15) / 31;
  unsigned int  g0 = (((c0 >> 5) & 0x3F) * 255 + 31) / 63;
  unsigned int  b0 = ((c0 & 0x1F) * 255 + 15) / 31;
  unsigned int  r1 = (((c1 >> 11) & 0x1F) * 255 + 15) / 31;
  unsigned int  g1 = (((c1 >> 5) & 0x3F) * 255 + 31) / 63;
  unsigned int  b1 = ((c1 & 0x1F) * 255 + 15) / 31;
  c[0] = r0 | (g0 << 8) | (b0 << 16) | 0xFF000000U;
  c[1] = r1 | (g1 << 8) | (b1 << 16) | 0xFF000000U;
  c[2] = ((r0 + r0 + r1 + 1) / 3) | (((g0 + g0 + g1 + 1) / 3) << 8)
         | (((b0 + b0 + b1 + 1) / 3) << 16) | 0xFF000000U;
  c[3] = ((r0 + r1 + r1 + 1) / 3) | (((g0 + g1 + g1 + 1) / 3) << 8)
         | (((b0 + b1 + b1 + 1) / 3) << 16) | 0xFF000000U;
  unsigned int  bc = 0;
  for (unsigned int i = 0; i < 4; i++)
    bc = bc | ((unsigned long long) src[i + 4] << (i << 3));
  for (unsigned int i = 0; i < 16; i++)
  {
    dst[i] = c[bc & 3];
    bc = bc >> 2;
  }
}

void DDSTexture::decodeBlock_BC3(unsigned int *dst, const unsigned char *src)
{
  unsigned int  c[4];
  unsigned int  a[8];
  a[0] = src[0];
  a[1] = src[1];
  unsigned int  c0 = ((unsigned int) src[9] << 8) | src[8];
  unsigned int  c1 = ((unsigned int) src[11] << 8) | src[10];
  unsigned int  r0 = (((c0 >> 11) & 0x1F) * 255 + 15) / 31;
  unsigned int  g0 = (((c0 >> 5) & 0x3F) * 255 + 31) / 63;
  unsigned int  b0 = ((c0 & 0x1F) * 255 + 15) / 31;
  unsigned int  r1 = (((c1 >> 11) & 0x1F) * 255 + 15) / 31;
  unsigned int  g1 = (((c1 >> 5) & 0x3F) * 255 + 31) / 63;
  unsigned int  b1 = ((c1 & 0x1F) * 255 + 15) / 31;
  c[0] = r0 | (g0 << 8) | (b0 << 16);
  c[1] = r1 | (g1 << 8) | (b1 << 16);
  c[2] = ((r0 + r0 + r1 + 1) / 3) | (((g0 + g0 + g1 + 1) / 3) << 8)
         | (((b0 + b0 + b1 + 1) / 3) << 16);
  c[3] = ((r0 + r1 + r1 + 1) / 3) | (((g0 + g1 + g1 + 1) / 3) << 8)
         | (((b0 + b1 + b1 + 1) / 3) << 16);
  if (a[0] > a[1])
  {
    for (unsigned int i = 1; i < 7; i++)
      a[i + 1] = ((a[0] * (7 - i)) + (a[1] * i) + 3) / 7;
  }
  else
  {
    for (unsigned int i = 1; i < 5; i++)
      a[i + 1] = ((a[0] * (5 - i)) + (a[1] * i) + 2) / 5;
    a[6] = 0;
    a[7] = 255;
  }
  unsigned long long  ba = 0;
  unsigned int        bc = 0;
  for (unsigned int i = 0; i < 6; i++)
    ba = ba | ((unsigned long long) src[i + 2] << (i << 3));
  for (unsigned int i = 0; i < 4; i++)
    bc = bc | ((unsigned long long) src[i + 12] << (i << 3));
  for (unsigned int i = 0; i < 16; i++)
  {
    dst[i] = c[bc & 3] | (a[ba & 7] << 24);
    ba = ba >> 3;
    bc = bc >> 2;
  }
}

void DDSTexture::loadTexture(FileBuffer& buf)
{
  buf.setPosition(0);
  if (buf.size() < 148 || !FileBuffer::checkType(buf.readUInt32(), "DDS "))
    throw errorMessage("unsupported texture file format");
  if (buf.readUInt32() != 0x7C)         // size of DDS_HEADER
    throw errorMessage("unsupported texture file format");
  unsigned int  flags = buf.readUInt32();
  if ((flags & 0x1006) != 0x1006)       // height, width, pixel format required
    throw errorMessage("unsupported texture file format");
  mipLevelCnt = 1;
  haveAlpha = false;
  ySizeMip0 = buf.readUInt32();
  xSizeMip0 = buf.readUInt32();
  if (ySizeMip0 < 4 || ySizeMip0 > 32768 ||
      xSizeMip0 < 4 || xSizeMip0 > 32768 ||
      (ySizeMip0 & (ySizeMip0 - 1)) != 0 || (xSizeMip0 & (xSizeMip0 - 1)) != 0)
  {
    throw errorMessage("invalid or unsupported texture dimensions");
  }
  (void) buf.readUInt32();              // dwPitchOrLinearSize
  (void) buf.readUInt32();              // dwDepth
  if (flags & 0x00020000)               // DDSD_MIPMAPCOUNT
  {
    mipLevelCnt = buf.readInt32();
    if (mipLevelCnt < 1 || mipLevelCnt > 16)
      throw errorMessage("invalid mipmap count");
  }
  buf.setPosition(0x4C);                // ddspf
  if (buf.readUInt32() != 0x20)         // size of DDS_PIXELFORMAT
    throw errorMessage("unsupported texture file format");
  if (!(buf.readUInt32() & 0x04))       // DDPF_FOURCC is required
    throw errorMessage("unsupported texture file format");
  unsigned int  fourCC = buf.readUInt32();
  unsigned int  dataOffs = 128;
  if (FileBuffer::checkType(fourCC, "DX10"))
  {
    dataOffs = 148;
    buf.setPosition(0x80);
    unsigned int  tmp = buf.readUInt32();
    switch (tmp)
    {
      case 0x47:                        // DXGI_FORMAT_BC1_UNORM
      case 0x48:                        // DXGI_FORMAT_BC1_UNORM_SRGB
        break;
      case 0x4D:                        // DXGI_FORMAT_BC3_UNORM
      case 0x4E:                        // DXGI_FORMAT_BC3_UNORM_SRGB
        haveAlpha = true;
        break;
      default:
        throw errorMessage("unsupported DXGI_FORMAT: 0x%08X", tmp);
    }
  }
  else if (FileBuffer::checkType(fourCC, "DXT5"))
  {
    haveAlpha = true;
  }
  else if (!FileBuffer::checkType(fourCC, "DXT1"))
  {
    throw errorMessage("unsupported DDS fourCC: 0x%08X",
                       FileBuffer::swapUInt32(fourCC));
  }
  size_t  sizeRequired = dataOffs;
  for (int i = 0; i < mipLevelCnt; i++)
  {
    unsigned int  w = xSizeMip0 >> (i + 2);
    unsigned int  h = ySizeMip0 >> (i + 2);
    if (w < 1)
      w = 1;
    if (h < 1)
      h = 1;
    sizeRequired = sizeRequired + (size_t(w) * h * (haveAlpha ? 16U : 8U));
  }
  if (buf.size() < sizeRequired)
    throw errorMessage("DDS file is shorter than expected");

  size_t  dataOffsets[20];
  size_t  bufSize = 0;
  for (int i = 0; i < 20; i++)
  {
    unsigned int  w = xSizeMip0 >> i;
    unsigned int  h = ySizeMip0 >> i;
    if (w < 1)
      w = 1;
    if (h < 1)
      h = 1;
    dataOffsets[i] = bufSize;
    bufSize = bufSize + (size_t(w) * h);
  }
  textureDataBuf.resize(bufSize);
  const unsigned char *srcPtr = buf.getDataPtr() + dataOffs;
  for (int i = 0; i < 20; i++)
  {
    unsigned int  *p = &(textureDataBuf.front()) + dataOffsets[i];
    textureData[i] = p;
    unsigned int  w = xSizeMip0 >> i;
    unsigned int  h = ySizeMip0 >> i;
    if (w < 1)
      w = 1;
    if (h < 1)
      h = 1;
    if (i < mipLevelCnt && w >= 4 && h >= 4)
    {
      unsigned int  tmpBuf[16];
      for (unsigned int y = 0; y < h; y = y + 4)
      {
        for (unsigned int x = 0; x < w; x = x + 4)
        {
          if (!haveAlpha)
          {
            decodeBlock_BC1(tmpBuf, srcPtr);
            srcPtr = srcPtr + 8;
          }
          else
          {
            decodeBlock_BC3(tmpBuf, srcPtr);
            srcPtr = srcPtr + 16;
          }
          for (unsigned int j = 0; j < 16; j++)
            p[(y + (j >> 2)) * w + (x + (j & 3))] = tmpBuf[j];
        }
      }
    }
    else
    {
      for (unsigned int y = 0; y < h; y++)
      {
        for (unsigned int x = 0; x < w; x++)
        {
          unsigned int  c0 = getPixelN(x << 1, y << 1, i - 1);
          unsigned int  c1 = getPixelN((x << 1) + 1, y << 1, i - 1);
          unsigned int  c2 = getPixelN(x << 1, (y << 1) + 1, i - 1);
          unsigned int  c3 = getPixelN((x << 1) + 1, (y << 1) + 1, i - 1);
          c0 = blendRGBA32(c0, c1, 128);
          c2 = blendRGBA32(c2, c3, 128);
          p[y * w + x] = blendRGBA32(c0, c2, 128);
        }
      }
    }
  }
}

DDSTexture::DDSTexture(const char *fileName)
{
  FileBuffer  tmpBuf(fileName);
  loadTexture(tmpBuf);
}

DDSTexture::DDSTexture(const unsigned char *buf, size_t bufSize)
{
  FileBuffer  tmpBuf(buf, bufSize);
  loadTexture(tmpBuf);
}

DDSTexture::DDSTexture(FileBuffer& buf)
{
  loadTexture(buf);
}

DDSTexture::~DDSTexture()
{
}

unsigned int DDSTexture::getPixelB(float x, float y, int mipLevel) const
{
  int     x0 = int(x * 256.0f + (x < 0.0f ? -0.5f : 0.5f));
  int     y0 = int(y * 256.0f + (y < 0.0f ? -0.5f : 0.5f));
  int     xf = x0 & 0xFF;
  int     yf = y0 & 0xFF;
  x0 = (x0 & 0x7FFFFF00) >> 8;
  y0 = (y0 & 0x7FFFFF00) >> 8;
  unsigned int  c0 = blendRGBA32(getPixelN(x0, y0, mipLevel),
                                 getPixelN(x0 + 1, y0, mipLevel), xf);
  unsigned int  c1 = blendRGBA32(getPixelN(x0, y0 + 1, mipLevel),
                                 getPixelN(x0 + 1, y0 + 1, mipLevel), xf);
  return blendRGBA32(c0, c1, yf);
}

unsigned int DDSTexture::getPixelT(float x, float y, float mipLevel) const
{
  int     x0 = int(x * 256.0f + (x < 0.0f ? -0.5f : 0.5f));
  int     y0 = int(y * 256.0f + (y < 0.0f ? -0.5f : 0.5f));
  int     m0 = int(mipLevel * 256.0f + (mipLevel < 0.0f ? -0.5f : 0.5f));
  int     x0m1 = (x0 & 0x7FFFFFFF) >> 1;
  int     y0m1 = (y0 & 0x7FFFFFFF) >> 1;
  int     xf = x0 & 0xFF;
  int     yf = y0 & 0xFF;
  int     mf = m0 & 0xFF;
  x0 = (x0 & 0x7FFFFF00) >> 8;
  y0 = (y0 & 0x7FFFFF00) >> 8;
  m0 = (m0 & 0x7FFFFF00) >> 8;
  int     m1 = m0 + 1;
  int     xfm1 = x0m1 & 0xFF;
  int     yfm1 = y0m1 & 0xFF;
  x0m1 = (x0m1 & 0x7FFFFF00) >> 8;
  y0m1 = (y0m1 & 0x7FFFFF00) >> 8;
  unsigned int  c0 = blendRGBA32(getPixelN(x0, y0, m0),
                                 getPixelN(x0 + 1, y0, m0), xf);
  unsigned int  c1 = blendRGBA32(getPixelN(x0, y0 + 1, m0),
                                 getPixelN(x0 + 1, y0 + 1, m0), xf);
  unsigned int  c = blendRGBA32(c0, c1, yf);
  c0 = blendRGBA32(getPixelN(x0m1, y0m1, m1),
                   getPixelN(x0m1 + 1, y0m1, m1), xfm1);
  c1 = blendRGBA32(getPixelN(x0m1, y0m1 + 1, m1),
                   getPixelN(x0m1 + 1, y0m1 + 1, m1), xfm1);
  return blendRGBA32(c, blendRGBA32(c0, c1, yfm1), mf);
}

