#pragma once

#include <common/concurrency/executor.h>

#include <core/video_format.h>

#include <tbb/spin_rw_mutex.h>

#include <boost/noncopyable.hpp>
#include <boost/lexical_cast.hpp>

#include <string>

namespace caspar { 

class executor;

namespace core {

class ogl_device;

class video_channel_context
{

public:
	video_channel_context(int index, ogl_device& ogl, const video_format_desc& format_desc);

	const int			index() const;
	video_format_desc	get_format_desc();
	void				set_format_desc(const video_format_desc& format_desc);
	executor&			execution();
	executor&			destruction();
	ogl_device&			ogl();
	std::wstring		print() const;
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
	
}}