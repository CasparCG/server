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

include_directories(..)
include_directories(../..)
include_directories(${BOOST_INCLUDE_PATH})
include_directories(${RXCPP_INCLUDE_PATH})
include_directories(${TBB_INCLUDE_PATH})
include_directories(${CEF_INCLUDE_PATH})
include_directories(${CEF_PATH})

set_target_properties(html PROPERTIES FOLDER modules)
source_group(sources\\producer producer/*)
source_group(sources ./*)

if(MSVC)
	target_link_libraries(html
		common
		core

		optimized release/libcef
		optimized release/libcef_dll_wrapper
		debug debug/libcef
		debug debug/libcef_dll_wrapper
	)
else()
	target_link_libraries(html
		common
		core

		cef
		cef_dll_wrapper
	)
endif()

casparcg_add_include_statement("modules/html/html.h")
casparcg_add_init_statement("html::init" "html")
casparcg_add_uninit_statement("html::uninit")
casparcg_add_command_line_arg_interceptor("html::intercept_command_line")
casparcg_add_module_project("html")

casparcg_add_runtime_dependency("${CEF_BIN_PATH}/locales")
casparcg_add_runtime_dependency("${CEF_BIN_PATH}/cef.pak")
casparcg_add_runtime_dependency("${CEF_BIN_PATH}/cef_100_percent.pak")
casparcg_add_runtime_dependency("${CEF_BIN_PATH}/cef_200_percent.pak")
casparcg_add_runtime_dependency("${CEF_BIN_PATH}/cef_extensions.pak")
casparcg_add_runtime_dependency("${CEF_BIN_PATH}/devtools_resources.pak")

if(MSVC)
	casparcg_add_runtime_dependency("${CEF_BIN_PATH}/libcef.dll")
	casparcg_add_runtime_dependency("${CEF_BIN_PATH}/d3dcompiler_43.dll")
	casparcg_add_runtime_dependency("${CEF_BIN_PATH}/d3dcompiler_47.dll")
	
	casparcg_add_runtime_dependency("${CEF_BIN_PATH}/icudtl.dat")
	casparcg_add_runtime_dependency("${CEF_BIN_PATH}/libEGL.dll")
	casparcg_add_runtime_dependency("${CEF_BIN_PATH}/libGLESv2.dll")
	casparcg_add_runtime_dependency("${CEF_BIN_PATH}/natives_blob.bin")
	casparcg_add_runtime_dependency("${CEF_BIN_PATH}/snapshot_blob.bin")
else()
	casparcg_add_runtime_dependency("${CEF_BIN_PATH}/libcef.so")
	casparcg_add_runtime_dependency("${CEF_BIN_PATH}/libffmpegsumo.so")
	casparcg_add_runtime_dependency("${CEF_BIN_PATH}/libudev.so.0")
endif()
