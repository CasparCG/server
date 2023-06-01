cmake_minimum_required (VERSION 3.16)
project (flash)

set(SOURCES
		interop/Flash9e_i.c

		producer/FlashAxContainer.cpp
		producer/flash_producer.cpp

		util/swf.cpp

		flash.cpp
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
target_compile_features(flash PRIVATE cxx_std_14)
target_include_directories(flash PRIVATE
    ..
    ../..
    ${BOOST_INCLUDE_PATH}
    ${TBB_INCLUDE_PATH}
    ${ZLIB_INCLUDE_PATH}
    )
target_precompile_headers(flash PRIVATE "StdAfx.h")
configure_file("${PROJECT_SOURCE_DIR}/packages.config" "${CMAKE_CURRENT_BINARY_DIR}/packages.config")


set_target_properties(flash PROPERTIES FOLDER modules)
source_group(sources\\interop interop/*)
source_group(sources\\producer producer/*)
source_group(sources\\util util/*)
source_group(sources ./*)

target_link_libraries(flash common core)

casparcg_add_include_statement("modules/flash/flash.h")
casparcg_add_init_statement("flash::init" "flash")
casparcg_add_module_project("flash")
