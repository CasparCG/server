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
 
#include "..\StdAfx.h"

#include "video_format.h"

#include <boost/algorithm/string.hpp>

#define DEFINE_VIDEOFORMATDESC(w, h, m, f, s, fmt) { (fmt), (w), (h), (m), (f), (w)*(h)*4, (s) }

namespace caspar { namespace core {

const video_format_desc video_format_desc::format_descs[video_format::count] =  
{									   
	DEFINE_VIDEOFORMATDESC(720,  576,  video_update_format::upper,			50,			TEXT("PAL"),		video_format::pal		), 
	DEFINE_VIDEOFORMATDESC(720,  486,  video_update_format::lower,			60/1.001,	TEXT("NTSC"),		video_format::ntsc		), 
	DEFINE_VIDEOFORMATDESC(720,  576,  video_update_format::progressive,		25,			TEXT("576p2500"),	video_format::x576p2500	),
	DEFINE_VIDEOFORMATDESC(1280, 720,  video_update_format::progressive,		25,			TEXT("720p2500"),	video_format::x720p2500	), 
	DEFINE_VIDEOFORMATDESC(1280, 720,  video_update_format::progressive,		50,			TEXT("720p5000"),	video_format::x720p5000	), 
	DEFINE_VIDEOFORMATDESC(1280, 720,  video_update_format::progressive,		60/1.001,	TEXT("720p5994"),	video_format::x720p5994	),
	DEFINE_VIDEOFORMATDESC(1280, 720,  video_update_format::progressive,		60,			TEXT("720p6000"),	video_format::x720p6000	),
	DEFINE_VIDEOFORMATDESC(1920, 1080, video_update_format::progressive,		24/1.001,	TEXT("1080p2397"),	video_format::x1080p2397),
	DEFINE_VIDEOFORMATDESC(1920, 1080, video_update_format::progressive,		24,			TEXT("1080p2400"),	video_format::x1080p2400),
	DEFINE_VIDEOFORMATDESC(1920, 1080, video_update_format::upper,			50,			TEXT("1080i5000"),	video_format::x1080i5000),
	DEFINE_VIDEOFORMATDESC(1920, 1080, video_update_format::upper,			60/1.001,	TEXT("1080i5994"),	video_format::x1080i5994),
	DEFINE_VIDEOFORMATDESC(1920, 1080, video_update_format::upper,			60,			TEXT("1080i6000"),	video_format::x1080i6000),
	DEFINE_VIDEOFORMATDESC(1920, 1080, video_update_format::progressive,		25,			TEXT("1080p2500"),	video_format::x1080p2500),
	DEFINE_VIDEOFORMATDESC(1920, 1080, video_update_format::progressive,		30/1.001,	TEXT("1080p2997"),	video_format::x1080p2997),
	DEFINE_VIDEOFORMATDESC(1920, 1080, video_update_format:: progressive,	30,			TEXT("1080p3000"),	video_format::x1080p3000)
};

video_format::type get_video_format(const std::wstring& name)
{
	for(int n = 0; n < video_format::count; ++n)
	{
		if(boost::iequals(video_format_desc::format_descs[n].name, name))
			return static_cast<video_format::type>(n);
	}

	return video_format::invalid;
}

}}

