
message("Download compiled grpc library")
SET(SHOULD_DOWNLOAD_GRPC TRUE)
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lib/grpc_1_43_0/grpc_1_43_0.tar.xz")
    file(SHA256 ${CMAKE_CURRENT_SOURCE_DIR}/lib/grpc_1_43_0/grpc_1_43_0.tar.xz HASH_GRPC_1_43_0)
    if (${HASH_GRPC_1_43_0} STREQUAL "741a073b21c1891df399a7ebd65094b3558bc365cdc4669e767cf8cd929a9fe8")
        message("grpc_1_43_0.tar.xz downloaded sha256:${HASH_GRPC_1_43_0}.")
        SET(SHOULD_DOWNLOAD_GRPC FALSE)
    endif ()
endif ()
if (${SHOULD_DOWNLOAD_GRPC})
    message("Downloading grpc_1_43_0.tar.xz")
    file(DOWNLOAD https://driveus.xttech.top/s/BLCwmQdz32LA347/download ${CMAKE_CURRENT_SOURCE_DIR}/lib/grpc_1_43_0/grpc_1_43_0.tar.xz SHOW_PROGRESS TLS_VERIFY ON)
endif ()

message("Extracting pre-compiled grpc library")
file(GLOB RESULT ${CMAKE_CURRENT_SOURCE_DIR}/lib/grpc_1_43_0/*)
list(REMOVE_ITEM RESULT ${CMAKE_CURRENT_SOURCE_DIR}/lib/grpc_1_43_0/grpc_1_43_0.tar.xz)
list(REMOVE_ITEM RESULT ${CMAKE_CURRENT_SOURCE_DIR}/lib/grpc_1_43_0/.gitignore)
list(LENGTH RESULT RES_LEN)
if (RES_LEN EQUAL 0)
    message("Extracting grpc_1_43_0.tar.xz")
    execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar -xf ${CMAKE_CURRENT_SOURCE_DIR}/lib/grpc_1_43_0/grpc_1_43_0.tar.xz
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib/grpc_1_43_0
    )
else ()
    message("${CMAKE_CURRENT_SOURCE_DIR}/lib/grpc_1_43_0 has content other than (.gitignore and *.xz), won't re-extract grpc_1_43_0.tar.xz. If build failed, it might be because an extraction is in progress, please wait until it finishes and retry the build. If that doesn't solve it, try to delete everything in this folder except for .gitignore and *.tar.xz. You can also manually extract .tar.xz into this file if download fails.")
endif ()

#The following is modified from https://github.com/IvanSafonov/grpc-cmake-example

list(PREPEND CMAKE_PREFIX_PATH
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/grpc_1_43_0/release")
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)

message("Build protobuffer models")
set(GRPC_CPP_PLUGIN ${CMAKE_CURRENT_SOURCE_DIR}/lib/grpc_1_43_0/release/bin/grpc_cpp_plugin)
# https://stackoverflow.com/questions/41430991/cmake-with-protobuf-file-in-subdirectory
file(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src PROTOMODEL_PATH)
file(TO_NATIVE_PATH ${CMAKE_CURRENT_BINARY_DIR}/build PROTOBINDING_PATH)
message("Remove everything in ${PROTOBINDING_PATH}")
file(REMOVE_RECURSE ${PROTOBINDING_PATH})
file(MAKE_DIRECTORY ${PROTOBINDING_PATH})
file(GLOB_RECURSE DATAMODEL_PROTOS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.proto")
FOREACH (proto ${DATAMODEL_PROTOS})
    file(TO_NATIVE_PATH ${proto} proto_native)
    message("${PROTOBUF_PROTOC_EXECUTABLE} --proto_path=${PROTOMODEL_PATH} --cpp_out=${PROTOBINDING_PATH} ${proto_native}")
    execute_process(COMMAND ${PROTOBUF_PROTOC_EXECUTABLE} --proto_path=${PROTOMODEL_PATH} --cpp_out=${PROTOBINDING_PATH} ${proto_native}
            RESULT_VARIABLE rv)
    if (NOT "${rv}" MATCHES "0")
        MESSAGE("Compiling ${proto_native} ret ${rv}")
    endif ()
    message("${PROTOBUF_PROTOC_EXECUTABLE} --proto_path=${PROTOMODEL_PATH} --grpc_out=${PROTOBINDING_PATH} --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN} ${proto_native}")
    execute_process(COMMAND ${PROTOBUF_PROTOC_EXECUTABLE} --proto_path=${PROTOMODEL_PATH} --grpc_out=${PROTOBINDING_PATH} --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN} ${proto_native}
            RESULT_VARIABLE rv)
    # Optional, but that can show the user if something have gone wrong with the proto generation
    if (NOT "${rv}" MATCHES "0")
        MESSAGE(FATAL_ERROR "Compiling ${proto_native} (grpc) ret ${rv}")
    endif ()
ENDFOREACH (proto)

# Make protobuffer variables avalable to other modules
FILE(GLOB_RECURSE PROTO_HDRS "${PROTOBINDING_PATH}/*.h")
FILE(GLOB_RECURSE PROTO_SRCS "${PROTOBINDING_PATH}/*.cc")

#message("Exporting global variables for built protobuffer models ${PROTOBINDING_PATH}")
#set(PROTO_HDR_PATH ${Protobuf_INCLUDE_DIRS} ${PROTOBINDING_PATH} CACHE INTERNAL "")
#set(Protobuf_LIBRARIES ${Protobuf_LIBRARIES} CACHE INTERNAL "")
#set(PROTOBUF_LITE_LIBRARY ${PROTOBUF_LITE_LIBRARY} CACHE INTERNAL "")
#set(PROTO_HDRS ${PROTO_HDRS} CACHE INTERNAL "")
#set(PROTO_SRCS ${PROTO_SRCS} CACHE INTERNAL "")

message("${PROTO_HDR_PATH} ${Protobuf_LIBRARIES} ${Protobuf_LITE_LIBRARIES} ${PROTO_HDRS} ${PROTO_SRCS}")

message("Exporting global variables for built protobuffer models ${PROTOBINDING_PATH}")
set(SCLAER_GRPC_INCLUDE ${PROTOBINDING_PATH} CACHE INTERNAL "")
set(SCALER_PROTO_SRCS ${PROTO_SRCS} CACHE INTERNAL "")
