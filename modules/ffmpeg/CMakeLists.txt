cmake_minimum_required (VERSION 2.6)
project (ffmpeg)

set(SOURCES
		consumer/ffmpeg_consumer.cpp
		consumer/streaming_consumer.cpp

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
		ffmpeg_pipeline.cpp
		ffmpeg_pipeline_backend_internal.cpp
		StdAfx.cpp
)
set(HEADERS
		consumer/ffmpeg_consumer.h
		consumer/streaming_consumer.h

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
		ffmpeg_pipeline.h
		ffmpeg_pipeline_backend.h
		ffmpeg_pipeline_backend_internal.h
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

if (MSVC)
	target_link_libraries(ffmpeg
			common
			core

			avformat.lib
			avcodec.lib
			avutil.lib
			avfilter.lib
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
			ilbc
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
			xavs
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
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/avcodec-56.dll")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/avdevice-56.dll")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/avfilter-5.dll")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/avformat-56.dll")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/avutil-54.dll")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/postproc-53.dll")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/swresample-1.dll")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/swscale-3.dll")
else()
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libavcodec.so.56")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libavdevice.so.56")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libavfilter.so.5")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libavformat.so.56")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libavutil.so.54")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libpostproc.so.53")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libswresample.so.1")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libswscale.so.3")

	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libass.so.5")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libbluray.so.1")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libcrypto.so.1.0.0")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libexpat.so.1")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libfontconfig.so.1")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libfreetype.so.6")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libfribidi.so.0")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libgme.so.0")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libgmp.so.10")
#	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libgnutls-openssl.so.27")
#	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libgnutls-xssl.so.0")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libgnutls.so.28")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libgsm.so.1")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libhogweed.so.2")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libilbc.so.2")
#	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/liblzma.so.5")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libmodplug.so.1")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libmp3lame.so.0")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libnettle.so.4")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libogg.so.0")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libopencore-amrnb.so.0")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libopencore-amrwb.so.0")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libopenjpeg.so.1")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libopus.so.0")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/liborc-0.4.so.0")
#	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/liborc-test-0.4.so.0")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libpcre.so.1")
#	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libpcrecpp.so.0")
#	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libpcreposix.so.0")
#	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libpng.so.3")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libpng12.so.0")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/librtmp.so.0")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libschroedinger-1.0.so.0")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libsoxr-lsr.so.0")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libsoxr.so.0")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libspeex.so.1")
#	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libspeexdsp.so.1")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libssl.so.1.0.0")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libtasn1.so.6")
#	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libtheora.so.1")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libtheoradec.so.1")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libtheoraenc.so.1")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libtwolame.so.0")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libva.so.1")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libvidstab.so.0.9")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libvo-aacenc.so.0")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libvo-amrwbenc.so.0")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libvorbis.so.0")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libvorbisenc.so.2")
#	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libvorbisfile.so.3")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libvpx.so.2")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libwavpack.so.1")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libwebp.so.5")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libx264.so.148")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libx265.so.59")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libxavs.so.1")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libxml2.so.2")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libxvidcore.so.4")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/libz.so.1")

endif()

