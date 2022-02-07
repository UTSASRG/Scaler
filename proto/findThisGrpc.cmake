list(PREPEND CMAKE_PREFIX_PATH
        "${CMAKE_SOURCE_DIR}/proto/lib/grpc_1_43_0/release")
list(PREPEND CMAKE_MODULE_PATH
        "${CMAKE_SOURCE_DIR}/proto/lib/grpc_1_43_0/release/lib/cmake/absl"
        "${CMAKE_SOURCE_DIR}/proto/lib/grpc_1_43_0/release/lib/cmake/c-ares"
        "${CMAKE_SOURCE_DIR}/proto/lib/grpc_1_43_0/release/lib/cmake/grpc"
        "${CMAKE_SOURCE_DIR}/proto/lib/grpc_1_43_0/release/lib/cmake/protobuf"
        "${CMAKE_SOURCE_DIR}/proto/lib/grpc_1_43_0/release/lib/cmake/re2")
include(${CMAKE_SOURCE_DIR}/proto/lib/grpc_1_43_0/release/lib/cmake/protobuf/protobuf-config.cmake)
include(${CMAKE_SOURCE_DIR}/proto/lib/grpc_1_43_0/release/lib/cmake/protobuf/protobuf-module.cmake)
include(${CMAKE_SOURCE_DIR}/proto/lib/grpc_1_43_0/release/lib/cmake/protobuf/protobuf-options.cmake)
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)