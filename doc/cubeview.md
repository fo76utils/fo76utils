    cubeview WIDTH HEIGHT ARCHIVEPATH TEXTURE.DDS [MIPLEVEL]

View a Fallout 4 or 76 cube map from archive files.

### Options

* **WIDTH**, **HEIGHT**: Image dimensions to display, matching the current screen resolution enables full screen mode.
* **ARCHIVEPATH**: Path to archive(s) containing the cube map texture.
* **TEXTURE.DDS**: Full path to texture file within the archives, it is expected to be a cube map with the faces in the order used by Fallout 4 and 76. Use **baunpack ARCHIVEPATH --list /cubemaps/** to print the list of available cube map textures.
* **MIPLEVEL**: Initial mip level to display.

### Keyboard controls

* **A**, **D**: Turn left or right.
* **W**, **S**: Look up or down.
* **Q**, **E**: Roll left or right.
* **Insert**, **Delete**: Zoom in or out.
* **Home**: Reset view direction.
* **Page Up**, **Page Down**: Enable or disable gamma correction by 2.0.
* **0** to **9**: Set mip level.
* **+**, **-**: Increase or decrease mip level by 0.125.
* **Esc**: Quit program.

### Example

    ./cubeview 1920 1080 Fallout76/Data textures/shared/cubemaps/outsideoldtownreflectcube_e.dds

