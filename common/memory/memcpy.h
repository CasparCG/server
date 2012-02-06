#pragma once

#include <intrin.h>
#include <stdint.h>

#include <array>

#include <tbb/parallel_for.h>

#include <asmlib.h>

#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4244)
#endif
extern "C" 
{
	#include <libavutil/cpu.h>
}
#if defined(_MSC_VER)
#pragma warning (pop)
#endif

namespace caspar {

static bool has_uswc_memcpy()
{
	static bool value = (ff_get_cpu_flags_x86() & AV_CPU_FLAG_SSE4) != 0;
	return value;
}
	  
// http://software.intel.com/en-us/articles/copying-accelerated-video-decode-frame-buffers/
static void uswc_memcpy(void* dest, void* source, int count)  
{  
	static const int CACHED_BUFFER_SIZE = 4096;
  
	const int block_count	= count / CACHED_BUFFER_SIZE;
	const int remainder		= count % CACHED_BUFFER_SIZE;
    
	tbb::parallel_for(tbb::blocked_range<int>(0, block_count), [&](const tbb::blocked_range<int>& r)
	{
		__declspec(align(64)) std::array<uint8_t, CACHED_BUFFER_SIZE> cache_block;

		auto load  = reinterpret_cast<__m128i*>(source)+r.begin()*CACHED_BUFFER_SIZE/16;
		auto store = reinterpret_cast<__m128i*>(dest)+r.begin()*CACHED_BUFFER_SIZE/16;

		for(int b = r.begin(); b != r.end(); ++b)
		{
			{
				_mm_mfence();   
				auto cache = reinterpret_cast<__m128i*>(cache_block.data());     

				for(int n = 0; n < CACHED_BUFFER_SIZE; n += 64)
				{
					auto x0 = _mm_stream_load_si128(load++);  
					auto x1 = _mm_stream_load_si128(load++);  
					auto x2 = _mm_stream_load_si128(load++);  
					auto x3 = _mm_stream_load_si128(load++);  
  
					_mm_store_si128(cache++, x0);  
					_mm_store_si128(cache++, x1);  
					_mm_store_si128(cache++, x2);  
					_mm_store_si128(cache++, x3);  
				}
			}
			{
				_mm_mfence();    
				auto cache = reinterpret_cast<__m128i*>(cache_block.data());  
		          
				for(int n = 0; n < CACHED_BUFFER_SIZE; n += 64)
				{  
					auto x0 = _mm_load_si128(cache++);  
					auto x1 = _mm_load_si128(cache++);  
					auto x2 = _mm_load_si128(cache++);  
					auto x3 = _mm_load_si128(cache++);  
  
					_mm_stream_si128(store++, x0);  
					_mm_stream_si128(store++, x1);  
					_mm_stream_si128(store++, x2);  
					_mm_stream_si128(store++, x3);   
				}  
			}
		}
	});
	
	__declspec(align(64)) std::array<uint8_t, CACHED_BUFFER_SIZE> cache_block;
	
	{
		_mm_mfence(); 
		auto cache = reinterpret_cast<__m128i*>(cache_block.data());   
		auto load  = reinterpret_cast<__m128i*>(source)+(count-remainder)/16;

		for(int n = 0; n < remainder; n += 16)
			_mm_store_si128(cache++, _mm_stream_load_si128(load++));  		
	}

	{	
		_mm_mfence(); 
		auto cache = reinterpret_cast<__m128i*>(cache_block.data());   
		auto store = reinterpret_cast<__m128i*>(dest)+(count-remainder)/16;

		for(int n = 0; n < remainder; n += 16)
			_mm_stream_si128(store++, _mm_load_si128(cache++));  		
	}
}  

}