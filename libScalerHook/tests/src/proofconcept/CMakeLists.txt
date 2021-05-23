cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -ldl --verbose -zlazy -fno-omit-frame-pointer -O3 -fPIE -fPIC -std=c++11")
set(CMAKE_VERBOSE_MAKEFILE on)

#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T ${CMAKE_SOURCE_DIR}/mylinker.ld")

include_directories(includes)

# set the project name
project(PltHook)

# Compile PltHookLib
add_library(PltHookLib SHARED library/plthook_elf.cpp)

# Compile libtest
add_library(libTest SHARED test/libtest.cpp)

# Compile PmParser
#add_library(libPMParser SHARED library/pmparser.cpp)


# Compile libOneMilionFunc
add_library(libOneMilionFunc SHARED test/milionFuncLibrary.cpp)

add_executable(DemoProg test/testprog.cpp)
set_property(TARGET DemoProg PROPERTY POSITION_INDEPENDENT_CODE TRUE)
target_link_libraries(DemoProg libTest PltHookLib dl)

#target_link_libraries(test1 mylib dl PltHookLib)

add_executable(TestFuncInvocation test/testFuncInvocation.cpp)
target_link_libraries(TestFuncInvocation dl)

add_executable(TestASMCallC test/testASMCallFunc.cpp)

add_executable(TestExtFunctionInvocation test/testExtFuncInvocation.cpp)
target_link_libraries(TestExtFunctionInvocation libTest dl)

add_executable(TestJMPHandler test/testJmpHandler.cpp)
target_link_libraries(TestJMPHandler libTest PltHookLib dl)

add_executable(TestJMPHandlerInlineHook test/testJmpHandlerInlineHook.cpp)
target_link_libraries(TestJMPHandlerInlineHook libTest PltHookLib dl)
target_link_options(TestJMPHandlerInlineHook PUBLIC "LINKER:SHELL: -T ${CMAKE_SOURCE_DIR}/mylinker.ld")

get_target_property(MAIN_CFLAGS TestJMPHandlerInlineHook LINKER_OPTOINS)
# also see: COMPILE_DEFINITIONS INCLUDE_DIRECTORIES
message("-- Target compiler flags are: ${MAIN_CFLAGS}")

add_executable(TestAddr2Relative test/testParsingAddr2Relative.cpp)
target_link_libraries(TestAddr2Relative libTest PltHookLib dl)


add_executable(SimpleProgPlt test/simeProgPLT.cpp)
target_link_libraries(SimpleProgPlt libTest)

add_executable(TestFindSection test/testFindSection.cpp)

add_executable(TestPLTInterception test/testPLTInterception.cpp)
target_link_options(TestPLTInterception PUBLIC "LINKER:SHELL: -T ${CMAKE_SOURCE_DIR}/mylinker.ld")


#add_executable(TestLocatingPLT test/testLocatingPLT.cpp)
#target_link_libraries(TestLocatingPLT libTest PltHookLib libPMParser dl)
#target_link_options(TestLocatingPLT PUBLIC "LINKER:SHELL: -T ${CMAKE_SOURCE_DIR}/mylinker.ld")


add_executable(TestFindRdebug test/testFindRdebug.cpp)
target_link_libraries(TestFindRdebug libTest dl)

