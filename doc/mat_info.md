    mat_info [OPTIONS...] ARCHIVEPATH [FILE1.MAT [FILE2.MAT...]]

List data from a set of materials in Starfield CDB file(s), or display the materials on a test model.

### Options

* **--help**: Print usage.
* **--**: Remaining options are file names.
* **-o FILENAME**: Set output file name (default: standard output).
* **-cdb FILENAME**: Set material database file name(s), can be a comma separated list of multiple CDB files (default: materials/materialsbeta.cdb).
* **-list FILENAME**: Read list of material paths from FILENAME. [mat\_names.txt](https://github.com/fo76utils/ce2utils/blob/main/mat_names.txt) contains an extensive list of materials used in the game.
* **-view[WIDTHxHEIGHT]**: View material(s) on a test model. The usage of this mode is identical to [nif\_info](nif_info.md), see its documentation for the complete list of controls.

### Examples

    ./mat_info -o materials.txt -list mat_names.txt Starfield/Data
    ./mat_info -view3840x2160 -list mat_names.txt Starfield/Data

