
#include "common.hpp"
#include "material.hpp"

static const size_t indentTabSize = 2;

static const char *objectTypeStrings[7] =
{
  "", "MaterialFile", "Blender", "Layer", "Material", "TextureSet", "UVStream"
};

const char * CE2Material::materialFlagNames[32] =
{
  "Tile U",
  "Tile V",
  "Is effect",
  "Is decal",
  "Two sided",
  "Is vegetation",
  "Grayscale to alpha",
  "Emissive",
  "No Z buffer write",
  "Falloff enabled",
  "Effect lighting",
  "Ordered with previous",
  "Alpha blending",
  "Has vertex colors",
  "Is water",
  "Hidden",
  "Alpha testing",
  "Is terrain",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  ""
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

static const char *effectBlendModeNames[8] =
{
  "AlphaBlend", "Additive", "SourceSoftAdditive", "Multiply",
  "DestinationSoftAdditive", "DestinationInvertedSoftAdditive", "TakeSmaller",
  "None"
};

static const char *effectFlagNames[32] =
{
  "Flag00", "Flag01", "Flag02", "Flag03", "Flag04",
  "Flag05", "Flag06", "Flag07", "Flag08", "Flag09",
  "Flag10", "Flag11", "Flag12", "Flag13", "Flag14",
  "Flag15", "Flag16", "Flag17", "Flag18", "Flag19",
  "Flag20", "Flag21", "Flag22", "Flag23", "Flag24",
  "Flag25", "Flag26", "Flag27", "Flag28", "Flag29",
  "Flag30", "Flag31"
};

static
#ifdef __GNUC__
__attribute__ ((__format__ (__printf__, 3, 4)))
#endif
void printToStringBuf(std::string& buf, size_t indentCnt, const char *fmt, ...)
{
  if (indentCnt > 0)
    buf.resize(buf.length() + indentCnt, ' ');
  char    tmpBuf[2048];
  std::va_list  ap;
  va_start(ap, fmt);
  std::vsnprintf(tmpBuf, 2048, fmt, ap);
  va_end(ap);
  tmpBuf[2047] = '\0';
  buf += tmpBuf;
}

void CE2MaterialObject::printObjectInfo(
    std::string& buf, size_t indentCnt) const
{
  printToStringBuf(buf, 0, "\"%s\":\n", name->c_str());
  indentCnt = indentCnt + indentTabSize;
  if (e == 0x0074616DU)                 // "mat\0"
  {
    printToStringBuf(buf, indentCnt,
                     "Hash: 0x%016llX\n", (unsigned long long) h);
  }
  if (parent)
  {
    const char  *parentType = "Unknown";
    int     t = parent->type & 0xFF;
    if (t >= 0 && t <= 6)
      parentType = objectTypeStrings[t];
    printToStringBuf(buf, indentCnt,
                     "Parent: %s \"%s\"\n", parentType, parent->name->c_str());
  }
}

void CE2Material::printObjectInfo(
    std::string& buf, size_t indentCnt, bool isLODMaterial) const
{
  CE2MaterialObject::printObjectInfo(buf, indentCnt);
  indentCnt = indentCnt + indentTabSize;
  if (flags)
  {
    buf.resize(buf.length() + indentCnt, ' ');
    buf += "Flags:";
    bool    firstFlag = true;
    for (unsigned int i = 0U; i < 32U && flags >= (1U << i); i++)
    {
      if (!(flags & (1U << i)))
        continue;
      buf += (firstFlag ? "  " : ", ");
      buf += materialFlagNames[i];
      firstFlag = false;
    }
    buf += '\n';
  }
  printToStringBuf(buf, indentCnt, "Alpha threshold: %f\n", alphaThreshold);
  printToStringBuf(buf, indentCnt,
                   "Opacity mode 1: %s\n", opacityModeNames[opacityMode1 & 3]);
  printToStringBuf(buf, indentCnt,
                   "Opacity mode 2: %s\n", opacityModeNames[opacityMode2 & 3]);
  printToStringBuf(buf, indentCnt,
                   "Emissive color and scale: %f, %f, %f, %f\n",
                   emissiveColor[0], emissiveColor[1], emissiveColor[2],
                   emissiveColor[3]);
  if (flags & Flag_IsEffect)
  {
    if (effectFlags)
    {
      buf.resize(buf.length() + indentCnt, ' ');
      buf += "Effect flags:";
      bool    firstFlag = true;
      for (unsigned int i = 0U; i < 32U && effectFlags >= (1U << i); i++)
      {
        if (!(effectFlags & (1U << i)))
          continue;
        buf += (firstFlag ? "  " : ", ");
        buf += effectFlagNames[i];
        firstFlag = false;
      }
      buf += '\n';
    }
    printToStringBuf(buf, indentCnt, "Effect blend mode: %s\n",
                     effectBlendModeNames[effectBlendMode & 7]);
  }
  for (unsigned int i = 0U; i < maxLayers && layerMask >= (1U << i); i++)
  {
    if (!((layerMask & (1U << i)) && layers[i]))
      continue;
    printToStringBuf(buf, indentCnt, "Layer %u:  ", i);
    layers[i]->printObjectInfo(buf, indentCnt);
    if (i < maxBlenders && blenders[i])
    {
      printToStringBuf(buf, indentCnt, "Blender %u:  ", i);
      blenders[i]->printObjectInfo(buf, indentCnt);
    }
  }
  if (!isLODMaterial)
  {
    for (unsigned int i = 0U; i < maxLODMaterials; i++)
    {
      if (!lodMaterials[i] || lodMaterials[i] == this)
        continue;
      printToStringBuf(buf, indentCnt, "LOD material %u:  ", i);
      lodMaterials[i]->printObjectInfo(buf, indentCnt, true);
    }
  }
}

void CE2Material::Blender::printObjectInfo(
    std::string& buf, size_t indentCnt) const
{
  CE2MaterialObject::printObjectInfo(buf, indentCnt);
  indentCnt = indentCnt + indentTabSize;
  if (uvStream)
  {
    buf.resize(buf.length() + indentCnt, ' ');
    buf += "UV stream:  ";
    uvStream->printObjectInfo(buf, indentCnt);
  }
  buf.resize(buf.length() + indentCnt, ' ');
  buf += "Float params:";
  for (unsigned int i = 0U; i < maxFloatParams; i++)
    printToStringBuf(buf, 0, "%s%f", (!i ? "  " : ", "), floatParams[i]);
  buf += '\n';
  buf.resize(buf.length() + indentCnt, ' ');
  buf += "Bool params:";
  for (unsigned int i = 0U; i < maxBoolParams; i++)
    printToStringBuf(buf, 0, "%s%d", (!i ? "  " : ", "), int(boolParams[i]));
  buf += '\n';
  printToStringBuf(buf, indentCnt,
                   "Texture replacement: 0x%08X, file: \"",
                   (unsigned int) textureReplacement);
  buf += *texturePath;
  buf += "\"\n";
}

void CE2Material::Layer::printObjectInfo(
    std::string& buf, size_t indentCnt) const
{
  CE2MaterialObject::printObjectInfo(buf, indentCnt);
  indentCnt = indentCnt + indentTabSize;
  if (uvStream)
  {
    buf.resize(buf.length() + indentCnt, ' ');
    buf += "UV stream:  ";
    uvStream->printObjectInfo(buf, indentCnt);
  }
  if (material)
  {
    buf.resize(buf.length() + indentCnt, ' ');
    buf += "Material:  ";
    material->printObjectInfo(buf, indentCnt);
  }
}

void CE2Material::Material::printObjectInfo(
    std::string& buf, size_t indentCnt) const
{
  CE2MaterialObject::printObjectInfo(buf, indentCnt);
  indentCnt = indentCnt + indentTabSize;
  printToStringBuf(buf, indentCnt,
                   "Color: %f, %f, %f, %f\n",
                   color[0], color[1], color[2], color[3]);
  printToStringBuf(buf, indentCnt,
                   "Color override mode: %s\n", colorModeNames[colorMode & 1]);
  if (textureSet)
  {
    buf.resize(buf.length() + indentCnt, ' ');
    buf += "Texture set:  ";
    textureSet->printObjectInfo(buf, indentCnt);
  }
}

void CE2Material::TextureSet::printObjectInfo(
    std::string& buf, size_t indentCnt) const
{
  CE2MaterialObject::printObjectInfo(buf, indentCnt);
  indentCnt = indentCnt + indentTabSize;
  printToStringBuf(buf, indentCnt, "Float param: %f\n", floatParam);
  for (unsigned int i = 0U; i < maxTexturePaths; i++)
  {
    printToStringBuf(buf, indentCnt,
                     "Texture %2u: replacement: 0x%08X, file: \"",
                     i, (unsigned int) textureReplacements[i]);
    buf += *(texturePaths[i]);
    buf += "\"\n";
  }
}

void CE2Material::UVStream::printObjectInfo(
    std::string& buf, size_t indentCnt) const
{
  CE2MaterialObject::printObjectInfo(buf, indentCnt);
  indentCnt = indentCnt + indentTabSize;
  printToStringBuf(buf, indentCnt,
                   "U, V scale: %f, %f\n",
                   scaleAndOffset[0], scaleAndOffset[1]);
  printToStringBuf(buf, indentCnt,
                   "U, V offset: %f, %f\n",
                   scaleAndOffset[2], scaleAndOffset[3]);
  printToStringBuf(buf, indentCnt,
                   "Texture address mode: %s\n",
                   textureAddressModeNames[textureAddressMode & 3]);
}

