    findwater INFILE.ESM[,...] OUTFILE.DDS [HMAPFILE.DDS [ARCHIVEPATH [worldID]]]

Create a height map of water bodies, optionally using .NIF meshes for Skyrim and newer games.

### Options

* **INFILE.ESM**: ESM input file, or a comma separated pair of base game + DLC files.
* **OUTFILE.DDS**: Output file name.
* **HMAPFILE.DDS**: Height map input file, see [btddump](btddump.md) and [fo4land](fo4land.md). Used as a reference for output resolution and X,Y,Z range. If not present, the default is for Appalachia at maximum detail, 25728x25728 resolution and a Z range of -700 to 38210.
* **ARCHIVEPATH**: Path to water mesh archive(s). If not specified, or with games older than Skyrim, water is rendered based on the object bounds (OBND) in the ESM file.
* **worldID**: Form ID of world. Defaults to 0x0025DA15 (Appalachia), or 0x0000003C if HMAPFILE.DDS is the output of fo4land.

