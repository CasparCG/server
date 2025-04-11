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

function(compile_gl_shaders target_name)
    cmake_parse_arguments(
        PARSED_ARGS # prefix of output variables
        "" # list of names of the boolean arguments (only defined ones will be true)
        "NAMESPACE" # list of names of mono-valued arguments
        "SOURCES" # list of names of multi-valued arguments (output variables are lists)
        ${ARGN} # arguments of the function to parse, here we take the all original ones
    )

	# Validate args
	if (NOT PARSED_ARGS_NAMESPACE)
        message(FATAL_ERROR "You must provide an NAMESPACE")
    endif()

    set (DEPS
        bin2c
    )

    foreach(SOURCE_FILE ${PARSED_ARGS_SOURCES})
        string(REGEX REPLACE "[^A-Za-z0-9_]" "_" OUT_FILE ${SOURCE_FILE})
        # perform the conversion to a C++ header file
        ADD_CUSTOM_COMMAND(
            OUTPUT ${OUT_FILE}.h
            COMMAND bin2c ${PARSED_ARGS_NAMESPACE} ${OUT_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE} > ${CMAKE_CURRENT_BINARY_DIR}/${OUT_FILE}.h
            DEPENDS bin2c ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE}
        )
        list(APPEND DEPS ${OUT_FILE}.h)

        if (GLSLANGVALIDATOR_EXE)
            # check if the shader is valid
            ADD_CUSTOM_COMMAND(
                OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${OUT_FILE}.check
                COMMAND glslangValidator ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE}
                DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE}
            )

            list(APPEND DEPS ${CMAKE_CURRENT_BINARY_DIR}/${OUT_FILE}.check)
        endif()
    endforeach(SOURCE_FILE)

    add_custom_target(${target_name} DEPENDS ${DEPS})
endfunction()
