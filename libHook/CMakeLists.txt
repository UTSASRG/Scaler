
# Set global (Relative this folder until reset) compiler options


set(SCALER_HOOK_COMPILATION_FLAGS "-O0" "-g" "-Werror")
# https://json.nlohmann.me/integration/cmake/
set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(lib/json)
add_subdirectory(lib/watcher)


#Add all files for libscalerhook
set(HookSrc
        src/ExtFuncCallHook.cpp
        src/ExtFuncCallHookAsm.cpp
        src/ProcInfoParser.cpp
        src/ElfParser.cpp
        src/MemTool.cpp
        src/MemTool_Linux.cpp
        src/FileTool.cpp
        src/install.cpp
        src/Timer.cpp
        src/Config.cpp
        src/Serilizable.cpp
        src/InvocationTree.cpp
        src/StringTool.cpp
        src/InfoServiceGprc.cpp
        ${SCALER_PROTO_SRCS}
        )
#src/VMEmulator.cpp

#list(APPEND CMAKE_PREFIX_PATH "/media/umass/datasystem/steven/lib/intel/build/kits/xed-install-base-2021-08-26-lin-x86-64")
#find_package(XED REQUIRED)


#add_executable(ScalerHook-HookManualPtrace
#        src/PtraceHookMain.cpp
#        ${HookSrc}
#        )
#target_include_directories(ScalerHook-HookManualPtrace PUBLIC src/include lib/inireader)
#target_link_libraries(ScalerHook-HookManualPtrace PUBLIC pthread dl nlohmann_json::nlohmann_json xed2 xed2-ild)
#target_compile_definitions(ScalerHook-HookManualPtrace PUBLIC XED_ENCODER)
#target_compile_options(ScalerHook-HookManualPtrace PRIVATE ${SCALER_HOOK_COMPILATION_FLAGS})

#
#add_library(ScalerHook-HookBrkpoint SHARED
#        ${HookSrc})
#target_include_directories(ScalerHook-HookBrkpoint PUBLIC src/include lib/inireader)
#target_link_libraries(ScalerHook-HookBrkpoint PUBLIC pthread dl nlohmann_json::nlohmann_json xed2 xed2-ild)
#target_compile_definitions(ScalerHook-HookBrkpoint PUBLIC XED_ENCODER)
#target_compile_options(ScalerHook-HookBrkpoint PRIVATE ${SCALER_HOOK_COMPILATION_FLAGS})

list(PREPEND CMAKE_PREFIX_PATH
        "${CMAKE_SOURCE_DIR}/proto/lib/grpc_1_43_0/release")
message("CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH} ${CMAKE_MODULE_PATH}")
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)

# Scaler hook library itself. Users need to manually install scalerhook through library functions.
add_library(ScalerHook-HookManualAsm SHARED
        ${HookSrc})
target_include_directories(ScalerHook-HookManualAsm PUBLIC src/include lib/inireader ${SCLAER_GRPC_INCLUDE})
target_link_libraries(ScalerHook-HookManualAsm PUBLIC pthread dl nlohmann_json::nlohmann_json protobuf::libprotobuf gRPC::grpc++)
target_compile_definitions(ScalerHook-HookManualAsm PUBLIC XED_ENCODER)
target_compile_options(ScalerHook-HookManualAsm PRIVATE ${SCALER_HOOK_COMPILATION_FLAGS})


#message(" ${PROTO_SRCS}  ${GRPC_SRCS}")



#ScalerHook with auto installation enabled. Scaler hook will run automatically before main.
add_library(ScalerHook-HookAuto SHARED src/autoinstall.cpp ${HookSrc})# ${PROTO_SRCS} ${GRPC_SRCS})
target_include_directories(ScalerHook-HookAuto PUBLIC src/include lib/inireader ${SCLAER_GRPC_INCLUDE})# ${PROTO_SRC_DIR})
target_link_libraries(ScalerHook-HookAuto PUBLIC pthread dl nlohmann_json::nlohmann_json protobuf::libprotobuf gRPC::grpc++)
target_compile_definitions(ScalerHook-HookAuto PUBLIC XED_ENCODER)#NDEBUG
target_link_options(ScalerHook-HookAuto PRIVATE -static-libgcc -static-libstdc++) #Todo; Statically link libc is not advised. Change to re-compile in o
target_compile_options(ScalerHook-HookAuto PRIVATE ${SCALER_HOOK_COMPILATION_FLAGS})

#protobufferStatic


#add_library(ScalerHook-HookAuto-Brkpoint SHARED src/autoinstall.cpp)
#target_link_libraries(ScalerHook-HookAuto-Brkpoint PUBLIC ScalerHook-HookBrkpoint)
#target_compile_options(ScalerHook-HookAuto-Brkpoint PRIVATE ${SCALER_HOOK_COMPILATION_FLAGS})

#Add tests
add_subdirectory(tests)