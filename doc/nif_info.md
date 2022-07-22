    nif_info [OPTION] ARCHIVEPATH PATTERN...
    nif_view -view[WIDTHxHEIGHT] ARCHIVEPATH PATTERN...

List data from a set of .NIF files in .BA2 or .BSA archives, convert to .OBJ format, or render the model to a DDS file, or display it (nif\_view on Windows).

### Options

* **-q**: Print author name, file name, and file size only.
* **-v**: Verbose mode, print block list, and vertex and triangle data.
* **-obj**: Print model data in .obj format.
* **-mtl**: Print material data in .mtl format.
* **-render[WIDTHxHEIGHT] DDSFILE**: Render model to DDS file (experimental).
* **-view[WIDTHxHEIGHT]**: View model. Full screen mode is enabled if the specified width and height match the current screen resolution.

### Keyboard controls in view mode

* **0** to **5**: Set debug render mode. 0 or 1: disabled (default), 2: depth, 3: normals, 4: diffuse texture only with no lighting, 5: light only (white textures).
* **+**, **-**: Zoom in or out.
* **Keypad 1**, **3**, **9**, **7**: Set isometric view from the SW, SE, NE, or NW (default).
* **Keypad 2**, **6**, **8**, **4**, **5**: Set view from the S, E, N, W, or top.
* **F1** to **F4**: Select default cube map.
* **A**, **D**: Rotate model around the Z axis.
* **S**, **W**: Rotate model around the X axis.
* **Q**, **E**: Rotate model around the Y axis.
* **Left**, **Right**: Rotate light vector around the X axis.
* **Up**, **Down**: Rotate light vector around the Y axis.
* **Page Up**: Enable downsampling (slow).
* **Page Down**: Disable downsampling.
* **Esc**: Quit program.

### Examples

##### Text output formats

    ./nif_info -q Fallout76/Data golf_ball.nif
    ./nif_info Fallout76/Data golf_ball.nif > golf_ball.txt
    ./nif_info -v Fallout76/Data golf_ball.nif > golf_ball_full.txt
    ./nif_info -obj Fallout76/Data golf_ball.nif > golf_ball.obj
    ./nif_info -mtl Fallout76/Data golf_ball.nif > golf_ball.mtl

##### Rendering and viewing

    ./nif_info -render1920x1080 glassdome.dds Fallout76/Data palaceofthewindingpath/palace_bld_glassdome01.nif
    ./nif_info -render1920x1080 44.dds Fallout4/Data weapons/44/44load.nif
    ./nif_info -view1920x1080 Fallout76/Data meshes/test/testpbrmaterials01.nif

