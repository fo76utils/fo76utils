    esmview FILENAME.ESM[,...] [ARCHIVEPATH [STRINGS_PREFIX]] [OPTIONS...]

Interactive version of [esmdump](esmdump.md). On Windows, if compiled without SDL, it is recommended to run esmview in the MSYS2 terminal for correct support of ANSI escape sequences, and for easy copying and pasting of form IDs with the mouse (double click to copy and middle button to paste).

### Options

* **-h**: Print usage.
* **--**: Remaining options are file names.
* **-F FILE**: Read field definitions from FILE.
* **-w COLUMNS,ROWS,FONT_HEIGHT,DOWNSAMPLE,BGCOLOR,FGCOLOR**: Set SDL console dimensions and colors, not available if esmview is built without SDL support. The defaults are 96,36,18,1,0xE6,0x00, any parameter that is omitted uses the default value. Font height is in pixels, the font is currently Courier in bitmap format with a native resolution of 20x30 per character. Colors use the 8-bit encoding described [here](https://en.wikipedia.org/wiki/ANSI_escape_code#8-bit).

### Commands

* **$EDID**: Select record by editor ID.
* **0xxxxxx**: Hexadecimal form ID of record to be shown.
* **#xxxxxx** or **#"xxxxxx"**: Decimal form ID of record to be shown.
* **C**: First child record.
* **N**: Next record in current group.
* **P**: Parent group.
* **V**: Previous record in current group.
* **L**: Back to previously shown record.

C, N, P, and V can be grouped and used as a single command.

* **B**: Print history of records viewed.
* **I**: Print short info and all parent groups.
* **T**: Toggle display of REFL type definitions.
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
* **S**: Select record interactively (requires SDL).
* **W [MOLM...]**: View the model (MODL) of the current record, if present. This command is not available if esmview is built without SDL support. Pressing H while in the NIF viewer prints the list of keyboard controls, Esc returns to the esmview console. The optional MOLM parameter can be used to add material swap form ID(s).
* **Ctrl-A**: Copy the contents of the text buffer to the clipboard (SDL only).
* **Q** or **Ctrl-D**: Quit.

##### Mouse controls

* Double click: copy word.
* Triple click: copy line.
* Middle button: paste.
* Right button: copy and paste word.

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

