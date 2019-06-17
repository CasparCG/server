cmake_minimum_required (VERSION 2.6)
project (html)

set(SOURCES
		producer/html_cg_proxy.cpp
		producer/html_producer.cpp

		html.cpp
)
set(HEADERS
		producer/html_cg_proxy.h
		producer/html_producer.h

		html.h
)

add_library(html ${SOURCES} ${HEADERS})
configure_file("${PROJECT_SOURCE_DIR}/packages.config" "${CMAKE_CURRENT_BINARY_DIR}/packages.config")

include_directories(..)
include_directories(../..)
include_directories(${BOOST_INCLUDE_PATH})
include_directories(${TBB_INCLUDE_PATH})
include_directories(${CEF_INCLUDE_PATH})

set_target_properties(html PROPERTIES FOLDER modules)
source_group(sources\\producer producer/*)
source_group(sources ./*)

if(MSVC)
	target_link_libraries(html
		common
		core
		accelerator

		optimized release/libcef
		optimized release/VS2015/libcef_dll_wrapper
		debug debug/libcef
		debug debug/VS2015/libcef_dll_wrapper
	)
else()
	target_link_libraries(html
		common
		core

		${CEF_BIN_PATH}/libcef.so
		${CEF_BIN_PATH}/libcef_dll_wrapper.a
	)
endif()

casparcg_add_include_statement("modules/html/html.h")
casparcg_add_init_statement("html::init" "html")
casparcg_add_uninit_statement("html::uninit")
casparcg_add_command_line_arg_interceptor("html::intercept_command_line")
casparcg_add_module_project("html")
