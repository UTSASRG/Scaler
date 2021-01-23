cmake_minimum_required(VERSION 3.10)

# set the project name
project(Scaler)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -ldl --verbose -znow -O0 -fPIE -fPIC -std=c++11")

include_directories(include)

# headers plthook library
add_subdirectory(libScalerHook)

# unit tests
add_subdirectory(tests)

