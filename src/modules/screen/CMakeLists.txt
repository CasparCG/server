cmake_minimum_required (VERSION 3.16)
project (screen)

set(SOURCES
	consumer/screen_consumer.cpp
	consumer/screen_consumer.h

	screen.cpp
	screen.h
)

compile_gl_shaders(screen-shaders
    NAMESPACE "caspar::screen"
    SOURCES
        "consumer/screen.vert"
        "consumer/screen.frag"
    )

if (MSVC)
else ()
	list(APPEND SOURCES
		util/x11_util.cpp
		util/x11_util.h
	)
endif ()

casparcg_add_module_project(screen
	SOURCES ${SOURCES}
	INIT_FUNCTION "screen::init"
)
add_dependencies(screen screen-shaders)
target_include_directories(screen PRIVATE
    ../..
    ${CMAKE_CURRENT_BINARY_DIR}
    ${GLEW_INCLUDE_PATH}
    ${SFML_INCLUDE_PATH}
    ${FFMPEG_INCLUDE_PATH}
    )

set_target_properties(screen PROPERTIES FOLDER modules)
source_group(sources\\consumer consumer/*)
source_group(sources ./*)

target_link_libraries(screen accelerator ffmpeg)
