cmake_minimum_required (VERSION 3.28)
project (html)

set(SOURCES
		producer/html_cg_proxy.cpp
		producer/html_cg_proxy.h
		producer/html_producer.cpp
		producer/html_producer.h

		html.cpp
		html.h
		util.h
)

if (APPLE)
	list(APPEND SOURCES html_mac.mm html_mac.h)
	set_source_files_properties(html_mac.mm PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
endif ()

casparcg_add_module_project(html
	SOURCES ${SOURCES}
	INIT_FUNCTION "html::init"
	UNINIT_FUNCTION "html::uninit"
	CLI_INTERCEPTOR "html::intercept_command_line"
)
target_link_libraries(html PRIVATE ffmpeg CEF::CEF)

target_include_directories(html PRIVATE
    ..
	${FFMPEG_INCLUDE_PATH}
)

set_target_properties(html PROPERTIES FOLDER modules)
source_group(sources\\producer producer/*)
source_group(sources ./*)
