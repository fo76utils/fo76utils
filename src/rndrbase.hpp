
#ifndef RNDRBASE_HPP_INCLUDED
#define RNDRBASE_HPP_INCLUDED

#include "common.hpp"
#include "filebuf.hpp"
#include "ba2file.hpp"
#include "esmfile.hpp"
#include "ddstxt.hpp"
#include "bgsmfile.hpp"
#include "nif_file.hpp"
#include "plot3d.hpp"

#include <thread>
#include <mutex>

struct Renderer_Base
{
  struct TextureCache
  {
    struct CachedTexture
    {
      DDSTexture    *texture;
      std::map< std::string, CachedTexture >::iterator  i;
      CachedTexture *prv;
      CachedTexture *nxt;
      std::mutex    *textureLoadMutex;
    };
    size_t  textureDataSize;
    size_t  textureCacheSize;
    CachedTexture *firstTexture;
    CachedTexture *lastTexture;
    std::mutex  textureCacheMutex;
    std::map< std::string, CachedTexture >  textureCache;
    static size_t getTextureDataSize(const DDSTexture *t);
    TextureCache(size_t n = 0x40000000)
      : textureDataSize(0),
        textureCacheSize(n),
        firstTexture((CachedTexture *) 0),
        lastTexture((CachedTexture *) 0)
    {
    }
    ~TextureCache();
    // returns NULL on failure
    // *waitFlag is set to true if the texture is locked by another thread
    const DDSTexture *loadTexture(const BA2File& ba2File,
                                  const std::string& fileName,
                                  std::vector< unsigned char >& fileBuf,
                                  int mipLevel, bool *waitFlag = (bool *) 0);
    void shrinkTextureCache();
    void clear();
  };
  struct MaterialSwaps
  {
    // materialSwaps[formID][materialPath] = replacement material
    std::map< unsigned int, std::map< std::string, BGSMFile > >
        materialSwaps;
    unsigned int loadMaterialSwap(const BA2File& ba2File,
                                  ESMFile& esmFile, unsigned int formID);
    void materialSwap(Plot3D_TriShape& t, unsigned int formID) const;
  };
  struct TriShapeSortObject
  {
    const NIFFile::NIFTriShape  *ts;
    double  z;
    inline TriShapeSortObject(const NIFFile::NIFTriShape& t, float zMin)
    {
      ts = &t;
      // sort transparent shapes back to front, and after all other shapes
      z = (!(t.m.flags & BGSMFile::Flag_TSAlphaBlending) ?
           double(zMin) : (double(0x02000000) - double(zMin)));
    }
    inline bool operator<(const TriShapeSortObject& r) const
    {
      return (z < r.z);
    }
    static void orderedNodeFix(
        std::vector< TriShapeSortObject >& sortBuf,
        const std::vector< NIFFile::NIFTriShape >& meshData);
  };
  static std::uint32_t getWaterColor(
      ESMFile& esmFile, const ESMFile::ESMRecord& r, unsigned int defaultColor);
  static inline std::uint32_t bgraToRGBA(std::uint32_t c)
  {
    return ((c & 0xFF00FF00U) | ((c & 0xFFU) << 16) | ((c >> 16) & 0xFFU));
  }
};

#endif

