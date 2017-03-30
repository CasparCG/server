cmake_minimum_required (VERSION 2.6)
project (reroute)

set(SOURCES
		producer/channel_producer.cpp
		producer/layer_producer.cpp
		producer/reroute_producer.cpp

		reroute.cpp
		stdafx.cpp
)
set(HEADERS
		producer/channel_producer.h
		producer/layer_producer.h
		producer/reroute_producer.h

		reroute.h
		stdafx.h
)

add_library(reroute ${SOURCES} ${HEADERS})
add_precompiled_header(reroute stdafx.h FORCEINCLUDE)

include_directories(..)
include_directories(../..)
include_directories(${BOOST_INCLUDE_PATH})
include_directories(${RXCPP_INCLUDE_PATH})
include_directories(${TBB_INCLUDE_PATH})
include_directories(${ASMLIB_INCLUDE_PATH})
include_directories(${FFMPEG_INCLUDE_PATH})

set_target_properties(reroute PROPERTIES FOLDER modules)
source_group(sources\\producer producer/*)
source_group(sources ./*)

target_link_libraries(reroute common core ffmpeg)

casparcg_add_include_statement("modules/reroute/reroute.h")
casparcg_add_init_statement("reroute::init" "reroute")
casparcg_add_module_project("reroute")
