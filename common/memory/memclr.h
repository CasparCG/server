#pragma once

#include <assert.h>

namespace caspar {

static void* fast_memclr(void* dest, size_t count)
{
	assert(count % (16*4) == 0);
	assert(dest != nullptr);
	assert(source != nullptr);

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
	return dest;
}

}