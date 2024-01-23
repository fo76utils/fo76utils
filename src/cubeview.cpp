
#include "common.hpp"
#include "nif_file.hpp"
#include "ba2file.hpp"
#include "ddstxt.hpp"
#include "sdlvideo.hpp"

#include <ctime>
#include <thread>

static void renderCubeMapThread(
    std::uint32_t *outBuf, int w, int h, int y0, int y1,
    const DDSTexture *texture, const NIFFile::NIFVertexTransform *vt,
    float viewScale, float mipLevel, bool isSRGB, float rgbScale)
{
  FloatVector4  tmpX(vt->rotateXX, vt->rotateYX, vt->rotateZX, vt->rotateXY);
  FloatVector4  tmpY(vt->rotateXY, vt->rotateYY, vt->rotateZY, vt->rotateXZ);
  tmpY *= viewScale;
  outBuf = outBuf + (size_t(y0) * size_t(w));
  for (int yc = y0; yc < y1; yc++)
  {
    FloatVector4  tmpYZ(vt->rotateXZ, vt->rotateYZ, vt->rotateZZ, vt->scale);
    tmpYZ *= ((float(h) * 0.5f) - float(yc));
    tmpYZ += tmpY;
    for (int xc = 0; xc < w; xc++)
    {
      FloatVector4  v(tmpYZ);
      v += (tmpX * (float(xc) - (float(w) * 0.5f)));
      FloatVector4  c(texture->cubeMap(v[0], v[1], v[2], mipLevel));
      c *= rgbScale;
      if (isSRGB)
      {
        c *= (1.0f / 255.0f);
        c.srgbCompress();
      }
      outBuf[xc] = c.convertToRGBA32(false, false);
    }
    outBuf = outBuf + w;
  }
}

static void renderTextureThread(
    std::uint32_t *outBuf, int w, int h, int y0, int y1,
    const DDSTexture *texture, bool isSRGB, bool fixNormalMap, bool enableAlpha)
{
  float   txtW = float(texture->getWidth());
  float   txtH = float(texture->getHeight());
  float   scale = std::max(txtW / float(w), txtH / float(h));
  float   uScale = scale / txtW;
  float   vScale = scale / txtH;
  float   uOffset = float(w) * -0.5f * uScale + 0.5f;
  float   vOffset = float(h) * -0.5f * vScale + 0.5f;
  float   mipLevel = FloatVector4::log2Fast(std::max(scale, 1.0f));
  mipLevel = std::min(std::max(mipLevel, 0.0f), 15.0f);
  outBuf = outBuf + (size_t(y0) * size_t(w));
  for (int yc = y0; yc < y1; yc++)
  {
    for (int xc = 0; xc < w; xc++)
    {
      float   u = float(xc) * uScale + uOffset;
      float   v = float(yc) * vScale + vOffset;
      if (u > -0.000001f && u < 1.000001f && v > -0.000001f && v < 1.000001f)
      {
        FloatVector4  c(texture->getPixelTC(u, v, mipLevel));
        float   a = c[3];
        if (fixNormalMap)
        {
          FloatVector4  tmp(c);
          tmp -= 127.5f;
          c[2] = float(std::sqrt(std::max(16256.25f - tmp.dotProduct2(tmp),
                                          0.0f))) + 127.5f;
        }
        else if (isSRGB)
        {
          c *= (1.0f / 255.0f);
          c.srgbCompress();
        }
        if (enableAlpha)
        {
          FloatVector4  c0(FloatVector4::convertRGBA32(outBuf[xc]));
          c = c0 + ((c - c0) * (a * (1.0f / 255.0f)));
        }
        outBuf[xc] = c.convertToRGBA32(false, true);
      }
    }
    outBuf = outBuf + w;
  }
}

static void saveScreenshot(SDLDisplay& display, const std::string& ddsFileName,
                           const DDSTexture *texture = (DDSTexture *) 0,
                           bool enableAlpha = true, bool fixNormalMap = false)
{
  try
  {
    size_t  n1 = ddsFileName.rfind('/');
    n1 = (n1 != std::string::npos ? (n1 + 1) : 0);
    size_t  n2 = ddsFileName.rfind('.');
    n2 = (n2 != std::string::npos ? n2 : 0);
    std::string fileName;
    if (n2 > n1)
      fileName.assign(ddsFileName, n1, n2 - n1);
    else
      fileName = "cubeview";
    std::time_t t = std::time((std::time_t *) 0);
    {
      unsigned int  s = (unsigned int) (t % (std::time_t) (24 * 60 * 60));
      unsigned int  m = s / 60U;
      s = s % 60U;
      unsigned int  h = m / 60U;
      m = m % 60U;
      h = h % 24U;
      char    buf[16];
      std::sprintf(buf, "_%02u%02u%02u.dds", h, m, s);
      fileName += buf;
    }
    if (texture)
    {
      int     w = texture->getWidth();
      int     h = texture->getHeight();
      DDSOutputFile f(fileName.c_str(), w, h, DDSInputFile::pixelFormatRGBA32);
      for (size_t i = 0; i < texture->getTextureCount(); i++)
      {
        for (int y = 0; y < h; y++)
        {
          for (int x = 0; x < w; x++)
          {
            std::uint32_t c = texture->getPixelN(x, y, 0, int(i));
            if (fixNormalMap)
            {
              FloatVector4  tmp(c);
              tmp -= 127.5f;
              tmp[2] = float(std::sqrt(std::max(16256.25f
                                                - tmp.dotProduct2(tmp), 0.0f)));
              tmp += 127.5f;
              c = std::uint32_t(tmp);
            }
            f.writeByte((unsigned char) ((c >> 16) & 0xFFU));
            f.writeByte((unsigned char) ((c >> 8) & 0xFFU));
            f.writeByte((unsigned char) (c & 0xFFU));
            f.writeByte((unsigned char) (!enableAlpha ? 0xFFU : (c >> 24)));
          }
        }
      }
    }
    else
    {
      int     w = display.getWidth() >> display.getDownsampleLevel();
      int     h = display.getHeight() >> display.getDownsampleLevel();
      display.blitSurface();
      const std::uint32_t *p = display.lockScreenSurface();
      DDSOutputFile f(fileName.c_str(), w, h, DDSInputFile::pixelFormatRGB24);
      size_t  pitch = display.getPitch();
      for (int y = 0; y < h; y++, p = p + pitch)
        f.writeImageData(p, size_t(w), DDSInputFile::pixelFormatRGB24);
      display.unlockScreenSurface();
    }
    display.consolePrint("Saved screenshot to %s\n", fileName.c_str());
  }
  catch (std::exception& e)
  {
    display.unlockScreenSurface();
    display.consolePrint("\033[41m\033[33m\033[1m    Error: %s    \033[m\n",
                         e.what());
  }
}

static const char *keyboardUsageString =
    "  \033[4m\033[38;5;228mA\033[m, "
    "\033[4m\033[38;5;228mD\033[m                  "
    "Turn left or right.                                             \n"
    "  \033[4m\033[38;5;228mW\033[m, "
    "\033[4m\033[38;5;228mS\033[m                  "
    "Look up or down.                                                \n"
    "  \033[4m\033[38;5;228mQ\033[m, "
    "\033[4m\033[38;5;228mE\033[m                  "
    "Roll left or right.                                             \n"
    "  \033[4m\033[38;5;228mInsert\033[m, "
    "\033[4m\033[38;5;228mDelete\033[m        "
    "Zoom cube map view in or out.                                   \n"
    "  \033[4m\033[38;5;228mHome\033[m                  "
    "Reset view direction, FOV and brightness.                       \n"
    "  \033[4m\033[38;5;228mPage Up\033[m               "
    "Enable gamma correction.                                        \n"
    "  \033[4m\033[38;5;228mPage Down\033[m             "
    "Disable gamma correction.                                       \n"
    "  \033[4m\033[38;5;228m0\033[m "
    "to \033[4m\033[38;5;228m9\033[m                "
    "Set mip level in cube map mode.                                 \n"
    "  \033[4m\033[38;5;228m+\033[m, "
    "\033[4m\033[38;5;228m-\033[m                  "
    "Increase or decrease cube map mip level by 0.125.               \n"
    "  \033[4m\033[38;5;228mL\033[m, "
    "\033[4m\033[38;5;228mK\033[m                  "
    "Increase or decrease cube map brightness.                       \n"
    "  \033[4m\033[38;5;228mSpace\033[m, "
    "\033[4m\033[38;5;228mBackspace\033[m      "
    "Load next or previous file matching the pattern.                \n"
    "  \033[4m\033[38;5;228mF1\033[m                    "
    "Cube map view mode.                                             \n"
    "  \033[4m\033[38;5;228mF2\033[m                    "
    "Texture view mode (disables view controls).                     \n"
    "  \033[4m\033[38;5;228mF3\033[m                    "
    "Disable normal map Z channel fix.                               \n"
    "  \033[4m\033[38;5;228mF4\033[m                    "
    "Enable normal map Z channel fix (texture view only).            \n"
    "  \033[4m\033[38;5;228mF5\033[m                    "
    "Disable alpha blending.                                         \n"
    "  \033[4m\033[38;5;228mF6\033[m                    "
    "Enable alpha blending (texture view only).                      \n"
    "  \033[4m\033[38;5;228mF9\033[m                    "
    "Select file from list.                                          \n"
    "  \033[4m\033[38;5;228mF11\033[m                   "
    "Save decompressed texture.                                      \n"
    "  \033[4m\033[38;5;228mF12\033[m "
    "or \033[4m\033[38;5;228mPrint Screen\033[m   "
    "Save screenshot.                                                \n"
    "  \033[4m\033[38;5;228mH\033[m                     "
    "Show help screen.                                               \n"
    "  \033[4m\033[38;5;228mC\033[m                     "
    "Clear messages.                                                 \n"
    "  \033[4m\033[38;5;228mEsc\033[m                   "
    "Quit viewer.                                                    \n";

static void renderCubeMap(const BA2File& ba2File,
                          const std::vector< std::string >& texturePaths,
                          int imageWidth, int imageHeight, float mipLevel)
{
  SDLDisplay  display(imageWidth, imageHeight, "cubeview", 4U, 48);
  display.setDefaultTextColor(0x00, 0xC0);
  std::vector< unsigned char >  fileBuf;
  float   viewRotationX = 0.0f;
  float   viewRotationY = 0.0f;
  float   viewRotationZ = 0.0f;
  float   mouseViewOffsX = 0.0f;
  float   mouseViewOffsZ = 0.0f;
  int     viewScale = 0;
  size_t  fileNum = 0;
  bool    quitFlag = false;
  bool    cubeViewMode = true;
  bool    enableGamma = false;
  bool    fixNormalMap = false;
  bool    enableAlpha = true;
  unsigned char screenshotFlag = 0;     // 1: save screenshot, 2: save texture
  float   cubeMapRGBScale = 1.0f;
  std::vector< SDLDisplay::SDLEvent > eventBuf;
  DDSTexture  *texture = (DDSTexture *) 0;
  if (texturePaths.size() > 10)
  {
    int     n = display.browseFile(texturePaths, "Select texture file",
                                   int(fileNum), 0x0B080F04FFFFULL);
    if (n >= 0 && size_t(n) < texturePaths.size())
      fileNum = size_t(n);
    else if (n < -1)
      return;
  }
  do
  {
    if (!texture)
    {
      try
      {
        ba2File.extractFile(fileBuf, texturePaths[fileNum]);
        texture = new DDSTexture(fileBuf.data(), fileBuf.size());
        display.clearTextBuffer();
        cubeViewMode = texture->getIsCubeMap();
        display.consolePrint("%s (%dx%d, %s)\n",
                             texturePaths[fileNum].c_str(),
                             texture->getWidth(), texture->getHeight(),
                             texture->getFormatName());
      }
      catch (std::exception& e)
      {
        display.consolePrint("\033[41m\033[33m\033[1m    Error: %s    \033[m\n",
                             e.what());
      }
    }
    if (texture)
    {
      if (!cubeViewMode)
        display.clearSurface(!enableAlpha ? 0U : 0x02888444U);
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
          if (!cubeViewMode)
          {
            threads[i] =
                new std::thread(renderTextureThread, dstPtr,
                                imageWidth, imageHeight, y0, y1, texture,
                                enableGamma, fixNormalMap, enableAlpha);
          }
          else
          {
            threads[i] =
                new std::thread(renderCubeMapThread, dstPtr,
                                imageWidth, imageHeight, y0, y1, texture,
                                &viewTransformInv, viewScale_f, mipLevel,
                                enableGamma, cubeMapRGBScale);
          }
        }
        catch (...)
        {
          threads[i] = (std::thread *) 0;
          if (!cubeViewMode)
          {
            renderTextureThread(
                dstPtr, imageWidth, imageHeight, y0, y1, texture,
                enableGamma, fixNormalMap, enableAlpha);
          }
          else
          {
            renderCubeMapThread(
                dstPtr, imageWidth, imageHeight, y0, y1, texture,
                &viewTransformInv, viewScale_f, mipLevel, enableGamma,
                cubeMapRGBScale);
          }
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
      if (screenshotFlag)
      {
        saveScreenshot(display, texturePaths[fileNum],
                       (screenshotFlag == 1 ? (DDSTexture *) 0 : texture),
                       enableAlpha, fixNormalMap);
      }
    }
    screenshotFlag = 0;
    display.drawText(0, -1, display.getTextRows(), 0.75f, 1.0f);
    display.blitSurface();
    bool    redrawFlag = false;
    while (!(redrawFlag || quitFlag))
    {
      display.pollEvents(eventBuf, -1000, false, true);
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
              display.blitSurface();
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
              case 'k':
              case 'l':
                {
                  int     tmp =
                      roundFloat(float(std::log2(cubeMapRGBScale)) * 4.0f);
                  tmp += (d1 == 'k' ? -1 : 1);
                  tmp = std::min< int >(std::max< int >(tmp, -4), 8);
                  cubeMapRGBScale = float(std::exp2(float(tmp) * 0.25f));
                }
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
                cubeMapRGBScale = 1.0f;
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
              case ' ':
                if (texturePaths.size() > 1)
                {
                  if (texture)
                  {
                    delete texture;
                    texture = (DDSTexture *) 0;
                  }
                  fileNum++;
                  if (fileNum >= texturePaths.size())
                    fileNum = 0;
                  redrawFlag = true;
                }
                break;
              case SDLDisplay::SDLKeySymBackspace:
                if (texturePaths.size() > 1)
                {
                  if (texture)
                  {
                    delete texture;
                    texture = (DDSTexture *) 0;
                  }
                  if (fileNum < 1)
                    fileNum = texturePaths.size();
                  fileNum--;
                  redrawFlag = true;
                }
                break;
              case SDLDisplay::SDLKeySymF1:
                if (!cubeViewMode)
                {
                  cubeViewMode = true;
                  if (texture)
                    redrawFlag = true;
                  display.consolePrint("Cube map view mode\n");
                }
                break;
              case SDLDisplay::SDLKeySymF2:
                if (cubeViewMode)
                {
                  cubeViewMode = false;
                  if (texture)
                    redrawFlag = true;
                  display.consolePrint("Texture view mode\n");
                }
                break;
              case SDLDisplay::SDLKeySymF3:
                if (fixNormalMap)
                {
                  fixNormalMap = false;
                  if (texture)
                    redrawFlag = true;
                  display.consolePrint("Normal map Z channel fix disabled\n");
                }
                break;
              case SDLDisplay::SDLKeySymF4:
                if (!fixNormalMap)
                {
                  fixNormalMap = true;
                  if (texture)
                    redrawFlag = true;
                  display.consolePrint("Normal map Z channel fix enabled\n");
                }
                break;
              case SDLDisplay::SDLKeySymF5:
                if (enableAlpha)
                {
                  enableAlpha = false;
                  if (texture)
                    redrawFlag = true;
                  display.consolePrint("Texture alpha blending disabled\n");
                }
                break;
              case SDLDisplay::SDLKeySymF6:
                if (!enableAlpha)
                {
                  enableAlpha = true;
                  if (texture)
                    redrawFlag = true;
                  display.consolePrint("Texture alpha blending enabled\n");
                }
                break;
              case SDLDisplay::SDLKeySymF9:
                {
                  int     n = display.browseFile(
                                  texturePaths, "Select texture file",
                                  int(fileNum), 0x0B080F04FFFFULL);
                  if (n >= 0 && size_t(n) < texturePaths.size() &&
                      size_t(n) != fileNum)
                  {
                    fileNum = size_t(n);
                    redrawFlag = true;
                    if (texture)
                    {
                      delete texture;
                      texture = (DDSTexture *) 0;
                    }
                  }
                  else if (n < -1)
                  {
                    quitFlag = true;
                  }
                }
                break;
              case SDLDisplay::SDLKeySymF11:
                if (texture)
                {
                  screenshotFlag = 2;
                  redrawFlag = true;
                }
                break;
              case SDLDisplay::SDLKeySymF12:
              case SDLDisplay::SDLKeySymPrintScr:
                if (texture)
                {
                  screenshotFlag = 1;
                  redrawFlag = true;
                }
                break;
              case 'h':
                display.clearTextBuffer();
                display.printString(keyboardUsageString);
                redrawFlag = true;
                break;
              case 'c':
                display.clearTextBuffer();
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
            if (texture)
            {
              mouseViewOffsZ = float((imageWidth >> 1) - d1)
                               * (3.14159265f / float(imageHeight));
              mouseViewOffsX = float((imageHeight >> 1) - d2)
                               * (3.14159265f / float(imageHeight));
              redrawFlag = true;
            }
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
  while (!quitFlag);
  if (texture)
    delete texture;
}

int main(int argc, char **argv)
{
  try
  {
    if (argc != 5 && argc != 6)
    {
      SDLDisplay::enableConsole();
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
    BA2File ba2File(argv[3], argv[4]);
    std::vector< std::string >  fileNames;
    ba2File.getFileList(fileNames);
    if (fileNames.size() < 1)
      errorMessage("no matching files found");
    renderCubeMap(ba2File, fileNames, renderWidth, renderHeight, mipLevel);
  }
  catch (std::exception& e)
  {
    SDLDisplay::enableConsole();
    std::fprintf(stderr, "cubeview: %s\n", e.what());
    return 1;
  }
  return 0;
}

