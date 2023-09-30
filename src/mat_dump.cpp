
#include "common.hpp"
#include "material.hpp"

static const size_t indentTabSize = 2;

static const char *objectTypeStrings[7] =
{
  "", "MaterialFile", "Blender", "Layer", "Material", "TextureSet", "UVStream"
};

const char * CE2Material::materialFlagNames[32] =
{
  "Has opacity",
  "Alpha uses vertex color",
  "Is effect",
  "Is decal",
  "Two sided",
  "Is vegetation",
  "Layered emissivity",
  "Emissive",
  "Is translucent",
  "Alpha detail blend mask",
  "Dithered transparency",
  "Ordered with previous",
  "Alpha blending",
  "Has vertex colors",
  "Is water",
  "Hidden",
  "Has opacity component",
  "Opacity layer 2 active",
  "Opacity layer 3 active",
  "Is terrain",
  "Is hair",
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

static const char *colorChannelNames[4] =
{
  "Red", "Green", "Blue", "Alpha"
};

static const char *alphaBlendModeNames[4] =
{
  "Linear", "Additive", "PositionContrast", "None"
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
  "UseFallOff", "UseRGBFallOff",
  "", "",
  "", "",
  "VertexColorBlend", "IsAlphaTested",
  "", "NoHalfResOptimization",
  "SoftEffect", "",
  "EmissiveOnlyEffect", "EmissiveOnlyAutomaticallyApplied",
  "ReceiveDirectionalShadows", "ReceiveNonDirectionalShadows",
  "IsGlass", "Frosting",
  "", "",
  "", "ZTest",
  "ZWrite", "",
  "BackLightingEnable", "",
  "", "",
  "", "DepthMVFixup",
  "DepthMVFixupEdgesOnly", "ForceRenderBeforeOIT"
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
  if (flags & Flag_HasOpacity)
  {
    printToStringBuf(buf, indentCnt, "Alpha threshold: %f\n", alphaThreshold);
    printToStringBuf(buf, indentCnt, "Alpha source layer: %d\n",
                     int(alphaSourceLayer));
    printToStringBuf(buf, indentCnt, "Alpha blend mode: %s\n",
                     alphaBlendModeNames[alphaBlendMode & 3]);
    if (flags & Flag_AlphaVertexColor)
    {
      printToStringBuf(buf, indentCnt, "Alpha vertex color channel: %s\n",
                       colorChannelNames[alphaVertexColorChannel & 3]);
    }
    printToStringBuf(buf, indentCnt, "Alpha height blend threshold: %f\n",
                     alphaHeightBlendThreshold);
    printToStringBuf(buf, indentCnt, "Alpha height blend factor: %f\n",
                     alphaHeightBlendFactor);
    if (alphaBlendMode == 2)
    {
      printToStringBuf(buf, indentCnt, "Alpha position: %f\n", alphaPosition);
      printToStringBuf(buf, indentCnt, "Alpha contrast: %f\n", alphaContrast);
    }
    if (alphaUVStream)
    {
      buf.resize(buf.length() + indentCnt, ' ');
      buf += "Alpha UV stream:  ";
      alphaUVStream->printObjectInfo(buf, indentCnt);
    }
  }
  if (flags & Flag_HasOpacityComponent)
  {
    printToStringBuf(buf, indentCnt, "Opacity layer 1: %d\n",
                     int(opacityLayer1));
    if (flags & Flag_OpacityLayer2Active)
    {
      printToStringBuf(buf, indentCnt, "Opacity layer 2: %d\n",
                       int(opacityLayer2));
      printToStringBuf(buf, indentCnt, "Opacity blender 1: %d\n",
                       int(opacityBlender1));
      printToStringBuf(buf, indentCnt, "Opacity blender 1 mode: %s\n",
                       opacityModeNames[opacityBlender1Mode & 3]);
    }
    if (flags & Flag_OpacityLayer3Active)
    {
      printToStringBuf(buf, indentCnt, "Opacity layer 3: %d\n",
                       int(opacityLayer3));
      printToStringBuf(buf, indentCnt, "Opacity blender 2: %d\n",
                       int(opacityBlender2));
      printToStringBuf(buf, indentCnt, "Opacity blender 2 mode: %s\n",
                       opacityModeNames[opacityBlender2Mode & 3]);
    }
    printToStringBuf(buf, indentCnt, "Specular opacity override: %f\n",
                     specularOpacityOverride);
  }
  if ((flags & Flag_IsEffect) && effectSettings)
  {
    if (effectSettings->flags)
    {
      buf.resize(buf.length() + indentCnt, ' ');
      buf += "Effect flags:";
      bool    firstFlag = true;
      for (unsigned int i = 0U;
           i < 32U && effectSettings->flags >= (1U << i); i++)
      {
        if (!(effectSettings->flags & (1U << i)))
          continue;
        buf += (firstFlag ? "  " : ", ");
        buf += effectFlagNames[i];
        firstFlag = false;
      }
      buf += '\n';
    }
    printToStringBuf(buf, indentCnt, "Effect blend mode: %s\n",
                     effectBlendModeNames[effectSettings->blendMode & 7]);
    if (effectSettings->flags
        & (EffectFlag_UseFalloff | EffectFlag_UseRGBFalloff))
    {
      printToStringBuf(buf, indentCnt, "Falloff start, stop angle: %f, %f\n",
                       effectSettings->falloffStartAngle,
                       effectSettings->falloffStopAngle);
      printToStringBuf(buf, indentCnt, "Falloff start, stop opacity: %f, %f\n",
                       effectSettings->falloffStartOpacity,
                       effectSettings->falloffStopOpacity);
    }
    printToStringBuf(buf, indentCnt, "Effect overall alpha: %f\n",
                     effectSettings->materialAlpha);
    if (effectSettings->flags & EffectFlag_IsAlphaTested)
    {
      printToStringBuf(buf, indentCnt, "Effect alpha threshold: %f\n",
                       effectSettings->alphaThreshold);
    }
    printToStringBuf(buf, indentCnt, "Effect soft falloff depth: %f\n",
                     effectSettings->softFalloffDepth);
    if (effectSettings->flags & EffectFlag_Frosting)
    {
      printToStringBuf(buf, indentCnt,
                       "Frosting unblurred background alpha blend: %f\n",
                       effectSettings->frostingBgndBlend);
      printToStringBuf(buf, indentCnt, "Frosting blur bias: %f\n",
                       effectSettings->frostingBlurBias);
    }
    if (effectSettings->flags & EffectFlag_BacklightEnable)
    {
      printToStringBuf(buf, indentCnt, "Backlight scale: %f\n",
                       effectSettings->backlightScale);
      printToStringBuf(buf, indentCnt, "Backlight sharpness: %f\n",
                       effectSettings->backlightSharpness);
      printToStringBuf(buf, indentCnt, "Backlight transparency factor: %f\n",
                       effectSettings->backlightTransparency);
      printToStringBuf(buf, indentCnt,
                       "Backlight tint color: %f, %f, %f, %f\n",
                       effectSettings->backlightTintColor[0],
                       effectSettings->backlightTintColor[1],
                       effectSettings->backlightTintColor[2],
                       effectSettings->backlightTintColor[3]);
    }
    printToStringBuf(buf, indentCnt, "Effect depth bias: %d\n",
                     effectSettings->depthBias);
  }
  if ((flags & Flag_Glow) && emissiveSettings && emissiveSettings->isEnabled)
  {
    printToStringBuf(buf, indentCnt, "Emissive source layer: %d\n",
                     int(emissiveSettings->sourceLayer));
    printToStringBuf(buf, indentCnt, "Emissive tint: %f, %f, %f, %f\n",
                     emissiveSettings->emissiveTint[0],
                     emissiveSettings->emissiveTint[1],
                     emissiveSettings->emissiveTint[2],
                     emissiveSettings->emissiveTint[3]);
    if (!emissiveSettings->maskSourceBlender)
    {
      printToStringBuf(buf, indentCnt, "Emissive mask source blender: None\n");
    }
    else
    {
      printToStringBuf(buf, indentCnt,
                       "Emissive mask source blender: Blender%d\n",
                       int(emissiveSettings->maskSourceBlender));
    }
    printToStringBuf(buf, indentCnt, "Adaptive emittance: %s\n",
                     (!emissiveSettings->adaptiveEmittance ? "False" : "True"));
    printToStringBuf(buf, indentCnt, "Enable adaptive limits: %s\n",
                     (!emissiveSettings->enableAdaptiveLimits ?
                      "False" : "True"));
    printToStringBuf(buf, indentCnt, "Emissive clip threshold: %f\n",
                     emissiveSettings->clipThreshold);
    printToStringBuf(buf, indentCnt, "Luminous emittance: %f\n",
                     emissiveSettings->luminousEmittance);
    printToStringBuf(buf, indentCnt, "Emittance exposure offset: %f\n",
                     emissiveSettings->exposureOffset);
    printToStringBuf(buf, indentCnt, "Emittance minimum offset: %f\n",
                     emissiveSettings->minOffset);
    printToStringBuf(buf, indentCnt, "Emittance maximum offset: %f\n",
                     emissiveSettings->maxOffset);
  }
  for (unsigned int i = 0U; i < maxLayers && layerMask >= (1U << i); i++)
  {
    if (!((layerMask & (1U << i)) && layers[i]))
      continue;
    printToStringBuf(buf, indentCnt, "Layer %u:  ", i);
    layers[i]->printObjectInfo(buf, indentCnt);
    unsigned int  j = i - 1U;
    if (j < maxBlenders && blenders[j])
    {
      printToStringBuf(buf, indentCnt, "Blender %u:  ", j);
      blenders[j]->printObjectInfo(buf, indentCnt);
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
    if (!((texturePathMask | textureReplacementMask) & (1U << i)))
      continue;
    if (!(textureReplacementMask & (1U << i)))
    {
      printToStringBuf(buf, indentCnt, "Texture %2u: file: \"", i);
    }
    else
    {
      printToStringBuf(buf, indentCnt,
                       "Texture %2u: replacement: 0x%08X, file: \"",
                       i, (unsigned int) textureReplacements[i]);
    }
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

