
#include "common.hpp"
#include "filebuf.hpp"
#include "esmfile.hpp"
#include "markers.hpp"

static const char *usageStrings[] =
{
  "Usage:",
  "    markers INFILE.ESM[,...] OUTFILE.DDS LANDFILE.DDS ICONLIST.TXT "
  "[worldID]",
  "    markers INFILE.ESM[,...] OUTFILE.DDS VIEWTRANSFORM ICONLIST.TXT "
  "[worldID]",
  "",
  "LANDFILE.DDS is the output of btddump, to be used as reference for image",
  "size and mapping coordinates. Alternatively, a comma separated list of",
  "image width, height, view scale, X, Y, Z rotation, and X, Y, Z offsets",
  "can be specified, these parameters are used similarly to render.",
  "The view transform may optionally also include a mip offset parameter,",
  "this is added to all mip levels in the marker definitions.",
  "",
  "The format of ICONLIST.TXT is a tab separated list of form ID, marker",
  "type, icon file, icon mip level (0.0 = full size, 1.0 = half size, etc.),",
  "and an optional priority (0-15).",
  "",
  "Form ID is the NAME field of the reference. Marker type is the TNAM field",
  "of the reference if it is a map marker (form ID = 0x00000010), or -1 for",
  "any type of map marker. For other object types, if not -1, it is a flags",
  "mask to filter references. Setting bit 0 or bit 1 disables exterior or",
  "interior objects, respectively. If any of bits 2 to 15 is set both in the",
  "mask and in the flags of the reference (for example, 0x0020 for deleted",
  "records), the marker is not shown.",
  "",
  "The icon file must be either a DDS texture in a supported format, or a",
  "32-bit integer defining a basic shape and color for a 256x256 icon in",
  "0xTARRGGBB format. A is the opacity, T is the type as shape + edges:",
  "  shape: 1: circle, 5: square, 9: diamond",
  "  edges: 0: soft, 1: outline only, 2: hard, 3: filled black/white outline",
  "Any invalid type defaults to TA = 0x4F.",
  "",
  "Simple macros can be defined in NAME = VALUE format, which also must be",
  "tab separated. VALUE can expand to multiple fields, and reference",
  "previously defined macros. Macro expansion is not supported on the first",
  "(form ID) field.",
  "",
  "The directives 'include' and 'mipoffset' can be used to read another",
  "marker definition file and to set a value to be added to all subsequent",
  "mip levels, respectively. These take a single tab separated argument,",
  "either a file name without quotes, or a floating point value between",
  "-16.0 and 16.0. If the mip offset is set again within the same file, the",
  "values are combined, but the result is limited to the allowed range.",
  "Mip offsets set in an included file are local to that file.",
  (char *) 0
};

int main(int argc, char **argv)
{
  if (argc < 5 || argc > 6)
  {
    for (size_t i = 0; usageStrings[i]; i++)
      std::fprintf(stderr, "%s\n", usageStrings[i]);
    return 1;
  }
  try
  {
    int     imageWidth = 0;
    int     imageHeight = 0;
    float   viewScale = 1.0f;
    float   viewRotationX = float(std::atan(1.0) * 4.0);
    float   viewRotationY = 0.0f;
    float   viewRotationZ = 0.0f;
    float   viewOffsX = 0.0f;
    float   viewOffsY = 0.0f;
    float   viewOffsZ = 0.0f;
    float   mipOffset = 0.0f;
    {
      const char  *s = argv[3];
      size_t  n = std::strlen(s);
      if ((n >= 5 && (FileBuffer::readUInt32Fast(s + (n - 4)) | 0x20202000U)
                     == 0x7364642EU) ||         // ".dds"
          !std::strchr(s, ','))
      {
        unsigned int  hdrBuf[11];
        int     pixelFormat = 0;
        DDSInputFile  landFile(s, imageWidth, imageHeight, pixelFormat, hdrBuf);
        // "FO", "LAND"
        if ((hdrBuf[0] & 0xFFFF) != 0x4F46 || hdrBuf[1] != 0x444E414C)
          errorMessage("invalid landscape image file");
        int     xMin = uint32ToSigned(hdrBuf[2]);
        int     yMax = uint32ToSigned(hdrBuf[6]);
        int     cellSize = int(hdrBuf[9]);
        bool    isFO76 = (hdrBuf[0] == 0x36374F46);
        int     x0 = xMin * 4096;
        int     y1 = (yMax + 1) * 4096;
        if (isFO76)
        {
          x0 = x0 - 2048;
          y1 = y1 - 2048;
        }
        viewScale = float(cellSize) / 4096.0f;
        viewOffsX = float(-x0) * viewScale;
        viewOffsY = float(y1) * viewScale - 1.0f;
        viewOffsZ = viewScale * 262144.0f;
      }
      else
      {
        for (int i = 0; i < 10; i++)
        {
          long    tmp1 = 0L;
          double  tmp2 = 0.0;
          char    *endp = (char *) 0;
          if (i < 2)
            tmp1 = std::strtol(s, &endp, 0);
          else
            tmp2 = std::strtod(s, &endp);
          if (!endp || endp == s || (i < 8 && *endp != ',') ||
              (i > 8 && *endp != '\0') || (*endp != '\0' && *endp != ','))
          {
            errorMessage("invalid view transform, "
                         "must be 9 or 10 comma separated numbers");
          }
          if (i < 2 && (tmp1 < 2L || tmp1 > 65536L))
            errorMessage("invalid image dimensions");
          if (i == 2 && !(tmp2 >= (1.0 / 512.0) && tmp2 <= 16.0))
            errorMessage("invalid view scale");
          if (i >= 3 && i < 6)
          {
            if (!(tmp2 >= -360.0 && tmp2 <= 360.0))
              errorMessage("invalid view rotation");
            tmp2 = tmp2 * (std::atan(1.0) / 45.0);
          }
          if (i >= 6 && !(tmp2 >= -1048576.0 && tmp2 <= 1048576.0))
            errorMessage("invalid view offset");
          if (i == 9 && !(tmp2 >= -16.0 && tmp2 <= 16.0))
            errorMessage("invalid mip offset");
          if (i == 0)
            imageWidth = int(tmp1);
          else if (i == 1)
            imageHeight = int(tmp1);
          else if (i == 2)
            viewScale = float(tmp2);
          else if (i == 3)
            viewRotationX = float(tmp2);
          else if (i == 4)
            viewRotationY = float(tmp2);
          else if (i == 5)
            viewRotationZ = float(tmp2);
          else if (i == 6)
            viewOffsX = float(tmp2) + (float(imageWidth) * 0.5f);
          else if (i == 7)
            viewOffsY = float(tmp2) + (float(imageHeight - 2) * 0.5f);
          else if (i == 8)
            viewOffsZ = float(tmp2);
          else if (i == 9)
            mipOffset = float(tmp2);
          if (*endp == '\0')
            break;
          s = endp + 1;
        }
      }
    }

    std::vector< std::uint32_t >
        outBuf(size_t(imageWidth) * size_t(imageHeight), 0U);
    ESMFile   esmFile(argv[1]);
    MapImage  mapImage(esmFile, argv[4], outBuf.data(), imageWidth, imageHeight,
                       NIFFile::NIFVertexTransform(
                           viewScale,
                           viewRotationX, viewRotationY, viewRotationZ,
                           viewOffsX, viewOffsY, viewOffsZ), mipOffset);
    unsigned int  worldID = 0U;
    if (argc > 5)
    {
      worldID = (unsigned int) parseInteger(argv[5], 0, "invalid world form ID",
                                            0L, 0x0FFFFFFFL);
    }
    if (!worldID)
      worldID = (esmFile.getESMVersion() < 0xC0U ? 0x0000003C : 0x0001251B);
    mapImage.findMarkers(worldID);

    DDSOutputFile outFile(argv[2], imageWidth, imageHeight,
                          DDSInputFile::pixelFormatRGBA32);
    outFile.writeImageData(outBuf.data(), outBuf.size(),
                           DDSInputFile::pixelFormatRGBA32);
    outFile.flush();
  }
  catch (std::exception& e)
  {
    std::fprintf(stderr, "markers: %s\n", e.what());
    return 1;
  }
  return 0;
}

