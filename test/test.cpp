#define NOMINMAX

#include <functional>
#include "timer.h"

#include <iostream>
#include <limits>
#include <numeric>

#include <tbb/scalable_allocator.h>
#include <tbb/parallel_for.h>
#include <intrin.h>
	
const unsigned int TEST_COUNT = 1000;
const unsigned int TEST_SIZE = 1920*1080*4;

void test(const std::function<void*(void*, const void*, size_t)>& func)
{
	size_t size = TEST_SIZE;
	void* src = scalable_aligned_malloc(size, 16);
	void* dest = scalable_aligned_malloc(size, 16);

	timer timer;
	double total_time = 0.0;
	for(int i = 0 ;i < TEST_COUNT; ++i)	
	{
		timer.start();
		func(dest, src, size);
		total_time += timer.time();
		if(memcmp(dest, src, size) != 0)
			std::cout << "ERROR";
		memset(src, rand(), size); // flush
	}

	scalable_aligned_free(dest);
	scalable_aligned_free(src);

	double unit_time = total_time/static_cast<double>(TEST_COUNT);
	std::cout << 1.0/unit_time*static_cast<double>(TEST_SIZE)/1000000000.0 << " gb/s";
}

void* memcpy_SSE2_1(void* dest, const void* source, size_t size)
{	
	__m128i* dest_128 = reinterpret_cast<__m128i*>(dest);
	const __m128i* source_128 = reinterpret_cast<const __m128i*>(source);

	for(size_t n = 0; n < size/16; n += 8) 
	{
		_mm_prefetch(reinterpret_cast<const char*>(source_128+8), _MM_HINT_NTA);
		_mm_prefetch(reinterpret_cast<const char*>(source_128+12), _MM_HINT_NTA);

		__m128i xmm0 = _mm_load_si128(source_128++);
		__m128i xmm1 = _mm_load_si128(source_128++);
		__m128i xmm2 = _mm_load_si128(source_128++);
		__m128i xmm3 = _mm_load_si128(source_128++);
		
		_mm_stream_si128(dest_128++, xmm0);
		_mm_stream_si128(dest_128++, xmm1);
		_mm_stream_si128(dest_128++, xmm2);
		_mm_stream_si128(dest_128++, xmm3);
		
		__m128i xmm4 = _mm_load_si128(source_128++);
		__m128i xmm5 = _mm_load_si128(source_128++);
		__m128i xmm6 = _mm_load_si128(source_128++);
		__m128i xmm7 = _mm_load_si128(source_128++);

		_mm_stream_si128(dest_128++, xmm4);
		_mm_stream_si128(dest_128++, xmm5);
		_mm_stream_si128(dest_128++, xmm6);
		_mm_stream_si128(dest_128++, xmm7);
	}
	return dest;
}

void* memcpy_SSE2_2(void* dest, const void* source, size_t num)
{	
	__asm
	{
		mov esi, source;    
		mov edi, dest;   
 
		mov ebx, num; 
		shr ebx, 7;      
  
		cpy:
			prefetchnta [esi+80h];
			prefetchnta [esi+0A0h];
			prefetchnta [esi+0C0h];
			prefetchnta [esi+0E0h];
 
			movdqa xmm0, [esi+00h];
			movdqa xmm1, [esi+10h];
			movdqa xmm2, [esi+20h];
			movdqa xmm3, [esi+30h];
	  
			movntdq [edi+00h], xmm0;
			movntdq [edi+10h], xmm1;
			movntdq [edi+20h], xmm2;
			movntdq [edi+30h], xmm3;
			
			movdqa xmm4, [esi+40h];
			movdqa xmm5, [esi+50h];
			movdqa xmm6, [esi+60h];
			movdqa xmm7, [esi+70h];
	  
			movntdq [edi+40h], xmm4;
			movntdq [edi+50h], xmm5;
			movntdq [edi+60h], xmm6;
			movntdq [edi+70h], xmm7;
 
			lea edi,[edi+80h];
			lea esi,[esi+80h];
			dec ebx;
 
		jnz cpy;
	}
	return dest;
}

void* memcpy_SSE2_3(void* dest, const void* source, size_t num)
{	
	__asm
	{
		mov esi, source;    
		mov edi, dest;   
 
		mov ebx, num; 
		shr ebx, 7;      
  
		cpy:
			prefetchnta [esi+80h];
			prefetchnta [esi+0C0h];
 
			movdqa xmm0, [esi+00h];
			movdqa xmm1, [esi+10h];
			movdqa xmm2, [esi+20h];
			movdqa xmm3, [esi+30h];
	  
			movntdq [edi+00h], xmm0;
			movntdq [edi+10h], xmm1;
			movntdq [edi+20h], xmm2;
			movntdq [edi+30h], xmm3;
			
			movdqa xmm4, [esi+40h];
			movdqa xmm5, [esi+50h];
			movdqa xmm6, [esi+60h];
			movdqa xmm7, [esi+70h];
	  
			movntdq [edi+40h], xmm4;
			movntdq [edi+50h], xmm5;
			movntdq [edi+60h], xmm6;
			movntdq [edi+70h], xmm7;
 
			lea edi, [edi+80h];
			lea esi, [esi+80h];
			dec ebx; 
		jnz cpy;
	}
	return dest;
}

void* memcpy_SSE2_3_tbb(void* dest, const void* source, size_t num)
{	
	tbb::parallel_for(tbb::blocked_range<size_t>(0, num/128), [&](const tbb::blocked_range<size_t>& r)
	{
		memcpy_SSE2_3(reinterpret_cast<char*>(dest) + r.begin()*128, reinterpret_cast<const char*>(source) + r.begin()*128, r.size()*128);
	}, tbb::affinity_partitioner());

	return dest;
}

void* X_aligned_memcpy_sse2(void* dest, const void* src, size_t size_t)
{ 
  __asm
  {
    mov esi, src;    //src pointer
    mov edi, dest;   //dest pointer
 
    mov ebx, size_t; //ebx is our counter 
    shr ebx, 7;      //divide by 128 (8 * 128bit registers)
 
 
    loop_copy:
      prefetchnta 128[ESI]; //SSE2 prefetch
      prefetchnta 160[ESI];
      prefetchnta 192[ESI];
      prefetchnta 224[ESI];
 
      movdqa xmm0, 0[ESI]; //move data from src to registers
      movdqa xmm1, 16[ESI];
      movdqa xmm2, 32[ESI];
      movdqa xmm3, 48[ESI];
      movdqa xmm4, 64[ESI];
      movdqa xmm5, 80[ESI];
      movdqa xmm6, 96[ESI];
      movdqa xmm7, 112[ESI];
 
      movntdq 0[EDI], xmm0; //move data from registers to dest
      movntdq 16[EDI], xmm1;
      movntdq 32[EDI], xmm2;
      movntdq 48[EDI], xmm3;
      movntdq 64[EDI], xmm4;
      movntdq 80[EDI], xmm5;
      movntdq 96[EDI], xmm6;
      movntdq 112[EDI], xmm7;
 
      add esi, 128;
      add edi, 128;
      dec ebx;
 
      jnz loop_copy; //loop please
  }
  return dest;
}

int main(int argc, wchar_t* argv[])
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	test(memcpy);
	std::cout << " memcpy" << std::endl;
	test(memcpy_SSE2_1);
	std::cout << " memcpy_SSE2_1" << std::endl;
	test(memcpy_SSE2_2);
	std::cout << " memcpy_SSE2_2" << std::endl;
	test(memcpy_SSE2_3);
	std::cout << " memcpy_SSE2_3" << std::endl;
	test(memcpy_SSE2_3_tbb);
	std::cout << " memcpy_SSE2_3_tbb" << std::endl;
	test(X_aligned_memcpy_sse2);
	std::cout << " X_aligned_memcpy_sse2" << std::endl;

	std::cout << "Press ENTER to continue... " << std::endl;
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	return 0;
}

