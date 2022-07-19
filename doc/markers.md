    markers INFILE.ESM[,...] OUTFILE.DDS LANDFILE.DDS ICONLIST.TXT [worldID]
    markers INFILE.ESM[,...] OUTFILE.DDS VIEWTRANSFORM ICONLIST.TXT [worldID]

LANDFILE.DDS is the output of fo4land or btddump, to be used as reference for image size and mapping coordinates. Alternatively, a comma separated list of image width, height, view scale, X, Y, Z rotation, and X, Y, Z offsets can be specified, these parameters are used similarly to render.

The format of ICONLIST.TXT is a tab separated list of form ID, marker type, icon file, icon mip level (0.0 = full size, 1.0 = half size, etc.), and an optional priority (0-15).

Form ID is the NAME field of the reference. Marker type is the TNAM field of the reference if it is a map marker (form ID = 0x00000010), or -1 for any type of map marker. For other object types, if not 0 or -1, it can be 1 or 2 to show references from interior or exterior cells only.

The icon file must be either a DDS texture in a supported format, or a 32-bit integer defining a basic shape and color for a 256x256 icon in 0xTARRGGBB format. A is the opacity, T is the type as shape + edges:

* shape: 1: circle, 5: square, 9: diamond
* edges: 0: soft, 1: outline only, 2: hard, 3: filled black/white outline

Any invalid type defaults to TA = 0x4F.

