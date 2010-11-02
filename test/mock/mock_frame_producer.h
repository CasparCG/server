#pragma once

#include <core/frame/gpu_frame.h>
#include <core/producer/frame_producer.h>
#include <common/exception/exceptions.h>

using namespace caspar;
using namespace caspar::core;

class mock_frame_producer : public frame_producer
{
public:
	mock_frame_producer(bool null = false, bool throws = false) 
		: null_(null), throws_(throws){}
	gpu_frame_ptr get_frame()
	{ 
		if(throws_)
			BOOST_THROW_EXCEPTION(caspar_exception());
		if(leading_)
			return leading_->get_frame();
		if(!null_)
			return std::make_shared<gpu_frame>(0, 0);
		return nullptr;
	}
	std::shared_ptr<frame_producer> get_following_producer() const 
	{ return following_; }
	void set_leading_producer(const std::shared_ptr<frame_producer>& leading) 
	{ leading_ = leading; }
	const frame_format_desc& get_frame_format_desc() const
	{ 
		static frame_format_desc format;
		return format;
	}
	void initialize(const frame_factory_ptr& factory)
	{}
	void set_following_producer(const std::shared_ptr<frame_producer>& following)
	{following_ = following;}
private:
	std::shared_ptr<frame_producer> following_;
	std::shared_ptr<frame_producer> leading_;
	bool null_;
	bool throws_;
};