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

#pragma once

#include <string>

#include <common/compiler/vs/disable_silly_warnings.h>

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
		x720p5994,
		x720p6000,
		x1080p2397,
		x1080p2400,
		x1080i5000,
		x1080i5994,
		x1080i6000,
		x1080p2500,
		x1080p2997,
		x1080p3000,
		x1080p5000,
		invalid,
		count
	};
};

struct video_mode 
{ 
	enum type
	{
		progressive = 0,
		lower,
		upper,
		count,
		invalid
	};
};

struct video_format_desc
{
	video_format::type		format;		// video output format

	size_t					width;		// output frame width
	size_t					height;		// output frame height
	video_mode::type		mode;		// progressive, interlaced upper field first, interlaced lower field first
	double					fps;		// actual framerate, e.g. i50 = 25 fps, p50 = 50 fps
	size_t					time_scale;
	size_t					duration;
	size_t					field_count;
	size_t					size;		// output frame size in bytes 
	std::wstring			name;		// name of output format

	size_t					audio_bytes_per_sample;
	size_t					audio_sample_rate;
	size_t					audio_channels;
	size_t					audio_samples_per_frame;

	static const video_format_desc& get(video_format::type format);
	static const video_format_desc& get(const std::wstring& name);
};

inline bool operator==(const video_format_desc& rhs, const video_format_desc& lhs)
{
	return rhs.format == lhs.format;
}

inline bool operator!=(const video_format_desc& rhs, const video_format_desc& lhs)
{
	return !(rhs == lhs);
}

inline std::wostream& operator<<(std::wostream& out, const video_format_desc& format_desc)
{
	out << format_desc.name.c_str();
	return out;
}

}}