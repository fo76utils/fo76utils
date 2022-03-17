#!/usr/bin/python3

import os, sys, subprocess

mipLevel = 1                    # resolution is 2 ^ (7 - mipLevel) per cell
resizeOutput = "100x100%"
cellRange = [-70, -70, 70, 70]  # xMin, yMin, xMax, yMax
waterColor = 0x60002030         # A7R8G8B8
jpegQuality = 80
enableIsometric = False
enableMarkers = not enableIsometric and True    # does not work in isometric
markerAlpha = 75                # percents
useAdditionalMarkers = False
enableTexture = True
textureMip = 4.0        # 5.0
textureBrightness = 1.2
terrainBrightness = 125
enableGCVR = False
enableVCLR = True
# enable these to save time if all the data is already extracted and up to date
noExtractIcons = False          # do not extract map marker icons
noExtractTerrain = False        # do not extract terrain from Appalachia.btd
markerDefFileName = "fo76icondefs.txt"
# SWFTools from http://www.swftools.org/download.html
swfToolsPath = "Program Files (x86)/SWFTools"

additionalMarkerDefs = []

if useAdditionalMarkers:
    additionalMarkerDefs += [[0x0035D223, 2, 0x4FFF4020, 3.0, 1]]
    additionalMarkerDefs += [[0x0035D238, 2, 0x4FFF4020, 3.0, 1]]

if "win" in sys.platform:
    imageMagickPath = "C:/Program Files/ImageMagick-7.1.0-Q16-HDRI/magick.exe"
    baunpackPath = "./baunpack.exe"
    fo76Path = "G:/SteamLibrary/steamapps/common/Fallout76/Data"
else:
    imageMagickPath = "/usr/bin/magick"
    baunpackPath = "./baunpack"
    fo76Path = "./Fallout76/Data"
    winePath = "/usr/bin/wine"

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
    runCmd([imageMagickPath] + args)

def removeFile(fileName):
    try:
        os.remove(fileName)
    except:
        pass

if enableMarkers:
    markerDefFile = open(markerDefFileName, "w")
    markerDefFile.write(getMapMarkerDefs("fo76", mipLevel + 0.5, 7))
    for i in additionalMarkerDefs:
        markerDefFile.write("0x%08X\t%d\t%s\t%g\t%d\n" % (
                                (i[0], i[1], str(i[2]), mipLevel + i[3],
                                 i[4] + 7)))
    markerDefFile.close()
if enableMarkers and not noExtractIcons:
    extractMapIcons("fo76", fo76Path, runBaunpack, runSWFTool, runImageMagick)

def createMap(gameName, gamePath, btdName, esmName, worldID, defTxtID):
    if not noExtractTerrain:
        args1 = ["btddump", gamePath + "/Terrain/" + btdName]
        args2 = cellRange + [mipLevel]
        runCmd(args1 + [gameName + "hmap.dds", "0"] + args2)
    runCmd(["findwater", gamePath + "/" + esmName, gameName + "wmap.dds",
            gameName + "hmap.dds", worldID, gamePath])
    landtxtOptions = [gamePath + "/" + esmName, gameName + "ltx2.dds"]
    landtxtOptions += ["-d", gamePath, "-btd", gamePath + "/Terrain/" + btdName]
    landtxtOptions += ["-r"] + cellRange + ["-l", mipLevel]
    landtxtOptions += ["-mip", mipLevel + textureMip]
    landtxtOptions += ["-mult", textureBrightness]
    if not enableGCVR:
        landtxtOptions += ["-no-gcvr"]
    if not enableVCLR:
        landtxtOptions += ["-no-vclr"]
    runCmd(["landtxt"] + landtxtOptions)
    terrainOptions = [gameName + "hmap.dds", gameName + "_map.dds"]
    w = (cellRange[2] + 1 - cellRange[0]) << (7 - mipLevel)
    h = (cellRange[3] + 1 - cellRange[1]) << (7 - mipLevel)
    if enableIsometric:
        xyOffs = -(256 >> mipLevel)
        terrainOptions += [(w * 3) >> 1, (h * 15) >> 4, "-iso"]
        terrainOptions += ["-xoffs", xyOffs, "-yoffs", xyOffs]
    else:
        terrainOptions += [w, h, "-2d"]
    if enableTexture:
        terrainOptions += ["-ltex", gameName + "ltx2.dds"]
    terrainOptions += ["-wmap", gameName + "wmap.dds"]
    terrainOptions += ["-lod", "0", "-watercolor", waterColor]
    terrainOptions += ["-light", terrainBrightness, "100", "35"]
    runCmd(["terrain"] + terrainOptions)
    if enableMarkers:
        runCmd(["markers", gamePath + "/" + esmName, gameName + "mmap.dds",
                gameName + "hmap.dds", markerDefFileName])
        compositeOptions = ["composite", "-dissolve", markerAlpha]
        compositeOptions += [gameName + "mmap.dds", gameName + "_map.dds"]
    else:
        compositeOptions = ["convert", gameName + "_map.dds"]
    if resizeOutput and resizeOutput != "100x100%":
        compositeOptions += ["-filter", "Lanczos", "-resize", resizeOutput]
    compositeOptions += ["-interlace", "Plane", "-sampling-factor", "1x1"]
    compositeOptions += ["-quality", jpegQuality]
    compositeOptions += ["JPEG:" + gameName + "_map.jpg"]
    runCmd([imageMagickPath] + compositeOptions)

createMap("fo76", fo76Path, "Appalachia.btd", "SeventySix.esm", 0x0025DA15, 0)

