import os
from SCons.Script import *

Import("env")

env = env.Clone()

SRC_DIR = "src"
OBJ_DIR = "obj"
LIB_DIR = "bin"

env.VariantDir(OBJ_DIR, SRC_DIR, duplicate=0)

sources = Glob(os.path.join(OBJ_DIR, "*.cpp"))

lib = env.StaticLibrary(
    target=os.path.join(LIB_DIR, "libwfc"),
    source=sources
)

Return("lib")
