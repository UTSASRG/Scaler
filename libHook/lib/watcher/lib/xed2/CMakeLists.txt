
add_library(xed2-ild SHARED IMPORTED GLOBAL)
set_target_properties(xed2-ild PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_LIST_DIR}/lib/libxed-ild.so)
#set_target_properties(xed2-ild PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_LIST_DIR}/include)
target_include_directories(xed2-ild INTERFACE include)

add_library(xed2 SHARED IMPORTED GLOBAL)
set_target_properties(xed2 PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_LIST_DIR}/lib/libxed.so)
#set_target_properties(xed2 PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_LIST_DIR}/include)
target_include_directories(xed2 INTERFACE include)
#
#list(APPEND CMAKE_PREFIX_PATH "/media/umass/datasystem/steven/Downloads/intel/build/kits/xed-install-base-2021-08-26-lin-x86-64/lib/cmake/XED" )
#find_package(XED REQUIRED)
