
#ifndef DOWNSAMP_HPP_INCLUDED
#define DOWNSAMP_HPP_INCLUDED

#include "common.hpp"
#include "fp32vec4.hpp"

// linePtr: line pointer in output buffer
// imageWidth, imageHeight: dimensions of input image
// y: line number on input image
// fmtFlags & 1: input pixel format (0: R8G8B8A8, 1: A2R10G10B10)
// fmtFlags & 2: output pixel format
void downsample2xFilter_Line(
    std::uint32_t *linePtr, const std::uint32_t *inBuf,
    int imageWidth, int imageHeight, int y,
    unsigned char fmtFlags = (USE_PIXELFMT_RGB10A2 * 3));

// imageWidth, imageHeight: dimensions of input image
// pitch: number of elements per line in outBuf
void downsample2xFilter(
    std::uint32_t *outBuf, const std::uint32_t *inBuf,
    int imageWidth, int imageHeight, int pitch,
    unsigned char fmtFlags = (USE_PIXELFMT_RGB10A2 * 3));

void downsample4xFilter_Line(
    std::uint32_t *linePtr, const std::uint32_t *inBuf,
    int imageWidth, int imageHeight, int y,
    unsigned char fmtFlags = (USE_PIXELFMT_RGB10A2 * 3));

void downsample4xFilter(
    std::uint32_t *outBuf, const std::uint32_t *inBuf,
    int imageWidth, int imageHeight, int pitch,
    unsigned char fmtFlags = (USE_PIXELFMT_RGB10A2 * 3));

#endif

