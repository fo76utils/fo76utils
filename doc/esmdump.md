    esmdump FILENAME.ESM[,...] [LOCALIZATION.BA2 [STRINGS_PREFIX]] [OPTIONS...]

### Options

* **-h**: Print usage.
* **--**: Remaining options are file names.
* **-o FN**: Set output file name to FN.
* **-F FILE**: Read field definitions from FILE.
* **-f 0xN**: Only include records with flags&N != 0.
* **-i TYPE**: Include groups and records of type TYPE.
* **-e 0xN**: Exclude records with flags&N != 0.
* **-x TYPE**: Exclude groups and records of type TYPE.
* **-d TYPE**: Exclude fields of type TYPE.
* **-n**: Do not print record and field stats.
* **-s**: Only print record and field stats.
* **-edid**: Print EDIDs of form ID fields.
* **-t**: TSV format output.
* **-u**: Print TSV format version control info.
* **-v**: Verbose mode.

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

