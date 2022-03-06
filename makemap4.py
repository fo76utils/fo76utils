#!/usr/bin/python3

import os, sys, subprocess

if "win" in sys.platform:
    convertPath = "C:/Program Files/ImageMagick-7.1.0-Q16-HDRI/convert.exe"
    tes4Path = "D:/SteamLibrary/steamapps/common/Oblivion/Data"
    fo3Path = "D:/SteamLibrary/steamapps/common/Fallout 3 goty/Data"
    fonvPath = "D:/SteamLibrary/steamapps/common/Fallout New Vegas/Data"
    tes5Path = "D:/SteamLibrary/steamapps/common/Skyrim Special Edition/Data"
    fo4Path = "E:/SteamLibrary/steamapps/common/Fallout 4/Data"
    fo76Path = "G:/SteamLibrary/steamapps/common/Fallout76/Data"
else:
    convertPath = "/usr/bin/convert"
    tes4Path = "Oblivion/Data"
    fo3Path = "Fallout3/Data"
    fonvPath = "FalloutNV/Data"
    tes5Path = "Skyrim/Data"
    fo4Path = "Fallout4/Data"
    fo76Path = "Fallout76/Data"

gameInfo = [["tes4", tes4Path, "Oblivion.esm",   "",
             0x0000003C, 0x000008C0, 4288, 4128, 110],
            ["fo3",  fo3Path,  "Fallout3.esm",   "",
             0x0000003C, 0x000357BA, 6304, 6400, 125],
            ["fonv", fonvPath, "FalloutNV.esm",  "",
             0x000DA726, 0x000357BA, 4096, 4160, 125],
            ["tes5", tes5Path, "Skyrim.esm",     "",
             0x0000003C, 0x00000C16, 3808, 3008, 160],
            ["fo4",  fo4Path,  "Fallout4.esm",   "",
             0x0000003C, 0x000BE9B6, 6144, 6144, 125],
            ["fo76", fo76Path, "SeventySix.esm", "Appalachia.btd",
             0x0025DA15, 0x00000000, 6432, 6432, 120]]

def runCmd(args):
    tmpArgs = []
    for i in args:
        tmpArgs += [str(i)]
    if not "/" in tmpArgs[0]:
        tmpArgs[0] = "./" + tmpArgs[0]
    subprocess.run(tmpArgs)

def createMap(gameNum, scale):
    gameName = gameInfo[gameNum][0]
    gamePath = gameInfo[gameNum][1]
    esmName = gameInfo[gameNum][2]
    btdName = gameInfo[gameNum][3]
    worldID = gameInfo[gameNum][4]
    defTxtID = gameInfo[gameNum][5]
    w = gameInfo[gameNum][6]
    h = gameInfo[gameNum][7]
    l = gameInfo[gameNum][8]
    if btdName:
        args1 = ["btddump", gamePath + "/Terrain/" + btdName]
        args2 = ["-100", "-100", "100", "100", "2"]
    else:
        args1 = ["fo4land", gamePath + "/" + esmName]
        args2 = [str(worldID), str(defTxtID)]
    runCmd(args1 + [gameName + "hmap.dds", "0"] + args2)
    runCmd(args1 + [gameName + "ltex.dds", "2"] + args2)
    if btdName:
        runCmd(args1 + [gameName + "gcvr.dds", "4"] + args2)
        runCmd(args1 + [gameName + "vclr.dds", "6"] + args2)
    else:
        runCmd(args1 + [gameName + "vclr.dds", "4"] + args2)
    if gameNum < 3:
        runCmd(["findwater", gamePath + "/" + esmName, gameName + "wmap.dds",
                gameName + "hmap.dds", str(worldID)])
    else:
        runCmd(["findwater", gamePath + "/" + esmName, gameName + "wmap.dds",
                gameName + "hmap.dds", str(worldID), gamePath])
    landtxtOptions = [gameName + "ltex.dds", gameName + "ltx2.dds"]
    landtxtOptions += ["ltex/" + gameName + ".txt", "-d", gamePath]
    landtxtOptions += ["-mult", "1.25", "-vclr", gameName + "vclr.dds"]
    if btdName:
        landtxtOptions += ["-mip", "7.0"]   # ["-gcvr", gameName + "gcvr.dds"]
    else:
        landtxtOptions += ["-mip", "5.0"]
    landtxtOptions += ["-scale", str(scale)]
    runCmd(["landtxt"] + landtxtOptions)
    runCmd(["terrain", gameName + "hmap.dds", gameName + "_map.dds",
            str(w << scale), str(h << scale), "-2d", "-lod", str(-scale),
            "-ltex", gameName + "ltx2.dds", "-light", str(l), "100", "35",
            "-wmap", gameName + "wmap.dds", "-watercolor", "0x60003048"])
    runCmd([convertPath, gameName + "_map.dds",
            # "-filter", "Lanczos", "-resize", "50x50%",
            "-interlace", "Plane", "-sampling-factor", "1x1", "-quality", "90",
            "JPEG:" + gameName + "_map.jpg"])

createMap(4, 0)

