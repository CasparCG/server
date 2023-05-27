cmake_minimum_required (VERSION 3.16)
project (oal)

set(SOURCES
		consumer/oal_consumer.cpp

		oal.cpp
)
set(HEADERS
		consumer/oal_consumer.h

		oal.h
)

add_library(oal ${SOURCES} ${HEADERS})
target_compile_features(oal PRIVATE cxx_std_14)
target_include_directories(oal PRIVATE
    ..
    ../..
    ${BOOST_INCLUDE_PATH}
    ${TBB_INCLUDE_PATH}
    ${FFMPEG_INCLUDE_PATH}
    ${OPENAL_INCLUDE_PATH}
    )
configure_file("${PROJECT_SOURCE_DIR}/packages.config" "${CMAKE_CURRENT_BINARY_DIR}/packages.config")

set_target_properties(oal PROPERTIES FOLDER modules)
source_group(sources\\consumer consumer/*)
source_group(sources ./*)

if(MSVC)
	target_link_libraries(oal
			common
			core

			OpenAL32)
else()
	target_link_libraries(oal
			common
			core

			openal)
endif()

casparcg_add_include_statement("modules/oal/oal.h")
casparcg_add_init_statement("oal::init" "oal")
casparcg_add_module_project("oal")
