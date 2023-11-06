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

	ffmpeg.h
	StdAfx.h
)

add_library(ffmpeg ${SOURCES} ${HEADERS})
target_compile_features(ffmpeg PRIVATE cxx_std_17)
target_include_directories(ffmpeg PRIVATE
    ..
    ../..
    ${BOOST_INCLUDE_PATH}
    ${TBB_INCLUDE_PATH}
    ${FFMPEG_INCLUDE_PATH}
    )
target_precompile_headers(ffmpeg PRIVATE "StdAfx.h")
casparcg_add_build_dependencies(ffmpeg)

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
			${FFMPEG_LIBRARIES}
	)
endif()

casparcg_add_include_statement("modules/ffmpeg/ffmpeg.h")
casparcg_add_init_statement("ffmpeg::init" "ffmpeg")
casparcg_add_uninit_statement("ffmpeg::uninit")
casparcg_add_module_project("ffmpeg")
