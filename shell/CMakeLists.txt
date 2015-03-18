cmake_minimum_required (VERSION 2.6)
project (shell)

set(SOURCES
		main.cpp
		server.cpp
		stdafx.cpp
)
set(HEADERS
		resource.h
		server.h
		stdafx.h
)

add_executable(shell ${SOURCES} ${HEADERS})
add_precompiled_header(shell stdafx.h FORCEINCLUDE)

include_directories(..)
include_directories(${BOOST_INCLUDE_PATH})
include_directories(${RXCPP_INCLUDE_PATH})
include_directories(${TBB_INCLUDE_PATH})

source_group(sources ./*)

target_link_libraries(shell
		accelerator
		common
		core
		protocol

		bluefish
		decklink
		ffmpeg
		flash
		image
		oal
		psd
		reroute
		screen

		alibcof64.lib
		jpeg.lib
		sndfile.lib
		sfml-system-2.lib
		sfml-window-2.lib
		sfml-graphics-2.lib
		Winmm.lib
		Ws2_32.lib
		avformat.lib
		avcodec.lib
		avutil.lib
		avfilter.lib
		swscale.lib
		swresample.lib
		tbb.lib
		OpenGL32.lib
		glew32.lib
		openal32.lib
		FreeImage.lib
		freetype.lib
		zlibstat.lib
)
