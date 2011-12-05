/*
* Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
*
* This file is part of CasparCG (www.casparcg.com).
*
* CasparCG is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* CasparCG is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
*
* Author: Robert Nagy, ronag89@gmail.com
*/

#pragma once

#include "../utility/assert.h"
#include "../memory/safe_ptr.h"

#include <assert.h>

#include <tbb/parallel_for.h>

namespace caspar {

namespace detail {

static void* fast_memcpy_aligned_impl(void* dest, const void* source, size_t count)
{
	CASPAR_ASSERT(dest != nullptr);
	CASPAR_ASSERT(source != nullptr);
	CASPAR_ASSERT(reinterpret_cast<int>(dest) % 16 == 0);
	CASPAR_ASSERT(reinterpret_cast<int>(source) % 16 == 0);

	if(count == 0)
		return dest;

	__asm   
	{      
		mov esi, source;          
		mov edi, dest;    
		mov ebx, count;     
		shr ebx, 7;

		cpy:             
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

static void* fast_memcpy_unaligned_impl(void* dest, const void* source, size_t count)
{
	CASPAR_ASSERT(dest != nullptr);
	CASPAR_ASSERT(source != nullptr);

	if(count == 0)
		return dest;

	__asm   
	{      
		mov esi, source;          
		mov edi, dest;    
		mov ebx, count;     
		shr ebx, 7;

		cpy:             
			movdqu xmm0, [esi+00h];       
			movdqu xmm1, [esi+10h];      
			movdqu xmm2, [esi+20h];         
			movdqu xmm3, [esi+30h];   

			movdqu [edi+00h], xmm0;
			movdqu [edi+10h], xmm1;
			movdqu [edi+20h], xmm2;    
			movdqu [edi+30h], xmm3;

			movdqu xmm4, [esi+40h];
			movdqu xmm5, [esi+50h];
			movdqu xmm6, [esi+60h];
			movdqu xmm7, [esi+70h];  

			movdqu [edi+40h], xmm4; 
			movdqu [edi+50h], xmm5;      
			movdqu [edi+60h], xmm6;    
			movdqu [edi+70h], xmm7;    

			lea edi, [edi+80h];       
			lea esi, [esi+80h];      

			dec ebx;      
		jnz cpy;  
	}   
	return dest;
}

static void* fast_memcpy_small_aligned(char* dest8, const char* source8, size_t count)
{   
	size_t rest = count & 127;
	count &= ~127;

	fast_memcpy_aligned_impl(dest8, source8, count);   

	return memcpy(dest8+count,  source8+count, rest);
}

static void* fast_memcpy_small_unaligned(char* dest8, const char* source8, size_t count)
{   
	size_t rest = count & 127;
	count &= ~127;

	fast_memcpy_unaligned_impl(dest8, source8, count);   

	return memcpy(dest8+count,  source8+count, rest);
}

static void* fast_memcpy_aligned(void* dest, const void* source, size_t count)
{   
	auto dest8			= reinterpret_cast<char*>(dest);
	auto source8		= reinterpret_cast<const char*>(source);
		
	size_t rest = count & 2047;
	count &= ~2047;
		
	tbb::affinity_partitioner ap;
	tbb::parallel_for(tbb::blocked_range<size_t>(0, count/128), [&](const tbb::blocked_range<size_t>& r)
	{       
		fast_memcpy_aligned_impl(reinterpret_cast<char*>(dest) + r.begin()*128, reinterpret_cast<const char*>(source) + r.begin()*128, r.size()*128);   
	}, ap);
	
	return fast_memcpy_small_aligned(dest8+count, source8+count, rest);
}

static void* fast_memcpy_unaligned(void* dest, const void* source, size_t count)
{   
	auto dest8			= reinterpret_cast<char*>(dest);
	auto source8		= reinterpret_cast<const char*>(source);
		
	size_t rest = count & 2047;
	count &= ~2047;
		
	tbb::affinity_partitioner ap;
	tbb::parallel_for(tbb::blocked_range<size_t>(0, count/128), [&](const tbb::blocked_range<size_t>& r)
	{       
		fast_memcpy_unaligned_impl(reinterpret_cast<char*>(dest) + r.begin()*128, reinterpret_cast<const char*>(source) + r.begin()*128, r.size()*128);   
	}, ap);
	
	return fast_memcpy_small_unaligned(dest8+count, source8+count, rest);
}

}

template<typename T>
T* fast_memcpy(T* dest, const void* source, size_t count)
{   
	if((reinterpret_cast<int>(source) & 15) || (reinterpret_cast<int>(dest) & 15))
		return reinterpret_cast<T*>(detail::fast_memcpy_unaligned(dest, source, count));
	else
		return reinterpret_cast<T*>(detail::fast_memcpy_aligned(dest, source, count));
}

}
