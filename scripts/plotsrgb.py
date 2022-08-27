#!/usr/bin/python3

import sys
import matplotlib
if sys.platform[:5] == "linux":
    matplotlib.use('TkAgg')
import matplotlib.pyplot as plt
import numpy as np
import math

x0 = 0.0
x1 = 1.0
y0 = -0.01
y1 = 1.01

xdata = []
ydata = [[], [], [], [], [], [], []]

def fromSRGB(x):
    y = x / 12.92
    if y > 0.0031308:
        y = math.pow((x + 0.055) / 1.055, 2.4)
    return y

def toSRGB(x):
    if x > 0.0031308:
        return math.pow(x, 1.0 / 2.4) * 1.055 - 0.055
    return x * 12.92

for i in range(1025):
    x = (i / 1024.0) * (x1 - x0) + x0
    x = max(x, 0.0)
    xdata += [x]
    ydata[0] += [toSRGB(x)]
    y1 = math.sqrt(x)
    y1 = (y1 * -0.13942692 + 1.13942692) * y1
    ydata[1] += [y1]
    ydata[2] += [fromSRGB(x)]
    y3 = (x * 0.13945550 + 0.86054450) * x
    y3 = y3 * y3
    ydata[3] += [y3]
    ydata[4] += [x]
    y5 = math.sqrt(max(y3, 0.0))
    y5 = (y5 * -0.13942692 + 1.13942692) * y5
    ydata[5] += [y5]
    y6 = x * x
    y6 = (y6 * 0.16916572 + 0.83083428) * y6
    ydata[6] += [y6]

# plot the data
fig = plt.figure()
ax = fig.add_subplot(1, 1, 1)
ax.plot(xdata, ydata[0], color='tab:blue', label='Linear -> sRGB')
ax.plot(xdata, ydata[1], color='tab:red', label='FloatVector4::srgbCompress()')
ax.plot(xdata, ydata[2], color='tab:cyan', label='sRGB -> Linear')
ax.plot(xdata, ydata[3], color='tab:orange', label='FloatVector4::srgbExpand()')
ax.plot(xdata, ydata[4], color='tab:green', label='Linear')
ax.plot(xdata, ydata[5], color='tab:olive', label='sRGB -> Linear -> sRGB')
ax.plot(xdata, ydata[6], color='tab:pink', label='FloatVector4 sRGB bilinear')

# set the limits
ax.set_xlim([x0, x1])
ax.set_ylim([y0, y1])
ax.grid(True)
plt.legend()

# display the plot
plt.show()

