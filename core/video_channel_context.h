#pragma once

#include <common/concurrency/executor.h>

#include <core/mixer/gpu/ogl_device.h>
#include <core/video_format.h>

#include <boost/noncopyable.hpp>
#include <boost/lexical_cast.hpp>

#include <string>

namespace caspar { namespace core {

struct video_channel_context
{
	video_channel_context(int index,  ogl_device& ogl, const video_format_desc& format_desc) 
		: index(index)
		, format_desc(format_desc)
		, execution(print() + L"/execution")
		, destruction(print() + L"/destruction")
		, ogl(ogl)
	{
		execution.set_priority_class(above_normal_priority_class);
	}

	const int			index;
	video_format_desc	format_desc;
	executor			execution;
	executor			destruction;
	ogl_device&			ogl;

	std::wstring print() const
	{
		return L"video_channel[" + boost::lexical_cast<std::wstring>(index+1) + L"-" +  format_desc.name + L"]";
	}
};
	
}}