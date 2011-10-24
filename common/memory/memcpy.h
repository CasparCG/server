/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/
#pragma once

#include "../utility/assert.h"

#include <assert.h>

#include <ppl.h>

namespace caspar {

namespace internal {

static void* fast_memcpy(void* dest, const void* source, size_t count)
{
	CASPAR_ASSERT(dest != nullptr);
	CASPAR_ASSERT(source != nullptr);
	CASPAR_ASSERT(reinterpret_cast<int>(dest) % 16 == 0);
	CASPAR_ASSERT(reinterpret_cast<int>(source) % 16 == 0);

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

}

static void* fast_memcpy(void* dest, const void* source, size_t count)
{   
	if(reinterpret_cast<int>(source) % 16 != 0)
		return memcpy(reinterpret_cast<char*>(dest),  reinterpret_cast<const char*>(source), count);

	size_t rest = count % 2048;
	count -= rest;

	Concurrency::parallel_for<int>(0, count / 2048, [&](size_t n)
	{       
		internal::fast_memcpy(reinterpret_cast<char*>(dest) + n*2048, reinterpret_cast<const char*>(source) + n*2048, 2048);   
	});

	return memcpy(reinterpret_cast<char*>(dest)+count,  reinterpret_cast<const char*>(source)+count, rest);
}

static void* fast_memcpy_small(void* dest, const void* source, size_t count)
{   
	if(reinterpret_cast<int>(source) % 16 != 0)
		return memcpy(reinterpret_cast<char*>(dest),  reinterpret_cast<const char*>(source), count);

	size_t rest = count % 128;
	count -= rest;

	internal::fast_memcpy(reinterpret_cast<char*>(dest), reinterpret_cast<const char*>(source), count);   
	return memcpy(reinterpret_cast<char*>(dest)+count,  reinterpret_cast<const char*>(source)+count, rest);
}

static void* fast_memcpy_w_align_hack(void* dest, const void* source, size_t count)
{   	
	auto dest8			= reinterpret_cast<char*>(dest);
	auto source8		= reinterpret_cast<const char*>(source);
	
	auto source_align	= reinterpret_cast<int>(source) % 16;
		
	source8 -= source_align;	

	size_t rest = count % 512;
	count -= rest;

	Concurrency::parallel_for<int>(0, count / 512, [&](size_t n)
	{       
		internal::fast_memcpy(dest8 + n*512, source8 + n*512, 512);   
	});

	memcpy(dest8+count, source8+count, rest);

	return dest8+source_align;
}


}