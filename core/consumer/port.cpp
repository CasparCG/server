#include "../StdAfx.h"

#include "port.h"

#include "frame_consumer.h"
#include "../frame/frame.h"

namespace caspar { namespace core {

struct port::impl
{
	monitor::basic_subject				event_subject_;
	std::shared_ptr<frame_consumer>		consumer_;
	int									index_;
	int									channel_index_;
public:
	impl(int index, int channel_index, spl::shared_ptr<frame_consumer> consumer)
		: event_subject_(monitor::path("port") % index)
		, consumer_(std::move(consumer))
		, index_(index)
		, channel_index_(channel_index)
	{
		consumer_->subscribe(event_subject_);
	}
	
	void video_format_desc(const struct video_format_desc& format_desc)
	{
		consumer_->initialize(format_desc, channel_index_);
	}
		
	bool send(const_frame frame)
	{
		event_subject_ << monitor::event("type") % consumer_->name();
		return consumer_->send(std::move(frame));
	}
	
	int index() const
	{
		return index_;
	}

	int buffer_depth() const
	{
		return consumer_->buffer_depth();
	}

	bool has_synchronization_clock() const
	{
		return consumer_->has_synchronization_clock();
	}

	boost::property_tree::wptree info() const
	{
		return consumer_->info();
	}
};

port::port(int index, int channel_index, spl::shared_ptr<frame_consumer> consumer) : impl_(new impl(index, channel_index, std::move(consumer))){}
port::port(port&& other) : impl_(std::move(other.impl_)){}
port::~port(){}
port& port::operator=(port&& other){impl_ = std::move(other.impl_); return *this;}
bool port::send(const_frame frame){return impl_->send(std::move(frame));}	
void port::subscribe(const monitor::observable::observer_ptr& o){impl_->event_subject_.subscribe(o);}
void port::unsubscribe(const monitor::observable::observer_ptr& o){impl_->event_subject_.unsubscribe(o);}
void port::video_format_desc(const struct video_format_desc& format_desc){impl_->video_format_desc(format_desc);}
int port::buffer_depth() const{return impl_->buffer_depth();}
bool port::has_synchronization_clock() const{return impl_->has_synchronization_clock();}
boost::property_tree::wptree port::info() const{return impl_->info();}
}}