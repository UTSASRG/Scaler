# Compile LibPltHook
project(ScalerHook)


set(CMAKE_CXX_FLAGS "-pthread -ldl --verbose -znow -O0 -fPIE -fPIC -std=c++11")

add_library(ScalerHook
        src/ExtFuncCallHook.cpp
        src/ProcInfoParser.cpp
        src/ElfParser.cpp
        src/MemTool.cpp
        src/FileTool.cpp
        src/install.cpp)

add_library(ScalerHook_Installer src/autoinstall.cpp)
target_link_libraries(ScalerHook_Installer scalerhook)
