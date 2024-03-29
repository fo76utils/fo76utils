    nif_info [OPTIONS] ARCHIVEPATH PATTERN...

List data from a set of .NIF files in .BA2 or .BSA archives, convert to .OBJ format, or render the model to a DDS file, or display it.

### Options

* **--**: Remaining options are file names.
* **-o FILENAME**: Set output file name (default: standard output).
* **-q**: Print author name, file name, and file size only.
* **-v**: Verbose mode, print block list, and vertex and triangle data.
* **-m**: Print detailed material information.
* **-obj**: Print model data in .obj format.
* **-mtl**: Print material data in .mtl format.
* **-c**: Enable vertex colors in .obj output.
* **-w**: Enable vertex weights in .obj output (only the first weight is printed).
* **-lN**: Set level of detail (default: -l0).
* **-render[WIDTHxHEIGHT] DDSFILE**: Render model to DDS file.
* **-view[WIDTHxHEIGHT]**: View model. Full screen mode is enabled if the specified width and height match the current screen resolution. Downsampling is enabled if the image dimensions exceed the screen resolution, and are even numbers. For example with a 1920x1080 display, -view1920x1080 runs in full screen mode, -view3200x1800 downsamples to 1600x900, and -view3840x2160 downsamples and uses full screen mode.

#### Render and view options

* **-cam SCALE DIR RX RY RZ**: Set view scale (default = 1.0), view direction (0 to 19, see [src/viewrtbl.cpp](../src/viewrtbl.cpp)), and model rotation.
* **-light SCALE RY RZ**: Set overall brightness, and light vector Y, Z rotation in degrees (0, 0 = top). The light direction is rotated around the Y axis first (positive = east, negative = west), then around the Z axis (positive = counter-clockwise, from east towards north). The default direction is RY=56.25 and RZ=-135.
* **-env FILENAME.DDS**: Default environment map texture path in archives. Defaults to **textures/cubemaps/cell_cityplazacube.dds**. Use **baunpack ARCHIVEPATH --list /cubemaps/** to print the list of available cube map textures, and [cubeview](cubeview.md) to preview them.
* **-lcolor LMULT LCOLOR EMULT ECOLOR**: Set light source, environment, and ambient light colors and levels. LCOLOR, ECOLOR, and ACOLOR are in 0xRRGGBB format (sRGB color space), -1 is interpreted as white for LCOLOR and ECOLOR. LMULT and LCOLOR multiply diffuse and specular lighting, while EMULT and ECOLOR are applied to environment maps and also to ambient light. Ambient light color is determined from the default cube map specified with **-env**.
* **-rscale FLOAT**: Scale factor to use when calculating a view vector (X \* -normalZ, Y \* -normalZ, H \* RSCALE) from the pixel coordinates and image height for cube mapping and specular reflections. A higher value simulates a narrower FOV for the purpose of reflections. Defaults to 1.0.
* **-wtxt FILENAME.DDS**: Water normal map texture path in archives. Defaults to **textures/water/wavesdefault_normal.dds**. Setting the path to an empty string disables normal mapping on water.
* **-wrefl FLOAT**: Water environment map scale, the default is 1.0.
* **-debug INT**: Set debug render mode (0 to 5).
* **-enable-markers**: Enable rendering editor markers and geometry flagged as hidden.

### Keyboard controls in view mode

* **0** to **5**: Set debug render mode. 0: disabled (default), 1: geometry block IDs, 2: depth, 3: normals, 4: diffuse texture only with no lighting, 5: light only (white textures).
* **+**, **-**: Zoom in or out.
* **Keypad 0**, **5**: Set view from the bottom or top.
* **Keypad 1** to **9**: Set isometric view from the SW to NE (default = NW).
* **Shift** + **Keypad 0** to **9**: Set side view, or top/bottom view rotated by 45 degrees.
* **F1** to **F8**: Select default cube map.
* **G**: Toggle rendering editor markers and geometry flagged as hidden.
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
* **B**: Cycle background type (black/checkerboard/gradient).
* **Space**, **Backspace**: Load next or previous file matching the pattern.
* **F9**: Select file from list. Use the mouse or the arrow keys to navigate the file list, left clicking selects an item, double or right clicking opens it.
* **F12** or **Print Screen**: Save screenshot.
* **F11**: Save high quality screenshot (double resolution and downsample).
* **P**: Print current settings and file name, and copy the full path.
* **V**: View detailed model information. The text can be scrolled with Page Up/Page Down or with the mouse wheel, Ctrl-A copies the entire buffer to the clipboard, C returns to model view mode.
* **Mouse buttons**: In debug mode 1 only: print the geometry block, mesh and material path of the selected shape based on the color of the pixel, and also copy it to the clipboard. The left button prints basic information only, while the middle and right buttons print verbose information (in text view mode) about the geometry and material, respectively.
* **H**: Show help screen.
* **C**: Clear messages.
* **Esc**: Quit program.

### Examples

##### Text output formats

    ./nif_info -q Fallout76/Data golf_ball.nif
    ./nif_info -o airpurifier01.txt Starfield/Data airpurifier01.nif
    ./nif_info -v -o airpurifier01_full.txt Starfield/Data airpurifier01.nif
    ./nif_info -m -o airpurifier01_mats.txt Starfield/Data airpurifier01.nif
    ./nif_info -obj -o airpurifier01.obj Starfield/Data airpurifier01.nif
    ./nif_info -mtl -o airpurifier01.mtl Starfield/Data airpurifier01.nif

##### Rendering and viewing

    ./nif_info -render4000x3000 videoshipcockpit.dds Starfield/Data meshes/ships/videoshipcockpit.nif
    ./nif_info -render1920x1080 ar99.dds Starfield/Data weapons/ar99/ar99.nif
    ./nif_info -view3840x2160 Starfield/Data meshes

