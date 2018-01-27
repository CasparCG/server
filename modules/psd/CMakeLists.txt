cmake_minimum_required (VERSION 2.6)
project (psd)

set(SOURCES
		util/bigendian_file_input_stream.cpp
		util/pdf_reader.cpp

		descriptor.cpp
		psd_document.cpp
		layer.cpp
		misc.cpp
		psd_scene_producer.cpp
)
set(HEADERS
		util/bigendian_file_input_stream.h
		util/pdf_reader.h

		channel.h
		descriptor.h
		psd_document.h
		image.h
		layer.h
		misc.h
		psd_scene_producer.h
)

add_library(psd ${SOURCES} ${HEADERS})

include_directories(..)
include_directories(../..)
include_directories(${BOOST_INCLUDE_PATH})
include_directories(${TBB_INCLUDE_PATH})

set_target_properties(psd PROPERTIES FOLDER modules)
source_group(sources\\util util/*)
source_group(sources ./*)

target_link_libraries(psd common core)

casparcg_add_include_statement("modules/psd/psd_scene_producer.h")
casparcg_add_init_statement("psd::init" "psd")
casparcg_add_module_project("psd")
