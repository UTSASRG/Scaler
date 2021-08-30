cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0079 NEW)
project(Scaler)


add_subdirectory(libScalerHook/lib/watcher/lib/libipt)
add_subdirectory(libScalerHook/lib/watcher/lib/heaplayers)
add_subdirectory(libScalerHook/lib/watcher/lib/libunwind)
add_subdirectory(libScalerHook/lib/watcher/lib/xed2)

# Adding scalerhook to this project
add_subdirectory(libScalerHook)
