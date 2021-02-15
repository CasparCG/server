cmake_minimum_required (VERSION 2.6)
project (pipe)

set(SOURCES
	consumer/pipe_consumer.cpp

	pipe.cpp
)
set(HEADERS
	consumer/pipe_consumer.h

	pipe.h
)

add_library(pipe ${SOURCES} ${HEADERS})
configure_file("${PROJECT_SOURCE_DIR}/packages.config" "${CMAKE_CURRENT_BINARY_DIR}/packages.config")

include_directories(..)
include_directories(../..)
include_directories(${BOOST_INCLUDE_PATH})
include_directories(${TBB_INCLUDE_PATH})

set_target_properties(pipe PROPERTIES FOLDER modules)
source_group(sources\\consumer consumer/*)
source_group(sources ./*)

target_link_libraries(pipe common core)

casparcg_add_include_statement("modules/pipe/pipe.h")
casparcg_add_init_statement("pipe::init" "pipe")
casparcg_add_module_project("pipe")
