# Compile LibPltHook
project(scalerhook)

add_library(scalerhook SHARED
        src/ExtFuncCallHook.cpp
        src/ProcInfoParser.cpp
        src/ElfParser.cpp
        src/MemTool.cpp
        src/FileTool.cpp
        src/install.cpp)
