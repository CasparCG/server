#include "StdAfx.h"

#include "channel.h"

#include "consumer/frame_consumer_device.h"
#include "mixer/frame_mixer_device.h"
#include "producer/frame_producer_device.h"

#include "mixer/frame/draw_frame.h"
#include "mixer/frame_mixer_device.h"

#include "producer/layer.h"

#include <common/concurrency/executor.h>

#include <boost/range/algorithm_ext/erase.hpp>

#include <tbb/parallel_for.h>

#include <memory>

namespace caspar { namespace core {

struct channel::implementation : boost::noncopyable
{					
	std::shared_ptr<frame_consumer_device>	consumer_;
	std::shared_ptr<frame_mixer_device>		mixer_;
	std::shared_ptr<frame_producer_device>	producer_;

	const video_format_desc format_desc_;

public:
	implementation(const video_format_desc& format_desc)  
		: format_desc_(format_desc)
		, consumer_(new frame_consumer_device(format_desc))
		, mixer_(new frame_mixer_device(format_desc, std::bind(&frame_consumer_device::send, consumer_.get(), std::placeholders::_1)))
		, producer_(new frame_producer_device(format_desc, safe_ptr<frame_factory>(mixer_), std::bind(&frame_mixer_device::send, mixer_.get(), std::placeholders::_1)))	{}

	~implementation()
	{
		// Shutdown order is important! Destroy all created frames to mixer before destroying mixer.
		CASPAR_LOG(info) << "Shutting down channel.";
		producer_.reset();
		CASPAR_LOG(info) << "Successfully shut down producer-device.";
		consumer_.reset();
		CASPAR_LOG(info) << "Successfully shut down consumer-device.";
		mixer_.reset();
		CASPAR_LOG(info) << "Successfully shut down mixer-device.";
	}
};

channel::channel(channel&& other) : impl_(std::move(other.impl_)){}
channel::channel(const video_format_desc& format_desc) : impl_(new implementation(format_desc)){}
frame_producer_device& channel::producer() { return *impl_->producer_;} 
frame_consumer_device& channel::consumer() { return *impl_->consumer_;} 
const video_format_desc& channel::get_video_format_desc() const{return impl_->format_desc_;}

}}