    bcdecode INFILE.DDS [MULT]

Print average red, green, and blue levels of a DDS texture, optionally multiplied by MULT.

    bcdecode INFILE.DDS OUTFILE.DATA [FACE [FLAGS]]
    bcdecode INFILE.DDS OUTFILE.RGBA [FACE [FLAGS]]

Decode DDS texture to raw RGBA data. For cube maps, FACE (0 to 5) selects the face to be extracted, it should be 0 for other textures. If FLAGS & 1 is set, the alpha channel of the input file is ignored. Setting FLAGS & 2 enables calculating the missing blue channel of Fallout 4, 76 and Starfield normal maps.

    bcdecode INFILE.DDS OUTFILE.DDS [FACE [FLAGS]]

Decode DDS texture to an uncompressed DDS file.

    bcdecode INFILE.DDS OUTFILE.DDS -cube_filter [WIDTH [ROUGHNESS...]]

Pre-filter cube map for PBR. The optional width parameter sets the dimensions of the output image, and defaults to 256. Note that large values of WIDTH result in very long processing (the time is a function of WIDTH<sup>4</sup>). The output image is in R9G9B9E5\_SHAREDEXP format. If no roughness parameters are specified, the default is 0.0 0.104 0.219 0.349 0.500 0.691 1.0 at mip levels 0 to 6.

    bcdecode INFILE.HDR OUTFILE.DDS -cube [WIDTH [MAXLEVEL [DXGI_FMT]]]

Convert Radiance .hdr format image to DDS cube map, without filtering or generating mipmaps.

WIDTH specifies the resolution of the output image, which defaults to 2048x2048 per face. A negative value inverts the Z axis.

MAXLEVEL can be used to limit the range of output colors, the default is 65504. MAXLEVEL < 0 enables simple Reinhard tone mapping: Cout = MAXLEVEL \* Cin / (MAXLEVEL - Cin).

DXGI\_FMT sets the output format, the allowed values are 0x0A (R16G16B16A16\_FLOAT, default) and 0x43 (R9G9B9E5\_SHAREDEXP).

### Examples

#### Decode DDS texture to raw RGBA data

    bcdecode texture.dds texture.data

#### Convert DDS texture to uncompressed DDS

    bcdecode texture1.dds texture2.dds

#### Extract all faces of a cube map to separate images

    bcdecode cell_cityplazacube.dds face0.dds 0 1
    bcdecode cell_cityplazacube.dds face1.dds 1 1
    bcdecode cell_cityplazacube.dds face2.dds 2 1
    bcdecode cell_cityplazacube.dds face3.dds 3 1
    bcdecode cell_cityplazacube.dds face4.dds 4 1
    bcdecode cell_cityplazacube.dds face5.dds 5 1

#### Calculate the missing blue channel of a normal map in BC5\_SNORM format

    bcdecode texture_normal.dds texture2_normal.dds 0 2

#### Convert HDR format image to cube map, and pre-filter it for PBR

    bcdecode studio_small_02_4k.hdr studio_small_02_cube.dds -cube 2048 -5
    bcdecode studio_small_02_cube.dds studio_small_02_cube.dds -cube_filter 512

