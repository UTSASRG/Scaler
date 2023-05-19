project(Hook VERSION 0.2.0)


set(SCALER_HOOK_COMPILATION_FLAGS "-O3" "-g")
# https://json.nlohmann.me/integration/cmake/

#Add all files for libscalerhook
set(HookSrc
        src/ExtFuncCallHook.cpp
        src/ProcInfoParser.cpp
        src/ElfParser.cpp
        src/MemTool.cpp
        src/FileTool.cpp
        src/StringTool.cpp
        src/HookContext.cpp
        src/SystemProxy.cpp
        src/PthreadProxy.cpp
        src/ExtSymInfo.cpp
        src/ELFImgInfo.cpp
        src/HookHandlers.cpp
        src/DLProxy.cpp
        )

# Scaler hook library itself. Users need to manually install scalerhook through library functions.
add_library(ScalerHook-HookManualAsm-C SHARED
        ${HookSrc} src/LibcProxy.cpp)
target_include_directories(ScalerHook-HookManualAsm-C PUBLIC src/include lib/inireader)
target_link_libraries(ScalerHook-HookManualAsm-C PUBLIC pthread dl)
target_compile_options(ScalerHook-HookManualAsm-C PRIVATE ${SCALER_HOOK_COMPILATION_FLAGS})
target_compile_definitions(ScalerHook-HookManualAsm-C PRIVATE MANUAL_INSTALL CMAKE_SCALERRUN_VERSION="${PROJECT_VERSION}")


#ScalerHook with auto installation enabled. Scaler hook will run automatically before main.
add_library(ScalerHook-HookAutoAsm-C SHARED src/LibcProxy.cpp ${HookSrc})# ${PROTO_SRCS} ${GRPC_SRCS})
target_include_directories(ScalerHook-HookAutoAsm-C PUBLIC src/include lib/inireader)# ${PROTO_SRC_DIR})
target_link_libraries(ScalerHook-HookAutoAsm-C PUBLIC pthread dl)
target_link_options(ScalerHook-HookAutoAsm-C PRIVATE -static-libgcc -static-libstdc++) #Todo; Statically link libc is not advised. Change to re-compile in o
target_compile_options(ScalerHook-HookAutoAsm-C PRIVATE ${SCALER_HOOK_COMPILATION_FLAGS})
target_compile_definitions(ScalerHook-HookAutoAsm-C PUBLIC CMAKE_SCALERRUN_VERSION="${PROJECT_VERSION}") #NDEBUG

#Add tests
add_subdirectory(tests)