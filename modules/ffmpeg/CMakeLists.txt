cmake_minimum_required (VERSION 2.6)
project (ffmpeg)

set(SOURCES
	producer/av_producer.cpp
	util/av_util.cpp
	producer/ffmpeg_producer.cpp
	consumer/ffmpeg_consumer.cpp

	ffmpeg.cpp
	StdAfx.cpp
)
set(HEADERS
	util/av_assert.h
	producer/av_producer.h
	util/av_util.h
	producer/ffmpeg_producer.h
	consumer/ffmpeg_consumer.cpp

	ffmpeg.h
	StdAfx.h
)

add_library(ffmpeg ${SOURCES} ${HEADERS})
add_precompiled_header(ffmpeg StdAfx.h FORCEINCLUDE)

include_directories(..)
include_directories(../..)
include_directories(${BOOST_INCLUDE_PATH})
include_directories(${TBB_INCLUDE_PATH})
include_directories(${FFMPEG_INCLUDE_PATH})

set_target_properties(ffmpeg PROPERTIES FOLDER modules)
source_group(sources ./*)
source_group(sources\\consumer ./consumer/.*)
source_group(sources\\producer ./producer/.*)
source_group(sources\\util ./util/.*)

if (MSVC)
	target_link_libraries(ffmpeg
		common
		core
	)
else()
	target_link_libraries(ffmpeg
		common
		core
	)
endif()

casparcg_add_include_statement("modules/ffmpeg/ffmpeg.h")
casparcg_add_init_statement("ffmpeg::init" "ffmpeg")
casparcg_add_uninit_statement("ffmpeg::uninit")
casparcg_add_module_project("ffmpeg")
