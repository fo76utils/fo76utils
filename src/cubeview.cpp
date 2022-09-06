
#include "common.hpp"
#include "nif_file.hpp"
#include "ba2file.hpp"
#include "ddstxt.hpp"
#include "sdlvideo.hpp"

#include <thread>

static void renderCubeMapThread(std::uint32_t *outBuf, int w, int h,
                                int y0, int y1, const DDSTexture *texture,
                                const NIFFile::NIFVertexTransform *vt,
                                float viewScale, float mipLevel, bool isSRGB)
{
  outBuf = outBuf + (size_t(y0) * size_t(w));
  for (int yc = y0; yc < y1; yc++)
  {
    for (int xc = 0; xc < w; xc++)
    {
      float   x = float(xc) - (float(w) * 0.5f);
      float   y = viewScale;
      float   z = (float(h) * 0.5f) - float(yc);
      vt->rotateXYZ(x, y, z);
      FloatVector4  c(texture->cubeMap(x, y, z, mipLevel));
      if (isSRGB)
      {
        c *= (1.0f / 255.0f);
        c.srgbCompress();
      }
      outBuf[xc] = (std::uint32_t) c;
    }
    outBuf = outBuf + w;
  }
}

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
  SDLDisplay  display(imageWidth, imageHeight, "cubeview");
  float   viewRotationX = 0.0f;
  float   viewRotationY = 0.0f;
  float   viewRotationZ = 0.0f;
  float   mouseViewOffsX = 0.0f;
  float   mouseViewOffsZ = 0.0f;
  int     viewScale = 0;
  bool    enableGamma = false;
  std::vector< SDLDisplay::SDLEvent > eventBuf;
  while (true)
  {
    NIFFile::NIFVertexTransform viewTransform(
        1.0f, viewRotationX + mouseViewOffsX, viewRotationY,
        viewRotationZ + mouseViewOffsZ, 0.0f, 0.0f, 0.0f);
    NIFFile::NIFVertexTransform viewTransformInv(viewTransform);
    viewTransformInv.rotateYX = viewTransform.rotateXY;
    viewTransformInv.rotateZX = viewTransform.rotateXZ;
    viewTransformInv.rotateXY = viewTransform.rotateYX;
    viewTransformInv.rotateZY = viewTransform.rotateYZ;
    viewTransformInv.rotateXZ = viewTransform.rotateZX;
    viewTransformInv.rotateYZ = viewTransform.rotateZY;
    float   viewScale_f =
        float(imageHeight) * float(std::pow(2.0f, float(viewScale) * 0.25f));
    std::thread *threads[16];
    int     threadCnt = int(std::thread::hardware_concurrency());
    threadCnt = (threadCnt > 1 ? (threadCnt < 16 ? threadCnt : 16) : 1);
    threadCnt = (threadCnt < imageHeight ? threadCnt : imageHeight);
    std::uint32_t *dstPtr = display.lockDrawSurface();
    int     y0 = 0;
    for (int i = 0; i < threadCnt; i++)
    {
      int     y1 = (imageHeight * (i + 1)) / threadCnt;
      try
      {
        threads[i] = new std::thread(renderCubeMapThread, dstPtr,
                                     imageWidth, imageHeight, y0, y1, &texture,
                                     &viewTransformInv, viewScale_f, mipLevel,
                                     enableGamma);
      }
      catch (...)
      {
        threads[i] = (std::thread *) 0;
        renderCubeMapThread(dstPtr, imageWidth, imageHeight, y0, y1, &texture,
                            &viewTransformInv, viewScale_f, mipLevel,
                            enableGamma);
      }
      y0 = y1;
    }
    for (int i = 0; i < threadCnt; i++)
    {
      if (threads[i])
      {
        threads[i]->join();
        delete threads[i];
      }
    }
    display.unlockDrawSurface();
    display.blitSurface();
    bool    redrawFlag = false;
    bool    quitFlag = false;
    while (!(redrawFlag || quitFlag))
    {
      display.pollEvents(eventBuf, 10, false, true);
      for (size_t i = 0; i < eventBuf.size(); i++)
      {
        int     t = eventBuf[i].type();
        int     d1 = eventBuf[i].data1();
        int     d2 = eventBuf[i].data2();
        switch (t)
        {
          case SDLDisplay::SDLEventWindow:
            if (d1 == 0)
              quitFlag = true;
            else if (d1 == 1)
              redrawFlag = true;
            break;
          case SDLDisplay::SDLEventKeyRepeat:
          case SDLDisplay::SDLEventKeyDown:
            redrawFlag = true;
            switch (d1)
            {
              case '0':
              case '1':
              case '2':
              case '3':
              case '4':
              case '5':
              case '6':
              case '7':
              case '8':
              case '9':
                mipLevel = float(d1 - '0');
                break;
              case '-':
              case SDLDisplay::SDLKeySymKPMinus:
                mipLevel = (mipLevel > 0.125f ? (mipLevel - 0.125f) : 0.0f);
                break;
              case '=':
              case SDLDisplay::SDLKeySymKPPlus:
                mipLevel = (mipLevel < 8.875f ? (mipLevel + 0.125f) : 9.0f);
                break;
              case 'a':
                viewRotationZ += 0.19634954f;           // 11.25 degrees
                break;
              case 'd':
                viewRotationZ -= 0.19634954f;
                break;
              case 's':
                viewRotationX -= 0.19634954f;
                break;
              case 'w':
                viewRotationX += 0.19634954f;
                break;
              case 'q':
                viewRotationY -= 0.19634954f;
                break;
              case 'e':
                viewRotationY += 0.19634954f;
                break;
              case SDLDisplay::SDLKeySymHome:
                viewRotationX = 0.0f;
                viewRotationY = 0.0f;
                viewRotationZ = 0.0f;
                viewScale = 0;
                break;
              case SDLDisplay::SDLKeySymInsert:
                viewScale += int(viewScale < 16);
                break;
              case SDLDisplay::SDLKeySymDelete:
                viewScale -= int(viewScale > -16);
                break;
              case SDLDisplay::SDLKeySymPageUp:
              case SDLDisplay::SDLKeySymPageDown:
                enableGamma = (d1 == SDLDisplay::SDLKeySymPageUp);
                break;
              case SDLDisplay::SDLKeySymEscape:
                quitFlag = true;
                break;
              default:
                redrawFlag = false;
                break;
            }
            break;
          case SDLDisplay::SDLEventMButtonUp:
          case SDLDisplay::SDLEventMButtonDown:
          case SDLDisplay::SDLEventMouseMotion:
            mouseViewOffsZ = float((imageWidth >> 1) - d1)
                             * (3.14159265f / float(imageHeight));
            mouseViewOffsX = float((imageHeight >> 1) - d2)
                             * (3.14159265f / float(imageHeight));
            redrawFlag = true;
            break;
        }
      }
    }
    if (quitFlag)
      break;
    if (viewRotationX < -3.14159265f)
      viewRotationX += 6.28318531f;
    else if (viewRotationX > 3.14159265f)
      viewRotationX -= 6.28318531f;
    if (viewRotationY < -3.14159265f)
      viewRotationY += 6.28318531f;
    else if (viewRotationY > 3.14159265f)
      viewRotationY -= 6.28318531f;
    if (viewRotationZ < -3.14159265f)
      viewRotationZ += 6.28318531f;
    else if (viewRotationZ > 3.14159265f)
      viewRotationZ -= 6.28318531f;
  }
}

int main(int argc, char **argv)
{
  SDLDisplay::enableConsole();
  try
  {
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
  }
  catch (std::exception& e)
  {
    std::fprintf(stderr, "cubeview: %s\n", e.what());
    return 1;
  }
  return 0;
}

