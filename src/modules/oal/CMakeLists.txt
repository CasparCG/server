cmake_minimum_required (VERSION 2.6)
project (oal)

set(SOURCES
		consumer/oal_consumer.cpp

		oal.cpp
)
set(HEADERS
		consumer/oal_consumer.h

		oal.h
)

add_library(oal ${SOURCES} ${HEADERS})
configure_file("${PROJECT_SOURCE_DIR}/packages.config" "${CMAKE_CURRENT_BINARY_DIR}/packages.config")

include_directories(..)
include_directories(../..)
include_directories(${BOOST_INCLUDE_PATH})
include_directories(${TBB_INCLUDE_PATH})
include_directories(${FFMPEG_INCLUDE_PATH})
include_directories(${OPENAL_INCLUDE_PATH})

set_target_properties(oal PROPERTIES FOLDER modules)
source_group(sources\\consumer consumer/*)
source_group(sources ./*)

if(MSVC)
	target_link_libraries(oal
			common
			core

			OpenAL32)
else()
	target_link_libraries(oal
			common
			core

			openal)
endif()

casparcg_add_include_statement("modules/oal/oal.h")
casparcg_add_init_statement("oal::init" "oal")
casparcg_add_module_project("oal")
