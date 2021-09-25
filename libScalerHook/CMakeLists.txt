# Compile LibPltHook
project(ScalerHook)

# Set global (Relative this folder until reset) compiler options
set(CMAKE_C_STANDARD 11)

# https://json.nlohmann.me/integration/cmake/
set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(lib/json)
add_subdirectory(lib/watcher)

set(ScalerHookSrc
        src/ExtFuncCallHook_Linux.cpp
        src/ExtFuncCallHookAsm.cpp
        src/ExtFuncCallHookPtrace.cpp
        src/ExtFuncCallHookBrkpoint.cpp
        src/ProcInfoParser.cpp
        src/ElfParser.cpp
        src/MemTool.cpp
        src/MemTool_Linux.cpp
        src/MemToolPtrace.cpp
        src/FileTool.cpp
        src/install.cpp
        src/Timer.cpp
        src/Config.cpp
        src/Serilizable.cpp
        src/VMEmulator.cpp
        src/InvocationTree.cpp
        src/StringTool.cpp
        src/breakpoint.cpp)

list(APPEND CMAKE_PREFIX_PATH "/media/umass/datasystem/steven/Downloads/intel/build/kits/xed-install-base-2021-08-26-lin-x86-64" )
find_package(XED REQUIRED)


# Scaler hook library itself. Users need to manually install scalerhook through library functions.
add_library(ScalerHook-HookManualAsm SHARED
            ${ScalerHookSrc})
target_include_directories(ScalerHook-HookManualAsm PUBLIC src/include lib/inireader)
target_link_libraries(ScalerHook-HookManualAsm PUBLIC pthread dl nlohmann_json::nlohmann_json xed2 xed2-ild)
target_compile_definitions(ScalerHook-HookManualAsm PUBLIC XED_ENCODER)

add_executable(ScalerHook-HookManualPtrace
        src/PtraceHookMain.cpp
        ${ScalerHookSrc}
       )
target_include_directories(ScalerHook-HookManualPtrace PUBLIC src/include lib/inireader)
target_link_libraries(ScalerHook-HookManualPtrace PUBLIC pthread dl nlohmann_json::nlohmann_json xed2 xed2-ild)
target_compile_definitions(ScalerHook-HookManualPtrace PUBLIC XED_ENCODER)


add_library(ScalerHook-HookBrkpoint SHARED
        ${ScalerHookSrc})
target_include_directories(ScalerHook-HookBrkpoint PUBLIC src/include lib/inireader)
target_compile_options(ScalerHook-HookBrkpoint PRIVATE "--verbose;")
target_link_libraries(ScalerHook-HookBrkpoint PUBLIC pthread dl nlohmann_json::nlohmann_json xed2 xed2-ild)
target_compile_definitions(ScalerHook-HookBrkpoint PUBLIC XED_ENCODER)



#ScalerHook with auto installation enabled. Scaler hook will run automatically before main.
add_library(ScalerHook-HookAuto SHARED src/autoinstall.cpp)
target_compile_options(ScalerHook-HookAuto PRIVATE "--verbose")
target_link_libraries(ScalerHook-HookAuto PUBLIC ScalerHook-HookManualAsm)

add_library(ScalerHook-HookAuto-Brkpoint SHARED src/autoinstall.cpp)
target_compile_options(ScalerHook-HookAuto-Brkpoint PRIVATE "--verbose")
target_link_libraries(ScalerHook-HookAuto-Brkpoint PUBLIC ScalerHook-HookBrkpoint)

#Add tests
add_subdirectory(tests)