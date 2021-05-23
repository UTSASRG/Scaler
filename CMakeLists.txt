cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0079 NEW)
# set the project name
project(Scaler)

include_directories(libScalerHook/src/include)

# headers plthook library
add_subdirectory(libScalerHook)




