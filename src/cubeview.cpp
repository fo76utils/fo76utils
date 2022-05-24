
#include "common.hpp"
#include "nif_file.hpp"
#include "ba2file.hpp"
#include "ddstxt.hpp"

#ifdef HAVE_SDL
#  include <SDL/SDL.h>

static void renderCubeMap(const BA2File& ba2File,
                          const std::string& texturePath,
                          int imageWidth, int imageHeight)
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
  try
  {
    SDL_Surface *sdlScreen =
        SDL_SetVideoMode(imageWidth, imageHeight, 24, SDL_SWSURFACE);
    if (!sdlScreen)
      throw errorMessage("error setting SDL video mode");
    size_t  imageDataSize = size_t(imageWidth) * size_t(imageHeight);
    std::vector< unsigned int > outBufRGBW(imageDataSize, 0U);
    float   viewRotationX = 0.0f;
    float   viewRotationY = 0.0f;
    float   viewRotationZ = 0.0f;
    float   scale =
        1.0f / float(imageWidth > imageHeight ? imageWidth : imageHeight);
    while (true)
    {
      NIFFile::NIFVertexTransform
          viewTransform(1.0f, viewRotationX, viewRotationY, viewRotationZ,
                        0.0f, 0.0f, 0.0f);
      for (int yc = 0; yc < imageHeight; yc++)
      {
        for (int xc = 0; xc < imageWidth; xc++)
        {
          float   x = float(xc - (imageWidth >> 1)) * scale;
          float   y = 1.0f;
          float   z = float((imageHeight >> 1) - yc) * scale;
          viewTransform.rotateXYZ(x, y, z);
          unsigned int  c = texture.cubeMap(x, y, z, 0);
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
            dstPtr[0] = (unsigned char) ((c >> 16) & 0xFF);     // B
            dstPtr[1] = (unsigned char) ((c >> 8) & 0xFF);      // G
            dstPtr[2] = (unsigned char) (c & 0xFF);             // R
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
            switch (event.key.keysym.sym)
            {
              case SDLK_a:
                viewRotationZ -= 0.19634954f;           // 11.25 degrees
                break;
              case SDLK_d:
                viewRotationZ += 0.19634954f;
                break;
              case SDLK_s:
                viewRotationX += 0.19634954f;
                break;
              case SDLK_w:
                viewRotationX -= 0.19634954f;
                break;
              case SDLK_q:
                viewRotationY += 0.19634954f;
                break;
              case SDLK_e:
                viewRotationY -= 0.19634954f;
                break;
              case SDLK_ESCAPE:
                quitFlag = true;
                break;
              default:
                continue;
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
    if (argc != 5)
    {
      std::fprintf(stderr,
                   "Usage: cubeview WIDTH HEIGHT ARCHIVEPATH TEXTURE.DDS\n");
      return 1;
    }
    int     renderWidth =
        int(parseInteger(argv[1], 10, "invalid image width", 8, 16384));
    int     renderHeight =
        int(parseInteger(argv[2], 10, "invalid image height", 8, 16384));
    BA2File ba2File(argv[3], ".dds");
    renderCubeMap(ba2File, std::string(argv[4]), renderWidth, renderHeight);
#endif
  }
  catch (std::exception& e)
  {
    std::fprintf(stderr, "cubeview: %s\n", e.what());
    return 1;
  }
  return 0;
}

