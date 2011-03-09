#include "StdAfx.h"

#include "channel.h"

#include "consumer/frame_consumer_device.h"
#include "mixer/frame_mixer_device.h"
#include "producer/frame_producer_device.h"

#include "mixer/frame/basic_frame.h"
#include "mixer/frame_mixer_device.h"

#include "producer/layer.h"

#include <common/concurrency/executor.h>
#include <common/utility/printer.h>

#include <boost/range/algorithm_ext/erase.hpp>

#include <tbb/parallel_for.h>

#include <memory>

#ifdef _MSC_VER
#pragma warning(disable : 4355)
#endif

namespace caspar { namespace core {

struct channel::implementation : boost::noncopyable
{					
	const int index_;
	const video_format_desc format_desc_;
	
	const std::shared_ptr<frame_mixer_device>	 mixer_;
	const std::shared_ptr<frame_consumer_device> consumer_;
	const std::shared_ptr<frame_producer_device> producer_;

	boost::signals2::scoped_connection mixer_connection_;
	boost::signals2::scoped_connection producer_connection_;

public:
	implementation(int index, const video_format_desc& format_desc)  
		: index_(index)
		, format_desc_(format_desc)
		, consumer_(new frame_consumer_device(std::bind(&implementation::print, this), format_desc))
		, mixer_(new frame_mixer_device(std::bind(&implementation::print, this), format_desc))
		, producer_(new frame_producer_device(std::bind(&implementation::print, this), safe_ptr<frame_factory>(mixer_)))	
	{
		mixer_connection_ = mixer_->connect([=](const safe_ptr<const read_frame>& frame){consumer_->send(frame);});
		producer_connection_ = producer_->connect([=](const std::vector<safe_ptr<basic_frame>>& frames){mixer_->send(frames);});
	}
		
	std::wstring print() const
	{
		return L"channel[" + boost::lexical_cast<std::wstring>(index_+1) + L"-" +  format_desc_.name + L"]";
	}
};

channel::channel(int index, const video_format_desc& format_desc) : impl_(new implementation(index, format_desc)){}
channel::channel(channel&& other) : impl_(std::move(other.impl_)){}
frame_producer_device& channel::producer() { return *impl_->producer_;} 
frame_mixer_device& channel::mixer() { return *impl_->mixer_;} 
frame_consumer_device& channel::consumer() { return *impl_->consumer_;} 
const video_format_desc& channel::get_video_format_desc() const{return impl_->format_desc_;}
std::wstring channel::print() const { return impl_->print();}

}}