cmake_minimum_required (VERSION 3.16)
project (flash)

set(SOURCES
		interop/Flash9e_i.c
		interop/axflash.h
		interop/TimerHelper.h

		producer/FlashAxContainer.cpp
		producer/FlashAxContainer.h
		producer/flash_producer.cpp
		producer/flash_producer.h

		util/swf.cpp
		util/swf.h

		flash.cpp
		flash.h
		StdAfx.h
)

casparcg_add_module_project(flash
	SOURCES ${SOURCES}
	INIT_FUNCTION "flash::init"
)
target_include_directories(flash PRIVATE
    ${ZLIB_INCLUDE_PATH}
    )
target_precompile_headers(flash PRIVATE "StdAfx.h")

set_target_properties(flash PROPERTIES FOLDER modules)
source_group(sources\\interop interop/*)
source_group(sources\\producer producer/*)
source_group(sources\\util util/*)
source_group(sources ./*)


