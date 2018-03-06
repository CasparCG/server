cmake_minimum_required (VERSION 2.6)
project (flash)

set(SOURCES
		interop/Flash9e_i.c

		producer/FlashAxContainer.cpp
		producer/flash_producer.cpp

		util/swf.cpp

		flash.cpp
		StdAfx.c
		StdAfx.cpp
)
set(HEADERS
		interop/axflash.h
		interop/TimerHelper.h

		producer/FlashAxContainer.h
		producer/flash_producer.h

		util/swf.h

		flash.h
		StdAfx.h
)

add_library(flash ${SOURCES} ${HEADERS})
add_precompiled_header(flash StdAfx.h FORCEINCLUDE)
configure_file("${PROJECT_SOURCE_DIR}/packages.config" "${CMAKE_CURRENT_BINARY_DIR}/packages.config")

include_directories(..)
include_directories(../..)
include_directories(${BOOST_INCLUDE_PATH})
include_directories(${TBB_INCLUDE_PATH})
include_directories(${ZLIB_INCLUDE_PATH})

set_target_properties(flash PROPERTIES FOLDER modules)
source_group(sources\\interop interop/*)
source_group(sources\\producer producer/*)
source_group(sources\\util util/*)
source_group(sources ./*)

target_link_libraries(flash common core)

casparcg_add_include_statement("modules/flash/flash.h")
casparcg_add_init_statement("flash::init" "flash")
casparcg_add_module_project("flash")
