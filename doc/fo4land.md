    fo4land INFILE.ESM[,...] OUTFILE.DDS [[-d ARCHIVEPATH] FMT [XMIN YMIN XMAX YMAX] [worldID [defTxtID]]]

* **FMT = 0**: Height map (16-bit grayscale).
* **FMT = 1**: Vertex normals (24-bit RGB).
* **FMT = 2**: Raw land textures (4 bytes per vertex).
* **FMT = 3**: Land textures (8-bit dithered).
* **FMT = 4**: Vertex colors (24-bit RGB).
* **FMT = 5**: Cell texture sets as 8-bit IDs.
* **FMT = 6**: Cell texture sets as 32-bit form IDs.

**worldID** defaults to 0x0000003C, use 0x000DA726 for Fallout: New Vegas.

