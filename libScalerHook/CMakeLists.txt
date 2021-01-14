# Compile LibPltHook
project(scalerhook)

add_library(scalerhook SHARED
        src/ExtFuncCallHook.cpp
        src/PMParser.cpp
        src/ElfParser.cpp src/FileTool.cpp)