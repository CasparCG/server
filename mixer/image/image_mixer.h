#pragma once

#include <core/producer/frame/frame_visitor.h>

#include "../gpu/host_buffer.h"

#include <core/video_format.h>

#include <boost/noncopyable.hpp>
#include <boost/thread/future.hpp>

#include <memory>
#include <vector>

namespace caspar { namespace core {

struct pixel_format_desc;	
class image_transform;

class image_mixer : public core::frame_visitor, boost::noncopyable
{
public:
	image_mixer(const video_format_desc& format_desc);
	
	virtual void begin(const basic_frame& frame);
	virtual void visit(write_frame& frame);
	virtual void end();

	boost::unique_future<safe_ptr<const host_buffer>> begin_pass();
	void end_pass();

	std::vector<safe_ptr<host_buffer>> create_buffers(const pixel_format_desc& format);

private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}}