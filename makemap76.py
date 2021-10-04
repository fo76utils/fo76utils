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
# enable these to save time if all the data is already extracted and up to date
noExtractIcons = False          # do not extract map marker icons
noExtractTextures = False       # do not extract land textures
noExtractTerrain = False        # do not extract terrain from Appalachia.btd
markerDefFileName = "fo76icondefs.txt"
# SWFTools from http://www.swftools.org/download.html
swfToolsPath = "Program Files (x86)/SWFTools"

additionalMarkerDefs = [[0x003BD4F4, 2, 0x3E000000, 1.0, 1]]
additionalMarkerDefs += [[0x003BD4F4, 2, "interface/fo76icon_1e.dds", 0.5, 1]]

if useAdditionalMarkers:
    additionalMarkerDefs += [[0x0035D223, 2, 0x4FFF4020, 3.0, 1]]
    additionalMarkerDefs += [[0x0035D238, 2, 0x4FFF4020, 3.0, 1]]

if "win" in sys.platform:
    imageMagickPath = "C:/Program Files/ImageMagick-7.1.0-Q16-HDRI/magick.exe"
    baunpackPath = "./baunpack.exe"
    fo76Path = "D:/SteamLibrary/steamapps/common/Fallout76/Data"
else:
    imageMagickPath = "/usr/bin/magick"
    baunpackPath = "./baunpack"
    fo76Path = "./Fallout76/Data"
    winePath = "/usr/bin/wine"

def runCmd(args):
    tmpArgs = []
    for i in args:
        tmpArgs += [str(i)]
    if not "/" in tmpArgs[0]:
        tmpArgs[0] = "./" + tmpArgs[0]
    subprocess.run(tmpArgs)

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

iconList = [ [ 0x0000, 386, "cave"              ],
             [ 0x0001, 382, "city"              ],
             [ 0x0002, 358, "camp"              ],
             [ 0x0003, 301, "factory"           ],
             [ 0x0004, 253, "Sugarmaple"        ],
             [ 0x0006, 259, "military"          ],
             [ 0x0007, 287, "mountains"         ],
             [ 0x0008, 231, "building"          ],
             [ 0x0009, 109, "ruins", "200,200"  ],
             [ 0x000B, 180, "monument"          ],
             [ 0x000C, 172, "settlement"        ],
             [ 0x000D, 170, "sewers"            ],
             [ 0x000E,  85, "vault"             ],
             [ 0x000F, 414, "airport"           ],
             [ 0x0011, 390, "dirt track"        ],
             [ 0x0012, 384, "church"            ],
             [ 0x0013, 378, "golf club"         ],
             [ 0x0016, 360, "highway"           ],
             [ 0x0017, 352, "farm"              ],
             [ 0x0018, 348, "Red Rocket"        ],
             [ 0x0019, 343, "forest"            ],
             [ 0x001A, 341, "institutional"     ],
             [ 0x001B, 339, "cemetery"          ],
             [ 0x001C, 333, "hospital"          ],
             [ 0x001D, 303, "chemical"          ],
             [ 0x001E, 125, "workshop"          ],
             [ 0x0021, 289, "junkyard"          ],
             [ 0x0023, 223, "pier"              ],
             [ 0x0024, 211, "pond / lake"       ],
             [ 0x0025, 196, "mine"              ],
             [ 0x0026, 190, "disposal site"     ],
             [ 0x0027, 188, "relay tower"       ],
             [ 0x002D, 111, "town"              ],
             [ 0x002E, 404, "Fort Atlas", "152,192" ],
             [ 0x002F, 400, "business"          ],
             [ 0x0030, 398, "bunker"            ],
             [ 0x0031, 388, "Prickett's Fort"   ],
             [ 0x0033, 168, "Portside Pub"      ],
             [ 0x0034, 277, "low rise"          ],
             [ 0x0036,  74, "player camp"       ],
             [ 0x0038, 182, "railroad"          ],
             [ 0x0039, 178, "satellite array"   ],
             [ 0x003D, 186, "raider camp"       ],
             [ 0x0040, 107, "train station"     ],
             [ 0x0041, 362, "power substation"  ],
             [ 0x0042, 346, "fissure site"      ],
             [ 0x0043,  95, "Vault 63"          ],
             [ 0x0044,  93, "Vault 76"          ],
             [ 0x0045,  89, "Vault 94"          ],
             [ 0x0046,  87, "Vault 96"          ],
             [ 0x0047, 412, "amusement park", "256,368" ],
             [ 0x0048, 275, "mansion", "368,280" ],
             [ 0x0049, 408, "Arktos Pharma", "256,144", "336,80", "352,48" ],
             [ 0x004A, 209, "power plant", "160,256", "160,320", "112,352" ],
             [ 0x004B, 166, "ski resort", "240,344", "288,344" ],
             [ 0x004C, 410, "App. Antiques"     ],
             [ 0x004D, 115, "The Giant Teapot"  ],
             [ 0x004E, 416, "agricult. center"  ],
             [ 0x004F,  77, "shack"             ],
             [ 0x0050, 331, "trailer", "224,112", "288,112" ],
             [ 0x0051, 279, "lookout", "192,128" ],
             [ 0x0052, 229, "overlook"          ],
             [ 0x0053, 198, "Pumpkin House"     ],
             [ 0x0054, 376, "cafe"              ],
             [ 0x0055, 396, "cabin"             ],
             [ 0x0056, 105, "trainyard"         ],
             [ 0x0057, 392, "capitol"           ],
             [ 0x0058, 335, "hi-tech building"  ],
             [ 0x0059, 281, "lighthouse"        ],
             [ 0x005A, 356, "Mount Blair"       ],
             [ 0x005C, 225, "P. Winding Path"   ],
             [ 0x005D, 113, "Top of the World"  ],
             [ 0x005E, 370, "dam",
               "96,344", "166,388", "228,344",
               "236,344", "304,388", "372,344"  ],
             [ 0x005F, 255, "Pylon V-13"        ],
             [ 0x0060,  79, "The Whitespring"   ],
             [ 0x0061, 239, "Nuka-Cola plant"   ],
             [ 0x0063, 164, "The Crater"        ],
             [ 0x0064, 337, "Foundation"        ],
             [ 0x0065, 374, "Mothman Cult", "176,192" ],
             [ 0x0066, 406, "Blood Eagles"      ],
             [ 0x0067,  91, "Vault 79"          ],
             [ 0x0069, 285, "The Rusty Pick", "192,192", "320,192" ],
             [ 0x006A,  97, "Vault 51"          ] ]

def runSWFTool(args):
    if "win" in sys.platform:
        fullPath = "C:/" + swfToolsPath
        runCmd([fullPath + "/" + args[0]] + args[1:])
    else:
        fullPath = os.environ["HOME"] + "/.wine/drive_c/" + swfToolsPath
        runCmd([winePath, fullPath + "/" + args[0]] + args[1:])

def removeFile(fileName):
    try:
        os.remove(fileName)
    except:
        pass

if enableMarkers:
    if not noExtractIcons:
        runCmd([baunpackPath, fo76Path + "/SeventySix - Interface.ba2", "--",
                "mapmarkerlibrary.swf", "mapmenu.swf"])
    markerDefFile = open(markerDefFileName, "w")
    for i in iconList:
        iconFileName = "interface/fo76icon_%02x.dds" % (i[0])
        n = i[0]
        if n == 0x001E:         # workshop
            swfName = "interface/mapmarkerlibrary.swf"
        else:
            swfName = "interface/mapmenu.swf"
            markerDefFile.write("0x%08X\t0x%04X\t%s\t%g\t7\t// %s\n" % (
                                    0x10, n, iconFileName, mipLevel + 0.5,
                                    i[2]))
        if noExtractIcons:
            continue
        runSWFTool(["swfextract.exe", "-i", i[1], swfName])
        runSWFTool(["swfbbox.exe", "-e", "output.swf"])
        runSWFTool(["swfrender.exe", "-o", "output.png", "output.swf"])
        removeFile("output.swf")
        convertOptions = ["-gravity", "center", "-background", "transparent"]
        for j in range(3, len(i)):
            if j == 3:
                convertOptions += ["-fill", "black", "-fuzz", "49%"]
            convertOptions += ["-draw", "color " + i[j] + " floodfill"]
        convertOptions += ["-filter", "Mitchell"]
        if i[0] == 0x0042:          # fissure site
            convertOptions += ["-modulate", "100,200,3"]
        convertOptions += ["-resize", "35x35%", "-extent", "256x256"]
        convertOptions += ["DDS:" + iconFileName]
        runCmd([imageMagickPath, "convert", "output.png"] + convertOptions)
        removeFile("output.png")
    for i in additionalMarkerDefs:
        markerDefFile.write("0x%08X\t%d\t%s\t%g\t%d\n" % (
                                (i[0], i[1], str(i[2]), mipLevel + i[3],
                                 i[4] + 7)))
    markerDefFile.close()

def createMap(gameName, gamePath, btdName, esmName, worldID, defTxtID):
    if not noExtractTextures:
        archiveList = findArchives(gamePath, "textures")
        runCmd(["baunpack"] + archiveList + ["--", "@ltex/%s.txt" % (gameName)])
    if not noExtractTerrain:
        args1 = ["btddump", gamePath + "/Terrain/" + btdName]
        args2 = cellRange + [mipLevel]
        runCmd(args1 + [gameName + "hmap.dds", "0"] + args2)
        runCmd(args1 + [gameName + "ltex.dds", "2"] + args2)
        if enableGCVR:
            runCmd(args1 + [gameName + "gcvr.dds", "4"] + args2)
    runCmd(["findwater", gamePath + "/" + esmName, gameName + "wmap.dds",
            gameName + "hmap.dds", worldID])
    landtxtOptions = [gameName + "ltex.dds", gameName + "ltx2.dds"]
    landtxtOptions += ["ltex/" + gameName + ".txt"]
    landtxtOptions += ["-mip", mipLevel + textureMip]
    landtxtOptions += ["-mult", textureBrightness]
    if enableGCVR:
        landtxtOptions += ["-gcvr", gameName + "gcvr.dds"]
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

