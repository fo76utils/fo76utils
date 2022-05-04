
import os, sys

env = Environment(ENV = { "PATH" : os.environ["PATH"],
                          "HOME" : os.environ["HOME"] })
env["CXXFLAGS"] = Split("-Wall -std=c++11 -Isrc")
env.Append(LIBS = ["m"])
if "win" in sys.platform:
    buildPackage = ARGUMENTS.get("buildpkg", "")
else:
    env.Append(LIBS = ["pthread"])
if ARGUMENTS.get("debug", 0):
    env.Append(CXXFLAGS = Split("-g -Og"))
elif ARGUMENTS.get("profile", 0):
    env.Append(CXXFLAGS = Split("-g -pg -O"))
    env.Prepend(LINKFLAGS = ["-pg"])
else:
    env.Append(CXXFLAGS = Split("-O3 -march=sandybridge -mtune=generic -fomit-frame-pointer -ffast-math"))
    env.Append(LINKFLAGS = ["-s"])

libSources = ["src/common.cpp", "src/filebuf.cpp", "src/zlib.cpp"]
libSources += ["src/ba2file.cpp", "src/esmfile.cpp", "src/stringdb.cpp"]
libSources += ["src/btdfile.cpp", "src/ddstxt.cpp", "src/esmdbase.cpp"]
libSources += ["src/nif_file.cpp", "src/bgsmfile.cpp", "src/landdata.cpp"]
libSources += ["src/plot3d.cpp", "src/landtxt.cpp", "src/terrmesh.cpp"]
fo76utilsLib = env.StaticLibrary("fo76utils", libSources)
env.Prepend(LIBS = [fo76utilsLib])
nifViewEnv = env.Clone()
buildNIFView = True
try:
    sdlPackageName = ["sdl", "SDL"][int("win" in sys.platform)]
    nifViewEnv.ParseConfig("pkg-config --cflags --libs " + sdlPackageName)
    nifViewEnv.Append(CXXFLAGS = ["-DHAVE_SDL=1"])
except:
    buildNIFView = False

baunpack = env.Program("baunpack", ["src/baunpack.cpp"])
bcdecode = env.Program("bcdecode", ["src/bcdecode.cpp"])
btddump = env.Program("btddump", ["src/btddump.cpp"])
esmdump = env.Program("esmdump", ["src/esmdump.cpp"])
esmview = env.Program("esmview", ["src/esmview.cpp"])
findwater = env.Program("findwater", ["src/findwater.cpp"])
fo4land = env.Program("fo4land", ["src/fo4land.cpp"])
landtxt = env.Program("landtxt", ["src/ltxtmain.cpp"])
markers = env.Program("markers", ["src/markers.cpp"])
if buildNIFView and not "win" in sys.platform:
    nif_info = nifViewEnv.Program("nif_info", ["src/nif_info.cpp"])
else:
    nif_info = env.Program("nif_info", ["src/nif_info.cpp"])
render = env.Program("render", ["src/render.cpp"])
terrain = env.Program("terrain", ["src/terrain.cpp"])

if "win" in sys.platform:
    if buildNIFView:
        nif_view_o = nifViewEnv.Object("nif_view", ["src/nif_info.cpp"])
        nif_view = nifViewEnv.Program("nif_view", nif_view_o)
    if buildPackage:
        pkgFiles = [baunpack, bcdecode, btddump, esmdump, esmview, findwater]
        pkgFiles += [fo4land, landtxt, markers, nif_info, render, terrain]
        if buildNIFView:
            pkgFiles += [nif_view, "/mingw64/bin/SDL.dll", "LICENSE.SDL"]
        pkgFiles += ["/mingw64/bin/libwinpthread-1.dll"]
        pkgFiles += ["/mingw64/bin/libgcc_s_seh-1.dll"]
        pkgFiles += ["/mingw64/bin/libstdc++-6.dll"]
        pkgFiles += Split("ltex scripts src SConstruct SConstruct.maps")
        pkgFiles += Split(".gitignore LICENSE README.md README.mman-win32")
        pkgFiles += Split("mapicons.py tes5cell.txt")
        package = env.Command(
                      "../fo76utils-" + str(buildPackage) + ".7z", pkgFiles,
                      "7za a -m0=lzma -mx=9 -x!src/*.o $TARGET $SOURCES")

