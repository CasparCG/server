linux_sources = [
    'linux_interop/DeckLinkAPIDispatch.cpp'
]

decklink_lib = static_library(
    'decklink',
    'consumer/decklink_consumer.cpp',

    'producer/decklink_producer.cpp',

    'decklink.cpp',
    linux_sources,

    link_with : [common_lib, core_lib, ffmpeg_lib],
    dependencies: [tbb_malloc, tbb_malloc_proxy, tbb_dep, boost_dep, ffmpeg_deps],
    cpp_pch: ['pch/StdAfx.h'],
    include_directories: [include_root, include_modules]
)

modules += decklink_lib
modules_include += 'modules/decklink/decklink.h'
modules_init += 'decklink::init decklink'
