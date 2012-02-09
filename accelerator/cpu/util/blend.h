//
// Copyright (c) 2012 Robert Nagy
//
// Premultiplied Alpha Blending
//

#pragma once

#include "simd.h"

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
	CASPAR_VERIFY(count % 16 == 0);

    const xmm_epi16 round  = 128;
    const xmm_epi16 lomask = 0x00FF;
		
	for(auto n = 0; n < count; n += 16)    
	{
		auto s = xmm_epi16::load(source1+n);
		auto d = xmm_epi8::load(source2+n);

		// T(S, D) = S * D[A] + 0x80
		auto aaaa   = xmm_epi8::shuffle(d, xmm_epi8(-1, 15, -1, 15, -1, 11, -1, 11, -1, 7, -1, 7, -1, 3, -1, 3));
		auto xaxa	= xmm_cast<xmm_epi16>(aaaa);
			      
		auto xrxb	= s & lomask;
		auto t1		= xmm_epi16::multiply_low(xrxb, xaxa) + round;    
			
		auto xaxg	= s >> 8;
		auto t2		= xmm_epi16::multiply_low(xaxg, xaxa) + round;
		
		// C(S, D) = S + D - (((T >> 8) + T) >> 8);
		auto rxbx	= (t1 >> 8) + t1;      
		auto axgx	= (t2 >> 8) + t2;    
		auto argb	= xmm_cast<xmm_epi8>((rxbx >> 8) | xmm_epi16::and_not(axgx, lomask));
				
		xmm_epi8::stream(xmm_cast<xmm_epi8>(s) + (d - argb), dest+n);
	} 
}

static void blend(uint8_t* dest, const uint8_t* source, size_t count)
{
	return blend(dest, dest, source, count);
}

}}}