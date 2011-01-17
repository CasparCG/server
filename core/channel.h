#pragma once

#include "consumer/frame_consumer.h"
#include "producer/frame_producer.h"

#include <common/memory/safe_ptr.h>

#include <boost/noncopyable.hpp>
#include <boost/thread/future.hpp>

namespace caspar { namespace core {

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \class	channel
///
/// \brief
/// 		
///                |**********| <-   empty frame   <- |***********| <-   frame format  <- |**********|
///   PROTOCOL ->  | PRODUCER |                       |   MIXER	  |                       | CONSUMER |  -> DISPLAY DEVICE
///                |**********| -> rendered frames -> |***********| -> formatted frame -> |**********|
///   
////////////////////////////////////////////////////////////////////////////////////////////////////
class channel : boost::noncopyable
{
public:
	explicit channel(const video_format_desc& format_desc);
	channel(channel&& other);

	// Consumers
	void add(int index, const safe_ptr<frame_consumer>& consumer);
	void remove(int index);
	
	// Layers and Producers
	void set_video_gain(int index, double value);
	void set_video_opacity(int index, double value);

	void set_audio_gain(int index, double value);

	void load(int index, const safe_ptr<frame_producer>& producer, bool play_on_load = false);
	void preview(int index, const safe_ptr<frame_producer>& producer);
	void pause(int index);
	void play(int index);
	void stop(int index);
	void clear(int index);
	void clear();	
	boost::unique_future<safe_ptr<frame_producer>> foreground(int index) const;
	boost::unique_future<safe_ptr<frame_producer>> background(int index) const;
	const video_format_desc& get_video_format_desc() const;

private:
	struct implementation;
	safe_ptr<implementation> impl_;
};

}}