cmake_minimum_required (VERSION 2.6)
project (core)

set(SOURCES
		consumer/frame_consumer.cpp
		consumer/output.cpp
		consumer/port.cpp

		diagnostics/call_context.cpp
		diagnostics/osd_graph.cpp
		diagnostics/subject_diagnostics.cpp

		frame/audio_channel_layout.cpp
		frame/draw_frame.cpp
		frame/frame.cpp
		frame/frame_transform.cpp
		frame/geometry.cpp

		help/help_repository.cpp
		help/util.cpp

		mixer/audio/audio_mixer.cpp
		mixer/image/blend_modes.cpp
		mixer/mixer.cpp

		producer/color/color_producer.cpp

		producer/media_info/in_memory_media_info_repository.cpp

		producer/scene/const_producer.cpp
		producer/scene/expression_parser.cpp
		producer/scene/hotswap_producer.cpp
		producer/scene/scene_cg_proxy.cpp
		producer/scene/scene_producer.cpp
		producer/scene/xml_scene_producer.cpp

		producer/separated/separated_producer.cpp

		producer/text/text_producer.cpp
		producer/text/utils/freetype_library.cpp
		producer/text/utils/texture_atlas.cpp
		producer/text/utils/texture_font.cpp

		producer/transition/transition_producer.cpp

		producer/cg_proxy.cpp
		producer/frame_producer.cpp
		producer/layer.cpp
		producer/stage.cpp

		system_info_provider.cpp
		StdAfx.cpp
		thumbnail_generator.cpp
		video_channel.cpp
		video_format.cpp
)
set(HEADERS
		consumer/frame_consumer.h
		consumer/output.h
		consumer/port.h
		consumer/write_frame_consumer.h

		diagnostics/call_context.h
		diagnostics/osd_graph.h
		diagnostics/subject_diagnostics.h

		frame/audio_channel_layout.h
		frame/draw_frame.h
		frame/frame.h
		frame/frame_factory.h
		frame/frame_transform.h
		frame/frame_visitor.h
		frame/geometry.h
		frame/pixel_format.h

		help/help_repository.h
		help/help_sink.h
		help/util.h

		interaction/interaction_aggregator.h
		interaction/interaction_event.h
		interaction/interaction_sink.h
		interaction/util.h

		mixer/mixer.h

		monitor/monitor.h

		producer/color/color_producer.h

		producer/media_info/in_memory_media_info_repository.h
		producer/media_info/media_info.h
		producer/media_info/media_info_repository.h

		producer/scene/const_producer.h
		producer/scene/expression_parser.h
		producer/scene/hotswap_producer.h
		producer/scene/scene_cg_proxy.h
		producer/scene/scene_producer.h
		producer/scene/xml_scene_producer.h

		producer/text/utils/color.h
		producer/text/utils/freetype_library.h
		producer/text/utils/string_metrics.h
		producer/text/utils/text_info.h
		producer/text/utils/texture_atlas.h
		producer/text/utils/texture_font.h

		producer/text/text_producer.h

		producer/transition/transition_producer.h

		producer/binding.h
		producer/cg_proxy.h
		producer/frame_producer.h
		producer/layer.h
		producer/stage.h
		producer/variable.h

		fwd.h
		module_dependencies.h
		system_info_provider.h
		StdAfx.h
		thumbnail_generator.h
		video_channel.h
		video_format.h
)

add_library(core ${SOURCES} ${HEADERS})
add_precompiled_header(core StdAfx.h FORCEINCLUDE)

include_directories(..)
include_directories(${BOOST_INCLUDE_PATH})
include_directories(${RXCPP_INCLUDE_PATH})
include_directories(${TBB_INCLUDE_PATH})
include_directories(${SFML_INCLUDE_PATH})
include_directories(${FREETYPE_INCLUDE_PATH})
include_directories(${GLEW_INCLUDE_PATH})

source_group(sources ./*)
source_group(sources\\consumer consumer/*)
source_group(sources\\diagnostics diagnostics/*)
source_group(sources\\frame frame/*)
source_group(sources\\help help/*)
source_group(sources\\interaction interaction/*)
source_group(sources\\mixer mixer/*)
source_group(sources\\producer\\media_info producer/media_info/*)
source_group(sources\\producer\\scene producer/scene/*)
source_group(sources\\producer\\text\\utils producer/text/utils/*)
source_group(sources\\producer\\text producer/text/*)
source_group(sources\\producer\\transition producer/transition/*)
source_group(sources\\producer producer/*)

if (MSVC)
	target_link_libraries(core
			common

			freetype.lib
	)
else()
	target_link_libraries(core
			common

			freetype
	)
endif()
