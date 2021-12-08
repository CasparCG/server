
core_lib = static_library(
    'core',
    'consumer/frame_consumer.cpp',
    'consumer/output.cpp',

    'diagnostics/call_context.cpp',
    'diagnostics/osd_graph.cpp',

    'frame/draw_frame.cpp',
    'frame/frame.cpp',
    'frame/frame_transform.cpp',
    'frame/geometry.cpp',

    'mixer/audio/audio_mixer.cpp',
    'mixer/image/blend_modes.cpp',
    'mixer/mixer.cpp',

    'producer/color/color_producer.cpp',
    'producer/separated/separated_producer.cpp',
    'producer/transition/transition_producer.cpp',
    'producer/transition/sting_producer.cpp',
    'producer/route/route_producer.cpp',

    'producer/cg_proxy.cpp',
    'producer/frame_producer.cpp',
    'producer/layer.cpp',
    'producer/stage.cpp',

    'video_channel.cpp',
    'video_format.cpp',

    cpp_pch : ['pch/StdAfx.h'],
    dependencies: [boost_dep, tbb_dep, sfml_dep, opengl_dep],
    include_directories: include_root,
    link_with: common_lib
)
