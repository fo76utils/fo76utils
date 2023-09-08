#!/usr/bin/python3

import sys

blockTypes = []
enumBlockTypes = []
enumBlockTypes += ["Unknown"]
enumBlockTypes += ["NiNode"]
enumBlockTypes += ["BSFadeNode"]
enumBlockTypes += ["BSMultiBoundNode"]
enumBlockTypes += ["BSLeafAnimNode"]
enumBlockTypes += ["NiSwitchNode"]
enumBlockTypes += ["BSGeometry"]
enumBlockTypes += ["BSTriShape"]
enumBlockTypes += ["BSMeshLODTriShape"]
enumBlockTypes += ["BSEffectShaderProperty"]
enumBlockTypes += ["NiAlphaProperty"]
enumBlockTypes += ["BSLightingShaderProperty"]
enumBlockTypes += ["BSShaderTextureSet"]
enumBlockTypes += ["BSWaterShaderProperty"]
enumBlockTypes += ["BSOrderedNode"]

blockTypeMap = {}
blockTypeMap["BSEffectShaderProperty"] = "BSLightingShaderProperty"
blockTypeMap["BSFadeNode"] = "NiNode"
blockTypeMap["BSGeometry"] = "BSTriShape"
blockTypeMap["BSLeafAnimNode"] = "NiNode"
blockTypeMap["BSMeshLODTriShape"] = "BSTriShape"
blockTypeMap["BSMultiBoundNode"] = "NiNode"
blockTypeMap["BSOrderedNode"] = "NiNode"
blockTypeMap["BSSubIndexTriShape"] = "BSTriShape"
blockTypeMap["BSTreeNode"] = "NiNode"
blockTypeMap["NiSwitchNode"] = "NiNode"

f = open(sys.argv[1], "r")
for l in f:
    s = l.strip()
    if s and not (s in blockTypes):
        blockTypes += [s]
blockTypes.sort()
n = len(blockTypes) + 1

blockTypeIDs = {}
blockTypeIDs["Unknown"] = 0
for i in range(len(blockTypes)):
    blockTypeIDs[blockTypes[i]] = i + 1

print("  enum")
print("  {")
print("    // the complete list of block types is in nifblock.cpp");
for i in range(len(enumBlockTypes)):
    s = enumBlockTypes[i]
    if (i + 1) < len(enumBlockTypes):
        print("    BlkType%s = %d," % (s, blockTypeIDs[s]))
    else:
        print("    BlkType%s = %d" % (s, blockTypeIDs[s]))
print("  };")

print("")
print("const char * NIFFile::blockTypeStrings[%d] =" % (n))
print("{")
print("  \"Unknown\",                                    //   0")
for i in range(n - 1):
    s = "  \"" + blockTypes[i] + '"'
    if (i + 1) < (n - 1):
        s += ','
    while len(s) < 48:
        s += ' '
    print("%s// %3d" % (s, i + 1))
print("};")

print("")
print("const unsigned char NIFFile::blockTypeBaseTable[%d] =" % (n))
print("{")
s = "    0"
for i in range(n - 1):
    j = i + 1
    if blockTypes[i] in blockTypeMap:
        j = blockTypeIDs[blockTypeMap[blockTypes[i]]]
    s += ", "
    if (i % 10) == 9:
        while len(s) < 56:
            s += ' '
        s += "// %3d" % (i - 9)
        print(s)
        s = "  "
    s += "%3d" % (j)
while len(s) < 56:
    s += ' '
s += "// %3d" % ((n - 1) - ((n - 1) % 10))
print(s)
print("};")
print("")

