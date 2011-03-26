#pragma once

#include "consumer/frame_consumer_device.h"
#include "mixer/frame_mixer_device.h"
#include "producer/frame_producer_device.h"

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
	explicit channel(int index, const video_format_desc& format_desc);
	channel(channel&& other);

	frame_producer_device& producer();
	frame_mixer_device& mixer();
	frame_consumer_device& consumer();

	const video_format_desc& get_video_format_desc() const;
	void set_video_format_desc(const video_format_desc& format_desc);

	std::wstring print() const;

private:
	struct implementation;
	safe_ptr<implementation> impl_;
};

}}