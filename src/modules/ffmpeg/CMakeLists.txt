cmake_minimum_required (VERSION 3.16)
project (ffmpeg)

set(SOURCES
	producer/av_producer.cpp
	producer/av_input.cpp
	util/av_util.cpp
	producer/ffmpeg_producer.cpp
	consumer/ffmpeg_consumer.cpp

	ffmpeg.cpp
)
set(HEADERS
	util/av_assert.h
	producer/av_producer.h
	producer/av_input.h
	util/av_util.h
	producer/ffmpeg_producer.h
	consumer/ffmpeg_consumer.h

	defines.h
	ffmpeg.h
	StdAfx.h
)

casparcg_add_module_project(ffmpeg
	SOURCES ${SOURCES} ${HEADERS}
	INIT_FUNCTION "ffmpeg::init"
	UNINIT_FUNCTION "ffmpeg::uninit"
)

target_include_directories(ffmpeg PRIVATE
    ..
    ../..
    ${FFMPEG_INCLUDE_PATH}
    )
target_precompile_headers(ffmpeg PRIVATE "StdAfx.h")

set_target_properties(ffmpeg PROPERTIES FOLDER modules)
source_group(sources ./*)
source_group(sources\\consumer ./consumer/.*)
source_group(sources\\producer ./producer/.*)
source_group(sources\\util ./util/.*)

if (MSVC)
	# target_link_libraries(ffmpeg
	# )
else()
	target_link_libraries(ffmpeg
			${FFMPEG_LIBRARIES}
	)
endif()
