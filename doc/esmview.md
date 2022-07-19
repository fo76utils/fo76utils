    esmview FILENAME.ESM[,...] [LOCALIZATION.BA2 [STRINGS_PREFIX]] [OPTIONS...]

### Options

* **-h**: Print usage.
* **--**: Remaining options are file names.
* **-F FILE**: Read field definitions from FILE.

### Commands

* **0xxxxxx**: Hexadecimal form ID of record to be shown.
* **C**: First child record.
* **N**: Next record in current group.
* **P**: Parent group.
* **V**: Previous record in current group.
* **L**: Back to previously shown record.

C, N, P, and V can be grouped and used as a single command.

* **B**: Print history of records viewed.
* **I**: Print short info and all parent groups.
* **U**: Toggle hexadecimal display of unknown field types.
* **D r:f:name:data**: Define field(s).
* **F xx xx xx xx**: Convert binary floating point value(s).
* **G cccc**: Find next top level group of record type cccc.
* **G d:xxxxxxxx**: Find group of type d with form ID label.
* **G d:d**: Find group of type 2 or 3 with int32 label.
* **G d:d,d**: Find group of type 4 or 5 with X,Y label.
* **R cccc:xxxxxxxx**: Find next reference to form ID in field cccc.
* **R \*:xxxxxxxx**: Find next reference to form ID.
* **S pattern**: Find next record with EDID matching pattern.
* **Q**: Quit.

### Field definition line format

    RECORD[,...]\tFIELD[,...]\tNAME\tDATATYPES

After #, ;, or //, the rest of the line is ignored. Data types include:

* **<**: Remaining data is an array, and can be repeated any number of times.
* **b**: Boolean.
* **c**: 8-bit unsigned integer (hexadecimal).
* **h**: 16-bit unsigned integer (hexadecimal).
* **s**: 16-bit signed integer.
* **x**: 32-bit unsigned integer (hexadecimal).
* **u**: 32-bit unsigned integer.
* **i**: 32-bit signed integer.
* **d**: Form ID.
* **f**: 32-bit float.
* **l**: Localized string.
* **z**: String.
* **n**: File name.
* **.**: Ignore byte.
* **\***: Ignore any remaining data.

