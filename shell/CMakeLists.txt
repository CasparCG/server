cmake_minimum_required (VERSION 2.6)
project (shell)

join_list("${CASPARCG_MODULE_INCLUDE_STATEMENTS}" "\n" CASPARCG_MODULE_INCLUDE_STATEMENTS)
join_list("${CASPARCG_MODULE_INIT_STATEMENTS}" "\n" CASPARCG_MODULE_INIT_STATEMENTS)
join_list("${CASPARCG_MODULE_UNINIT_STATEMENTS}" "\n" CASPARCG_MODULE_UNINIT_STATEMENTS)
join_list("${CASPARCG_MODULE_COMMAND_LINE_ARG_INTERCEPTORS_STATEMENTS}" "\n" CASPARCG_MODULE_COMMAND_LINE_ARG_INTERCEPTORS_STATEMENTS)
configure_file("${PROJECT_SOURCE_DIR}/included_modules.tmpl" "${PROJECT_SOURCE_DIR}/included_modules.h")

if (MSVC)
	set(OS_SPECIFIC_SOURCES
			resource.h
			shell.rc
			windows_specific.cpp
	)
elseif (CMAKE_COMPILER_IS_GNUCXX)
	set(OS_SPECIFIC_SOURCES
			linux_specific.cpp
	)
endif ()
set(SOURCES
		casparcg.config
		main.cpp
		server.cpp
		stdafx.cpp
)
set(HEADERS
		included_modules.h
		platform_specific.h
		server.h
		stdafx.h
)

add_executable(casparcg ${SOURCES} ${HEADERS} ${OS_SPECIFIC_SOURCES})
add_precompiled_header(casparcg stdafx.h FORCEINCLUDE)

include_directories(..)
include_directories(${BOOST_INCLUDE_PATH})
include_directories(${RXCPP_INCLUDE_PATH})
include_directories(${TBB_INCLUDE_PATH})

source_group(sources ./*)

if (MSVC)
	target_link_libraries(casparcg
		zlibstat.lib
	)
endif ()

target_link_libraries(casparcg
		accelerator
		common
		core
		protocol

		"${CASPARCG_MODULE_PROJECTS}"

		reroute
)

add_custom_target(casparcg_copy_dependencies ALL)

casparcg_add_runtime_dependency("${LIBERATION_FONTS_BIN_PATH}/LiberationSans-Regular.ttf")
casparcg_add_runtime_dependency("${CMAKE_CURRENT_SOURCE_DIR}/casparcg.config")

set(OUTPUT_FOLDER "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}")

foreach(FILE_TO_COPY ${CASPARCG_RUNTIME_DEPENDENCIES})
	if(IS_DIRECTORY "${FILE_TO_COPY}")
		add_custom_command(TARGET casparcg_copy_dependencies POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory \"${FILE_TO_COPY}\" \"${OUTPUT_FOLDER}/\")
		add_custom_command(TARGET casparcg_copy_dependencies POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory \"${FILE_TO_COPY}\" \"${CMAKE_CURRENT_BINARY_DIR}/\")
	else()
		add_custom_command(TARGET casparcg_copy_dependencies POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy \"${FILE_TO_COPY}\" \"${OUTPUT_FOLDER}/\")
		add_custom_command(TARGET casparcg_copy_dependencies POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy \"${FILE_TO_COPY}\" \"${CMAKE_CURRENT_BINARY_DIR}/\")
	endif()
endforeach(FILE_TO_COPY)
