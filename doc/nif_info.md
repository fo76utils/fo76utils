    nif_info [OPTIONS] ARCHIVEPATH PATTERN...

List data from a set of .NIF files in .BA2 or .BSA archives, convert to .OBJ format, or render the model to a DDS file, or display it.

### Options

* **--**: Remaining options are file names.
* **-o FILENAME**: Set output file name (default: standard output).
* **-cdb FILENAME**: Set material database file name(s), can be a comma separated list of multiple CDB files (default: materials/materialsbeta.cdb).
* **-q**: Print author name, file name, and file size only.
* **-v**: Verbose mode, print block list, and vertex and triangle data.
* **-m**: Print detailed material information.
* **-obj**: Print model data in .obj format.
* **-mtl**: Print material data in .mtl format.
* **-c**: Enable vertex colors in .obj output.
* **-w**: Enable vertex weights in .obj output (only the first weight is printed).
* **-lN**: Set level of detail (default: -l0).
* **-render[WIDTHxHEIGHT] DDSFILE**: Render model to DDS file.
* **-view[WIDTHxHEIGHT]**: View model. Full screen mode is enabled if the specified width and height match the current screen resolution.

### Keyboard controls in view mode

* **0** to **5**: Set debug render mode. 0 or 1: disabled (default), 2: depth, 3: normals, 4: diffuse texture only with no lighting, 5: light only (white textures).
* **+**, **-**: Zoom in or out.
* **Keypad 0**, **5**: Set view from the bottom or top.
* **Keypad 1** to **9**: Set isometric view from the SW to NE (default = NW).
* **Shift** + **Keypad 0** to **9**: Set side view, or top/bottom view rotated by 45 degrees.
* **F1** to **F8**: Select default cube map.
* **A**, **D**: Rotate model around the Z axis.
* **S**, **W**: Rotate model around the X axis.
* **Q**, **E**: Rotate model around the Y axis.
* **K**, **L**: Decrease or increase overall brightness.
* **U**, **7**: Decrease or increase light source red level.
* **I**, **8**: Decrease or increase light source green level.
* **O**, **9**: Decrease or increase light source blue level.
* **Left**, **Right**: Rotate light vector around the Z axis.
* **Up**, **Down**: Rotate light vector around the Y axis.
* **Home**: Reset rotations.
* **Insert**, **Delete**: Zoom reflected environment in or out.
* **Caps Lock**: Toggle fine adjustment of view and lighting parameters.
* **Page Up**: Enable downsampling (slow).
* **Page Down**: Disable downsampling.
* **Space**, **Backspace**: Load next or previous file matching the pattern.
* **F9**: Select file from list.
* **F12** or **Print Screen**: Save screenshot.
* **F11**: Save high quality screenshot (double resolution and downsample).
* **P**: Print current settings and file name.
* **V**: View detailed model information.
* **H**: Show help screen.
* **C**: Clear messages.
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
    ./nif_info -render1920x1080 44.dds Fallout76/Data weapons/44/44load.nif
    ./nif_info -view1920x1080 Fallout76/Data meshes/test/testpbrmaterials01.nif

