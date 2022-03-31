cmake_minimum_required (VERSION 2.6)
project (decklink)

set(SOURCES
		consumer/decklink_consumer.cpp

		producer/decklink_producer.cpp

		decklink.cpp

		interop/DeckLinkAPIDispatch.cpp
)
set(HEADERS
		consumer/decklink_consumer.h

		producer/decklink_producer.h

		util/util.h

		decklink.h
		decklink_api.h
		
		interop/DeckLinkAPIConfiguration.h
		interop/DeckLinkAPIDeckControl.h
		interop/DeckLinkAPIModes.h
		interop/DeckLinkAPI_v10_2.h
		interop/DeckLinkAPI_v7_3.h
		interop/DeckLinkAPI_v7_9.h
		interop/DeckLinkAPI_v8_1.h
		interop/DeckLinkAPI_v9_9.h
		interop/LinuxCOM.h
		interop/DeckLinkAPIConfiguration_v10_2.h
		interop/DeckLinkAPIDiscovery.h
		interop/DeckLinkAPI.h
		interop/DeckLinkAPITypes.h
		interop/DeckLinkAPI_v7_1.h
		interop/DeckLinkAPI_v7_6.h
		interop/DeckLinkAPI_v8_0.h
		interop/DeckLinkAPI_v9_2.h
		interop/DeckLinkAPIVersion.h
)

add_library(decklink ${SOURCES} ${HEADERS})

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

target_link_libraries(decklink
common
core
ffmpeg

dl
)

casparcg_add_include_statement("modules/decklink/decklink.h")
casparcg_add_init_statement("decklink::init" "decklink")
casparcg_add_module_project("decklink")
