cmake_minimum_required (VERSION 2.6)
project (unit-test)

set(SOURCES
		audio_channel_layout_test.cpp
		base64_test.cpp
		image_mixer_test.cpp
		main.cpp
		param_test.cpp
		stdafx.cpp
		tweener_test.cpp
)
set(HEADERS
		stdafx.h
)

add_executable(unit-test ${SOURCES} ${HEADERS})
add_precompiled_header(unit-test stdafx.h FORCEINCLUDE)

target_link_libraries(unit-test
		accelerator
		common
		core
		ffmpeg
)

if (MSVC)
	target_link_libraries(unit-test
		optimized gtest.lib
		debug gtestd.lib
	)
else ()
	target_link_libraries(unit-test
		gtest
	)
endif ()

include_directories(..)
include_directories(${BOOST_INCLUDE_PATH})
include_directories(${TBB_INCLUDE_PATH})
include_directories(${GTEST_INCLUDE_PATH})

source_group(sources ./*)

set(OUTPUT_FOLDER "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}")

foreach(FILE_TO_COPY ${CASPARCG_RUNTIME_DEPENDENCIES})
	if(IS_DIRECTORY ${FILE_TO_COPY})
		get_filename_component(FOLDER_NAME "${FILE_TO_COPY}" NAME)
		add_custom_command(TARGET unit-test POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory \"${FILE_TO_COPY}\" \"${OUTPUT_FOLDER}/${FOLDER_NAME}\")
		add_custom_command(TARGET unit-test POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory \"${FILE_TO_COPY}\" \"${CMAKE_CURRENT_BINARY_DIR}/${FOLDER_NAME}\")
	else()
		add_custom_command(TARGET unit-test POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy \"${FILE_TO_COPY}\" \"${OUTPUT_FOLDER}/\")
		add_custom_command(TARGET unit-test POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy \"${FILE_TO_COPY}\" \"${CMAKE_CURRENT_BINARY_DIR}/\")
	endif()
endforeach(FILE_TO_COPY)
