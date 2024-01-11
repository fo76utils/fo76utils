    baunpack ARCHIVES...

List the contents of .ba2 or .bsa files, or archive directories.

    baunpack ARCHIVES... --list [PATTERNS...]
    baunpack ARCHIVES... --list @LISTFILE

List archive contents, filtered by name patterns or list file. Using --list-packed instead of --list prints compressed file sizes.

    baunpack ARCHIVES... -- [PATTERNS...]

Extract files with a name including any of the patterns, archives listed first have higher priority. Empty pattern list or "\*" extracts all files, patterns beginning with -x: exclude files. Patterns that are 4 or 5 characters long and begin with '.' (e.g. .nif or .mesh) are interpreted as extensions, and are checked for only at the end of each file name, while other patterns can be matched anywhere in the file name.

    baunpack ARCHIVES... -- @LISTFILE

Extract all valid file names specified in the list file. Names are separated by tabs or new lines, any string not including at least one /, \\, or . character is ignored.

### Examples

    ./baunpack Fallout76/Data -- textures/interface/pip-boy/ textures/interface/season/
    ./baunpack Fallout4/Data --list textures/shared/cubemaps/
    ./baunpack Skyrim/Data --list-packed meshes/terrain/tamriel/

