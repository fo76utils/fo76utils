    cubeview WIDTH HEIGHT ARCHIVEPATH TEXTURE.DDS [MIPLEVEL]

View Fallout 4 or 76 cube maps from archive files. Can also be used to view any textures in supported formats.

### Options

* **WIDTH**, **HEIGHT**: Image dimensions to display, matching the current screen resolution enables full screen mode.
* **ARCHIVEPATH**: Path to archive(s) containing the cube map texture.
* **TEXTURE.DDS**: File name pattern to search for within the archives. For cube map textures, the order and orientation of faces are expected to be compatible with Starfield (see [sfcube.cpp](https://github.com/fo76utils/libfo76utils/src/sfcube.cpp)). Use **baunpack ARCHIVEPATH --list /cubemaps/** to print the list of available cube map textures.
* **MIPLEVEL**: Initial mip level to display.

### Keyboard controls

* **A**, **D**: Turn left or right.
* **W**, **S**: Look up or down.
* **Q**, **E**: Roll left or right.
* **Insert**, **Delete**: Zoom in or out.
* **Home**: Reset view direction, FOV and brightness.
* **Page Up**, **Page Down**: Enable or disable conversion to sRGB color space.
* **0** to **9**: Set cube map mip level.
* **+**, **-**: Increase or decrease mip level by 0.125.
* **L**, **K**: Increase or decrease cube map brightness.
* **Space**, **Backspace**: Load the next or previous file matching the pattern.
* **F1**: Switch to cube map view mode.
* **F2**: Switch to 2D texture view mode (disables view controls).
* **F3**: Disable normal map Z channel fix.
* **F4**: Enable normal map Z channel fix (texture view only).
* **F5**: Disable alpha blending.
* **F6**: Enable alpha blending (texture view only).
* **F9**: Select file from list.
* **F11**: Save decompressed texture, including all faces of a cube map, but no mipmaps. Alpha channel and normal map Z fix settings are applied to the output file.
* **F12** or **Print Screen**: Save screenshot.
* **H**: Show help screen.
* **C**: Clear messages.
* **Esc**: Quit program.

### Example

    ./cubeview 1920 1080 Fallout76/Data textures/shared/cubemaps/outsideoldtownreflectcube_e.dds

