    baunpack ARCHIVES...

List the contents of .ba2 or .bsa files, or archive directories.

    baunpack ARCHIVES... --list [PATTERNS...]
    baunpack ARCHIVES... --list @LISTFILE

List archive contents, filtered by name patterns or list file. Using --list-packed instead of --list prints compressed file sizes.

    baunpack ARCHIVES... -- [PATTERNS...]

Extract files with a name including any of the patterns, archives listed first have higher priority. Empty pattern list or "\*" extracts all files, patterns beginning with -x: exclude files.

    baunpack ARCHIVES... -- @LISTFILE

Extract all valid file names specified in the list file. Names are separated by tabs or new lines, any string not including at least one /, \\, or . character is ignored.

