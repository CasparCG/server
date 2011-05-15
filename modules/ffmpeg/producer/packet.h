#pragma once

#include <vector>
#include <tbb/cache_aligned_allocator.h>

namespace caspar {
	
enum packet_type
{
	data_packet,
	flush_packet,
	empty_packet
};

struct packet
{
	typedef std::vector<unsigned char, tbb::cache_aligned_allocator<unsigned char>> aligned_buffer;

	packet_type type;
	std::shared_ptr<aligned_buffer> data;

	packet(packet_type t = empty_packet) 
		: type(t){}

	template<typename T>
	packet(T first, T last) 
		: type(data_packet)
		, data(new aligned_buffer(first, last)){}
};

}