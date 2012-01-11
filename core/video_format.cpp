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

#include "StdAfx.h"

#include "video_format.h"

#include <boost/algorithm/string.hpp>
#include <boost/assign.hpp>

namespace caspar { namespace core {
	
const std::vector<video_format_desc> format_descs = boost::assign::list_of
	(video_format_desc(video_format::pal,			 720,  576, 1024, 576,  field_mode::upper,			   25,	   1, L"PAL",		boost::assign::list_of<int>(3840)))
	(video_format_desc(video_format::ntsc,			 720,  486,  720, 540,  field_mode::lower,			30000,	1001, L"NTSC",		boost::assign::list_of<int>(3204)(3202)(3204)(3202)(3204)))
	(video_format_desc(video_format::x576p2500,		 720,  576,  720, 576,  field_mode::progressive,	   25,	   1, L"576p2500",	boost::assign::list_of<int>(3840)						))
	(video_format_desc(video_format::x720p2500,		1280,  720, 1280, 720,  field_mode::progressive,	   25,	   1, L"720p2500",	boost::assign::list_of<int>(3840)						)) 
	(video_format_desc(video_format::x720p5000,		1280,  720, 1280, 720,  field_mode::progressive,	   50,	   1, L"720p5000",	boost::assign::list_of<int>(1920)						)) 
	(video_format_desc(video_format::x720p5994,		1280,  720, 1280, 720,  field_mode::progressive,	60000,	1001, L"720p5994",	boost::assign::list_of<int>(1602)(1601)(1602)(1601)(1602)))
	(video_format_desc(video_format::x720p6000,		1280,  720, 1280, 720,  field_mode::progressive,	   60,	   1, L"720p6000",	boost::assign::list_of<int>(1600)						))
	(video_format_desc(video_format::x1080p2397,	1920, 1080, 1920, 1080, field_mode::progressive,	24000,	1001, L"1080p2398",	boost::assign::list_of<int>(4004)						))
	(video_format_desc(video_format::x1080p2400,	1920, 1080, 1920, 1080, field_mode::progressive,	   24,	   1, L"1080p2400",	boost::assign::list_of<int>(4000)						))
	(video_format_desc(video_format::x1080i5000,	1920, 1080, 1920, 1080, field_mode::upper,			   25,	   1, L"1080i5000",	boost::assign::list_of<int>(3840)						))
	(video_format_desc(video_format::x1080i5994,	1920, 1080, 1920, 1080, field_mode::upper,			30000,	1001, L"1080i5994",	boost::assign::list_of<int>(3204)(3202)(3204)(3202)(3204)))
	(video_format_desc(video_format::x1080i6000,	1920, 1080, 1920, 1080, field_mode::upper,			   30,	   1, L"1080i6000",	boost::assign::list_of<int>(3200)						))
	(video_format_desc(video_format::x1080p2500,	1920, 1080, 1920, 1080, field_mode::progressive,	   25,	   1, L"1080p2500",	boost::assign::list_of<int>(3840)						))
	(video_format_desc(video_format::x1080p2997,	1920, 1080, 1920, 1080, field_mode::progressive,	30000,	1001, L"1080p2997",	boost::assign::list_of<int>(3204)(3202)(3204)(3202)(3204)))
	(video_format_desc(video_format::x1080p3000,	1920, 1080, 1920, 1080, field_mode::progressive,	   30,	   1, L"1080p3000",	boost::assign::list_of<int>(3200)						))
	(video_format_desc(video_format::x1080p5000,	1920, 1080, 1920, 1080, field_mode::progressive,	   50,	   1, L"1080p5000",	boost::assign::list_of<int>(1920)						))
	(video_format_desc(video_format::invalid,		   0,	 0,    0,	 0, field_mode::progressive,	    1,	   1, L"invalid",	boost::assign::list_of<int>(1)							));

video_format_desc::video_format_desc(video_format format,
					int width,
					int height,
					int square_width,
					int square_height,
					core::field_mode field_mode,
					int time_scale,
					int duration,
					const std::wstring& name,
					const std::vector<int>& audio_cadence)
	: format(format)
	, width(width)
	, height(height)
	, square_width(square_width)
	, square_height(square_height)
	, field_mode(field_mode)
	, fps((double)time_scale/(double)duration)
	, time_scale(time_scale)
	, duration(duration)
	, field_count(field_mode == field_mode::progressive ? 1 : 2)
	, size(width*height*4)
	, name(name)
	, audio_sample_rate(48000)
	, audio_channels(2)
	, audio_cadence(audio_cadence)
{
}

video_format_desc::video_format_desc(video_format format)
	: format(video_format::invalid)
	, field_mode(field_mode::empty)
{
	*this = format_descs.at(format.value());
}

video_format_desc::video_format_desc(const std::wstring& name)
	: format(video_format::invalid)
	, field_mode(field_mode::empty)
{	
	*this = video_format_desc(video_format::invalid);
	for(auto it = std::begin(format_descs); it != std::end(format_descs)-1; ++it)
	{
		if(boost::iequals(it->name, name))
		{
			*this = *it;
			break;
		}
	}
}

video_format_desc& video_format_desc::operator=(const video_format_desc& other)
{
	format				= other.format;			
	width				= other.width;
	height				= other.height;
	square_width		= other.square_width;
	square_height		= other.square_height;
	field_mode			= other.field_mode;
	fps					= other.fps;
	time_scale			= other.time_scale;
	duration			= other.duration;
	field_count			= other.field_count;
	size				= other.size;
	name				= other.name;
	audio_sample_rate 	= other.audio_sample_rate;
	audio_channels		= other.audio_channels;
	audio_cadence		= other.audio_cadence;

	return *this;
}

bool operator==(const video_format_desc& lhs, const video_format_desc& rhs)
{											
	return lhs.format == rhs.format;
}

bool operator!=(const video_format_desc& lhs, const video_format_desc& rhs)
{
	return !(lhs == rhs);
}

std::wostream& operator<<(std::wostream& out, const video_format_desc& format_desc)
{
	out << format_desc.name.c_str();
	return out;
}


}}

