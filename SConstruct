
import os, sys

env = Environment(ENV = { "PATH" : os.environ["PATH"],
                          "HOME" : os.environ["HOME"] })
env["CXXFLAGS"] = "-Wall -std=c++11 -Isrc -O3 -fomit-frame-pointer -ffast-math"
if "win" in sys.platform:
    env.Append(LINKFLAGS = ["-static", "-s"])
    env.Append(LIBS = ["m"])
else:
    env.Append(LINKFLAGS = ["-s"])
    env.Append(LIBS = ["m", "pthread"])

libSources = ["src/common.cpp", "src/filebuf.cpp", "src/zlib.cpp"]
libSources += ["src/ba2file.cpp", "src/esmfile.cpp", "src/stringdb.cpp"]
libSources += ["src/btdfile.cpp", "src/ddstxt.cpp", "src/esmdbase.cpp"]
libSources += ["src/nif_file.cpp", "src/landdata.cpp", "src/plot3d.cpp"]
fo76utilsLib = env.StaticLibrary("fo76utils", libSources)
env.Prepend(LIBS = [fo76utilsLib])

baunpack = env.Program("baunpack", ["src/baunpack.cpp"])
bcdecode = env.Program("bcdecode", ["src/bcdecode.cpp"])
btddump = env.Program("btddump", ["src/btddump.cpp"])
esmdump = env.Program("esmdump", ["src/esmdump.cpp"])
esmview = env.Program("esmview", ["src/esmview.cpp"])
findwater = env.Program("findwater", ["src/findwater.cpp"])
fo4land = env.Program("fo4land", ["src/fo4land.cpp"])
landtxt = env.Program("landtxt", ["src/landtxt.cpp"])
markers = env.Program("markers", ["src/markers.cpp"])
nif_info = env.Program("nif_info", ["src/nif_info.cpp"])
terrain = env.Program("terrain", ["src/terrain.cpp"])

