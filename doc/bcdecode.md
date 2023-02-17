    bcdecode INFILE.DDS [MULT]

Print average red, green, and blue levels of a DDS texture, optionally multiplied by MULT.

    bcdecode INFILE.DDS OUTFILE.DATA [FACE [FLAGS]]
    bcdecode INFILE.DDS OUTFILE.RGBA [FACE [FLAGS]]

Decode DDS texture to raw RGBA data. For cube maps, FACE (0 to 5) selects the face to be extracted, it should be 0 for other textures. If FLAGS & 1 is set, the alpha channel of the input file is ignored. Setting FLAGS & 2 enables calculating the missing blue channel of Fallout 4 and 76 normal maps.

    bcdecode INFILE.DDS OUTFILE.DDS [FACE [FLAGS]]

Decode DDS texture to an uncompressed DDS file.

