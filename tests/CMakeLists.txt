# This cmake file builds
project(Tests)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -ldl --verbose -zlazy -O0 -fPIE -fPIC -fno-access-control -std=c++11")

# Improt google test
add_subdirectory(googletest)

include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

# Build unittest examples for libhook
add_executable(TestLibHook
        TestPMParser.cpp
        TestExtFuncCallHook.cpp
        TestFileTool.cpp
        TestException.cpp
        )
target_include_directories(TestLibHook PUBLIC ../libScalerHook/headers)
target_link_libraries(TestLibHook scalerhook gtest gtest_main dl)
target_link_options(TestLibHook PUBLIC "LINKER:SHELL: -T ${CMAKE_SOURCE_DIR}/mylinker.ld")




