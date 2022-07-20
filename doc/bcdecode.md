    bcdecode INFILE.DDS [MULT]

Print average red, green, and blue levels of a DDS texture, optionally multiplied by MULT.

    bcdecode INFILE.DDS OUTFILE.DATA [FACE [NOALPHA]]
    bcdecode INFILE.DDS OUTFILE.RGBA [FACE [NOALPHA]]

Decode DDS texture to raw RGBA data. For cube maps, FACE (0 to 5) selects the face to be extracted. If NOALPHA is 1, the alpha channel of the input file is ignored.

    bcdecode INFILE.DDS OUTFILE.DDS [FACE [NOALPHA]]

Decode DDS texture to an uncompressed DDS file.

