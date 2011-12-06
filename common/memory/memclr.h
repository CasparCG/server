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

#include <assert.h>

#include <cstring>

namespace caspar {

static void* fast_memclr(void* dest, size_t count)
{
	if(count < 2048)
		return memset(dest, 0, count);

	assert(dest != nullptr);
	
	size_t rest = count % 128;
	count -= rest;

	__asm   
	{              
		mov edi, dest;    
		mov ebx, count;     
		shr ebx, 7;
		pxor xmm0, xmm0; 

		clr:             
			movntdq [edi+00h], xmm0;
			movntdq [edi+10h], xmm0;
			movntdq [edi+20h], xmm0;    
			movntdq [edi+30h], xmm0;
			
			movntdq [edi+40h], xmm0; 
			movntdq [edi+50h], xmm0;      
			movntdq [edi+60h], xmm0;    
			movntdq [edi+70h], xmm0;    

			lea edi, [edi+80h];         

			dec ebx;      
		jnz clr;  
	}   
	return memset(reinterpret_cast<char*>(dest)+count, 0, rest);
}

}