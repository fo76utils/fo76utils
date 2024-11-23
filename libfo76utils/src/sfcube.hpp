
#ifndef SFCUBE_HPP_INCLUDED
#define SFCUBE_HPP_INCLUDED

#include "common.hpp"
#include "fp32vec4.hpp"

class SFCubeMapFilter
{
 protected:
  enum
  {
    minWidth = 16,
    filterMinWidth = 16
  };
  // roughness = (5.0 - sqrt(25.0 - mipLevel * 4.0)) / 4.0
  static const float  defaultRoughnessTable[7];
  FloatVector4  *imageBuf;
  std::uint32_t faceDataSize;
  std::uint32_t width;
  // width * width * 3 * 10 floats (X, Y, Z, W only for the +X, +Y, +Z faces):
  //   vec8 X, vec8 Y, vec8 Z, vec8 W, vec8 R0, vec8 G0, vec8 B0,
  //   vec8 R1, vec8 G1, vec8 B1
  float   *cubeFilterTable;
  const float   *roughnessTable;
  int     roughnessTableSize;
  float   normalizeLevel;
  void (*pixelStoreFunction)(unsigned char *p, FloatVector4 c);
 public:
  static inline FloatVector4 convertCoord(int x, int y, int w, int n);
 protected:
  void processImage_Specular(unsigned char *outBufP, int w,
                             size_t startPos, size_t endPos, float roughness);
  void processImage_Copy(unsigned char *outBufP, int w, int w2,
                         size_t startPos, size_t endPos,
                         const FloatVector4 *inBufP);
  static void pixelStore_R8G8B8A8(unsigned char *p, FloatVector4 c);
  static void pixelStore_R9G9B9E5(unsigned char *p, FloatVector4 c);
  // endPos - startPos must be even if the filter is enabled
  // roughness = 1.0 simulates diffuse filter
  static void threadFunction(SFCubeMapFilter *p, unsigned char *outBufP,
                             int w, size_t startPos, size_t endPos,
                             float roughness, bool enableFilter);
  void createFilterTable(int w);
  // returns the number of mip levels, or 0 on error
  int readImageData(std::vector< FloatVector4 >& imageData,
                    const unsigned char *buf, size_t bufSize);
  static void upsampleImage(
      std::vector< FloatVector4 >& outBuf, int outWidth,
      const std::vector< FloatVector4 >& inBuf, int inWidth);
 public:
  SFCubeMapFilter(size_t outputWidth = 256);
  ~SFCubeMapFilter();
  // Returns the new buffer size. If outFmtFloat is true, the output format is
  // DXGI_FORMAT_R9G9B9E5_SHAREDEXP instead of DXGI_FORMAT_R8G8B8A8_UNORM_SRGB.
  // The buffer must have sufficient capacity for width * width * 8 * 4 + 148
  // bytes.
  // On error, 0 is returned and no changes are made to the contents of buf.
  size_t convertImage(unsigned char *buf, size_t bufSize,
                      bool outFmtFloat = false, size_t bufCapacity = 0);
  void setRoughnessTable(const float *p, size_t n);
  // input data in FP16 formats is normalized to have an average level of
  // at most 'n' (default: 1.0 / 12.5)
  inline void setNormalizeLevel(float n)
  {
    normalizeLevel = 1.0f / ((n > 0.0f ? n : 65536.0f) * 3.0f);
  }
  void setOutputWidth(size_t w)
  {
    if (w < minWidth || w > 2048 || (w & (w - 1)))
      errorMessage("SFCubeMapFilter: invalid output dimensions");
    width = std::uint32_t(w);
  }
};

// face 0: E,      -X = up,   +X = down, -Y = N,    +Y = S
// face 1: W,      -X = down, +X = up,   -Y = N,    +Y = S
// face 2: N,      -X = W,    +X = E,    -Y = down, +Y = up
// face 3: S,      -X = W,    +X = E,    -Y = up,   +Y = down
// face 4: top,    -X = W,    +X = E,    -Y = N,    +Y = S
// face 5: bottom, -X = E,    +X = W,    -Y = N,    +Y = S

inline FloatVector4 SFCubeMapFilter::convertCoord(int x, int y, int w, int n)
{
  std::uint64_t tmp = std::uint64_t(std::uint16_t(x))
                      | (std::uint64_t(std::uint16_t(y)) << 16)
                      | (std::uint64_t(std::uint32_t(w)) << 32);
  FloatVector4  v(FloatVector4::convertInt16(tmp));
  v = FloatVector4(v[2]) - (v + v);
  // w - (x * 2 + 1), w - (y * 2 + 1), -w, w
  v += FloatVector4(-1.0f, -1.0f, 0.0f, 0.0f);
  switch (n)
  {
    case 0:
      v.shuffleValues(0xC7);    // w, -y, -x, w
      break;
    case 1:
      v.shuffleValues(0xC6);    // -w, -y, -x, w
      v *= FloatVector4(1.0f, 1.0f, -1.0f, 1.0f);
      break;
    case 2:
      v.shuffleValues(0xDC);    // -x, w, -y, w
      v *= FloatVector4(-1.0f, 1.0f, -1.0f, 1.0f);
      break;
    case 3:
      v.shuffleValues(0xD8);    // -x, -w, -y, w
      v *= FloatVector4(-1.0f, 1.0f, 1.0f, 1.0f);
      break;
    case 4:                     // -x, -y, -w, w
      v *= FloatVector4(-1.0f, 1.0f, -1.0f, 1.0f);
      break;
    default:
      break;
  }
  // normalize vector
  v *= (1.0f / float(std::sqrt(v.dotProduct3(v))));
  // v[3] = scale factor to account for variable angular resolution
  v[3] = v[3] * v[3] * v[3];
  return v;
}

class SFCubeMapCache : public SFCubeMapFilter
{
 protected:
  std::map< std::uint64_t, std::vector< unsigned char > > cachedTextures;
  static void convertHDRToDDSThread(
      unsigned char *outBuf, size_t outPixelSize, int cubeWidth,
      int yStart, int yEnd,
      const FloatVector4 *hdrTexture, int w, int h, float maxLevel);
 public:
  SFCubeMapCache();
  ~SFCubeMapCache();
  size_t convertImage(unsigned char *buf, size_t bufSize,
                      bool outFmtFloat = false, size_t bufCapacity = 0,
                      size_t outputWidth = 0);
  // Convert Radiance HDR format image to DDS cube map.
  //   cubeWidth:       output resolution per face
  //   invertCoord:     invert Z axis if true
  //   maxLevel > 0.0:  clamp output at maxLevel
  //   maxLevel < 0.0:  limit output to < -maxLevel with Reinhard tone mapping
  //   outFmt:          output format DXGI code (must be 0x0A or 0x43)
  // Returns true if the conversion was successful.
  static bool convertHDRToDDS(std::vector< unsigned char >& outBuf,
                              const unsigned char *inBufData, size_t inBufSize,
                              int cubeWidth, bool invertCoord, float maxLevel,
                              unsigned char outFmt);
};

#endif

