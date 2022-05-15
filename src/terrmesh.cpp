
#include "common.hpp"
#include "terrmesh.hpp"
#include "landtxt.hpp"

void TerrainMesh::calculateNormal(
    float& normalX, float& normalY, float& normalZ,
    float dx, float dy, float dz1, float dz2)
{
  float   tmp1 = 1.0f + (dz1 * dz1);
  float   tmp2 = 1.0f + (dz2 * dz2);
  float   tmp3 = float(std::sqrt(tmp2 / tmp1));
  float   dz = (dz1 * tmp3 + dz2) / (tmp3 + 1.0f);
  normalZ += 0.5f;
  normalX -= (dz * dx);
  normalY -= (dz * dy);
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
    const unsigned short *hmapData, int hmapWidth, int hmapHeight,
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
  NIFFile::NIFVertex    *vertexPtr = &(vertexDataBuf.front());
  NIFFile::NIFTriangle  *trianglePtr = &(triangleDataBuf.front());
  float   xyScale = 4096.0f / float(cellResolution);
  float   zScale = (zMax - zMin) / 65535.0f;
  float   normalScale1 = float(cellResolution) * (1.0f / 4096.0f);
  float   normalScale2 = float(cellResolution) * (0.7071068f / 4096.0f);
  float   uScale = 1.0f / float(txtWP2 >> textureScale);
  float   vScale = 1.0f / float(txtHP2 >> textureScale);
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
      float   normalX = 0.0f;
      float   normalY = 0.0f;
      float   normalZ = 0.0f;
      calculateNormal(normalX, normalY, normalZ, 1.0f, 0.0f,
                      (z[4] - z[3]) * normalScale1,     // -W
                      (z[5] - z[4]) * normalScale1);    // +E
      calculateNormal(normalX, normalY, normalZ, 0.0f, 1.0f,
                      (z[4] - z[7]) * normalScale1,     // -S
                      (z[1] - z[4]) * normalScale1);    // +N
      if ((x ^ y) & 1)
      {
        //    0 1 2
        // -1 +-+-+
        //    |\|/|
        //  0 +-+-+
        //    |/|\|
        //  1 +-+-+
        calculateNormal(normalX, normalY, normalZ, -0.7071068f, 0.7071068f,
                        (z[4] - z[8]) * normalScale2,   // -SE
                        (z[0] - z[4]) * normalScale2);  // +NW
        calculateNormal(normalX, normalY, normalZ, 0.7071068f, 0.7071068f,
                        (z[4] - z[6]) * normalScale2,   // -SW
                        (z[2] - z[4]) * normalScale2);  // +NE
      }
      float   tmp =
          (normalX * normalX) + (normalY * normalY) + (normalZ * normalZ);
      tmp = 1.0f / float(std::sqrt(tmp));
      vertexPtr->normalX = normalX * tmp;
      vertexPtr->normalY = normalY * tmp;
      vertexPtr->normalZ = normalZ * tmp;
      vertexPtr->u = convertToFloat16(float(x - x0) * uScale);
      vertexPtr->v = convertToFloat16(float(y - y0) * vScale);
      if (x != x1 && y != y0)
      {
        int     v0 = int(vertexPtr - &(vertexDataBuf.front())); // SW
        int     v1 = v0 + 1;            // SE
        int     v2 = v0 + (w + 1);      // NE
        int     v3 = v0 + w;            // NW
        trianglePtr[0].v0 = (unsigned short) v0;
        trianglePtr[0].v1 = (unsigned short) v1;
        if ((x ^ y) & 1)                // split SW -> NE
        {                               // vertices must be in CCW order
          trianglePtr[0].v2 = (unsigned short) v2;
          trianglePtr[1].v0 = (unsigned short) v0;
        }
        else                            // split SE -> NW
        {
          trianglePtr[0].v2 = (unsigned short) v3;
          trianglePtr[1].v0 = (unsigned short) v1;
        }
        trianglePtr[1].v1 = (unsigned short) v2;
        trianglePtr[1].v2 = (unsigned short) v3;
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
    float textureMip, float textureRGBScale, unsigned int textureDefaultColor)
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
  unsigned short  *dstPtr = &(hmapBuf.front());
  int     landWidth = landData.getImageWidth();
  int     landHeight = landData.getImageHeight();
  for (int y = y0; y <= y1; y++)
  {
    int     yc = (y > 0 ? (y < (landHeight - 1) ? y : (landHeight - 1)) : 0);
    const unsigned short  *srcPtr = landData.getHeightMap();
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

