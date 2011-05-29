#pragma once

#include <common/concurrency/executor.h>

#include <core/mixer/gpu/ogl_device.h>
#include <core/video_format.h>

#include <boost/noncopyable.hpp>
#include <boost/lexical_cast.hpp>

#include <string>

namespace caspar { namespace core {

struct channel_context
{
	channel_context(int index,  ogl_device& ogl, const video_format_desc& format_desc) 
		: index(index)
		, execution(print() + L"/execution")
		, destruction(print() + L"/destruction")
		, ogl(ogl)
		, format_desc(format_desc)
	{
		execution.set_priority_class(above_normal_priority_class);
		destruction.set_priority_class(below_normal_priority_class);
	}

	const int			index;
	executor			execution;
	executor			destruction;
	ogl_device&			ogl;
	video_format_desc	format_desc;

	std::wstring print() const
	{
		return L"channel[" + boost::lexical_cast<std::wstring>(index+1) + L"-" +  format_desc.name + L"]";
	}
};
	
}}