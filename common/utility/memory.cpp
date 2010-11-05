#include "../stdafx.h"

#include "memory.h"

#include <intrin.h>

#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>

namespace caspar { namespace common {
	
void* memcpy_SSE2(void* dest, const void* source, size_t num)
{	
	__asm
	{
		mov esi, source;    
		mov edi, dest;   
 
		mov ebx, num; 
		shr ebx, 7;      
  
		cpy:
			prefetchnta [esi+80h];
 
			movdqa xmm0, [esi+00h];
			movdqa xmm1, [esi+10h];
			movdqa xmm2, [esi+20h];
			movdqa xmm3, [esi+30h];
	  
			movntdq [edi+00h], xmm0;
			movntdq [edi+10h], xmm1;
			movntdq [edi+20h], xmm2;
			movntdq [edi+30h], xmm3;
			
			prefetchnta [esi+0C0h];
			
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

void* aligned_memcpy(void* dest, const void* source, size_t num)
{	
	if(num < 128)
		return memcpy(dest, source, num);

	tbb::parallel_for(tbb::blocked_range<size_t>(0, num/128), [&](const tbb::blocked_range<size_t>& r)
	{
		memcpy_SSE2(reinterpret_cast<char*>(dest) + r.begin()*128, reinterpret_cast<const char*>(source) + r.begin()*128, r.size()*128);
	}, tbb::affinity_partitioner());

	return dest;
}

void* clear(void* dest, size_t size)
{
	tbb::parallel_for(tbb::blocked_range<size_t>(0, size/16), [&](const tbb::blocked_range<size_t>& r)
	{
		__m128i val = _mm_setzero_si128();
		__m128i* ptr = reinterpret_cast<__m128i*>(dest)+r.begin();
		__m128i* end = ptr + r.size();

		while(ptr != end)	
			_mm_stream_si128(ptr++, val);
	});	
	return dest;
}

}}