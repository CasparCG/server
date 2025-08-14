cmake_minimum_required (VERSION 3.16)
project (oal)

set(SOURCES
		consumer/oal_consumer.cpp
		consumer/oal_consumer.h

		oal.cpp
		oal.h
)

casparcg_add_module_project(oal
	SOURCES ${SOURCES}
	INIT_FUNCTION "oal::init"
)
target_include_directories(oal PRIVATE
    ${FFMPEG_INCLUDE_PATH}
)
target_link_libraries(oal PRIVATE OpenAL::OpenAL)

set_target_properties(oal PROPERTIES FOLDER modules)
source_group(sources\\consumer consumer/*)
source_group(sources ./*)
