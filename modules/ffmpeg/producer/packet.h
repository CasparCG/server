#pragma once

#include <vector>

#include <memory>

namespace caspar {
	
enum packet_type
{
	data_packet,
	flush_packet,
	empty_packet
};

struct packet
{
	packet_type type;
	std::shared_ptr<AVPacket> av_packet;
	
	packet(packet_type t = empty_packet) 
		: type(t){}

	packet(const std::shared_ptr<AVPacket>& av_packet) 
		: type(data_packet)
		, av_packet(av_packet){}
};

}