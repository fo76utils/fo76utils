
#ifndef RNDRBASE_HPP_INCLUDED
#define RNDRBASE_HPP_INCLUDED

#include "common.hpp"
#include "filebuf.hpp"
#include "ba2file.hpp"
#include "esmfile.hpp"
#include "ddstxt.hpp"
#include "material.hpp"
#include "nif_file.hpp"
#include "plot3d.hpp"

#include <bit>
#include <thread>
#include <mutex>

struct Renderer_Base
{
  struct TextureCache
  {
    struct CachedTextureKey
    {
      const BA2File::FileDeclaration  *fd;      // pointer from BA2File
      int     mipLevel;
      inline bool operator<(const CachedTextureKey& r) const
      {
        return (fd < r.fd || (fd == r.fd && mipLevel < r.mipLevel));
      }
      inline bool operator==(const CachedTextureKey& r) const
      {
        return (fd == r.fd && mipLevel == r.mipLevel);
      }
    };
    struct CachedTexture
    {
      DDSTexture    *texture;
      std::map< CachedTextureKey, CachedTexture >::iterator i;
      CachedTexture *prv;
      CachedTexture *nxt;
      std::mutex    *textureLoadMutex;
    };
    size_t  textureDataSize;
    size_t  textureCacheSize;
    CachedTexture *firstTexture;
    CachedTexture *lastTexture;
    std::mutex  textureCacheMutex;
    std::map< CachedTextureKey, CachedTexture > textureCache;
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
  struct TriShapeSortObject
  {
    std::uint64_t n;
    inline TriShapeSortObject(size_t i, float zMin, bool isAlphaBlending)
    {
      std::uint32_t zTmp;
#if defined(__i386__) || defined(__x86_64__) || defined(__x86_64)
      union
      {
        float   z_f;
        std::uint32_t z_i;
      }
      tmp;
      tmp.z_f = zMin * (1.0f / float(0x40000000));
      zTmp = tmp.z_i;
      zTmp = (std::int32_t(zTmp) >= 0 ?
              (0x40000000U + zTmp) : (0xC0000000U - zTmp));
#else
      zTmp = std::uint32_t(roundFloat(zMin * 64.0f) + 0x40000000);
#endif
      // sort transparent shapes back to front, and after all other shapes
      if (BRANCH_UNLIKELY(isAlphaBlending))
        zTmp = ~zTmp;
      n = std::uint64_t(i) | (std::uint64_t(zTmp) << 32);
    }
    inline bool operator<(const TriShapeSortObject& r) const
    {
      return (n < r.n);
    }
    inline operator size_t() const
    {
      return size_t(n & 0xFFFFFFFFU);
    }
    static void orderedNodeFix(
        std::vector< TriShapeSortObject >& sortBuf,
        const std::vector< NIFFile::NIFTriShape >& meshData);
  };
  struct WaterProperties
  {
    std::uint32_t deepColor;
    float   normalScale;
    float   reflectivity;
    float   specularScale;
    FloatVector4  alphaDepth0;
    FloatVector4  depthMult;
    std::string texturePath;
  };
  // returns key to 'm' where the material properties have been stored
  static unsigned int getWaterMaterial(
      std::map< unsigned int, WaterProperties >& m,
      ESMFile& esmFile, const ESMFile::ESMRecord *r,
      unsigned int defaultColor, bool storeAsDefault = false);
  static inline const CE2Material::TextureSet *
      findTextureSet(const CE2Material *m)
  {
    if (BRANCH_LIKELY(m))
    {
      unsigned int  i = (unsigned int) std::countr_zero(m->layerMask);
      i = std::min(i, (unsigned int) (CE2Material::maxLayers - 1));
      const CE2Material::Layer  *layer = m->layers[i];
      if (BRANCH_LIKELY(layer && layer->material))
        return layer->material->textureSet;
    }
    return (CE2Material::TextureSet *) 0;
  }
  static inline std::uint32_t bgraToRGBA(std::uint32_t c)
  {
    return ((c & 0xFF00FF00U) | ((c & 0xFFU) << 16) | ((c >> 16) & 0xFFU));
  }
};

#endif

