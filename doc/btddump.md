    btddump INFILE.BTD OUTFILE.DDS FMT XMIN YMIN XMAX YMAX [LOD [PALETTE.GPL]]

Extract terrain data from [Fallout 76 .BTD files](../src/btdfile.cpp) to raw height map, land textures, ground cover, or terrain color.

### Output formats

* **FMT = 0**: Raw 16-bit height map.
* **FMT = 1**: Vertex normals in 24-bit RGB format.
* **FMT = 2**: Raw landscape textures. The pixel format is 2 bytes per vertex, defining 3-bit alphas for 5 additional texture layers (the bottom layer in bits 0 to 2, and the top layer in bits 12 to 14).
* **FMT = 3**: Dithered 8-bit landscape texture.
* **FMT = 4**: Raw ground cover. The pixel format is 1 byte per vertex, each bit defines whether the corresponding ground cover layer (0 = bottom, 7 = top) is enabled on the vertex.
* **FMT = 5**: Dithered 8-bit ground cover.
* **FMT = 6**: Raw 16-bit terrain color (5 bits per channel). Resolution is limited to LOD >= 2, or at most 32x32 per cell.
* **FMT = 7**: Terrain color in 24-bit RGB format.
* **FMT = 8**: Cell texture sets as 8-bit IDs. A table of form IDs corresponding to each of these is printed to standard output. The texture set of each cell quadrant is 16x1 pixels on the output image, containing the ID of the base texture, 5 additional texture layers (bottom to top), 2 unused pixels, and 8 ground cover IDs (bottom to top layers).
* **FMT = 9**: Cell texture sets as 32-bit form IDs. Similar to format 8, but with 4-byte pixels containing the form IDs of LTEX and GCVR records.

Using a palette file changes formats 3 and 5 from dithered 8-bit to RGB.

### Options

* **XMIN**: X coordinate of the cell in the SW corner.
* **YMIN**: Y coordinate of the cell in the SW corner.
* **XMAX**: X coordinate of the cell in the NE corner.
* **YMAX**: Y coordinate of the cell in the NE corner.

To extract all data from Appalachia.btd, use a range of -100 -100 100 100 (201x201 cells). The paper map in the game, including its borders, is limited to -70 -70 70 70 (141x141 cells). Terrain cells are offset so that the origin of the coordinate system is in the center of cell 0, 0, rather than in its SW corner.

* **LOD**: Level of detail from 0 to 4. Not supported by texture set output formats, and limited to a minimum of 2 for vertex colors (formats 6 and 7). Cell resolution is 128x128 at level 0, and 8x8 at level 4.

* **PALETTE.GPL**: Optional The GIMP palette file for output formats 3 and 5.

