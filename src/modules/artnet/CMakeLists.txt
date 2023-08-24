cmake_minimum_required (VERSION 3.16)
project (artnet)

set(SOURCES
        consumer/artnet_consumer.cpp

		artnet.cpp
)
set(HEADERS
        consumer/artnet_consumer.h

		artnet.h
        util/fixture_calculation.cpp util/fixture_calculation.h)

add_library(artnet ${SOURCES} ${HEADERS})
target_compile_features(artnet PRIVATE cxx_std_14)
target_include_directories(artnet PRIVATE
    ..
    ../..
    ${BOOST_INCLUDE_PATH}
	${TBB_INCLUDE_PATH}
)
configure_file("${PROJECT_SOURCE_DIR}/packages.config" "${CMAKE_CURRENT_BINARY_DIR}/packages.config")

set_target_properties(artnet PROPERTIES FOLDER modules)
source_group(sources\\consumer consumer/*)
source_group(sources\\util util/*)
source_group(sources ./*)

target_link_libraries(artnet common core)

if(MSVC)
	target_link_libraries(
		artnet
		common
		core
	)
else()
	target_link_libraries(
			artnet
		common
		core
	)
endif()

casparcg_add_include_statement("modules/artnet/artnet.h")
casparcg_add_init_statement("artnet::init" "artnet")
casparcg_add_module_project("artnet")
