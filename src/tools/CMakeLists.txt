CMAKE_MINIMUM_REQUIRED (VERSION 3.0)

include_directories(..)
add_executable(bin2c bin2c.cpp)

function(bin2c source_file dest_file namespace obj_name)
    ADD_CUSTOM_COMMAND(
        OUTPUT ${dest_file}
        COMMAND bin2c ${namespace} ${obj_name} ${CMAKE_CURRENT_SOURCE_DIR}/${source_file} > ${CMAKE_CURRENT_BINARY_DIR}/${dest_file}
        DEPENDS bin2c ${CMAKE_CURRENT_SOURCE_DIR}/${source_file}
    )
endfunction()

