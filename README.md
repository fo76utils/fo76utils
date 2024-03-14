# ce2utils

Simple command line utilities for extracting data from Starfield game files.

**Note:** ce2utils is work in progress, and support for older games is deprecated in these tools. For Oblivion to Fallout 76, use [fo76utils](https://github.com/fo76utils/fo76utils/) instead.

* [baunpack](doc/baunpack.md) - list the contents of, or extract from .BA2 or .BSA archives.
* [bcdecode](doc/bcdecode.md) - convert BC1 to BC5 block compressed DDS textures to uncompressed RGBA image data in raw or DDS format. BC6H and BC7 decompression are also supported, using code from [detex](https://github.com/hglm/detex). bcdecode can also be used to convert from .hdr to .dds, and to pre-filter cube maps for use in PBR.
* [btddump](doc/btddump.md) - extract terrain data from Starfield .BTD files to raw height map or land textures.
* [cubeview](doc/cubeview.md) - view cube maps or other textures from archive files.
* [esmdump](doc/esmdump.md) - list records from .ESM files in text or TSV format.
* [esmview](doc/esmview.md) - interactive version of esmdump, also includes a NIF viewer for objects with an associated model (MODL).
* [findwater](doc/findwater.md) - create a height map of water bodies, optionally using .NIF meshes.
* [landtxt](doc/landtxt.md) - create an RGB land texture from the output of btddump (formats 2 and 8), or directly from terrain data in ESM/BTD file(s), using a set of DDS texture files.
* [markers](doc/markers.md) - find references to a set of form IDs defined in a text file, and mark their locations on an RGBA format map, optionally using DDS icon files.
* [mat\_info](doc/mat_info.md) - list data from materials in Starfield component database. It can also extract .mat files in JSON format.
* [nif\_info](doc/nif_info.md) - list data from a set of .NIF files in .BA2 or .BSA archives, convert to .OBJ format, or render the model to a DDS file, or display it.
* [render](doc/render.md) - render a Starfield world, cell, or object from ESM file(s), terrain data, and archives. Currently does not support material swaps.
* [terrain](doc/terrain.md) - older and simpler program to render terrain and water only to an RGB image, using files created by btddump, or findwater. Includes 2D and isometric mode.
* [wrldview](doc/wrldview.md) - interactive version of render.

Running any of the programs without arguments prints detailed usage information.

### Installation from binary packages

Windows and Linux binaries are automatically compiled on every update to the source code, using GitHub workflows. To download these as .zip archives, select the most recent successful "Build CE2Utils" workflow run on the GitHub [Actions](https://github.com/fo76utils/ce2utils/actions) page, then either "build-linux" or "build-win" under Artifacts. Note that downloading the files requires logging in to GitHub.

#### Setting the game data path

The environment variable **CE2UTILS\_DATAPATH** can be used to set the default input data path globally for all tools. If this environment variable is set, for example to "D:/SteamLibrary/steamapps/common/Starfield/Data", then the paths Starfield/Data and Starfield/Data/Starfield.esm can be replaced in all the example commands with "" and Starfield.esm, respectively.

### Building from source code

Can be built with MSYS2 (https://www.msys2.org/) on 64-bit Windows, and also on Linux. Run "scons" to compile. mman.c and mman.h are from mman-win32 (https://github.com/alitrack/mman-win32). All source code is under the MIT license.

**Note:** when downloading the source code with 'git clone', the --recurse-submodules or --recursive option needs to be added to also download libfo76utils:

    git clone --recurse-submodules https://github.com/fo76utils/ce2utils.git

#### Building on Windows with MSYS2

* Download and run the MSYS2 installer from https://www.msys2.org/.
* In the MSYS2 MSYS terminal, run these commands to install development packages:
  *     pacman -Syu
        pacman -Su
        pacman -S --needed base-devel mingw-w64-ucrt-x86_64-toolchain
        pacman -S msys/scons
* Optionally, install SDL 2 for cubeview, wrldview, and the NIF viewer mode of esmview and nif\_info, matplotlib for plotting scripts, and SWIG for building the Python interface to libce2utils:
  *     pacman -S ucrt64/mingw-w64-ucrt-x86_64-SDL2
        pacman -S ucrt64/mingw-w64-ucrt-x86_64-python-matplotlib
        pacman -S ucrt64/mingw-w64-ucrt-x86_64-swig
* The installed MSYS2 and MinGW packages can be updated anytime by running **pacman -Syu** again.
* In the MSYS2 UCRT64 terminal, compile the utilities with **scons**. Use **scons -j 8** for building with 8 parallel jobs, and **scons -c** to clean up and delete the object files and executables.
* By default, the code generated is compatible with Intel Haswell or newer CPUs. Adding **avx=1** disables the use of instruction set extensions (including F16C, FMA and AVX2) that are not compatible with Sandy Bridge, while **avx=0** disables all instruction set extensions and compiles for a generic x86\_64 CPU.
* Running scons with the **rgb10a2=1** option compiles all tools that can render NIF files with RGB10A2 frame buffer format.
* Adding the **pymodule=1** option builds a Python interface to libce2utils under scripts.
* If Visual Studio is also installed on the system, **tools=mingw** needs to be added to the scons options.
* Optionally, for the makemap and icon extraction scripts only, download and install [ImageMagick](https://imagemagick.org/script/download.php#windows) and [SWFTools](http://www.swftools.org/download.html).

#### Example 1

    ./btddump terrain/newatlantis.btd newatlantis_hmap.dds 0 Starfield/Data
    ./btddump terrain/newatlantis.btd newatlantis_norm.dds 1 Starfield/Data
    ./btddump terrain/newatlantis.btd newatlantis_ltex.dds 2 Starfield/Data
    ./btddump terrain/newatlantis.btd newatlantis_txts.dds 8 Starfield/Data
    export CE2UTILS_DATAPATH="./Starfield/Data"
    ./btddump terrain/newatlantis.btd newatlantis_hmap.dds 0 ""

#### Example 2

    ./baunpack Starfield/Data -- textures/cubemaps/

#### Example 3

    ./esmdump Fallout4/Data/Fallout4.esm -u | python3 scripts/esmvcdisp.py -h 69 60 184 -

#### Example 4

    ./esmview Starfield/Data/Starfield.esm Starfield/Data

#### Example 5

    ./nif_info -obj -o airpurifier01.obj Starfield/Data meshes/setdressing/airpurifier01.nif
    ./nif_info -view1920x1080 Starfield/Data meshes/
    ./cubeview 3200 1800 Starfield/Data textures/

#### Example 6

    ./render Starfield/Data/Starfield.esm newatlantis.dds 4096 4096 Starfield/Data -w 0x0001251B -ltxtres 512 -light 1.0 45 90 -lcolor 1.5 -1 1.0 -1 -1 -rq 0x2F -cam 5 54.7356 180 135 -1238.59 -1164.737 1428.076 -ssaa 2

#### Example 7

    ./wrldview Starfield/Data/Starfield.esm 3200 1800 Starfield/Data -w 0x0001251B -light 1.0 45 90 -lcolor 1.5 -1 1.0 -1 -1 -rq 0x2F -minscale 1 -cam 2.828427 1 -1188.59 -1014.737 1428.076

