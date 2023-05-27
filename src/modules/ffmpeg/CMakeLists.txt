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
)

add_library(ffmpeg ${SOURCES} ${HEADERS})
target_compile_features(ffmpeg PRIVATE cxx_std_14)
target_include_directories(ffmpeg PRIVATE
    ../../
    ${BOOST_INCLUDE_PATH}
    ${TBB_INCLUDE_PATH}
    ${FFMPEG_INCLUDE_PATH}
    )

target_compile_definitions(ffmpeg PRIVATE
    __STDC_CONSTANT_MACROS
    __STDC_LIMIT_MACROS
)

target_precompile_headers(ffmpeg PRIVATE
    "../../common/compiler/vs/disable_silly_warnings.h"
    <algorithm>
    <atomic>
    <boost/algorithm/string/predicate.hpp>
    <boost/exception/all.hpp>
    <boost/exception/exception.hpp>
    <boost/filesystem.hpp>
    <boost/format.hpp>
    <boost/lexical_cast.hpp>
    <boost/optional.hpp>
    <boost/range/algorithm/rotate.hpp>
    <boost/rational.hpp>
    <cinttypes>
    "../../common/diagnostics/graph.h"
    "../../common/env.h"
    "../../common/except.h"
    "../../common/log.h"
    "../../common/memory.h"
    "../../common/os/filesystem.h"
    "../../common/param.h"
    "../../common/scope_exit.h"
    "../../core/consumer/frame_consumer.h"
    "../../core/frame/draw_frame.h"
    "../../core/frame/frame.h"
    "../../core/frame/frame_factory.h"
    "../../core/frame/pixel_format.h"
    "../../core/fwd.h"
    "../../core/module_dependencies.h"
    "../../core/producer/frame_producer.h"
    "../../core/video_format.h"
    <condition_variable>
    <deque>
    <exception>
    <functional>
    <memory>
    <mutex>
    <queue>
    <set>
    <string>
    <tbb/concurrent_queue.h>
    <thread>
    <vector>

    # May need to go generator here
    $<$<COMPILE_LANGUAGE:C>:libavcodec/avcodec.h$<ANGLE-R>>
    $<$<COMPILE_LANGUAGE:C>:libavdevice/avdevice.h$<ANGLE-R>>
    $<$<COMPILE_LANGUAGE:C>:libavfilter/avfilter.h$<ANGLE-R>>
    $<$<COMPILE_LANGUAGE:C>:libavfilter/buffersink.h$<ANGLE-R>>
    $<$<COMPILE_LANGUAGE:C>:libavfilter/buffersrc.h$<ANGLE-R>>
    $<$<COMPILE_LANGUAGE:C>:libavformat/avformat.h$<ANGLE-R>>
    $<$<COMPILE_LANGUAGE:C>:libavutil/avutil.h$<ANGLE-R>>
    $<$<COMPILE_LANGUAGE:C>:libavutil/frame.h$<ANGLE-R>>
    $<$<COMPILE_LANGUAGE:C>:libavutil/opt.h$<ANGLE-R>>
    $<$<COMPILE_LANGUAGE:C>:libavutil/pixfmt.h$<ANGLE-R>>
    $<$<COMPILE_LANGUAGE:C>:libavutil/samplefmt.h$<ANGLE-R>>
    $<$<COMPILE_LANGUAGE:C>:libavutil/timecode.h$<ANGLE-R>>
    $<$<COMPILE_LANGUAGE:C>:libswscale/swscale.h$<ANGLE-R>>
    )
configure_file("${PROJECT_SOURCE_DIR}/packages.config" "${CMAKE_CURRENT_BINARY_DIR}/packages.config")

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
