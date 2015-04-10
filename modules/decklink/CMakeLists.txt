cmake_minimum_required (VERSION 2.6)
project (decklink)

set(SOURCES
		consumer/decklink_consumer.cpp

		interop/DeckLinkAPI_i.c

		producer/decklink_producer.cpp

		decklink.cpp
		StdAfx.c
		StdAfx.cpp
)
set(HEADERS
		consumer/decklink_consumer.h

		interop/DeckLinkAPI_h.h
		interop/DeckLinkAPIVersion.h

		producer/decklink_producer.h

		util/util.h

		decklink.h
		StdAfx.h
)

add_library(decklink ${SOURCES} ${HEADERS})
add_precompiled_header(decklink StdAfx.h FORCEINCLUDE)

include_directories(..)
include_directories(../..)
include_directories(${BOOST_INCLUDE_PATH})
include_directories(${TBB_INCLUDE_PATH})
include_directories(${FFMPEG_INCLUDE_PATH})
include_directories(${RXCPP_INCLUDE_PATH})

set_target_properties(decklink PROPERTIES FOLDER modules)
source_group(sources ./*)
source_group(sources\\consumer consumer/*)
source_group(sources\\interop interop/*)
source_group(sources\\producer producer/*)

target_link_libraries(decklink common core ffmpeg)

casparcg_add_include_statement("modules/decklink/decklink.h")
casparcg_add_init_statement("decklink::init" "decklink")
casparcg_add_module_project("decklink")
