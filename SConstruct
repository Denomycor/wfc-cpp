import os
from SCons.Script import *

# ---- Build mode ----
mode = ARGUMENTS.get("mode", "release")

# ---- Directories (separate per mode to avoid stale objects) ----
SRC_DIR = "src"
OBJ_DIR = os.path.join("obj", mode)
LIB_DIR = os.path.join("bin", mode)

# ---- Base compiler flags ----
cxxflags = ["-std=c++17", "-Wall"]

if mode == "debug":
    cxxflags += ["-g", "-O0", "-fsanitize=address"]
    linkflags = ["-fsanitize=address"]
else:
    cxxflags += ["-O2"]
    linkflags = []

# ---- Environment ----
env = Environment(
    CXX="g++",
    CXXFLAGS=cxxflags,
    LINKFLAGS=linkflags,
    CPPPATH=["include"],   # wfc-cpp headers
)

# Enable compilation database generation
env.Tool("compilation_db")

# ---- Create output directories ----
env.Execute(Mkdir(LIB_DIR))

# ---- Variant directory ----
env.VariantDir(OBJ_DIR, SRC_DIR, duplicate=0)

# ---- Source files ----
sources = Glob(os.path.join(OBJ_DIR, "*.cpp"))

# ---- Build static library ----
lib = env.StaticLibrary(
    target=os.path.join(LIB_DIR, "libwfc"),
    source=sources
)

Default(lib)

# ---- Compile commands target ----
env.CompilationDatabase("compile_commands.json")

# Return the library object so the main SConstruct can link it
Return("lib")
