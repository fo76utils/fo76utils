
import os, sys

env = Environment(tools = [*filter(None, ARGUMENTS.get('tools','').split(','))] or None,
                  ENV = { "PATH" : os.environ["PATH"],
                          "HOME" : os.environ["HOME"] })
env["CCFLAGS"] = Split("-Wall -Isrc")
env["CFLAGS"] = Split("-std=c99")
env["CXXFLAGS"] = Split("-std=c++11")
if int(ARGUMENTS.get("avx2", 0)):
    env.Append(CCFLAGS = ["-march=haswell"])
elif int(ARGUMENTS.get("avx", 1)):
    env.Append(CCFLAGS = ["-march=sandybridge"])
env.Append(CCFLAGS = ["-mtune=generic"])
env.Append(LIBS = ["m"])
if "win" in sys.platform:
    buildPackage = ARGUMENTS.get("buildpkg", "")
else:
    env.Append(LIBS = ["pthread"])
if int(ARGUMENTS.get("debug", 0)):
    env.Append(CCFLAGS = Split("-g -Og"))
elif int(ARGUMENTS.get("profile", 0)):
    env.Append(CCFLAGS = Split("-g -pg -O"))
    env.Prepend(LINKFLAGS = ["-pg"])
else:
    env.Append(CCFLAGS = Split("-O3 -fomit-frame-pointer -ffast-math"))
    env.Append(LINKFLAGS = ["-s"])
if int(ARGUMENTS.get("rgb10a2", 0)):
    env.Append(CCFLAGS = ["-DUSE_PIXELFMT_RGB10A2=1"])

libSources = ["src/common.cpp", "src/filebuf.cpp", "src/zlib.cpp"]
libSources += ["src/ba2file.cpp", "src/esmfile.cpp", "src/stringdb.cpp"]
libSources += ["src/btdfile.cpp", "src/ddstxt.cpp", "src/downsamp.cpp"]
libSources += ["src/esmdbase.cpp", "src/nif_file.cpp", "src/bgsmfile.cpp"]
libSources += ["src/landdata.cpp", "src/plot3d.cpp", "src/landtxt.cpp"]
libSources += ["src/terrmesh.cpp", "src/render.cpp", "src/rndrbase.cpp"]
libSources += ["src/markers.cpp"]
# detex source files
libSources += ["src/bits.c", "src/bptc-tables.c", "src/decompress-bptc.c"]
fo76utilsLib = env.StaticLibrary("fo76utils", libSources)

if int(ARGUMENTS.get("pymodule", 0)):
    pyModuleEnv = env.Clone()
    if "win" in sys.platform:
        pyModuleEnv.ParseConfig("pkg-config --cflags --libs python3")
        pyModuleEnv["SHLIBSUFFIX"] = ".pyd"
    else:
        pyModuleEnv.ParseConfig("pkg-config --cflags --libs python-3.8")
    pyModuleEnv["SHLIBPREFIX"] = "_"
    pyModuleEnv["SWIGFLAGS"] = Split("-c++ -python -Isrc")
    pythonInterface = pyModuleEnv.SharedLibrary(
                          "scripts/fo76utils",
                          ["scripts/fo76utils.i"] + libSources)

env.Prepend(LIBS = [fo76utilsLib])
nifViewEnv = env.Clone()
buildCubeView = True
try:
    try:
        nifViewEnv.ParseConfig("pkg-config --short-errors --cflags --libs sdl2")
    except:
        nifViewEnv.ParseConfig("pkg-config --short-errors --cflags --libs SDL2")
    nifViewEnv.Append(CCFLAGS = ["-DHAVE_SDL2=1"])
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
markers = env.Program("markers", ["src/markmain.cpp"])
nif_info = nifViewEnv.Program("nif_info", ["src/nif_info.cpp"])
esm_view_o = env.Object("esm_view", ["src/esmview.cpp"])
sdlvstub_o = env.Object("sdlvstub", ["src/sdlvideo.cpp"])
esm_view = env.Program("esm_view", [esm_view_o, sdlvstub_o])
esmview_o = nifViewEnv.Object("esmview", ["src/esmview.cpp"])
esmview = nifViewEnv.Program("esmview", [esmview_o])
if buildCubeView:
    cubeview = nifViewEnv.Program("cubeview", ["src/cubeview.cpp"])
    wrldview = nifViewEnv.Program("wrldview", ["src/wrldview.cpp"])
render = env.Program("render", ["src/rndrmain.cpp"])
terrain = env.Program("terrain", ["src/terrain.cpp"])

if "win" in sys.platform:
    if buildPackage:
        pkgFiles = [baunpack, bcdecode, btddump, esmdump, esmview, esm_view]
        pkgFiles += [findwater, fo4land, landtxt, markers, nif_info, render]
        pkgFiles += [terrain]
        if buildCubeView:
            pkgFiles += [cubeview, wrldview]
            pkgFiles += ["/mingw64/bin/SDL2.dll", "LICENSE.SDL"]
        pkgFiles += ["/mingw64/bin/libwinpthread-1.dll"]
        pkgFiles += ["/mingw64/bin/libgcc_s_seh-1.dll"]
        pkgFiles += ["/mingw64/bin/libstdc++-6.dll"]
        pkgFiles += Split("doc ltex scripts src SConstruct SConstruct.maps")
        pkgFiles += Split(".gitignore LICENSE README.md README.mman-win32")
        pkgFiles += Split("README.detex mapicons.py tes5cell.txt")
        package = env.Command(
                      "../fo76utils-" + str(buildPackage) + ".7z", pkgFiles,
                      "7za a -m0=lzma -mx=9 -x!src/*.o $TARGET $SOURCES")

