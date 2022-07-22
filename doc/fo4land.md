    fo4land INFILE.ESM[,...] OUTFILE.DDS [[-d ARCHIVEPATH] FMT [XMIN YMIN XMAX YMAX] [worldID [defTxtID]]]

Extract landscape data from Oblivion, Fallout 3/New Vegas, Skyrim, or Fallout 4 [LAND](https://en.uesp.net/wiki/Oblivion_Mod:Mod_File_Format/LAND) records.

### Output formats

* **FMT = 0**: Height map (16-bit grayscale).
* **FMT = 1**: Vertex normals (24-bit RGB).
* **FMT = 2**: Raw land textures. The pixel format is 4 bytes per vertex, defining 4-bit alphas for 8 additional texture layers (the bottom layer in bits 0 to 3, and the top layer in bits 28 to 31).
* **FMT = 3**: Land textures (8-bit dithered).
* **FMT = 4**: Vertex colors (24-bit RGB).
* **FMT = 5**: Cell texture sets as 8-bit IDs. A table of form IDs corresponding to each of these is printed to standard output. The texture set of each cell quadrant is 16x1 pixels on the output image, containing the ID of the base texture, 8 additional texture layers (bottom to top), and 7 unused pixels.
* **FMT = 6**: Cell texture sets as 32-bit form IDs. Similar to format 5, but with 4-byte pixels containing the form IDs of LTEX records.

### Options

* **INFILE.ESM**: ESM input file, can be a comma separated pair of file names for base game + DLC.
* **OUTFILE.DDS**: Output file name.
* **-d ARCHIVEPATH**: Path to archive file or directory to read material information from.
* **XMIN YMIN XMAX YMAX**: Range of cells to extract, from SW (min) to NE (max). The default is to use all available terrain data, and world dimensions are printed on standard output.
* **worldID**: Form ID of world to extract the landscape from, defaults to 0x0000003C.
* **defTxtID**: Form ID of LTEX record to be used as default texture.

For game and DLC specific values of **worldID** and **defTxtID**, see the table in [SConstruct.maps](../SConstruct.maps).

