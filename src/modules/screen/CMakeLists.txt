cmake_minimum_required (VERSION 2.6)
project (screen)

set(SOURCES
	consumer/screen_consumer.cpp

	screen.cpp
)
set(HEADERS
	consumer/screen_consumer.h

	consumer_screen_vertex.h
	consumer_screen_fragment.h

	screen.h
)

bin2c("consumer/screen.vert" "consumer_screen_vertex.h" "caspar::screen" "vertex_shader")
bin2c("consumer/screen.frag" "consumer_screen_fragment.h" "caspar::screen" "fragment_shader")

if (MSVC)
else ()
	set(OS_SPECIFIC_HEADERS
			util/x11_util.h
			)
	set(OS_SPECIFIC_SOURCES
			util/x11_util.cpp
			)
endif ()

add_library(screen ${SOURCES} ${OS_SPECIFIC_SOURCES} ${HEADERS} ${OS_SPECIFIC_HEADERS})
configure_file("${PROJECT_SOURCE_DIR}/packages.config" "${CMAKE_CURRENT_BINARY_DIR}/packages.config")

include_directories(..)
include_directories(../..)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${BOOST_INCLUDE_PATH})
include_directories(${TBB_INCLUDE_PATH})
include_directories(${GLEW_INCLUDE_PATH})
include_directories(${SFML_INCLUDE_PATH})
include_directories(${FFMPEG_INCLUDE_PATH})

set_target_properties(screen PROPERTIES FOLDER modules)
source_group(sources\\consumer consumer/*)
source_group(sources ./*)

target_link_libraries(screen common core accelerator ffmpeg)

casparcg_add_include_statement("modules/screen/screen.h")
casparcg_add_init_statement("screen::init" "screen")
casparcg_add_module_project("screen")
