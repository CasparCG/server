cmake_minimum_required (VERSION 3.16)
project (protocol)

set(SOURCES
		amcp/AMCPCommand.cpp
		amcp/AMCPCommandQueue.cpp
		amcp/AMCPCommandsImpl.cpp
		amcp/AMCPProtocolStrategy.cpp
		amcp/amcp_command_repository.cpp
		amcp/amcp_args.cpp
		amcp/amcp_command_repository_wrapper.cpp

		osc/oscpack/OscOutboundPacketStream.cpp
		osc/oscpack/OscPrintReceivedElements.cpp
		osc/oscpack/OscReceivedElements.cpp
		osc/oscpack/OscTypes.cpp

		osc/client.cpp

		util/AsyncEventServer.cpp
		util/lock_container.cpp
		util/strategy_adapters.cpp
		util/http_request.cpp
		util/tokenize.cpp
		util/websocket_server.cpp
		util/websocket_monitor_client.cpp
		util/websocket_monitor_server.cpp
)

set(HEADERS
		amcp/AMCPCommand.h
		amcp/AMCPCommandQueue.h
		amcp/AMCPCommandsImpl.h
		amcp/AMCPProtocolStrategy.h
		amcp/amcp_command_repository.h
		amcp/amcp_command_repository_wrapper.h
		amcp/amcp_shared.h
		amcp/amcp_args.h
		amcp/amcp_command_context.h

		osc/oscpack/MessageMappingOscPacketListener.h
		osc/oscpack/OscException.h
		osc/oscpack/OscHostEndianness.h
		osc/oscpack/OscOutboundPacketStream.h
		osc/oscpack/OscPacketListener.h
		osc/oscpack/OscPrintReceivedElements.h
		osc/oscpack/OscReceivedElements.h
		osc/oscpack/OscTypes.h

		osc/client.h

		util/AsyncEventServer.h
		util/ClientInfo.h
		util/lock_container.h
		util/ProtocolStrategy.h
		util/protocol_strategy.h
		util/strategy_adapters.h
		util/http_request.h
		util/tokenize.h
		util/websocket_server.h
		util/websocket_monitor_client.h
		util/websocket_monitor_server.h

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
source_group(sources\\osc\\oscpack osc/oscpack/*)
source_group(sources\\osc osc/*)
source_group(sources\\util util/*)
source_group(sources ./*)

target_link_libraries(protocol PRIVATE common core nlohmann_json::nlohmann_json)
