#pragma once

#include "../producer/frame_producer.h"
#include "../consumer/frame_consumer.h"
#include "../frame/frame_format.h"

#include "layer.h"

namespace caspar { namespace core { namespace renderer {
	
class render_device : boost::noncopyable
{	
public:
	render_device(const frame_format_desc& format_desc, const std::vector<frame_consumer_ptr>& consumers);
	
	void load(int render_layer, const frame_producer_ptr& producer, load_option option = load_option::none);	
	void pause(int render_layer);
	void play(int render_layer);
	void stop(int render_layer);
	void clear(int render_layer);
	void clear();

	frame_producer_ptr active(int render_layer) const;
	frame_producer_ptr background(int render_layer) const;

	const frame_format_desc& get_frame_format_desc() const;		
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<render_device> render_device_ptr;
typedef std::unique_ptr<render_device> render_device_uptr;

}}}
