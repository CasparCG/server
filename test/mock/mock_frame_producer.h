#pragma once

#include "mock_frame.h"

#include <core/producer/frame_producer.h>
#include <common/exception/exceptions.h>

using namespace caspar;
using namespace caspar::core;

class mock_frame_producer : public frame_producer
{
public:
	mock_frame_producer(bool null = false, bool throws = false) 
		: null_(null), throws_(throws), initialized_(false), volume_(100){}
	void set_volume(short volume) { volume_ = volume;}
	frame_ptr render_frame()
	{ 
		if(throws_)
			BOOST_THROW_EXCEPTION(caspar_exception());
		if(leading_)
			return leading_->render_frame();
		if(!null_)
			return std::make_shared<mock_frame>(this, volume_);
		return nullptr;
	}
	std::shared_ptr<frame_producer> get_following_producer() const 
	{ return following_; }
	void set_leading_producer(const std::shared_ptr<frame_producer>& leading) 
	{ leading_ = leading; }
	const video_format_desc& get_video_format_desc() const
	{ 
		static video_format_desc format;
		return format;
	}
	void initialize(const frame_processor_device_ptr& factory)
	{initialized_ = true;}
	void set_following_producer(const std::shared_ptr<frame_producer>& following)
	{following_ = following;}

	bool is_initialized() const { return initialized_;}
private:
	std::shared_ptr<frame_producer> following_;
	std::shared_ptr<frame_producer> leading_;
	bool null_;
	bool throws_;
	bool initialized_;
	short volume_;
};