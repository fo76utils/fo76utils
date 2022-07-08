
#include "common.hpp"
#include "nif_file.hpp"
#include "ba2file.hpp"
#include "ddstxt.hpp"

#ifdef HAVE_SDL
#  include <SDL/SDL.h>

static void renderCubeMap(const BA2File& ba2File,
                          const std::string& texturePath,
                          int imageWidth, int imageHeight, int mipLevel)
{
  std::vector< unsigned char >  fileBuf;
  ba2File.extractFile(fileBuf, texturePath);
  DDSTexture  texture(&(fileBuf.front()), fileBuf.size());
  if (!texture.getIsCubeMap())
  {
    std::fprintf(stderr, "Warning: %s is not a cube map\n",
                 texturePath.c_str());
  }
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
    throw errorMessage("error initializing SDL");
  bool    enableFullScreen = false;
  const SDL_VideoInfo *v = SDL_GetVideoInfo();
  if (v)
  {
    enableFullScreen =
        (imageWidth == v->current_w && imageHeight == v->current_h);
  }
  try
  {
    SDL_Surface *sdlScreen =
        SDL_SetVideoMode(imageWidth, imageHeight, 24,
                         SDL_SWSURFACE
                         | (enableFullScreen ? SDL_FULLSCREEN : 0));
    if (!sdlScreen)
      throw errorMessage("error setting SDL video mode");
    size_t  imageDataSize = size_t(imageWidth) * size_t(imageHeight);
    std::vector< unsigned int > outBufRGBW(imageDataSize, 0U);
    NIFFile::NIFVertexTransform
        viewTransform(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    float   scale =
        1.0f / float(imageWidth > imageHeight ? imageWidth : imageHeight);
    while (true)
    {
      for (int yc = 0; yc < imageHeight; yc++)
      {
        for (int xc = 0; xc < imageWidth; xc++)
        {
          float   x = float(xc - (imageWidth >> 1)) * scale;
          float   y = 1.0f;
          float   z = float((imageHeight >> 1) - yc) * scale;
          viewTransform.rotateXYZ(x, y, z);
          unsigned int  c = texture.cubeMap(x, y, z, mipLevel);
          outBufRGBW[size_t(yc) * size_t(imageWidth) + size_t(xc)] = c;
        }
      }
      {
        SDL_LockSurface(sdlScreen);
        unsigned int  *srcPtr = &(outBufRGBW.front());
        unsigned char *dstPtr =
            reinterpret_cast< unsigned char * >(sdlScreen->pixels);
        for (int y = 0; y < imageHeight; y++)
        {
          for (int x = 0; x < imageWidth; x++, srcPtr++, dstPtr = dstPtr + 3)
          {
            unsigned int  c = *srcPtr;
#if defined(_WIN32) || defined(_WIN64)
            dstPtr[0] = (unsigned char) ((c >> 16) & 0xFF);     // B
            dstPtr[1] = (unsigned char) ((c >> 8) & 0xFF);      // G
            dstPtr[2] = (unsigned char) (c & 0xFF);             // R
#else
            // FIXME: should detect pixel format
            dstPtr[0] = (unsigned char) (c & 0xFF);             // R
            dstPtr[1] = (unsigned char) ((c >> 8) & 0xFF);      // G
            dstPtr[2] = (unsigned char) ((c >> 16) & 0xFF);     // B
#endif
            *srcPtr = 0U;
          }
          dstPtr = dstPtr + (int(sdlScreen->pitch) - (imageWidth * 3));
        }
        SDL_UnlockSurface(sdlScreen);
        SDL_UpdateRect(sdlScreen, 0, 0, imageWidth, imageHeight);
        bool    keyPressed = false;
        bool    quitFlag = false;
        while (!(keyPressed || quitFlag))
        {
          SDL_Delay(10);
          SDL_Event event;
          while (SDL_PollEvent(&event))
          {
            if (event.type != SDL_KEYDOWN)
            {
              if (event.type == SDL_QUIT)
                quitFlag = true;
              continue;
            }
            bool    rotateFlag = false;
            float   viewRotationX = 0.0f;
            float   viewRotationY = 0.0f;
            float   viewRotationZ = 0.0f;
            switch (event.key.keysym.sym)
            {
              case SDLK_0:
              case SDLK_1:
              case SDLK_2:
              case SDLK_3:
              case SDLK_4:
              case SDLK_5:
              case SDLK_6:
              case SDLK_7:
              case SDLK_8:
              case SDLK_9:
                mipLevel = int(event.key.keysym.sym - SDLK_0);
                break;
              case SDLK_a:
                viewRotationZ = -0.19634954f;           // 11.25 degrees
                rotateFlag = true;
                break;
              case SDLK_d:
                viewRotationZ = 0.19634954f;
                rotateFlag = true;
                break;
              case SDLK_s:
                viewRotationX = 0.19634954f;
                rotateFlag = true;
                break;
              case SDLK_w:
                viewRotationX = -0.19634954f;
                rotateFlag = true;
                break;
              case SDLK_q:
                viewRotationY = 0.19634954f;
                rotateFlag = true;
                break;
              case SDLK_e:
                viewRotationY = -0.19634954f;
                rotateFlag = true;
                break;
              case SDLK_ESCAPE:
                quitFlag = true;
                break;
              default:
                continue;
            }
            if (rotateFlag)
            {
              NIFFile::NIFVertexTransform
                  vt(1.0f, viewRotationX, viewRotationY, viewRotationZ,
                     0.0f, 0.0f, 0.0f);
              vt *= viewTransform;
              // normalize
              float   tmp = vt.rotateXX * vt.rotateXX;
              tmp += (vt.rotateXY * vt.rotateXY);
              tmp += (vt.rotateXZ * vt.rotateXZ);
              tmp += (vt.rotateYX * vt.rotateYX);
              tmp += (vt.rotateYY * vt.rotateYY);
              tmp += (vt.rotateYZ * vt.rotateYZ);
              tmp += (vt.rotateZX * vt.rotateZX);
              tmp += (vt.rotateZY * vt.rotateZY);
              tmp += (vt.rotateZZ * vt.rotateZZ);
              tmp = float(std::sqrt(3.0f / tmp));
              vt.rotateXX *= tmp;
              vt.rotateXY *= tmp;
              vt.rotateXZ *= tmp;
              vt.rotateYX *= tmp;
              vt.rotateYY *= tmp;
              vt.rotateYZ *= tmp;
              vt.rotateZX *= tmp;
              vt.rotateZY *= tmp;
              vt.rotateZZ *= tmp;
              viewTransform = vt;
            }
            keyPressed = true;
            break;
          }
        }
        if (!quitFlag)
          continue;
      }
      break;
    }
    SDL_Quit();
  }
  catch (...)
  {
    SDL_Quit();
    throw;
  }
}
#endif

int main(int argc, char **argv)
{
  try
  {
#ifndef HAVE_SDL
    throw errorMessage("SDL 1.2 is required");
#else
    if (argc != 5 && argc != 6)
    {
      std::fprintf(stderr,
                   "Usage: cubeview WIDTH HEIGHT ARCHIVEPATH TEXTURE.DDS "
                   "[MIPLEVEL]\n");
      return 1;
    }
    int     renderWidth =
        int(parseInteger(argv[1], 10, "invalid image width", 8, 16384));
    int     renderHeight =
        int(parseInteger(argv[2], 10, "invalid image height", 8, 16384));
    int     mipLevel = 0;
    if (argc > 5)
      mipLevel = int(parseInteger(argv[5], 10, "invalid mip level", 0, 15));
    BA2File ba2File(argv[3], ".dds");
    renderCubeMap(ba2File, std::string(argv[4]), renderWidth, renderHeight,
                  mipLevel);
#endif
  }
  catch (std::exception& e)
  {
    std::fprintf(stderr, "cubeview: %s\n", e.what());
    return 1;
  }
  return 0;
}

