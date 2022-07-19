    btddump INFILE.BTD OUTFILE FMT XMIN YMIN XMAX YMAX [LOD [PALETTE.GPL]]

* FMT = 0: raw 16-bit height map
* FMT = 1: vertex normals in 24-bit RGB format
* FMT = 2: raw landscape textures (2 bytes / vertex)
* FMT = 3: dithered 8-bit landscape texture
* FMT = 4: raw ground cover (1 byte / vertex)
* FMT = 5: dithered 8-bit ground cover
* FMT = 6: raw 16-bit terrain color
* FMT = 7: terrain color in 24-bit RGB format
* FMT = 8: cell texture sets as 8-bit IDs
* FMT = 9: cell texture sets as 32-bit form IDs

Using a palette file changes formats 3 and 5 from dithered 8-bit to RGB.

