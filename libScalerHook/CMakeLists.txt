# Compile LibPltHook
project(ScalerHook)

# Set global (Relative this folder until reset) compiler options
set(CMAKE_C_STANDARD 11)

# https://json.nlohmann.me/integration/cmake/
set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(lib/json)
add_subdirectory(lib/watcher)

# Scaler hook library itself. Users need to manually install scalerhook through library functions.
add_library(ScalerHook-HookManualAsm SHARED
        src/ExtFuncCallHook_Linux.cpp
        src/ExtFuncCallHookAsm.cpp
        src/ExtFuncCallHookPtrace.cpp
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
        src/InvocationTree.cpp
        src/StringTool.cpp
        )
target_include_directories(ScalerHook-HookManualAsm PUBLIC src/include lib/inireader)
target_compile_options(ScalerHook-HookManualAsm PRIVATE "--verbose;-lazy;-fPIE;-fPIC;")
target_link_libraries(ScalerHook-HookManualAsm PRIVATE pthread dl nlohmann_json::nlohmann_json)

add_executable(ScalerHook-HookManualPtrace
        src/PtraceHookMain.cpp
        src/ExtFuncCallHook_Linux.cpp
        src/ExtFuncCallHookAsm.cpp
        src/ExtFuncCallHookPtrace.cpp
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
        src/InvocationTree.cpp
        src/StringTool.cpp
        )
target_include_directories(ScalerHook-HookManualPtrace PUBLIC src/include lib/inireader)
target_compile_options(ScalerHook-HookManualPtrace PRIVATE "--verbose;-fPIE;-fPIC;")
target_link_libraries(ScalerHook-HookManualPtrace PRIVATE pthread dl nlohmann_json::nlohmann_json)


add_executable(ScalerHook-HookBrkpoint
        src/PtraceHookMain.cpp
        src/ExtFuncCallHook_Linux.cpp
        src/ExtFuncCallHookAsm.cpp
        src/ExtFuncCallHookPtrace.cpp
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
        src/InvocationTree.cpp
        src/StringTool.cpp
        )
target_include_directories(ScalerHook-HookBrkpoint PUBLIC src/include lib/inireader)
target_compile_options(ScalerHook-HookBrkpoint PRIVATE "--verbose;-fPIE;-fPIC;")
target_link_libraries(ScalerHook-HookBrkpoint PRIVATE pthread dl nlohmann_json::nlohmann_json)


#ScalerHook with auto installation enabled. Scaler hook will run automatically before main.
add_library(ScalerHook-HookAuto SHARED src/autoinstall.cpp src/include/util/tool/MemoryTool_Linux.h src/include/util/tool/MemToolPtrace.h src/include/exceptions/ErrCode.h)
target_compile_options(ScalerHook-HookAuto PRIVATE "--verbose" "-fPIC")
target_link_libraries(ScalerHook-HookAuto PUBLIC ScalerHook-HookManualAsm pthread dl)

#Add tests
add_subdirectory(tests)