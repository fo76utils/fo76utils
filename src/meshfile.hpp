
#ifndef MESHFILE_HPP_INCLUDED
#define MESHFILE_HPP_INCLUDED

#include "common.hpp"
#include "filebuf.hpp"
#include "fp32vec4.hpp"
#include "nif_file.hpp"

void readStarfieldMeshFile(std::vector< NIFFile::NIFVertex >& vertexData,
                           std::vector< NIFFile::NIFTriangle >& triangleData,
                           FileBuffer& buf);

#endif

