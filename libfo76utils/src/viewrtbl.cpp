
#ifndef VIEWRTBL_CPP_INCLUDED
#define VIEWRTBL_CPP_INCLUDED

#ifdef HAVE_SDL2
static const char *viewRotationMessages[20] =
{
  "View  0: isometric from the NW\n",
  "View  1: isometric from the SW\n",
  "View  2: isometric from the SE\n",
  "View  3: isometric from the NE\n",
  "View  4: top (up = N)\n",
  "View  5: S\n",
  "View  6: E\n",
  "View  7: N\n",
  "View  8: W\n",
  "View  9: bottom (up = S)\n",
  "View 10: isometric from the N\n",
  "View 11: isometric from the W\n",
  "View 12: isometric from the S\n",
  "View 13: isometric from the E\n",
  "View 14: top (up = NE)\n",
  "View 15: SW\n",
  "View 16: SE\n",
  "View 17: NE\n",
  "View 18: NW\n",
  "View 19: bottom (up = SW)\n"
};

static const float  viewRotations[60] =
{
  54.73561f,  180.0f,     45.0f,        // isometric from NW
  54.73561f,  180.0f,     135.0f,       // isometric from SW
  54.73561f,  180.0f,     -135.0f,      // isometric from SE
  54.73561f,  180.0f,     -45.0f,       // isometric from NE
  180.0f,     0.0f,       0.0f,         // top (up = N)
  -90.0f,     0.0f,       0.0f,         // S
  -90.0f,     0.0f,       90.0f,        // E
  -90.0f,     0.0f,       180.0f,       // N
  -90.0f,     0.0f,       -90.0f,       // W
    0.0f,     0.0f,       0.0f,         // bottom (up = S)
  54.73561f,  180.0f,     0.0f,         // isometric from N
  54.73561f,  180.0f,     90.0f,        // isometric from W
  54.73561f,  180.0f,     180.0f,       // isometric from S
  54.73561f,  180.0f,     -90.0f,       // isometric from E
  180.0f,     0.0f,       -45.0f,       // top (up = NE)
  -90.0f,     0.0f,       -45.0f,       // SW
  -90.0f,     0.0f,       45.0f,        // SE
  -90.0f,     0.0f,       135.0f,       // NE
  -90.0f,     0.0f,       -135.0f,      // NW
    0.0f,     0.0f,       -45.0f        // bottom (up = SW)
};

static const unsigned char keypadToViewRotation[20] =
{
   9,           // KP0: bottom (up = S)
   1,           // KP1: isometric from SW
  12,           // KP2: isometric from S
   2,           // KP3: isometric from SE
  11,           // KP4: isometric from W
   4,           // KP5: top (up = N)
  13,           // KP6: isometric from E
   0,           // KP7: isometric from NW
  10,           // KP8: isometric from N
   3,           // KP9: isometric from NE
  19,           // Shift + KP0: bottom (up = SW)
  15,           // Shift + KP1: SW
   5,           // Shift + KP2: S
  16,           // Shift + KP3: SE
   8,           // Shift + KP4: W
  14,           // Shift + KP5: top (up = NE)
   6,           // Shift + KP6: E
  18,           // Shift + KP7: NW
   7,           // Shift + KP8: N
  17            // Shift + KP9: NE
};

static inline int getViewRotation(int d1, int d2)
{
  int     n = -1;
  if (d1 >= SDLDisplay::SDLKeySymKP1 && d1 <= SDLDisplay::SDLKeySymKP9)
    n = (d1 - SDLDisplay::SDLKeySymKP1) + 1;
  else if (d1 == SDLDisplay::SDLKeySymKPIns)
    n = 0;
  if (!(n >= 0 && n <= 9))
    return -1;
  if (d2 & (SDLDisplay::SDLKeyModLShift | SDLDisplay::SDLKeyModRShift))
    n = n + 10;
  return int(keypadToViewRotation[n]);
}
#endif

#endif

