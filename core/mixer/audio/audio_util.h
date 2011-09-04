#pragma once

#include <vector>

#include <stdint.h>

#include <boost/range/iterator_range.hpp>

#include <tbb/cache_aligned_allocator.h>

namespace caspar { namespace core {

// NOTE: Input data pointer should be larger than input.size() to allow sse to read beyond
static std::vector<int16_t, tbb::cache_aligned_allocator<int16_t>> audio_32_to_16_sse(const boost::iterator_range<int32_t*>& input)
{	
	std::vector<int16_t, tbb::cache_aligned_allocator<int16_t>> audio16(input.size());
	auto audio32_ptr = reinterpret_cast<const __m128i*>(input.begin());
	auto audio16_ptr = reinterpret_cast<__m128i*>(audio16.data());
	auto size		 = input.size();
	for(int n = 0; n < size/8; ++n)		
	{
		auto xmm0 = _mm_srai_epi32(_mm_load_si128(audio32_ptr++), 16);
		auto xmm1 = _mm_srai_epi32(_mm_load_si128(audio32_ptr++), 16);
		auto xmm3 = _mm_packs_epi32(xmm0, xmm1);
		_mm_store_si128(audio16_ptr++, xmm3);
	}
	return audio16;
}

static std::vector<int8_t> audio_32_to_24(const boost::iterator_range<int32_t*>& input)
{	
	std::vector<int8_t> audio24(input.size()*3+16);
	auto audio32_ptr = reinterpret_cast<const uint32_t*>(input.begin());
	auto audio24_ptr = reinterpret_cast<uint8_t*>(audio24.data());
	auto size		 = input.size();
	for(int n = 0; n < size; ++n)		
		*reinterpret_cast<uint32_t*>(audio24_ptr + n*3) = *(audio32_ptr + n) >> 8;	
	audio24.resize(input.size());
	return audio24;
}

}}