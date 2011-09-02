#pragma once

#include <vector>

#include <stdint.h>

#include <boost/range/iterator_range.hpp>

namespace caspar { namespace core {

static std::vector<int16_t> audio_32_to_16(const boost::iterator_range<int32_t*>& input)
{	
	std::vector<int16_t> audio16(input.size());
	auto audio32_ptr = reinterpret_cast<const uint32_t*>(input.begin());
	auto audio16_ptr = reinterpret_cast<uint32_t*>(audio16.data());
	auto size		 = input.size()/2;
	for(int n = 0; n < size; ++n)		
		audio16_ptr[n] = (audio32_ptr[n*2+1] & 0xffff0000) | (audio32_ptr[n*2+0] >> 16);	
	return audio16;
}

}}