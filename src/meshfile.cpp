
// Starfield .mesh file format:
//
// uint32               Always 1, possibly format version.
// uint32               Number of triangles (triCnt) * 3.
// uint16 * triCnt * 3  Vertex list for each triangle. Vertices are
//                      in CCW order on the front face of the triangle,
//                      vertex numbers are in the range 0 to vCnt - 1.
// float32              Vertex coordinate scale. All vertex coordinates
//                      are multiplied by this number / 32767.0.
// uint32               Unknown, usually 0, but can be in the range 0 to 8.
// uint32               Number of vertices (vCnt).
// int16 * vCnt * 3     Vertex coordinates as signed 16-bit integers.
// uint32               Number of vertex texture UVs, should be equal to vCnt.
// float16 * vCnt * 2   Vertex texture U, V coordinates as 16-bit floats.
// uint32               Size of next chunk (n1), either 0 or vCnt.
// n1 * 4 bytes         Unknown optional vertex attribute.
// uint32               Size of vertex color data (vclrCnt), either 0 or vCnt.
// uint32 * vclrCnt     Vertex colors (optional) in R8G8B8A8 format.
// uint32               Number of vertex normals, should be equal to vCnt.
// uint32 * vCnt        Normals (Z axis of tangent space) in X10Y10Z10 format:
//                          b0  to b9  = (X + 1.0) * 511.5
//                          b10 to b19 = (Y + 1.0) * 511.5
//                          b20 to b29 = (Z + 1.0) * 511.5
//                          b30 to b31 = unknown, either 00 or 01
// uint32               Number of vertex tangents, should be equal to vCnt.
// uint32 * vCnt        Tangents (X axis of tangent space) in X10Y10Z10 format:
//                          b0  to b9  = (X + 1.0) * 511.5
//                          b10 to b19 = (Y + 1.0) * 511.5
//                          b20 to b29 = (Z + 1.0) * 511.5
//                          b30 to b31 = unknown, either 00 or 11
// uint32               Size of next chunk (n2).
// n2 * 4 bytes         Unknown optional data.
// uint32               Number of following chunks (n3).
// {
//     uint32           Size of next chunk (n4).
//     uint16 * n4      Unknown optional data.
// } * n3
// -------------------  The remaining data is optional, the file may end here.
// uint32               Size of next chunk (n5).
// n5 * 16 bytes        Unknown optional data.
// uint32               Number of bounding spheres (bndCnt).
// {
//     float32          Bounding sphere center X.
//     float32          Bounding sphere center Y.
//     float32          Bounding sphere center Z.
//     float32          Bounding sphere radius.
//     8 bytes          Unknown data.
// } * bndCnt

#include "common.hpp"
#include "meshfile.hpp"

void readStarfieldMeshFile(std::vector< NIFFile::NIFVertex >& vertexData,
                           std::vector< NIFFile::NIFTriangle >& triangleData,
                           FileBuffer& buf)
{
  vertexData.clear();
  triangleData.clear();
  if (buf.readUInt32() != 1U)
    errorMessage("unsupported mesh file version");
  unsigned int  triangleCnt = buf.readUInt32();
  if ((triangleCnt % 3U) != 0U)
    errorMessage("invalid triangle data size in mesh file");
  triangleCnt = triangleCnt / 3U;
  if ((buf.getPosition() + (size_t(triangleCnt) * 6)) > buf.size())
    errorMessage("unexpected end of mesh file");
  triangleData.resize(triangleCnt);
  int     maxVertexNum = -1;
  for (size_t i = 0; i < triangleCnt; i++)
  {
    triangleData[i].v0 = buf.readUInt16Fast();
    triangleData[i].v1 = buf.readUInt16Fast();
    triangleData[i].v2 = buf.readUInt16Fast();
    maxVertexNum = std::max(maxVertexNum, int(triangleData[i].v0));
    maxVertexNum = std::max(maxVertexNum, int(triangleData[i].v1));
    maxVertexNum = std::max(maxVertexNum, int(triangleData[i].v2));
  }

  float   xyzScale = buf.readFloat() / 32767.0f;
  (void) buf.readUInt32();
  unsigned int  vertexCnt = buf.readUInt32();
  if (vertexCnt > 65536U)
    errorMessage("invalid vertex count in mesh file");
  if ((unsigned int) (maxVertexNum + 1) > vertexCnt)
    errorMessage("vertex number out of range in mesh file");
  if ((buf.getPosition() + (size_t(vertexCnt) * 6)) > buf.size())
    errorMessage("unexpected end of mesh file");
  vertexData.resize(vertexCnt);
  for (size_t i = 0; i < vertexCnt; i++)
  {
    vertexData[i].x = float(int(std::int16_t(buf.readUInt16Fast()))) * xyzScale;
    vertexData[i].y = float(int(std::int16_t(buf.readUInt16Fast()))) * xyzScale;
    vertexData[i].z = float(int(std::int16_t(buf.readUInt16Fast()))) * xyzScale;
  }

  if (buf.readUInt32() != vertexCnt)
    errorMessage("invalid vertex texture coordinate data size in mesh file");
  if ((buf.getPosition() + (size_t(vertexCnt) * 4)) > buf.size())
    errorMessage("unexpected end of mesh file");
  for (size_t i = 0; i < vertexCnt; i++)
  {
    vertexData[i].u = buf.readUInt16Fast();
    vertexData[i].v = buf.readUInt16Fast();
    if (BRANCH_UNLIKELY((vertexData[i].u & 0x7C00) == 0x7C00))  // Inf or NaN
      vertexData[i].u = 0;
    if (BRANCH_UNLIKELY((vertexData[i].v & 0x7C00) == 0x7C00))
      vertexData[i].v = 0;
  }

  unsigned int  n = buf.readUInt32();
  if (n != 0U && n != vertexCnt)
    errorMessage("invalid chunk size in mesh file");
  if ((buf.getPosition() + (size_t(n) * 4)) > buf.size())
    errorMessage("unexpected end of mesh file");
  buf.setPosition(buf.getPosition() + (size_t(n) * 4));

  n = buf.readUInt32();
  if (n != 0U && n != vertexCnt)
    errorMessage("invalid vertex color data size in mesh file");
  if ((buf.getPosition() + (size_t(n) * 4)) > buf.size())
    errorMessage("unexpected end of mesh file");
  for (size_t i = 0; i < n; i++)
    vertexData[i].vertexColor = buf.readUInt32Fast();

  if (buf.readUInt32() != vertexCnt)
    errorMessage("invalid vertex normal data size in mesh file");
  if ((buf.getPosition() + (size_t(vertexCnt) * 4)) > buf.size())
    errorMessage("unexpected end of mesh file");
  for (size_t i = 0; i < vertexCnt; i++)
    vertexData[i].normal = buf.readUInt32Fast() & 0x3FFFFFFFU;

  if (buf.readUInt32() != vertexCnt)
    errorMessage("invalid vertex tangent data size in mesh file");
  if ((buf.getPosition() + (size_t(vertexCnt) * 4)) > buf.size())
    errorMessage("unexpected end of mesh file");
  for (size_t i = 0; i < vertexCnt; i++)
  {
    std::uint32_t tmp = buf.readUInt32Fast() & 0x3FFFFFFFU;
    vertexData[i].bitangent = tmp;
    FloatVector4  normal(FloatVector4::convertX10Y10Z10(vertexData[i].normal));
    FloatVector4  bitangent(FloatVector4::convertX10Y10Z10(tmp));
    // calculate cross product
    FloatVector4  tangent(
                      (bitangent[1] * normal[2]) - (bitangent[2] * normal[1]),
                      (bitangent[2] * normal[0]) - (bitangent[0] * normal[2]),
                      (bitangent[0] * normal[1]) - (bitangent[1] * normal[0]),
                      0.0f);
    vertexData[i].tangent = tangent.convertToX10Y10Z10();
  }
}

