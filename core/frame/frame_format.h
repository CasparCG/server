#pragma once

#include <string>

#include "../../common/compiler/vs/disable_silly_warnings.h"

namespace caspar { namespace core {

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \enum	video_mode
///
////////////////////////////////////////////////////////////////////////////////////////////////////
enum video_mode
{
	progressive,
	lower,
	upper
};

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \enum	frame_format
///
////////////////////////////////////////////////////////////////////////////////////////////////////
enum frame_format
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

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \struct	frame_format_desc
///
////////////////////////////////////////////////////////////////////////////////////////////////////
struct frame_format_desc
{
	size_t			width;
	size_t			height;
	video_mode		mode;
	double			fps;
	size_t			size;
	std::wstring	name;
	frame_format	format;

	static const frame_format_desc format_descs[frame_format::count];
};

inline bool operator==(const frame_format_desc& rhs, const frame_format_desc& lhs)
{
	return rhs.format == lhs.format;
}

inline bool operator!=(const frame_format_desc& rhs, const frame_format_desc& lhs)
{
	return !(rhs == lhs);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \fn	frame_format get_video_format(const std::wstring& strVideoMode);
///
/// \brief	Gets the *frame_format* associated with the specified name.
///
/// \param	name	Name of the *frame_format*.. 
///
/// \return	The video format. 
////////////////////////////////////////////////////////////////////////////////////////////////////
frame_format get_video_format(const std::wstring& name);

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \fn	frame_format get_video_format_desc(const std::wstring& strVideoMode);
///
/// \brief	Gets the *frame_format_desc* associated with the specified name.
///
/// \param	name	Name of the *frame_format_desc*.. 
///
/// \return	The video format. 
////////////////////////////////////////////////////////////////////////////////////////////////////
inline frame_format_desc get_video_format_desc(const std::wstring& name, frame_format default_format = frame_format::x576p2500)
{			
	auto casparVideoFormat = default_format;
	if(!name.empty())
		casparVideoFormat = get_video_format(name);
	return frame_format_desc::format_descs[casparVideoFormat];
}

inline double render_frame_format_period(const frame_format_desc& format_desc)
{
	return 1.0/(format_desc.mode == video_mode::progressive ? format_desc.fps : format_desc.fps/2.0);
}

inline std::wostream& operator<<(std::wostream& out, const frame_format_desc& format_desc)
{
  out << format_desc.name.c_str();
  return out;
}

}}