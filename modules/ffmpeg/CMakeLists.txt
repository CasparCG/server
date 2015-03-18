cmake_minimum_required (VERSION 2.6)
project (ffmpeg)

set(SOURCES
		consumer/ffmpeg_consumer.cpp

		producer/audio/audio_decoder.cpp

		producer/filter/filter.cpp

		producer/input/input.cpp

		producer/muxer/frame_muxer.cpp

		producer/util/flv.cpp
		producer/util/util.cpp

		producer/video/video_decoder.cpp

		producer/ffmpeg_producer.cpp
		producer/tbb_avcodec.cpp

		ffmpeg.cpp
		ffmpeg_error.cpp
		StdAfx.cpp
)
set(HEADERS
		consumer/ffmpeg_consumer.h

		producer/audio/audio_decoder.h

		producer/filter/filter.h

		producer/input/input.h

		producer/muxer/display_mode.h
		producer/muxer/frame_muxer.h

		producer/util/flv.h
		producer/util/util.h

		producer/video/video_decoder.h

		producer/ffmpeg_producer.h
		producer/tbb_avcodec.h

		ffmpeg.h
		ffmpeg_error.h
		StdAfx.h
)

add_library(ffmpeg ${SOURCES} ${HEADERS})
add_precompiled_header(ffmpeg StdAfx.h FORCEINCLUDE)

include_directories(..)
include_directories(../..)
include_directories(${BOOST_INCLUDE_PATH})
include_directories(${TBB_INCLUDE_PATH})
include_directories(${FFMPEG_INCLUDE_PATH})
include_directories(${RXCPP_INCLUDE_PATH})
include_directories(${ASMLIB_INCLUDE_PATH})

set_target_properties(ffmpeg PROPERTIES FOLDER modules)
source_group(sources ./*)
source_group(sources\\consumer consumer/*)
source_group(sources\\producer\\audio producer/audio/*)
source_group(sources\\producer\\filter producer/filter/*)
source_group(sources\\producer\\input producer/input/*)
source_group(sources\\producer\\muxer producer/muxer/*)
source_group(sources\\producer\\util producer/util/*)
source_group(sources\\producer\\video producer/video/*)
source_group(sources\\producer producer/*)

target_link_libraries(ffmpeg common core)
