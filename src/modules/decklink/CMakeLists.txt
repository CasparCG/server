cmake_minimum_required (VERSION 3.28)
project (decklink)

set(SOURCES
		consumer/decklink_consumer.cpp
		consumer/decklink_consumer.h
		consumer/v210_strategies.cpp
		consumer/sdr_bgra_strategy.cpp
		consumer/format_strategy.h
		consumer/config.cpp
		consumer/config.h
		consumer/monitor.cpp
		consumer/monitor.h
		consumer/vanc.cpp
		consumer/vanc.h
		consumer/vanc_scte104_strategy.cpp
		consumer/vanc_op47_strategy.cpp

		producer/decklink_producer.cpp
		producer/decklink_producer.h

		util/util.h

		decklink.cpp
		decklink.h

		decklink_api.h
		StdAfx.h
)

if (MSVC)
	list(APPEND SOURCES
			interop/DeckLinkAPI_i.c
			interop/DeckLinkAPI.h
			interop/DeckLinkAPIVersion.h
	)
else()
	list(APPEND SOURCES
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

casparcg_add_module_project(decklink
	SOURCES ${SOURCES}
	INIT_FUNCTION "decklink::init"
)
target_include_directories(decklink PRIVATE
    ..
    ${FFMPEG_INCLUDE_PATH}
    )
target_precompile_headers(decklink PRIVATE "StdAfx.h")

set_target_properties(decklink PROPERTIES FOLDER modules)
source_group(sources ./*)
source_group(sources\\consumer consumer/*)
source_group(sources\\interop interop/*)
source_group(sources\\producer producer/*)
source_group(sources\\util util/*)

if (MSVC)
	target_link_libraries(decklink PRIVATE
			ffmpeg
	)
else()
	target_link_libraries(decklink PRIVATE
			ffmpeg

			dl
	)
endif ()

