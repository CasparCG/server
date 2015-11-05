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
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${RXCPP_INCLUDE_PATH})
include_directories(${TBB_INCLUDE_DIRS})
include_directories(${OPENAL_INCLUDE_DIR})

set_target_properties(oal PROPERTIES FOLDER modules)
source_group(sources\\consumer consumer/*)
source_group(sources ./*)

target_link_libraries(oal
	common
	core

	"${OPENAL_LIBRARY}"
)

casparcg_add_include_statement("modules/oal/oal.h")
casparcg_add_init_statement("oal::init" "oal")
casparcg_add_module_project("oal")

if(NOT USE_SYSTEM_OPENAL)
	if(MSVC)
		casparcg_add_runtime_dependency("${OPENAL_BIN_DIR}/OpenAL32.dll")
	else()
		casparcg_add_runtime_dependency("${OPENAL_BIN_DIR}/libopenal.so.1")
	endif()
endif()
