    esmdump FILENAME.ESM[,...] [LOCALIZATION.BA2 [STRINGS_PREFIX]] [OPTIONS...]

List records from ESM file(s) in text or TSV format.

* **FILENAME.ESM\[,...\]**: Path to ESM file, can be a comma separated pair of file names for base game + DLC.
* **LOCALIZATION.BA2**: Archive file or directory containing localization files. Only used by Skyrim and newer games.
* **STRINGS_PREFIX**: Name prefix of localization files to load, defaults to **strings/starfield_en** for Starfield in English. Use **baunpack ARCHIVEPATH --list .ilstrings** to print the list of available localization files, for the Skyrim and Fallout 4 base game in English, the prefix should be **skyrim_english** and **fallout4_en**, respectively.

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

#### Note

The Fallout: New Vegas base game ESM includes a LAND record (form ID = 0x00150FC0, the parent cell is 0x0014F962) with invalid compressed data that causes an error when running esmdump. Adding the option **-x LAND** avoids this error.

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

### Examples

    ./esmdump Starfield/Data/Starfield.esm Starfield/Data > starfield.txt
    ./esmdump Fallout4/Data/Fallout4.esm -u -edid -o fallout4vc.tsv
    ./esmdump Fallout4/Data/Fallout4.esm -u | python3 scripts/esmvcdisp.py -h 69 48 183 -

