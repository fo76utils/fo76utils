    mat_info [OPTIONS...] ARCHIVEPATH [FILE1.MAT [FILE2.MAT...]]

List data from a set of materials in Starfield CDB file(s), or display the materials on a test model.

### Options

* **--help**: Print usage.
* **--**: Remaining options are file names.
* **-o FILENAME**: Set output file name (default: standard output).
* **-cdb FILENAME**: Set material database file name(s), can be a comma separated list of multiple CDB files (default: materials/materialsbeta.cdb).
* **-list FILENAME**: Read list of material paths from FILENAME.
* **-view[WIDTHxHEIGHT]**: View material(s) on a test model. The usage of this mode is very similar to [nif\_info](nif_info.md), see its documentation for the list of controls. In addition to those, the 'M' key can be used to print the material data in JSON format, and copy it to the clipboard.
* **-all**: Use built-in list of all known material paths. This option also changes how the optional file name arguments after the archive path are interpreted. If any are present, then the file list is limited to paths that contain at least one of the specified patterns. Patterns beginning with -x: can also be used to exclude files.
* **-dump\_list**: Print list of .mat paths (useful with -all).
* **-dump\_db**: Dump all reflection data.
* **-json**: Write JSON format .mat file(s).

### Examples

    ./mat_info -o materials.txt -all Starfield/Data
    ./mat_info -view3840x2160 -all Starfield/Data
    ./mat_info -dump_db -o cdb_data.json Starfield/Data
    ./mat_info -json -o airpurifier01.mat Starfield/Data materials/setdressing/airpurifier/airpurifier01.mat
    ./mat_info -json -all Starfield/Data
    ./mat_info -json -all Starfield/Data weapons/ar99 -x:ammocounter
    ./mat_info -dump_list -o mat_names.txt -all Starfield/Data

