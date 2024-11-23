#!/usr/bin/python3

import sys
import matplotlib
if sys.platform[:5] == "linux":
    matplotlib.use('TkAgg')
import matplotlib.pyplot as plt
import numpy as np
import math

x0 = -1.0
x1 = 1.0
y0 = 0.0
y1 = 2.0

def solveEquationSystem(m):
    h = len(m)
    w = len(m[0])
    # Gaussian elimination
    for i in range(h):
        a = m[i][i]
        l = i
        for j in range(i + 1, h):
            if abs(m[j][i]) > abs(a):
                a = m[j][i]
                l = j
        if l != i:
            tmp = m[i]
            m[i] = m[l]
            m[l] = tmp
        for j in range(w):
            m[i][j] = m[i][j] / a
        m[i][i] = 1.0
        for j in range(i + 1, h):
            a = m[j][i]
            for k in range(w):
                m[j][k] = m[j][k] - (m[i][k] * a)
            m[j][i] = 0.0
    for i in range(h - 1, -1, -1):
        for j in range(i - 1, -1, -1):
            a = m[j][i]
            for k in range(w):
                m[j][k] = m[j][k] - (m[i][k] * a)
            m[j][i] = 0.0
    return m

a = []
k = -1

if len(sys.argv) >= 9 and (len(sys.argv) & 1) == 1:
    k = (len(sys.argv) - 3) >> 1
    m = []
    for i in range(k + 1):
        m += [[]]
    for i in range(k + 1):
        m[i] += [1.0, float(sys.argv[(i << 1) + 1])]
        for j in range(2, k + 1):
            m[i] += [m[i][j - 1] * m[i][1]]
        m[i] += [float(sys.argv[(i << 1) + 2])]
    m = solveEquationSystem(m)
    for i in range(k + 1):
        a += [m[i][k + 1]]
    print(a)
else:
    k = len(sys.argv) - 2
    for i in range(k + 1, 0, -1):
        a += [float(sys.argv[i])]

xdata = []
ydata = []

for i in range(1025):
    x = (i / 1024.0) * (x1 - x0) + x0
    xdata += [x]
    y = 0.0
    for j in range(k, -1, -1):
        y = y * x + a[j]
    ydata += [y]

# plot the data
fig = plt.figure()
ax = fig.add_subplot(1, 1, 1)
ax.plot(xdata, ydata, color='tab:blue')

# set the limits
ax.set_xlim([x0, x1])
ax.set_ylim([y0, y1])
ax.grid(True)

# display the plot
plt.show()

