

protocol_lib = static_library(
    'protocol',
    'amcp/AMCPCommandQueue.cpp',
    'amcp/AMCPCommandsImpl.cpp',
    'amcp/AMCPProtocolStrategy.cpp',
    'amcp/amcp_command_repository.cpp',

    'cii/CIICommandsImpl.cpp',
    'cii/CIIProtocolStrategy.cpp',

    'clk/CLKProtocolStrategy.cpp',
    'clk/clk_commands.cpp',
    'clk/clk_command_processor.cpp',

    'osc/oscpack/OscOutboundPacketStream.cpp',
    'osc/oscpack/OscPrintReceivedElements.cpp',
    'osc/oscpack/OscReceivedElements.cpp',
    'osc/oscpack/OscTypes.cpp',

    'osc/client.cpp',

    'util/AsyncEventServer.cpp',
    'util/lock_container.cpp',
    'util/strategy_adapters.cpp',
    'util/http_request.cpp',
    'util/tokenize.cpp',

    include_directories: include_root,
    dependencies: [boost_dep, tbb_dep],
    link_with: [core_lib, common_lib],
    cpp_pch: ['pch/StdAfx.h']
)
