cmake_minimum_required (VERSION 3.16)

add_executable(bin2c bin2c.cpp)
target_compile_features(bin2c PRIVATE cxx_std_17)

target_include_directories(bin2c PRIVATE ..)


if (NOT SKIP_SHADER_CHECK)
    #
    # glslangValidator
    #
    find_program(GLSLANGVALIDATOR_EXE "glslangValidator")
    mark_as_advanced(FORCE GLSLANGVALIDATOR_EXE)

    if(GLSLANGVALIDATOR_EXE)
        message(STATUS "Performing GLSL shader validation with: ${GLSLANGVALIDATOR_EXE}")
    else()
        message(FATAL_ERROR "glslangValidator not found!")
    endif()

endif()

function(compile_gl_shader source_file dest_file namespace obj_name)
    if (GLSLANGVALIDATOR_EXE)
        # check if the shader is valid
        ADD_CUSTOM_COMMAND(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${dest_file}.check
            COMMAND glslangValidator ${CMAKE_CURRENT_SOURCE_DIR}/${source_file}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${source_file}

        )

        # perform the conversion to a C++ header file
        ADD_CUSTOM_COMMAND(
            OUTPUT ${dest_file}
            COMMAND bin2c ${namespace} ${obj_name} ${CMAKE_CURRENT_SOURCE_DIR}/${source_file} > ${CMAKE_CURRENT_BINARY_DIR}/${dest_file}
            DEPENDS bin2c ${CMAKE_CURRENT_SOURCE_DIR}/${source_file} ${CMAKE_CURRENT_BINARY_DIR}/${dest_file}.check
        )
    else()
        # perform the conversion to a C++ header file
        ADD_CUSTOM_COMMAND(
            OUTPUT ${dest_file}
            COMMAND bin2c ${namespace} ${obj_name} ${CMAKE_CURRENT_SOURCE_DIR}/${source_file} > ${CMAKE_CURRENT_BINARY_DIR}/${dest_file}
            DEPENDS bin2c ${CMAKE_CURRENT_SOURCE_DIR}/${source_file}
        )
    endif()
endfunction()
