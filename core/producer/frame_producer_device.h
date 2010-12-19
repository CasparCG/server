#pragma once

#include "frame_producer.h"

#include "../format/video_format.h"

#include <common/utility/safe_ptr.h>

#include <boost/thread/future.hpp>

#include <memory>

namespace caspar { namespace core {

class frame_producer;
class frame_processor_device;
class layer;
		
class frame_producer_device : boost::noncopyable
{	
public:
	frame_producer_device(frame_producer_device&& other);  // nothrow
	frame_producer_device(const safe_ptr<frame_processor_device>& frame_processor);  // nothrow
	
	void load	(int render_layer, const safe_ptr<frame_producer>& producer, bool autoplay = false); // throws if producer->initialize throws
	void preview(int render_layer, const safe_ptr<frame_producer>& producer); // throws if producer->initialize throws
	void pause	(int render_layer);  // nothrow
	void play	(int render_layer);  // nothrow
	void stop	(int render_layer);  // nothrow
	void clear	(int render_layer);  // nothrow
	void clear	();  // nothrow
	
	boost::unique_future<safe_ptr<frame_producer>> foreground(int render_layer) const; // nothrow
	boost::unique_future<safe_ptr<frame_producer>> background(int render_layer) const; // nothrow
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<frame_producer_device> frame_producer_device_ptr;

}}
