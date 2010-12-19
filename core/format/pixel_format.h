#pragma once

#include <memory>
#include <vector>

namespace caspar { namespace core {
	
struct pixel_format
{
	enum type
	{
		bgra,
		rgba,
		argb,
		abgr,
		ycbcr,
		ycbcra,
		count,
		invalid
	};
};

struct pixel_format_desc
{
	struct plane
	{
		plane() : linesize(0), width(0), height(0), size(0), channels(0){}
		plane(size_t width, size_t height, size_t channels)
			: linesize(width*channels), width(width), height(height), size(width*height*channels), channels(channels)
		{}
		size_t linesize;
		size_t width;
		size_t height;
		size_t size;
		size_t channels;
	};

	pixel_format_desc() : pix_fmt(pixel_format::invalid){}
	
	pixel_format::type pix_fmt;
	std::vector<plane> planes;
};

size_t hash(const pixel_format_desc& desc);
	
inline bool operator==(const pixel_format_desc& lhs, const pixel_format_desc& rhs)
{
	return hash(lhs) == hash(rhs);
}

inline bool operator!=(const pixel_format_desc& lhs, const pixel_format_desc& rhs)
{
	return !(lhs == rhs);
}
	
}}

namespace std {

template<> struct hash<caspar::core::pixel_format_desc>
{
	size_t operator()(const caspar::core::pixel_format_desc& desc) const { return caspar::core::hash(desc);}
};

}