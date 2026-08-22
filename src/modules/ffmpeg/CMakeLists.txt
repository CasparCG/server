cmake_minimum_required (VERSION 3.28)
project (ffmpeg)

set(SOURCES
	producer/av_producer.cpp
	producer/av_producer.h
	producer/av_input.cpp
	producer/av_input.h
	producer/ffmpeg_producer.cpp
	producer/ffmpeg_producer.h
	producer/video_strategy.h
	producer/cpu_video_strategy.cpp
	consumer/ffmpeg_consumer.cpp
	consumer/ffmpeg_consumer.h

	util/av_util.cpp
	util/av_util.h
	util/audio_resampler.cpp
	util/audio_resampler.h
	util/av_assert.h

	ffmpeg.cpp
	ffmpeg.h
	StdAfx.h
)

if (ENABLE_VULKAN)
	list(APPEND SOURCES
		producer/vulkan_frame_import.h
		producer/vulkan_frame_import.cpp
		producer/vulkan_video_strategy.cpp
		producer/cuda_video_strategy.cpp
	)
endif ()

if (ENABLE_VULKAN)
	casparcg_add_module_project(ffmpeg
		SOURCES ${SOURCES}
		INIT_FUNCTION "ffmpeg::init"
		UNINIT_FUNCTION "ffmpeg::uninit"
		VULKAN_REQUIREMENTS_FUNCTION "ffmpeg::register_vulkan_requirements"
	)
else ()
	casparcg_add_module_project(ffmpeg
		SOURCES ${SOURCES}
		INIT_FUNCTION "ffmpeg::init"
		UNINIT_FUNCTION "ffmpeg::uninit"
	)
endif ()

target_include_directories(ffmpeg PRIVATE
    ${FFMPEG_INCLUDE_PATH}
    )
target_precompile_headers(ffmpeg PRIVATE "StdAfx.h")

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	# GCC 14 false positive inside boost::regex's parser (vector<digraph<char>> realloc).
	set_source_files_properties(producer/av_input.cpp PROPERTIES COMPILE_OPTIONS "-Wno-stringop-overflow")
endif()

set_target_properties(ffmpeg PROPERTIES FOLDER modules)
source_group(sources ./*)
source_group(sources\\consumer ./consumer/.*)
source_group(sources\\producer ./producer/.*)
source_group(sources\\util ./util/.*)

if (MSVC)
	# target_link_libraries(ffmpeg
	# )
else()
	target_link_libraries(ffmpeg PRIVATE
			${FFMPEG_LIBRARIES}
	)
endif()

if (ENABLE_VULKAN)
	# The Vulkan video strategy decodes into the accelerator's own device, so it needs the
	# accelerator's Vulkan types as well as FFmpeg's.
	target_compile_definitions(ffmpeg PUBLIC ENABLE_VULKAN VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)
	target_link_libraries(ffmpeg PRIVATE accelerator Vulkan::Headers vk-bootstrap::vk-bootstrap)
	if (WIN32)
		target_compile_definitions(ffmpeg PUBLIC VK_USE_PLATFORM_WIN32_KHR)
	endif ()
endif ()
