#message("Download compiled protobuffer library")
#SET(SHOULD_DOWNLOAD_PROTOBUFFER TRUE)
#if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lib/protobuf_3_19_2/protobuf_3_19_2.tar.xz")
#    file(SHA256 ${CMAKE_CURRENT_SOURCE_DIR}/lib/protobuf_3_19_2/protobuf_3_19_2.tar.xz HASH_PROTOBUFFER_3_19_2)
#    if (${HASH_PROTOBUFFER_3_19_2} STREQUAL "34c64fdef8cf9bb5162bbdf3104f044d8a5e6af5a62bc3b35b6b6f292f2eb3ad")
#        message("protobuf_3_19_2.tar.xz downloaded sha256:${HASH_PROTOBUFFER_3_19_2}.")
#        SET(SHOULD_DOWNLOAD_PROTOBUFFER FALSE)
#    endif ()
#endif ()
#message("SHOULD_DOWNLOAD_PROTOBUFFER=${SHOULD_DOWNLOAD_PROTOBUFFER} ${HASH_PROTOBUFFER_3_19_2}")
#if (${SHOULD_DOWNLOAD_PROTOBUFFER})
#    message("Downloading protobuf_3_19_2.tar.xz")
#    file(DOWNLOAD https://driveus.xttech.top/s/9m6XPST3AZEs8gZ/download ${CMAKE_CURRENT_SOURCE_DIR}/lib/protobuf_3_19_2/protobuf_3_19_2.tar.xz SHOW_PROGRESS TLS_VERIFY ON)
#endif ()
#
#
#message("Downloading pre-compiled protobuffer library")
#file(GLOB RESULT ${CMAKE_CURRENT_SOURCE_DIR}/lib/protobuf_3_19_2/*)
#list(REMOVE_ITEM RESULT ${CMAKE_CURRENT_SOURCE_DIR}/lib/protobuf_3_19_2/protobuf_3_19_2.tar.xz)
#list(REMOVE_ITEM RESULT ${CMAKE_CURRENT_SOURCE_DIR}/lib/protobuf_3_19_2/.gitignore)
#list(LENGTH RESULT RES_LEN)
#if (RES_LEN EQUAL 0)
#    message("Extracting protobuf_3_19_2.tar.xz")
#    execute_process(
#            COMMAND ${CMAKE_COMMAND} -E tar -xf ${CMAKE_CURRENT_SOURCE_DIR}/lib/protobuf_3_19_2/protobuf_3_19_2.tar.xz
#            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib/protobuf_3_19_2
#    )
#else ()
#    message("${CMAKE_CURRENT_SOURCE_DIR}/lib/protobuf_3_19_2 has content other than (.gitignore and *.xz), won't re-extract protobuf_3_19_2.tar.xz")
#endif ()
#
#message("Find protobuf library")
#set(Protobuf_USE_STATIC_LIBS ON)  #Static link
#IF (CMAKE_BUILD_TYPE MATCHES Debug)
#    message("Link debug mode protobuffer")
#    set(PROTOBUFFER_SUB_DIR "debug")
#ELSEIF (CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
#    message("Link relWithDebInfo mode protobuffer")
#    set(PROTOBUFFER_SUB_DIR "relwithdebinfo")
#ELSE ()
#    message("Link release mode protobuffer (Default case)")
#    set(PROTOBUFFER_SUB_DIR "release")
#ENDIF ()
#include(lib/protobuf_3_19_2/${PROTOBUFFER_SUB_DIR}/lib/cmake/protobuf/protobuf-config.cmake)
#include(lib/protobuf_3_19_2/${PROTOBUFFER_SUB_DIR}/lib/cmake/protobuf/protobuf-module.cmake)
#include(lib/protobuf_3_19_2/${PROTOBUFFER_SUB_DIR}/lib/cmake/protobuf/protobuf-options.cmake)
#include(lib/protobuf_3_19_2/${PROTOBUFFER_SUB_DIR}/lib/cmake/protobuf/protobuf-targets.cmake)
#find_package(Protobuf REQUIRED HINTS lib/protobuf_3_19_2/${PROTOBUFFER_SUB_DIR}/lib/cmake/protobuf)
#
#message("Build protobuffer models")
## https://stackoverflow.com/questions/41430991/cmake-with-protobuf-file-in-subdirectory
#file(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src PROTOMODEL_PATH)
#file(TO_NATIVE_PATH ${CMAKE_BINARY_DIR}/protobuffer/build PROTOBINDING_PATH)
#message("Remove everything in ${PROTOBINDING_PATH}")
#file(REMOVE_RECURSE ${PROTOBINDING_PATH})
#file(MAKE_DIRECTORY ${PROTOBINDING_PATH})
#file(GLOB_RECURSE DATAMODEL_PROTOS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.proto")
#FOREACH (proto ${DATAMODEL_PROTOS})
#    file(TO_NATIVE_PATH ${proto} proto_native)
#    execute_process(COMMAND ${PROTOBUF_PROTOC_EXECUTABLE} --proto_path=${PROTOMODEL_PATH} --cpp_out=${PROTOBINDING_PATH} ${proto_native}
#            RESULT_VARIABLE rv)
#    # Optional, but that can show the user if something have gone wrong with the proto generation
#    MESSAGE("Compiling ${proto_native} ret ${rv}")
#ENDFOREACH (proto)
#
## Make protobuffer variables avalable to other modules
#FILE(GLOB_RECURSE PROTO_HDRS "${PROTOBINDING_PATH}/*.h")
#FILE(GLOB_RECURSE PROTO_SRCS "${PROTOBINDING_PATH}/*.cc")
#
#
#message("Exporting global variables for built protobuffer models ${PROTOBINDING_PATH}")
#set(PROTO_HDR_PATH ${Protobuf_INCLUDE_DIRS} ${PROTOBINDING_PATH} CACHE INTERNAL "")
#set(Protobuf_LIBRARIES ${Protobuf_LIBRARIES} CACHE INTERNAL "")
#set(Protobuf_LITE_LIBRARIES ${Protobuf_LITE_LIBRARIES} CACHE INTERNAL "")
#set(PROTO_HDRS ${PROTO_HDRS} CACHE INTERNAL "")
#set(PROTO_SRCS ${PROTO_SRCS} CACHE INTERNAL "")
#
#message("${PROTO_HDR_PATH} ${Protobuf_LIBRARIES} ${Protobuf_LITE_LIBRARIES} ${PROTO_HDRS} ${PROTO_SRCS}")

include(FetchContent)

FetchContent_Declare(
        gRPC
        GIT_REPOSITORY https://github.com/grpc/grpc
        GIT_TAG        v1.43.0
)
set(FETCHCONTENT_QUIET OFF)
FetchContent_MakeAvailable(gRPC)
