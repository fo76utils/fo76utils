
#include "common.hpp"
#include "nif_file.hpp"
#include "ba2file.hpp"
#include "ddstxt.hpp"
#include "plot3d.hpp"
#include "sdlvideo.hpp"
#include "nif_view.hpp"

#include <algorithm>
#include <ctime>

static void printAuthorName(std::FILE *f,
                            const std::vector< unsigned char >& fileBuf,
                            const char *nifFileName)
{
  if (fileBuf.size() < 57)
    return;
  if (std::memcmp(fileBuf.data(), "Gamebryo File Format, Version 20", 32) != 0)
    return;
  size_t  nameLen = fileBuf[56];
  if ((nameLen + 57) > fileBuf.size())
    throw FO76UtilsError("%s: end of input file", nifFileName);
  std::string authorName;
  for (size_t i = 0; i < nameLen; i++)
  {
    char    c = char(fileBuf[i + 57]);
    if (!c)
      continue;
    if ((unsigned char) c < 0x20)
      c = ' ';
    authorName += c;
  }
  std::fprintf(f, "%s\t%s\t%lu\n",
               authorName.c_str(), nifFileName, (unsigned long) fileBuf.size());
}

static void printVertexTransform(std::FILE *f,
                                 const NIFFile::NIFVertexTransform& t)
{
  std::fprintf(f, "  NIFVertexTransform:\n");
  std::fprintf(f, "    Scale: %f\n", t.scale);
  std::fprintf(f, "    Rotation matrix: %8f, %8f, %8f\n",
               t.rotateXX, t.rotateXY, t.rotateXZ);
  std::fprintf(f, "                     %8f, %8f, %8f\n",
               t.rotateYX, t.rotateYY, t.rotateYZ);
  std::fprintf(f, "                     %8f, %8f, %8f\n",
               t.rotateZX, t.rotateZY, t.rotateZZ);
  std::fprintf(f, "    Offset: %f, %f, %f\n", t.offsX, t.offsY, t.offsZ);
}

static void printBlockList(std::FILE *f, const NIFFile& nifFile,
                           bool verboseMaterialInfo)
{
  std::fprintf(f, "BS version: 0x%08X\n", nifFile.getVersion());
  std::fprintf(f, "Author name: %s\n", nifFile.getAuthorName().c_str());
  std::fprintf(f, "Process script: %s\n",
               nifFile.getProcessScriptName().c_str());
  std::fprintf(f, "Export script: %s\n", nifFile.getExportScriptName().c_str());
  std::fprintf(f, "Block count: %u\n", (unsigned int) nifFile.getBlockCount());
  for (size_t i = 0; i < nifFile.getBlockCount(); i++)
  {
    std::fprintf(f, "  Block %3d: offset = 0x%08X, size = %7u, "
                 "name = \"%s\", type = %s\n",
                 int(i),
                 (unsigned int) nifFile.getBlockOffset(i),
                 (unsigned int) nifFile.getBlockSize(i),
                 nifFile.getBlockName(i), nifFile.getBlockTypeAsString(i));
    const NIFFile::NIFBlkNiNode *nodeBlock = nifFile.getNode(i);
    const NIFFile::NIFBlkBSTriShape *triShapeBlock = nifFile.getTriShape(i);
    const NIFFile::NIFBlkBSLightingShaderProperty *
        lspBlock = nifFile.getLightingShaderProperty(i);
    const NIFFile::NIFBlkBSShaderTextureSet *
        tsBlock = nifFile.getShaderTextureSet(i);
    const NIFFile::NIFBlkNiAlphaProperty *
        alphaPropertyBlock = nifFile.getAlphaProperty(i);
    if (nodeBlock)
    {
      if (nodeBlock->controller >= 0)
        std::fprintf(f, "    Controller: %3d\n", nodeBlock->controller);
      if (nodeBlock->collisionObject >= 0)
      {
        std::fprintf(f, "    Collision object: %3d\n",
                     nodeBlock->collisionObject);
      }
      if (nodeBlock->children.size() > 0)
      {
        std::fprintf(f, "    Children: ");
        for (size_t j = 0; j < nodeBlock->children.size(); j++)
          std::fprintf(f, "%s%3u", (!j ? "" : ", "), nodeBlock->children[j]);
        std::fprintf(f, "\n");
      }
    }
    else if (triShapeBlock)
    {
      if (triShapeBlock->controller >= 0)
        std::fprintf(f, "    Controller: %3d\n", triShapeBlock->controller);
      if (triShapeBlock->collisionObject >= 0)
      {
        std::fprintf(f, "    Collision object: %3d\n",
                     triShapeBlock->collisionObject);
      }
      if (triShapeBlock->skinID >= 0)
        std::fprintf(f, "    Skin: %3d\n", triShapeBlock->skinID);
      if (triShapeBlock->shaderProperty >= 0)
      {
        std::fprintf(f, "    Shader property: %3d\n",
                     triShapeBlock->shaderProperty);
      }
      if (triShapeBlock->alphaProperty >= 0)
      {
        std::fprintf(f, "    Alpha property: %3d\n",
                     triShapeBlock->alphaProperty);
      }
    }
    else if (lspBlock)
    {
      if (lspBlock->controller >= 0)
        std::fprintf(f, "    Controller: %3d\n", lspBlock->controller);
      if (lspBlock->textureSet >= 0)
        std::fprintf(f, "    Texture set: %3d\n", lspBlock->textureSet);
      std::fprintf(f, "    Material version: %2u\n",
                   (unsigned int) lspBlock->material.version);
      if (!verboseMaterialInfo)
      {
        std::fprintf(f, "    Material flags: 0x%04X\n",
                     (unsigned int) lspBlock->material.flags);
      }
      else
      {
        std::uint32_t m = lspBlock->material.flags;
        for (unsigned int j = 0U; m; j++, m = m >> 1)
        {
          if (!(m & 1U))
            continue;
          if ((m << j) != lspBlock->material.flags)
          {
            std::fprintf(f, ", %s", NIF_View::materialFlagNames[j]);
          }
          else
          {
            std::fprintf(f, "    Material flags: %s",
                         NIF_View::materialFlagNames[j]);
          }
        }
        if (lspBlock->material.flags)
          std::fputc('\n', f);
      }
      std::fprintf(f, "    Material alpha flags: 0x%04X\n",
                   (unsigned int) lspBlock->material.alphaFlags);
      std::fprintf(f, "    Material alpha threshold: %3u (%.3f)\n",
                   (unsigned int) lspBlock->material.alphaThreshold,
                   lspBlock->material.alphaThresholdFloat);
      std::fprintf(f, "    Material alpha: %.3f\n", lspBlock->material.alpha);
      if (!verboseMaterialInfo)
        continue;
      std::fprintf(f, "    Material texture U, V offset: %.3f, %.3f\n",
                   lspBlock->material.textureOffsetU,
                   lspBlock->material.textureOffsetV);
      std::fprintf(f, "    Material texture U, V scale: %.3f, %.3f\n",
                   lspBlock->material.textureScaleU,
                   lspBlock->material.textureScaleV);
      if (lspBlock->material.flags & BGSMFile::Flag_TSWater)
      {
        unsigned int  shallowColor =
            std::uint32_t(lspBlock->material.w.shallowColor * 255.0f);
        unsigned int  deepColor =
            std::uint32_t(lspBlock->material.w.deepColor * 255.0f);
        std::fprintf(f, "    Water shallow color (0xAABBGGRR): 0x%08X\n",
                     shallowColor);
        std::fprintf(f, "    Water deep color (0xAABBGGRR): 0x%08X\n",
                     deepColor);
        std::fprintf(f, "    Water depth range: %.3f\n",
                     lspBlock->material.w.maxDepth);
        std::fprintf(f, "    Water environment map scale: %.3f\n",
                     lspBlock->material.w.envMapScale);
        std::fprintf(f, "    Water specular smoothness: %.3f\n",
                     lspBlock->material.w.specularSmoothness);
      }
      else if (lspBlock->material.flags & BGSMFile::Flag_IsEffect)
      {
        unsigned int  baseColor =
            std::uint32_t(lspBlock->material.e.baseColor * 255.0f);
        std::fprintf(f, "    Effect base color (0xAABBGGRR): 0x%08X\n",
                     baseColor);
        std::fprintf(f, "    Effect base color scale: %.3f\n",
                     lspBlock->material.e.baseColorScale);
        std::fprintf(f, "    Effect lighting influence: %.3f\n",
                     lspBlock->material.e.lightingInfluence);
        std::fprintf(f, "    Effect environment map scale: %.3f\n",
                     lspBlock->material.e.envMapScale);
        std::fprintf(f, "    Effect specular smoothness: %.3f\n",
                     lspBlock->material.e.specularSmoothness);
        std::fprintf(f,
                     "    Effect falloff parameters: %.3f, %.3f, %.3f, %.3f\n",
                     lspBlock->material.e.falloffParams[0],
                     lspBlock->material.e.falloffParams[1],
                     lspBlock->material.e.falloffParams[2],
                     lspBlock->material.e.falloffParams[3]);
      }
      else
      {
        std::fprintf(f, "    Material gradient map scale: %.3f\n",
                     lspBlock->material.s.gradientMapV);
        std::fprintf(f, "    Material environment map scale: %.3f\n",
                     lspBlock->material.s.envMapScale);
        unsigned int  specularColor =
            std::uint32_t(lspBlock->material.s.specularColor * 255.0f);
        unsigned int  emissiveColor =
            std::uint32_t(lspBlock->material.s.emissiveColor * 255.0f);
        std::fprintf(f, "    Material specular color (0xBBGGRR): 0x%06X\n",
                     specularColor & 0x00FFFFFFU);
        std::fprintf(f, "    Material specular scale: %.3f\n",
                     lspBlock->material.s.specularColor[3]);
        std::fprintf(f, "    Material specular smoothness: %.3f\n",
                     lspBlock->material.s.specularSmoothness);
        std::fprintf(f, "    Material emissive color (0xBBGGRR): 0x%06X\n",
                     emissiveColor & 0x00FFFFFFU);
        std::fprintf(f, "    Material emissive scale: %.3f\n",
                     lspBlock->material.s.emissiveColor[3]);
      }
      unsigned int  m = lspBlock->material.texturePathMask;
      for (size_t j = 0; m; j++, m = m >> 1)
      {
        if (m & 1U)
        {
          std::fprintf(f, "    Material texture %d: %s\n",
                       int(j), lspBlock->material.texturePaths[j].c_str());
        }
      }
    }
    else if (tsBlock)
    {
      unsigned int  m = tsBlock->texturePathMask;
      for (size_t j = 0; m; j++, m = m >> 1)
      {
        if (m & 1U)
        {
          std::fprintf(f, "    Texture %2d: %s\n",
                       int(j), tsBlock->texturePaths[j].c_str());
        }
      }
    }
    else if (alphaPropertyBlock)
    {
      if (alphaPropertyBlock->controller >= 0)
      {
        std::fprintf(f, "    Controller: %3d\n",
                     alphaPropertyBlock->controller);
      }
      std::fprintf(f, "    Flags: 0x%04X\n",
                   (unsigned int) alphaPropertyBlock->flags);
      std::fprintf(f, "    Alpha threshold: %u\n",
                   (unsigned int) alphaPropertyBlock->alphaThreshold);
    }
  }
}

static void printMeshData(std::FILE *f, const NIFFile& nifFile)
{
  std::vector< NIFFile::NIFTriShape > meshData;
  nifFile.getMesh(meshData);
  for (size_t i = 0; i < meshData.size(); i++)
  {
    const NIFFile::NIFTriShape& ts = meshData[i];
    const char  *tsName = "";
    if (nifFile.getString(ts.nameID))
      tsName = nifFile.getString(ts.nameID)->c_str();
    std::fprintf(f, "TriShape %3d (%s):\n", int(i), tsName);
    std::fprintf(f, "  Vertex count: %u\n", ts.vertexCnt);
    std::fprintf(f, "  Triangle count: %u\n", ts.triangleCnt);
    if (ts.m.flags)
    {
      std::uint32_t m = ts.m.flags;
      for (unsigned int j = 0U; m; j++, m = m >> 1)
      {
        if (!(m & 1U))
          continue;
        if ((m << j) != ts.m.flags)
          std::fprintf(f, ", %s", NIF_View::materialFlagNames[j]);
        else
          std::fprintf(f, "  Flags: %s", NIF_View::materialFlagNames[j]);
      }
      std::fputc('\n', f);
    }
    if (ts.m.alphaThresholdFloat > 0.0f)
      std::fprintf(f, "  Alpha threshold: %.3f\n", ts.m.alphaThresholdFloat);
    if (ts.m.flags & BGSMFile::Flag_TSAlphaBlending)
    {
      std::fprintf(f, "  Alpha blend scale: %.3f\n", ts.m.alpha);
    }
    printVertexTransform(f, ts.vertexTransform);
    if (ts.haveMaterialPath())
      std::fprintf(f, "  Material: %s\n", ts.materialPath().c_str());
    std::fprintf(f, "  Texture UV offset, scale: (%f, %f), (%f, %f)\n",
                 ts.m.textureOffsetU, ts.m.textureOffsetV,
                 ts.m.textureScaleU, ts.m.textureScaleV);
    unsigned int  m = ts.m.texturePathMask;
    for (int j = 0; m; j++, m = m >> 1)
    {
      if (m & 1U)
        std::fprintf(f, "  Texture %2d: %s\n", j, ts.m.texturePaths[j].c_str());
    }
    std::fprintf(f, "  Vertex list:\n");
    for (size_t j = 0; j < ts.vertexCnt; j++)
    {
      NIFFile::NIFVertex  v = ts.vertexData[j];
      ts.vertexTransform.transformXYZ(v.x, v.y, v.z);
      FloatVector4  normal(ts.vertexTransform.rotateXYZ(v.getNormal()));
      std::fprintf(f, "    %4d: XYZ: (%f, %f, %f), normals: (%f, %f, %f), "
                   "UV: (%f, %f), color: 0x%08X\n",
                   int(j), v.x, v.y, v.z, normal[0], normal[1], normal[2],
                   v.getU(), v.getV(), (unsigned int) v.vertexColor);
    }
    std::fprintf(f, "  Triangle list:\n");
    for (size_t j = 0; j < ts.triangleCnt; j++)
    {
      std::fprintf(f, "    %4d: %4u, %4u, %4u\n",
                   int(j), ts.triangleData[j].v0,
                   ts.triangleData[j].v1, ts.triangleData[j].v2);
    }
  }
}

struct ObjFileMaterialNames
{
  std::set< std::string > namesUsed;
  std::map< std::string, std::string >  nameMap;
  bool getMaterialName(std::string& s, const NIFFile::NIFTriShape& ts);
};

bool ObjFileMaterialNames::getMaterialName(
    std::string& s, const NIFFile::NIFTriShape& ts)
{
  s.clear();
  std::string fullPath;
  if (ts.haveMaterialPath())
  {
    s = ts.materialPath();
    fullPath = s;
    if (s.ends_with(".bgsm") || s.ends_with(".bgem"))
      s.resize(s.length() - 5);
    size_t  n = s.rfind('/');
    if (n != std::string::npos)
      s.erase(0, n + 1);
  }
  if (s.empty())
  {
    fullPath.clear();
    s = "Material";
  }
  if (!fullPath.empty())
  {
    std::map< std::string, std::string >::const_iterator  i =
        nameMap.find(fullPath);
    if (i != nameMap.end())
    {
      s = i->second;
      return false;
    }
  }
  for (size_t j = 0; j < s.length(); j++)
  {
    char    c = s[j];
    if (!((c >= '0' && c <= '9') ||
          (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')))
    {
      s[j] = '_';
    }
  }
  while (namesUsed.find(s) != namesUsed.end())
  {
    if (!(s.length() > 5 && s[s.length() - 5] == '_' &&
          (FileBuffer::readUInt32Fast(s.c_str() + (s.length() - 4))
           & 0xF0F0F0F0U) == 0x30303030U))
    {
      s += "_0001";
    }
    else
    {
      for (size_t j = s.length() - 1; s[j] >= '0' && s[j] <= '9'; j--)
      {
        if (s[j] == '9')
        {
          s[j] = '0';
          continue;
        }
        s[j]++;
        break;
      }
      if (s.ends_with("_0000"))
        s += '1';
    }
  }
  namesUsed.insert(s);
  if (!fullPath.empty())
    nameMap.insert(std::pair< std::string, std::string >(fullPath, s));
  return true;
}

static void printOBJData(std::FILE *f, const NIFFile& nifFile,
                         const char *mtlFileName, bool enableVertexColors)
{
  ObjFileMaterialNames  matNames;
  std::string matName;
  std::vector< NIFFile::NIFTriShape > meshData;
  nifFile.getMesh(meshData);
  unsigned int  vertexNumBase = 1;
  std::fprintf(f, "mtllib %s\n\n", mtlFileName);
  for (size_t i = 0; i < meshData.size(); i++)
  {
    const NIFFile::NIFTriShape& ts = meshData[i];
    const char  *tsName = "";
    if (nifFile.getString(ts.nameID))
      tsName = nifFile.getString(ts.nameID)->c_str();
    std::fprintf(f, "# %s\n\ng ", tsName);
    for (size_t j = 0; tsName[j]; j++)
    {
      char    c = tsName[j];
      if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
          (c >= '0' && c <= '9'))
      {
        std::fputc(c, f);
      }
    }
    matNames.getMaterialName(matName, ts);
    std::fprintf(f, "\nusemtl %s\n\n", matName.c_str());
    bool    haveVertexColors = false;
    if (enableVertexColors)
    {
      for (size_t j = 0; j < ts.vertexCnt; j++)
      {
        if (ts.vertexData[j].vertexColor != 0xFFFFFFFFU)
        {
          haveVertexColors = true;
          break;
        }
      }
    }
    for (size_t j = 0; j < ts.vertexCnt; j++)
    {
      NIFFile::NIFVertex  v = ts.vertexData[j];
      ts.vertexTransform.transformXYZ(v.x, v.y, v.z);
      if (!haveVertexColors)
      {
        std::fprintf(f, "v %.8f %.8f %.8f\n", v.x, v.y, v.z);
      }
      else
      {
        FloatVector4  c(&(v.vertexColor));
        c *= (1.0f / 255.0f);
        std::fprintf(f, "v %.8f %.8f %.8f %.4f %.4f %.4f\n",
                     v.x, v.y, v.z, c[0], c[1], c[2]);
      }
    }
    for (size_t j = 0; j < ts.vertexCnt; j++)
    {
      NIFFile::NIFVertex  v = ts.vertexData[j];
      std::fprintf(f, "vt %.8f %.8f\n", v.getU(), 1.0 - v.getV());
    }
    for (size_t j = 0; j < ts.vertexCnt; j++)
    {
      FloatVector4  normal(ts.vertexTransform.rotateXYZ(
                               ts.vertexData[j].getNormal()));
      std::fprintf(f, "vn %.8f %.8f %.8f\n", normal[0], normal[1], normal[2]);
    }
    for (size_t j = 0; j < ts.triangleCnt; j++)
    {
      unsigned int  v0 = ts.triangleData[j].v0 + vertexNumBase;
      unsigned int  v1 = ts.triangleData[j].v1 + vertexNumBase;
      unsigned int  v2 = ts.triangleData[j].v2 + vertexNumBase;
      std::fprintf(f, "f %u/%u/%u %u/%u/%u %u/%u/%u\n",
                   v0, v0, v0, v1, v1, v1, v2, v2, v2);
    }
    vertexNumBase = vertexNumBase + ts.vertexCnt;
    std::fprintf(f, "\n");
  }
}

static void printMTLData(std::FILE *f, const NIFFile& nifFile)
{
  ObjFileMaterialNames  matNames;
  std::string matName;
  std::vector< NIFFile::NIFTriShape > meshData;
  nifFile.getMesh(meshData);
  for (size_t i = 0; i < meshData.size(); i++)
  {
    const NIFFile::NIFTriShape& ts = meshData[i];
    if (!matNames.getMaterialName(matName, ts))
      continue;
    std::fprintf(f, "newmtl %s\n", matName.c_str());
    std::fprintf(f, "Ka 1.0 1.0 1.0\n");
    std::fprintf(f, "Kd 1.0 1.0 1.0\n");
    FloatVector4  specularColor(ts.m.s.specularColor);
    float   specularScale = specularColor[3];
    float   specularGlossiness = ts.m.s.specularSmoothness;
    if (ts.m.texturePathMask & 0x0240)
    {
      specularScale *= 0.5f;
      specularGlossiness *= 0.5f;
    }
    specularColor *= specularScale;
    specularGlossiness =
        float(std::pow(2.0f, std::min(specularGlossiness, 1.0f) * 9.0f + 1.0f));
    std::fprintf(f, "Ks %.3f %.3f %.3f\n",
                 specularColor[0], specularColor[1], specularColor[2]);
    std::fprintf(f, "d 1.0\n");
    std::fprintf(f, "Ns %.1f\n", specularGlossiness);
    if (ts.m.texturePathMask & 0x0001U)
      std::fprintf(f, "map_Kd %s\n", ts.m.texturePaths[0].c_str());
    if (ts.m.texturePathMask & 0x0002U)
      std::fprintf(f, "map_Bump %s\n", ts.m.texturePaths[1].c_str());
    if (ts.m.texturePathMask & 0x0004U)
      std::fprintf(f, "map_Ke %s\n", ts.m.texturePaths[2].c_str());
    std::fprintf(f, "\n");
  }
}

static bool archiveFilterFunction(void *p, const std::string& s)
{
  std::vector< std::string >& includePatterns =
      *(reinterpret_cast< std::vector< std::string > * >(p));
  if (includePatterns.begin() == includePatterns.end())
    return true;
  for (std::vector< std::string >::const_iterator
           i = includePatterns.begin(); i != includePatterns.end(); i++)
  {
    if ((i->length() == 4 || i->length() == 5) && (*i)[0] == '.')
    {
      if (s.ends_with(*i))
        return true;
    }
    else if (s.find(*i) != std::string::npos)
    {
      return true;
    }
  }
  return false;
}

static const char *usageStrings[] =
{
  "Usage: nif_info [OPTIONS] ARCHIVEPATH PATTERN...",
  "Options:",
  "    --      Remaining options are file names",
  "    -o FILENAME     Set output file name (default: standard output)",
  "    -q      Print author name, file name, and file size only",
  "    -v      Verbose mode, print block list, and vertex and triangle data",
  "    -m      Print detailed material information",
  "    -obj    Print model data in .obj format",
  "    -mtl    Print material data in .mtl format",
  "    -c      Enable vertex colors in .obj output",
  "    -render[WIDTHxHEIGHT] DDSFILE   Render model to DDS file",
#ifndef HAVE_SDL2
  "Render options:",
#else
  "    -view[WIDTHxHEIGHT]     View model",
  "Render and view options:",
#endif
  "    -cam SCALE DIR RX RY RZ",
  "                        set view scale and direction, and model rotation",
  "    -light SCALE RY RZ  set RGB scale and Y, Z rotation (0, 0 = top)",
  "    -env FILENAME.DDS   default environment map texture path in archives",
  "    -lcolor LMULT LCOLOR EMULT ECOLOR",
  "                        set light source, environment, and ambient light",
  "                        colors and levels (colors in 0xRRGGBB format)",
  "    -rscale FLOAT       reflection view vector Z scale",
  "    -wtxt FILENAME.DDS  water normal map texture path in archives",
  "    -wrefl FLOAT        water environment map scale",
  "    -debug INT          set debug render mode (0: disabled, 1: block IDs,",
  "                        2: depth, 3: normals, 4: diffuse, 5: light only)",
  "    -enable-markers     enable rendering markers and hidden geometry",
  nullptr
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

static FloatVector4 convertLightColor(std::uint32_t c, float l)
{
  FloatVector4  tmp(c);
  tmp.shuffleValues(0xC6).srgbExpand();
  l = ((l * -0.01728125f + 0.17087940f) * l + 0.83856107f) * l + 0.00784078f;
  l *= l;
  return (tmp * l).blendValues(FloatVector4(1.0f), 0x08);
}

int main(int argc, char **argv)
{
  std::FILE   *outFile = (std::FILE *) 0;
  const char  *outFileName = (char *) 0;
  NIF_View    *renderer = (NIF_View *) 0;
  bool    consoleFlag = true;
  try
  {
    // 0: default (block list)
    // 1: quiet (author info)
    // 2: verbose (block list and model data)
    // 3: .obj format
    // 4: .mtl format
    // 5: render to DDS file
    // 6: render and view in real time (requires SDL 2)
    int     outFmt = 0;
    int     renderWidth = 1792;
    int     renderHeight = 896;
    bool    verboseMaterialInfo = false;
    bool    enableVertexColors = false;
    // render and view options
    unsigned char debugMode = 0;
    unsigned char viewDirection = 0;
    float   viewScale = 1.0f;
    float   modelRotationX = 0.0f;
    float   modelRotationY = 0.0f;
    float   modelRotationZ = 0.0f;
    float   lightScale = 1.0f;
    float   lightRotationY = 56.25f;
    float   lightRotationZ = -135.0f;
    float   waterEnvMapLevel = 1.0f;
    float   lightLevel = 1.0f;
    std::uint32_t lightColor = 0xFFFFFFFFU;
    float   envLevel = 1.0f;
    std::uint32_t envColor = 0xFFFFFFFFU;
    const char  *defaultEnvMap = nullptr;
    const char  *waterTexture = nullptr;
    float   reflZScale = 1.0f;
    bool    enableHidden = false;
    for ( ; argc >= 2 && argv[1][0] == '-'; argc--, argv++)
    {
      if (std::strcmp(argv[1], "--") == 0)
      {
        argc--;
        argv++;
        break;
      }
      if (std::strcmp(argv[1], "-q") == 0)
      {
        outFmt = 1;
      }
      else if (std::strcmp(argv[1], "-v") == 0)
      {
        outFmt = 2;
      }
      else if (std::strcmp(argv[1], "-obj") == 0)
      {
        outFmt = 3;
      }
      else if (std::strcmp(argv[1], "-mtl") == 0)
      {
        outFmt = 4;
      }
      else if (std::strcmp(argv[1], "-c") == 0)
      {
        enableVertexColors = true;
      }
      else if (std::strncmp(argv[1], "-render", 7) == 0 ||
               std::strncmp(argv[1], "-view", 5) == 0)
      {
        outFmt = (argv[1][1] == 'r' ? 5 : 6);
        std::string tmp(argv[1] + (outFmt == 5 ? 7 : 5));
        size_t  n = tmp.find('x');
        if (n != std::string::npos)
        {
          renderHeight = int(parseInteger(tmp.c_str() + (n + 1), 10,
                                          "invalid image height", 16, 16384));
          tmp.resize(n);
          renderWidth = int(parseInteger(tmp.c_str(), 10,
                                         "invalid image width", 16, 16384));
        }
        else if (!tmp.empty())
        {
          errorMessage("invalid image dimensions");
        }
        if (outFmt == 5)
        {
          if (argc < 3)
            errorMessage("missing output file name for -render");
          outFileName = argv[2];
          argc--;
          argv++;
        }
      }
      else if (std::strcmp(argv[1], "-o") == 0)
      {
        if (argc < 3)
          errorMessage("missing output file name");
        outFileName = argv[2];
        argc--;
        argv++;
      }
      else if (std::strcmp(argv[1], "-m") == 0)
      {
        verboseMaterialInfo = true;
      }
      else if (std::strcmp(argv[1], "-cam") == 0)
      {
        if (argc < 7)
          throw FO76UtilsError("missing argument for %s", argv[1]);
        viewScale = float(parseFloat(argv[2], "invalid view scale",
                                     1.0 / 512.0, 16.0));
        viewDirection = (unsigned char) parseInteger(argv[3], 10,
                                                     "invalid view direction",
                                                     0, 19);
        modelRotationX = float(parseFloat(argv[4], "invalid model X rotation",
                                          -360.0, 360.0));
        modelRotationY = float(parseFloat(argv[5], "invalid model Y rotation",
                                          -360.0, 360.0));
        modelRotationZ = float(parseFloat(argv[6], "invalid model Z rotation",
                                          -360.0, 360.0));
        argc = argc - 5;
        argv = argv + 5;
      }
      else if (std::strcmp(argv[1], "-light") == 0)
      {
        if (argc < 5)
          throw FO76UtilsError("missing argument for %s", argv[1]);
        lightScale = float(parseFloat(argv[2], "invalid RGB scale",
                                      0.125, 4.0));
        lightRotationY = float(parseFloat(argv[3], "invalid light Y rotation",
                                          -360.0, 360.0));
        lightRotationZ = float(parseFloat(argv[4], "invalid light Z rotation",
                                          -360.0, 360.0));
        argc = argc - 3;
        argv = argv + 3;
      }
      else if (std::strcmp(argv[1], "-env") == 0)
      {
        if (argc < 3)
          throw FO76UtilsError("missing argument for %s", argv[1]);
        defaultEnvMap = argv[2];
        argc--;
        argv++;
      }
      else if (std::strcmp(argv[1], "-lcolor") == 0)
      {
        if (argc < 6)
          throw FO76UtilsError("missing argument for %s", argv[1]);
        lightLevel = float(parseFloat(argv[2], "invalid light source level",
                                      0.125, 4.0));
        lightColor = std::uint32_t(parseInteger(argv[3], 0,
                                                "invalid light source color",
                                                -1, 0x00FFFFFF) & 0x00FFFFFF);
        envLevel = float(parseFloat(argv[4], "invalid environment light level",
                                    0.125, 4.0));
        envColor = std::uint32_t(parseInteger(argv[5], 0,
                                              "invalid environment light color",
                                              -1, 0x00FFFFFF) & 0x00FFFFFF);
        argc = argc - 4;
        argv = argv + 4;
      }
      else if (std::strcmp(argv[1], "-rscale") == 0)
      {
        if (argc < 3)
          throw FO76UtilsError("missing argument for %s", argv[1]);
        reflZScale =
            float(parseFloat(argv[2], "invalid reflection view vector Z scale",
                             0.25, 16.0));
        argc--;
        argv++;
      }
      else if (std::strcmp(argv[1], "-wtxt") == 0)
      {
        if (argc < 3)
          throw FO76UtilsError("missing argument for %s", argv[1]);
        waterTexture = argv[2];
        argc--;
        argv++;
      }
      else if (std::strcmp(argv[1], "-wrefl") == 0)
      {
        if (argc < 3)
          throw FO76UtilsError("missing argument for %s", argv[1]);
        waterEnvMapLevel =
            float(parseFloat(argv[2], "invalid water environment map scale",
                             0.0, 4.0));
        argc--;
        argv++;
      }
      else if (std::strcmp(argv[1], "-debug") == 0)
      {
        if (argc < 3)
          throw FO76UtilsError("missing argument for %s", argv[1]);
        debugMode = (unsigned char) parseInteger(argv[2], 10,
                                                 "invalid debug mode", 0, 5);
        argc--;
        argv++;
      }
      else if (std::strcmp(argv[1], "-enable-markers") == 0)
      {
        enableHidden = true;
      }
      else if (std::strcmp(argv[1], "-h") == 0 ||
               std::strcmp(argv[1], "--help") == 0)
      {
        argc = 1;
        outFmt = 0;
        break;
      }
      else
      {
        throw FO76UtilsError("invalid option: %s", argv[1]);
      }
    }
    consoleFlag = false;
    if (outFmt != 6)
      SDLDisplay::enableConsole();
    if (argc < 3)
    {
      for (size_t i = 0; usageStrings[i]; i++)
        std::fprintf(stderr, "%s\n", usageStrings[i]);
      return 1;
    }
    std::vector< std::string >  fileNames;
    bool    haveMaterialPath = false;
    for (int i = 2; i < argc; i++)
    {
      fileNames.emplace_back(argv[i]);
      if (fileNames.back() == "materials" ||
          fileNames.back().starts_with("materials/") ||
          fileNames.back().ends_with(".bgsm") ||
          fileNames.back().ends_with(".bgem"))
      {
        haveMaterialPath = true;
      }
    }
    if (haveMaterialPath && outFmt == 6)
    {
      fileNames.emplace_back("meshes/preview/previewsphere01.nif");
    }
    else
    {
      fileNames.emplace_back(".bgem");
      fileNames.emplace_back(".bgsm");
    }
    if (outFmt >= 5)
      fileNames.emplace_back(".dds");
    BA2File ba2File(argv[1], &archiveFilterFunction, &fileNames);
    ba2File.getFileList(fileNames);
    if (outFmt >= 5)
    {
      renderer = new NIF_View(ba2File);
      renderer->debugMode = debugMode;
      renderer->viewRotationX = viewRotations[viewDirection * 3];
      renderer->viewRotationY = viewRotations[viewDirection * 3 + 1];
      renderer->viewRotationZ = viewRotations[viewDirection * 3 + 2];
      renderer->viewScale = viewScale;
      renderer->modelRotationX = modelRotationX;
      renderer->modelRotationY = modelRotationY;
      renderer->modelRotationZ = modelRotationZ;
      renderer->rgbScale = convertLightColor(0xFFFFFFFFU, lightScale)[0];
      renderer->lightRotationY = lightRotationY;
      renderer->lightRotationZ = lightRotationZ;
      renderer->waterEnvMapLevel = waterEnvMapLevel;
      renderer->lightColor = convertLightColor(lightColor, lightLevel);
      renderer->envColor = convertLightColor(envColor, envLevel);
      if (defaultEnvMap)
        renderer->defaultEnvMap = defaultEnvMap;
      if (waterTexture)
        renderer->waterTexture = waterTexture;
      renderer->reflZScale = reflZScale;
      renderer->enableHidden = enableHidden;
    }
    if (outFmt == 6)
    {
      for (size_t i = 0; i < fileNames.size(); )
      {
        if (fileNames[i].length() >= 5)
        {
          if (fileNames[i].ends_with(".nif") ||
              (haveMaterialPath && (fileNames[i].ends_with(".bgsm") ||
                                    fileNames[i].ends_with(".bgem"))) ||
              fileNames[i].ends_with(".btr") || fileNames[i].ends_with(".bto"))
          {
            i++;
            continue;
          }
        }
        if ((i + 1) < fileNames.size())
          fileNames[i] = fileNames.back();
        fileNames.pop_back();
      }
      if (fileNames.size() > 0)
      {
        std::sort(fileNames.begin(), fileNames.end());
        SDLDisplay  display(renderWidth, renderHeight, "nif_info", 4U, 56);
        display.setDefaultTextColor(0x00, 0xC0);
        renderer->viewModels(display, fileNames);
      }
      delete renderer;
      return 0;
    }
    if (outFileName)
    {
      outFile = std::fopen(outFileName, (outFmt != 5 ? "w" : "wb"));
      if (!outFile)
        errorMessage("error opening output file");
    }
    else
    {
      outFile = stdout;
    }
    for (size_t i = 0; i < fileNames.size(); i++)
    {
      if (fileNames[i].length() < 5)
        continue;
      const char  *suffix = fileNames[i].c_str() + (fileNames[i].length() - 4);
      if (std::strcmp(suffix, ".nif") != 0 &&
          std::strcmp(suffix, ".btr") != 0 && std::strcmp(suffix, ".bto") != 0)
      {
        continue;
      }
      if (outFmt == 0 || outFmt == 2)
        std::fprintf(outFile, "==== %s ====\n", fileNames[i].c_str());
      else if (outFmt == 3 || outFmt == 4)
        std::fprintf(outFile, "# %s\n\n", fileNames[i].c_str());
      std::vector< unsigned char >  fileBuf;
      ba2File.extractFile(fileBuf, fileNames[i]);
      if (outFmt == 1)
      {
        printAuthorName(outFile, fileBuf, fileNames[i].c_str());
        continue;
      }
      NIFFile nifFile(fileBuf.data(), fileBuf.size(), &ba2File);
      if (outFmt == 0 || outFmt == 2)
        printBlockList(outFile, nifFile, verboseMaterialInfo);
      if (outFmt == 2)
        printMeshData(outFile, nifFile);
      if (outFmt == 3)
      {
        size_t  n = fileNames[i].rfind('/');
        if (n == std::string::npos)
          n = 0;
        else
          n++;
        std::string mtlName(fileNames[i].c_str() + n);
        mtlName.resize(mtlName.length() - 3);
        mtlName += "mtl";
        printOBJData(outFile, nifFile, mtlName.c_str(), enableVertexColors);
      }
      if (outFmt == 4)
        printMTLData(outFile, nifFile);
      if (outFmt == 5)
      {
        std::fprintf(stderr, "%s\n", fileNames[i].c_str());
        renderer->loadModel(fileNames[i]);
        renderer->renderModelToFile(outFileName, renderWidth, renderHeight);
        break;
      }
    }
  }
  catch (std::exception& e)
  {
    if (consoleFlag)
      SDLDisplay::enableConsole();
    if (outFileName && outFile)
      std::fclose(outFile);
    if (renderer)
      delete renderer;
    std::fprintf(stderr, "nif_info: %s\n", e.what());
    return 1;
  }
  if (outFileName && outFile)
    std::fclose(outFile);
  if (renderer)
    delete renderer;
  return 0;
}

