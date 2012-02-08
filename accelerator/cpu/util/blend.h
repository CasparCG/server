//
// Copyright (c) 2012 Robert Nagy
//
// Premultiplied Alpha Blending
//

#pragma once

#include "simd.h"

#include <tbb/parallel_for.h>

#include <intrin.h>
#include <stdint.h>

namespace caspar { namespace accelerator { namespace cpu {

/*
	Function: blend

	Description:
		
		Premultiplied Alpha Blending.
		Based on http://www.alvyray.com/Memos/CG/Microsoft/4_comp.pdf.
				
	Parameters:

		dest	 - (rgb)a destination image.
		source1	 - (rgb)a lower source image.
		source2	 - (rgb)a upper source image.
		count	 - Size in bytes of source1 and source2.			
*/
static void blend(uint8_t* dest, const uint8_t* source1, const uint8_t* source2, size_t count)
{
    const xmm_epi16 round  = 128;
    const xmm_epi16 lomask = 0x00FF;
		
	tbb::parallel_for(tbb::blocked_range<int>(0, count/sizeof(xmm_epi8)), [&](const tbb::blocked_range<int>& r)
	{
		for(auto n = r.begin(); n != r.end(); ++n)    
		{
			auto s = xmm_epi16::load(source1+n*16);
			auto d = xmm_epi16::load(source2+n*16);

			// T(S, D) = S * D[A] + 0x80
			auto xxxa	= xmm_cast<xmm_epi32>(d) >> 24;
			auto xaxa	= xmm_cast<xmm_epi16>((xxxa << 16) | xxxa);
			      
			auto xbxr	= s & lomask;
			auto t1		= xmm_epi16::multiply_low(xbxr, xaxa) + round;    
			
			auto xaxg	= s >> 8;
			auto t2		= xmm_epi16::multiply_low(xaxg, xaxa) + round;
		
			// C(S, D) = S + D - (((T >> 8) + T) >> 8);
			auto bxrx	= (t1 >> 8) + t1;      
			auto axgx	= (t2 >> 8) + t2;    
			auto bgra	= xmm_cast<xmm_epi8>((bxrx >> 8) | xmm_epi16::and_not(axgx, lomask));
				
			xmm_epi8::stream(xmm_cast<xmm_epi8>(s) + (xmm_cast<xmm_epi8>(d) - bgra), dest + n*16);
		}     
	});
}

}}}