
oal_lib = static_library(
    'oal',
	'consumer/oal_consumer.cpp',

	'oal.cpp',

    link_with: [common_lib, core_lib],
    include_directories: [include_root],
    dependencies: [openal_dep, boost_dep, ffmpeg_deps]
)

modules += oal_lib
modules_include += 'modules/oal/oal.h'
modules_init += 'oal::init oal'
