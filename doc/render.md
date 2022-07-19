    render INFILE.ESM[,...] OUTFILE.DDS W H ARCHIVEPATH [OPTIONS...]

### General options

* **--help**: Print usage.
* **--list-defaults**: Print defaults for all options, and exit.
* **--**: Remaining options are file names.
* **-q**: Do not print messages other than errors.
* **-threads INT**: Set the number of threads to use.
* **-debug INT**: Set debug render mode (0: disabled, 1: form IDs, 2: depth, 3: normals, 4: diffuse, 5: light only).
* **-ssaa BOOL**: Render at double resolution and downsample.
* **-w FORMID**: Form ID of world, cell, or object to render.

### Texture options

* **-textures BOOL**: Make all diffuse textures white if false.
* **-txtcache INT**: Texture cache size in megabytes.
* **-mip INT**: Base mip level for all textures.
* **-env FILENAME.DDS**: Default environment map texture path in archives.

### Terrain options

* **-btd FILENAME.BTD**: Read terrain data from Fallout 76 .btd file.
* **-r X0 Y0 X1 Y1**: Terrain cell range, X0,Y0 = SW to X1,Y1 = NE.
* **-l INT**: Level of detail to use from BTD file (0 to 4).
* **-deftxt FORMID**: Form ID of default land texture.
* **-defclr 0x00RRGGBB**: Default color for untextured terrain.
* **-lmip FLOAT**: Additional mip level for land textures.
* **-lmult FLOAT**: Land texture RGB level scale.
* **-ltxtres INT**: Land texture resolution per cell.

### Model options

* **-scol BOOL**: Enable the use of pre-combined meshes.
* **-a**: Render all object types.
* **-mlod INT**: Set level of detail for models, 0 (best) to 4.
* **-vis BOOL**: Render only objects visible from distance.
* **-ndis BOOL**: Do not render initially disabled objects.
* **-hqm STRING**: Add high quality model path name pattern.
* **-xm STRING**: Add excluded model path name pattern.

### View options

* **-view SCALE RX RY RZ OFFS_X OFFS_Y OFFS_Z**: Set transform from world coordinates to image coordinates, rotations are in degrees.
* **-cam SCALE RX RY RZ X Y Z**: Set view transform for camera position X,Y,Z.
* **-zrange MIN MAX**: Limit view Z range to MIN <= Z < MAX.

### Lighting options

* **-light SCALE RX RY**: Set light level and X, Y rotation (0, 0 = top).
* **-lpoly A5 A4 A3 A2 A1 A0**: Set lighting polynomial, -1 <= x <= 1.

### Water options

* **-wtxt FILENAME.DDS**: Water normal map texture path in archives.
* **-watercolor UINT32**: Water color (A7R8G8B8), 0 disables water.
* **-wrefl FLOAT**: Water environment map scale.

### Examples

