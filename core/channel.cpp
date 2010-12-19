#include "StdAfx.h"

#include "channel.h"

#include <boost/noncopyable.hpp>

#include <memory>

namespace caspar { namespace core {

struct channel::implementation : boost::noncopyable
{
public:
	implementation(const safe_ptr<frame_producer_device>& producer_device, const safe_ptr<frame_processor_device>& processor_device, const safe_ptr<frame_consumer_device>& consumer_device)
		: producer_device_(producer_device), processor_device_(processor_device), consumer_device_(consumer_device)
	{
	}

	~implementation()
	{
		producer_device_->clear();
	}
	
	void load(int render_layer, const safe_ptr<frame_producer>& producer, bool autoplay)
	{
		producer_device_->load(render_layer, producer, autoplay);
	}

	void preview(int render_layer, const safe_ptr<frame_producer>& producer)
	{
		producer_device_->preview(render_layer, producer);
	}

	void pause(int render_layer)
	{
		producer_device_->pause(render_layer);
	}

	void play(int render_layer)
	{
		producer_device_->play(render_layer);
	}

	void stop(int render_layer)
	{
		producer_device_->stop(render_layer);
	}

	void clear(int render_layer)
	{
		producer_device_->clear(render_layer);
	}

	void clear()
	{
		producer_device_->clear();
	}
	
	boost::unique_future<safe_ptr<frame_producer>> foreground(int render_layer) const
	{
		return producer_device_->foreground(render_layer);
	}

	boost::unique_future<safe_ptr<frame_producer>> background(int render_layer) const
	{
		return producer_device_->background(render_layer);
	}

	const video_format_desc& get_video_format_desc() const
	{
		return processor_device_->get_video_format_desc();
	}

private:
	const safe_ptr<frame_processor_device> processor_device_; // Destroyed last inorder to have all frames returned to their pools.
	const safe_ptr<frame_producer_device>  producer_device_;
	const safe_ptr<frame_consumer_device>  consumer_device_;
};

channel::channel(channel&& other) : impl_(std::move(other.impl_)){}
channel::channel(const safe_ptr<frame_producer_device>& producer_device, const safe_ptr<frame_processor_device>& processor_device, const safe_ptr<frame_consumer_device>& consumer_device)
	: impl_(new implementation(producer_device, processor_device, consumer_device)){}
void channel::load(int render_layer, const safe_ptr<frame_producer>& producer, bool autoplay){impl_->load(render_layer, producer, autoplay);}
void channel::preview(int render_layer, const safe_ptr<frame_producer>& producer){impl_->preview(render_layer, producer);}
void channel::pause(int render_layer){impl_->pause(render_layer);}
void channel::play(int render_layer){impl_->play(render_layer);}
void channel::stop(int render_layer){impl_->stop(render_layer);}
void channel::clear(int render_layer){impl_->clear(render_layer);}
void channel::clear(){impl_->clear();}
boost::unique_future<safe_ptr<frame_producer>> channel::foreground(int render_layer) const{	return impl_->foreground(render_layer);}
boost::unique_future<safe_ptr<frame_producer>> channel::background(int render_layer) const{return impl_->background(render_layer);}
const video_format_desc& channel::get_video_format_desc() const{	return impl_->get_video_format_desc();}

}}