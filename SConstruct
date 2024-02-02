
import os, sys

env = Environment(tools = [*filter(None, ARGUMENTS.get('tools','').split(','))] or None,
                  ENV = { "PATH" : os.environ["PATH"],
                          "HOME" : os.environ["HOME"] })
env["CCFLAGS"] = Split("-Wall -Isrc -Ilibfo76utils/src -DBUILD_CE2UTILS")
env["CFLAGS"] = Split("-std=c99")
env["CXXFLAGS"] = Split("-std=c++20")
avxLevel = int(ARGUMENTS.get("avx", 2))
avx2Flag = int(ARGUMENTS.get("avx2", 1))
if avxLevel > 1 and avx2Flag > 0:
    env.Append(CCFLAGS = ["-march=haswell"])
elif avxLevel > 0:
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
cdbDebugLevel = int(ARGUMENTS.get("matdebug", -1))
if cdbDebugLevel >= 0:
    env.Append(CCFLAGS = ["-DENABLE_CDB_DEBUG=%d" % (cdbDebugLevel)])

libSources = ["libfo76utils/src/common.cpp", "libfo76utils/src/filebuf.cpp"]
libSources += ["libfo76utils/src/zlib.cpp", "libfo76utils/src/ba2file.cpp"]
libSources += ["libfo76utils/src/esmfile.cpp", "libfo76utils/src/stringdb.cpp"]
libSources += ["src/btdfile.cpp", "libfo76utils/src/ddstxt.cpp"]
libSources += ["libfo76utils/src/downsamp.cpp", "src/esmdbase.cpp"]
libSources += ["src/nif_file.cpp", "src/meshfile.cpp"]
libSources += ["libfo76utils/src/bsrefl.cpp", "libfo76utils/src/material.cpp"]
libSources += ["libfo76utils/src/matcomps.cpp", "libfo76utils/src/mat_dump.cpp"]
libSources += ["libfo76utils/src/bsmatcdb.cpp", "src/markers.cpp"]
libSources += ["src/landdata.cpp", "src/landtxt.cpp", "src/terrmesh.cpp"]
libSources += ["src/plot3d.cpp", "src/render.cpp", "src/rndrbase.cpp"]
libSources += ["libfo76utils/src/sfcube.cpp", "libfo76utils/src/ddstxt16.cpp"]
# detex source files
libSources += ["libfo76utils/src/bits.c", "libfo76utils/src/bptc-tables.c"]
libSources += ["libfo76utils/src/decompress-bptc.c"]
libSources += ["libfo76utils/src/decompress-bptc-float.c"]
ce2utilsLib = env.StaticLibrary("ce2utils", libSources)

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
                          "scripts/ce2utils",
                          ["scripts/ce2utils.i"] + libSources)

env.Prepend(LIBS = [ce2utilsLib])
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
                                       ["src/nif_view.cpp", "libfo76utils/src/sdlvideo.cpp"])
nifViewEnv.Prepend(LIBS = [sdlVideoLib])

baunpack = env.Program("baunpack", ["src/baunpack.cpp"])
bcdecode = env.Program("bcdecode", ["src/bcdecode.cpp"])
btddump = env.Program("btddump", ["src/btddump.cpp"])
esmdump = env.Program("esmdump", ["src/esmdump.cpp"])
findwater = env.Program("findwater", ["src/findwater.cpp"])
landtxt = env.Program("landtxt", ["src/ltxtmain.cpp"])
markers = env.Program("markers", ["src/markmain.cpp"])
mat_info = nifViewEnv.Program("mat_info", ["src/mat_info.cpp"])
nif_info = nifViewEnv.Program("nif_info", ["src/nif_info.cpp"])
esm_view_o = env.Object("esm_view", ["src/esmview.cpp"])
sdlvstub_o = env.Object("sdlvstub", ["libfo76utils/src/sdlvideo.cpp"])
esm_view = env.Program("esm_view", [esm_view_o, sdlvstub_o])
esmview_o = nifViewEnv.Object("esmview", ["src/esmview.cpp"])
esmview = nifViewEnv.Program("esmview", [esmview_o])
if buildCubeView:
    cubeview = nifViewEnv.Program("cubeview", ["src/cubeview.cpp"])
    wrldview = nifViewEnv.Program("wrldview", ["src/wrldview.cpp"])
render = env.Program("render", ["src/rndrmain.cpp"])
terrain = env.Program("terrain", ["src/terrain.cpp"])

if ("win" in sys.platform) and buildPackage:
    pkgFiles = [baunpack, bcdecode, btddump, esmdump, esmview, esm_view]
    pkgFiles += [findwater, landtxt, markers, mat_info, nif_info]
    pkgFiles += [render, terrain]
    if buildCubeView:
        pkgFiles += [cubeview, wrldview]
        pkgFiles += ["/mingw64/bin/SDL2.dll", "LICENSE.SDL"]
    pkgFiles += ["/mingw64/bin/libwinpthread-1.dll"]
    pkgFiles += ["/mingw64/bin/libgcc_s_seh-1.dll"]
    pkgFiles += ["/mingw64/bin/libstdc++-6.dll"]
    pkgFiles += Split("doc libfo76utils ltex scripts src SConstruct")
    pkgFiles += Split("SConstruct.maps .github .gitignore LICENSE README.detex")
    pkgFiles += Split("README.md README.mman-win32 mapicons.py tes5cell.txt")
    package = env.Command(
                  "ce2utils-" + str(buildPackage) + ".7z", pkgFiles,
                  "7za a -m0=lzma -mx=9 -x!*/*.o -x!*/*/*.o $TARGET $SOURCES")

