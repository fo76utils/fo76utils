
#include "common.hpp"
#include "ba2file.hpp"
#include "material.hpp"

#include "matnames.cpp"

static void printObjectInfo(
    std::FILE *f, const CE2MaterialObject *o, const char *indentString)
{
  if (!o)
    return;
  std::fprintf(f, "\"%s\":\n", o->name->c_str());
  indentString = indentString - 4;
  if (o->e == 0x0074616DU)              // "mat\0"
  {
    std::fprintf(f, "%sHash: 0x%016llX\n",
                 indentString, (unsigned long long) o->h);
  }
  if (o->parent)
  {
    const char  *parentType = "Unknown";
    if (o->parent->type >= 0 && o->parent->type <= 6)
      parentType = objectTypeStrings[o->parent->type];
    std::fprintf(f, "%sParent: %s \"%s\"\n",
                 indentString, parentType, o->parent->name->c_str());
  }
}

static void printUVStream(
    std::FILE *f, const CE2Material::UVStream *o, const char *indentString)
{
  printObjectInfo(f, o, indentString);
  indentString = indentString - 4;
  std::fprintf(f, "%sU, V scale: %f, %f\n",
               indentString, o->scaleAndOffset[0], o->scaleAndOffset[1]);
  std::fprintf(f, "%sU, V offset: %f, %f\n",
               indentString, o->scaleAndOffset[2], o->scaleAndOffset[3]);
  std::fprintf(f, "%sTexture address mode: %s\n",
               indentString,
               textureAddressModeNames[o->textureAddressMode & 3]);
}

static void printTextureSet(
    std::FILE *f, const CE2Material::TextureSet *o, const char *indentString)
{
  printObjectInfo(f, o, indentString);
  indentString = indentString - 4;
  std::fprintf(f, "%sFloat param: %f\n", indentString, o->floatParam);
  for (size_t i = 0; i < CE2Material::TextureSet::maxTexturePaths; i++)
  {
    std::fprintf(f, "%sTexture %2d: replacement: 0x%08X, file: \"%s\"\n",
                 indentString, int(i), (unsigned int) o->textureReplacements[i],
                 o->texturePaths[i]->c_str());
  }
}

static void printMaterial(
    std::FILE *f, const CE2Material::Material *o, const char *indentString)
{
  printObjectInfo(f, o, indentString);
  indentString = indentString - 4;
  std::fprintf(f, "%sColor: %f, %f, %f, %f\n",
               indentString,
               o->color[0], o->color[1], o->color[2], o->color[3]);
  std::fprintf(f, "%sColor override mode: %s\n",
               indentString, colorModeNames[o->colorMode & 1]);
  if (o->textureSet)
  {
    std::fprintf(f, "%sTexture set:  ", indentString);
    printTextureSet(f, o->textureSet, indentString);
  }
}

static void printLayer(
    std::FILE *f, const CE2Material::Layer *o, const char *indentString)
{
  if (!o)
    return;
  printObjectInfo(f, o, indentString);
  indentString = indentString - 4;
  if (o->uvStream)
  {
    std::fprintf(f, "%sUV stream:  ", indentString);
    printUVStream(f, o->uvStream, indentString);
  }
  if (o->material)
  {
    std::fprintf(f, "%sMaterial:  ", indentString);
    printMaterial(f, o->material, indentString);
  }
}

static void printBlender(
    std::FILE *f, const CE2Material::Blender *o, const char *indentString)
{
  printObjectInfo(f, o, indentString);
  indentString = indentString - 4;
  if (o->uvStream)
  {
    std::fprintf(f, "%sUV stream:  ", indentString);
    printUVStream(f, o->uvStream, indentString);
  }
  std::fprintf(f, "%sFloat params:", indentString);
  for (size_t i = 0; i < CE2Material::Blender::maxFloatParams; i++)
    std::fprintf(f, "%s%f", (!i ? "  " : ", "), o->floatParams[i]);
  std::fprintf(f, "\n%sBool params:", indentString);
  for (size_t i = 0; i < CE2Material::Blender::maxBoolParams; i++)
    std::fprintf(f, "%s%d", (!i ? "  " : ", "), int(o->boolParams[i]));
  std::fprintf(f, "\n%sTexture replacement: 0x%08X, file: \"%s\"\n",
               indentString,
               (unsigned int) o->textureReplacement, o->texturePath->c_str());
}

static void printMaterialFile(
    std::FILE *f, const CE2Material *o, const char *indentString,
    bool isLODMaterial = false)
{
  printObjectInfo(f, o, indentString);
  indentString = indentString - 4;
  unsigned int  flags = o->flags;
  if (flags)
  {
    std::fprintf(f, "%sFlags:", indentString);
    bool    firstFlag = true;
    for (size_t i = 0; i < 32 && flags; i++, flags = flags >> 1)
    {
      if (!(flags & 1U))
        continue;
      std::fprintf(f, "%s%s", (firstFlag ? "  " : ", "), materialFlagNames[i]);
      firstFlag = false;
    }
    std::fputc('\n', f);
  }
  std::fprintf(f, "%sAlpha threshold: %f\n", indentString, o->alphaThreshold);
  std::fprintf(f, "%sOpacity mode 1: %s\n",
               indentString, opacityModeNames[o->opacityMode1 & 3]);
  std::fprintf(f, "%sOpacity mode 2: %s\n",
               indentString, opacityModeNames[o->opacityMode2 & 3]);
  unsigned int  layerMask = o->layerMask;
  for (size_t i = 0; i < CE2Material::maxLayers && layerMask;
       i++, layerMask = layerMask >> 1)
  {
    if (!(layerMask & 1U))
      continue;
    std::fprintf(f, "%sLayer %d:  ", indentString, int(i));
    printLayer(f, o->layers[i], indentString);
    if (i < CE2Material::maxBlenders && o->blenders[i])
    {
      std::fprintf(f, "%sBlender %d:  ", indentString, int(i));
      printBlender(f, o->blenders[i], indentString);
    }
  }
  if (!isLODMaterial)
  {
    for (size_t i = 0; i < CE2Material::maxLODMaterials; i++)
    {
      if (!o->lodMaterials[i] || o->lodMaterials[i] == o)
        continue;
      std::fprintf(f, "%sLOD material %d:  ", indentString, int(i));
      printMaterialFile(f, o->lodMaterials[i], indentString, true);
    }
  }
}

static const char *indentString = "                                ";

static const char *usageString =
    "Usage: mat_info [OPTIONS...] ARCHIVEPATH [FILE1.MAT [FILE2.MAT...]]\n"
    "\n"
    "Options:\n"
    "    --help          print usage\n"
    "    --              remaining options are file names\n"
    "    -cdb FILENAME   set material database file name\n";

int main(int argc, char **argv)
{
  try
  {
    std::vector< const char * > args;
    const char  *cdbFileName = (char *) 0;
    for (int i = 1; i < argc; i++)
    {
      if (argv[i][0] != '-')
      {
        args.push_back(argv[i]);
        continue;
      }
      if (std::strcmp(argv[i], "--") == 0)
      {
        while (++i < argc)
          args.push_back(argv[i]);
        break;
      }
      if (std::strcmp(argv[i], "-h") == 0 ||
          std::strcmp(argv[i], "--help") == 0)
      {
        std::printf("%s", usageString);
        return 0;
      }
      if (std::strcmp(argv[i], "-cdb") == 0)
      {
        if (++i >= argc)
          throw FO76UtilsError("missing argument for %s", argv[i - 1]);
        cdbFileName = argv[i];
      }
      else
      {
        throw FO76UtilsError("invalid option: %s", argv[i]);
      }
    }
    if (args.size() < 1)
    {
      std::fprintf(stderr, "%s", usageString);
      return 1;
    }
    BA2File ba2File(args[0], ".cdb");
    CE2MaterialDB materials(ba2File, cdbFileName);
    for (size_t i = 1; i < args.size(); i++)
    {
      const CE2Material *o = materials.findMaterial(std::string(args[i]));
      if (!o)
      {
        std::fprintf(stderr, "Material '%s' not found in the database\n",
                     args[i]);
        continue;
      }
      std::printf("==== %s ====\n", args[i]);
      printMaterialFile(stdout, o, indentString + std::strlen(indentString));
    }
  }
  catch (std::exception& e)
  {
    std::fprintf(stderr, "mat_info: %s\n", e.what());
    return 1;
  }
  return 0;
}

