#!/usr/bin/python3

import os, sys, subprocess

# SWFTools from http://www.swftools.org/download.html
swfToolsPath = "Program Files (x86)/SWFTools"

if "win" in sys.platform:
    magickPath = "C:/Program Files/ImageMagick-7.1.0-Q16-HDRI/magick.exe"
    baunpackPath = "./baunpack.exe"
    tes5Path = "D:/SteamLibrary/steamapps/common/Skyrim Special Edition/Data"
    fo4Path = "E:/SteamLibrary/steamapps/common/Fallout 4/Data"
    fo76Path = "G:/SteamLibrary/steamapps/common/Fallout76/Data"
else:
    magickPath = "/usr/bin/magick"
    baunpackPath = "./baunpack"
    tes5Path = "./Skyrim/Data"
    fo4Path = "./Fallout4/Data"
    fo76Path = "./Fallout76/Data"
    winePath = "/usr/bin/wine"

if len(sys.argv) < 2:
    print("Usage: mapicons.py tes5|fo4|fo76 [DATAPATH]")
    raise SystemExit(1)

mipLevel = 2.5
priority = 7

if sys.argv[1] == "tes5":
    gamePath = tes5Path
    markerDefFileName = "tes5icondefs.txt"
elif sys.argv[1] == "fo4":
    gamePath = fo4Path
    markerDefFileName = "fo4icondefs.txt"
elif sys.argv[1] == "fo76":
    gamePath = fo76Path
    markerDefFileName = "fo76icondefs.txt"
    mipLevel = 2.0
else:
    print("Game name must be tes5, fo4, or fo76")
    raise SystemExit(1)
if len(sys.argv) > 2:
    gamePath = sys.argv[2]

exec(open("./scripts/mapicons.py", "r").read())

def runCmd(args):
    tmpArgs = []
    for i in args:
        tmpArgs += [str(i)]
    if not "/" in tmpArgs[0]:
        tmpArgs[0] = "./" + tmpArgs[0]
    return subprocess.run(tmpArgs)

def runBaunpack(args):
    runCmd(["./baunpack"] + args)

def runSWFTool(args):
    if "win" in sys.platform:
        fullPath = "C:/" + swfToolsPath
        runCmd([fullPath + "/" + args[0]] + args[1:])
    else:
        fullPath = os.environ["HOME"] + "/.wine/drive_c/" + swfToolsPath
        runCmd([winePath, fullPath + "/" + args[0]] + args[1:])

def runImageMagick(args):
    runCmd([magickPath] + args)

markerDefFile = open(markerDefFileName, "w")
markerDefFile.write(getMapMarkerDefs(sys.argv[1], mipLevel, priority))
markerDefFile.close()

extractMapIcons(sys.argv[1], gamePath, runBaunpack, runSWFTool, runImageMagick)

