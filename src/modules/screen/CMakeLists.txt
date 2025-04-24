cmake_minimum_required (VERSION 3.16)
project (screen)

set(SOURCES
	consumer/screen_consumer.cpp
	consumer/screen_consumer.h

	consumer_screen_vertex.h
	consumer_screen_fragment.h

	screen.cpp
	screen.h
)

bin2c("consumer/screen.vert" "consumer_screen_vertex.h" "caspar::screen" "vertex_shader")
bin2c("consumer/screen.frag" "consumer_screen_fragment.h" "caspar::screen" "fragment_shader")

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

target_link_libraries(screen PRIVATE accelerator ffmpeg)
