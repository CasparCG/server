cmake_minimum_required (VERSION 2.6)
project (ffmpeg)

set(SOURCES
		ffmpeg.cpp
		StdAfx.cpp
)
set(HEADERS
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
source_group(sources\\consumer consumer/*)
source_group(sources\\producer producer/*)

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
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/avcodec-56.dll")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/avdevice-56.dll")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/avfilter-5.dll")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/avformat-56.dll")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/avutil-54.dll")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/postproc-53.dll")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/swresample-1.dll")
	casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/swscale-3.dll")
else()

endif()
