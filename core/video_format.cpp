/*
* Copyright 2013 Sveriges Television AB http://casparcg.com/
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

#include "StdAfx.h"

#include "video_format.h"

#include <boost/algorithm/string.hpp>
#include <boost/assign.hpp>

#define DEFINE_VIDEOFORMATDESC(fmt, w, h, sw, sh, m, scale, duration, audio_samples, name) \
{ \
	(fmt), \
	(w), \
	(h), \
	(sw),\
	(sh),\
	(m), \
	((double)scale/(double)duration),\
	(scale),\
	(duration),\
	(m == field_mode::progressive ? 1 : 2),\
	((w)*(h)*4),\
	(name),\
	(48000),\
	(audio_samples)\
}


namespace caspar { namespace core {
	
const video_format_desc format_descs[video_format::count] =  
{									   
	DEFINE_VIDEOFORMATDESC(video_format::pal			,720,  576,  1024, 576,  field_mode::upper,			25,		1,		boost::assign::list_of(1920),							L"PAL"), 
	DEFINE_VIDEOFORMATDESC(video_format::ntsc			,720,  486,  720,  540,  field_mode::lower,			30000,	1001,	boost::assign::list_of(1602)(1601)(1602)(1601)(1602),	L"NTSC"), 
	DEFINE_VIDEOFORMATDESC(video_format::x576p2500		,1024, 576,  1024, 576,  field_mode::progressive,	25,		1,		boost::assign::list_of(1920),							L"576p2500"),
	DEFINE_VIDEOFORMATDESC(video_format::x720p2398		,1280, 720,  1280, 720,  field_mode::progressive,	24000,	1001,	boost::assign::list_of(2002),							L"720p2398"),
	DEFINE_VIDEOFORMATDESC(video_format::x720p2400		,1280, 720,  1280, 720,  field_mode::progressive,	24,		1,		boost::assign::list_of(2000),							L"720p2400"),
	DEFINE_VIDEOFORMATDESC(video_format::x720p2500		,1280, 720,  1280, 720,  field_mode::progressive,	25,		1,		boost::assign::list_of(1920),							L"720p2500"), 
	DEFINE_VIDEOFORMATDESC(video_format::x720p5000		,1280, 720,  1280, 720,  field_mode::progressive,	50,		1,		boost::assign::list_of(960),							L"720p5000"), 
	DEFINE_VIDEOFORMATDESC(video_format::x720p2997		,1280, 720,  1280, 720,  field_mode::progressive,	30000,	1001,	boost::assign::list_of(1602)(1601)(1602)(1601)(1602),	L"720p2997"),
	DEFINE_VIDEOFORMATDESC(video_format::x720p5994		,1280, 720,  1280, 720,  field_mode::progressive,	60000,	1001,	boost::assign::list_of(801) (800) (801) (801) (801),	L"720p5994"),
	DEFINE_VIDEOFORMATDESC(video_format::x720p3000		,1280, 720,  1280, 720,  field_mode::progressive,	30,		1,		boost::assign::list_of(1600),							L"720p3000"),
	DEFINE_VIDEOFORMATDESC(video_format::x720p6000		,1280, 720,  1280, 720,  field_mode::progressive,	60,		1,		boost::assign::list_of(800),							L"720p6000"),
	DEFINE_VIDEOFORMATDESC(video_format::x1080p2398		,1920, 1080, 1920, 1080, field_mode::progressive,	24000,	1001,	boost::assign::list_of(2002),							L"1080p2398"),
	DEFINE_VIDEOFORMATDESC(video_format::x1080p2400		,1920, 1080, 1920, 1080, field_mode::progressive,	24,		1,		boost::assign::list_of(2000),							L"1080p2400"),
	DEFINE_VIDEOFORMATDESC(video_format::x1080i5000		,1920, 1080, 1920, 1080, field_mode::upper,			25,		1,		boost::assign::list_of(1920),							L"1080i5000"),
	DEFINE_VIDEOFORMATDESC(video_format::x1080i5994		,1920, 1080, 1920, 1080, field_mode::upper,			30000,	1001,	boost::assign::list_of(1602)(1601)(1602)(1601)(1602),	L"1080i5994"),
	DEFINE_VIDEOFORMATDESC(video_format::x1080i6000		,1920, 1080, 1920, 1080, field_mode::upper,			30,		1,		boost::assign::list_of(1600),							L"1080i6000"),
	DEFINE_VIDEOFORMATDESC(video_format::x1080p2500		,1920, 1080, 1920, 1080, field_mode::progressive,	25,		1,		boost::assign::list_of(1920),							L"1080p2500"),
	DEFINE_VIDEOFORMATDESC(video_format::x1080p2997		,1920, 1080, 1920, 1080, field_mode::progressive,	30000,	1001,	boost::assign::list_of(1602)(1601)(1602)(1601)(1602),	L"1080p2997"),
	DEFINE_VIDEOFORMATDESC(video_format::x1080p3000		,1920, 1080, 1920, 1080, field_mode::progressive,	30,		1,		boost::assign::list_of(1600),							L"1080p3000"),
	DEFINE_VIDEOFORMATDESC(video_format::x1080p5000		,1920, 1080, 1920, 1080, field_mode::progressive,	50,		1,		boost::assign::list_of(960),							L"1080p5000"),
	DEFINE_VIDEOFORMATDESC(video_format::x1080p5994		,1920, 1080, 1920, 1080, field_mode::progressive,	60000,	1001,	boost::assign::list_of(801) (800) (801) (801) (801),	L"1080p5994"),
	DEFINE_VIDEOFORMATDESC(video_format::x1080p6000		,1920, 1080, 1920, 1080, field_mode::progressive,	60,		1,		boost::assign::list_of(800),							L"1080p6000"),
	DEFINE_VIDEOFORMATDESC(video_format::x1556p2398		,2048, 1556, 2048, 1556, field_mode::progressive,	24000,	1001,	boost::assign::list_of(2002),							L"1556p2398"),
	DEFINE_VIDEOFORMATDESC(video_format::x1556p2400		,2048, 1556, 2048, 1556, field_mode::progressive,	24,		1,		boost::assign::list_of(2000),							L"1556p2400"),
	DEFINE_VIDEOFORMATDESC(video_format::x1556p2500		,2048, 1556, 2048, 1556, field_mode::progressive,	25,		1,		boost::assign::list_of(1920),							L"1556p2500"),
	DEFINE_VIDEOFORMATDESC(video_format::dci1080p2398	,2048, 1080, 2048, 1080, field_mode::progressive,	24000,	1001,	boost::assign::list_of(2002),							L"dci1080p2398"),
	DEFINE_VIDEOFORMATDESC(video_format::dci1080p2400	,2048, 1080, 2048, 1080, field_mode::progressive,	24,		1,		boost::assign::list_of(2000),							L"dci1080p2400"),
	DEFINE_VIDEOFORMATDESC(video_format::dci1080p2500	,2048, 1080, 2048, 1080, field_mode::progressive,	25,		1,		boost::assign::list_of(1920),							L"dci1080p2500"),
	DEFINE_VIDEOFORMATDESC(video_format::x2160p2398		,3840, 2160, 3840, 2160, field_mode::progressive,	24000,	1001,	boost::assign::list_of(2002),							L"2160p2398"),
	DEFINE_VIDEOFORMATDESC(video_format::x2160p2400		,3840, 2160, 3840, 2160, field_mode::progressive,	24,		1,		boost::assign::list_of(2000),							L"2160p2400"),
	DEFINE_VIDEOFORMATDESC(video_format::x2160p2500		,3840, 2160, 3840, 2160, field_mode::progressive,	25,		1,		boost::assign::list_of(1920),							L"2160p2500"),
	DEFINE_VIDEOFORMATDESC(video_format::x2160p2997		,3840, 2160, 3840, 2160, field_mode::progressive,	30000,	1001,	boost::assign::list_of(1602)(1601)(1602)(1601)(1602),	L"2160p2997"),
	DEFINE_VIDEOFORMATDESC(video_format::x2160p3000		,3840, 2160, 3840, 2160, field_mode::progressive,	30,		1,		boost::assign::list_of(1600),							L"2160p3000"),
	DEFINE_VIDEOFORMATDESC(video_format::x2160p5000		,3840, 2160, 3840, 2160, field_mode::progressive,	50,		1,		boost::assign::list_of(960),							L"2160p5000"),
	DEFINE_VIDEOFORMATDESC(video_format::x2160p5994		,3840, 2160, 3840, 2160, field_mode::progressive,	60000,	1001,	boost::assign::list_of(801) (800) (801) (801) (801),	L"2160p5994"),
	DEFINE_VIDEOFORMATDESC(video_format::x2160p6000		,3840, 2160, 3840, 2160, field_mode::progressive,	60,		1,		boost::assign::list_of(800),							L"2160p6000"),
	DEFINE_VIDEOFORMATDESC(video_format::dci2160p2398	,4096, 2160, 4096, 2160, field_mode::progressive,	24000,	1001,	boost::assign::list_of(2002),							L"dci2160p2398"),
	DEFINE_VIDEOFORMATDESC(video_format::dci2160p2400	,4096, 2160, 4096, 2160, field_mode::progressive,	24,		1,		boost::assign::list_of(2000),							L"dci2160p2400"),
	DEFINE_VIDEOFORMATDESC(video_format::dci2160p2500	,4096, 2160, 4096, 2160, field_mode::progressive,	25,		1,		boost::assign::list_of(1920),							L"dci2160p2500"),
	DEFINE_VIDEOFORMATDESC(video_format::invalid		,0,		0,   0,		0,   field_mode::progressive,	1,		1,		boost::assign::list_of(1),								L"invalid")
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

