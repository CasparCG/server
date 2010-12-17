#include "../StdAfx.h"

#include "draw_frame.h"
#include "composite_frame.h"
#include "transform_frame.h"
#include "image_processor.h"
#include "../../common/utility/singleton_pool.h"

#include <boost/range/algorithm.hpp>

#include <tbb/parallel_for.h>

namespace caspar { namespace core {
	
struct composite_frame::implementation
{		
	implementation(const std::vector<safe_ptr<draw_frame>>& frames) : frames_(frames){}
			
	void process_image(image_processor& processor)
	{
		boost::range::for_each(frames_, std::bind(&draw_frame::process_image, std::placeholders::_1, std::ref(processor)));
	}

	void process_audio(audio_processor& processor)
	{
		boost::range::for_each(frames_, std::bind(&draw_frame::process_audio, std::placeholders::_1, std::ref(processor)));
	}
					
	std::vector<safe_ptr<draw_frame>> frames_;
};

composite_frame::composite_frame(const std::vector<safe_ptr<draw_frame>>& frames) : impl_(singleton_pool<implementation>::make_shared(frames)){}
composite_frame::composite_frame(composite_frame&& other) : impl_(std::move(other.impl_)){}
composite_frame::composite_frame(const composite_frame& other) : impl_(singleton_pool<implementation>::make_shared(*other.impl_)){}
void composite_frame::swap(composite_frame& other){impl_.swap(other.impl_);}
composite_frame& composite_frame::operator=(const composite_frame& other)
{
	composite_frame temp(other);
	temp.swap(*this);
	return *this;
}
composite_frame& composite_frame::operator=(composite_frame&& other)
{
	composite_frame temp(std::move(other));
	temp.swap(*this);
	return *this;
}

composite_frame::composite_frame(const safe_ptr<draw_frame>& frame1, const safe_ptr<draw_frame>& frame2)
{
	std::vector<safe_ptr<draw_frame>> frames;
	frames.push_back(frame1);
	frames.push_back(frame2);
	impl_ = singleton_pool<implementation>::make_shared(std::move(frames));
}

void composite_frame::process_image(image_processor& processor){impl_->process_image(processor);}
void composite_frame::process_audio(audio_processor& processor){impl_->process_audio(processor);}

safe_ptr<composite_frame> composite_frame::interlace(const safe_ptr<draw_frame>& frame1, const safe_ptr<draw_frame>& frame2, video_mode::type mode)
{			
	auto my_frame1 = make_safe<transform_frame>(frame1);
	auto my_frame2 = make_safe<transform_frame>(frame2);
	if(mode == video_mode::upper)
	{
		my_frame1->video_mode(video_mode::upper);	
		my_frame2->video_mode(video_mode::lower);	
	}
	else
	{
		my_frame1->video_mode(video_mode::lower);	
		my_frame2->video_mode(video_mode::upper);	
	}

	std::vector<safe_ptr<draw_frame>> frames;
	frames.push_back(my_frame1);
	frames.push_back(my_frame2);
	return make_safe<composite_frame>(frames);
}

}}