
import os, sys

env = Environment(ENV = { "PATH" : os.environ["PATH"],
                          "HOME" : os.environ["HOME"] })
env["CXXFLAGS"] = "-Wall -std=c++11 -Isrc -O3 -fomit-frame-pointer -ffast-math"
if "win" in sys.platform:
    env["LINKFLAGS"] = "-static -lm -s"
else:
    env["LINKFLAGS"] = "-lm -lpthread -s"

libSources = ["src/common.cpp", "src/filebuf.cpp", "src/zlib.cpp"]
libSources += ["src/ba2file.cpp", "src/esmfile.cpp", "src/stringdb.cpp"]
libSources += ["src/btdfile.cpp"]
libSourcesDDS = ["src/ddstxt.cpp"] + libSources
libSourcesESM = ["src/esmdbase.cpp"] + libSources

baunpack = env.Program("baunpack", ["src/baunpack.cpp"] + libSources)
bcdecode = env.Program("bcdecode", ["src/bcdecode.cpp"] + libSourcesDDS)
btddump = env.Program("btddump", ["src/btddump.cpp"] + libSources)
esmdump = env.Program("esmdump", ["src/esmdump.cpp"] + libSourcesESM)
esmview = env.Program("esmview", ["src/esmview.cpp"] + libSourcesESM)
findwater = env.Program("findwater", ["src/findwater.cpp"] + libSources)
fo4land = env.Program("fo4land", ["src/fo4land.cpp"] + libSources)
landtxt = env.Program("landtxt", ["src/landtxt.cpp"] + libSourcesDDS)
markers = env.Program("markers", ["src/markers.cpp"] + libSourcesDDS)
terrain = env.Program("terrain", ["src/terrain.cpp"] + libSources)

