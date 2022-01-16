project(ScalerRun VERSION 0.1.0)

include(${CMAKE_SOURCE_DIR}/proto/findThisGrpc.cmake)

add_subdirectory(lib/yamlcpp_0_7_0)
add_executable(ScalerRun src/injector.cpp
        src/InfoServiceGrpc.cpp
        ${SCALER_PROTO_SRCS}
        )
target_include_directories(ScalerRun PUBLIC src/include ${SCLAER_GRPC_INCLUDE})
target_link_libraries(ScalerRun yaml-cpp::yaml-cpp protobuf::libprotobuf gRPC::grpc++ ScalerHook-Config)
target_compile_definitions(ScalerRun PRIVATE CMAKE_SCALERRUN_VERSION="${PROJECT_VERSION}")




