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

#include <vector>
#include <string>

namespace caspar { namespace core {
	
struct video_format 
{ 
	enum type
	{
		pal = 0,
		ntsc,
		x576p2500,
		x720p2500,
		x720p5000,
		x720p2398,
		x720p2400,
		x720p2997,
		x720p5994,
		x720p3000,
		x720p6000,
		x1080p2398,
		x1080p2400,
		x1080i5000,
		x1080i5994,
		x1080i6000,
		x1080p2500,
		x1080p2997,
		x1080p3000,
		x1080p5000,
		x1080p5994,
		x1080p6000,
		invalid,
		count
	};
};

struct field_mode 
{ 
	enum type
	{
		empty		= 0,
		lower		= 1,
		upper		= 2,
		progressive = 3 // NOTE: progressive == lower | upper;
	};

	static std::wstring print(field_mode::type value)
	{
		switch(value)
		{
			case progressive:
				return L"progressive"; 
			case lower:
				return L"lower";
			case upper:
				return L"upper";
			default:
				return L"invalid";
		}
	}
};

struct video_format_desc
{
	video_format::type		format;		// video output format

	size_t					width;		// output frame width
	size_t					height;		// output frame height
	size_t					square_width;
	size_t					square_height;
	field_mode::type		field_mode;	// progressive, interlaced upper field first, interlaced lower field first
	double					fps;		// actual framerate, e.g. i50 = 25 fps, p50 = 50 fps
	size_t					time_scale;
	size_t					duration;
	size_t					field_count;
	size_t					size;		// output frame size in bytes 
	std::wstring			name;		// name of output format

	size_t					audio_sample_rate;
	size_t					audio_channels;
	std::vector<size_t>		audio_cadence;

	static const video_format_desc& get(video_format::type format);
	static const video_format_desc& get(const std::wstring& name);
	
	bool operator==(const video_format_desc& lhs)
	{
		return format == lhs.format;
	}

	bool operator!=(const video_format_desc& lhs)
	{
		return !(*this == lhs);
	}
};

inline std::wostream& operator<<(std::wostream& out, const video_format_desc& format_desc)
{
	out << format_desc.name.c_str();
	return out;
}

}}