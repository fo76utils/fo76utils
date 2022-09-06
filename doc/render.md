    render INFILE.ESM[,...] OUTFILE.DDS W H ARCHIVEPATH [OPTIONS...]

Render a world, cell, or object from ESM file(s), terrain data, and archives.

### General options

* **--help**: Print usage.
* **--list-defaults**: Print defaults for all options, and exit. If used after other options, then the updated values are printed, including the rotation matrix and light vector calculated from **-view** and **-light**.
* **--**: Remaining options are file names.
* **-q**: Do not print messages other than errors.
* **-threads INT**: Set the number of threads to use.
* **-debug INT**: Set debug render mode (0: disabled, 1: reference form IDs as 0xRRGGBB, 2: depth \* 16, 3: normals, 4: diffuse texture only, 5: light only).
* **-ssaa BOOL**: Render at double resolution and downsample.
* **-w FORMID**: Form ID of world, cell, or object to render. A table of game and DLC world form IDs can be found in [SConstruct.maps](../SConstruct.maps).

### Texture options

* **-textures BOOL**: Make all diffuse textures white if false.
* **-txtcache INT**: Texture cache size in megabytes.
* **-mip INT**: Base mip level for all textures other than cube maps. Defaults to 2.
* **-env FILENAME.DDS**: Default environment map texture path in archives. Use **baunpack ARCHIVEPATH --list /cubemaps/** to print the list of available cube map textures, and [cubeview](cubeview.md) to preview them. The cube map is expected to be in sRGB color space for Fallout 76, and linear for the older games.

### Terrain options

* **-btd FILENAME.BTD**: Read terrain data from Fallout 76 .btd file.
* **-r X0 Y0 X1 Y1**: Terrain cell range, X0,Y0 = SW to X1,Y1 = NE. The default is to use all available terrain, limiting the range can be useful for Fallout 76 to reduce the load time and memory usage.
* **-l INT**: Level of detail to use from BTD file (0 to 4), see [btddump](btddump.md). Fallout 76 defaults to 0, all other games use a fixed level of 2.
* **-deftxt FORMID**: Form ID of default land texture. See the table in [SConstruct.maps](../SConstruct.maps) for game specific values.
* **-defclr 0x00RRGGBB**: Default color for untextured terrain.
* **-lmip FLOAT**: Additional mip level for land textures, defaults to 3.0.
* **-lmult FLOAT**: Land texture RGB level scale.
* **-ltxtres INT**: Land texture resolution per cell, must be power of two, and in the range 2<sup>(7-l)</sup> to 4096. Using a value greater than 2<sup>(7-l)</sup> enables normal mapping on terrain. For approximately correct scaling, use 16384 / 2<sup>(mip+lmip)</sup> for Fallout 4 and 76, and 8192 / 2<sup>(mip+lmip)</sup> for Skyrim.

### Model options

* **-scol BOOL**: Enable the use of pre-combined meshes. This works around some problems related to material swaps, but is generally slower, and may have other issues.
* **-a**: Render all object types.
* **-mlod INT**: Set level of detail for models, 0 (default and best) to 4.
* **-vis BOOL**: Render only objects visible from distance.
* **-ndis BOOL**: If zero, also render initially disabled objects.
* **-hqm STRING**: Add high quality model path name pattern. Meshes that match the pattern are always rendered at the highest level of detail, with normal mapping and reflections enabled. Using **meshes** as the pattern matches all models.
* **-xm STRING**: Add excluded model path name pattern. **-xm meshes** disables all solid objects. Use **-xm babylon** to disable Nuclear Winter objects in Fallout 76.

### View options

* **-view SCALE RX RY RZ OFFS_X OFFS_Y OFFS_Z**: Set transform from world coordinates to image coordinates, rotations are in degrees (see [figure](view.png)). At a scale of 1.0, the size of one exterior cell is 4096 pixels. X and Y offsets are relative to the center of the image.
* **-cam SCALE RX RY RZ X Y Z**: Set view transform for camera position X,Y,Z. The scale and rotation parameters are identical to **-view**, offsets are calculated from the specified camera world coordinates.
* **-zrange MIN MAX**: Limit view Z range to MIN <= Z < MAX.

### Lighting options

* **-light SCALE RY RZ**: Set overall brightness, and light vector Y, Z rotation in degrees (0, 0 = top). The light direction is rotated around the Y axis first (positive = east, negative = west), then around the Z axis (positive = counter-clockwise, from east towards north). The default direction of RY=70.5288 and RZ=135 translates to a light vector of -2/3, 2/3, 1/3.
* **-lcolor LMULT LCOLOR EMULT ECOLOR ACOLOR**: Set light source, environment, and ambient light colors and levels. LCOLOR, ECOLOR, and ACOLOR are in 0xRRGGBB format (sRGB color space), -1 is interpreted as white for LCOLOR and ECOLOR. LMULT and LCOLOR multiply diffuse and specular lighting, while EMULT and ECOLOR are applied to environment maps and also to ambient light. If ACOLOR is negative, the ambient light color is determined from the default cube map specified with **-env**, if one is available, otherwise it defaults to 0x404040.

For testing view and light settings, it is recommended to use faster low quality rendering, with no downsampling, low landscape texture resolution, no high quality models, and if necessary, disabling all meshes.

### Water options

* **-wtxt FILENAME.DDS**: Water normal map texture path in archives.
* **-watercolor UINT32**: Water color (A7R8G8B8), 0 disables water, 0x7FFFFFFF (the default) uses values from the ESM file. The alpha channel determines the depth at which water reaches maximum opacity, maxDepth = (128 - a) \* (128 - a) / 2.
* **-wrefl FLOAT**: Water environment map scale, the default is 1.0.

### Examples

    ./render Fallout76/Data/SeventySix.esm fo76_map_4k.dds 4096 4096 Fallout76/Data -btd Fallout76/Data/Terrain/Appalachia.btd -l 0 -r -71 -71 71 71 -light 1.7 70.5288 135 -env textures/shared/cubemaps/mipblur_defaultoutside1.dds -wtxt textures/water/defaultwater.dds -ltxtres 512 -a -hqm meshes -ssaa 1 -xm swamptree -view 0.0070922 180 0 0 0 0 4096
    ./render Fallout76/Data/SeventySix.esm fo76_map_8k.dds 8192 8192 Fallout76/Data -btd Fallout76/Data/Terrain/Appalachia.btd -l 0 -r -71 -71 71 71 -light 1.7 70.5288 135 -env textures/shared/cubemaps/mipblur_defaultoutside1.dds -wtxt textures/water/defaultwater.dds -ltxtres 512 -a -hqm meshes -ssaa 1 -xm swamptree -view 0.0141844 180 0 0 0 0 8192
    ./render Fallout76/Data/SeventySix.esm fo76_map_16k.dds 16384 16384 Fallout76/Data -btd Fallout76/Data/Terrain/Appalachia.btd -l 0 -r -71 -71 71 71 -light 1.7 70.5288 135 -env textures/shared/cubemaps/mipblur_defaultoutside1.dds -wtxt textures/water/defaultwater.dds -ltxtres 512 -a -hqm meshes -ssaa 1 -xm swamptree -view 0.0283688 180 0 0 0 0 16384
    ./render Fallout4/Data/Fallout4.esm,Fallout4/Data/DLCNukaWorld.esm fo4_map.dds 8192 8192 Fallout4/Data -deftxt 0x000AB07E -env textures/shared/cubemaps/mipblur_defaultoutside1.dds -wtxt textures/water/defaultwater.dds -light 1.5 70.5288 135 -lcolor 1.0 -1 0.5 -1 -1 -ltxtres 512 -view 0.03064 180 0 0 316.1 -680.1 16384 -a -hqm meshes -ssaa 1
    ./render Fallout4/Data/Fallout4.esm,Fallout4/Data/DLCCoast.esm fo4fh_map.dds 8192 8192 Fallout4/Data -deftxt 0x000AB07E -w 0x01000B0F -env textures/shared/cubemaps/mipblur_defaultoutside1.dds -wtxt textures/water/defaultwater.dds -light 1.5 70.5288 135 -lcolor 1.0 -1 0.5 -1 -1 -ltxtres 512 -view 0.05482 180 0 0 -444.1 -20.1 16384 -a -hqm meshes -ssaa 1
    ./render Fallout4/Data/Fallout4.esm,Fallout4/Data/DLCNukaWorld.esm fo4nw_map.dds 8192 8192 Fallout4/Data -deftxt 0x000AB07E -w 0x0100290F -env textures/shared/cubemaps/mipblur_defaultoutside1.dds -wtxt textures/water/defaultwater.dds -light 1.5 70.5288 135 -lcolor 1.0 -1 0.5 -1 -1 -ltxtres 512 -view 0.06857 180 0 0 444.1 520.1 16384 -a -hqm meshes -scol 1 -ssaa 1
    ./render Skyrim/Data/Skyrim.esm,Skyrim/Data/Dawnguard.esm tes5_map.dds 8192 6471 Skyrim/Data -deftxt 0x00000C16 -env textures/cubemaps/chrome_e.dds -wtxt textures/water/defaultwater.dds -light 2.5 70.5288 135 -lcolor 1.0 -1 0.75 -1 -1 -ltxtres 256 -view 0.0168067 180 0 0 -173 274.5 8192 -a -hqm meshes -ssaa 1
    ./render Skyrim/Data/Skyrim.esm,Skyrim/Data/Dragonborn.esm tes5db_map.dds 8192 8192 Skyrim/Data -deftxt 0x00000C16 -w 0x02000800 -env textures/cubemaps/chrome_e.dds -wtxt textures/water/defaultwater.dds -light 2.5 70.5288 135 -lcolor 1.0 -1 0.75 -1 -1 -ltxtres 256 -view 0.0714286 180 0 0 -3776.1 3584.1 8192 -a -hqm meshes -ssaa 1

