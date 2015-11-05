cmake_minimum_required (VERSION 2.6)
project (screen)

set(SOURCES
		consumer/screen_consumer.cpp

		screen.cpp
)
set(HEADERS
		consumer/screen_consumer.h

		screen.h
)

add_library(screen ${SOURCES} ${HEADERS})

include_directories(..)
include_directories(../..)
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${RXCPP_INCLUDE_PATH})
include_directories(${TBB_INCLUDE_DIRS})
include_directories(${GLEW_INCLUDE_DIRS})
include_directories(${SFML_INCLUDE_DIR})
include_directories(${ASMLIB_INCLUDE_PATH})
include_directories(${FFmpeg_INCLUDE_DIRS})

set_target_properties(screen PROPERTIES FOLDER modules)
source_group(sources\\consumer consumer/*)
source_group(sources ./*)

target_link_libraries(screen common core ffmpeg)

casparcg_add_include_statement("modules/screen/screen.h")
casparcg_add_init_statement("screen::init" "screen")
casparcg_add_module_project("screen")
