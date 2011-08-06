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
 
#include "StdAfx.h"

#include "video_format.h"

#include <boost/algorithm/string.hpp>

#include <array>

#define DEFINE_VIDEOFORMATDESC(fmt, w, h, m, scale, duration, name) \
{ \
	(fmt), \
	(w), \
	(h), \
	(m), \
	((double)scale/(double)duration),\
	(scale),\
	(duration),\
	(m == video_mode::progressive ? 1 : 2),\
	((w)*(h)*4),\
	(name),\
	(2),\
	(48000),\
	(2),\
	(static_cast<size_t>(48000.0*2.0/((double)scale/(double)duration)+0.99))\
}

namespace caspar { namespace core {
	
const video_format_desc format_descs[video_format::count] =  
{									   
	DEFINE_VIDEOFORMATDESC(video_format::pal		,720,  576,  video_mode::upper,			25,		1,		TEXT("PAL")		), 
	DEFINE_VIDEOFORMATDESC(video_format::ntsc		,720,  486,  video_mode::lower,			30000,	1001,	TEXT("NTSC")	), 
	DEFINE_VIDEOFORMATDESC(video_format::x576p2500	,720,  576,  video_mode::progressive,	25,		1,		TEXT("576p2500")),
	DEFINE_VIDEOFORMATDESC(video_format::x720p2500	,1280, 720,  video_mode::progressive,	25,		1,		TEXT("720p2500")), 
	DEFINE_VIDEOFORMATDESC(video_format::x720p5000	,1280, 720,  video_mode::progressive,	50,		1,		TEXT("720p5000")), 
	DEFINE_VIDEOFORMATDESC(video_format::x720p5994	,1280, 720,  video_mode::progressive,	60000,	1001,	TEXT("720p5994")),
	DEFINE_VIDEOFORMATDESC(video_format::x720p6000	,1280, 720,  video_mode::progressive,	60,		1,		TEXT("720p6000")),
	DEFINE_VIDEOFORMATDESC(video_format::x1080p2397	,1920, 1080, video_mode::progressive,	24000,	1001,	TEXT("1080p2398")),
	DEFINE_VIDEOFORMATDESC(video_format::x1080p2400	,1920, 1080, video_mode::progressive,	24,		1,		TEXT("1080p2400")),
	DEFINE_VIDEOFORMATDESC(video_format::x1080i5000	,1920, 1080, video_mode::upper,			25,		1,		TEXT("1080i5000")),
	DEFINE_VIDEOFORMATDESC(video_format::x1080i5994	,1920, 1080, video_mode::upper,			30000,	1001,	TEXT("1080i5994")),
	DEFINE_VIDEOFORMATDESC(video_format::x1080i6000	,1920, 1080, video_mode::upper,			30,		1,		TEXT("1080i6000")),
	DEFINE_VIDEOFORMATDESC(video_format::x1080p2500	,1920, 1080, video_mode::progressive,	25,		1,		TEXT("1080p2500")),
	DEFINE_VIDEOFORMATDESC(video_format::x1080p2997	,1920, 1080, video_mode::progressive,	30000,	1001,	TEXT("1080p2997")),
	DEFINE_VIDEOFORMATDESC(video_format::x1080p3000	,1920, 1080, video_mode::progressive,	30,		1,		TEXT("1080p3000")),
	DEFINE_VIDEOFORMATDESC(video_format::x1080p5000	,1920, 1080, video_mode::progressive,	50,		1,		TEXT("1080p5000")),
	DEFINE_VIDEOFORMATDESC(video_format::invalid	,0,		0, video_mode::count,			1,		1,		TEXT("invalid"))
};

const video_format_desc& video_format_desc::get(video_format::type format)	
{
	return format_descs[format];
}

const video_format_desc& video_format_desc::get(const std::wstring& name)	
{
	for(int n = 0; n < video_format::invalid; ++n)
	{
		if(boost::iequals(format_descs[n].name, name))
			return format_descs[n];
	}
	return format_descs[video_format::invalid];
}

}}

