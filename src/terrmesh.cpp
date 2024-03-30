
#include "common.hpp"
#include "fp32vec4.hpp"
#include "filebuf.hpp"
#include "terrmesh.hpp"
#include "landtxt.hpp"
#include "material.hpp"

static inline FloatVector4 calculateNormal(FloatVector4 v1, FloatVector4 v2)
{
  // cross product of v1 and v2
  FloatVector4  tmp((v1[1] * v2[2]) - (v1[2] * v2[1]),
                    (v1[2] * v2[0]) - (v1[0] * v2[2]),
                    (v1[0] * v2[1]) - (v1[1] * v2[0]), 0.0f);
  return tmp.normalize3Fast();
}

static inline unsigned char landTxtPixelFormat(size_t n)
{
  unsigned char b = (unsigned char) (n << 3);
  return (unsigned char) ((0x1D3D3D3D3D3D331DULL >> b) & 0xFFU);
}

TerrainMesh::TerrainMesh()
  : NIFFile::NIFTriShape()
{
  for (size_t i = 0; i < (sizeof(landTexture) / sizeof(DDSTexture16 *)); i++)
    landTexture[i] = nullptr;
}

TerrainMesh::~TerrainMesh()
{
  for (size_t i = 0; i < (sizeof(landTexture) / sizeof(DDSTexture16 *)); i++)
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
  int     txtW = w << textureScale;
  int     txtH = h << textureScale;
  x0 = (x0 < x1 ? x0 : x1);
  y0 = (y0 < y1 ? y0 : y1);
  x1 = x0 + w - 1;
  y1 = y0 + h - 1;
  int     txtWP2 = 1 << int(std::bit_width((unsigned int) txtW + (4U - 1U)));
  int     txtHP2 = 1 << int(std::bit_width((unsigned int) txtH + (4U - 1U)));
  // create vertex and triangle data
  vertexCnt = (unsigned int) w * (unsigned int) h;
  if (vertexCnt > 65536U)
    errorMessage("TerrainMesh: vertex count is out of range");
  triangleCnt = ((unsigned int) (w - 1) * (unsigned int) (h - 1)) << 1;
  vertexDataBuf.resize(vertexCnt);
  triangleDataBuf.resize(triangleCnt);
  vertexData = vertexDataBuf.data();
  triangleData = triangleDataBuf.data();
  flags = CE2Material::Flag_IsTerrain;
  NIFFile::NIFVertex    *vertexPtr = vertexDataBuf.data();
  NIFFile::NIFTriangle  *trianglePtr = triangleDataBuf.data();
  float   xyScale = 100.0f / float(cellResolution);
  float   zScale = (zMax - zMin) / 65535.0f;
  float   uvScale = 1.0f / float(txtWP2 >> textureScale);
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
      vertexPtr->xyz[0] = xOffset + (float(x) * xyScale);
      vertexPtr->xyz[1] = yOffset - (float(y) * xyScale);
      vertexPtr->xyz[2] = z[4];
      vertexPtr->xyz[3] = 1.0f;
      vertexPtr->texCoord =
          FloatVector4(float(x - x0), float(y - y0), 0.0f, 0.0f) * uvScale;
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
      FloatVector4  tangent(normal[2], 0.0f, -(normal[0]), 0.0f);
      tangent.normalize3Fast();
      FloatVector4  bitangent(0.0f, -(normal[2]), normal[1], 0.0f);
      bitangent.normalize3Fast();
      vertexPtr->normal = normal.convertToX10Y10Z10();
      vertexPtr->tangent = tangent.convertToX10Y10Z10();
      vertexPtr->bitangent = bitangent.convertToX10Y10Z10();
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
  for (size_t i = 0; i < (sizeof(landTexture) / sizeof(DDSTexture16 *)); i++)
  {
    if (landTexture[i])
    {
      delete landTexture[i];
      landTexture[i] = nullptr;
    }
  }
  textureBuf.reserve(size_t(txtWP2) * size_t(txtHP2) * 4U + 148U);
  int     txtOffsX = (txtWP2 - txtW) >> 1;
  int     txtOffsY = (txtHP2 - txtH) >> 1;
  for (size_t k = 0; k < (sizeof(landTexture) / sizeof(DDSTexture16 *)); k++)
  {
    unsigned int  b = 1U << (unsigned char) k;
    if (!(ltexMask & b))
      continue;
    unsigned char dxgiFormat = landTxtPixelFormat(k);
    unsigned int  pixelBytes =
        FileBuffer::dxgiFormatSizeTable[dxgiFormat] & 0x7FU;
    textureBuf.resize(size_t(txtWP2) * size_t(txtHP2) * pixelBytes + 148U);
    unsigned char *dstPtr = textureBuf.data();
    FileBuffer::writeDDSHeader(dstPtr, dxgiFormat, txtWP2, txtHP2);
    dstPtr = dstPtr + 148;
    for (int y = 0; y < txtHP2; y++)
    {
      int     yc = (y0 << textureScale) + ((y + txtOffsY) & (txtHP2 - 1));
      yc = std::min(std::max(yc - txtOffsY, 0), ltexHeight - 1);
      const unsigned char *srcPtr =
          ltexData[k] + (size_t(yc) * size_t(ltexWidth) * pixelBytes);
      if (pixelBytes == 1U)
      {
        for (int x = 0; x < txtWP2; x++, dstPtr++)
        {
          int     xc = (x0 << textureScale) + ((x + txtOffsX) & (txtWP2 - 1));
          xc = std::min(std::max(xc - txtOffsX, 0), ltexWidth - 1);
          *dstPtr = srcPtr[xc];
        }
      }
      else if (pixelBytes == 2U)
      {
        for (int x = 0; x < txtWP2; x++, dstPtr = dstPtr + 2)
        {
          int     xc = (x0 << textureScale) + ((x + txtOffsX) & (txtWP2 - 1));
          xc = std::min(std::max(xc - txtOffsX, 0), ltexWidth - 1);
          size_t  offs = size_t(xc) * 2U;
          FileBuffer::writeUInt16Fast(
              dstPtr, FileBuffer::readUInt16Fast(srcPtr + offs));
        }
      }
      else
      {
        for (int x = 0; x < txtWP2; x++, dstPtr = dstPtr + 4)
        {
          int     xc = (x0 << textureScale) + ((x + txtOffsX) & (txtWP2 - 1));
          xc = std::min(std::max(xc - txtOffsX, 0), ltexWidth - 1);
          size_t  offs = size_t(xc) * 4U;
          FileBuffer::writeUInt32Fast(
              dstPtr, FileBuffer::readUInt32Fast(srcPtr + offs));
        }
      }
    }
    landTexture[k] = new DDSTexture16(textureBuf.data(), textureBuf.size());
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
  x0 = (x0 < x1 ? x0 : x1) - 2;
  y0 = (y0 < y1 ? y0 : y1) - 2;
  x1 = x0 + w - 1;
  y1 = y0 + h - 1;
  size_t  totalDataSize = 0;
  for (size_t i = 0; i < 8; i++)
  {
    unsigned int  b = 1U << (unsigned char) i;
    if (ltexMask & b)
      totalDataSize = totalDataSize + ((!i || i == 7) ? 4 : (i == 1 ? 2 : 1));
  }
  totalDataSize = totalDataSize * (size_t(txtW) * size_t(txtH));
  textureBuf2.resize(totalDataSize);
  unsigned char *ltexData[8];
  unsigned char *p = textureBuf2.data();
  for (size_t i = 0; i < 8; i++)
  {
    unsigned int  b = 1U << (unsigned char) i;
    if (!(ltexMask & b))
    {
      ltexData[i] = nullptr;
      continue;
    }
    ltexData[i] = p;
    p = p + (size_t(txtW) * size_t(txtH)
             * ((!i || i == 7) ? 4U : (i == 1 ? 2U : 1U)));
  }
  if (totalDataSize > 0)
  {
    LandscapeTexture  landTxt(landData, landTextures, landTextureCnt);
    landTxt.setMipLevel(textureMip);
    landTxt.setRGBScale(textureRGBScale);
    landTxt.setDefaultColor(textureDefaultColor);
    landTxt.renderTexture(ltexData, textureScale, x0, y0, x1, y1);
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
  xOffset = xOffset * (100.0f / float(cellResolution));
  yOffset = yOffset * (100.0f / float(cellResolution));
  createMesh(hmapBuf.data(), w, h, ltexData, ltexMask, txtW, txtH,
             textureScale, 2, 2, w - 3, h - 3, cellResolution,
             xOffset, yOffset, landData.getZMin(), landData.getZMax());
}

