    landtxt INFILE.DDS OUTFILE.DDS TXTSET.DDS LTEXLIST.TXT [OPTIONS...]
    landtxt INFILE.ESM[,...] OUTFILE.DDS [OPTIONS...]

### General options

* **--**: Remaining options are file names.
* **-d PATHNAME**: Game data path or texture archive file.
* **-mip FLOAT**: Texture mip level.
* **-mult FLOAT**: Multiply RGB output with the specified value.
* **-defclr 0x00RRGGBB**: Default color for untextured areas.
* **-scale INT**: Scale output resolution by 2^N.
* **-threads INT**: Set the number of threads to use.
* **-ssaa BOOL**: Render at double resolution and downsample.
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

