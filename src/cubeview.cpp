
#include "common.hpp"
#include "nif_file.hpp"
#include "ba2file.hpp"
#include "ddstxt.hpp"

#ifdef HAVE_SDL2
#  include <SDL2/SDL.h>

static void renderCubeMap(const BA2File& ba2File,
                          const std::string& texturePath,
                          int imageWidth, int imageHeight, float mipLevel)
{
  std::vector< unsigned char >  fileBuf;
  ba2File.extractFile(fileBuf, texturePath);
  DDSTexture  texture(&(fileBuf.front()), fileBuf.size());
  if (!texture.getIsCubeMap())
  {
    std::fprintf(stderr, "Warning: %s is not a cube map\n",
                 texturePath.c_str());
  }
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS) != 0)
    throw errorMessage("error initializing SDL");
  bool    enableFullScreen = false;
  {
    SDL_DisplayMode m;
    if (SDL_GetDesktopDisplayMode(0, &m) == 0)
      enableFullScreen = (imageWidth == m.w && imageHeight == m.h);
  }
  SDL_Window  *sdlWindow = (SDL_Window *) 0;
  SDL_Surface *sdlScreen = (SDL_Surface *) 0;
  try
  {
    sdlWindow =
        SDL_CreateWindow("cubeview",
                         SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                         imageWidth, imageHeight,
                         (enableFullScreen ? SDL_WINDOW_FULLSCREEN : 0));
    if (!sdlWindow)
      throw errorMessage("error creating SDL window");
    sdlScreen = SDL_CreateRGBSurface(0, imageWidth, imageHeight, 32,
                                     0x000000FFU, 0x0000FF00U,
                                     0x00FF0000U, 0xFF000000U);
    if (!sdlScreen)
      throw errorMessage("error setting SDL video mode");
    SDL_SetSurfaceBlendMode(sdlScreen, SDL_BLENDMODE_NONE);
    NIFFile::NIFVertexTransform viewTransform;
    int     viewScale = 0;
    bool    enableGamma = false;
    while (true)
    {
      float   y0 =
          float(imageHeight) * float(std::pow(2.0f, float(viewScale) * 0.25f));
      SDL_LockSurface(sdlScreen);
      for (int yc = 0; yc < imageHeight; yc++)
      {
        Uint32  *dstPtr = reinterpret_cast< Uint32 * >(sdlScreen->pixels);
        dstPtr = dstPtr + (size_t(sdlScreen->pitch >> 2) * size_t(yc));
        for (int xc = 0; xc < imageWidth; xc++, dstPtr++)
        {
          float   x = float(xc - (imageWidth >> 1));
          float   y = y0;
          float   z = float((imageHeight >> 1) - yc);
          viewTransform.rotateXYZ(x, y, z);
          FloatVector4  c(texture.cubeMap(x, y, z, mipLevel));
          if (enableGamma)
          {
            c *= (1.0f / 255.0f);
            c.srgbCompress();
          }
          *dstPtr = (unsigned int) c;
        }
      }
      SDL_UnlockSurface(sdlScreen);
      SDL_BlitSurface(sdlScreen, (SDL_Rect *) 0,
                      SDL_GetWindowSurface(sdlWindow), (SDL_Rect *) 0);
      SDL_UpdateWindowSurface(sdlWindow);
      {
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
                mipLevel = float(int(event.key.keysym.sym - SDLK_0));
                break;
              case SDLK_MINUS:
              case SDLK_KP_MINUS:
                mipLevel = (mipLevel > 0.125f ? (mipLevel - 0.125f) : 0.0f);
                break;
              case SDLK_EQUALS:
              case SDLK_KP_PLUS:
                mipLevel = (mipLevel < 8.875f ? (mipLevel + 0.125f) : 9.0f);
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
              case SDLK_HOME:
                viewTransform = NIFFile::NIFVertexTransform();
                viewScale = 0;
                break;
              case SDLK_INSERT:
                viewScale += int(viewScale < 16);
                break;
              case SDLK_DELETE:
                viewScale -= int(viewScale > -16);
                break;
              case SDLK_PAGEUP:
              case SDLK_PAGEDOWN:
                enableGamma = (event.key.keysym.sym == SDLK_PAGEUP);
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
              FloatVector4  v(vt.rotateXX, vt.rotateYX,
                              vt.rotateZX, vt.rotateXY);
              float   tmp = v.dotProduct(v);
              v = FloatVector4(vt.rotateYY, vt.rotateZY,
                               vt.rotateXZ, vt.rotateYZ);
              tmp += v.dotProduct(v);
              tmp += (vt.rotateZZ * vt.rotateZZ);
              tmp = float(std::sqrt(3.0f / tmp));
              vt.rotateXX *= tmp;
              vt.rotateYX *= tmp;
              vt.rotateZX *= tmp;
              vt.rotateXY *= tmp;
              vt.rotateYY *= tmp;
              vt.rotateZY *= tmp;
              vt.rotateXZ *= tmp;
              vt.rotateYZ *= tmp;
              vt.rotateZZ *= tmp;
              viewTransform = vt;
            }
            keyPressed = true;
          }
        }
        if (!quitFlag)
          continue;
      }
      break;
    }
    SDL_FreeSurface(sdlScreen);
    SDL_DestroyWindow(sdlWindow);
    SDL_Quit();
  }
  catch (...)
  {
    if (sdlScreen)
      SDL_FreeSurface(sdlScreen);
    if (sdlWindow)
      SDL_DestroyWindow(sdlWindow);
    SDL_Quit();
    throw;
  }
}
#endif

int main(int argc, char **argv)
{
  try
  {
#ifndef HAVE_SDL2
    throw errorMessage("SDL 2 is required");
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
    float   mipLevel = 0.0f;
    if (argc > 5)
      mipLevel = float(parseFloat(argv[5], "invalid mip level", 0.0, 15.0));
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

