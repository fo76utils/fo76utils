    markers INFILE.ESM[,...] OUTFILE.DDS LANDFILE.DDS ICONLIST.TXT [worldID]
    markers INFILE.ESM[,...] OUTFILE.DDS VIEWTRANSFORM ICONLIST.TXT [worldID]

Find references to a set of form IDs defined in a text file, and mark their locations on an RGBA format map, optionally using DDS icon files.

**LANDFILE.DDS** is the output of [fo4land](fo4land.md) or [btddump](btddump.md), to be used as reference for image size and mapping coordinates. Alternatively, a comma separated list of image width, height, view scale, X, Y, Z rotation, and X, Y, Z offsets can be specified, these parameters are used similarly to [render](render.md).

The format of **ICONLIST.TXT** is a tab separated list of form ID, marker type, icon file, icon mip level (0.0 = full size, 1.0 = half size, etc.), and an optional priority (0-15).

Form ID is the NAME field of the reference. Marker type is the TNAM field of the reference if it is a map marker (form ID = 0x00000010), or -1 for any type of map marker. For other object types, if not 0 or -1, it can be 1 or 2 to show references from interior or exterior cells only.

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

#### Note

If markers is built with the rgb10a2=1 option, the alpha channel of the output file is limited to 2 bits of precision.

