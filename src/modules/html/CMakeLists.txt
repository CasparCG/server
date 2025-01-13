cmake_minimum_required (VERSION 3.16)
project (html)

set(SOURCES
		producer/html_cg_proxy.cpp
		producer/html_cg_proxy.h
		producer/html_producer.cpp
		producer/html_producer.h

		html.cpp
		html.h
)

casparcg_add_module_project(html
	SOURCES ${SOURCES}
	INIT_FUNCTION "html::init"
	UNINIT_FUNCTION "html::uninit"
	CLI_INTERCEPTOR "html::intercept_command_line"
)

target_include_directories(html PRIVATE
    ../..
    ${CEF_INCLUDE_PATH}
    )
target_link_libraries(html ${CEF_LIB})

set_target_properties(html PROPERTIES FOLDER modules)
source_group(sources\\producer producer/*)
source_group(sources ./*)
