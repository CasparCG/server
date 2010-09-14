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

#include "frame_format.h"

#include <boost/algorithm/string.hpp>

#define DEFINE_VIDEOFORMATDESC(w, h, m, f, s, fmt) { (w), (h), (m), (f), (w)*(h)*4, s, (fmt) }

namespace caspar {

const frame_format_desc frame_format_desc::format_descs[frame_format::count] =  
{	
	DEFINE_VIDEOFORMATDESC(720, 576, video_mode::upper, 50, TEXT("PAL"), frame_format::pal), 
	DEFINE_VIDEOFORMATDESC(720, 486, video_mode::lower, 60/1.001, TEXT("NTSC"), frame_format::ntsc), 
	DEFINE_VIDEOFORMATDESC(720, 576, video_mode::progressive, 25, TEXT("576p2500"), frame_format::x576p2500),
	DEFINE_VIDEOFORMATDESC(1280, 720, video_mode::progressive, 50, TEXT("720p5000"), frame_format::x720p5000), 
	DEFINE_VIDEOFORMATDESC(1280, 720, video_mode::progressive, 60/1.001, TEXT("720p5994"), frame_format::x720p5994),
	DEFINE_VIDEOFORMATDESC(1280, 720, video_mode::progressive, 60, TEXT("720p6000"), frame_format::x720p6000),
	DEFINE_VIDEOFORMATDESC(1920, 1080, video_mode::progressive, 24/1.001, TEXT("1080p2397"), frame_format::x1080p2397),
	DEFINE_VIDEOFORMATDESC(1920, 1080, video_mode::progressive, 24, TEXT("1080p2400"), frame_format::x1080p2400),
	DEFINE_VIDEOFORMATDESC(1920, 1080, video_mode::upper, 50, TEXT("1080i5000"), frame_format::x1080i5000),
	DEFINE_VIDEOFORMATDESC(1920, 1080, video_mode::upper, 60/1.001, TEXT("1080i5994"), frame_format::x1080i5994),
	DEFINE_VIDEOFORMATDESC(1920, 1080, video_mode::upper, 60, TEXT("1080i6000"), frame_format::x1080i6000),
	DEFINE_VIDEOFORMATDESC(1920, 1080, video_mode::progressive, 25, TEXT("1080p2500"), frame_format::x1080p2500),
	DEFINE_VIDEOFORMATDESC(1920, 1080, video_mode::progressive, 30/1.001, TEXT("1080p2997"), frame_format::x1080p2997),
	DEFINE_VIDEOFORMATDESC(1920, 1080,video_mode:: progressive, 30, TEXT("1080p3000"), frame_format::x1080p3000)
};

frame_format get_video_format(const std::wstring& strVideoMode)
{
	for(int n = 0; n < frame_format::count; ++n)
	{
		if(boost::iequals(frame_format_desc::format_descs[n].name, strVideoMode))
			return static_cast<frame_format>(n);
	}

	return frame_format::invalid;
}

}	//namespace caspar

