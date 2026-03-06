cmake_minimum_required (VERSION 3.28)
project (portaudio_module)

set(SOURCES
		consumer/portaudio_consumer.cpp
		consumer/portaudio_consumer.h
		portaudio.cpp
		portaudio.h
)

casparcg_add_module_project(portaudio
	SOURCES ${SOURCES}
	INIT_FUNCTION "portaudio::init"
)

# Find PortAudio - check bundled dependencies FIRST, then vcpkg
find_path(PORTAUDIO_INCLUDE_DIR portaudio.h
    PATHS
        "${CMAKE_SOURCE_DIR}/../dependencies/portaudio/include"
        "C:/vcpkg/installed/x64-windows/include"
)

find_library(PORTAUDIO_LIBRARY 
    NAMES portaudio portaudio_x64 portaudio_static
    PATHS
        "${CMAKE_SOURCE_DIR}/../dependencies/portaudio/lib"
        "C:/vcpkg/installed/x64-windows/lib"
)

if(NOT PORTAUDIO_INCLUDE_DIR OR NOT PORTAUDIO_LIBRARY)
    message(WARNING "PortAudio not found - portaudio module will not be built")
    return()
endif()

target_include_directories(portaudio PRIVATE ${PORTAUDIO_INCLUDE_DIR})
target_link_libraries(portaudio PRIVATE ${PORTAUDIO_LIBRARY})

message(STATUS "PortAudio found: ${PORTAUDIO_LIBRARY}")

set_target_properties(portaudio PROPERTIES FOLDER modules)
source_group(sources\\consumer consumer/*)
source_group(sources ./*)

# Copy PortAudio DLL to shell output directory
if(WIN32)
    set(PORTAUDIO_DLL "${CMAKE_SOURCE_DIR}/../dependencies/portaudio/bin/portaudio.dll")
    
    if(EXISTS ${PORTAUDIO_DLL})
        message(STATUS "PortAudio DLL will be copied: ${PORTAUDIO_DLL}")
        
        # Copy to shell output (where casparcg.exe will be)
        add_custom_command(TARGET portaudio POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${PORTAUDIO_DLL}
                ${CMAKE_BINARY_DIR}/shell/$<CONFIG>
            COMMENT "Copying PortAudio DLL to shell output directory"
        )
    else()
        message(WARNING "PortAudio DLL not found at: ${PORTAUDIO_DLL}")
    endif()
endif()