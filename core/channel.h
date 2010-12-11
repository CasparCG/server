#pragma once

#include "producer/frame_producer_device.h"
#include "consumer/frame_consumer_device.h"
#include "processor/frame_processor_device.h"

#include <boost/noncopyable.hpp>

#include <memory>

namespace caspar { namespace core {

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \class	channel
///
/// \brief	Channel. 
/// 		
///                |**********| <-   empty frame   <- |***********| <-   frame format  <- |**********|
///   PROTOCOL ->  | PRODUCER |                       | PROCESSOR |                       | CONSUMER |  -> DISPLAY DEVICE
///                |**********| -> rendered frames -> |***********| -> formatted frame -> |**********|
///   
////////////////////////////////////////////////////////////////////////////////////////////////////
class channel : boost::noncopyable
{
public:
	channel(const frame_producer_device_ptr& producer_device, const frame_processor_device_ptr& processor_device, const frame_consumer_device_ptr& consumer_device);
	
	void load(int render_layer, const frame_producer_ptr& producer, load_option::type option = load_option::none);
	void pause(int render_layer);
	void play(int render_layer);
	void stop(int render_layer);
	void clear(int render_layer);
	void clear();	
	boost::unique_future<frame_producer_ptr> foreground(int render_layer) const;
	boost::unique_future<frame_producer_ptr> background(int render_layer) const;
	const video_format_desc& get_video_format_desc() const;
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<channel> channel_ptr;

}}