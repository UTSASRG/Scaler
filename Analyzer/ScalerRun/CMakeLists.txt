project(ScalerRun VERSION 0.1.0)

include(${CMAKE_SOURCE_DIR}/proto/findThisGrpc.cmake)


function(embed_resource resource_file_name source_file_name variable_name)
    file(READ ${resource_file_name} hex_content HEX)
    string(REPEAT "[0-9a-f]" 32 column_pattern)
    string(REGEX REPLACE "(${column_pattern})" "\\1\n" content "${hex_content}")
    string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1, " content "${content}")
    string(REGEX REPLACE ", $" "" content "${content}")
    set(array_definition "static const unsigned char ${variable_name}[] =\n{\n${content}\n};")
    set(source "// Auto generated file.\n${array_definition}\n")
    file(WRITE "${source_file_name}" "${source}")
endfunction()

embed_resource(config.yml.demo src/resource.cpp configBin)


add_subdirectory(lib/yamlcpp_0_7_0)
target_compile_options(yaml-cpp PRIVATE "-fPIC")

add_executable(ScalerRun src/injector.cpp
        )
target_include_directories(ScalerRun PUBLIC src/include)
target_compile_definitions(ScalerRun PRIVATE CMAKE_SCALERRUN_VERSION="${PROJECT_VERSION}")




