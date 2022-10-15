    landtxt INFILE.DDS OUTFILE.DDS TXTSET.DDS LTEXLIST.TXT [OPTIONS...]
    landtxt INFILE.ESM[,...] OUTFILE.DDS [OPTIONS...]

Create an RGB land texture from the output of [btddump](btddump.md) (formats 2, 4, 6, and 8) or [fo4land](fo4land.md) (formats 2, 4, and 5), or directly from terrain data in ESM/BTD file(s), using a set of DDS texture files.

### General options

* **--**: Remaining options are file names.
* **-d PATHNAME**: Game data path or texture archive file.
* **-mip FLOAT**: Texture mip level.
* **-mult FLOAT**: Multiply RGB output with the specified value.
* **-defclr 0x00RRGGBB**: Default color for untextured areas.
* **-scale INT**: Scale output resolution by 2^N.
* **-threads INT**: Set the number of threads to use.
* **-ssaa INT**: Render at 2<sup>N</sup> (double or quadruple) resolution and downsample.
* **-q**: Do not print texture file names.

### DDS input file options

* **-vclr FILENAME.DDS**: Vertex color file name.
* **-gcvr FILENAME.DDS**: Ground cover file name.

### ESM/BTD input file options

* **-btd FILENAME.BTD**: Read terrain data from Fallout 76 .btd file.
* **-w FORMID**: Form ID of world to use from ESM input file.
* **-r X0 Y0 X1 Y1**: Limit range of cells to X0,Y0 (SW) to X1,Y1 (NE).
* **-l INT**: Level of detail to use from BTD file (0 to 4).
* **-deftxt FORMID**: Form ID of default texture.
* **-no-vclr**: Do not use vertex color data.
* **-gcvr**: Use ground cover data.

### Examples

##### Using image input files

    ./baunpack Fallout76/Data -- @ltex/fo76.txt
    ./btddump Fallout76/Data/Terrain/Appalachia.btd fo76ltex.dds 2 -80 -80 80 80 1
    ./btddump Fallout76/Data/Terrain/Appalachia.btd fo76vclr.dds 6 -80 -80 80 80 2
    ./btddump Fallout76/Data/Terrain/Appalachia.btd fo76txts.dds 8 -80 -80 80 80
    ./landtxt fo76ltex.dds fo76ltx2.dds fo76txts.dds ltex/fo76.txt -vclr fo76vclr.dds -mip 6

##### Using game files

    ./landtxt Fallout76/Data/SeventySix.esm fo76ltx2.dds -d Fallout76/Data -btd Fallout76/Data/Terrain/Appalachia.btd -r -80 -80 80 80 -l 1 -mip 6

