#!/usr/bin/python3

import os, sys, subprocess

# SWFTools from http://www.swftools.org/download.html
swfToolsPath = "Program Files (x86)/SWFTools"

if "win" in sys.platform:
    magickPath = "C:/Program Files/ImageMagick-7.1.0-Q16-HDRI/magick.exe"
    baunpackPath = "./baunpack.exe"
    fo76DataPath = "E:/SteamLibrary/steamapps/common/Fallout 4/Data"
else:
    magickPath = "/usr/bin/magick"
    baunpackPath = "./baunpack"
    fo76DataPath = "./Fallout4/Data"
    winePath = "/usr/bin/wine"

markerDefFileName = "fo4icondefs.txt"
mipLevel = 2.5
priority = 7

iconList = [ [ 0x0000, 243, "cave"              ],
             [ 0x0001, 237, "city"              ],
             [ 0x0002, 225, "Diamond City"      ],
             [ 0x0003, 212, "camp"              ],
             [ 0x0004, 147, "industrial site"   ],
             [ 0x0005, 114, "state house"       ],
             [ 0x0006, 123, "metro"             ],
             [ 0x0007, 120, "military"          ],
             [ 0x0008, 135, "island"            ],
             [ 0x0009, 108, "building"          ],
             [ 0x000A,  20, "ruins"             ],
             [ 0x000B,  17, "district"          ],
             [ 0x000C,  53, "Sanctuary"         ],
             [ 0x000D,  41, "settlement"        ],
             [ 0x000E,  38, "sewers"            ],
             [ 0x000F,   8, "vault"             ],
             [ 0x0010, 267, "airport"           ],
             [ 0x0011, 258, "Bunker Hill"       ],
             [ 0x0012, 252, "trailer"           ],
             [ 0x0013, 249, "car"               ],
             [ 0x0014, 240, "church"            ],
             [ 0x0016, 231, "Custom House"      ],
             [ 0x0017, 218, "drive-in"          ],
             [ 0x0018, 215, "highway"           ],
             [ 0x0019, 208, "Faneuil Hall"      ],
             [ 0x001A, 205, "farm"              ],
             [ 0x001B, 202, "Red Rocket"        ],
             [ 0x001C, 199, "forest / park"     ],
             [ 0x001D, 196, "Goodneighbor"      ],
             [ 0x001E, 193, "graveyard"         ],
             [ 0x001F, 190, "hospital"          ],
             [ 0x0020, 150, "water treatment"   ],
             [ 0x0021, 147, "factory"           ],
             [ 0x0022, 144, "The Institute"     ],
             [ 0x0023, 141, "Irish Pride"       ],
             [ 0x0024, 138, "junkyard"          ],
             [ 0x0025, 111, "observatory"       ],
             [ 0x0026, 105, "pier / marina"     ],
             [ 0x0027,  94, "pond / lake"       ],
             [ 0x0028,  82, "mine / quarry"     ],
             [ 0x0029,  71, "radioactive"       ],
             [ 0x002A,  68, "relay tower"       ],
             [ 0x002B,  56, "Witchcraft museum" ],
             [ 0x002C,  47, "school"            ],
             [ 0x002D,  35, "ship wreck"        ],
             [ 0x002E,  32, "submarine"         ],
             [ 0x002F,  29, "Swan's Pond"       ],
             [ 0x0030,  26, "synth"             ],
             [ 0x0031,  23, "town"              ],
             [ 0x0032, 264, "BoS"               ],
             [ 0x0033, 261, "apartment"         ],
             [ 0x0034, 255, "bunker"            ],
             [ 0x0035, 246, "The Castle"        ],
             [ 0x0036, 228, "skyscraper"        ],
             [ 0x0037, 132, "Libertalia"        ],
             [ 0x0038, 129, "low rise"          ],
             [ 0x0039, 117, "Minutemen"         ],
             [ 0x003A,  97, "police station"    ],
             [ 0x003B,  85, "The Prydwen"       ],
             [ 0x003C,  62, "Railroad"          ],
             [ 0x003D,  59, "train station"     ],
             [ 0x003E,  50, "satellite array"   ],
             [ 0x003F,  44, "Sentinel Site"     ],
             [ 0x0040,  14, "USS Constitution"  ],
             [ 0x0041, 126, "Mechanist"         ],
             [ 0x0042,  65, "Fizztop Grille"    ],
             [ 0x0046, 181, "Galactic Zone"     ],
             [ 0x0049, 100, "NW Transit Center" ],
             [ 0x004A, 159, "Ferris wheel"      ],
             [ 0x004B, 156, "safari"            ],
             [ 0x004E, 184, "Fizztop Mountain"  ],
             [ 0x004F, 169, "The Parlor"        ],
             [ 0x0050, 165, "Bradberton Amph."  ] ]

def runCmd(args):
    tmpArgs = []
    for i in args:
        tmpArgs += [str(i)]
    if not "/" in tmpArgs[0]:
        tmpArgs[0] = "./" + tmpArgs[0]
    subprocess.run(tmpArgs)

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

runCmd([baunpackPath, fo76DataPath + "/Fallout4 - Interface.ba2", "--",
        "pipboy_mappage.swf"])

markerDefFile = open(markerDefFileName, "w")
for i in iconList:
    n = i[0]
    iconFileName = "interface/fo4icon_%02x.dds" % (n)
    swfName = "interface/pipboy_mappage.swf"
    markerDefFile.write("0x%08X\t0x%04X\t%s\t%g\t%d\t// %s\n" % (
                            0x10, n, iconFileName, mipLevel, priority, i[2]))
    runSWFTool(["swfextract.exe", "-i", i[1], swfName])
    runSWFTool(["swfbbox.exe", "-e", "output.swf"])
    runSWFTool(["swfrender.exe", "-o", "output.png", "output.swf"])
    removeFile("output.swf")
    convertOptions = ["-background", "transparent", "-gravity", "center"]
    convertOptions += ["-fill", "black", "-filter", "Mitchell"]
    runCmd([magickPath, "convert", "output.png"] + convertOptions + [
                "-resize", "38x38%", "-shadow", "90x4",
                "-extent", "256x256", "PNG:tmp1.png"])
    runCmd([magickPath, "convert", "output.png"] + convertOptions + [
                "-resize", "35x35%", "-colorize", "50,0,50",
                "-extent", "256x256", "PNG:tmp2.png"])
    removeFile("output.png")
    runCmd([magickPath, "composite", "-compose", "dst-over",
            "tmp1.png", "tmp2.png", "DDS:" + iconFileName])
    removeFile("tmp1.png")
    removeFile("tmp2.png")
markerDefFile.close()

