
#ifndef SDLVIDEO_HPP_INCLUDED
#define SDLVIDEO_HPP_INCLUDED

#include "common.hpp"

#ifdef HAVE_SDL2
#  include <SDL2/SDL.h>
#endif

class SDLDisplay
{
 protected:
  int     imageWidth;
  int     imageHeight;
#ifdef HAVE_SDL2
  SDL_Window  *sdlWindow;
  SDL_Surface *sdlScreen;
  int     surfaceLockCnt;
#else
  std::vector< std::uint32_t >  screenBuf;
#endif
  bool    isDownsampled;
  bool    usingImageBuf;
  std::vector< std::uint32_t >  imageBuf;
  std::vector< unsigned char >  fontData;
  // bits 0 to 13: Unicode character
  // bit 14: underline
  // bit 15: have background color
  // bits 16 to 23: background color
  // bits 24 to 31: text color
  std::vector< std::uint32_t >  textBuf;
  std::vector< std::uint32_t >  ansiColor256Table;
  int     textWidth;
  int     textHeight;
  float   textXScale;
  float   textYScale;
  float   fontWidth;
  float   fontHeight;
  float   fontUScale;
  float   fontVScale;
  int     printXPos;
  int     printYPos;
  std::uint32_t textColor;
  std::uint32_t defaultTextColor;
  int     printBufCnt;
  unsigned char printBuf[12];
  void drawCharacterBG(std::uint32_t *p,
                       int x, int y, std::uint32_t c, float bgAlpha);
  void drawCharacterFG(std::uint32_t *p,
                       int x, int y, std::uint32_t c, float fgAlpha);
  static void drawTextThreadFunc(SDLDisplay *p, int y0Src, int y0Dst,
                                 int lineCnt, bool isBackground, float alpha);
  // returns 0 if there is not enough data yet, 0xFFFD on error
  std::uint32_t decodeUTF8Character();
  inline void printCharacter(std::uint32_t c);
 public:
  enum
  {
    SDLEventWindow = 0,
    SDLEventKeyRepeat = 1,              // key down repeated
    SDLEventKeyUp = 2,
    SDLEventKeyDown = 3,
    SDLEventMButtonUp = 4,
    SDLEventMButtonDown = 5,
    SDLEventMouseMotion = 6,
    SDLEventMouseWheel = 7,
    SDLKeySymBackspace = 8,
    SDLKeySymTab = 9,
    SDLKeySymReturn = 13,
    SDLKeySymEscape = 27,
    SDLKeySymDelete = 127,
    SDLKeySymCapsLock = 0x4039,
    SDLKeySymF1 = 0x403A,               // F1-F12 = 0x403A-0x4045
    SDLKeySymPrintScr = 0x4046,
    SDLKeySymScrollLock = 0x4047,
    SDLKeySymPause = 0x4048,
    SDLKeySymInsert = 0x4049,
    SDLKeySymHome = 0x404A,
    SDLKeySymPageUp = 0x404B,           // Delete = 127 instead of 0x404C
    SDLKeySymEnd = 0x404D,
    SDLKeySymPageDown = 0x404E,
    SDLKeySymRight = 0x404F,
    SDLKeySymLeft = 0x4050,
    SDLKeySymDown = 0x4051,
    SDLKeySymUp = 0x4052,
    SDLKeySymNumLock = 0x4053,
    SDLKeySymKPDivide = 0x4054,
    SDLKeySymKPMultiply = 0x4055,
    SDLKeySymKPMinus = 0x4056,
    SDLKeySymKPPlus = 0x4057,
    SDLKeySymKPEnter = 0x4058,
    SDLKeySymKP1 = 0x4059,              // KP1-KP9 = 0x4059-0x4061
    SDLKeySymKPIns = 0x4062,
    SDLKeySymKPDel = 0x4063
  };
  struct SDLEvent
  {
    unsigned long long  d;
    SDLEvent(int t, int d1, int d2, int d3, int d4)
    {
      d1 = (d1 > -32768 ? (d1 < 98303 ? d1 : 98303) : -32768) + 32768;
      d2 = (d2 > -32768 ? (d2 < 98303 ? d2 : 98303) : -32768) + 32768;
      d3 = (d3 > -8192 ? (d3 < 8191 ? d3 : 8191) : -8192) + 8192;
      d4 = (d4 > -4096 ? (d4 < 4095 ? d4 : 4095) : -4096) + 4096;
      d = (unsigned long long) (t | (d1 << 3)) | ((unsigned long long) d2 << 20)
          | ((unsigned long long) (d3 | (d4 << 14)) << 37);
    }
    // event type (SDLEventWindow to SDLEventMouseWheel)
    inline int type() const
    {
      return int(d & 7);
    }
    // SDLEventWindow: 0 = quit (window closed), 1 = window state change
    // SDLEventKeyUp/Down/Repeat: key code
    // SDLEventMButtonUp/Down, SDLEventMouseMotion: X coordinate in window
    // SDLEventMouseWheel: the amount scrolled horizontally * 32
    inline int data1() const
    {
      return (int((d >> 3) & 0x0001FFFFU) - 32768);
    }
    // SDLEventMButtonUp/Down, SDLEventMouseMotion: Y coordinate in window
    // SDLEventMouseWheel: the amount scrolled vertically * 32
    inline int data2() const
    {
      return (int((d >> 20) & 0x0001FFFFU) - 32768);
    }
    // SDLEventMButtonUp/Down: mouse button index
    // SDLEventMouseMotion: relative motion in the X direction
    inline int data3() const
    {
      return (int((d >> 37) & 0x3FFFU) - 8192);
    }
    // SDLEventMButtonUp/Down: number of clicks
    // SDLEventMouseMotion: relative motion in the Y direction
    inline int data4() const
    {
      return (int((d >> 51) & 0x1FFFU) - 4096);
    }
  };
  // flags & 1: enable OpenGL
  // flags & 2: enable double buffering in OpenGL mode
  // flags & 4: enable downsampling if w or h > display width or height
  // textColumns <= 0: calculate from textRows and window dimensions
  SDLDisplay(int w, int h, const char *windowTitle, unsigned int flags = 0U,
             int textRows = 48, int textColumns = 0);
  virtual ~SDLDisplay();
  inline int getWidth() const
  {
    return imageWidth;
  }
  inline int getHeight() const
  {
    return imageHeight;
  }
  inline int getTextColumns() const
  {
    return textWidth;
  }
  inline int getTextRows() const
  {
    return textHeight;
  }
  inline bool getIsDownsampled() const
  {
    return isDownsampled;
  }
  inline std::uint32_t *lockDrawSurface()
  {
    if (usingImageBuf)
      return &(imageBuf.front());
#ifdef HAVE_SDL2
    SDL_LockSurface(sdlScreen);
    surfaceLockCnt++;
    return reinterpret_cast< std::uint32_t * >(sdlScreen->pixels);
#else
    return &(screenBuf.front());
#endif
  }
  inline void unlockDrawSurface()
  {
#ifdef HAVE_SDL2
    if (!usingImageBuf)
    {
      if (surfaceLockCnt > 0)
      {
        surfaceLockCnt--;
        SDL_UnlockSurface(sdlScreen);
      }
    }
#endif
  }
  inline std::uint32_t *lockScreenSurface()
  {
#ifdef HAVE_SDL2
    SDL_LockSurface(sdlScreen);
    surfaceLockCnt++;
    return reinterpret_cast< std::uint32_t * >(sdlScreen->pixels);
#else
    return &(screenBuf.front());
#endif
  }
  inline void unlockScreenSurface()
  {
#ifdef HAVE_SDL2
    if (surfaceLockCnt > 0)
    {
      surfaceLockCnt--;
      SDL_UnlockSurface(sdlScreen);
    }
#endif
  }
  inline size_t getPitch() const
  {
#ifdef HAVE_SDL2
    return (size_t(sdlScreen->pitch) >> 2);
#else
    return size_t(imageWidth >> int(isDownsampled));
#endif
  }
  // enable console on Windows
  static void enableConsole();
  void setEnableDownsample(bool isEnabled);
  void blitSurface();
  void clearSurface(std::uint32_t c = 0U);
  void pollEvents(std::vector< SDLEvent >& buf, int waitTime = 10,
                  bool enableKeyModifiers = false,
                  bool pollMouseEvents = false);
  // y0Dst = Y position on screen (0 = top)
  // y0Src = line number in text buffer (0 = first, < 0: until last + 1 + y0Src)
  // lineCnt = maximum number of lines to draw
  // bgAlpha = background alpha (0.0 to 1.0)
  // fgAlpha = text alpha
  void drawText(int y0Dst, int y0Src, int lineCnt,
                float bgAlpha, float fgAlpha);
  void clearTextBuffer();
  void printString(const char *s);
  // set default background and text color (N = 0-255,
  // using the set of colors from the ESC[38;5;Nm sequence),
  // bgColor < 0 makes the background transparent
  inline void setDefaultTextColor(int bgColor, int fgColor)
  {
    defaultTextColor =
        (std::uint32_t(fgColor & 0xFF) << 24)
        | std::uint32_t(bgColor >= 0 ? (((bgColor & 0xFF) << 16) | 0x8000) : 0);
    textColor = defaultTextColor;
  }
  // read character from the text buffer
  inline int readCharacter(int x, int y) const
  {
    if (x < 0 || y < 0)
      return -1;
    size_t  n = size_t(y) * size_t(textWidth) + size_t(x);
    if (n >= textBuf.size())
      return -1;
    return (textBuf[n] ? int(textBuf[n] & 0x3FFFU) : -1);
  }
  inline int getTextBufferLines() const
  {
    return int(textBuf.size() / size_t(textWidth));
  }
  // colors = background + (foreground << 8)
  //          + (selection_bg << 16) + (selection_fg << 24)
  //          + (title_bg << 32) + (title_fg << 40)
  // color = 255 is interpreted as the default color
  // Returns -1 if nothing is selected, -2 if the window is closed
  int browseList(const std::vector< std::string >& v, const char *titleString,
                 int itemSelected, std::uint64_t colors = 0xFFFF0F04FFFFULL);
#ifdef __GNUC__
  __attribute__ ((__format__ (__printf__, 2, 3)))
#endif
  void consolePrint(const char *fmt, ...);
  // returns false if the window is closed
  bool consoleInput(
      std::string& s, std::map< size_t, std::string >& cmdHistory1,
      std::map< std::string, size_t >& cmdHistory2, size_t historySize = 4096);
};

#endif

