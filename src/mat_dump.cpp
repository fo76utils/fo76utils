
#include "common.hpp"
#include "material.hpp"

static const size_t indentTabSize = 2;

static const char *objectTypeStrings[7] =
{
  "", "MaterialFile", "Blender", "Layer", "Material", "TextureSet", "UVStream"
};

const char * CE2Material::materialFlagNames[32] =
{
  "Has opacity",                        //  0 (0x00000001)
  "Alpha uses vertex color",            //  1 (0x00000002)
  "Is effect",                          //  2 (0x00000004)
  "Is decal",                           //  3 (0x00000008)
  "Two sided",                          //  4 (0x00000010)
  "Is vegetation",                      //  5 (0x00000020)
  "Layered emissivity",                 //  6 (0x00000040)
  "Emissive",                           //  7 (0x00000080)
  "Is translucent",                     //  8 (0x00000100)
  "Alpha detail blend mask",            //  9 (0x00000200)
  "Dithered transparency",              // 10 (0x00000400)
  "Ordered with previous",              // 11 (0x00000800)
  "Alpha blending",                     // 12 (0x00001000)
  "Has vertex colors",                  // 13 (0x00002000)
  "Is water",                           // 14 (0x00004000)
  "Hidden",                             // 15 (0x00008000)
  "Has opacity component",              // 16 (0x00010000)
  "Opacity layer 2 active",             // 17 (0x00020000)
  "Opacity layer 3 active",             // 18 (0x00040000)
  "Is terrain",                         // 19 (0x00080000)
  "Is hair",                            // 20 (0x00100000)
  "Use detail blender",                 // 21 (0x00200000)
  "Layered edge falloff",               // 22 (0x00400000)
  "",                                   // 23 (0x00800000)
  "",                                   // 24 (0x01000000)
  "",                                   // 25 (0x02000000)
  "",                                   // 26 (0x04000000)
  "",                                   // 27 (0x08000000)
  "",                                   // 28 (0x10000000)
  "",                                   // 29 (0x20000000)
  "",                                   // 30 (0x40000000)
  ""                                    // 31 (0x80000000)
};

const char * CE2Material::shaderModelNames[64] =
{
  "1LayerEffect",                       //  0
  "1LayerEffectEmissive",               //  1
  "1LayerEffectEmissiveOnly",           //  2
  "1LayerEffectEmissiveOnlyRemap",      //  3
  "1LayerEffectEmissiveRemap",          //  4
  "1LayerEffectGlass",                  //  5
  "1LayerEffectGlassNoFrost",           //  6
  "1LayerEffectRemap",                  //  7
  "1LayerEyebrow",                      //  8
  "1LayerMouth",                        //  9
  "1LayerPlanetDecal",                  // 10
  "1LayerStandard",                     // 11
  "1LayerStandardDecal",                // 12
  "1LayerStandardDecalEmissive",        // 13
  "2LayerEffect",                       // 14
  "2LayerEffectEmissive",               // 15
  "2LayerEffectEmissiveOnly",           // 16
  "2LayerEffectEmissiveOnlyRemap",      // 17
  "2LayerEffectEmissiveRemap",          // 18
  "2LayerEffectRemap",                  // 19
  "2LayerPlanetDecal",                  // 20
  "2LayerStandard",                     // 21
  "2LayerStandardEmissive",             // 22
  "3LayerEffect",                       // 23
  "3LayerEffectEmissive",               // 24
  "3LayerEffectEmissiveOnly",           // 25
  "3LayerPlanetDecal",                  // 26
  "3LayerStandard",                     // 27
  "3LayerStandardEmissive",             // 28
  "4LayerPlanetDecal",                  // 29
  "4LayerStandard",                     // 30
  "BaseMaterial",                       // 31
  "BodySkin1Layer",                     // 32
  "BodySkin2Layer",                     // 33
  "Character2Layer",                    // 34
  "Character3Layer",                    // 35
  "Character4Layer",                    // 36
  "CloudCard",                          // 37
  "ColorEmissive",                      // 38
  "Debug",                              // 39
  "Experimental",                       // 40
  "Eye1Layer",                          // 41
  "EyeWetness1Layer",                   // 42
  "GlobalLayer",                        // 43
  "Hair1Layer",                         // 44
  "Invisible",                          // 45
  "Occlusion",                          // 46
  "PlanetaryRing",                      // 47
  "Skin5Layer",                         // 48
  "StarmapBodyEffect",                  // 49
  "Terrain1Layer",                      // 50
  "TerrainEmissive1Layer",              // 51
  "TranslucentThin1Layer",              // 52
  "TranslucentTwoSided1Layer",          // 53
  "TwoSided1Layer",                     // 54
  "Unknown",                            // 55
  "VegetationEmissive1Layer",           // 56
  "VegetationEmissive2Layer",           // 57
  "VegetationStandard1Layer",           // 58
  "VegetationStandard2Layer",           // 59
  "VegetationTranslucent1Layer",        // 60
  "VegetationTranslucent2Layer",        // 61
  "Water",                              // 62
  "Water1Layer"                         // 63
};

static const char *shaderRouteNames[8] =
{
  "Deferred", "Effect", "PlanetaryRing", "PrecomputedScattering",
  "Water", "Unknown", "Unknown", "Unknown"
};

static const char *colorChannelNames[4] =
{
  "Red", "Green", "Blue", "Alpha"
};

static const char *alphaBlendModeNames[8] =
{
  "Linear", "Additive", "PositionContrast", "None",
  "CharacterCombine", "Skin", "Unknown", "Unknown"
};

static const char *blenderModeNames[4] =
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

static const char *decalBlendModeNames[2] =
{
  "None", "Additive"
};

static const char *decalRenderLayerNames[2] =
{
  "Top", "Middle"
};

static const char *maskSourceBlenderNames[4] =
{
  "None", "Blender1", "Blender2", "Blender3"
};

static const char *channelNames[4] =
{
  "Zero", "One", "Two", "Three"
};

static const char *resolutionSettingNames[4] =
{
  "Tiling", "UniqueMap", "DetailMapTiling", "HighResUniqueMap"
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
  if (shaderModel != 31 || shaderRoute != 0)
  {
    printToStringBuf(buf, indentCnt, "Shader model: %s, route: %s\n",
                     shaderModelNames[shaderModel & 63],
                     shaderRouteNames[shaderRoute & 7]);
  }
  if (flags & Flag_HasOpacity)
  {
    printToStringBuf(buf, indentCnt, "Alpha threshold: %f\n", alphaThreshold);
    printToStringBuf(buf, indentCnt, "Alpha source layer: %d\n",
                     int(alphaSourceLayer));
    printToStringBuf(buf, indentCnt, "Alpha blend mode: %s\n",
                     alphaBlendModeNames[alphaBlendMode & 7]);
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
                       blenderModeNames[opacityBlender1Mode & 3]);
    }
    if (flags & Flag_OpacityLayer3Active)
    {
      printToStringBuf(buf, indentCnt, "Opacity layer 3: %d\n",
                       int(opacityLayer3));
      printToStringBuf(buf, indentCnt, "Opacity blender 2: %d\n",
                       int(opacityBlender2));
      printToStringBuf(buf, indentCnt, "Opacity blender 2 mode: %s\n",
                       blenderModeNames[opacityBlender2Mode & 3]);
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
    printToStringBuf(buf, indentCnt, "Emissive mask source blender: %s\n",
                     maskSourceBlenderNames[
                         emissiveSettings->maskSourceBlender & 3]);
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
  if ((flags & Flag_LayeredEmissivity) && layeredEmissiveSettings &&
      layeredEmissiveSettings->isEnabled)
  {
    printToStringBuf(buf, indentCnt, "Layered emissivity layer 1 index: %d\n",
                     int(layeredEmissiveSettings->layer1Index));
    FloatVector4  c(&(layeredEmissiveSettings->layer1Tint));
    c *= (1.0f / 255.0f);
    printToStringBuf(buf, indentCnt,
                     "Layered emissivity layer 1 tint: %f, %f, %f, %f\n",
                     c[0], c[1], c[2], c[3]);
    printToStringBuf(buf, indentCnt, "Layered emissivity layer 1 mask: %s\n",
                     maskSourceBlenderNames[
                         layeredEmissiveSettings->layer1MaskIndex & 3]);
    if (layeredEmissiveSettings->layer2Active)
    {
      printToStringBuf(buf, indentCnt, "Layered emissivity layer 2 index: %d\n",
                       int(layeredEmissiveSettings->layer2Index));
      c = FloatVector4(&(layeredEmissiveSettings->layer2Tint));
      c *= (1.0f / 255.0f);
      printToStringBuf(buf, indentCnt,
                       "Layered emissivity layer 2 tint: %f, %f, %f, %f\n",
                       c[0], c[1], c[2], c[3]);
      printToStringBuf(buf, indentCnt, "Layered emissivity layer 2 mask: %s\n",
                       maskSourceBlenderNames[
                           layeredEmissiveSettings->layer2MaskIndex & 3]);
      printToStringBuf(buf, indentCnt,
                       "Layered emissivity blender 1 index: %d\n",
                       int(layeredEmissiveSettings->blender1Index));
      printToStringBuf(buf, indentCnt,
                       "Layered emissivity blender 1 mode: %s\n",
                       blenderModeNames[
                           layeredEmissiveSettings->blender1Mode & 3]);
    }
    if (layeredEmissiveSettings->layer3Active)
    {
      printToStringBuf(buf, indentCnt, "Layered emissivity layer 3 index: %d\n",
                       int(layeredEmissiveSettings->layer3Index));
      c = FloatVector4(&(layeredEmissiveSettings->layer3Tint));
      c *= (1.0f / 255.0f);
      printToStringBuf(buf, indentCnt,
                       "Layered emissivity layer 3 tint: %f, %f, %f, %f\n",
                       c[0], c[1], c[2], c[3]);
      printToStringBuf(buf, indentCnt, "Layered emissivity layer 3 mask: %s\n",
                       maskSourceBlenderNames[
                           layeredEmissiveSettings->layer3MaskIndex & 3]);
      printToStringBuf(buf, indentCnt,
                       "Layered emissivity blender 2 index: %d\n",
                       int(layeredEmissiveSettings->blender2Index));
      printToStringBuf(buf, indentCnt,
                       "Layered emissivity blender 2 mode: %s\n",
                       blenderModeNames[
                           layeredEmissiveSettings->blender2Mode & 3]);
    }
    printToStringBuf(buf, indentCnt, "Layered emissivity: adaptive: %s\n",
                     (!layeredEmissiveSettings->adaptiveEmittance ?
                      "False" : "True"));
    printToStringBuf(buf, indentCnt,
                     "Layered emissivity: adaptive limits: %s\n",
                     (!layeredEmissiveSettings->enableAdaptiveLimits ?
                      "False" : "True"));
    printToStringBuf(buf, indentCnt, "Layered emissive clip threshold: %f\n",
                     layeredEmissiveSettings->clipThreshold);
    printToStringBuf(buf, indentCnt, "Layered luminous emittance: %f\n",
                     layeredEmissiveSettings->luminousEmittance);
    printToStringBuf(buf, indentCnt, "Layered emittance exposure offset: %f\n",
                     layeredEmissiveSettings->exposureOffset);
    printToStringBuf(buf, indentCnt, "Layered emittance minimum offset: %f\n",
                     layeredEmissiveSettings->minOffset);
    printToStringBuf(buf, indentCnt, "Layered emittance maximum offset: %f\n",
                     layeredEmissiveSettings->maxOffset);
  }
  if ((flags & Flag_Translucency) && translucencySettings &&
      translucencySettings->isEnabled)
  {
    printToStringBuf(buf, indentCnt, "Translucency: is thin: %s\n",
                     (!translucencySettings->isThin ? "False" : "True"));
    printToStringBuf(buf, indentCnt,
                     "Translucency: flip back face normals in view space: %s\n",
                     (!translucencySettings->flipBackFaceNormalsInVS ?
                      "False" : "True"));
    if (translucencySettings->useSSS)
    {
      printToStringBuf(buf, indentCnt,
                       "Translucency: subsurface scattering width: %f\n",
                       translucencySettings->sssWidth);
      printToStringBuf(buf, indentCnt,
                       "Translucency: subsurface scattering strength: %f\n",
                       translucencySettings->sssStrength);
    }
    printToStringBuf(buf, indentCnt, "Translucency: transmissive scale: %f\n",
                     translucencySettings->transmissiveScale);
    printToStringBuf(buf, indentCnt, "Translucency: transmittance width: %f\n",
                     translucencySettings->transmittanceWidth);
    printToStringBuf(buf, indentCnt,
                     "Translucency: spec lobe 0 roughness scale: %f\n",
                     translucencySettings->specLobe0RoughnessScale);
    printToStringBuf(buf, indentCnt,
                     "Translucency: spec lobe 1 roughness scale: %f\n",
                     translucencySettings->specLobe1RoughnessScale);
    printToStringBuf(buf, indentCnt, "Translucency source layer: %d\n",
                     int(translucencySettings->sourceLayer));
  }
  if ((flags & Flag_IsDecal) && decalSettings && decalSettings->isDecal)
  {
    printToStringBuf(buf, indentCnt, "Decal: is planet: %s\n",
                     (!decalSettings->isPlanet ? "False" : "True"));
    printToStringBuf(buf, indentCnt, "Decal blend mode: %s\n",
                     decalBlendModeNames[decalSettings->blendMode & 1]);
    printToStringBuf(buf, indentCnt, "Animated decal ignores TAA: %s\n",
                     (!decalSettings->animatedDecalIgnoresTAA ?
                      "False" : "True"));
    printToStringBuf(buf, indentCnt, "Decal overall alpha: %f\n",
                     decalSettings->decalAlpha);
    printToStringBuf(buf, indentCnt, "Decal write mask: 0x%08X\n",
                     (unsigned int) decalSettings->writeMask);
    if (decalSettings->isProjected)
    {
      printToStringBuf(buf, indentCnt,
                       "Projected decal uses parallax mapping: %s\n",
                       (!decalSettings->useParallaxMapping ? "False" : "True"));
      printToStringBuf(buf, indentCnt,
                       "Projected decal parallax mapping shadows: %s\n",
                       (!decalSettings->parallaxOcclusionShadows ?
                        "False" : "True"));
      printToStringBuf(buf, indentCnt,
                       "Projected decal parallax mapping maximum steps: %d\n",
                       int(decalSettings->maxParallaxSteps));
      printToStringBuf(buf, indentCnt,
                       "Projected decal height map file: \"%s\"\n",
                       decalSettings->surfaceHeightMap->c_str());
      printToStringBuf(buf, indentCnt,
                       "Projected decal parallax mapping scale: %f\n",
                       decalSettings->parallaxOcclusionScale);
      printToStringBuf(buf, indentCnt, "Projected decal render layer: %s\n",
                       decalRenderLayerNames[decalSettings->renderLayer & 1]);
      printToStringBuf(buf, indentCnt,
                       "Projected decal uses G buffer normals: %s\n",
                       (!decalSettings->useGBufferNormals ? "False" : "True"));
    }
  }
  if ((flags & Flag_IsVegetation) && vegetationSettings &&
      vegetationSettings->isEnabled)
  {
    printToStringBuf(buf, indentCnt, "Vegetation leaf frequency: %f\n",
                     vegetationSettings->leafFrequency);
    printToStringBuf(buf, indentCnt, "Vegetation leaf amplitude: %f\n",
                     vegetationSettings->leafAmplitude);
    printToStringBuf(buf, indentCnt, "Vegetation branch flexibility: %f\n",
                     vegetationSettings->branchFlexibility);
    printToStringBuf(buf, indentCnt, "Vegetation trunk flexibility: %f\n",
                     vegetationSettings->trunkFlexibility);
    printToStringBuf(buf, indentCnt,
                     "Vegetation terrain blend strength (deprecated): %f\n",
                     vegetationSettings->terrainBlendStrength);
    printToStringBuf(buf, indentCnt,
                     "Vegetation terrain blend gradient factor "
                     "(deprecated): %f\n",
                     vegetationSettings->terrainBlendGradientFactor);
  }
  if ((flags & Flag_UseDetailBlender) && detailBlenderSettings &&
      detailBlenderSettings->isEnabled)
  {
    if (!detailBlenderSettings->textureReplacementEnabled)
    {
      printToStringBuf(buf, indentCnt, "Detail blender texture file: \"");
    }
    else
    {
      printToStringBuf(buf, indentCnt,
                       "Detail blender texture replacement: 0x%08X, file: \"",
                       (unsigned int)
                           detailBlenderSettings->textureReplacement);
    }
    buf += *(detailBlenderSettings->texturePath);
    buf += "\"\n";
    if (detailBlenderSettings->uvStream)
    {
      buf.resize(buf.length() + indentCnt, ' ');
      buf += "Detail blender UV stream:  ";
      detailBlenderSettings->uvStream->printObjectInfo(buf, indentCnt);
    }
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
  printToStringBuf(buf, indentCnt, "Blend mode: %s\n",
                   alphaBlendModeNames[blendMode & 7]);
  printToStringBuf(buf, indentCnt, "Color channel: %s\n",
                   colorChannelNames[colorChannel & 3]);
  if (!textureReplacementEnabled)
  {
    printToStringBuf(buf, indentCnt, "Texture file: \"");
  }
  else
  {
    printToStringBuf(buf, indentCnt,
                     "Texture replacement: 0x%08X, file: \"",
                     (unsigned int) textureReplacement);
  }
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
  printToStringBuf(buf, indentCnt, "Resolution setting: %s\n",
                   resolutionSettingNames[resolutionHint & 3]);
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
                   "Texture address mode: %s, Channel: %s\n",
                   textureAddressModeNames[textureAddressMode & 3],
                   channelNames[channel & 3]);
}

