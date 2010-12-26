#include "../StdAfx.h"

#include "draw_frame.h"

#include "image_processor.h"
#include "audio_processor.h"

#include "../format/pixel_format.h"
#include "../../common/utility/singleton_pool.h"

#include <boost/range/algorithm.hpp>

namespace caspar { namespace core {
																																						
struct draw_frame::implementation
{
	implementation(const std::vector<safe_ptr<draw_frame>>& frames) : frames_(frames){}
	implementation(std::vector<safe_ptr<draw_frame>>&& frames) : frames_(std::move(frames)){}
	
	void process_image(image_processor& processor)
	{
		processor.begin(image_transform_);
		boost::range::for_each(frames_, std::bind(&draw_frame::process_image, std::placeholders::_1, std::ref(processor)));
		processor.end();
	}

	void process_audio(audio_processor& processor)
	{
		processor.begin(audio_transform_);
		boost::range::for_each(frames_, std::bind(&draw_frame::process_audio, std::placeholders::_1, std::ref(processor)));
		processor.end();
	}
		
	std::vector<safe_ptr<draw_frame>> frames_;

	image_transform image_transform_;	
	audio_transform audio_transform_;	
};
	
draw_frame::draw_frame() : impl_(singleton_pool<implementation>::make_shared(std::vector<safe_ptr<draw_frame>>())){}
draw_frame::draw_frame(const std::vector<safe_ptr<draw_frame>>& frames) : impl_(singleton_pool<implementation>::make_shared(frames)){}
draw_frame::draw_frame(std::vector<safe_ptr<draw_frame>>&& frames) : impl_(singleton_pool<implementation>::make_shared(std::move(frames))){}
draw_frame::draw_frame(const draw_frame& other) : impl_(singleton_pool<implementation>::make_shared(*other.impl_)){}
draw_frame::draw_frame(const safe_ptr<draw_frame>& frame)
{
	std::vector<safe_ptr<draw_frame>> frames;
	frames.push_back(frame);
	impl_ = singleton_pool<implementation>::make_shared(std::move(frames));
}
draw_frame::draw_frame(safe_ptr<draw_frame>&& frame)
{
	std::vector<safe_ptr<draw_frame>> frames;
	frames.push_back(std::move(frame));
	impl_ = singleton_pool<implementation>::make_shared(std::move(frames));
}
draw_frame::draw_frame(const safe_ptr<draw_frame>& frame1, const safe_ptr<draw_frame>& frame2)
{
	std::vector<safe_ptr<draw_frame>> frames;
	frames.push_back(frame1);
	frames.push_back(frame2);
	impl_ = singleton_pool<implementation>::make_shared(std::move(frames));
}
draw_frame::draw_frame(safe_ptr<draw_frame>&& frame1, safe_ptr<draw_frame>&& frame2)
{
	std::vector<safe_ptr<draw_frame>> frames;
	frames.push_back(std::move(frame1));
	frames.push_back(std::move(frame2));
	impl_ = singleton_pool<implementation>::make_shared(std::move(frames));
}

void draw_frame::swap(draw_frame& other){impl_.swap(other.impl_);}
draw_frame& draw_frame::operator=(const draw_frame& other)
{
	draw_frame temp(other);
	temp.swap(*this);
	return *this;
}
draw_frame::draw_frame(draw_frame&& other) : impl_(std::move(other.impl_)){}
draw_frame& draw_frame::operator=(draw_frame&& other)
{
	draw_frame temp(std::move(other));
	temp.swap(*this);
	return *this;
}
void draw_frame::process_image(image_processor& processor){impl_->process_image(processor);}
void draw_frame::process_audio(audio_processor& processor){impl_->process_audio(processor);}
void draw_frame::audio_volume(double volume){impl_->audio_transform_.volume = volume;}
void draw_frame::translate(double x, double y){impl_->image_transform_.pos = boost::make_tuple(x, y);}
void draw_frame::texcoord(double left, double top, double right, double bottom){impl_->image_transform_.uv = boost::make_tuple(left, top, right, bottom);}
void draw_frame::video_mode(video_mode::type mode){impl_->image_transform_.mode = mode;}
void draw_frame::alpha(double value){impl_->image_transform_.alpha = value;}

safe_ptr<draw_frame> draw_frame::interlace(const safe_ptr<draw_frame>& frame1, const safe_ptr<draw_frame>& frame2, video_mode::type mode)
{			
	auto my_frame1 = make_safe<draw_frame>(frame1);
	auto my_frame2 = make_safe<draw_frame>(frame2);
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
	return make_safe<draw_frame>(frames);
}

}}