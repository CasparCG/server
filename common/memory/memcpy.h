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

#include <assert.h>

#include <tbb/parallel_for.h>

namespace caspar {

namespace internal {

static void* fast_memcpy(void* dest, const void* source, size_t count)
{
	assert(count % (16*8) == 0);
	assert(dest != nullptr);
	assert(source != nullptr);

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

static void* fast_memcpy(void* dest, const void* source, size_t num)
{   
	tbb::affinity_partitioner partitioner;
	tbb::parallel_for(tbb::blocked_range<size_t>(0, num/128), [&](const tbb::blocked_range<size_t>& r)
	{       
		internal::fast_memcpy(reinterpret_cast<char*>(dest) + r.begin()*128, reinterpret_cast<const char*>(source) + r.begin()*128, r.size()*128);   
	}, partitioner);   
	return dest;
}


}