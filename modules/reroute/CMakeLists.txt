cmake_minimum_required (VERSION 2.6)
project (reroute)

set(SOURCES
		producer/reroute_producer.cpp

		stdafx.cpp
)
set(HEADERS
		producer/reroute_producer.h

		stdafx.h
)

add_library(reroute ${SOURCES} ${HEADERS})
add_precompiled_header(reroute stdafx.h FORCEINCLUDE)

include_directories(..)
include_directories(../..)
include_directories(${BOOST_INCLUDE_PATH})
include_directories(${RXCPP_INCLUDE_PATH})
include_directories(${TBB_INCLUDE_PATH})

set_target_properties(reroute PROPERTIES FOLDER modules)
source_group(sources\\producer producer/*)
source_group(sources ./*)

target_link_libraries(reroute common core)
