    terrain HMAPFILE.DDS OUTFILE.DDS W H [OPTIONS...]

Render height map to an RGB image, using files extracted with [btddump](btddump.md), [landtxt](landtxt.md), and [findwater](findwater.md). [render](render.md) is a newer and more advanced tool that can be used for the same purpose, however, **terrain** is faster and accepts image files as input.

### Options

* **HMAPFILE.DDS**: Height map input file.
* **OUTFILE.DDS**: Output file name.
* **W**: Output image width.
* **H**: Output image height.
* **-iso[_se|_sw|_nw|_ne]**: Isometric view, optionally from the specified direction. The default is SE.
* **-2d**: 2D (top) view.
* **-ltex FILENAME**: Land texture file, typically the output of [landtxt](landtxt.md). Can have higher resolution than the height map by power of two factors, for use with a **-lod** setting below zero.
* **-ltxpal FILENAME**: Land texture palette file, only needed when using an 8-bit texture file.
* **-lod INT**: Level of detail, the height map is scaled by a factor of 2<sup>-lod</sup>.
* **-xoffs INT**: X offset on the height map.
* **-yoffs INT**: Y offset on the height map.
* **-zrange INT**: ZMAX - ZMIN, multiplied by 4 for Fallout 76. Height map files created with btddump include this information in the header.
* **-waterlevel UINT16**: Water level as 16-bit normalized height value. Not needed when using a water map file, or a height map image created with btddump that includes the water level in the header.
* **-watercolor UINT32_A7R8G8B8**: Water color, defaults to 0x60004080.
* **-wmap FILENAME**: Water height map file name (optional). Can be created with [findwater](findwater.md).
* **-light[_nw|_w|_sw|_s|_se|_e|_ne|_n] LMULTx100 ZMULTx100 POWx100**: Light direction, level, and curve parameters. The default is NW 88 100 35.
* **-threads INT**: Number of threads to use, defaults to the number of CPU logical cores.

