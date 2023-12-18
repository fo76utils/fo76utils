    mat_info [OPTIONS...] ARCHIVEPATH [FILE1.MAT [FILE2.MAT...]]

List data from a set of materials in Starfield CDB file(s), or display the materials on a test model.

### Options

* **--help**: Print usage.
* **--**: Remaining options are file names.
* **-o FILENAME**: Set output file name (default: standard output).
* **-cdb FILENAME**: Set material database file name(s), can be a comma separated list of multiple CDB files (default: materials/materialsbeta.cdb).
* **-list FILENAME**: Read list of material paths from FILENAME.
* **-view[WIDTHxHEIGHT]**: View material(s) on a test model. The usage of this mode is identical to [nif\_info](nif_info.md), see its documentation for the complete list of controls.
* **-dump\_db**: Dump all reflection data.
* **-json**: Write JSON format .mat file(s).

### Examples

    ./mat_info -o materials.txt -list mat_names.txt Starfield/Data
    ./mat_info -view3840x2160 -list mat_names.txt Starfield/Data
    ./mat_info -dump_db -o cdb_data.txt Starfield/Data
    ./mat_info -json -o airpurifier01.mat Starfield/Data materials/setdressing/airpurifier/airpurifier01.mat
    ./mat_info -json -list mat_names.txt Starfield/Data

