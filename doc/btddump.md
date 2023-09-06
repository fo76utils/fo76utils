    btddump INFILE.BTD OUTFILE.DDS FMT XMIN YMIN XMAX YMAX [LOD [PALETTE.GPL]]

Extract terrain data from [Starfield .BTD files](../src/btdfile.cpp) to raw height map or land textures.

### Output formats

* **FMT = 0**: Raw 16-bit height map.
* **FMT = 1**: Vertex normals calculated from the height map, in 24-bit RGB format.
* **FMT = 2**: Raw landscape textures. The pixel format is 2 bytes per vertex, defining 3-bit alphas for 5 additional texture layers (the top layer in bits 0 to 2, and the bottom layer in bits 12 to 14).
* **FMT = 3**: Dithered 8-bit landscape texture.
* **FMT = 8**: Cell texture sets as 8-bit IDs. A table of form IDs corresponding to each of these is printed to standard output. The texture set of each cell quadrant is 16x1 pixels on the output image, containing the IDs of 6 additional layers (top to bottom) and the base texture.
* **FMT = 9**: Cell texture sets as 32-bit form IDs. Similar to format 8, but with 4-byte pixels containing the form IDs of LTEX records.

Using a palette file changes format 3 from dithered 8-bit to RGB.

### Options

* **XMIN**: X coordinate of the cell in the SW corner.
* **YMIN**: Y coordinate of the cell in the SW corner.
* **XMAX**: X coordinate of the cell in the NE corner.
* **YMAX**: Y coordinate of the cell in the NE corner.

To extract all data, use a large range like -1000 -1000 1000 1000.

* **LOD**: Level of detail from 0 to 4. Not supported by texture set output formats. Cell resolution is 128x128 at level 0, and 8x8 at level 4.

* **PALETTE.GPL**: Optional The GIMP palette file for output formats 3 and 5.

