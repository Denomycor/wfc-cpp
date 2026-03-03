import os
from SCons.Script import *

# ---- Build mode ----
mode = ARGUMENTS.get("mode", "release")

# ---- Compiler flags ----
cxxflags = ["-std=c++17", "-Wall"]

if mode == "debug":
    cxxflags.append("-g")
    # cxxflags.append("-fsanitize=address")

env = Environment(
    CXX="g++",
    CXXFLAGS=cxxflags,
    CPPPATH=["include"],
)

# Enable compilation database generation
env.Tool("compilation_db")

# ---- Directories ----
SRC_DIR = "src"
OBJ_DIR = "obj"
BIN_DIR = "bin"

env.VariantDir(OBJ_DIR, SRC_DIR, duplicate=0)

sources = Glob(os.path.join(OBJ_DIR, "*.cpp"))

# Build program
program = env.Program(
    target=os.path.join(BIN_DIR, "main"),
    source=sources
)

env.Execute(Mkdir(BIN_DIR))

Default(program)

# ---- Compile commands target ----
compdb = env.CompilationDatabase("compile_commands.json")

