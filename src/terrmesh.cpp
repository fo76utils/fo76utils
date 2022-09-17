
#include "common.hpp"
#include "terrmesh.hpp"
#include "landtxt.hpp"
#include "fp32vec4.hpp"

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
  landTexture[0] = (DDSTexture *) 0;
  landTexture[1] = (DDSTexture *) 0;
}

TerrainMesh::~TerrainMesh()
{
  if (landTexture[0])
    delete landTexture[0];
  if (landTexture[1])
    delete landTexture[1];
}

void TerrainMesh::createMesh(
    const std::uint16_t *hmapData, int hmapWidth, int hmapHeight,
    const unsigned char *ltexData, const unsigned char *ltexDataN,
    int ltexWidth, int ltexHeight, int textureScale,
    int x0, int y0, int x1, int y1, int cellResolution,
    float xOffset, float yOffset, float zMin, float zMax)
{
  int     w = std::abs(x1 - x0) + 1;
  int     h = std::abs(y1 - y0) + 1;
  int     txtW = (w + 7) << textureScale;
  int     txtH = (h + 7) << textureScale;
  x0 = (x0 < x1 ? x0 : x1);
  y0 = (y0 < y1 ? y0 : y1);
  x1 = x0 + w - 1;
  y1 = y0 + h - 1;
  int     txtWP2 = 1;
  int     txtHP2 = 1;
  while (txtWP2 < txtW)
    txtWP2 = txtWP2 << 1;
  while (txtHP2 < txtH)
    txtHP2 = txtHP2 << 1;
  // create vertex and triangle data
  vertexCnt = (unsigned int) w * (unsigned int) h;
  if (vertexCnt > 65536U)
    throw errorMessage("TerrainMesh: vertex count is out of range");
  triangleCnt = ((unsigned int) (w - 1) * (unsigned int) (h - 1)) << 1;
  vertexDataBuf.resize(vertexCnt);
  triangleDataBuf.resize(triangleCnt);
  vertexData = &(vertexDataBuf.front());
  triangleData = &(triangleDataBuf.front());
  specularColor = 0x80FFFFFFU;
  specularSmoothness = 0;
  NIFFile::NIFVertex    *vertexPtr = &(vertexDataBuf.front());
  NIFFile::NIFTriangle  *trianglePtr = &(triangleDataBuf.front());
  float   xyScale = 4096.0f / float(cellResolution);
  float   zScale = (zMax - zMin) / 65535.0f;
  textureScaleU = 1.0f / float(txtWP2 >> textureScale);
  textureScaleV = 1.0f / float(txtHP2 >> textureScale);
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
      FloatVector4  normal(0.0f);
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
      bitangent += 1.0f;
      bitangent *= 127.5f;
      tangent += 1.0f;
      tangent *= 127.5f;
      normal += 1.0f;
      normal *= 127.5f;
      vertexPtr->bitangent = std::uint32_t(bitangent) & 0x00FFFFFFU;
      vertexPtr->tangent = std::uint32_t(tangent) & 0x00FFFFFFU;
      vertexPtr->normal = std::uint32_t(normal) & 0x00FFFFFFU;
      vertexPtr->u = convertToFloat16(float(x - x0));
      vertexPtr->v = convertToFloat16(float(y - y0));
      if (x != x1 && y != y0)
      {
        int     v0 = int(vertexPtr - &(vertexDataBuf.front())); // SW
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
  // create texture
  textureBuf.resize(size_t(txtWP2) * size_t(txtHP2) * 3U + 128U);
  unsigned int  ddsHdrBuf[32];
  for (size_t i = 6; i < 32; i++)
    ddsHdrBuf[i] = 0U;
  ddsHdrBuf[0] = 0x20534444;    // "DDS "
  ddsHdrBuf[1] = 124;           // size of DDS_HEADER
  ddsHdrBuf[2] = 0x0000100F;    // caps, height, width, pitch, pixel format
  ddsHdrBuf[3] = (unsigned int) txtHP2;
  ddsHdrBuf[4] = (unsigned int) txtWP2;
  ddsHdrBuf[5] = (unsigned int) txtWP2 * 3U;    // pitch
  ddsHdrBuf[19] = 32;           // size of DDS_PIXELFORMAT
  ddsHdrBuf[20] = 0x00000040;   // DDPF_RGB
  ddsHdrBuf[22] = 24;           // bits per pixel
  ddsHdrBuf[23] = 0x00FF0000;   // red mask
  ddsHdrBuf[24] = 0x0000FF00;   // green mask
  ddsHdrBuf[25] = 0x000000FF;   // blue mask
  ddsHdrBuf[27] = 0x00001000;   // DDSCAPS_TEXTURE
  unsigned char *dstPtr = &(textureBuf.front());
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
    int     yc = (y0 << textureScale) + y;
    if (y >= ((txtH + txtHP2) >> 1))
      yc = yc - txtHP2;
    yc = (yc > 0 ? (yc < (ltexHeight - 1) ? yc : (ltexHeight - 1)) : 0);
    const unsigned char *srcPtr =
        ltexData + (size_t(yc) * size_t(ltexWidth) * 3U);
    for (int x = 0; x < txtWP2; x++, dstPtr = dstPtr + 3)
    {
      int     xc = (x0 << textureScale) + x;
      if (x >= ((txtW + txtWP2) >> 1))
        xc = xc - txtWP2;
      xc = (xc > 0 ? (xc < (ltexWidth - 1) ? xc : (ltexWidth - 1)) : 0);
      size_t  offs = size_t(xc) * 3U;
      dstPtr[0] = srcPtr[offs];
      dstPtr[1] = srcPtr[offs + 1];
      dstPtr[2] = srcPtr[offs + 2];
    }
  }
  for (int i = 0; i < 2; i++)
  {
    if (landTexture[i])
    {
      delete landTexture[i];
      landTexture[i] = (DDSTexture *) 0;
    }
  }
  landTexture[0] = new DDSTexture(&(textureBuf.front()), textureBuf.size());
  if (!ltexDataN)
    return;
  // create normal map texture
  textureBuf.resize(size_t(txtWP2) * size_t(txtHP2) * 2U + 128U);
  ddsHdrBuf[5] = (unsigned int) txtWP2 * 2U;    // pitch
  ddsHdrBuf[22] = 16;           // bits per pixel
  ddsHdrBuf[23] = 0x000000FF;   // red mask
  ddsHdrBuf[24] = 0x0000FF00;   // green mask
  ddsHdrBuf[25] = 0x00000000;   // blue mask
  dstPtr = &(textureBuf.front());
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
    int     yc = (y0 << textureScale) + y;
    if (y >= ((txtH + txtHP2) >> 1))
      yc = yc - txtHP2;
    yc = (yc > 0 ? (yc < (ltexHeight - 1) ? yc : (ltexHeight - 1)) : 0);
    const unsigned char *srcPtr =
        ltexDataN + (size_t(yc) * size_t(ltexWidth) * 2U);
    for (int x = 0; x < txtWP2; x++, dstPtr = dstPtr + 2)
    {
      int     xc = (x0 << textureScale) + x;
      if (x >= ((txtW + txtWP2) >> 1))
        xc = xc - txtWP2;
      xc = (xc > 0 ? (xc < (ltexWidth - 1) ? xc : (ltexWidth - 1)) : 0);
      size_t  offs = size_t(xc) * 2U;
      dstPtr[0] = srcPtr[offs];
      dstPtr[1] = srcPtr[offs + 1];
    }
  }
  landTexture[1] = new DDSTexture(&(textureBuf.front()), textureBuf.size());
}

void TerrainMesh::createMesh(
    const LandscapeData& landData,
    int textureScale, int x0, int y0, int x1, int y1,
    const DDSTexture * const *landTextures,
    const DDSTexture * const *landTexturesN, size_t landTextureCnt,
    float textureMip, float textureRGBScale, std::uint32_t textureDefaultColor)
{
  int     w = std::abs(x1 - x0) + 8;
  int     h = std::abs(y1 - y0) + 8;
  int     txtW = w << textureScale;
  int     txtH = h << textureScale;
  x0 = (x0 < x1 ? x0 : x1) - 4;
  y0 = (y0 < y1 ? y0 : y1) - 4;
  x1 = x0 + w - 1;
  y1 = y0 + h - 1;
  unsigned char *ltexData = (unsigned char *) 0;
  unsigned char *ltexDataN = (unsigned char *) 0;
  {
    if (landTexturesN && textureScale > 0)
    {
      textureBuf2.resize(size_t(txtW) * size_t(txtH) * 5U);
      ltexDataN = &(textureBuf2.front());
      ltexData = ltexDataN + (size_t(txtW) * size_t(txtH) * 2U);
    }
    else
    {
      textureBuf2.resize(size_t(txtW) * size_t(txtH) * 3U);
      ltexData = &(textureBuf2.front());
    }
    LandscapeTexture  landTxt(landData, landTextures, landTextureCnt,
                              landTexturesN);
    landTxt.setMipLevel(textureMip);
    landTxt.setRGBScale(textureRGBScale);
    landTxt.setDefaultColor(textureDefaultColor);
    landTxt.renderTexture(ltexData, textureScale, x0, y0, x1, y1, ltexDataN);
  }
  hmapBuf.resize(size_t(w) * size_t(h));
  std::uint16_t *dstPtr = &(hmapBuf.front());
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
  createMesh(&(hmapBuf.front()), w, h, ltexData, ltexDataN, txtW, txtH,
             textureScale, 4, 4, w - 4, h - 4, cellResolution,
             xOffset, yOffset, landData.getZMin(), landData.getZMax());
}

