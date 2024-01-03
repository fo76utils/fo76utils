    bcdecode INFILE.DDS [MULT]

Print average red, green, and blue levels of a DDS texture, optionally multiplied by MULT.

    bcdecode INFILE.DDS OUTFILE.DATA [FACE [FLAGS]]
    bcdecode INFILE.DDS OUTFILE.RGBA [FACE [FLAGS]]

Decode DDS texture to raw RGBA data. For cube maps, FACE (0 to 5) selects the face to be extracted, it should be 0 for other textures. If FLAGS & 1 is set, the alpha channel of the input file is ignored. Setting FLAGS & 2 enables calculating the missing blue channel of Fallout 4, 76 and Starfield normal maps.

    bcdecode INFILE.DDS OUTFILE.DDS [FACE [FLAGS]]

Decode DDS texture to an uncompressed DDS file.

    bcdecode INFILE.DDS OUTFILE.DDS -cube_filter [WIDTH]

Pre-filter cube map for PBR with roughness = 0.0, 0.103, 0.220, 0.358, 0.535, 0.857 and 1.0 at mip levels 0 to 6, respectively. The optional width parameter sets the dimensions of the output image, and defaults to 256. It must be at least 128, and not greater than the size of the input image. Note that large values of WIDTH result in very long processing (the time is a function of WIDTH<sup>4</sup>). The output image is in R9G9B9E5\_SHAREDEXP format.

    bcdecode INFILE.HDR OUTFILE.DDS -cube [WIDTH [MAXLEVEL]]

Convert Radiance .hdr format image to DDS cube map in R16G16B16A16\_FLOAT format, without filtering or generating mipmaps. WIDTH specifies the resolution of the output image, which defaults to 2048x2048 per face. A negative value rotates the image by 180 degrees. MAXLEVEL can be used to limit the range of output colors, the default is 65504.

