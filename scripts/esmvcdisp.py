#!/usr/bin/python3

import sys
import matplotlib
if sys.platform[:5] == "linux":
    matplotlib.use('TkAgg')
import matplotlib.pyplot as plt
import numpy as np
import math

monthOffset = 117
nMonths = 75
maxUID = 319

verticalFormat = False
displayGroups = False
rangeParams = []
fileNames = []
includePatterns = []
excludePatterns = []
recordList = {}

for i in range(1, len(sys.argv)):
    if i >= 2 and sys.argv[i - 1] == "-i":
        includePatterns += [sys.argv[i]]
        continue
    if i >= 2 and sys.argv[i - 1] == "-x":
        excludePatterns += [sys.argv[i]]
        continue
    try:
        tmp = int(sys.argv[i])
        if len(rangeParams) < 3:
            rangeParams += [tmp]
            continue
    except:
        pass
    if sys.argv[i] == "-v":
        verticalFormat = True
    elif sys.argv[i] == "-h":
        verticalFormat = False
    elif sys.argv[i] == "-g":
        displayGroups = True
    elif not (sys.argv[i] in ["-i", "-x"]):
        fileNames += [sys.argv[i]]

if not fileNames:
    print("Usage: %s [OPTIONS...] INFILE.TSV..." % (sys.argv[0]))
    print("\nOptions:")
    print("    STARTMONTH  first month to display, relative to January 2006")
    print("    NMONTHS     number of months (default: %d)" % (nMonths))
    print("    MAXUID      maximum user ID (default: %d)" % (maxUID))
    print("    -v          vertical format")
    print("    -h          horizontal format (default)")
    print("    -g          display groups")
    print("    -i PATTERN  include records matching the pattern")
    print("    -x PATTERN  exclude records matching the pattern")
    raise SystemExit(1)

if rangeParams:
    monthOffset = rangeParams[0]
if len(rangeParams) >= 2:
    nMonths = rangeParams[1]
if len(rangeParams) >= 3:
    maxUID = rangeParams[2]

startYear = 2006.0 + (monthOffset / 12.0)
zdata1 = []
ticks1 = []
ticks2 = []
for i in range(nMonths):
    if ((i + monthOffset) % 6) == 0:
        ticks1 += [startYear + (i / 12.0)]
    elif ((i + monthOffset) % 6) == 3 and nMonths < 80:
        ticks1 += [startYear + (i / 12.0)]
for i in range(maxUID + 1):
    if (i % 5) == 0:
        ticks2 += [i]
    zdata1 += [[]]
    for j in range(nMonths):
        zdata1[i] += [0]

for i in fileNames:
    f = open(i, "r")
    for l in f:
        if len(l) < 38 or l[:38] in recordList:
            continue
        recordList[l[:38]] = True
        if includePatterns or excludePatterns:
            matchFlag = not includePatterns
            for j in includePatterns:
                if j in l:
                    matchFlag = True
                    break
            for j in excludePatterns:
                if j in l:
                    matchFlag = False
                    break
            if not matchFlag:
                continue
        if l[4] != '-' or l[7] != '-' or l[10] != '\t':
            continue
        y = int(l[:4])
        m = int(l[5:7])
        x = (y - 2006) * 12 + m - (monthOffset + 1)
        if x >= 0 and x < nMonths and (displayGroups or l[18:22] != "GRUP"):
            if len(l) >= 18 and l[11] == '0' and l[12] == 'x':
                userID = int(l[11:17], base = 16) & 0xFFFF
                if userID >= 0x0100:
                    if y < 2019 or (y == 2019 and m < 6):
                        userID = userID & 0xFF
                if userID > maxUID:
                    continue
                zdata1[userID][x] += 1
    f.close()

# plot the data
fig = plt.figure(tight_layout = True)
ax = fig.add_subplot(1, 1, 1, facecolor = "black")

# set the limits
if verticalFormat:
    ax.set_xlim(0.0, maxUID + 1.0)
    ax.set_ylim(startYear + (nMonths / 12.0), startYear)
    ax.set_xticks(ticks2, minor = False)
    ax.set_yticks(ticks1, minor = False)
else:
    ax.set_xlim(startYear, startYear + (nMonths / 12.0))
    ax.set_ylim(0.0, maxUID + 1.0)
    ax.set_xticks(ticks1, minor = False)
    ax.set_yticks(ticks2, minor = False)
ax.grid(True)

def calculateColor(n):
    if n < 1:
        return None
    y = math.log10(float(n)) * 0.5
    if y <= 1.0:
        return (0.5 * (1.0 - y), y, 1.0)
    elif y <= 2.0:
        return (y - 1.0, 1.0, 2.0 - y)
    elif y <= 3.0:
        return (1.0, 3.0 - y, 0.0)
    else:
        return (1.0, 0.0, 0.0)

if verticalFormat:
    for i in range(nMonths):
        barX = []
        barColors = []
        for j in range(maxUID + 1):
            c = calculateColor(zdata1[j][i])
            if c == None:
                continue
            barX += [(j, 1.0)]
            barColors += [c]
        if barX:
            ax.broken_barh(barX, (startYear + (i / 12.0), 1.0 / 12.0),
                           facecolors = barColors)
else:
    for i in range(maxUID + 1):
        barX = []
        barColors = []
        for j in range(nMonths):
            c = calculateColor(zdata1[i][j])
            if c == None:
                continue
            barX += [(startYear + (j / 12.0), 1.0 / 12.0)]
            barColors += [c]
        if barX:
            ax.broken_barh(barX, (i, 1.0), facecolors = barColors)

# display the plot
plt.show()

