cmake_minimum_required (VERSION 2.6)
project (shell)

join_list("${CASPARCG_MODULE_INCLUDE_STATEMENTS}" "\n" CASPARCG_MODULE_INCLUDE_STATEMENTS)
join_list("${CASPARCG_MODULE_INIT_STATEMENTS}" "\n" CASPARCG_MODULE_INIT_STATEMENTS)
join_list("${CASPARCG_MODULE_UNINIT_STATEMENTS}" "\n" CASPARCG_MODULE_UNINIT_STATEMENTS)
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

set(OUTPUT_FOLDER "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}")

add_custom_command(TARGET casparcg POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy \"${FFMPEG_BIN_PATH}/avcodec-55.dll\" \"${OUTPUT_FOLDER}/\"
		COMMAND ${CMAKE_COMMAND} -E copy \"${FFMPEG_BIN_PATH}/avdevice-55.dll\" \"${OUTPUT_FOLDER}/\"
		COMMAND ${CMAKE_COMMAND} -E copy \"${FFMPEG_BIN_PATH}/avfilter-4.dll\" \"${OUTPUT_FOLDER}/\"
		COMMAND ${CMAKE_COMMAND} -E copy \"${FFMPEG_BIN_PATH}/avformat-55.dll\" \"${OUTPUT_FOLDER}/\"
		COMMAND ${CMAKE_COMMAND} -E copy \"${FFMPEG_BIN_PATH}/avutil-52.dll\" \"${OUTPUT_FOLDER}/\"
		COMMAND ${CMAKE_COMMAND} -E copy \"${FFMPEG_BIN_PATH}/postproc-52.dll\" \"${OUTPUT_FOLDER}/\"
		COMMAND ${CMAKE_COMMAND} -E copy \"${FFMPEG_BIN_PATH}/swresample-0.dll\" \"${OUTPUT_FOLDER}/\"
		COMMAND ${CMAKE_COMMAND} -E copy \"${FFMPEG_BIN_PATH}/swscale-2.dll\" \"${OUTPUT_FOLDER}/\"
		COMMAND ${CMAKE_COMMAND} -E copy \"${FREEIMAGE_BIN_PATH}/FreeImage.dll\" \"${OUTPUT_FOLDER}/\"
		COMMAND ${CMAKE_COMMAND} -E copy \"${FREEIMAGE_BIN_PATH}/FreeImaged.dll\" \"${OUTPUT_FOLDER}/\"
		COMMAND ${CMAKE_COMMAND} -E copy \"${GLEW_BIN_PATH}/glew32.dll\" \"${OUTPUT_FOLDER}/\"
		COMMAND ${CMAKE_COMMAND} -E copy \"${OPENAL_BIN_PATH}/OpenAL32.dll\" \"${OUTPUT_FOLDER}/\"
		COMMAND ${CMAKE_COMMAND} -E copy \"${OPENAL_BIN_PATH}/wrap_oal.dll\" \"${OUTPUT_FOLDER}/\"
		COMMAND ${CMAKE_COMMAND} -E copy \"${TBB_BIN_PATH}/tbb.dll\" \"${OUTPUT_FOLDER}/\"
		COMMAND ${CMAKE_COMMAND} -E copy \"${TBB_BIN_PATH}/tbb_debug.dll\" \"${OUTPUT_FOLDER}/\"
		COMMAND ${CMAKE_COMMAND} -E copy \"${TBB_BIN_PATH}/tbbmalloc.dll\" \"${OUTPUT_FOLDER}/\"
		COMMAND ${CMAKE_COMMAND} -E copy \"${TBB_BIN_PATH}/tbbmalloc_debug.dll\" \"${OUTPUT_FOLDER}/\"
		COMMAND ${CMAKE_COMMAND} -E copy \"${TBB_BIN_PATH}/tbbmalloc_proxy.dll\" \"${OUTPUT_FOLDER}/\"
		COMMAND ${CMAKE_COMMAND} -E copy \"${TBB_BIN_PATH}/tbbmalloc_proxy_debug.dll\" \"${OUTPUT_FOLDER}/\"

		COMMAND ${CMAKE_COMMAND} -E copy \"${LIBERATION_FONTS_BIN_PATH}/LiberationSans-Regular.ttf\" \"${OUTPUT_FOLDER}/\"
		COMMAND ${CMAKE_COMMAND} -E copy \"${LIBERATION_FONTS_BIN_PATH}/LiberationSans-Regular.ttf\" \"${CMAKE_CURRENT_BINARY_DIR}/\"

		COMMAND ${CMAKE_COMMAND} -E copy \"${CMAKE_CURRENT_SOURCE_DIR}/casparcg.config\" \"${OUTPUT_FOLDER}/\"
		COMMAND ${CMAKE_COMMAND} -E copy \"${CMAKE_CURRENT_SOURCE_DIR}/casparcg.config\" \"${CMAKE_CURRENT_BINARY_DIR}/\"
)
