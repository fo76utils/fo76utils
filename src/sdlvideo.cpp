
#include "common.hpp"
#include "sdlvideo.hpp"
#include "fp32vec4.hpp"
#include "filebuf.hpp"
#include "ddstxt.hpp"
#include "zlib.hpp"

#include <thread>
#if defined(_WIN32) || defined(_WIN64)
#  include <windows.h>
#endif

#include "courb24.cpp"

void SDLDisplay::drawCharacterBG(std::uint32_t *p,
                                 int x, int y, std::uint32_t c, float bgAlpha)
{
  int     x0 = roundFloat(float(x) * textXScale);
  int     y0 = roundFloat(float(y) * textYScale);
  int     x1 = roundFloat(float(x + 1) * textXScale);
  int     y1 = roundFloat(float(y + 1) * textYScale);
  x0 = (x0 > 0 ? (x0 < (imageWidth - 1) ? x0 : (imageWidth - 1)) : 0);
  y0 = (y0 > 0 ? (y0 < (imageHeight - 1) ? y0 : (imageHeight - 1)) : 0);
  x1 = (x1 > 0 ? (x1 < (imageWidth - 1) ? x1 : (imageWidth - 1)) : 0);
  y1 = (y1 > 0 ? (y1 < (imageHeight - 1) ? y1 : (imageHeight - 1)) : 0);
  p = p + (size_t(y0) * size_t(imageWidth));
  if (bgAlpha < 1.0f)
  {
    FloatVector4  a0(1.0f - bgAlpha);
    FloatVector4  c1(FloatVector4::convertRGBA32(
                         ansiColor256Table[(c >> 16) & 0xFFU]));
    c1 = c1 * c1 * bgAlpha;
    for ( ; y0 < y1; y0++, p = p + imageWidth)
    {
      for (int i = x0; i < x1; i++)
      {
        FloatVector4  c0(FloatVector4::convertRGBA32(p[i]));
        p[i] =
            (c0 * c0 * a0 + c1).squareRootFast().convertToRGBA32(false, true);
      }
    }
  }
  else
  {
    std::uint32_t c1 = ansiColor256Table[(c >> 16) & 0xFFU];
    for ( ; y0 < y1; y0++, p = p + imageWidth)
    {
      for (int i = x0; i < x1; i++)
        p[i] = c1;
    }
  }
}

static const std::uint32_t  fontTextureDecodeTable[16] =
{
  0x00000000U, 0x00000001U, 0x00000100U, 0x00000101U,
  0x00010000U, 0x00010001U, 0x00010100U, 0x00010101U,
  0x01000000U, 0x01000001U, 0x01000100U, 0x01000101U,
  0x01010000U, 0x01010001U, 0x01010100U, 0x01010101U
};

void SDLDisplay::drawCharacterFG(std::uint32_t *p,
                                 int x, int y, std::uint32_t c, float fgAlpha)
{
  if (BRANCH_UNLIKELY(c & 0x4000U))     // underline
    drawCharacterFG(p, x, y, (c & 0xFFFF8000U) | 0x005FU, fgAlpha);
  size_t  n0 = 0;
  size_t  n2 = sizeof(courB24CharacterTable) / sizeof(unsigned short);
  {
    unsigned short  tmp = (unsigned short) (c & 0x3FFFU);
    // find in character table with binary search
    size_t  n1;
    while ((n1 = ((n0 + n2) >> 1)) > n0)
    {
      if (tmp < courB24CharacterTable[n1])
        n2 = n1;
      else
        n0 = n1;
    }
    if (tmp != courB24CharacterTable[n0])
      return;                   // not found
  }
  float   xc = (float(x) + 0.5f) * textXScale;
  float   yc = (float(y) + 0.5f) * textYScale;
  int     x0 = roundFloat(xc - (fontWidth * 0.5f));
  int     y0 = roundFloat(yc - (fontHeight * 0.5f));
  int     x1 = roundFloat(xc + (fontWidth * 0.5f));
  int     y1 = roundFloat(yc + (fontHeight * 0.5f));
  x0 = (x0 > 0 ? (x0 < (imageWidth - 1) ? x0 : (imageWidth - 1)) : 0);
  y0 = (y0 > 0 ? (y0 < (imageHeight - 1) ? y0 : (imageHeight - 1)) : 0);
  x1 = (x1 > 0 ? (x1 < (imageWidth - 1) ? x1 : (imageWidth - 1)) : 0);
  y1 = (y1 > 0 ? (y1 < (imageHeight - 1) ? y1 : (imageHeight - 1)) : 0);
  p = p + (size_t(y0) * size_t(imageWidth));
  // font texture size = 1024x1024, font size = 28x40, 36 characters per row
  float   uOffset = float(int(((unsigned int) n0 % 36U) * 28U + 14U));
  float   vOffset = float(int(((unsigned int) n0 / 36U) * 40U + 22U));
  uOffset = uOffset - (xc * fontUScale);
  vOffset = vOffset - (yc * fontVScale);
  std::uint32_t c1 = ansiColor256Table[(c >> 24) & 0xFFU];
  FloatVector4  c1l(FloatVector4::convertRGBA32(
                        ansiColor256Table[(c >> 24) & 0xFFU]));
  c1l *= c1l;
  float   v = float(y0) * fontVScale + vOffset;
  for ( ; y0 < y1; y0++, p = p + imageWidth, v = v + fontVScale)
  {
    float   v_f = float(std::floor(v));
    int     v_i = int(v_f) & 1023;
    v_f = v - v_f;
    const unsigned char *textureDataPtr =
        &(fontData.front()) + (size_t(v_i) << 10);
    float   u = float(x0) * fontUScale + uOffset;
    for (int i = x0; i < x1; i++, u = u + fontUScale)
    {
      float   u_f = float(std::floor(u));
      int     u_i = int(u_f) & 1023;
      u_f = u - u_f;
      unsigned char tmp = textureDataPtr[u_i];
      if (!tmp)
        continue;
      if (tmp < 0x0F || fgAlpha < 1.0f)
      {
        float   a =
            FloatVector4(&(fontTextureDecodeTable[tmp])).dotProduct(
                FloatVector4(1.0f - u_f, u_f, 1.0f - u_f, u_f)
                * FloatVector4(1.0f - v_f, 1.0f - v_f, v_f, v_f)) * fgAlpha;
        FloatVector4  c0(FloatVector4::convertRGBA32(p[i]));
        c0 *= c0;
        p[i] = (c0 + ((c1l - c0) * a)).squareRootFast().convertToRGBA32(
                                                            false, true);
      }
      else
      {
        p[i] = c1;
      }
    }
  }
}

void SDLDisplay::drawTextThreadFunc(SDLDisplay *p, int y0Src, int y0Dst,
                                    int lineCnt, bool isBackground, float alpha)
{
  std::uint32_t *imageBufPtr;
  if (p->usingImageBuf)
    imageBufPtr = &(p->imageBuf.front());
  else
#ifdef HAVE_SDL2
    imageBufPtr = reinterpret_cast< std::uint32_t * >(p->sdlScreen->pixels);
#else
    imageBufPtr = &(p->screenBuf.front());
#endif
  size_t  w = size_t(p->textWidth);
  const std::uint32_t *srcPtr = &(p->textBuf.front()) + (size_t(y0Src) * w);
  for ( ; lineCnt > 0; lineCnt--, y0Dst++, srcPtr = srcPtr + w)
  {
    if (isBackground)
    {
      for (int x = 0; x < int(w); x++)
      {
        std::uint32_t c = srcPtr[x];
        if (!(c & 0x8000U))
          continue;                     // no background color
        p->drawCharacterBG(imageBufPtr, x, y0Dst, c, alpha);
      }
    }
    else
    {
      for (int x = 0; x < int(w); x++)
      {
        std::uint32_t c = srcPtr[x];
        if (!c || (c & 0x7F7FU) == 0x20U)
          continue;                     // empty, or space and not underlined
        p->drawCharacterFG(imageBufPtr, x, y0Dst, c, alpha);
      }
    }
  }
}

std::uint32_t SDLDisplay::decodeUTF8Character()
{
  std::uint32_t c = printBuf[0];
  int     n = 0;
  do
  {
    if (BRANCH_UNLIKELY(c < 0xC0U))
    {
      c = 0xFFFDU;                      // invalid continuation byte
      n = 1;
      break;
    }
    if (c < 0xE0U)                      // 110xxxxx 10xxxxxx
    {
      if (printBufCnt < 2)
        return 0U;
      if (BRANCH_UNLIKELY((printBuf[1] & 0xC0) != 0x80))
      {
        c = 0xFFFDU;                    // unexpected end of sequence
        n = 1;
        break;
      }
      c = ((c & 0x1FU) << 6) | ((std::uint32_t) printBuf[1] & 0x3FU);
      c = (c >= 0x0080U ? c : 0xFFFDU); // check for overlong encoding
      n = 2;
      break;
    }
    if (c < 0xF0U)                      // 1110xxxx 10xxxxxx 10xxxxxx
    {
      if (printBufCnt < 3)
        return 0U;
      std::uint32_t c1 = printBuf[1];
      std::uint32_t c2 = printBuf[2];
      if (BRANCH_UNLIKELY((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80))
      {
        c = 0xFFFDU;                    // unexpected end of sequence
        n = ((c1 & 0xC0) != 0x80 ? 1 : 2);
        break;
      }
      c = ((c & 0x0FU) << 12) | ((c1 & 0x3FU) << 6) | (c2 & 0x3FU);
      c = (c >= 0x0800U ? c : 0xFFFDU); // check for overlong encoding
      n = 3;
      break;
    }
    c = 0xFFFDU;                        // unsupported encoding
    if (printBufCnt >= 2 && (printBuf[1] & 0xC0) != 0x80)
      n = 1;
    else if (printBufCnt >= 3 && (printBuf[2] & 0xC0) != 0x80)
      n = 2;
    else if (printBufCnt >= 4)
      n = ((printBuf[3] & 0xC0) != 0x80 ? 3 : 4);
    else
      return 0U;
  }
  while (false);
  if (BRANCH_UNLIKELY(n < printBufCnt))
  {
    for (int i = n; i < printBufCnt; i++)
      printBuf[i - n] = printBuf[i];
  }
  printBufCnt -= n;
  return c;
}

inline void SDLDisplay::printCharacter(std::uint32_t c)
{
  if (printXPos >= textWidth)
  {
    printXPos = 0;
    printYPos++;
  }
  size_t  sizeRequired = size_t(printYPos + 1) * size_t(textWidth);
  if (sizeRequired > textBuf.size())
    textBuf.resize(sizeRequired, 0U);
  textBuf[sizeRequired - size_t(textWidth - printXPos)] = c | textColor;
  printXPos++;
}

SDLDisplay::SDLDisplay(int w, int h, const char *windowTitle,
                       unsigned int flags, int textRows, int textColumns)
{
#ifndef HAVE_SDL2
  (void) windowTitle;
#else
  if (!windowTitle)
    windowTitle = "";
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS) != 0)
    errorMessage("error initializing SDL");
  sdlWindow = (SDL_Window *) 0;
  sdlScreen = (SDL_Surface *) 0;
  try
#endif
  {
#ifdef HAVE_SDL2
    int     displayWidth = w;
    int     displayHeight = h;
    SDL_DisplayMode m;
    bool    isFullScreen = (SDL_GetDesktopDisplayMode(0, &m) == 0);
    if (isFullScreen)
    {
      displayWidth = m.w;
      displayHeight = m.h;
    }
    isDownsampled = ((w > displayWidth || h > displayHeight) && !((w | h) & 1));
    if (!(flags & 4U))
      isDownsampled = false;
    if (isDownsampled)
    {
      w = w >> 1;
      h = h >> 1;
    }
    if (w < 8 || w > (displayWidth << 1) || h < 8 || h > (displayHeight << 1))
      errorMessage("invalid window dimensions");
    if (!(w == displayWidth && h == displayHeight))
      isFullScreen = false;
    imageWidth = w << int(isDownsampled);
    imageHeight = h << int(isDownsampled);
    Uint32  windowFlags = 0U;
    if (isFullScreen)
      windowFlags = windowFlags | SDL_WINDOW_FULLSCREEN;
    if (flags & 1U)
    {
      windowFlags = windowFlags | SDL_WINDOW_OPENGL;
      SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
      SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
      SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
      SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
      SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
      SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, int(bool(flags & 2U)));
      SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
      SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                          SDL_GL_CONTEXT_PROFILE_CORE);
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
      SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);
    }
    sdlWindow = SDL_CreateWindow(
                    windowTitle,
                    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                    w, h, windowFlags);
    if (!sdlWindow)
      errorMessage("error creating SDL window");
    sdlScreen = SDL_CreateRGBSurface(
                    0, w, h, 32,
#if USE_PIXELFMT_RGB10A2
                    0x3FF00000U, 0x000FFC00U, 0x000003FFU, 0xC0000000U
#else
                    0x000000FFU, 0x0000FF00U, 0x00FF0000U, 0xFF000000U
#endif
                    );
    if (!sdlScreen)
      errorMessage("error setting SDL video mode");
    SDL_SetSurfaceBlendMode(sdlScreen, SDL_BLENDMODE_NONE);
    surfaceLockCnt = 0;
    usingImageBuf = (isDownsampled || int(sdlScreen->pitch >> 2) != imageWidth);
#else
    std::fprintf(stderr, "Warning: video display support "
                         "is not available without SDL 2\n");
    isDownsampled = bool(flags & 4U);
    if (isDownsampled)
    {
      w = w >> 1;
      h = h >> 1;
    }
    if (w < 8 || w > 32768 || h < 8 || h > 32768)
      errorMessage("invalid image dimensions");
    imageWidth = w << int(isDownsampled);
    imageHeight = h << int(isDownsampled);
    screenBuf.resize(size_t(w) * size_t(h), 0U);
    usingImageBuf = isDownsampled;
#endif
    if (usingImageBuf)
      imageBuf.resize(size_t(imageWidth) * size_t(imageHeight), 0U);
    fontData.resize(1024 * 1024);
    ZLibDecompressor::decompressData(
        &(fontData.front()), 131072,
        &(courB24FontData[0]), sizeof(courB24FontData) / sizeof(unsigned char));
    for (size_t i = fontData.size(); i-- > 0; )
    {
      size_t  j = i >> 3;
      unsigned char b = (unsigned char) (~i & 7);
      fontData[i] = (unsigned char) bool(fontData[j] & (1U << b));
    }
    for (size_t i = 0; i < fontData.size(); i++)
    {
      fontData[i] = (fontData[i] & 1)
                    | ((fontData[(i + 1) & (fontData.size() - 1)] & 1) << 1)
                    | ((fontData[(i + 1024) & (fontData.size() - 1)] & 1) << 2)
                    | ((fontData[(i + 1025) & (fontData.size() - 1)] & 1) << 3);
    }
    textWidth =
        (textColumns > 0 ? textColumns : (((textRows * w * 3) + h) / (h << 1)));
    textHeight = textRows;
    if ((textWidth * 5) > w)
      textWidth = w / 5;
    if ((textHeight * 8) > h)
      textHeight = h / 8;
    textBuf.reserve(size_t(textWidth) * size_t(textHeight));
    textXScale = float(imageWidth) / float(textWidth);
    textYScale = float(imageHeight) / float(textHeight);
    fontWidth = float(imageWidth * 26) / float(textWidth * 20);
    fontHeight = float(imageHeight * 38) / float(textHeight * 30);
    fontUScale = float(textWidth * 20) / float(imageWidth);
    fontVScale = float(textHeight * 30) / float(imageHeight);
    printXPos = 0;
    printYPos = 0;
    textColor = 0x00FF8000U;
    defaultTextColor = 0x00FF8000U;
    printBufCnt = 0;
    ansiColor256Table.resize(256);
    for (int i = 0; i < 256; i++)
    {
      std::uint32_t c = std::uint32_t(i);
      if (i < 16)
      {
        // standard and high intensity colors
        c = fontTextureDecodeTable[c & 7U] * 0xFFU;
        if (i < 7)
          c = c & 0x00808080U;
        else if (i < 9)
          c = (c & 0x00404040U) | 0x00808080U;
      }
      else if (i < 232)
      {
        // 6x6x6 color cube
        std::uint32_t b = (c - 16U) % 6U;
        std::uint32_t g = ((c - 16U) / 6U) % 6U;
        std::uint32_t r = ((c - 16U) / 6U) / 6U;
        c = (r | (g << 8) | (b << 16)) * 51U;
      }
      else
      {
        // grayscale colors
        c = (c - 232U) * 0x000A0A0AU + 0x00080808U;
      }
#if USE_PIXELFMT_RGB10A2
      ansiColor256Table[i] = FloatVector4(c).convertToRGBA32(true, true);
#else
      ansiColor256Table[i] = c | 0xFF000000U;
#endif
    }
  }
#ifdef HAVE_SDL2
  catch (...)
  {
    if (sdlScreen)
      SDL_FreeSurface(sdlScreen);
    if (sdlWindow)
      SDL_DestroyWindow(sdlWindow);
    SDL_Quit();
    throw;
  }
#endif
}

SDLDisplay::~SDLDisplay()
{
#ifdef HAVE_SDL2
  while (surfaceLockCnt > 0)
    unlockScreenSurface();
  if (sdlScreen)
    SDL_FreeSurface(sdlScreen);
  if (sdlWindow)
    SDL_DestroyWindow(sdlWindow);
  SDL_Quit();
#endif
}

void SDLDisplay::enableConsole()
{
#ifdef HAVE_SDL2
#  if defined(_WIN32) || defined(_WIN64)
  if (!std::getenv("TERM") && AttachConsole((DWORD) -1))
  {
    std::FILE *tmp = (std::FILE *) 0;
    (void) freopen_s(&tmp, "CONOUT$", "w", stdout);
    (void) freopen_s(&tmp, "CONOUT$", "w", stderr);
    (void) freopen_s(&tmp, "CONIN$", "r", stdin);
  }
#  endif
#endif
}

void SDLDisplay::setEnableDownsample(bool isEnabled)
{
  if (isEnabled == isDownsampled)
    return;
  if (isEnabled)
  {
    imageBuf.resize(size_t(imageWidth << 1) * size_t(imageHeight << 1), 0U);
    imageWidth = imageWidth << 1;
    imageHeight = imageHeight << 1;
    usingImageBuf = true;
  }
  else
  {
    imageWidth = imageWidth >> 1;
    imageHeight = imageHeight >> 1;
#ifdef HAVE_SDL2
    usingImageBuf = (int(sdlScreen->pitch >> 2) != imageWidth);
    if (usingImageBuf)
      imageBuf.resize(size_t(imageWidth) * size_t(imageHeight));
#else
    usingImageBuf = false;
#endif
  }
  isDownsampled = isEnabled;
  textXScale = float(imageWidth) / float(textWidth);
  textYScale = float(imageHeight) / float(textHeight);
  fontWidth = float(imageWidth * 26) / float(textWidth * 20);
  fontHeight = float(imageHeight * 38) / float(textHeight * 30);
  fontUScale = float(textWidth * 20) / float(imageWidth);
  fontVScale = float(textHeight * 30) / float(imageHeight);
}

void SDLDisplay::blitSurface()
{
  if (usingImageBuf)
  {
    const std::uint32_t *s = &(imageBuf.front());
    std::uint32_t *p;
    int     w = imageWidth;
    int     pitch;
#ifdef HAVE_SDL2
    SDL_LockSurface(sdlScreen);
    p = reinterpret_cast< std::uint32_t * >(sdlScreen->pixels);
    pitch = int(sdlScreen->pitch >> 2);
#else
    p = &(screenBuf.front());
    pitch = w >> int(isDownsampled);
#endif
    if (isDownsampled)
    {
      downsample2xFilter(p, s, w, imageHeight, pitch);
    }
    else
    {
      for (int y = 0; y < imageHeight; y++, p = p + pitch, s = s + w)
        std::memcpy(p, s, size_t(w) * sizeof(std::uint32_t));
    }
#ifdef HAVE_SDL2
    SDL_UnlockSurface(sdlScreen);
#endif
  }
#ifdef HAVE_SDL2
  SDL_BlitSurface(sdlScreen, (SDL_Rect *) 0,
                  SDL_GetWindowSurface(sdlWindow), (SDL_Rect *) 0);
  SDL_UpdateWindowSurface(sdlWindow);
#endif
}

void SDLDisplay::clearSurface(std::uint32_t c)
{
  size_t  n = size_t(imageWidth) * size_t(imageHeight);
#ifdef HAVE_SDL2
  if (!usingImageBuf)
    SDL_LockSurface(sdlScreen);
#endif
  std::uint32_t *p;
  if (usingImageBuf)
    p = &(imageBuf.front());
  else
#ifdef HAVE_SDL2
    p = reinterpret_cast< std::uint32_t * >(sdlScreen->pixels);
#else
    p = &(screenBuf.front());
#endif
  if (c == ((c & 0xFFU) * 0x01010101U))
  {
    std::memset(p, int(c & 0xFFU), n * sizeof(std::uint32_t));
  }
  else
  {
    for (size_t i = 0; i < n; i++)
      p[i] = c;
  }
#ifdef HAVE_SDL2
  if (!usingImageBuf)
    SDL_UnlockSurface(sdlScreen);
#endif
}

void SDLDisplay::pollEvents(std::vector< SDLEvent >& buf, int waitTime,
                            bool enableKeyModifiers, bool pollMouseEvents)
{
  buf.clear();
#ifdef HAVE_SDL2
  static const char *modifiedKeysShift =
      "@ABCDEFGHIJKLMNOPQRSTUVWXYZ{|}^_~!\"#$%&\"()*+<_>?)!@#$%^&*(::>+>?";
  if (waitTime > 0)
    SDL_Delay((Uint32) waitTime);
  SDL_Event event;
  while (SDL_PollEvent(&event))
  {
    int     t = SDLEventWindow;
    int     d1 = 0;
    int     d2 = 0;
    int     d3 = 0;
    int     d4 = 0;
    SDL_Keycode sym;
    switch (event.type)
    {
      case SDL_QUIT:
        break;
      case SDL_WINDOWEVENT:
        d1 = 1;
        break;
      case SDL_KEYDOWN:
      case SDL_KEYUP:
        if (event.type == SDL_KEYUP)
          t = SDLEventKeyUp;
        else
          t = (!event.key.repeat ? SDLEventKeyDown : SDLEventKeyRepeat);
        sym = event.key.keysym.sym;
        if (sym >= 0x20 && sym < 0x7F)
        {
          // printable characters
          if (enableKeyModifiers)
          {
            if ((sym >= 0x41 && sym <= 0x5A) || (sym >= 0x61 && sym <= 0x7A))
            {
              if (event.key.keysym.mod & KMOD_CAPS)
                sym = sym & 0xDF;
              else
                sym = sym | 0x20;
              if (event.key.keysym.mod & KMOD_SHIFT)
                sym = sym ^ 0x20;
            }
            else if (sym > 0x20 && sym < 0x61)
            {
              if (event.key.keysym.mod & KMOD_SHIFT)
                sym = (SDL_Keycode) modifiedKeysShift[sym & 0x3F];
            }
          }
          d1 = int(sym);
        }
        else
        {
          switch (sym)
          {
            case SDLK_BACKSPACE:
              d1 = SDLKeySymBackspace;
              break;
            case SDLK_TAB:
              d1 = SDLKeySymTab;
              break;
            case SDLK_RETURN:
              d1 = SDLKeySymReturn;
              break;
            case SDLK_ESCAPE:
              d1 = SDLKeySymEscape;
              break;
            case SDLK_SPACE:
              d1 = 0x20;
              break;
            case SDLK_DELETE:
              d1 = SDLKeySymDelete;
              break;
            case SDLK_CAPSLOCK:
              d1 = SDLKeySymCapsLock;
              break;
            case SDLK_F1:
            case SDLK_F2:
            case SDLK_F3:
            case SDLK_F4:
            case SDLK_F5:
            case SDLK_F6:
            case SDLK_F7:
            case SDLK_F8:
            case SDLK_F9:
            case SDLK_F10:
            case SDLK_F11:
            case SDLK_F12:
              d1 = int(sym - SDLK_F1) + SDLKeySymF1;
              break;
            case SDLK_PRINTSCREEN:
              d1 = SDLKeySymPrintScr;
              break;
            case SDLK_SCROLLLOCK:
              d1 = SDLKeySymScrollLock;
              break;
            case SDLK_PAUSE:
              d1 = SDLKeySymPause;
              break;
            case SDLK_INSERT:
              d1 = SDLKeySymInsert;
              break;
            case SDLK_HOME:
              d1 = SDLKeySymHome;
              break;
            case SDLK_PAGEUP:
              d1 = SDLKeySymPageUp;
              break;
            case SDLK_END:
              d1 = SDLKeySymEnd;
              break;
            case SDLK_PAGEDOWN:
              d1 = SDLKeySymPageDown;
              break;
            case SDLK_RIGHT:
              d1 = SDLKeySymRight;
              break;
            case SDLK_LEFT:
              d1 = SDLKeySymLeft;
              break;
            case SDLK_DOWN:
              d1 = SDLKeySymDown;
              break;
            case SDLK_UP:
              d1 = SDLKeySymUp;
              break;
            case SDLK_NUMLOCKCLEAR:
              d1 = SDLKeySymNumLock;
              break;
            case SDLK_KP_DIVIDE:
              d1 = SDLKeySymKPDivide;
              break;
            case SDLK_KP_MULTIPLY:
              d1 = SDLKeySymKPMultiply;
              break;
            case SDLK_KP_MINUS:
              d1 = SDLKeySymKPMinus;
              break;
            case SDLK_KP_PLUS:
              d1 = SDLKeySymKPPlus;
              break;
            case SDLK_KP_ENTER:
              d1 = SDLKeySymKPEnter;
              break;
            case SDLK_KP_1:
            case SDLK_KP_2:
            case SDLK_KP_3:
            case SDLK_KP_4:
            case SDLK_KP_5:
            case SDLK_KP_6:
            case SDLK_KP_7:
            case SDLK_KP_8:
            case SDLK_KP_9:
              d1 = int(sym - SDLK_KP_1) + SDLKeySymKP1;
              break;
            case SDLK_KP_0:
              d1 = SDLKeySymKPIns;
              break;
            case SDLK_KP_PERIOD:
              d1 = SDLKeySymKPDel;
              break;
            default:
              if (sym >= 0x00A1 && sym <= 0x3FFF)
                d1 = int(sym);
              else if (sym >= 0x40000000 && sym <= 0x40003FFF)
                d1 = int(sym) - 0x3FFFC000;
              else if (sym == 0x00A0)
                d1 = 0x20;
              else
                continue;
              break;
          }
        }
        break;
      case SDL_MOUSEMOTION:
        if (!pollMouseEvents)
          continue;
        t = SDLEventMouseMotion;
        d1 = int(event.motion.x) << int(isDownsampled);
        d2 = int(event.motion.y) << int(isDownsampled);
        d3 = int(event.motion.xrel) << int(isDownsampled);
        d4 = int(event.motion.yrel) << int(isDownsampled);
        break;
      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP:
        if (!pollMouseEvents)
          continue;
        if (event.type == SDL_MOUSEBUTTONUP)
          t = SDLEventMButtonUp;
        else
          t = SDLEventMButtonDown;
        d1 = int(event.button.x) << int(isDownsampled);
        d2 = int(event.button.y) << int(isDownsampled);
        d3 = int(event.button.button);
        d4 = int(event.button.clicks);
        break;
      case SDL_MOUSEWHEEL:
        if (!pollMouseEvents)
          continue;
        t = SDLEventMouseWheel;
#if (SDL_MAJOR_VERSION > 2) || (SDL_MINOR_VERSION > 0) || (SDL_PATCHLEVEL >= 18)
        d1 = roundFloat(event.wheel.preciseX * 32.0f);
        d2 = roundFloat(event.wheel.preciseY * 32.0f);
#else
        d1 = int(event.wheel.x) << 5;
        d2 = int(event.wheel.y) << 5;
#endif
        break;
      default:
        continue;
    }
    buf.push_back(SDLEvent(t, d1, d2, d3, d4));
  }
#else
  (void) waitTime;
  (void) enableKeyModifiers;
  (void) pollMouseEvents;
  buf.push_back(SDLEvent(SDLEventWindow, 0, 0, 0, 0));
#endif
}

void SDLDisplay::drawText(int y0Dst, int y0Src, int lineCnt,
                          float bgAlpha, float fgAlpha)
{
  int     h = int(textBuf.size() / size_t(textWidth));
  if (y0Src < 0)
    y0Src = (h + 1 - lineCnt) + y0Src;
  if (y0Src < 0)
  {
    lineCnt = lineCnt + y0Src;
    y0Src = 0;
  }
  if (y0Dst < 0)
  {
    y0Src = y0Src - y0Dst;
    lineCnt = lineCnt + y0Dst;
    y0Dst = 0;
  }
  if (y0Src >= h || y0Dst >= textHeight || lineCnt < 1)
    return;
  if ((y0Src + lineCnt) > h)
    lineCnt = h - y0Src;
  if ((y0Dst + lineCnt) > textHeight)
    lineCnt = textHeight - y0Dst;

  std::thread *threads[32];
  int     threadCnt = int(std::thread::hardware_concurrency());
  if (threadCnt > (lineCnt >> (fontVScale < 1.0f ? 1 : 2)))
    threadCnt = lineCnt >> (fontVScale < 1.0f ? 1 : 2);
  threadCnt = (threadCnt > 1 ? (threadCnt < 32 ? threadCnt : 32) : 1);
#ifdef HAVE_SDL2
  if (!usingImageBuf)
    SDL_LockSurface(sdlScreen);
#endif
  // i = 0: draw background
  // i = 1: draw text
  for (int i = 0; i < 2; i++)
  {
    float   alpha = (i == 0 ? bgAlpha : fgAlpha);
    if (!(alpha > 0.0f))
      continue;
    alpha = (alpha < 1.0f ? alpha : 1.0f);
    bool    isBackground = (i == 0);
    if (threadCnt < 2)
    {
      drawTextThreadFunc(this, y0Src, y0Dst, lineCnt, isBackground, alpha);
      continue;
    }
    for (int j = 0; j < 2; j++)
    {
      int     y0 = 0;
      for (int k = 0; k < threadCnt; k++)
      {
        int     y2 = (lineCnt * (k + 1)) / threadCnt;
        int     y1 = (y0 + y2) >> 1;
        int     yOffs = (j == 0 ? y0 : y1);
        int     n = (j == 0 ? (y1 - y0) : (y2 - y1));
        try
        {
          threads[k] = new std::thread(drawTextThreadFunc,
                                       this, y0Src + yOffs, y0Dst + yOffs, n,
                                       isBackground, alpha);
        }
        catch (...)
        {
          threads[k] = (std::thread *) 0;
          drawTextThreadFunc(this, y0Src + yOffs, y0Dst + yOffs, n,
                             isBackground, alpha);
        }
        y0 = y2;
      }
      for (int k = 0; k < threadCnt; k++)
      {
        if (threads[k])
        {
          threads[k]->join();
          delete threads[k];
          threads[k] = (std::thread *) 0;
        }
      }
    }
  }
#ifdef HAVE_SDL2
  if (!usingImageBuf)
    SDL_UnlockSurface(sdlScreen);
#endif
}

void SDLDisplay::clearTextBuffer()
{
  printXPos = 0;
  printYPos = 0;
  textColor = defaultTextColor;
  printBufCnt = 0;
  textBuf.clear();
}

void SDLDisplay::printString(const char *s)
{
  if (!s)
    return;
  for ( ; *s; s++)
  {
    int     bufSize = int(sizeof(printBuf) / sizeof(unsigned char));
    if (BRANCH_LIKELY(printBufCnt < bufSize))
    {
      printBuf[printBufCnt] = (unsigned char) *s;
      printBufCnt++;
    }
    else
    {
      printBuf[bufSize - 1] = 0x6D;     // add 'm' to terminate escape sequence
    }
    std::uint32_t c = printBuf[0];
    if (c >= 0x20U && c != 0x7FU)
    {
      // printable character
      do
      {
        if (c >= 0x80U)
        {
          c = decodeUTF8Character();
          if (!c)
            break;
          c = (c < 0x4000U ? c : 0x003FU);
        }
        else
        {
          printBufCnt = 0;
        }
        printCharacter(c);
      }
      while (BRANCH_UNLIKELY(printBufCnt > 0));
      continue;
    }
    // control characters
    if (c == 0x1BU)                     // Escape
    {
      if (printBufCnt < 2)
        continue;
      if (printBuf[1] != 0x5B)          // not '[', ignore sequence
      {
        printBufCnt = 0;
        continue;
      }
      c = printBuf[printBufCnt - 1];
      if (printBufCnt < 3 || c < 0x40U) // sequence not complete yet
        continue;
      printBufCnt--;
      int     n1 = 0;
      int     n2 = -1;
      int     n3 = -1;
      int     i = 2;
      for ( ; i < printBufCnt; i++)
      {
        int&    n = (n2 < 0 ? n1 : (n3 < 0 ? n2 : n3));
        if (printBuf[i] >= 0x30 && printBuf[i] <= 0x39)
        {
          n = (n * 10) + (int(printBuf[i]) - 0x30);
        }
        else if (printBuf[i] == 0x3A || printBuf[i] == 0x3B)
        {
          if (n2 < 0)
            n2 = 0;
          else if (n3 < 0)
            n3 = 0;
        }
        else
        {
          break;
        }
      }
      int     n = i;
      for ( ; i < printBufCnt; i++)
        printBuf[i - n] = printBuf[i];
      printBufCnt -= n;
      if (c != 0x6DU)                   // not 'm'
      {
        if (c == 0x47U)                 // 'G': set X position
        {
          n1--;
          printXPos = (n1 > 0 ? (n1 < textWidth ? n1 : textWidth) : 0);
        }
        else if (c == 0x4AU && n1 >= 2) // 'J': clear screen
        {
          printXPos = 0;
          printYPos = 0;
          textBuf.clear();
        }
        continue;
      }
      switch (n1)                       // 'm': select graphic rendition
      {
        case 0:                         // reset attributes
          textColor = defaultTextColor;
          break;
        case 1:                         // increased intensity
          textColor = textColor | 0x08000000U;
          break;
        case 4:                         // underline
          textColor = textColor | 0x4000U;
          break;
        case 24:                        // underline off
          textColor = textColor & ~0x4000U;
          break;
        case 38:                        // set foreground color
          if (n2 == 5 && n3 >= 0)
          {
            textColor = (textColor & 0x00FFFFFFU)
                        | ((std::uint32_t) (n3 & 0xFF) << 24);
          }
          break;
        case 39:                        // default foreground color
          textColor = (textColor & 0x00FFFFFFU)
                      | (defaultTextColor & 0xFF000000U);
          break;
        case 48:                        // set background color
          if (n2 == 5 && n3 >= 0)
          {
            textColor = (textColor & 0xFF007FFFU)
                        | ((std::uint32_t) (n3 & 0xFF) << 16) | 0x8000U;
          }
          break;
        case 49:                        // default background color
          textColor = (textColor & 0xFF007FFFU)
                      | (defaultTextColor & 0x00FF8000U);
          break;
        default:
          if (n1 >= 30 && n1 <= 37)     // set foreground color
          {
            textColor = (textColor
                         & ((textColor & 0xF8000000U) != 0x08000000U ?
                            0x00FFFFFFU : 0x08FFFFFFU))
                        | ((std::uint32_t) (n1 - 30) << 24);
          }
          else if (n1 >= 40 && n1 <= 47)        // set background color
          {
            textColor = (textColor & 0xFF007FFFU)
                        | ((std::uint32_t) (n1 - 40) << 16) | 0x8000U;
          }
          break;
      }
    }
    else
    {
      switch (c)
      {
        case 0x08:                      // backspace
          if (printXPos > 0)
            printXPos--;
          break;
        case 0x09:                      // tab
          do
          {
            printCharacter(0x20U);
          }
          while (printXPos & 7);
          break;
        case 0x0A:                      // end of line
          printXPos = 0;
          printYPos++;
          break;
        case 0x0D:                      // carriage return
          printXPos = 0;
          break;
      }
      printBufCnt = 0;
    }
  }
}

int SDLDisplay::browseList(
    const std::vector< std::string >& v, const char *titleString,
    int itemSelected, std::uint64_t colors)
{
  if (v.size() < 1)
    return -1;
  if (itemSelected < 0)
    itemSelected = 0;
  else if (size_t(itemSelected) >= v.size())
    itemSelected = int(v.size()) - 1;
#ifdef HAVE_SDL2
  // TODO: implement this
  (void) titleString;
  (void) colors;
#else
  (void) titleString;
  (void) colors;
#endif
  return itemSelected;
}

static void convertUTF8ToUInt32(
    std::vector< std::uint32_t >& v, const char *s,
    size_t maxLen = 1024, size_t *insertPosPtr = (size_t *) 0)
{
  if (!s)
    return;
  for (size_t i = 0; s[i] != '\0'; i++)
  {
    if (v.size() >= maxLen)
      break;
    std::uint32_t c = (unsigned char) s[i];
    if (c >= 0x80U)
    {
      unsigned char c1 = (unsigned char) s[i + 1];
      unsigned char c2 = 0;
      if (c1)
        c2 = (unsigned char) s[i + 2];
      if ((c & 0xE0U) == 0xC0U && (c1 & 0xC0) == 0x80)
      {
        c = ((c & 0x1FU) << 6) | (c1 & 0x3FU);
        i++;
      }
      else if ((c & 0xF0U) == 0xE0U &&
               (c1 & 0xC0) == 0x80 && (c2 & 0xC0) == 0x80)
      {
        c = ((c & 0x0FU) << 6) | (c1 & 0x3FU);
        c = (c << 6) | (c2 & 0x3FU);
        i = i + 2;
      }
    }
    if (c == 0x09U)
      c = 0x20U;
    if (c >= 0x0020U && c < 0x4000U)
    {
      if (!insertPosPtr)
      {
        v.push_back(c);
      }
      else
      {
        v.insert(v.begin() + (*insertPosPtr), c);
        (*insertPosPtr)++;
      }
    }
  }
}

static void convertUInt32ToUTF8(
    std::string& s, const std::vector< std::uint32_t >& v,
    size_t startPos = 0, size_t endPos = 0x7FFFFFFF)
{
  s.clear();
  endPos = (endPos < v.size() ? endPos : v.size());
  for (size_t i = startPos; i < endPos; i++)
  {
    std::uint32_t c = v[i] & 0x3FFFU;
    if (c >= 0x0020U && c < 0x0080U)
    {
      s += char(c);
    }
    else if (c >= 0x0080U && c < 0x0800U)
    {
      s += char(((c >> 6) & 0x1FU) | 0xC0U);
      s += char((c & 0x3FU) | 0x80U);
    }
    else if (c >= 0x0800U)
    {
      s += char(((c >> 12) & 0x0FU) | 0xE0U);
      s += char(((c >> 6) & 0x3FU) | 0x80U);
      s += char((c & 0x3FU) | 0x80U);
    }
  }
}

#ifdef __GNUC__
__attribute__ ((__format__ (__printf__, 2, 3)))
#endif
void SDLDisplay::consolePrint(const char *fmt, ...)
{
#ifdef HAVE_SDL2
  if (BRANCH_UNLIKELY(textBuf.size() >= 1048576))
  {
    int     n = 524288 / textWidth;
    printYPos = (printYPos > n ? (printYPos - n) : 0);
    textBuf.erase(textBuf.begin(), textBuf.begin() + size_t(n * textWidth));
  }
  if (!fmt || !std::strchr(fmt, '%'))
  {
    printString(fmt);
    return;
  }
  char    tmpBuf[2048];
  std::va_list  ap;
  va_start(ap, fmt);
  std::vsnprintf(tmpBuf, 2048, fmt, ap);
  va_end(ap);
  tmpBuf[2047] = '\0';
  printString(tmpBuf);
#else
  std::va_list  ap;
  va_start(ap, fmt);
  std::vfprintf(stdout, fmt, ap);
  va_end(ap);
#endif
}

bool SDLDisplay::consoleInput(
    std::string& s, std::map< size_t, std::string >& cmdHistory1,
    std::map< std::string, size_t >& cmdHistory2, size_t historySize)
{
  s.clear();
#ifdef HAVE_SDL2
  if (textWidth < 4 || textHeight < 2)
    return false;
  std::vector< SDLEvent > eventBuf;
  std::vector< std::uint32_t >  tmpBuf;
  int     displayLine = -1;
  int     waitTime = 10;
  unsigned char redrawFlag = 1;         // 1: redraw, 2: do not reset Y position
  bool    eolFlag = false;
  bool    quitFlag = false;
  size_t  insertPos = 0;
  std::map< size_t, std::string >::const_iterator historyPos =
      cmdHistory1.end();
  while (!quitFlag)
  {
    if (redrawFlag)
    {
      clearSurface(ansiColor256Table[(defaultTextColor >> 16) & 0xFFU]);
      printXPos = 0;
      printYPos = int(textBuf.size() / size_t(textWidth));
      printYPos -= int(printYPos > 0);
      for (int i = 0; i < textWidth; i++)
      {
        std::uint32_t c = std::uint32_t(' ');
        if (!i)
          c = std::uint32_t('>');
        else if (i >= 2 && size_t(i - 2) < tmpBuf.size())
          c = tmpBuf[i - 2];
        printCharacter(c);
      }
      if (int(insertPos + 2) < textWidth && !eolFlag)
      {
        // reverse colors at cursor position
        std::uint32_t&  c =
            textBuf[textBuf.size() - (size_t(textWidth) - (insertPos + 2))];
        c = (c & 0xFFFFU) | ((c >> 8) & 0x00FF0000U) | ((c << 8) & 0xFF000000U)
            | 0x8000U;
      }
      if (redrawFlag == 1)
        displayLine = -1;
      if (eolFlag)
      {
        printCharacter(0x0020U);
        printXPos = 0;
      }
      drawText(0, displayLine, textHeight, 1.0f, 1.0f);
      blitSurface();
      redrawFlag = 0;
    }
    if (eolFlag)
      break;
    pollEvents(eventBuf, waitTime, true, true);
    waitTime += int(waitTime < 10);
    int     textBufLines = int(textBuf.size() / size_t(textWidth));
    for (size_t i = 0; i < eventBuf.size(); i++)
    {
      int     t = eventBuf[i].type();
      int     d1 = eventBuf[i].data1();
      if (t == SDLEventWindow)
      {
        if (d1 == 0)
          quitFlag = true;
        else
          redrawFlag = 2;
        continue;
      }
      if (t == SDLEventMButtonDown)
      {
        int     d3 = eventBuf[i].data3();
        int     d4 = eventBuf[i].data4();
        // double click = copy word
        // triple click = copy line
        // middle click = paste
        // right click = copy and paste word
        if ((d3 == 1 && d4 >= 2) || d3 == 3)
        {
          int     d2 = eventBuf[i].data2();
          int     x = (d1 * textWidth) / imageWidth;
          int     y = (d2 * textHeight) / imageHeight;
          x = (x > 0 ? (x < (textWidth - 1) ? x : (textWidth - 1)) : 0);
          y = (y > 0 ? (y < (textHeight - 1) ? y : (textHeight - 1)) : 0);
          if (textBufLines > textHeight)
            y = y + ((textBufLines + 1 - textHeight) + displayLine);
          std::string clipboardBuf;
          if (y >= 0 && y < textBufLines)
          {
            size_t  startPos, endPos;
            if (d4 < 3)         // copy word
            {
              startPos = size_t(y) * size_t(textWidth) + size_t(x);
              endPos = startPos;
              int     x0 = x;
              int     x1 = x;
              while (x0 >= 0 && (textBuf[startPos] & 0x3FFFU) > 0x20U)
                x0--, startPos--;
              startPos++;
              while (x1 < textWidth && (textBuf[endPos] & 0x3FFFU) > 0x20U)
                x1++, endPos++;
            }
            else                // copy line
            {
              startPos = size_t(y) * size_t(textWidth);
              endPos = startPos + size_t(textWidth);
              int     x0 = 0;
              int     x1 = textWidth;
              while (x0 < textWidth && (textBuf[startPos] & 0x3FFFU) <= 0x20U)
                x0++, startPos++;
              while (x1 > x0 && (textBuf[endPos - 1] & 0x3FFFU) <= 0x20U)
                x1--, endPos--;
            }
            convertUInt32ToUTF8(clipboardBuf, textBuf, startPos, endPos);
          }
          if (clipboardBuf.empty() ||
              SDL_SetClipboardText(clipboardBuf.c_str()) != 0)
          {
            d3 = 0;
          }
        }
        if ((d3 == 2 || d3 == 3) && SDL_HasClipboardText())
        {
          char    *clipboardBuf = SDL_GetClipboardText();
          if (clipboardBuf)
          {
            try
            {
              size_t  prvSize = tmpBuf.size();
              convertUTF8ToUInt32(tmpBuf, clipboardBuf, size_t(textWidth - 3),
                                  &insertPos);
              redrawFlag = (unsigned char) (tmpBuf.size() != prvSize);
            }
            catch (...)
            {
              SDL_free(clipboardBuf);
              throw;
            }
            SDL_free(clipboardBuf);
          }
        }
        continue;
      }
      if (t == SDLEventMouseWheel)
      {
        int     d2 = eventBuf[i].data2();
        int     minOffs = (textHeight - 1) - textBufLines;
        minOffs = (minOffs < -1 ? minOffs : -1);
        d2 = displayLine - (d2 / 10);
        d2 = (d2 > minOffs ? (d2 < -1 ? d2 : -1) : minOffs);
        if (d2 != displayLine)
        {
          displayLine = d2;
          waitTime = 1;
          redrawFlag = 2;
        }
        continue;
      }
      if (t != SDLEventKeyDown && t != SDLEventKeyRepeat)
        continue;
      if (d1 >= 0x0020 && d1 <= 0x3FFF && d1 != SDLKeySymDelete)
      {
        if (int(tmpBuf.size() + 3) < textWidth)
        {
          tmpBuf.insert(tmpBuf.begin() + insertPos, std::uint32_t(d1));
          insertPos++;
          redrawFlag = 1;
        }
        continue;
      }
      switch (d1)
      {
        case SDLKeySymBackspace:
          if (insertPos > 0)
          {
            insertPos--;
            tmpBuf.erase(tmpBuf.begin() + insertPos);
            redrawFlag = 1;
          }
          break;
        case SDLKeySymReturn:
          redrawFlag = 1;
          eolFlag = true;
          if (tmpBuf.size() > 0)
            break;
          historyPos = cmdHistory1.end();
        case SDLKeySymUp:
        case SDLKeySymDown:
          if (d1 != SDLKeySymDown)
          {
            if (historyPos == cmdHistory1.begin())
              continue;
            historyPos--;
          }
          else
          {
            if (historyPos == cmdHistory1.end())
              continue;
            historyPos++;
          }
          tmpBuf.clear();
          if (historyPos != cmdHistory1.end())
          {
            convertUTF8ToUInt32(tmpBuf, historyPos->second.c_str(),
                                size_t(textWidth - 3));
          }
          insertPos = tmpBuf.size();
          redrawFlag = 1;
          break;
        case SDLKeySymDelete:
          if (insertPos < tmpBuf.size())
          {
            tmpBuf.erase(tmpBuf.begin() + insertPos);
            redrawFlag = 1;
          }
          break;
        case SDLKeySymLeft:
          if (insertPos > 0)
          {
            insertPos--;
            redrawFlag = 1;
          }
          break;
        case SDLKeySymRight:
          if (insertPos < tmpBuf.size())
          {
            insertPos++;
            redrawFlag = 1;
          }
          break;
        case SDLKeySymHome:
          if (insertPos)
          {
            insertPos = 0;
            redrawFlag = 1;
          }
          break;
        case SDLKeySymEnd:
          if (insertPos != tmpBuf.size())
          {
            insertPos = tmpBuf.size();
            redrawFlag = 1;
          }
          break;
        case SDLKeySymPageUp:
        case SDLKeySymPageDown:
          {
            int     minOffs = (textHeight - 1) - textBufLines;
            minOffs = (minOffs < -1 ? minOffs : -1);
            int     tmp = (textHeight > 2 ? (textHeight - 2) : 1);
            tmp = (d1 == SDLKeySymPageUp ?
                   (displayLine - tmp) : (displayLine + tmp));
            tmp = (tmp > minOffs ? (tmp < -1 ? tmp : -1) : minOffs);
            if (tmp != displayLine)
            {
              displayLine = tmp;
              redrawFlag = 2;
            }
          }
          break;
        default:
          break;
      }
      if (t == SDLEventKeyRepeat && redrawFlag)
        waitTime = 1;
    }
  }
  if (quitFlag)
    s = "\004";
  else
    convertUInt32ToUTF8(s, tmpBuf);
#else
  std::printf("> ");
  std::fflush(stdout);
  while (true)
  {
    int     c = std::fgetc(stdin);
    if (c == EOF || c == 0x04)
    {
      std::fputc('\n', stdout);
      if (c == 0x04 || s.empty())
        s = "\004";
      break;
    }
    c = c & 0xFF;
    if (c == '\n')
      break;
    if (c)
      s += char(c);
  }
#endif
  if (s == "\004")
    return false;
  size_t  n;
  for (n = 0; n < s.length() && (unsigned char) s[n] <= 0x20; n++)
    ;
  if (n > 0)
    s.erase(0, n);
  for (n = s.length(); n > 0 && (unsigned char) s[n - 1] <= 0x20; n--)
    ;
  if (n < s.length())
    s.erase(n, s.length() - n);
#ifndef HAVE_SDL2
  if (s.empty())
  {
    if (cmdHistory1.begin() != cmdHistory1.end())
    {
      std::map< size_t, std::string >::const_iterator i = cmdHistory1.end();
      i--;
      s = i->second;
      std::printf("> %s\n", s.c_str());
      std::fflush(stdout);
    }
  }
  else
#endif
  if (historySize > 0 && !s.empty())
  {
    if (cmdHistory1.begin() != cmdHistory1.end())
    {
      std::map< std::string, size_t >::iterator i = cmdHistory2.find(s);
      if (i != cmdHistory2.end())
      {
        cmdHistory1.erase(i->second);
        cmdHistory2.erase(i);
      }
      else if (cmdHistory1.size() >= historySize)
      {
        cmdHistory2.erase(cmdHistory1.begin()->second);
        cmdHistory1.erase(cmdHistory1.begin());
      }
    }
    n = 0;
    if (cmdHistory1.begin() != cmdHistory1.end())
    {
      std::map< size_t, std::string >::const_iterator i = cmdHistory1.end();
      i--;
      n = i->first + 1;
    }
    cmdHistory1.insert(std::pair< size_t, std::string >(n, s));
    cmdHistory2.insert(std::pair< std::string, size_t >(s, n));
  }
  return true;
}

