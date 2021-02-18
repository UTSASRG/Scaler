cmake_minimum_required(VERSION 3.10)

# set the project name
project(Scaler)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -ldl --verbose -znow -fomit-frame-pointer -O0 -fPIE -fPIC -std=c++11")

include_directories(include)

# headers plthook library
add_subdirectory(libScalerHook)

# Improt google test
add_subdirectory(unittests/googletest)
# unit tests
add_subdirectory(unittests/libscalerhook)

