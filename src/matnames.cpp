
#ifndef MATNAMES_CPP_INCLUDED
#define MATNAMES_CPP_INCLUDED

static const char *objectTypeStrings[7] =
{
  "", "MaterialFile", "Blender", "Layer", "Material", "TextureSet", "UVStream"
};

static const char *materialFlagNames[32] =
{
  "", "", "Is effect", "Is decal", "Two sided",
  "Is vegetation", "", "Emissive", "", "",
  "", "", "Alpha blending", "", "Is water",
  "", "Alpha testing", "Is terrain", "", "",
  "", "", "", "", "",
  "", "", "", "", "",
  "", ""
};

static const char *opacityModeNames[4] =
{
  "Lerp", "Additive", "Subtractive", "Multiplicative"
};

static const char *textureAddressModeNames[4] =
{
  "Wrap", "Clamp", "Mirror", "Border"
};

static const char *colorModeNames[2] =
{
  "Multiply", "Lerp"
};

#endif

