cmake_minimum_required (VERSION 3.16)
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
target_compile_features(html PRIVATE cxx_std_14)
target_include_directories(html PRIVATE
    ..
    ../..
    ${BOOST_INCLUDE_PATH}
    ${TBB_INCLUDE_PATH}
    ${CEF_INCLUDE_PATH}
    )
configure_file("${PROJECT_SOURCE_DIR}/packages.config" "${CMAKE_CURRENT_BINARY_DIR}/packages.config")


set_target_properties(html PROPERTIES FOLDER modules)
source_group(sources\\producer producer/*)
source_group(sources ./*)

if(MSVC)
	target_link_libraries(html
		common
		core
		accelerator

		optimized libcef
		optimized release/VS2019/libcef_dll_wrapper
		debug debug/libcef
		debug debug/VS2019/libcef_dll_wrapper
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
