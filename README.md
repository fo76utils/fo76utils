# fo76utils

Simple command line utilities for extracting data from Fallout 4 and 76 files.

* baunpack - list the contents of, or extract from .BA2 archives. Also includes limited support for .BSA files.
* btddump - extract terrain data from Fallout 76 .BTD files to raw height map, land textures, or ground cover.
* esmdump - list records from Skyrim, Fallout 4, and Fallout 76 .ESM files.
* esmview - interactive version of esmdump.
* findwater - create a (currently not accurate) height map of water bodies in Fallout 76.
* fo4land - extract landscape data from Fallout 3, Skyrim, and Fallout 4.
* landtxt - create an RGB land texture from the output of btddump or fo4land formats 2 and 4, and a set of DDS files.
* markers - find references to a set of form IDs defined in a text file, and mark their locations on an RGBA format map, optionally using DDS icon files.
* terrain - render terrain to an RGB image, using files created by btddump, findwater, or fo4land. Includes 2D and isometric mode.

Can be built with MSYS2 (https://www.msys2.org/) on 64-bit Windows, and also on Linux. Run "scons" to compile. mman.c and mman.h are from mman-win32 (https://github.com/alitrack/mman-win32). All source code is under the MIT license.

### Example

    baunpack Fallout76/Data/*.ba2 -- @ltex/fo76.txt
    btddump Fallout76/Data/Terrain/Appalachia.btd fo76hmap.dds 0 -80 -80 80 80 1
    btddump Fallout76/Data/Terrain/Appalachia.btd fo76ltex.dds 2 -80 -80 80 80 1
    btddump Fallout76/Data/Terrain/Appalachia.btd fo76gcvr.dds 4 -80 -80 80 80 1
    findwater Fallout76/Data/SeventySix.esm fo76wmap.dds fo76hmap.dds
    landtxt fo76ltex.dds fo76ltx2.dds ltex/fo76.txt -gcvr fo76gcvr.dds -mip 6
    terrain fo76hmap.dds fo76terr.dds 9216 6144 -ltex fo76ltx2.dds -light 150 100 35 -wmap fo76wmap.dds

