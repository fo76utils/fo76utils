# fo76utils

Simple command line utilities for extracting data from Elder Scrolls and Fallout game files, from Oblivion to Fallout 76.

* [baunpack](doc/baunpack.md) - list the contents of, or extract from .BA2 or .BSA archives.
* [bcdecode](doc/bcdecode.md) - convert BC1 to BC5 block compressed DDS textures to uncompressed RGBA image data in raw or DDS format.
* [btddump](doc/btddump.md) - extract terrain data from Fallout 76 .BTD files to raw height map, land textures, ground cover, or terrain color.
* [cubeview](doc/cubeview.md) - view a cube map from archive files. Does not work correctly with games older than Fallout 4.
* [esmdump](doc/esmdump.md) - list records from .ESM files in text or TSV format.
* [esmview](doc/esmview.md) - interactive version of esmdump.
* [findwater](doc/findwater.md) - create a height map of water bodies, optionally using .NIF meshes for Skyrim and newer games.
* [fo4land](doc/fo4land.md) - extract landscape data from Oblivion, Fallout 3, Skyrim, and Fallout 4.
* [landtxt](doc/landtxt.md) - create an RGB land texture from the output of btddump (formats 2, 4, 6, and 8) or fo4land (formats 2, 4, and 5), or directly from terrain data in ESM/BTD file(s), using a set of DDS texture files.
* [markers](doc/markers.md) - find references to a set of form IDs defined in a text file, and mark their locations on an RGBA format map, optionally using DDS icon files.
* [nif\_info](doc/nif_info.md) - list data from a set of .NIF files in .BA2 or .BSA archives, convert to .OBJ format, or render the model to a DDS file, or display it.
* [render](doc/render.md) - render a world, cell, or object from ESM file(s), terrain data, and archives.
* [terrain](doc/terrain.md) - older and simpler program to render terrain and water only to an RGB image, using files created by btddump, findwater, or fo4land. Includes 2D and isometric mode.

Can be built with MSYS2 (https://www.msys2.org/) on 64-bit Windows, and also on Linux. Run "scons" to compile. mman.c and mman.h are from mman-win32 (https://github.com/alitrack/mman-win32). All source code is under the MIT license.

Running any of the programs without arguments prints detailed usage information.

### Building from source code on Windows

* Download and run the MSYS2 installer from https://www.msys2.org/.
* In the MSYS2 MSYS terminal, run these commands to install development packages:
  *     pacman -Syu
        pacman -Su
        pacman -S --needed base-devel mingw-w64-x86_64-toolchain
        pacman -S msys/scons
* Optionally, for nif\_view and plotting scripts only, install SDL 2 and matplotlib:
  *     pacman -S mingw64/mingw-w64-x86_64-SDL2
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

#### Example 6

    ./nif_info -render1920x1080 glassdome.dds Fallout76/Data palaceofthewindingpath/palace_bld_glassdome01.nif
    ./nif_info -render1920x1080 44.dds Fallout4/Data weapons/44/44load.nif
    ./nif_info -view1920x1080 Fallout76/Data meshes/test/testpbrmaterials01.nif
    ./cubeview 1280 720 Fallout76/Data textures/shared/cubemaps/outsideoldtownreflectcube_e.dds

#### Example 7

    ./render Fallout76/Data/SeventySix.esm whitespring.dds 4096 4096 Fallout76/Data -btd Fallout76/Data/Terrain/Appalachia.btd -l 0 -r -32 -32 32 32 -cam 0.125 54.7356 180 -135 53340 -99681 74002.25 -light 1.7 70.5288 135 -a -ssaa 1 -hqm meshes -env textures/shared/cubemaps/mipblur_defaultoutside1.dds -wtxt textures/water/defaultwater.dds -ltxtres 512
    ./markers Fallout76/Data/SeventySix.esm fo76mmap.dds 4096,4096,0.125,54.7356,180,-135,4096,-256,16384 fo76icondefs.txt
    ./render Fallout76/Data/SeventySix.esm watoga.dds 9024 9024 Fallout76/Data -btd Fallout76/Data/Terrain/Appalachia.btd -l 0 -r 0 -71 71 0 -cam 0.25 180 0 0 146368.4 -141504.4 65536 -light 1.7 70.5288 135 -a -ssaa 1 -hqm meshes -env textures/shared/cubemaps/mipblur_defaultoutside1.dds -wtxt textures/water/defaultwater.dds -ltxtres 2048 -mip 1 -lmip 2

