cmake_minimum_required (VERSION 2.6)
project (ffmpeg)

set(SOURCES
		consumer/ffmpeg_consumer.cpp

		producer2/av_producer.cpp
		producer2/av_util.cpp
		producer2/ffmpeg_producer2.cpp

		producer/audio/audio_decoder.cpp

		producer/filter/audio_filter.cpp
		producer/filter/filter.cpp

		producer/input/input.cpp

		producer/muxer/frame_muxer.cpp

		producer/util/flv.cpp
		producer/util/util.cpp

		producer/video/video_decoder.cpp

		producer/ffmpeg_producer.cpp
		producer/tbb_avcodec.cpp

		audio_channel_remapper.cpp
		ffmpeg.cpp
		ffmpeg_error.cpp
		StdAfx.cpp
)
set(HEADERS
		consumer/ffmpeg_consumer.h

		producer2/av_assert.h
		producer2/av_producer.h
		producer2/av_util.h
		producer2/ffmpeg_producer2.h

		producer/audio/audio_decoder.h

		producer/filter/audio_filter.h
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

set_target_properties(ffmpeg PROPERTIES FOLDER modules)
source_group(sources ./*)
source_group(sources\\consumer ./consumer/*)
source_group(sources\\producer\\audio ./producer/audio/*)
source_group(sources\\producer\\filter ./producer/filter/*)
source_group(sources\\producer\\input ./producer/input/*)
source_group(sources\\producer\\muxer ./producer/muxer/*)
source_group(sources\\producer\\util ./producer/util/*)
source_group(sources\\producer\\video ./producer/video/*)
source_group(sources\\producer2 ./producer2/.*)
source_group(sources\\producer ./producer/.*)

if (MSVC)
	target_link_libraries(ffmpeg
			common
			core

			avformat.lib
			avcodec.lib
			avutil.lib
			avfilter.lib
			avdevice.lib
			swscale.lib
			swresample.lib
	)
else()
	target_link_libraries(ffmpeg
			common
			core

			-Wl,--start-group
			avformat.so
			avcodec.so
			avutil.so
			avfilter.so
			avdevice.so
			swscale.so
			swresample.so
			postproc.so

			ass
			bluray
			crypto
			expat
			fontconfig
			fribidi
			gmp
			gnutls
			gsm
			hogweed
			modplug
			mp3lame
			nettle
			ogg
			opencore-amrnb
			opencore-amrwb
			opus
			rtmp
			ssl
			tasn1
			theoradec
			theoraenc
			vorbis
			vorbisenc
			vpx
			x264
			xvidcore
			z
			-Wl,--end-group
	)
endif()

casparcg_add_include_statement("modules/ffmpeg/ffmpeg.h")
casparcg_add_init_statement("ffmpeg::init" "ffmpeg")
casparcg_add_uninit_statement("ffmpeg::uninit")
casparcg_add_module_project("ffmpeg")

if(MSVC)
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/avcodec-58.dll")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/avdevice-58.dll")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/avfilter-7.dll")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/avformat-58.dll")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/avutil-56.dll")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/postproc-55.dll")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/swresample-3.dll")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/swscale-5.dll")
else()

endif()
