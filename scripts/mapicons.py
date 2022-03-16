#!/usr/bin/python3

iconList_TES5 = [
    [ 0x0000,   4, "door"              ],
    [ 0x002D,  13, "College of WH"     ],
    [ 0x002E,  15, "Winterhold"        ],
    [ 0x0025,  17, "Palace of Kings"   ],
    [ 0x0026,  19, "Windhelm"          ],
    [ 0x0027,  21, "Dragonsreach"      ],
    [ 0x0028,  23, "Whiterun"          ],
    [ 0x0013,  25, "wheat mill"        ],
    [ 0x0002,  27, "town"              ],
    [ 0x0011,  29, "Stormcloak camp"   ],
    [ 0x0015,  31, "stable"            ],
    [ 0x0029,  33, "Blue Palace"       ],
    [ 0x002A,  35, "Solitude"          ],
    [ 0x0022,  39, "shrine"            ],
    [ 0x0009,  41, "shipwreck"         ],
    [ 0x001E,  43, "shack"             ],
    [ 0x0003,  45, "settlement"        ],
    [ 0x0023,  49, "castle"            ],
    [ 0x0024,  51, "Riften"            ],
    [ 0x0018,  53, "pass"              ],
    [ 0x001C,  55, "Orc stronghold"    ],
    [ 0x001F,  57, "Nordic tower"      ],
    [ 0x0007,  59, "Nordic ruin"       ],
    [ 0x0020,  61, "Nordic dwelling"   ],
    [ 0x0030,  65, "Morthal"           ],
    [ 0x000F,  67, "mine"              ],
    [ 0x002B,  69, "Understone Keep"   ],
    [ 0x002C,  71, "Markarth"          ],
    [ 0x001B,  73, "lighthouse"        ],
    [ 0x000B,  75, "landmark"          ],
    [ 0x0016,  77, "Imperial tower"    ],
    [ 0x0010,  79, "Imperial camp"     ],
    [ 0x000A,  81, "grove"             ],
    [ 0x001D,  83, "giant camp"        ],
    [ 0x0006,  85, "fort"              ],
    [ 0x000D,  87, "farm"              ],
    [ 0x0032,  90, "Falkreath"         ],
    [ 0x0008,  92, "Dwarven ruin"      ],
    [ 0x000C,  94, "dragon lair"       ],
    [ 0x0021,  96, "docks"             ],
    [ 0x003A,  98, "Solstheim"         ],
    [ 0x0039, 100, "Skyrim"            ],
    [ 0x0038, 102, "Tel Mithryn"       ],
    [ 0x0037, 104, "All-Maker stone"   ],
    [ 0x0036, 106, "Raven Rock"        ],
    [ 0x0035, 108, "Temple of Miraak"  ],
    [ 0x0034, 113, "Dawnstar"          ],
    [ 0x0017, 115, "clearing"          ],
    [ 0x0004, 119, "cave"              ],
    [ 0x0005, 121, "camp"              ],
    [ 0x000E, 125, "lumber mill"       ],
    [ 0x0012, 127, "Standing Stone"    ] ]

iconList_FO4 = [
    [ 0x000F,   8, "vault"             ],
    [ 0x0040,  14, "USS Constitution"  ],
    [ 0x000B,  17, "district"          ],
    [ 0x000A,  20, "ruins"             ],
    [ 0x0031,  23, "town"              ],
    [ 0x0030,  26, "synth"             ],
    [ 0x002F,  29, "Swan's Pond"       ],
    [ 0x002E,  32, "submarine"         ],
    [ 0x002D,  35, "ship wreck"        ],
    [ 0x000E,  38, "sewers"            ],
    [ 0x000D,  41, "settlement"        ],
    [ 0x003F,  44, "Sentinel Site"     ],
    [ 0x002C,  47, "school"            ],
    [ 0x003E,  50, "satellite array"   ],
    [ 0x000C,  53, "Sanctuary"         ],
    [ 0x002B,  56, "Witchcraft museum" ],
    [ 0x003D,  59, "train station"     ],
    [ 0x003C,  62, "Railroad"          ],
    [ 0x0042,  65, "Fizztop Grille"    ],
    [ 0x002A,  68, "relay tower"       ],
    [ 0x0029,  71, "radioactive"       ],
    [ 0x0028,  82, "mine / quarry"     ],
    [ 0x003B,  85, "The Prydwen"       ],
    [ 0x0027,  94, "pond / lake"       ],
    [ 0x003A,  97, "police station"    ],
    [ 0x0049, 100, "NW Transit Center" ],
    [ 0x0026, 105, "pier / marina"     ],
    [ 0x0009, 108, "building"          ],
    [ 0x0025, 111, "observatory"       ],
    [ 0x0005, 114, "state house"       ],
    [ 0x0039, 117, "Minutemen"         ],
    [ 0x0007, 120, "military"          ],
    [ 0x0006, 123, "metro"             ],
    [ 0x0041, 126, "Mechanist"         ],
    [ 0x0038, 129, "low rise"          ],
    [ 0x0037, 132, "Libertalia"        ],
    [ 0x0008, 135, "island"            ],
    [ 0x0024, 138, "junkyard"          ],
    [ 0x0023, 141, "Irish Pride"       ],
    [ 0x0022, 144, "The Institute"     ],
    [ 0x0004, 147, "industrial site"   ],
    [ 0x0021, 147, "factory"           ],
    [ 0x0020, 150, "water treatment"   ],
    [ 0x004B, 156, "safari"            ],
    [ 0x004A, 159, "Ferris wheel"      ],
    [ 0x0050, 165, "Bradberton Amph."  ],
    [ 0x004F, 169, "The Parlor"        ],
    [ 0x0046, 181, "Galactic Zone"     ],
    [ 0x004E, 184, "Fizztop Mountain"  ],
    [ 0x001F, 190, "hospital"          ],
    [ 0x001E, 193, "graveyard"         ],
    [ 0x001D, 196, "Goodneighbor"      ],
    [ 0x001C, 199, "forest / park"     ],
    [ 0x001B, 202, "Red Rocket"        ],
    [ 0x001A, 205, "farm"              ],
    [ 0x0019, 208, "Faneuil Hall"      ],
    [ 0x0003, 212, "camp"              ],
    [ 0x0018, 215, "highway"           ],
    [ 0x0017, 218, "drive-in"          ],
    [ 0x0002, 225, "Diamond City"      ],
    [ 0x0036, 228, "skyscraper"        ],
    [ 0x0016, 231, "Custom House"      ],
    [ 0x0001, 237, "city"              ],
    [ 0x0014, 240, "church"            ],
    [ 0x0000, 243, "cave"              ],
    [ 0x0035, 246, "The Castle"        ],
    [ 0x0013, 249, "car"               ],
    [ 0x0012, 252, "trailer"           ],
    [ 0x0034, 255, "bunker"            ],
    [ 0x0011, 258, "Bunker Hill"       ],
    [ 0x0033, 261, "apartment"         ],
    [ 0x0032, 264, "BoS"               ],
    [ 0x0010, 267, "airport"           ] ]

iconList_FO76 = [
    [ 0x0036,  77, "player camp"       ],
    [ 0x004F,  80, "shack"             ],
    [ 0x0060,  82, "The Whitespring"   ],
    [ 0x000E,  88, "vault"             ],
    [ 0x0046,  90, "Vault 96"          ],
    [ 0x0045,  92, "Vault 94"          ],
    [ 0x0067,  94, "Vault 79"          ],
    [ 0x0044,  96, "Vault 76"          ],
    [ 0x0043,  98, "Vault 63"          ],
    [ 0x006A, 100, "Vault 51"          ],
    [ 0x0056, 108, "trainyard"         ],
    [ 0x0040, 110, "train station"     ],
    [ 0x0009, 112, "ruins", "200,200"  ],
    [ 0x002D, 114, "town"              ],
    [ 0x005D, 116, "Top of the World"  ],
    [ 0x004D, 118, "The Giant Teapot"  ],
    [ 0x001E, 125, "workshop"          ],
    [ 0x0063, 167, "The Crater"        ],
    [ 0x004B, 169, "ski resort", "240,344", "288,344" ],
    [ 0x0033, 171, "Portside Pub"      ],
    [ 0x000D, 173, "sewers"            ],
    [ 0x000C, 175, "settlement"        ],
    [ 0x0039, 181, "satellite array"   ],
    [ 0x000B, 183, "monument"          ],
    [ 0x0038, 185, "railroad"          ],
    [ 0x003D, 189, "raider camp"       ],
    [ 0x0027, 191, "relay tower"       ],
    [ 0x0026, 193, "disposal site"     ],
    [ 0x0025, 199, "mine"              ],
    [ 0x0053, 201, "Pumpkin House"     ],
    [ 0x004A, 212, "power plant", "160,256", "160,320", "112,352" ],
    [ 0x0024, 214, "pond / lake"       ],
    [ 0x0023, 226, "pier"              ],
    [ 0x005C, 228, "P. Winding Path"   ],
    [ 0x0052, 232, "overlook"          ],
    [ 0x0008, 234, "building"          ],
    [ 0x0061, 242, "Nuka-Cola plant"   ],
    [ 0x0004, 256, "Sugarmaple"        ],
    [ 0x005F, 258, "Pylon V-13"        ],
    [ 0x0006, 262, "military"          ],
    [ 0x0048, 278, "mansion", "224,224", "368,280" ],
    [ 0x0034, 280, "low rise"          ],
    [ 0x0051, 282, "lookout", "192,128" ],
    [ 0x0059, 284, "lighthouse"        ],
    [ 0x0069, 288, "The Rusty Pick", "192,192", "320,192" ],
    [ 0x0007, 290, "mountains"         ],
    [ 0x0021, 292, "junkyard"          ],
    [ 0x0003, 304, "factory"           ],
    [ 0x001D, 306, "chemical"          ],
    [ 0x0050, 336, "trailer", "224,112", "288,112" ],
    [ 0x001C, 338, "hospital"          ],
    [ 0x0058, 340, "hi-tech building"  ],
    [ 0x0064, 342, "Foundation"        ],
    [ 0x001B, 344, "cemetery"          ],
    [ 0x001A, 346, "institutional"     ],
    [ 0x0019, 348, "forest"            ],
    [ 0x0042, 351, "fissure site"      ],
    [ 0x0018, 353, "Red Rocket"        ],
    [ 0x0017, 357, "farm"              ],
    [ 0x005A, 361, "Mount Blair"       ],
    [ 0x0002, 363, "camp"              ],
    [ 0x0016, 365, "highway"           ],
    [ 0x0041, 367, "power substation"  ],
    [ 0x005E, 375, "dam", "96,344", "166,388", "228,344",
                          "236,344", "304,388", "372,344" ],
    [ 0x0065, 379, "Mothman Cult", "176,192" ],
    [ 0x0054, 381, "cafe"              ],
    [ 0x0013, 383, "golf club"         ],
    [ 0x0001, 387, "city"              ],
    [ 0x0012, 389, "church"            ],
    [ 0x0000, 391, "cave"              ],
    [ 0x0031, 393, "Prickett's Fort"   ],
    [ 0x0011, 395, "dirt track"        ],
    [ 0x0057, 397, "capitol"           ],
    [ 0x0055, 401, "cabin"             ],
    [ 0x0030, 403, "bunker"            ],
    [ 0x002F, 405, "business"          ],
    [ 0x002E, 409, "Fort Atlas", "152,192" ],
    [ 0x0066, 411, "Blood Eagles"      ],
    [ 0x0049, 413, "Arktos Pharma", "256,144", "336,80", "352,48" ],
    [ 0x004C, 415, "App. Antiques"     ],
    [ 0x0047, 417, "amusement park", "256,368" ],
    [ 0x000F, 419, "airport"           ],
    [ 0x004E, 421, "agricult. center"  ] ]

def getMapIconList(gameName):
    if gameName == "tes5":
        swfNames = ["interface/map.swf"]
        iconList = iconList_TES5
    elif gameName == "fo4":
        swfNames = ["interface/pipboy_mappage.swf"]
        iconList = iconList_FO4
    elif gameName == "fo76":
        swfNames = ["interface/mapmenu.swf", "interface/mapmarkerlibrary.swf"]
        iconList = iconList_FO76
    else:
        print("Unrecognized game name: " + gameName)
        raise SystemExit(1)
    return (swfNames, iconList)

def extractMapIcons(gameName, gamePath, baunpackFunc, swfToolFunc, magickFunc):
    swfNames, iconList = getMapIconList(gameName)
    baunpackFunc([gamePath, "--"] + swfNames)
    for i in iconList:
        n = i[0]
        iconFileName = "interface/%sicon_%02x.dds" % (gameName, n)
        swfName = swfNames[0]
        if n == 0x001E and gameName == "fo76":
            swfName = swfNames[1]       # workshop
        swfToolFunc(["swfextract.exe", "-i", i[1], swfName])
        swfToolFunc(["swfbbox.exe", "-e", "output.swf"])
        swfToolFunc(["swfrender.exe", "-o", "output.png", "output.swf"])
        removeFileList = ["output.swf", "output.png"]
        if gameName == "fo4":
            convertOptions = ["-background", "transparent"]
            convertOptions += ["-gravity", "center", "-fill", "black"]
            convertOptions += ["-filter", "Mitchell"]
            magickFunc(["convert", "output.png"] + convertOptions + [
                            "-resize", "38x38%", "-shadow", "90x4",
                            "-extent", "256x256", "PNG:tmp1.png"])
            magickFunc(["convert", "output.png"] + convertOptions + [
                            "-resize", "35x35%", "-colorize", "50,0,50",
                            "-extent", "256x256", "PNG:tmp2.png"])
            magickFunc(["composite", "-compose", "dst-over",
                            "tmp1.png", "tmp2.png", "DDS:" + iconFileName])
            removeFileList += ["tmp1.png", "tmp2.png"]
        else:
            convertOptions = ["-gravity", "center"]
            convertOptions += ["-background", "transparent"]
            for j in range(3, len(i)):
                if j == 3:
                    convertOptions += ["-fill", "black", "-fuzz", "49%"]
                convertOptions += ["-draw", "color " + i[j] + " floodfill"]
            convertOptions += ["-filter", "Mitchell"]
            if gameName == "tes5":
                convertOptions += ["-channel", "rgb", "-level"]
                # Raven Rock icon needs different white level
                convertOptions += [["75%,0%", "75%,12%"][int(n == 0x0036)]]
                convertOptions += ["-channel", "rgba"]
            elif n == 0x0042:           # fissure site
                convertOptions += ["-modulate", "100,200,3"]
            convertOptions += ["-resize", "35x35%", "-extent", "256x256"]
            convertOptions += ["DDS:" + iconFileName]
            magickFunc(["convert", "output.png"] + convertOptions)
        for j in removeFileList:
            try:
                os.remove(j)
            except:
                pass

def getMapMarkerDefs(gameName, markerMip, markerPriority):
    swfNames, iconList = getMapIconList(gameName)
    markerDefs = ""
    for i in iconList:
        n = i[0]
        iconFileName = "interface/%sicon_%02x.dds" % (gameName, n)
        if not (n == 0x001E and gameName == "fo76"):
            markerDefs += "0x%08X\t0x%04X\t%s\t%g\t%d\t// %s\n" % (
                              0x10, n, iconFileName, markerMip, markerPriority,
                              i[2])
    if gameName == "fo76":
        # workshop
        markerDefs += "0x003BD4F4\t2\t0x3E000000\t%g\t%d\t// workshop\n" % (
                          markerMip + 0.5, markerPriority + 1)
        markerDefs += "0x003BD4F4\t2\tinterface/fo76icon_1e.dds\t%g\t%d\n" % (
                          markerMip, markerPriority + 1)
    return markerDefs

