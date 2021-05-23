# Compile LibPltHook
project(ScalerHook)

# Set compiler options
set(CMAKE_CXX_FLAGS "--verbose -O0 -pthread -ldl -znow -fPIE -fPIC -std=c++11")

# Scaler hook library itself. Users need to manually install scalerhook through library functions.
add_library(ScalerHook
        src/ExtFuncCallHook.cpp
        src/ProcInfoParser.cpp
        src/ElfParser.cpp
        src/MemTool.cpp
        src/FileTool.cpp
        src/install.cpp)
target_include_directories(ScalerHook PUBLIC src/include)

#ScalerHook with auto installation enabled. Scaler hook will run automatically before main.
add_library(ScalerHookWithAutoInstall src/autoinstall.cpp)
target_link_libraries(ScalerHookWithAutoInstall PUBLIC ScalerHook)
