#pragma once

#include <common/concurrency/executor.h>

#include <core/mixer/gpu/ogl_device.h>
#include <core/video_format.h>

#include <tbb/spin_rw_mutex.h>

#include <boost/noncopyable.hpp>
#include <boost/lexical_cast.hpp>

#include <string>

namespace caspar { namespace core {

class video_channel_context
{
	mutable tbb::spin_rw_mutex	mutex_;
	const int					index_;
	video_format_desc			format_desc_;
	executor					execution_;
	executor					destruction_;
	ogl_device&					ogl_;

public:
	video_channel_context(int index, ogl_device& ogl, const video_format_desc& format_desc) 
		: index_(index)
		, format_desc_(format_desc)
		, execution_(print() + L"/execution")
		, destruction_(print() + L"/destruction")
		, ogl_(ogl)
	{
		execution_.set_priority_class(above_normal_priority_class);
		destruction_.set_priority_class(below_normal_priority_class);
	}

	const int index() const {return index_;}

	video_format_desc get_format_desc()
	{
		tbb::spin_rw_mutex::scoped_lock lock(mutex_, false);
		return format_desc_;
	}

	void set_format_desc(const video_format_desc& format_desc)
	{
		tbb::spin_rw_mutex::scoped_lock lock(mutex_, true);
		format_desc_ = format_desc;
	}

	executor& execution() {return execution_;}
	executor& destruction() {return destruction_;}
	ogl_device& ogl() { return ogl_;}

	std::wstring print() const
	{
		return L"video_channel[" + boost::lexical_cast<std::wstring>(index_+1) + L"-" +  format_desc_.name + L"]";
	}
};
	
}}