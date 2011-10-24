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
#include "../memory/safe_ptr.h"

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

static void* fast_memcpy_small(void* dest, const void* source, size_t count)
{   
	size_t rest = count & 127;
	count &= ~127;

	internal::fast_memcpy(reinterpret_cast<char*>(dest), reinterpret_cast<const char*>(source), count);   
	return memcpy(reinterpret_cast<char*>(dest)+count,  reinterpret_cast<const char*>(source)+count, rest);
}

}

static void* fast_memcpy(void* dest, const void* source, size_t count)
{   
	if((reinterpret_cast<int>(source) & 15) || (reinterpret_cast<int>(dest) & 15))
		return memcpy(reinterpret_cast<char*>(dest),  reinterpret_cast<const char*>(source), count);
	
	size_t rest = count & 511;
	count &= ~511;

	Concurrency::parallel_for<int>(0, count / 512, [&](size_t n)
	{       
		internal::fast_memcpy(reinterpret_cast<char*>(dest) + n*512, reinterpret_cast<const char*>(source) + n*512, 512);   
	});

	return internal::fast_memcpy_small(reinterpret_cast<char*>(dest)+count,  reinterpret_cast<const char*>(source)+count, rest);
}

template<typename T>
static safe_ptr<T> fast_memdup(const T* source, size_t count)
{   	
	auto dest			= reinterpret_cast<T*>(scalable_aligned_malloc(count + 16, 32));
	auto dest8			= reinterpret_cast<char*>(dest);
	auto source8		= reinterpret_cast<const char*>(source);	
	auto source_align	= reinterpret_cast<int>(source) & 15;
		
	try
	{
		fast_memcpy(dest8, source8-source_align, count+source_align);
	}
	catch(...)
	{
		scalable_free(dest);
		throw;
	}

	return safe_ptr<T>(reinterpret_cast<T*>(dest8+source_align), [dest](T*){scalable_free(dest);});
}


}