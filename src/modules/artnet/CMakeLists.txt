cmake_minimum_required (VERSION 3.16)
project (artnet)

set(SOURCES
	consumer/artnet_consumer.cpp

	artnet.cpp
)
set(HEADERS
	consumer/artnet_consumer.h

	artnet.h
	util/fixture_calculation.cpp util/fixture_calculation.h
)

casparcg_add_module_project(artnet
	SOURCES ${SOURCES} ${HEADERS}
	INIT_FUNCTION "artnet::init"
)
target_include_directories(artnet PRIVATE
    ..
    ../..
)

set_target_properties(artnet PROPERTIES FOLDER modules)
source_group(sources\\consumer consumer/*)
source_group(sources\\util util/*)
source_group(sources ./*)

