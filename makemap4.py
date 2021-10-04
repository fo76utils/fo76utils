#!/usr/bin/python

import os, sys

convertPath = "C:/Program Files/ImageMagick-7.1.0-Q16-HDRI/convert.exe"
fo3Path = "H:/SteamLibrary/steamapps/common/Fallout 3 goty/Data"
fonvPath = "H:/SteamLibrary/steamapps/common/Fallout New Vegas enplczru/Data"
tes5Path = "H:/SteamLibrary/steamapps/common/Skyrim Special Edition/Data"
fo4Path = "I:/SteamLibrary/steamapps/common/Fallout 4/Data"
fo76Path = "D:/SteamLibrary/steamapps/common/Fallout76/Data"

def runCmd(args):
    tmpArgs = args
    if not ("/" in tmpArgs[0] or "\\" in tmpArgs[0]):
        tmpArgs[0] = ".\\" + tmpArgs[0] + ".exe"
    return os.spawnv(os.P_WAIT, tmpArgs[0], tmpArgs)

def findArchives(path, pattern = ""):
    fileList = []
    d = os.scandir(path)
    try:
        while True:
            fileName = d.__next__().name
            tmpName = fileName.lower()
            if pattern and not pattern in tmpName:
                continue
            if tmpName.endswith(".bsa") or tmpName.endswith(".ba2"):
                fileList += [path + "/" + fileName]
    except:
        pass
    return fileList

def createMap(gameName, gamePath, btdName, esmName, worldID, defTxtID, w, h, l):
    archiveList = findArchives(gamePath, "textures")
    runCmd(["baunpack"] + archiveList + ["--", "@ltex/%s.txt" % (gameName)])
    if btdName:
        args1 = ["btddump", gamePath + "/Terrain/" + btdName]
        args2 = ["-100", "-100", "100", "100", "2"]
    else:
        args1 = ["fo4land", gamePath + "/" + esmName]
        args2 = ["-32", "-32", "31", "31", str(worldID), str(defTxtID)]
    runCmd(args1 + [gameName + "hmap.dds", "0"] + args2)
    runCmd(args1 + [gameName + "ltex.dds", "2"] + args2)
    if btdName:
        runCmd(args1 + [gameName + "gcvr.dds", "4"] + args2)
    else:
        runCmd(args1 + [gameName + "vclr.dds", "4"] + args2)
    runCmd(["findwater", gamePath + "/" + esmName, gameName + "wmap.dds",
            gameName + "hmap.dds", str(worldID)])
    if btdName:
        runCmd(["landtxt", gameName + "ltex.dds", gameName + "ltx2.dds",
                "ltex/" + gameName + ".txt", "-mip", "7.0", "-mult", "1.25"])
        #       , "-gcvr", gameName + "gcvr.dds"])
    else:
        runCmd(["landtxt", gameName + "ltex.dds", gameName + "ltx2.dds",
                "ltex/" + gameName + ".txt", "-mip", "5.0", "-mult", "1.25",
                "-scale", "1", "-vclr", gameName + "vclr.dds"])
    runCmd(["terrain", gameName + "hmap.dds", gameName + "_map.dds",
            str(w), str(h), "-2d", "-ltex", gameName + "ltx2.dds", "-lod", "-1",
            "-wmap", gameName + "wmap.dds", "-watercolor", "0x60003048",
            "-light", str(l), "100", "35"])
    runCmd([convertPath, gameName + "_map.dds",
            # "-filter", "Lanczos", "-resize", "50x50%",
            "-interlace", "Plane", "-sampling-factor", "1x1", "-quality", "90",
            "JPEG:" + gameName + "_map.jpg"])

# createMap("fo3", fo3Path, "", "Fallout3.esm", 0x3C, 0x357BA, 6304, 6400, 125)
# createMap("fonv", fonvPath, "", "FalloutNV.esm", 0xDA726, 0x357BA, 4096, 4160, 125)
# createMap("tes5", tes5Path, "", "Skyrim.esm", 0x3C, 0xC16, 3808, 3008, 160)
createMap("fo4", fo4Path, "", "Fallout4.esm", 0x3C, 0xBE9B6, 4096, 4096, 125)
# createMap("fo76", fo76Path, "Appalachia.btd", "SeventySix.esm", 0x25DA15, 0,
#           6432, 6432, 120)

