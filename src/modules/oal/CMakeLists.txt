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

casparcg_add_module_project(oal
	SOURCES ${SOURCES} ${HEADERS}
	INIT_FUNCTION "oal::init"
)
target_include_directories(oal PRIVATE
    ..
    ../..
    ${FFMPEG_INCLUDE_PATH}
    ${OPENAL_INCLUDE_PATH}
    )

set_target_properties(oal PROPERTIES FOLDER modules)
source_group(sources\\consumer consumer/*)
source_group(sources ./*)

if(MSVC)
	target_link_libraries(oal
			OpenAL32)
else()
	target_link_libraries(oal
			openal)
endif()
