
// Starfield .mesh file format:
//
// uint32               Always 1, possibly format version.
// uint32               Number of triangles (triCnt) * 3.
// uint16 * triCnt * 3  Vertex list for each triangle. Vertices are
//                      in CCW order on the front face of the triangle,
//                      vertex numbers are in the range 0 to vCnt - 1.
// float32              Vertex coordinate scale. All vertex coordinates
//                      are multiplied by this number / 32767.0.
// uint32               Number of weights per vertex, usually 0, can be up to 8.
// uint32               Number of vertices (vCnt).
// int16 * vCnt * 3     Vertex coordinates as signed 16-bit integers.
// uint32               Number of vertex texture UVs, should be equal to vCnt.
// float16 * vCnt * 2   Vertex texture U, V coordinates as 16-bit floats.
// uint32               Number of second texture coordinates (n1), 0 or vCnt.
// float16 * n1 * 2     Second set of vertex texture coordinates (optional).
// uint32               Size of vertex color data (vclrCnt), either 0 or vCnt.
// uint32 * vclrCnt     Vertex colors (optional) in B8G8R8A8 format.
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
//                          b30 to b31 = 00 if bitangent = tangent x normal
//                                       11 if bitangent = normal x tangent
// uint32               Number of vertex weights, n2 = vCnt * weightsPerVertex.
// uint32 * n2          Vertex weights * 2^32-1 as 32-bit integers (optional).
// uint32               Number of LODs (n3), usually 0.
// {
//     uint32           Number of LOD triangles * 3 (n4).
//     uint16 * n4      Vertex list for LOD triangles.
// } * n3
// -------------------  The remaining data is optional, the file may end here.
// uint32               Number of meshlets (n5).
// n5 * 16 bytes        Meshlet data.
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

  float   xyzScale = buf.readFloat();
  unsigned int  vertexWeightCnt = buf.readUInt32();
  unsigned int  vertexCnt = buf.readUInt32();
  if (vertexCnt > 65536U)
    errorMessage("invalid vertex count in mesh file");
  if ((unsigned int) (maxVertexNum + 1) > vertexCnt)
    errorMessage("vertex number out of range in mesh file");
  if ((buf.getPosition() + (size_t(vertexCnt) * 6 + 2)) > buf.size())
    errorMessage("unexpected end of mesh file");
  vertexData.resize(vertexCnt);
  for (size_t i = 0; i < vertexCnt; i++)
  {
    const unsigned char *p = buf.data() + buf.getPosition();
    buf.setPosition(buf.getPosition() + 6);
#if ENABLE_X86_64_AVX
    const std::uint64_t&  tmp = *(reinterpret_cast< const std::uint64_t * >(p));
#else
    std::uint64_t tmp = FileBuffer::readUInt64Fast(p);
#endif
    vertexData[i].xyz = FloatVector4::convertInt16(tmp) * xyzScale / 32767.0f;
    vertexData[i].xyz[3] = 1.0f;
  }

  if (buf.readUInt32() != vertexCnt)
    errorMessage("invalid vertex texture coordinate data size in mesh file");
  size_t  texCoordOffs = buf.getPosition();
  if ((texCoordOffs + (size_t(vertexCnt) * 4)) > buf.size())
    errorMessage("unexpected end of mesh file");
  buf.setPosition(texCoordOffs + (size_t(vertexCnt) * 4));

  unsigned int  n = buf.readUInt32();
  if (n != 0U && n != vertexCnt)
    errorMessage("invalid vertex texture coordinate data size in mesh file");
  if ((buf.getPosition() + (size_t(n) * 4)) > buf.size())
    errorMessage("unexpected end of mesh file");
  for (size_t i = 0; i < vertexCnt; i++)
  {
    std::uint64_t tmp =
        FileBuffer::readUInt32Fast(buf.data() + (texCoordOffs + (i << 2)));
    if (n)
      tmp = tmp | (std::uint64_t(buf.readUInt32Fast()) << 32);
    vertexData[i].texCoord = FloatVector4::convertFloat16(tmp, true);
  }

  n = buf.readUInt32();
  if (n != 0U && n != vertexCnt)
    errorMessage("invalid vertex color data size in mesh file");
  if ((buf.getPosition() + (size_t(n) * 4)) > buf.size())
    errorMessage("unexpected end of mesh file");
  for (size_t i = 0; i < n; i++)
  {
    std::uint32_t c = buf.readUInt32Fast();
    vertexData[i].vertexColor =
        (c & 0xFF00FF00U) | ((c & 0xFFU) << 16) | ((c >> 16) & 0xFFU);
  }

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
    std::uint32_t tmp = buf.readUInt32Fast();
    vertexData[i].tangent = tmp & 0x3FFFFFFFU;
    tmp = (!(tmp & 0x80000000U) ? tmp : ~tmp);
    FloatVector4  normal(FloatVector4::convertX10Y10Z10(vertexData[i].normal));
    FloatVector4  tangent(FloatVector4::convertX10Y10Z10(tmp));
    // calculate cross product
    FloatVector4  bitangent(tangent.crossProduct3(normal));
    vertexData[i].bitangent = bitangent.convertToX10Y10Z10();
  }

  n = buf.readUInt32();
  if (n)
  {
    if (n != (std::uint64_t(vertexWeightCnt) * vertexCnt))
      errorMessage("invalid vertex weight data size in mesh file");
    if ((buf.getPosition() + (size_t(n) * 4)) > buf.size())
      errorMessage("unexpected end of mesh file");
    for (size_t i = 0; i < vertexCnt; i++)
    {
      std::int32_t  tmp = std::int32_t(buf.readUInt32Fast() >> 1);
      vertexData[i].xyz[3] = float(tmp) * (1.0f / 2147483648.0f);
      buf.setPosition(buf.getPosition() + ((vertexWeightCnt - 1U) << 2));
    }
  }
}

