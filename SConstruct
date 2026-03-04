import os
from SCons.Script import *

# ---- Build mode ----
mode = ARGUMENTS.get("mode", "release")

# ---- Directories (separate per mode to avoid stale objects) ----
SRC_DIR = "src"
OBJ_DIR = os.path.join("obj", mode)
BIN_DIR = os.path.join("bin", mode)

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
    CPPPATH=["include"],
)

# Enable compilation database generation
env.Tool("compilation_db")

# ---- Create directories ----
env.Execute(Mkdir(BIN_DIR))

# ---- Variant directory ----
env.VariantDir(OBJ_DIR, SRC_DIR, duplicate=0)

# ---- Source files ----
sources = Glob(os.path.join(OBJ_DIR, "*.cpp"))

# ---- Build program ----
program = env.Program(
    target=os.path.join(BIN_DIR, "main"),
    source=sources
)

Default(program)

# ---- Compile commands target ----
env.CompilationDatabase("compile_commands.json")
