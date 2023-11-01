cmake_minimum_required (VERSION 3.16)
project (bluefish)

set(SOURCES
		consumer/bluefish_consumer.cpp
        producer/bluefish_producer.cpp
		util/blue_velvet.cpp
		bluefish.cpp
)
set(HEADERS
	    consumer/bluefish_consumer.h
        producer/bluefish_producer.h
        util/blue_velvet.h
	    util/memory.h
        bluefish.h
	StdAfx.h
)

casparcg_add_module_project(bluefish
    SOURCES ${SOURCES} ${HEADERS}
	INIT_FUNCTION "bluefish::init"
)
target_include_directories(bluefish PRIVATE
    ..
    ../..
    ${FFMPEG_INCLUDE_PATH}
    )
target_precompile_headers(bluefish PRIVATE "StdAfx.h")

set_target_properties(bluefish PROPERTIES FOLDER modules)
source_group(sources ./*)
source_group(sources\\consumer consumer/*)
source_group(sources\\producer producer/*)
source_group(sources\\util util/*)
source_group(sources\\interop interop/*)

target_link_libraries(bluefish ffmpeg)
