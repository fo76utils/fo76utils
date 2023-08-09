#!/usr/bin/python3

import os, sys, subprocess

if "win" in sys.platform:
    magickPath = "C:/Program Files/ImageMagick-7.1.0-Q16-HDRI/magick.exe"
    renderPath = "./render.exe"
    fo76Path = "F:/SteamLibrary/steamapps/common/Fallout 76 Playtest/Data"
else:
    magickPath = "/usr/bin/magick"
    renderPath = "./render"
    fo76Path = "./Fallout76/Data"

esmPath = fo76Path + "/SeventySix.esm"
convertOptions = ["-interlace", "Plane", "-sampling-factor", "1x1"]
convertOptions += ["-quality", 80]

baseOptions = [ fo76Path,
                "-light", 1.6, 70.5288, 135, "-ltxtres", 1024, "-lmip", 2,
                "-ssaa", 1, "-rq", 0x2F ]

# map name, form ID, is isometric, X offset, Y offset, [additional options]
mapList = [ [ "the_pitt_industrial",        0x00616199, False,  768, -256 ],
            [ "the_pitt_industrial_iso",    0x00616199, True,   1024, 0 ],
            [ "the_pitt_foundry",           0x0062406E, False,  -256, 512 ],
            [ "the_pitt_foundry_iso",       0x0062406E, True,   -512, 512 ],
            [ "the_pitt_trench",            0x006378CE, False,  512, -1024 ],
            [ "the_pitt_trench_iso",        0x006378CE, True,   1792, 256 ],
            [ "the_pitt_sanctum",           0x006378CF, False,  -256, -256 ],
            [ "the_pitt_sanctum_iso",       0x006378CF, True,   -256, -512 ],
            [ "the_pitt_beautiful_corner",  0x0062BC0D, False,  -384, -1024 ],
            [ "the_pitt_hub",               0x00620537, True,   2048, -1536 ],
            [ "the_pitt_exterior",          0x006139EA, True,   2048, -1024 ],
            [ "the_pitt_dungeon",           0x006139EB, True,   2048, -1536 ],
            [ "the_pitt_world",             0x00635F96, False,  0, 0 ],
            [ "protoexpo01",                0x005B0624, True,   1024, 256 ],
            [ "protoexpo02",                0x005B30C8, True,   -1024, 768 ],
            [ "protoexpo03",                0x005AE424, True,   1536, 768 ],
            [ "protoexpo04",                0x0053E8DF, True,   1024, 0 ],
            [ "protoexpo05",                0x005B5E1E, True,   1280, 256 ],
            [ "morgantown",                 0x0025DA15, False,  1792, 11008,
              [ "-xm", "/babylon" ] ],
            [ "watoga",                     0x0025DA15, False,  -18296, -17688,
              ] ]

def runCmd(args):
    tmpArgs = []
    for i in args:
        tmpArgs += [str(i)]
    if not "/" in tmpArgs[0]:
        tmpArgs[0] = "./" + tmpArgs[0]
    s = ""
    for i in tmpArgs:
        if s:
            s += ' '
        if ' ' in i:
            s += '"' + i + '"'
        else:
            s += i
    print(s)
    return subprocess.run(tmpArgs)

for i in mapList:
    mapName = i[0]
    formID = i[1]
    isIsometric = i[2]
    xOffs = i[3]
    yOffs = i[4]
    args = [renderPath, esmPath, mapName + ".dds", "-w", formID]
    if "protoexpo" in mapName:
        args += [6656, 5632, "-mip", 1]
    else:
        args += [6144, 6144]
    args += baseOptions
    if "the_pitt" in mapName:
        args += ["-env"]
        args += ["textures/shared/cubemaps/mipblur_defaultoutside_pitt.dds"]
        args += ["-lcolor", 1.3733, 0xFFA971, 1.0, -1, -1]
    if isIsometric:
        args += ["-light", 1.6, 70.5288, -135]
        if "protoexpo" in mapName:
            args += ["-view", 0.306186, 54.7356, 180, -135]
        else:
            args += ["-view", 0.153093, 54.7356, 180, -135]
    else:
        args += ["-view", 0.125, 180, 0, 0]
    xOffs = xOffs + [0.1, -0.1][int(xOffs < 0)]
    yOffs = yOffs + [0.1, -0.1][int(yOffs < 0)]
    args += [ xOffs, yOffs, 16384 ]
    if len(i) > 5:
        args += i[5]
    runCmd(args)
    runCmd([magickPath, "convert", mapName + ".dds"] + convertOptions
           + ["JPEG:" + mapName + ".jpg"])

