
#include "common.hpp"
#include "terrmesh.hpp"
#include "landtxt.hpp"
#include "fp32vec4.hpp"

#include <bit>

static inline FloatVector4 calculateNormal(FloatVector4 v1, FloatVector4 v2)
{
  // cross product of v1 and v2
  FloatVector4  tmp((v1[1] * v2[2]) - (v1[2] * v2[1]),
                    (v1[2] * v2[0]) - (v1[0] * v2[2]),
                    (v1[0] * v2[1]) - (v1[1] * v2[0]), 0.0f);
  return tmp.normalize3Fast();
}

TerrainMesh::TerrainMesh()
  : NIFFile::NIFTriShape()
{
  for (size_t i = 0; i < (sizeof(landTexture) / sizeof(DDSTexture *)); i++)
    landTexture[i] = (DDSTexture *) 0;
}

TerrainMesh::~TerrainMesh()
{
  for (size_t i = 0; i < (sizeof(landTexture) / sizeof(DDSTexture *)); i++)
  {
    if (landTexture[i])
      delete landTexture[i];
  }
}

void TerrainMesh::createMesh(
    const std::uint16_t *hmapData, int hmapWidth, int hmapHeight,
    const unsigned char * const *ltexData, unsigned int ltexMask,
    int ltexWidth, int ltexHeight, int textureScale,
    int x0, int y0, int x1, int y1, int cellResolution,
    float xOffset, float yOffset, float zMin, float zMax)
{
  int     w = std::abs(x1 - x0) + 1;
  int     h = std::abs(y1 - y0) + 1;
  int     txtW = (w + 4) << textureScale;
  int     txtH = (h + 4) << textureScale;
  x0 = (x0 < x1 ? x0 : x1);
  y0 = (y0 < y1 ? y0 : y1);
  x1 = x0 + w - 1;
  y1 = y0 + h - 1;
  int     txtWP2 = 1 << int(std::bit_width((unsigned int) txtW - 1U));
  int     txtHP2 = 1 << int(std::bit_width((unsigned int) txtH - 1U));
  // create vertex and triangle data
  vertexCnt = (unsigned int) w * (unsigned int) h;
  if (vertexCnt > 65536U)
    errorMessage("TerrainMesh: vertex count is out of range");
  triangleCnt = ((unsigned int) (w - 1) * (unsigned int) (h - 1)) << 1;
  vertexDataBuf.resize(vertexCnt);
  triangleDataBuf.resize(triangleCnt);
  vertexData = vertexDataBuf.data();
  triangleData = triangleDataBuf.data();
  m.s.specularColor = FloatVector4(1.0f);
  m.s.specularSmoothness = 1.0f;
  NIFFile::NIFVertex    *vertexPtr = vertexDataBuf.data();
  NIFFile::NIFTriangle  *trianglePtr = triangleDataBuf.data();
  float   xyScale = 100.0f / float(cellResolution);
  float   zScale = (zMax - zMin) / 65535.0f;
  m.textureScaleU = 1.0f / float(txtWP2 >> textureScale);
  m.textureScaleV = 1.0f / float(txtHP2 >> textureScale);
  for (int y = y1; y >= y0; y--)
  {
    for (int x = x0; x <= x1; x++, vertexPtr++)
    {
      static const int  xOffsTable[9] = { -1, 0, 1, -1, 0, 1, -1, 0, 1 };
      static const int  yOffsTable[9] = { -1, -1, -1, 0, 0, 0, 1, 1, 1 };
      float   z[9];
      for (int i = 0; i < 9; i++)
      {
        int     xc = x + xOffsTable[i];
        int     yc = y + yOffsTable[i];
        xc = (xc > 0 ? (xc < (hmapWidth - 1) ? xc : (hmapWidth - 1)) : 0);
        yc = (yc > 0 ? (yc < (hmapHeight - 1) ? yc : (hmapHeight - 1)) : 0);
        z[i] = float(int(hmapData[size_t(yc) * size_t(hmapWidth) + size_t(xc)]))
               * zScale + zMin;
      }
      vertexPtr->x = xOffset + (float(x) * xyScale);
      vertexPtr->y = yOffset - (float(y) * xyScale);
      vertexPtr->z = z[4];
      FloatVector4  v_n(0.0f, xyScale, z[1] - z[4], 0.0f);
      FloatVector4  v_w(-xyScale, 0.0f, z[3] - z[4], 0.0f);
      FloatVector4  v_s(0.0f, -xyScale, z[7] - z[4], 0.0f);
      FloatVector4  v_e(xyScale, 0.0f, z[5] - z[4], 0.0f);
      FloatVector4  normal;
      if ((x ^ y) & 1)
      {
        //    0 1 2
        // -1 +-+-+
        //    |\|/|
        //  0 +-+-+
        //    |/|\|
        //  1 +-+-+
        FloatVector4  v_nw(-xyScale, xyScale, z[0] - z[4], 0.0f);
        FloatVector4  v_sw(-xyScale, -xyScale, z[6] - z[4], 0.0f);
        FloatVector4  v_se(xyScale, -xyScale, z[8] - z[4], 0.0f);
        FloatVector4  v_ne(xyScale, xyScale, z[2] - z[4], 0.0f);
        normal = calculateNormal(v_e, v_ne);
        normal += calculateNormal(v_ne, v_n);
        normal += calculateNormal(v_n, v_nw);
        normal += calculateNormal(v_nw, v_w);
        normal += calculateNormal(v_w, v_sw);
        normal += calculateNormal(v_sw, v_s);
        normal += calculateNormal(v_s, v_se);
        normal += calculateNormal(v_se, v_e);
      }
      else
      {
        normal = calculateNormal(v_e, v_n);
        normal += calculateNormal(v_n, v_w);
        normal += calculateNormal(v_w, v_s);
        normal += calculateNormal(v_s, v_e);
      }
      normal.normalize();
      FloatVector4  bitangent(normal[2], 0.0f, -(normal[0]), 0.0f);
      bitangent.normalize3Fast();
      FloatVector4  tangent(0.0f, -(normal[2]), normal[1], 0.0f);
      tangent.normalize3Fast();
      vertexPtr->bitangent = bitangent.convertToX10Y10Z10();
      vertexPtr->tangent = tangent.convertToX10Y10Z10();
      vertexPtr->normal = normal.convertToX10Y10Z10();
      vertexPtr->u = convertToFloat16(float(x - x0));
      vertexPtr->v = convertToFloat16(float(y - y0));
      if (x != x1 && y != y0)
      {
        int     v0 = int(vertexPtr - vertexDataBuf.data());     // SW
        int     v1 = v0 + 1;            // SE
        int     v2 = v0 + (w + 1);      // NE
        int     v3 = v0 + w;            // NW
        trianglePtr[0].v0 = std::uint16_t(v0);
        trianglePtr[0].v1 = std::uint16_t(v1);
        if ((x ^ y) & 1)                // split SW -> NE
        {                               // vertices must be in CCW order
          trianglePtr[0].v2 = std::uint16_t(v2);
          trianglePtr[1].v0 = std::uint16_t(v0);
        }
        else                            // split SE -> NW
        {
          trianglePtr[0].v2 = std::uint16_t(v3);
          trianglePtr[1].v0 = std::uint16_t(v1);
        }
        trianglePtr[1].v1 = std::uint16_t(v2);
        trianglePtr[1].v2 = std::uint16_t(v3);
        trianglePtr = trianglePtr + 2;
      }
    }
  }
  // create textures
  landTextureMask = 0U;
  for (size_t i = 0; i < (sizeof(landTexture) / sizeof(DDSTexture *)); i++)
  {
    if (landTexture[i])
    {
      delete landTexture[i];
      landTexture[i] = (DDSTexture *) 0;
    }
  }
  textureBuf.reserve(size_t(txtWP2) * size_t(txtHP2) * 3U + 128U);
  unsigned int  ddsHdrBuf[32];
  for (size_t i = 6; i < 32; i++)
    ddsHdrBuf[i] = 0U;
  ddsHdrBuf[0] = 0x20534444;    // "DDS "
  ddsHdrBuf[1] = 124;           // size of DDS_HEADER
  ddsHdrBuf[2] = 0x0000100F;    // caps, height, width, pitch, pixel format
  ddsHdrBuf[3] = (unsigned int) txtHP2;
  ddsHdrBuf[4] = (unsigned int) txtWP2;
  ddsHdrBuf[19] = 32;           // size of DDS_PIXELFORMAT
  ddsHdrBuf[20] = 0x00000040;   // DDPF_RGB
  ddsHdrBuf[24] = 0x0000FF00;   // green mask
  ddsHdrBuf[27] = 0x00001000;   // DDSCAPS_TEXTURE
  for (size_t k = 0; k < (sizeof(landTexture) / sizeof(DDSTexture *)); k++)
  {
    unsigned int  b = 1U << (unsigned char) k;
    if (!(ltexMask & b))
      continue;
    unsigned int  pixelBytes = (!(b & 0x0105U) ? 2U : 3U);
    ddsHdrBuf[5] = (unsigned int) txtWP2 * pixelBytes;  // pitch
    ddsHdrBuf[22] = pixelBytes << 3;                    // bits per pixel
    ddsHdrBuf[23] = 0xFFU << ((pixelBytes - 2U) << 4);  // red mask
    ddsHdrBuf[25] = 0xFFU & (2U - pixelBytes);          // blue mask
    textureBuf.resize(size_t(txtWP2) * size_t(txtHP2) * pixelBytes + 128U);
    unsigned char *dstPtr = textureBuf.data();
    for (size_t i = 0; i < 32; i++, dstPtr = dstPtr + 4)
    {
      unsigned int  n = ddsHdrBuf[i];
      dstPtr[0] = (unsigned char) (n & 0xFF);
      dstPtr[1] = (unsigned char) ((n >> 8) & 0xFF);
      dstPtr[2] = (unsigned char) ((n >> 16) & 0xFF);
      dstPtr[3] = (unsigned char) ((n >> 24) & 0xFF);
    }
    for (int y = 0; y < txtHP2; y++)
    {
      int     yc =
          std::min(std::max((y0 << textureScale) + y, 0), ltexHeight - 1);
      const unsigned char *srcPtr =
          ltexData[k] + (size_t(yc) * size_t(ltexWidth) * pixelBytes);
      if (pixelBytes == 2U)
      {
        for (int x = 0; x < txtWP2; x++, dstPtr = dstPtr + 2)
        {
          int     xc =
              std::min(std::max((x0 << textureScale) + x, 0), ltexWidth - 1);
          size_t  offs = size_t(xc) * 2U;
          dstPtr[0] = srcPtr[offs];
          dstPtr[1] = srcPtr[offs + 1];
        }
      }
      else
      {
        for (int x = 0; x < txtWP2; x++, dstPtr = dstPtr + 3)
        {
          int     xc =
              std::min(std::max((x0 << textureScale) + x, 0), ltexWidth - 1);
          size_t  offs = size_t(xc) * 3U;
          dstPtr[0] = srcPtr[offs];
          dstPtr[1] = srcPtr[offs + 1];
          dstPtr[2] = srcPtr[offs + 2];
        }
      }
    }
    landTexture[k] = new DDSTexture(textureBuf.data(), textureBuf.size());
    landTextureMask = landTextureMask | b;
  }
}

void TerrainMesh::createMesh(
    const LandscapeData& landData,
    int textureScale, int x0, int y0, int x1, int y1,
    const LandscapeTextureSet *landTextures, size_t landTextureCnt,
    unsigned int ltexMask, float textureMip, float textureRGBScale,
    std::uint32_t textureDefaultColor)
{
  int     w = std::abs(x1 - x0) + 5;
  int     h = std::abs(y1 - y0) + 5;
  int     txtW = w << textureScale;
  int     txtH = h << textureScale;
  x0 = (x0 < x1 ? x0 : x1) - 1;
  y0 = (y0 < y1 ? y0 : y1) - 1;
  x1 = x0 + w - 1;
  y1 = y0 + h - 1;
  size_t  totalDataSize = 0;
  for (size_t i = 0; i < 10; i++)
  {
    unsigned int  b = 1U << (unsigned char) i;
    if (ltexMask & b)
      totalDataSize = totalDataSize + (!(b & 0x0105U) ? 2 : 3);
  }
  totalDataSize = totalDataSize * (size_t(txtW) * size_t(txtH));
  textureBuf2.resize(totalDataSize);
  unsigned char *ltexData[10];
  unsigned char *p = textureBuf2.data();
  for (size_t i = 0; i < 10; i++)
  {
    unsigned int  b = 1U << (unsigned char) i;
    if (!(ltexMask & b))
    {
      ltexData[i] = (unsigned char *) 0;
      continue;
    }
    ltexData[i] = p;
    p = p + (size_t(txtW) * size_t(txtH) * (!(b & 0x0105U) ? 2U : 3U));
  }
  if (totalDataSize > 0)
  {
    LandscapeTexture  landTxt(landData, landTextures, landTextureCnt);
    landTxt.setMipLevel(textureMip);
    landTxt.setRGBScale(textureRGBScale);
    landTxt.setDefaultColor(textureDefaultColor);
    landTxt.renderTexture(
        ltexData[0], textureScale, x0, y0, x1, y1, ltexData[1],
        (ltexData[9] ? ltexData[9] : ltexData[6]), ltexData[8]);
  }
  hmapBuf.resize(size_t(w) * size_t(h));
  std::uint16_t *dstPtr = hmapBuf.data();
  int     landWidth = landData.getImageWidth();
  int     landHeight = landData.getImageHeight();
  for (int y = y0; y <= y1; y++)
  {
    int     yc = (y > 0 ? (y < (landHeight - 1) ? y : (landHeight - 1)) : 0);
    const std::uint16_t *srcPtr = landData.getHeightMap();
    srcPtr = srcPtr + (size_t(yc) * size_t(landWidth));
    for (int x = x0; x <= x1; x++, dstPtr++)
    {
      int     xc = (x > 0 ? (x < (landWidth - 1) ? x : (landWidth - 1)) : 0);
      *dstPtr = srcPtr[xc];
    }
  }
  int     cellResolution = landData.getCellResolution();
  float   xOffset = float(x0 - landData.getOriginX());
  float   yOffset = float(landData.getOriginY() - y0);
  xOffset = xOffset * (4096.0f / float(cellResolution));
  yOffset = yOffset * (4096.0f / float(cellResolution));
  createMesh(hmapBuf.data(), w, h, ltexData, ltexMask, txtW, txtH,
             textureScale, 1, 1, w - 4, h - 4, cellResolution,
             xOffset, yOffset, landData.getZMin(), landData.getZMax());
}

