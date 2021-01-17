# This cmake file builds
project(Tests)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -ldl --verbose -zlazy -O0 -fPIE -fPIC -fno-access-control -std=c++11")

# Improt google test
add_subdirectory(googletest)

include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

# This library is used to test external funciton invocation
add_library(FuncCallTest ./libtest/src/FuncWithDiffParms.cpp
        ./libtest/src/TenThousandFunc.cpp)
target_include_directories(FuncCallTest PUBLIC ./libtest/header)

# PltHook is used to comapre results
add_library(KuBoPltHook ./plthook/plthook_elf.c)
target_include_directories(KuBoPltHook PUBLIC ./plthook)

# Add a naive application that calls
# PltHook is used to comapre results
add_executable(NaiveInvocationApp ./NaiveFuncCallApp/main.cpp)
target_link_libraries(NaiveInvocationApp z dl)

# Build unittest examples for libhook
add_executable(TestLibHook
        TestPMParser.cpp
        TestExtFuncCallHook.cpp
        TestFileTool.cpp
        TestException.cpp
        TestELFParser.cpp)
target_include_directories(TestLibHook PUBLIC ../libScalerHook/headers)
target_link_libraries(TestLibHook scalerhook KuBoPltHook gtest gtest_main dl)
target_link_options(TestLibHook PUBLIC "LINKER:SHELL: -T ${CMAKE_SOURCE_DIR}/mylinker.ld")





