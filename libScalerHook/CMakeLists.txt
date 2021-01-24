# Compile LibPltHook
project(scalerhook)

add_library(scalerhook SHARED
        src/ExtFuncCallHook.cpp
        src/ProcInfoParser.cpp
        src/ElfParser.cpp
        src/MemTool.cpp
        src/FileTool.cpp)

add_library(scalerhook_installer SHARED
        src/install.cpp)
target_link_libraries(scalerhook_installer scalerhook)
