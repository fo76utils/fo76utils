# fo76utils

A collection of command line tools for rendering Fallout 76 and Fallout 4 worldspaces, and for extracting and visualizing data from Elder Scrolls and Fallout game files (Oblivion to Fallout 76, and limited support for Starfield).

* [render](doc/render.md) - render a world, cell, or object from ESM file(s), terrain data, and archives. Supports Fallout 76 and Fallout 4, and partly the older games (TES4/FO3/FNV are limited to terrain and water only).
* [wrldview](doc/wrldview.md) - interactive version of render.

Running any of the programs without arguments prints usage information. Detailed documentation is available [here](doc/main.md).

### Installation from binary packages

Windows and Linux binaries are automatically compiled on every update to the source code, using GitHub workflows. To download these as .zip archives, select the most recent successful "Build FO76Utils" workflow run on the GitHub [Actions](https://github.com/fo76utils/fo76utils/actions) page, then either "build-linux" or "build-win" under Artifacts. Note that downloading the files requires logging in to GitHub.

**Note:** the source code can be built on macOS, see [detailed instructions](doc/main.md#building-the-source-code-on-macos).

#### Setting the game data path

The environment variable **FO76UTILS\_DATAPATH** can be used to set the default input data path globally for all tools. If this environment variable is set, for example to "D:/SteamLibrary/steamapps/common/Fallout76/Data", then the paths Fallout76/Data and Fallout76/Data/SeventySix.esm can be replaced in all the example commands with "" and SeventySix.esm, respectively.

#### Examples

    ./render Fallout76/Data/SeventySix.esm whitespring.dds 4096 4096 Fallout76/Data -r -32 -32 32 32 -cam 0.125 54.7356 180 -135 53340 -99681 74002.25 -light 1.7 70.5288 135 -lcolor 1 0xFFFCF0 0.875 -1 -1 -ssaa 1 -rq 0x2F -ltxtres 512
    ./render Fallout76/Data/SeventySix.esm watoga.dds 9024 9024 Fallout76/Data -r 0 -71 71 0 -cam 0.25 180 0 0 146368.4 -141504.4 65536 -light 1.7 70.5288 135 -ssaa 1 -rq 0x2F -ltxtres 2048 -mip 1 -lmip 2
    ./wrldview Fallout76/Data/SeventySix.esm 2304 1296 Fallout76/Data -r -71 -71 71 71 -rq 10 -ltxtres 1024 -lcolor 1 -1 0.875 -1 -1 -light 1.6 70.5288 135 -cam 0.125 1 -29332.19 79373.61 115577.4

