#pragma once

#include <vector>

#include <stdint.h>

#include <tbb/cache_aligned_allocator.h>

namespace caspar { namespace core {

// NOTE: Input data pointer should be larger than input.size() to allow sse to read beyond
template<typename T>
static std::vector<int16_t, tbb::cache_aligned_allocator<int16_t>> audio_32_to_16_sse(const T& audio_data)
{	
	auto size		 = std::distance(std::begin(audio_data), std::end(audio_data));
	auto input32	 = &(*std::begin(audio_data));
	auto output16	 = std::vector<int16_t, tbb::cache_aligned_allocator<int16_t>>(size);

	auto input128	 = reinterpret_cast<const __m128i*>(input32);
	auto output128	 = reinterpret_cast<__m128i*>(output16.data());

	for(int n = 0; n < size/8; ++n)		
	{
		auto xmm0 = _mm_srai_epi32(_mm_load_si128(input128++), 16);
		auto xmm1 = _mm_srai_epi32(_mm_load_si128(input128++), 16);
		auto xmm3 = _mm_packs_epi32(xmm0, xmm1);
		_mm_store_si128(output128++, xmm3);
	}

	for(int n = size/8; n < size; ++n)
		output16[n] = input32[n] >> 16;

	return output16;
}

template<typename T>
static std::vector<int8_t, tbb::cache_aligned_allocator<int8_t>> audio_32_to_24(const T& audio_data)
{	
	auto size		 = std::distance(std::begin(audio_data), std::end(audio_data));
	auto input8		 = reinterpret_cast<const int8_t*>(&(*std::begin(audio_data)));
	auto output8	 = std::vector<int8_t, tbb::cache_aligned_allocator<int8_t>>();
			
	output8.reserve(size*3);
	for(int n = 0; n < size; ++n)
	{
		output8.push_back(input8[n*4+1]);
		output8.push_back(input8[n*4+2]);
		output8.push_back(input8[n*4+3]);
	}

	return output8;
}

template<typename T>
static std::vector<int8_t, tbb::cache_aligned_allocator<int8_t>> audio_32_to_16(const T& audio_data)
{	
	auto size		 = std::distance(std::begin(audio_data), std::end(audio_data));
	auto input8		 = reinterpret_cast<const int8_t*>(&(*std::begin(audio_data)));
	auto output8	 = std::vector<int8_t, tbb::cache_aligned_allocator<int8_t>>();
			
	output8.reserve(size*2);
	for(int n = 0; n < size; ++n)
	{
		output8.push_back(input8[n*4+2]);
		output8.push_back(input8[n*4+3]);
	}

	return output8;
}

}}