cmake_minimum_required (VERSION 3.28)
project (dmf_mxl)

# @todo - make more portable:
set(MXL_SDK_PREFIX "$ENV{HOME}/Documents/devProjects/mxl/install/Linux-Clang-Release/" CACHE PATH "Path to MXL SDK")
LIST (APPEND CMAKE_PREFIX_PATH "${MXL_SDK_PREFIX}")
# set(CMAKE_PREFIX_PATH "${MXL_SDK_PREFIX}" ${CMAKE_PREFIX_PATH})
message(STATUS "CMAKE_PREFIX_PATH is: ${CMAKE_PREFIX_PATH}")

find_package(mxl CONFIG REQUIRED)


set(SOURCES
		dmf_mxl.cpp
		dmf_mxl.h

		producer/dmf_mxl_producer.cpp
		producer/dmf_mxl_producer.h

		StdAfx.h
)

casparcg_add_module_project(dmf_mxl
	SOURCES ${SOURCES}
	INIT_FUNCTION "dmf_mxl::init"
)
target_include_directories(dmf_mxl PRIVATE
    ..
    ${FFMPEG_INCLUDE_PATH}
    )
target_precompile_headers(dmf_mxl PRIVATE "StdAfx.h")

set_target_properties(dmf_mxl PROPERTIES FOLDER modules)
source_group(sources\\producer producer/*)
source_group(sources ./*)

target_link_libraries(dmf_mxl
    PRIVATE
    mxl::mxl
	ffmpeg
)