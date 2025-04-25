cmake_minimum_required (VERSION 3.16)
project (image)

set(SOURCES
		consumer/image_consumer.cpp
		consumer/image_consumer.h

		producer/image_producer.cpp
		producer/image_producer.h

		producer/image_scroll_producer.cpp
		producer/image_scroll_producer.h

		util/image_algorithms.cpp
		util/image_algorithms.h
		util/image_converter.cpp
		util/image_converter.h
		util/image_loader.cpp
		util/image_loader.h
		util/image_view.h

		image.cpp
		image.h
)

casparcg_add_module_project(image
	SOURCES ${SOURCES}
	INIT_FUNCTION "image::init"
)
target_include_directories(image PRIVATE
	..
    ../..
	${FFMPEG_INCLUDE_PATH}
    )

set_target_properties(image PROPERTIES FOLDER modules)
source_group(sources\\consumer consumer/*)
source_group(sources\\producer producer/*)
source_group(sources\\util util/*)
source_group(sources ./*)

target_link_libraries(image PRIVATE
	ffmpeg
)

