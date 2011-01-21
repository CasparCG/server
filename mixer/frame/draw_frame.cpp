#include "../stdafx.h"

#include "draw_frame.h"

#include "../image/image_transform.h"
#include "../image/image_mixer.h"
#include "../audio/audio_mixer.h"
#include "../audio/audio_transform.h"
#include "pixel_format.h"

#include <core/video_format.h>

#include <boost/range/algorithm.hpp>

namespace caspar { namespace core {
																																						
struct draw_frame::implementation
{		
	std::vector<safe_ptr<draw_frame>> frames_;

	image_transform image_transform_;	
	audio_transform audio_transform_;

	int index_;
public:
	implementation(const std::vector<safe_ptr<draw_frame>>& frames) 
		: frames_(frames)
		, index_(std::numeric_limits<int>::min()) {}
	implementation(std::vector<safe_ptr<draw_frame>>&& frames) 
		: frames_(std::move(frames))
		, index_(std::numeric_limits<int>::min()) {}
	
	void process_image(image_mixer& mixer)
	{
		mixer.begin(image_transform_);
		BOOST_FOREACH(auto frame, frames_)
			frame->process_image(mixer);
		mixer.end();
	}

	void process_audio(audio_mixer& mixer)
	{
		mixer.begin(audio_transform_);
		BOOST_FOREACH(auto frame, frames_)
			frame->process_audio(mixer);
		mixer.end();
	}	
};
	
draw_frame::draw_frame() : impl_(new implementation(std::vector<safe_ptr<draw_frame>>())){}
draw_frame::draw_frame(const std::vector<safe_ptr<draw_frame>>& frames) : impl_(new implementation(frames)){}
draw_frame::draw_frame(std::vector<safe_ptr<draw_frame>>&& frames) : impl_(new implementation(std::move(frames))){}
draw_frame::draw_frame(const draw_frame& other) : impl_(new implementation(*other.impl_)){}
draw_frame::draw_frame(const safe_ptr<draw_frame>& frame)
{
	std::vector<safe_ptr<draw_frame>> frames;
	frames.push_back(frame);
	impl_.reset(new implementation(std::move(frames)));
}
draw_frame::draw_frame(safe_ptr<draw_frame>&& frame)
{
	std::vector<safe_ptr<draw_frame>> frames;
	frames.push_back(std::move(frame));
	impl_.reset(new implementation(std::move(frames)));
}
draw_frame::draw_frame(const safe_ptr<draw_frame>& frame1, const safe_ptr<draw_frame>& frame2)
{
	std::vector<safe_ptr<draw_frame>> frames;
	frames.push_back(frame1);
	frames.push_back(frame2);
	impl_.reset(new implementation(std::move(frames)));
}
draw_frame::draw_frame(safe_ptr<draw_frame>&& frame1, safe_ptr<draw_frame>&& frame2)
{
	std::vector<safe_ptr<draw_frame>> frames;
	frames.push_back(std::move(frame1));
	frames.push_back(std::move(frame2));
	impl_.reset(new implementation(std::move(frames)));
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
void draw_frame::process_image(image_mixer& mixer){impl_->process_image(mixer);}
void draw_frame::process_audio(audio_mixer& mixer){impl_->process_audio(mixer);}

const image_transform& draw_frame::get_image_transform() const { return impl_->image_transform_;}
image_transform& draw_frame::get_image_transform() { return impl_->image_transform_;}
const audio_transform& draw_frame::get_audio_transform() const { return impl_->audio_transform_;}
audio_transform& draw_frame::get_audio_transform() { return impl_->audio_transform_;}

safe_ptr<draw_frame> draw_frame::interlace(const safe_ptr<draw_frame>& frame1, const safe_ptr<draw_frame>& frame2, video_mode::type mode)
{			
	if(frame1 == frame2 || mode == video_mode::progressive)
		return frame1;

	auto my_frame1 = make_safe<draw_frame>(frame1);
	auto my_frame2 = make_safe<draw_frame>(frame2);
	if(mode == video_mode::upper)
	{
		my_frame1->get_image_transform().set_mode(video_mode::upper);	
		my_frame2->get_image_transform().set_mode(video_mode::lower);	
	}											 
	else										 
	{											 
		my_frame1->get_image_transform().set_mode(video_mode::lower);	
		my_frame2->get_image_transform().set_mode(video_mode::upper);	
	}

	std::vector<safe_ptr<draw_frame>> frames;
	frames.push_back(my_frame1);
	frames.push_back(my_frame2);
	return make_safe<draw_frame>(frames);
}

void draw_frame::set_layer_index(int index) { impl_->index_ = index; }
int draw_frame::get_layer_index() const { return impl_->index_; }

}}