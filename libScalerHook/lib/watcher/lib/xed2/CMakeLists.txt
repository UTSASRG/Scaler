
add_library(xed2-ild STATIC IMPORTED GLOBAL)
set_target_properties(xed2-ild PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_LIST_DIR}/lib/libxed-ild.a)
set_target_properties(xed2-ild PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_LIST_DIR}/include)
