import os
from SCons.Script import *

Import("env")

# Clone parent environment so flags match godot-cpp
env = env.Clone()

# ---- Build mode ----
mode = ARGUMENTS.get("mode", "release")

# ---- Directories ----
SRC_DIR = "src"
OBJ_DIR = os.path.join("obj", mode)
LIB_DIR = os.path.join("bin", mode)

# ---- Compiler flags ----
cxxflags = ["-std=c++17", "-Wall"]

if mode == "debug":
    cxxflags += ["-g", "-O0", "-fsanitize=address"]
    linkflags = ["-fsanitize=address"]
else:
    cxxflags += ["-O2"]
    linkflags = []

env.Append(
    CXXFLAGS=cxxflags,
    LINKFLAGS=linkflags,
    CPPPATH=["include"]
)

# ---- Directories ----
env.Execute(Mkdir(OBJ_DIR))
env.Execute(Mkdir(LIB_DIR))

# ---- Variant build ----
env.VariantDir(OBJ_DIR, SRC_DIR, duplicate=0)

# ---- Sources ----
sources = Glob(os.path.join(OBJ_DIR, "*.cpp"))

# ---- Static library ----
lib = env.StaticLibrary(
    target=os.path.join(LIB_DIR, "libwfc"),
    source=sources
)

Return("lib")
