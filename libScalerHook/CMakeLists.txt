# Compile LibPltHook
project(scalerhook)

add_library(scalerhook SHARED
        src/ExtFuncCallHook.cpp
        src/PMParser.cpp
        src/ElfParser.cpp src/FileTool.cpp)

add_executable(TestRdebug
        src/rdbg.cpp)
target_link_libraries(TestRdebug scalerhook dl)