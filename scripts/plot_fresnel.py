#!/usr/bin/python3

import sys
import matplotlib
if sys.platform[:5] == "linux":
    matplotlib.use('TkAgg')
import matplotlib.pyplot as plt
import numpy as np
import math

frTable = []
f = sys.stdin
if len(sys.argv) > 1:
    f = open(sys.argv[1], "r")
for l in f:
    if "f, " in l:
        tmp = l.replace(',', '').split('f')
        if len(tmp) >= 4:
            frTable += [float(tmp[0].strip()), float(tmp[1].strip()),
                        float(tmp[2].strip()), float(tmp[3].strip())]
if len(sys.argv) > 1:
    f.close()
while len(frTable) < 1024:
    frTable += [0.0]

def fresnel(nDotV, n1, n2):
    if not (nDotV > 0.0):
        return 1.0
    if not (nDotV < 1.0):
        return (((n2 - n1) * (n2 - n1)) / ((n1 + n2) * (n1 + n2)))
    # sqrt(1.0 - ((n1 / n2) * sin(acos(nDotV)))^2)
    y = math.sqrt(1.0 - ((n1 / n2) * (n1 / n2) * (1.0 - (nDotV * nDotV))))
    r_s = ((n1 * nDotV) - (n2 * y)) / ((n1 * nDotV) + (n2 * y))
    r_p = ((n1 * y) - (n2 * nDotV)) / ((n1 * y) + (n2 * nDotV))
    return (((r_s * r_s) + (r_p * r_p)) * 0.5)

def fresnelGGX(v_r, n1, n2, r, n = 4096):
    # v_r = NdotV, r = roughness
    v_r = max(min(v_r, 0.9999999), 0.0000001)
    v = complex(v_r, math.sqrt(1.0 - (v_r * v_r)))
    r = max(min(r, 1.0), 0.015625)
    a2 = r * r * r * r
    k = (r + 1.0) * (r + 1.0) * 0.125
    g = v_r / (v_r * (1.0 - k) + k)
    s = 0.0
    w = 0.0
    f = complex(math.cos(math.pi * 2.0 / float(n)),
                math.sin(math.pi * 2.0 / float(n)))
    l = complex(1.0, 0.0)
    for i in range(n):
        h = v + l
        tmp = abs(h)
        if tmp > 0.0:
            h /= tmp
        nDotL = max(l.real, 0.0)
        nDotH = max(h.real, 0.0)
        vDotH = abs((v.real * h.real) + (v.imag * h.imag))
        d = nDotH * nDotH * (a2 - 1.0) + 1.0
        d = a2 / (math.pi * d * d)
        d = d * g * (nDotL / (nDotL * (1.0 - k) + k))
        w += d
        s += (fresnel(vDotH, n1, n2) * d)
        l *= f
    return (s / w)

def fresnelRoughSimple(nDotV, n1, n2, r):
    f0 = fresnel(1.0, n1, n2)
    f = fresnel(nDotV, n1, n2)
    f = max(min((f - f0) / (1.0 - f0), 1.0), 0.0)
    return (f0 + ((max(f0, 1.0 - r) - f0) * f))

def fresnelRoughTable(nDotV, n1, n2, r):
    f0 = fresnel(1.0, n1, n2)
    n = int(max(min((0.96875 - r) * 255.0 / 0.9375, 255.0), 0.0) + 0.5) << 2
    a3 = frTable[n]
    a2 = frTable[n + 1]
    a1 = frTable[n + 2]
    a0 = frTable[n + 3]
    f = ((nDotV * a3 + a2) * nDotV + a1) * nDotV + a0
    f = f * f * (1.0 - f0) + f0
    return f

# plot the data
fig = plt.figure()
ax = fig.add_subplot(1, 1, 1)

roughness = [0.03125, 0.18750, 0.34375, 0.50000, 0.65625, 0.81250, 0.96875]
colors = ['tab:blue', 'tab:cyan', 'tab:green', 'tab:olive',
          'tab:orange', 'tab:red', 'tab:pink']
linestyles = ['solid', 'dashed', 'dotted']

for i in range(len(roughness)):
    r = roughness[i]
    for j in range(3):
        xdata = []
        ydata = []
        for x in range(901):
            xf = float(x) / 10.0
            xdata += [xf]
            nDotV = math.cos(xf * math.pi / 180.0)
            if j == 0:
                ydata += [fresnelGGX(nDotV, 1.0, 1.5, r)]
            elif j == 1:
                ydata += [fresnelRoughTable(nDotV, 1.0, 1.5, r)]
            else:
                ydata += [fresnelRoughSimple(nDotV, 1.0, 1.5, r)]
        ax.plot(xdata, ydata, color = colors[i], linestyle = linestyles[j])

# set the limits
ax.set_xlim([0.0, 90.0])
ax.set_ylim([-0.01, 1.01])
ax.grid(True)
# plt.legend()

# display the plot
plt.show()

