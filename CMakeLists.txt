cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0079 NEW)
project(Scaler)

# Adding protobuffers to this project
add_subdirectory(proto)

# Adding scalerhook to this project
add_subdirectory(libHook)

# Adding scalerhook to this project
add_subdirectory(Analyzer)