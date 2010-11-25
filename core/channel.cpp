#include "StdAfx.h"

#include "channel.h"

#include <boost/noncopyable.hpp>

#include <memory>

namespace caspar { namespace core {

struct channel::implementation : boost::noncopyable
{
public:
	implementation(const frame_producer_device_ptr& producer_device, const frame_processor_device_ptr& processor_device, const frame_consumer_device_ptr& consumer_device)
		: producer_device_(producer_device), processor_device_(processor_device), consumer_device_(consumer_device)
	{
	}

	~implementation()
	{
		producer_device_->clear();
	}
	
	void load(int render_layer, const frame_producer_ptr& producer, load_option::type option = load_option::none)
	{
		producer_device_->load(render_layer, producer, option);
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
	
	frame_producer_ptr foreground(int render_layer) const
	{
		return producer_device_->foreground(render_layer);
	}

	frame_producer_ptr background(int render_layer) const
	{
		return producer_device_->background(render_layer);
	}

	const video_format_desc& get_video_format_desc() const
	{
		return processor_device_->get_video_format_desc();
	}

private:
	const frame_processor_device_ptr processor_device_; // Destroyed last inorder to have all frames returned to their pools.
	const frame_producer_device_ptr producer_device_;
	const frame_consumer_device_ptr consumer_device_;
};

channel::channel(const frame_producer_device_ptr& producer_device, const frame_processor_device_ptr& processor_device, const frame_consumer_device_ptr& consumer_device)
	: impl_(new implementation(producer_device, processor_device, consumer_device)){}
void channel::load(int render_layer, const frame_producer_ptr& producer, load_option::type option){impl_->load(render_layer, producer, option);}
void channel::pause(int render_layer){impl_->pause(render_layer);}
void channel::play(int render_layer){impl_->play(render_layer);}
void channel::stop(int render_layer){impl_->stop(render_layer);}
void channel::clear(int render_layer){impl_->clear(render_layer);}
void channel::clear(){impl_->clear();}
frame_producer_ptr channel::foreground(int render_layer) const{	return impl_->foreground(render_layer);}
frame_producer_ptr channel::background(int render_layer) const{return impl_->background(render_layer);}
const video_format_desc& channel::get_video_format_desc() const{	return impl_->get_video_format_desc();}

}}