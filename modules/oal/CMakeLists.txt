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

include_directories(..)
include_directories(../..)
include_directories(${BOOST_INCLUDE_PATH})
include_directories(${RXCPP_INCLUDE_PATH})
include_directories(${TBB_INCLUDE_PATH})
include_directories(${OPENAL_INCLUDE_PATH})

set_target_properties(oal PROPERTIES FOLDER modules)
source_group(sources\\consumer consumer/*)
source_group(sources ./*)

target_link_libraries(oal common core)
