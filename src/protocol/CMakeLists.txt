cmake_minimum_required (VERSION 3.16)
project (protocol)

set(SOURCES
)

set(HEADERS
		amcp/amcp_shared.h

		StdAfx.h
)

casparcg_add_library(protocol SOURCES ${SOURCES} ${HEADERS})
target_include_directories(protocol  PRIVATE
    ..
    )
target_precompile_headers(protocol PRIVATE "StdAfx.h")

source_group(sources\\amcp amcp/*)
source_group(sources\\cii cii/*)
source_group(sources\\clk clk/*)
source_group(sources\\log log/*)
source_group(sources\\util util/*)
source_group(sources ./*)

target_link_libraries(protocol common core)
