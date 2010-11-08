#pragma once

#include <string>

#include "../../common/compiler/vs/disable_silly_warnings.h"

namespace caspar { namespace core {
	
////////////////////////////////////////////////////////////////////////////////////////////////////
/// \enum	video_format
///
////////////////////////////////////////////////////////////////////////////////////////////////////
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
		count,
		invalid
	};
};

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \enum	video_update_format
///
////////////////////////////////////////////////////////////////////////////////////////////////////
struct video_update_format 
{ 
	enum type
	{
			progressive,
			lower,
			upper
	};
};

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \struct	video_format_desc
///
////////////////////////////////////////////////////////////////////////////////////////////////////
struct video_format_desc
{
	video_format::type			format;

	size_t						width;
	size_t						height;
	video_update_format::type	update;
	double						fps;
	size_t						size;
	std::wstring				name;

	static const video_format_desc format_descs[video_format::count];
};

inline bool operator==(const video_format_desc& rhs, const video_format_desc& lhs)
{
	return rhs.format == lhs.format;
}

inline bool operator!=(const video_format_desc& rhs, const video_format_desc& lhs)
{
	return !(rhs == lhs);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \fn	video_format get_video_format(const std::wstring& strVideoMode);
///
/// \brief	Gets the *video_format* associated with the specified name.
///
/// \param	name	Name of the *video_format*.. 
///
/// \return	The video format. 
////////////////////////////////////////////////////////////////////////////////////////////////////
video_format::type get_video_format(const std::wstring& name);

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \fn	video_format get_video_format_desc(const std::wstring& strVideoMode);
///
/// \brief	Gets the *video_format_desc* associated with the specified name.
///
/// \param	name	Name of the *video_format_desc*.. 
///
/// \return	The video format. 
////////////////////////////////////////////////////////////////////////////////////////////////////
inline video_format_desc get_video_format_desc(const std::wstring& name, video_format::type default_format = video_format::x576p2500)
{			
	auto casparVideoFormat = default_format;
	if(!name.empty())
		casparVideoFormat = get_video_format(name);
	return video_format_desc::format_descs[casparVideoFormat];
}

inline double render_video_format_period(const video_format_desc& format_desc)
{
	return 1.0/(format_desc.update == video_update_format::progressive ? format_desc.fps : format_desc.fps/2.0);
}

inline std::wostream& operator<<(std::wostream& out, const video_format_desc& format_desc)
{
  out << format_desc.name.c_str();
  return out;
}

}}