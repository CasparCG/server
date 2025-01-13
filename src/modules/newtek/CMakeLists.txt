cmake_minimum_required (VERSION 3.16)
project (newtek)

set(SOURCES
		consumer/newtek_ndi_consumer.cpp
		consumer/newtek_ndi_consumer.h

		producer/newtek_ndi_producer.cpp
		producer/newtek_ndi_producer.h

		util/ndi.cpp
		util/ndi.h

		newtek.cpp
		newtek.h

		interop/Processing.NDI.compat.h
		interop/Processing.NDI.deprecated.h
		interop/Processing.NDI.DynamicLoad.h
		interop/Processing.NDI.Find.h
		interop/Processing.NDI.FrameSync.h
		interop/Processing.NDI.Lib.cplusplus.h
		interop/Processing.NDI.Lib.h
		interop/Processing.NDI.Recv.ex.h
		interop/Processing.NDI.Recv.h
		interop/Processing.NDI.Routing.h
		interop/Processing.NDI.Send.h
		interop/Processing.NDI.structs.h
		interop/Processing.NDI.utilities.h

		StdAfx.h
)

casparcg_add_module_project(newtek
	SOURCES ${SOURCES}
	INIT_FUNCTION "newtek::init"
)
target_include_directories(newtek PRIVATE
	..
    ../..
    ${FFMPEG_INCLUDE_PATH}
    )
target_precompile_headers(newtek PRIVATE "StdAfx.h")

set_target_properties(newtek PROPERTIES FOLDER modules)
source_group(sources\\consumer consumer/*)
source_group(sources\\producer producer/*)
source_group(sources\\interop interop/*)
source_group(sources\\util util/*)
source_group(sources ./*)

target_link_libraries(newtek ffmpeg)
