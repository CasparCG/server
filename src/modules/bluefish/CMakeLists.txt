cmake_minimum_required (VERSION 3.16)
project (bluefish)

set(SOURCES
		consumer/bluefish_consumer.cpp
        producer/bluefish_producer.cpp
		util/blue_velvet.cpp
		bluefish.cpp
)
set(HEADERS
	    consumer/bluefish_consumer.h
        producer/bluefish_producer.h
        util/blue_velvet.h
	    util/memory.h
        bluefish.h
	StdAfx.h
)

add_library(bluefish ${SOURCES} ${HEADERS})
target_compile_features(bluefish PRIVATE cxx_std_17)
target_include_directories(bluefish PRIVATE
    ..
    ../..
    ${BOOST_INCLUDE_PATH}
    ${TBB_INCLUDE_PATH}
    ${FFMPEG_INCLUDE_PATH}
    )
target_precompile_headers(bluefish PRIVATE "StdAfx.h")
configure_file("${PROJECT_SOURCE_DIR}/packages.config" "${CMAKE_CURRENT_BINARY_DIR}/packages.config")

set_target_properties(bluefish PROPERTIES FOLDER modules)
source_group(sources ./*)
source_group(sources\\consumer consumer/*)
source_group(sources\\producer producer/*)
source_group(sources\\util util/*)
source_group(sources\\interop interop/*)

target_link_libraries(  bluefish
                        common
                        core
                        ffmpeg)

casparcg_add_include_statement("modules/bluefish/bluefish.h")
casparcg_add_init_statement("bluefish::init" "bluefish")
casparcg_add_module_project("bluefish")
