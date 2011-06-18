#pragma once

#include <core/video_format.h>
#include <core/producer/frame/pixel_format.h>

namespace caspar {
	
static core::pixel_format::type get_pixel_format(PixelFormat pix_fmt)
{
	switch(pix_fmt)
	{
	case PIX_FMT_GRAY8:		return core::pixel_format::gray;
	case PIX_FMT_BGRA:		return core::pixel_format::bgra;
	case PIX_FMT_ARGB:		return core::pixel_format::argb;
	case PIX_FMT_RGBA:		return core::pixel_format::rgba;
	case PIX_FMT_ABGR:		return core::pixel_format::abgr;
	case PIX_FMT_YUV444P:	return core::pixel_format::ycbcr;
	case PIX_FMT_YUV422P:	return core::pixel_format::ycbcr;
	case PIX_FMT_YUV420P:	return core::pixel_format::ycbcr;
	case PIX_FMT_YUV411P:	return core::pixel_format::ycbcr;
	case PIX_FMT_YUV410P:	return core::pixel_format::ycbcr;
	case PIX_FMT_YUVA420P:	return core::pixel_format::ycbcra;
	default:				return core::pixel_format::invalid;
	}
}

static core::pixel_format_desc get_pixel_format_desc(PixelFormat pix_fmt, size_t width, size_t height)
{
	// Get linesizes
	AVPicture dummy_pict;	
	avpicture_fill(&dummy_pict, nullptr, pix_fmt, width, height);

	core::pixel_format_desc desc;
	desc.pix_fmt = get_pixel_format(pix_fmt);
		
	switch(desc.pix_fmt)
	{
	case core::pixel_format::gray:
		{
			desc.planes.push_back(core::pixel_format_desc::plane(dummy_pict.linesize[0]/4, height, 1));						
			return desc;
		}
	case core::pixel_format::bgra:
	case core::pixel_format::argb:
	case core::pixel_format::rgba:
	case core::pixel_format::abgr:
		{
			desc.planes.push_back(core::pixel_format_desc::plane(dummy_pict.linesize[0]/4, height, 4));						
			return desc;
		}
	case core::pixel_format::ycbcr:
	case core::pixel_format::ycbcra:
		{		
			// Find chroma height
			size_t size2 = dummy_pict.data[2] - dummy_pict.data[1];
			size_t h2 = size2/dummy_pict.linesize[1];			

			desc.planes.push_back(core::pixel_format_desc::plane(dummy_pict.linesize[0], height, 1));
			desc.planes.push_back(core::pixel_format_desc::plane(dummy_pict.linesize[1], h2, 1));
			desc.planes.push_back(core::pixel_format_desc::plane(dummy_pict.linesize[2], h2, 1));

			if(desc.pix_fmt == core::pixel_format::ycbcra)						
				desc.planes.push_back(core::pixel_format_desc::plane(dummy_pict.linesize[3], height, 1));	
			return desc;
		}		
	default:		
		desc.pix_fmt = core::pixel_format::invalid;
		return desc;
	}
}

}