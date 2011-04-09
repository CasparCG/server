#pragma once

#include <core/video_format.h>
#include <core/producer/frame/frame_visitor.h>
#include <core/producer/frame/pixel_format.h>

#include "../gpu/host_buffer.h"

#include <boost/noncopyable.hpp>
#include <boost/thread/future.hpp>

#include <memory>
#include <vector>

namespace caspar { namespace mixer {
	
class image_mixer : public core::frame_visitor, boost::noncopyable
{
public:
	image_mixer(const core::video_format_desc& format_desc);
	
	virtual void begin(const core::basic_frame& frame);
	virtual void visit(core::write_frame& frame);
	virtual void end();

	boost::unique_future<safe_ptr<const host_buffer>> begin_pass();
	void end_pass();

	std::vector<safe_ptr<host_buffer>> create_buffers(const core::pixel_format_desc& format);

private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}}