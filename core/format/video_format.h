#pragma once

#include <string>

#include "../../common/compiler/vs/disable_silly_warnings.h"

namespace caspar { namespace core {
	
struct video_format 
{ 
	enum type
	{
		pal = 0,
		//ntsc,
		x576p2500,
		x720p2500,
		x720p5000,
		//x720p5994,
		//x720p6000,
		//x1080p2397,
		//x1080p2400,
		x1080i5000,
		//x1080i5994,
		//x1080i6000,
		x1080p2500,
		//x1080p2997,
		//x1080p3000,
		count,
		invalid
	};
};

struct video_mode 
{ 
	enum type
	{
		progressive,
		lower,
		upper,
		count,
		invalid
	};
};

struct video_format_desc
{
	video_format::type		format;

	size_t					width;
	size_t					height;
	video_mode::type		mode;
	double					fps;
	double					actual_fps;
	double					actual_interval;
	size_t					size;
	std::wstring			name;

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