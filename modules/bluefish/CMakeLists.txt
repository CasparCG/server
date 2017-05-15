cmake_minimum_required (VERSION 2.6)
project (bluefish)

set(SOURCES
		consumer/bluefish_consumer.cpp
		util/blue_velvet.cpp
		bluefish.cpp
		StdAfx.cpp
)
set(HEADERS
		consumer/bluefish_consumer.h
		util/blue_velvet.h
		util/memory.h
		bluefish.h
		StdAfx.h
)

add_library(bluefish ${SOURCES} ${HEADERS})
add_precompiled_header(bluefish StdAfx.h FORCEINCLUDE)

include_directories(..)
include_directories(../..)
include_directories(${BOOST_INCLUDE_PATH})
include_directories(${TBB_INCLUDE_PATH})
include_directories(${RXCPP_INCLUDE_PATH})
include_directories(${BLUEFISH_INCLUDE_PATH})

set_target_properties(bluefish PROPERTIES FOLDER modules)
source_group(sources ./*)
source_group(sources\\consumer consumer/*)
source_group(sources\\util util/*)

target_link_libraries(bluefish common core)

casparcg_add_include_statement("modules/bluefish/bluefish.h")
casparcg_add_init_statement("bluefish::init" "bluefish")
casparcg_add_module_project("bluefish")
