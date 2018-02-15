cmake_minimum_required (VERSION 2.6)
project (newtek)

set(SOURCES
		consumer/newtek_ivga_consumer.cpp

		util/air_send.cpp

		newtek.cpp

		StdAfx.cpp
)
set(HEADERS
		consumer/newtek_ivga_consumer.h

		util/air_send.h

		newtek.h

		StdAfx.h
)

add_library(newtek ${SOURCES} ${HEADERS})
add_precompiled_header(newtek StdAfx.h FORCEINCLUDE)
configure_file("${PROJECT_SOURCE_DIR}/packages.config" "${CMAKE_CURRENT_BINARY_DIR}/packages.config")

include_directories(..)
include_directories(../..)
include_directories(${BOOST_INCLUDE_PATH})
include_directories(${TBB_INCLUDE_PATH})

set_target_properties(newtek PROPERTIES FOLDER modules)
source_group(sources\\consumer consumer/*)
source_group(sources\\util util/*)
source_group(sources ./*)

target_link_libraries(newtek
	common
	core
)

casparcg_add_include_statement("modules/newtek/newtek.h")
casparcg_add_init_statement("newtek::init" "newtek")
casparcg_add_module_project("newtek")
