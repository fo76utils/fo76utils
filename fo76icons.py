#!/usr/bin/python3

import os, sys, subprocess

# SWFTools from http://www.swftools.org/download.html
swfToolsPath = "Program Files (x86)/SWFTools"

if "win" in sys.platform:
    magickPath = "C:/Program Files/ImageMagick-7.1.0-Q16-HDRI/magick.exe"
    baunpackPath = "./baunpack.exe"
    fo76DataPath = "G:/SteamLibrary/steamapps/common/Fallout76/Data"
else:
    magickPath = "/usr/bin/magick"
    baunpackPath = "./baunpack"
    fo76DataPath = "./Fallout76/Data"
    winePath = "/usr/bin/wine"

markerDefFileName = "fo76icondefs.txt"
mipLevel = 2.0
priority = 7

iconList = [ [ 0x0000, 391, "cave"              ],
             [ 0x0001, 387, "city"              ],
             [ 0x0002, 363, "camp"              ],
             [ 0x0003, 304, "factory"           ],
             [ 0x0004, 256, "Sugarmaple"        ],
             [ 0x0006, 262, "military"          ],
             [ 0x0007, 290, "mountains"         ],
             [ 0x0008, 234, "building"          ],
             [ 0x0009, 112, "ruins", "200,200"  ],
             [ 0x000B, 183, "monument"          ],
             [ 0x000C, 175, "settlement"        ],
             [ 0x000D, 173, "sewers"            ],
             [ 0x000E,  88, "vault"             ],
             [ 0x000F, 419, "airport"           ],
             [ 0x0011, 395, "dirt track"        ],
             [ 0x0012, 389, "church"            ],
             [ 0x0013, 383, "golf club"         ],
             [ 0x0016, 365, "highway"           ],
             [ 0x0017, 357, "farm"              ],
             [ 0x0018, 353, "Red Rocket"        ],
             [ 0x0019, 348, "forest"            ],
             [ 0x001A, 346, "institutional"     ],
             [ 0x001B, 344, "cemetery"          ],
             [ 0x001C, 338, "hospital"          ],
             [ 0x001D, 306, "chemical"          ],
             [ 0x001E, 125, "workshop"          ],
             [ 0x0021, 292, "junkyard"          ],
             [ 0x0023, 226, "pier"              ],
             [ 0x0024, 214, "pond / lake"       ],
             [ 0x0025, 199, "mine"              ],
             [ 0x0026, 193, "disposal site"     ],
             [ 0x0027, 191, "relay tower"       ],
             [ 0x002D, 114, "town"              ],
             [ 0x002E, 409, "Fort Atlas", "152,192" ],
             [ 0x002F, 405, "business"          ],
             [ 0x0030, 403, "bunker"            ],
             [ 0x0031, 393, "Prickett's Fort"   ],
             [ 0x0033, 171, "Portside Pub"      ],
             [ 0x0034, 280, "low rise"          ],
             [ 0x0036,  77, "player camp"       ],
             [ 0x0038, 185, "railroad"          ],
             [ 0x0039, 181, "satellite array"   ],
             [ 0x003D, 189, "raider camp"       ],
             [ 0x0040, 110, "train station"     ],
             [ 0x0041, 367, "power substation"  ],
             [ 0x0042, 351, "fissure site"      ],
             [ 0x0043,  98, "Vault 63"          ],
             [ 0x0044,  96, "Vault 76"          ],
             [ 0x0045,  92, "Vault 94"          ],
             [ 0x0046,  90, "Vault 96"          ],
             [ 0x0047, 417, "amusement park", "256,368" ],
             [ 0x0048, 278, "mansion", "224,224", "368,280" ],
             [ 0x0049, 413, "Arktos Pharma", "256,144", "336,80", "352,48" ],
             [ 0x004A, 212, "power plant", "160,256", "160,320", "112,352" ],
             [ 0x004B, 169, "ski resort", "240,344", "288,344" ],
             [ 0x004C, 415, "App. Antiques"     ],
             [ 0x004D, 118, "The Giant Teapot"  ],
             [ 0x004E, 421, "agricult. center"  ],
             [ 0x004F,  80, "shack"             ],
             [ 0x0050, 336, "trailer", "224,112", "288,112" ],
             [ 0x0051, 282, "lookout", "192,128" ],
             [ 0x0052, 232, "overlook"          ],
             [ 0x0053, 201, "Pumpkin House"     ],
             [ 0x0054, 381, "cafe"              ],
             [ 0x0055, 401, "cabin"             ],
             [ 0x0056, 108, "trainyard"         ],
             [ 0x0057, 397, "capitol"           ],
             [ 0x0058, 340, "hi-tech building"  ],
             [ 0x0059, 284, "lighthouse"        ],
             [ 0x005A, 361, "Mount Blair"       ],
             [ 0x005C, 228, "P. Winding Path"   ],
             [ 0x005D, 116, "Top of the World"  ],
             [ 0x005E, 375, "dam",
               "96,344", "166,388", "228,344",
               "236,344", "304,388", "372,344"  ],
             [ 0x005F, 258, "Pylon V-13"        ],
             [ 0x0060,  82, "The Whitespring"   ],
             [ 0x0061, 242, "Nuka-Cola plant"   ],
             [ 0x0063, 167, "The Crater"        ],
             [ 0x0064, 342, "Foundation"        ],
             [ 0x0065, 379, "Mothman Cult", "176,192" ],
             [ 0x0066, 411, "Blood Eagles"      ],
             [ 0x0067,  94, "Vault 79"          ],
             [ 0x0069, 288, "The Rusty Pick", "192,192", "320,192" ],
             [ 0x006A, 100, "Vault 51"          ] ]

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

runCmd([baunpackPath, fo76DataPath + "/SeventySix - Interface.ba2", "--",
        "mapmarkerlibrary.swf", "mapmenu.swf"])

markerDefFile = open(markerDefFileName, "w")
for i in iconList:
    iconFileName = "interface/fo76icon_%02x.dds" % (i[0])
    if i[0] == 0x001E:          # workshop
        swfName = "interface/mapmarkerlibrary.swf"
    else:
        swfName = "interface/mapmenu.swf"
        markerDefFile.write("0x%08X\t0x%04X\t%s\t%g\t%d\t// %s\n" % (
                                0x10, i[0], iconFileName, mipLevel, priority,
                                i[2]))
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
    runCmd([magickPath, "convert", "output.png"] + convertOptions)
    removeFile("output.png")
markerDefFile.write("0x003BD4F4\t2\t0x3E000000\t%g\t%d\t// workshop\n" % (
                        mipLevel + 0.5, priority + 1))
markerDefFile.write("0x003BD4F4\t2\tinterface/fo76icon_1e.dds\t%g\t%d\n" % (
                        mipLevel, priority + 1))
markerDefFile.close()

