cmake_minimum_required (VERSION 3.16)
project (dmx)

set(SOURCES
		consumer/dmx_consumer.cpp

		util/artnet_send.cpp

		dmx.cpp
)
set(HEADERS
		consumer/dmx_consumer.h

		util/artnet_send.h

		dmx.h
)

add_library(dmx ${SOURCES} ${HEADERS})
target_compile_features(dmx PRIVATE cxx_std_14)
target_include_directories(dmx PRIVATE
    ..
    ../..
    ${BOOST_INCLUDE_PATH}
)
configure_file("${PROJECT_SOURCE_DIR}/packages.config" "${CMAKE_CURRENT_BINARY_DIR}/packages.config")

set_target_properties(dmx PROPERTIES FOLDER modules)
source_group(sources\\consumer consumer/*)
source_group(sources\\util util/*)
source_group(sources ./*)

target_link_libraries(dmx common core)

if(MSVC)
	target_link_libraries(
		dmx
		common
		core
	)
else()
	target_link_libraries(
		dmx
		common
		core
	)
endif()

casparcg_add_include_statement("modules/dmx/dmx.h")
casparcg_add_init_statement("dmx::init" "dmx")
casparcg_add_uninit_statement("dmx::uninit")
casparcg_add_module_project("dmx")
