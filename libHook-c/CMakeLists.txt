project(Hook VERSION 0.1.4)

# Set global (Relative this folder until reset) compiler options

set(SCALER_HOOK_COMPILATION_FLAGS "-O0" "-g" "-Werror")
# https://json.nlohmann.me/integration/cmake/


add_library(ScalerHook-Config INTERFACE)
target_include_directories(ScalerHook-Config INTERFACE src/include/config)

#Add all files for libscalerhook
set(HookSrc
        src/ExtFuncCallHook.cpp
        src/ProcInfoParser.cpp
        src/ElfParser.cpp
        src/MemTool.cpp
        src/FileTool.cpp
        src/Timer.cpp
        src/InvocationTree.cpp
        src/StringTool.cpp
        src/HookContext.cpp
        src/systemProxy.cpp
        src/pthreadProxy.cpp
        src/ExtSymInfo.cpp
        src/ELFImgInfo.cpp
        src/HookHandlers.cpp
        src/AddrFileIdMapping.cpp
        )
#src/VMEmulator.cpp

#list(APPEND CMAKE_PREFIX_PATH "/media/umass/datasystem/steven/lib/intel/build/kits/xed-install-base-2021-08-26-lin-x86-64")
#find_package(XED REQUIRED)

include(${CMAKE_SOURCE_DIR}/proto/findThisGrpc.cmake)

# Scaler hook library itself. Users need to manually install scalerhook through library functions.
add_library(ScalerHook-HookManualAsm-C SHARED
        ${HookSrc})
target_include_directories(ScalerHook-HookManualAsm-C PUBLIC src/include lib/inireader)
target_link_libraries(ScalerHook-HookManualAsm-C PUBLIC pthread dl)
target_compile_options(ScalerHook-HookManualAsm-C PRIVATE ${SCALER_HOOK_COMPILATION_FLAGS})
target_compile_definitions(ScalerHook-HookManualAsm-C PRIVATE CMAKE_SCALERRUN_VERSION="${PROJECT_VERSION}")


#message(" ${PROTO_SRCS}  ${GRPC_SRCS}")


#ScalerHook with auto installation enabled. Scaler hook will run automatically before main.
add_library(ScalerHook-HookAutoAsm-C SHARED src/libcProxy.cpp src/libcProxy.cpp ${HookSrc})# ${PROTO_SRCS} ${GRPC_SRCS})
target_include_directories(ScalerHook-HookAutoAsm-C PUBLIC src/include lib/inireader)# ${PROTO_SRC_DIR})
target_link_libraries(ScalerHook-HookAutoAsm-C PUBLIC pthread dl)
target_link_options(ScalerHook-HookAutoAsm-C PRIVATE -static-libgcc -static-libstdc++) #Todo; Statically link libc is not advised. Change to re-compile in o
target_compile_options(ScalerHook-HookAutoAsm-C PRIVATE ${SCALER_HOOK_COMPILATION_FLAGS})
target_compile_definitions(ScalerHook-HookAutoAsm-C PRIVATE XED_ENCODER CMAKE_SCALERRUN_VERSION="${PROJECT_VERSION}") #NDEBUG

#protobufferStatic


#add_library(ScalerHook-HookAuto-Brkpoint SHARED src/autoinstall.cpp)
#target_link_libraries(ScalerHook-HookAuto-Brkpoint PUBLIC ScalerHook-HookBrkpoint)
#target_compile_options(ScalerHook-HookAuto-Brkpoint PRIVATE ${SCALER_HOOK_COMPILATION_FLAGS})

#Add tests
add_subdirectory(tests)