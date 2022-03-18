# fo76utils

Simple command line utilities for extracting data from Elder Scrolls and Fallout game files, from Oblivion to Fallout 76.

* baunpack - list the contents of, or extract from .BA2 or .BSA archives.
* bcdecode - convert BC1 to BC5 block compressed DDS textures to headerless RGBA image data.
* btddump - extract terrain data from Fallout 76 .BTD files to raw height map, land textures, ground cover, or terrain color.
* esmdump - list records from .ESM files in text or TSV format.
* esmview - interactive version of esmdump.
* findwater - create a height map of water bodies, optionally using .NIF meshes for Skyrim and newer games.
* fo4land - extract landscape data from Oblivion, Fallout 3, Skyrim, and Fallout 4.
* landtxt - create an RGB land texture from the output of btddump (formats 2, 4, 6, and 8) or fo4land (formats 2, 4, and 5), or directly from terrain data in ESM/BTD file(s), using a set of DDS texture files.
* markers - find references to a set of form IDs defined in a text file, and mark their locations on an RGBA format map, optionally using DDS icon files.
* nif\_info - list data from a set of .NIF files in .BA2 or .BSA archives.
* terrain - render terrain to an RGB image, using files created by btddump, findwater, or fo4land. Includes 2D and isometric mode.

Can be built with MSYS2 (https://www.msys2.org/) on 64-bit Windows, and also on Linux. Run "scons" to compile. mman.c and mman.h are from mman-win32 (https://github.com/alitrack/mman-win32). All source code is under the MIT license.

Running any of the programs without arguments prints detailed usage information.

### Building from source code on Windows

* Download and run the MSYS2 installer from https://www.msys2.org/.
* In the MSYS2 MSYS terminal, run these commands to install development packages:
  *     pacman -Syu
        pacman -Su
        pacman -S --needed base-devel mingw-w64-x86_64-toolchain
        pacman -S msys/scons
* Optionally, for plotting scripts only, install matplotlib:
  *     pacman -S mingw64/mingw-w64-x86_64-python-matplotlib
* In the MSYS2 MinGW x64 terminal, compile the utilities with **scons**. Use **scons -j 8** for building with 8 parallel jobs, and **scons -c** to clean up and delete the object files and executables.
* Optionally, for the makemap and icon extraction scripts only, download and install [ImageMagick](https://imagemagick.org/script/download.php#windows) and [SWFTools](http://www.swftools.org/download.html).

#### Example 1

    ./btddump Fallout76/Data/Terrain/Appalachia.btd fo76hmap.dds 0 -80 -80 80 80 1
    ./btddump Fallout76/Data/Terrain/Appalachia.btd fo76ltex.dds 2 -80 -80 80 80 1
    ./btddump Fallout76/Data/Terrain/Appalachia.btd fo76gcvr.dds 4 -80 -80 80 80 1
    ./btddump Fallout76/Data/Terrain/Appalachia.btd fo76vclr.dds 6 -80 -80 80 80 2
    ./btddump Fallout76/Data/Terrain/Appalachia.btd fo76txts.dds 8 -80 -80 80 80
    ./findwater Fallout76/Data/SeventySix.esm fo76wmap.dds fo76hmap.dds Fallout76/Data
    ./landtxt fo76ltex.dds fo76ltx2.dds fo76txts.dds ltex/fo76.txt -d Fallout76/Data -gcvr fo76gcvr.dds -vclr fo76vclr.dds -mip 6
    ./terrain fo76hmap.dds fo76terr.dds 9216 6144 -ltex fo76ltx2.dds -light 150 100 35 -wmap fo76wmap.dds -watercolor 0x600A1A22

##### Example 1 without extracting raw land texture data

    ./btddump Fallout76/Data/Terrain/Appalachia.btd fo76hmap.dds 0 -80 -80 80 80 1
    ./findwater Fallout76/Data/SeventySix.esm fo76wmap.dds fo76hmap.dds Fallout76/Data
    ./landtxt Fallout76/Data/SeventySix.esm fo76ltx2.dds -d Fallout76/Data -btd Fallout76/Data/Terrain/Appalachia.btd -r -80 -80 80 80 -l 1 -mip 6
    ./terrain fo76hmap.dds fo76terr.dds 9216 6144 -ltex fo76ltx2.dds -light 150 100 35 -wmap fo76wmap.dds -watercolor 0x600A1A22

#### Example 2

    ./baunpack Fallout76/Data -- textures/interface/season/season

#### Example 3

    ./esmdump Fallout4/Data/Fallout4.esm -u | python3 scripts/esmvcdisp.py -h 69 60 184 -

#### Example 4

    ./esmview Skyrim/Data/Skyrim.esm Skyrim/Data skyrim_english -F tes5cell.txt

#### Example 5

    # list options
    scons -f SConstruct.maps game=tes5 listvars=1
    # build Skyrim map
    scons -f SConstruct.maps game=tes5
    # extract the map icons only
    scons -f SConstruct.maps game=tes5 interface/tes5icon_02.dds

