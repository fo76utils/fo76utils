    markers INFILE.ESM[,...] OUTFILE.DDS LANDFILE.DDS ICONLIST.TXT [worldID]
    markers INFILE.ESM[,...] OUTFILE.DDS VIEWTRANSFORM ICONLIST.TXT [worldID]

Find references to a set of form IDs defined in a text file, and mark their locations on an RGBA format map, optionally using DDS icon files.

**LANDFILE.DDS** is the output of [btddump](btddump.md), to be used as reference for image size and mapping coordinates. Alternatively, a comma separated list of image width, height, view scale, X, Y, Z rotation, and X, Y, Z offsets can be specified, these parameters are used similarly to [render](render.md). The view transform may optionally also include a mip offset parameter, this is added to all mip levels in the marker definitions.

The format of **ICONLIST.TXT** is a tab separated list of form ID, marker type, icon file, icon mip level (0.0 = full size, 1.0 = half size, etc.), and an optional priority (0-15).

Form ID is the NAME field of the reference. Marker type is the TNAM field of the reference if it is a map marker (form ID = 0x00000010), or -1 for any type of map marker. For other object types, if not -1, it is a flags mask to filter references. Setting bit 0 or bit 1 disables exterior or interior objects, respectively. If any of bits 2 to 15 is set both in the mask and in the flags of the reference (for example, 0x0020 for deleted records), the marker is not shown.

The icon file must be either a DDS texture in a supported format, or a 32-bit integer defining a basic shape and color for a 256x256 icon in 0xTARRGGBB format. A is the opacity (logarithmic scale from 1/32 to 1.0), T is the type as shape + edges:

* **1**: Circle (soft).
* **2**: Circle, outline only.
* **3**: Circle (hard).
* **4**: Circle with black/white outline.
* **5**: Square (soft).
* **6**: Square, outline only.
* **7**: Square (hard).
* **8**: Square with black/white outline.
* **9**: Diamond (soft).
* **A**: Diamond, outline only.
* **B**: Diamond (hard).
* **C**: Diamond with black/white outline.

Any invalid type defaults to TA = 0x4F.

Simple macros can be defined in NAME = VALUE format, which also must be tab separated. VALUE can expand to multiple fields, and reference previously defined macros. Macro expansion is not supported on the first (form ID) field.

The directives **include** and **mipoffset** can be used to read another marker definition file and to set a value to be added to all subsequent mip levels, respectively. These take a single tab separated argument, either a file name without quotes, or a floating point value between -16.0 and 16.0. If the mip offset is set again within the same file, the values are combined, but the result is limited to the allowed range. Mip offsets set in an included file are local to that file.

#### Note

If markers is built with the rgb10a2=1 option, the alpha channel of the output file is limited to 2 bits of precision.

