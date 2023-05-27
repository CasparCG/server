cmake_minimum_required (VERSION 3.16)

add_executable(bin2c bin2c.cpp)
target_compile_features(bin2c PRIVATE cxx_std_14)

target_include_directories(bin2c PRIVATE ..)

function(bin2c source_file dest_file namespace obj_name)
    ADD_CUSTOM_COMMAND(
        OUTPUT ${dest_file}
        COMMAND bin2c ${namespace} ${obj_name} ${CMAKE_CURRENT_SOURCE_DIR}/${source_file} > ${CMAKE_CURRENT_BINARY_DIR}/${dest_file}
        DEPENDS bin2c ${CMAKE_CURRENT_SOURCE_DIR}/${source_file}
    )
endfunction()
