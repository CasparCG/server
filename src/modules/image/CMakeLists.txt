cmake_minimum_required (VERSION 3.16)
project (image)

set(SOURCES
		consumer/image_consumer.cpp

		producer/image_producer.cpp

		producer/image_scroll_producer.cpp

		util/image_algorithms.cpp
		util/image_loader.cpp

		image.cpp
)
set(HEADERS
		consumer/image_consumer.h

		producer/image_producer.h

		producer/image_scroll_producer.h

		util/image_algorithms.h
		util/image_loader.h
		util/image_view.h

		image.h
)

add_library(image ${SOURCES} ${HEADERS})
target_compile_features(image PRIVATE cxx_std_17)
target_include_directories(image PRIVATE
    ..
    ../..
    ${BOOST_INCLUDE_PATH}
    ${FREEIMAGE_INCLUDE_PATH}
    ${TBB_INCLUDE_PATH}
    )
configure_file("${PROJECT_SOURCE_DIR}/packages.config" "${CMAKE_CURRENT_BINARY_DIR}/packages.config")

set_target_properties(image PROPERTIES FOLDER modules)
source_group(sources\\consumer consumer/*)
source_group(sources\\producer producer/*)
source_group(sources\\util util/*)
source_group(sources ./*)

target_link_libraries(image common core)

if(MSVC)
	target_link_libraries(image
		common
		core

		optimized FreeImage.lib
		debug FreeImaged.lib
	)
else()
	target_link_libraries(image
		common
		core

		freeimage
	)
endif()

casparcg_add_include_statement("modules/image/image.h")
casparcg_add_init_statement("image::init" "image")
casparcg_add_uninit_statement("image::uninit")
casparcg_add_module_project("image")
