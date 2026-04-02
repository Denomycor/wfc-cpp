#!/usr/bin/env python

import os

EnsureSConsVersion(4, 0)
EnsurePythonVersion(3, 8)

# Try importing an existing environment
try:
    Import("env")
except Exception:
    env = Environment(tools=["default", "compilation_db"])

if not env.get("COMPILATIONDB"):
    env.Tool("compilation_db")

env.Append(CXXFLAGS=["-Wall", "-O2"])
env.Append(CPPPATH=["include"])

# env.Append(CXXFLAGS=[
#     "-O0",
#     "-fno-omit-frame-pointer"
# ])

sources = Glob("src/*.cpp")

lib = env.StaticLibrary(
    target="libwfc",
    source=sources
)

env.CompilationDatabase("compile_commands.json")

Default(lib)
Return("lib")

