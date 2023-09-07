
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
// uint32               Number of vertex normals (Z axis of normal map) = vCnt.
// uint32 * vCnt        Vertex normals in X10Y10Z10 format:
//                          b0  to b9  = (X + 1.0) * 511.5
//                          b10 to b19 = (Y + 1.0) * 511.5
//                          b20 to b29 = (Z + 1.0) * 511.5
//                          b30 to b31 = unknown, either 00 or 01
// uint32               Number of vertex tangents (X axis of normal map) = vCnt.
// uint32 * vCnt        Vertex tangents in X10Y10Z10 format:
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
  // TODO: implement function
  (void) buf;
  vertexData.clear();
  triangleData.clear();
}

