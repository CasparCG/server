cmake_minimum_required (VERSION 2.6)
project (decklink)

set(SOURCES
		consumer/decklink_consumer.cpp

		producer/decklink_producer.cpp

		decklink.cpp
		StdAfx.cpp
)
set(HEADERS
		consumer/decklink_consumer.h

		producer/decklink_producer.h

		util/util.h

		decklink.h
		decklink_api.h
		StdAfx.h
)
if (MSVC)
	set(OS_SPECIFIC_SOURCES
			StdAfx.c
			interop/DeckLinkAPI_i.c
			interop/DeckLinkAPI.h
			interop/DeckLinkAPIVersion.h
	)
else()
	set(OS_SPECIFIC_SOURCES
			linux_interop/DeckLinkAPIConfiguration.h
			linux_interop/DeckLinkAPIDeckControl.h
			linux_interop/DeckLinkAPIDispatch.cpp
			linux_interop/DeckLinkAPIModes.h
			linux_interop/DeckLinkAPI_v10_2.h
			linux_interop/DeckLinkAPI_v7_3.h
			linux_interop/DeckLinkAPI_v7_9.h
			linux_interop/DeckLinkAPI_v8_1.h
			linux_interop/DeckLinkAPI_v9_9.h
			linux_interop/LinuxCOM.h
			linux_interop/DeckLinkAPIConfiguration_v10_2.h
			linux_interop/DeckLinkAPIDiscovery.h
			linux_interop/DeckLinkAPI.h
			linux_interop/DeckLinkAPITypes.h
			linux_interop/DeckLinkAPI_v7_1.h
			linux_interop/DeckLinkAPI_v7_6.h
			linux_interop/DeckLinkAPI_v8_0.h
			linux_interop/DeckLinkAPI_v9_2.h
			linux_interop/DeckLinkAPIVersion.h
	)
endif ()

add_library(decklink ${SOURCES} ${HEADERS} ${OS_SPECIFIC_SOURCES})
add_precompiled_header(decklink StdAfx.h FORCEINCLUDE)
configure_file("${PROJECT_SOURCE_DIR}/packages.config" "${CMAKE_CURRENT_BINARY_DIR}/packages.config")

include_directories(..)
include_directories(../..)
include_directories(${BOOST_INCLUDE_PATH})
include_directories(${TBB_INCLUDE_PATH})
include_directories(${FFMPEG_INCLUDE_PATH})

set_target_properties(decklink PROPERTIES FOLDER modules)
source_group(sources ./*)
source_group(sources\\consumer consumer/*)
source_group(sources\\interop interop/*)
source_group(sources\\producer producer/*)
source_group(sources\\util util/*)

if (MSVC)
	target_link_libraries(decklink
			common
			core
			ffmpeg
	)
else()
	target_link_libraries(decklink
			common
			core
			ffmpeg

			dl
	)
endif ()

casparcg_add_include_statement("modules/decklink/decklink.h")
casparcg_add_init_statement("decklink::init" "decklink")
casparcg_add_module_project("decklink")
