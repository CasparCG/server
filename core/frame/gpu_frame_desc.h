#pragma once

#include <memory>
#include <array>

enum pixel_format
{
	invalid_pixel_format = 0,
	bgra,
	rgba,
	argb,
	abgr,
	ycbcr,
	ycbcra,
	pixel_format_count,
};

namespace caspar { namespace core {
	
struct plane
{
	plane(){}
	plane(size_t width, size_t height, size_t channels)
		: width(width), height(height), size(width*height*channels), channels(channels)
	{}
	size_t width;
	size_t height;
	size_t size;
	size_t channels;
};

struct gpu_frame_desc
{
	gpu_frame_desc(){memset(this, sizeof(gpu_frame_desc), 0);}
	pixel_format pix_fmt;
	std::array<plane, 4> planes;
	size_t plane_count;
};
	
}}