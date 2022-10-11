
import os, sys

env = Environment(ENV = { "PATH" : os.environ["PATH"],
                          "HOME" : os.environ["HOME"] })
env["CXXFLAGS"] = Split("-Wall -std=c++11 -Isrc")
env.Append(CXXFLAGS = Split("-march=sandybridge -mtune=generic"))
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
    env.Append(CXXFLAGS = Split("-O3 -fomit-frame-pointer -ffast-math"))
    env.Append(LINKFLAGS = ["-s"])
if ARGUMENTS.get("rgb10a2", 0):
    env.Append(CXXFLAGS = ["-DUSE_PIXELFMT_RGB10A2=1"])

libSources = ["src/common.cpp", "src/filebuf.cpp", "src/zlib.cpp"]
libSources += ["src/ba2file.cpp", "src/esmfile.cpp", "src/stringdb.cpp"]
libSources += ["src/btdfile.cpp", "src/ddstxt.cpp", "src/esmdbase.cpp"]
libSources += ["src/nif_file.cpp", "src/bgsmfile.cpp", "src/landdata.cpp"]
libSources += ["src/plot3d.cpp", "src/landtxt.cpp", "src/terrmesh.cpp"]
libSources += ["src/rndrbase.cpp"]
fo76utilsLib = env.StaticLibrary("fo76utils", libSources)
env.Prepend(LIBS = [fo76utilsLib])
nifViewEnv = env.Clone()
buildCubeView = True
try:
    sdlPackageName = ["sdl2", "SDL2"][int("win" in sys.platform)]
    nifViewEnv.ParseConfig("pkg-config --cflags --libs " + sdlPackageName)
    nifViewEnv.Append(CXXFLAGS = ["-DHAVE_SDL2=1"])
except:
    buildCubeView = False
sdlVideoLib = nifViewEnv.StaticLibrary("sdlvideo",
                                       ["src/nif_view.cpp", "src/sdlvideo.cpp"])
nifViewEnv.Prepend(LIBS = [sdlVideoLib])

baunpack = env.Program("baunpack", ["src/baunpack.cpp"])
bcdecode = env.Program("bcdecode", ["src/bcdecode.cpp"])
btddump = env.Program("btddump", ["src/btddump.cpp"])
esmdump = env.Program("esmdump", ["src/esmdump.cpp"])
findwater = env.Program("findwater", ["src/findwater.cpp"])
fo4land = env.Program("fo4land", ["src/fo4land.cpp"])
landtxt = env.Program("landtxt", ["src/ltxtmain.cpp"])
markers = env.Program("markers", ["src/markers.cpp"])
nif_info = nifViewEnv.Program("nif_info", ["src/nif_info.cpp"])
esm_view_o = env.Object("esm_view", ["src/esmview.cpp"])
sdlvstub_o = env.Object("sdlvstub", ["src/sdlvideo.cpp"])
esm_view = env.Program("esm_view", [esm_view_o, sdlvstub_o])
esmview_o = nifViewEnv.Object("esmview", ["src/esmview.cpp"])
esmview = nifViewEnv.Program("esmview", [esmview_o])
if buildCubeView:
    cubeview = nifViewEnv.Program("cubeview", ["src/cubeview.cpp"])
render = env.Program("render", ["src/render.cpp"])
terrain = env.Program("terrain", ["src/terrain.cpp"])

if "win" in sys.platform:
    if buildPackage:
        pkgFiles = [baunpack, bcdecode, btddump, esmdump, esmview, findwater]
        pkgFiles += [fo4land, landtxt, markers, nif_info, render, terrain]
        if buildCubeView:
            pkgFiles += [cubeview]
            pkgFiles += ["/mingw64/bin/SDL2.dll", "LICENSE.SDL"]
        pkgFiles += ["/mingw64/bin/libwinpthread-1.dll"]
        pkgFiles += ["/mingw64/bin/libgcc_s_seh-1.dll"]
        pkgFiles += ["/mingw64/bin/libstdc++-6.dll"]
        pkgFiles += Split("doc ltex scripts src SConstruct SConstruct.maps")
        pkgFiles += Split(".gitignore LICENSE README.md README.mman-win32")
        pkgFiles += Split("mapicons.py tes5cell.txt")
        package = env.Command(
                      "../fo76utils-" + str(buildPackage) + ".7z", pkgFiles,
                      "7za a -m0=lzma -mx=9 -x!src/*.o $TARGET $SOURCES")

