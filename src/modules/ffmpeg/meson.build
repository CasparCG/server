
ffmpeg_lib = static_library(
    'ffmpeg',
    'producer/av_producer.cpp',
	'producer/av_input.cpp',
	'util/av_util.cpp',
	'producer/ffmpeg_producer.cpp',
	'consumer/ffmpeg_consumer.cpp',

	'ffmpeg.cpp',

    link_with: [core_lib, common_lib],
    cpp_pch : ['pch/StdAfx.h'],
    dependencies: [tbb_dep, boost_dep, ffmpeg_deps],
    include_directories: include_root,
)

modules += ffmpeg_lib
modules_include += 'modules/ffmpeg/ffmpeg.h'
modules_init += 'ffmpeg::init ffmpeg'
modules_uninit += 'ffmpeg::uninit'
