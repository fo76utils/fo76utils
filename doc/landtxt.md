    landtxt INFILE.DDS OUTFILE.DDS TXTSET.DDS LTEXLIST.TXT [OPTIONS...]
    landtxt INFILE.ESM[,...] OUTFILE.DDS [OPTIONS...]

Create an RGB land texture from the output of [btddump](btddump.md) (formats 2, 4, 6, and 8), or directly from terrain data in ESM/BTD file(s), using a set of DDS texture files.

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

### ESM/BTD input file options

* **-btd FILENAME.BTD**: Read terrain data from Starfield .btd file.
* **-cdb FILENAME.CDB**: Set material database file name.
* **-w FORMID**: Form ID of world to use from ESM input file.
* **-r X0 Y0 X1 Y1**: Limit range of cells to X0,Y0 (SW) to X1,Y1 (NE).
* **-l INT**: Level of detail to use from BTD file (0 to 4).

### Examples

##### Using game files

    ./landtxt Starfield/Data/Starfield.esm sf_ltx2.dds -d Starfield/Data -r -7 -7 6 6 -l 1 -mip 6 -w 0x0001251B -ssaa 1

