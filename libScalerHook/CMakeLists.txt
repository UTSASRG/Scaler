# Compile LibPltHook
project(scalerhook)

add_library(scalerhook SHARED
        src/ExtFuncCallHook.cpp
        src/ProcInfoParser.cpp
        src/ElfParser.cpp
        src/MemTool.cpp
        src/FileTool.cpp
        src/install.cpp)

add_library(scalerhook_installer SHARED
        src/autoinstall.cpp)
target_link_libraries(scalerhook_installer scalerhook)
