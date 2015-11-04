cmake_minimum_required (VERSION 2.6)
project (ffmpeg)

set(SOURCES
		consumer/ffmpeg_consumer.cpp

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
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${TBB_INCLUDE_DIRS})
include_directories(${FFmpeg_INCLUDE_DIRS})
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

if(NOT USE_SYSTEM_FFMPEG)
	if(MSVC)
		set(FFmpeg_LIBRARIES
				avformat.lib
				avcodec.lib
				avutil.lib
				avfilter.lib
                avdevice.lib
				swscale.lib
				swresample.lib)
	else()
		set(FFmpeg_LIBRARIES
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
				-Wl,--end-group)
	endif()
endif()

target_link_libraries(ffmpeg common core "${FFmpeg_LIBRARIES}")

casparcg_add_include_statement("modules/ffmpeg/ffmpeg.h")
casparcg_add_init_statement("ffmpeg::init" "ffmpeg")
casparcg_add_uninit_statement("ffmpeg::uninit")
casparcg_add_module_project("ffmpeg")

if(NOT USE_SYSTEM_FFMPEG)
	if(MSVC)
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/avcodec-56.dll")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/avdevice-56.dll")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/avfilter-5.dll")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/avformat-56.dll")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/avutil-54.dll")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/postproc-53.dll")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/swresample-1.dll")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/swscale-3.dll")
	else()
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libavcodec.so.56")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libavdevice.so.56")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libavfilter.so.5")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libavformat.so.56")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libavutil.so.54")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libpostproc.so.53")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libswresample.so.1")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libswscale.so.3")

		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libass.so.5")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libbluray.so.1")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libcrypto.so.1.0.0")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libexpat.so.1")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libfontconfig.so.1")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libfreetype.so.6")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libfribidi.so.0")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libgme.so.0")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libgmp.so.10")
#		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libgnutls-openssl.so.27")
#		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libgnutls-xssl.so.0")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libgnutls.so.28")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libgsm.so.1")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libhogweed.so.2")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libilbc.so.2")
#		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/liblzma.so.5")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libmodplug.so.1")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libmp3lame.so.0")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libnettle.so.4")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libogg.so.0")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libopencore-amrnb.so.0")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libopencore-amrwb.so.0")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libopenjpeg.so.1")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libopus.so.0")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/liborc-0.4.so.0")
#		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/liborc-test-0.4.so.0")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libpcre.so.1")
#		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libpcrecpp.so.0")
#		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libpcreposix.so.0")
#		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libpng.so.3")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libpng12.so.0")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/librtmp.so.0")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libschroedinger-1.0.so.0")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libsoxr-lsr.so.0")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libsoxr.so.0")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libspeex.so.1")
#		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libspeexdsp.so.1")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libssl.so.1.0.0")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libtasn1.so.6")
#		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libtheora.so.1")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libtheoradec.so.1")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libtheoraenc.so.1")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libtwolame.so.0")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libva.so.1")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libvidstab.so.0.9")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libvo-aacenc.so.0")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libvo-amrwbenc.so.0")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libvorbis.so.0")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libvorbisenc.so.2")
#		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libvorbisfile.so.3")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libvpx.so.2")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libwavpack.so.1")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libwebp.so.5")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libx264.so.148")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libx265.so.59")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libxavs.so.1")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libxml2.so.2")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libxvidcore.so.4")
		casparcg_add_runtime_dependency("${FFmpeg_BIN_DIR}/libz.so.1")
	endif()
endif()
