#pragma once

#include "../consumer/frame_consumer.h"

#include <common/memory/safe_ptr.h>

#include <vector>

#include <boost/noncopyable.hpp>

namespace caspar { namespace core {
	
class basic_frame;
struct video_format_desc;

class frame_consumer_device : boost::noncopyable
{
public:
	explicit frame_consumer_device(const video_format_desc& format_desc);

	void add(int index, safe_ptr<frame_consumer>&& consumer);
	void remove(int index);

	void send(const safe_ptr<const read_frame>& future_frame); // nothrow
	
	void set_video_format_desc(const video_format_desc& format_desc);
private:
	struct implementation;
	safe_ptr<implementation> impl_;
};

}}