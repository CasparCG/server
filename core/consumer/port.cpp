#include "../StdAfx.h"

#include "port.h"

#include "frame_consumer.h"
#include "../frame/frame.h"

#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>

#include <future>

namespace caspar { namespace core {

struct port::impl
{
	int									index_;
	spl::shared_ptr<monitor::subject>	monitor_subject_ = spl::make_shared<monitor::subject>("/port/" + boost::lexical_cast<std::string>(index_));
	spl::shared_ptr<frame_consumer>		consumer_;
	int									channel_index_;
public:
	impl(int index, int channel_index, spl::shared_ptr<frame_consumer> consumer)
		: index_(index)
		, consumer_(std::move(consumer))
		, channel_index_(channel_index)
	{
		consumer_->monitor_output().attach_parent(monitor_subject_);
	}

	void change_channel_format(const core::video_format_desc& format_desc)
	{
		consumer_->initialize(format_desc, index_);
	}

	std::future<bool> send(const_frame frame)
	{
		*monitor_subject_ << monitor::message("/type") % consumer_->name();
		return consumer_->send(std::move(frame));
	}
	std::wstring print() const
	{
		return consumer_->print();
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

	spl::shared_ptr<const frame_consumer> consumer() const
	{
		return consumer_;
	}
};

port::port(int index, int channel_index, spl::shared_ptr<frame_consumer> consumer) : impl_(new impl(index, channel_index, std::move(consumer))){}
port::port(port&& other) : impl_(std::move(other.impl_)){}
port::~port(){}
port& port::operator=(port&& other){impl_ = std::move(other.impl_); return *this;}
std::future<bool> port::send(const_frame frame){return impl_->send(std::move(frame));}
monitor::subject& port::monitor_output() {return *impl_->monitor_subject_;}
void port::change_channel_format(const core::video_format_desc& format_desc){impl_->change_channel_format(format_desc);}
int port::buffer_depth() const{return impl_->buffer_depth();}
std::wstring port::print() const{ return impl_->print();}
bool port::has_synchronization_clock() const{return impl_->has_synchronization_clock();}
boost::property_tree::wptree port::info() const{return impl_->info();}
spl::shared_ptr<const frame_consumer> port::consumer() const { return impl_->consumer(); }
}}
