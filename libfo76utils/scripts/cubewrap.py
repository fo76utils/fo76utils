#!/usr/bin/python3

import math

def cubeCoordToVector(x, y, w, n):
    v = [float(w), float(w), float(w)]
    if n == 0:
        v[1] = float((w - 1) - (y + y))
        v[2] = float((w - 1) - (x + x))
    elif n == 1:
        v[0] = float(-w)
        v[1] = float((w - 1) - (y + y))
        v[2] = float((x + x) - (w - 1))
    elif n == 2:
        v[0] = float((x + x) - (w - 1))
        v[2] = float((y + y) - (w - 1))
    elif n == 3:
        v[0] = float((x + x) - (w - 1))
        v[1] = float(-w)
        v[2] = float((w - 1) - (y + y))
    elif n == 4:
        v[0] = float((x + x) - (w - 1))
        v[1] = float((w - 1) - (y + y))
    else:
        v[0] = float((w - 1) - (x + x))
        v[1] = float((w - 1) - (y + y))
        v[2] = float(-w)
    tmp = math.sqrt((v[0] * v[0]) + (v[1] * v[1]) + (v[2] * v[2]))
    v[0] = v[0] / tmp
    v[1] = v[1] / tmp
    v[2] = v[2] / tmp
    return v[0], v[1], v[2]

def vectorToCubeCoord(x, y, z, w):
    if abs(x) > abs(y) and abs(x) > abs(z):
        if x > 0.0:                     # face 0: E
            u = -z / abs(x)
            v = -y / abs(x)
            n = 0
        else:                           # face 1: W
            u = z / abs(x)
            v = -y / abs(x)
            n = 1
    elif abs(y) > abs(x) and abs(y) > abs(z):
        if y > 0.0:                     # face 2: N
            u = x / abs(y)
            v = z / abs(y)
            n = 2
        else:                           # face 3: S
            u = x / abs(y)
            v = -z / abs(y)
            n = 3
    else:
        if z > 0.0:                     # face 4: top
            u = x / abs(z)
            v = -y / abs(z)
            n = 4
        else:                           # face 5: bottom
            u = -x / abs(z)
            v = -y / abs(z)
            n = 5
    return (u + 1.0) * float(w) * 0.5 - 0.5, (v + 1.0) * float(w) * 0.5 - 0.5, n

cubeWrapTable = []

for i in range(24):
    cubeWrapTable += [0]

for n in range(6):
    for i in range(4):
        xi = (1 - ((i & 1) << 1)) & (((i & 2) >> 1) - 1)
        yi = (1 - ((i & 1) << 1)) & -((i & 2) >> 1)
        x = 64.0
        y = 64.0
        if xi != 0:
            x = 127.75 + (129.25 * xi)
        if yi != 0:
            y = 127.75 + (129.25 * yi)
        vx, vy, vz = cubeCoordToVector(x, y, 256, n)
        xc, yc, nn = vectorToCubeCoord(vx, vy, vz, 256)
        x = math.fmod(x + 256.5, 256.0) - 0.5
        y = math.fmod(y + 256.5, 256.0) - 0.5
        # print("%f, %f, %d -> %f, %f, %d" % (x, y, n, xc, yc, nn))
        if (xc >= 32.0 and xc < 224.0) != (x >= 32.0 and x < 224.0):
            nn = nn | 0x80
            tmp = xc
            xc = yc
            yc = tmp
        if (xc < 128.0) != (x < 128.0):
            nn = nn | 0x20
        if (yc < 128.0) != (y < 128.0):
            nn = nn | 0x40
        cubeWrapTable[(n * 4) + i] = nn

cubeFaceNames = ["+X", "-X", "+Y", "-Y", "+Z", "-Z"]
s = "\nconst unsigned char DDSTexture::cubeWrapTable[24] =\n{\n  "
s += "// value = new_face + mirror_U * 0x20 + mirror_V * 0x40 + swap_UV * 0x80"
s += "\n  // +U   -U     +V    -V\n"
for n in range(6):
    for i in range(4):
        if (i & 1) == 0:
            s += " "
        nn = (n * 4) + i
        s += " 0x%02X" % (cubeWrapTable[nn])
        if not (n == 5 and i == 3):
            s += ","
        else:
            s += " "
        if i == 3:
            s += "      // face %d (%s)" % (n, cubeFaceNames[n])
    s += "\n"
s += "};\n"
print(s)

