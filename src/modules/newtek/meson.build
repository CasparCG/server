newtek_lib = static_library(
    'newtek',
    'consumer/newtek_ndi_consumer.cpp',

    'producer/newtek_ndi_producer.cpp',

    'util/ndi.cpp',

    'newtek.cpp',

    include_directories: [include_root, include_modules],
    link_with: [common_lib, protocol_lib, core_lib, ffmpeg_lib],
    dependencies: [boost_dep, ffmpeg_deps],
    cpp_pch : ['pch/StdAfx.h'],
)

modules += newtek_lib
modules_include += 'modules/newtek/newtek.h'
modules_init += 'newtek::init newtek'
modules_uninit += 'image::uninit'
