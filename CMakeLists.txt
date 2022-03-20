cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0079 NEW)
project(Scaler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Adding protobuffers to this project
add_subdirectory(proto)

# Adding scalerhook to this project
# add_subdirectory(libHook)

add_subdirectory(libHook-c)


# Adding scalerhook to this project
add_subdirectory(Analyzer)

