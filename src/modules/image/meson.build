
image_lib = static_library(
    'image',
    'consumer/image_consumer.cpp',

	'producer/image_producer.cpp',

	'producer/image_scroll_producer.cpp',

	'util/image_algorithms.cpp',
	'util/image_loader.cpp',

	'image.cpp',

    link_with: [core_lib, common_lib],
    dependencies: [boost_dep, freeimage_dep],
    include_directories: [include_root, include_modules],
)
modules += image_lib
modules_include += 'modules/image/image.h'
modules_init += 'image::init image'
