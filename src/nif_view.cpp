
#include "common.hpp"
#include "nif_view.hpp"

#include <algorithm>

Renderer::Renderer(std::uint32_t *outBufRGBA, float *outBufZ,
                   int imageWidth, int imageHeight, unsigned int nifVersion)
  : lightX(0.0f),
    lightY(0.0f),
    lightZ(1.0f),
    waterEnvMapLevel(1.0f),
    waterColor(0xC0804000U),
    whiteTexture(0xFFFFFFFFU)
{
  threadCnt = int(std::thread::hardware_concurrency());
  if (threadCnt < 1 || imageHeight < 64)
    threadCnt = 1;
  else if (threadCnt > 8)
    threadCnt = 8;
  ba2File = (BA2File *) 0;
  renderers.resize(size_t(threadCnt), (Plot3D_TriShape *) 0);
  threadErrMsg.resize(size_t(threadCnt));
  viewOffsetY.resize(size_t(threadCnt + 1), 0);
  unsigned int  renderMode =
      (nifVersion < 0x80U ? 7U : (nifVersion < 0x90U ? 11U : 15U));
  try
  {
    for (size_t i = 0; i < renderers.size(); i++)
    {
      renderers[i] = new Plot3D_TriShape(outBufRGBA, outBufZ,
                                         imageWidth, imageHeight, renderMode);
    }
  }
  catch (...)
  {
    for (size_t i = 0; i < renderers.size(); i++)
    {
      if (renderers[i])
      {
        delete renderers[i];
        renderers[i] = (Plot3D_TriShape *) 0;
      }
    }
    throw;
  }
  if (outBufRGBA && outBufZ)
    setBuffers(outBufRGBA, outBufZ, imageWidth, imageHeight, 1.0f);
}

Renderer::~Renderer()
{
  for (size_t i = 0; i < renderers.size(); i++)
  {
    if (renderers[i])
      delete renderers[i];
  }
  for (std::map< std::string, DDSTexture * >::iterator
           i = textureSet.begin(); i != textureSet.end(); i++)
  {
    if (i->second)
      delete i->second;
  }
}

void Renderer::setBuffers(std::uint32_t *outBufRGBA, float *outBufZ,
                          int imageWidth, int imageHeight, float envMapScale)
{
  float   y0 = 0.0f;
  for (size_t i = 0; i < renderers.size(); i++)
  {
    int     y0i = roundFloat(y0);
    int     y1i = imageHeight;
    if ((i + 1) < renderers.size())
    {
      float   y1 = float(int(i + 1)) / float(int(renderers.size()));
      y1 = (y1 - 0.5f) * (y1 - 0.5f) * 2.0f;
      if (i < (renderers.size() >> 1))
        y1 = 0.5f - y1;
      else
        y1 = 0.5f + y1;
      y1 = y1 * float(imageHeight);
      y1i = roundFloat(y1);
      y0 = y1;
    }
    viewOffsetY[i] = y0i;
    size_t  offs = size_t(y0i) * size_t(imageWidth);
    renderers[i]->setBuffers(outBufRGBA + offs, outBufZ + offs,
                             imageWidth, y1i - y0i);
    renderers[i]->setEnvMapOffset(float(imageWidth) * -0.5f,
                                  float(imageHeight) * -0.5f + float(y0i),
                                  float(imageHeight) * envMapScale);
  }
  viewOffsetY[renderers.size()] = imageHeight;
}

const DDSTexture * Renderer::loadTexture(const std::string *texturePath)
{
  if (!texturePath || texturePath->empty() || !ba2File)
    return (DDSTexture *) 0;
  DDSTexture  *t = (DDSTexture *) 0;
  textureSetMutex.lock();
  try
  {
    std::map< std::string, DDSTexture * >::iterator i =
        textureSet.find(*texturePath);
    if (i == textureSet.end())
    {
      try
      {
        ba2File->extractFile(fileBuf, *texturePath);
        t = new DDSTexture(&(fileBuf.front()), fileBuf.size());
      }
      catch (std::runtime_error& e)
      {
        std::fprintf(stderr, "Warning: failed to load texture '%s': %s\n",
                     texturePath->c_str(), e.what());
        t = (DDSTexture *) 0;
      }
      i = textureSet.insert(std::pair< std::string, DDSTexture * >(
                                *texturePath, t)).first;
    }
    t = i->second;
  }
  catch (...)
  {
    textureSetMutex.unlock();
    if (t)
      delete t;
    return (DDSTexture *) 0;
  }
  textureSetMutex.unlock();
  return t;
}

void Renderer::threadFunction(Renderer *p, size_t n)
{
  p->threadErrMsg[n].clear();
  try
  {
    std::vector< TriShapeSortObject > sortBuf;
    sortBuf.reserve(p->meshData.size());
    NIFFile::NIFVertexTransform vt(p->viewTransform);
    NIFFile::NIFVertexTransform mt(p->modelTransform);
    mt *= vt;
    vt.offsY = vt.offsY - float(p->viewOffsetY[n]);
    for (size_t i = 0; i < p->meshData.size(); i++)
    {
      if (p->meshData[i].flags & 0x01)          // ignore if hidden
        continue;
      NIFFile::NIFBounds  b;
      p->meshData[i].calculateBounds(b, &mt);
      if (roundFloat(b.xMax()) < 0 ||
          roundFloat(b.yMin()) > p->viewOffsetY[n + 1] ||
          roundFloat(b.yMax()) < p->viewOffsetY[n] ||
          b.zMax() < 0.0f)
      {
        continue;
      }
      TriShapeSortObject  tmp;
      tmp.ts = &(p->meshData.front()) + i;
      tmp.z = double(b.zMin());
      if (tmp.ts->alphaBlendScale)
        tmp.z = double(0x02000000) - tmp.z;
      sortBuf.push_back(tmp);
    }
    if (sortBuf.size() < 1)
      return;
    std::stable_sort(sortBuf.begin(), sortBuf.end());
    for (size_t i = 0; i < sortBuf.size(); i++)
    {
      const NIFFile::NIFTriShape& ts = *(sortBuf[i].ts);
      *(p->renderers[n]) = ts;
      if (BRANCH_UNLIKELY(ts.flags & 0x02))
      {
        const DDSTexture  *textures[5];
        unsigned int  textureMask = 0U;
        if (bool(textures[1] = p->loadTexture(&(p->waterTexture))))
          textureMask |= 0x0002U;
        if (bool(textures[4] = p->loadTexture(&(p->defaultEnvMap))))
          textureMask |= 0x0010U;
        p->renderers[n]->drawWater(p->modelTransform, vt,
                                   p->lightX, p->lightY, p->lightZ,
                                   textures, textureMask,
                                   p->waterColor, p->waterEnvMapLevel);
      }
      else
      {
        const DDSTexture  *textures[10];
        unsigned int  texturePathMask = (!(ts.flags & 0x80) ? 0x037BU : 0x037FU)
                                        & (unsigned int) ts.texturePathMask;
        unsigned int  textureMask = 0U;
        for (size_t j = 0; j < 10; j++, texturePathMask >>= 1)
        {
          if (texturePathMask & 1)
          {
            if (bool(textures[j] = p->loadTexture(ts.texturePaths[j])))
              textureMask |= (1U << (unsigned char) j);
          }
        }
        if (!(textureMask & 0x0001U))
        {
          textures[0] = &(p->whiteTexture);
          textureMask |= 0x0001U;
        }
        if (!(textureMask & 0x0010U) && ts.envMapScale > 0)
        {
          if (bool(textures[4] = p->loadTexture(&(p->defaultEnvMap))))
            textureMask |= 0x0010U;
        }
        p->renderers[n]->drawTriShape(p->modelTransform, vt,
                                      p->lightX, p->lightY, p->lightZ,
                                      textures, textureMask);
      }
    }
  }
  catch (std::exception& e)
  {
    p->threadErrMsg[n] = e.what();
    if (p->threadErrMsg[n].empty())
      p->threadErrMsg[n] = "unknown error in render thread";
  }
}

void Renderer::renderModel()
{
  std::vector< std::thread * >  threads(renderers.size(), (std::thread *) 0);
  try
  {
    for (size_t i = 0; i < threads.size(); i++)
      threads[i] = new std::thread(threadFunction, this, i);
    for (size_t i = 0; i < threads.size(); i++)
    {
      if (threads[i])
      {
        threads[i]->join();
        delete threads[i];
        threads[i] = (std::thread *) 0;
      }
      if (!threadErrMsg[i].empty())
        throw errorMessage("%s", threadErrMsg[i].c_str());
    }
  }
  catch (...)
  {
    for (size_t i = 0; i < threads.size(); i++)
    {
      if (threads[i])
      {
        threads[i]->join();
        delete threads[i];
      }
    }
    throw;
  }
}

