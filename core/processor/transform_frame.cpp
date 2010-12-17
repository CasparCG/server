#include "../StdAfx.h"

#include "transform_frame.h"

#include "draw_frame.h"
#include "image_processor.h"
#include "audio_processor.h"

#include "../format/pixel_format.h"
#include "../../common/gl/utility.h"
#include "../../common/gl/pixel_buffer_object.h"
#include "../../common/utility/singleton_pool.h"

#include <boost/range/algorithm.hpp>

#include <tbb/parallel_for.h>

namespace caspar { namespace core {
																																						
struct transform_frame::implementation
{
	implementation(const safe_ptr<draw_frame>& frame) : frame_(frame){}
	implementation(safe_ptr<draw_frame>&& frame) : frame_(std::move(frame)){}
	
	void process_image(image_processor& processor)
	{
		processor.begin(image_transform_);
		frame_->process_image(processor);
		processor.end();
	}

	void process_audio(audio_processor& processor)
	{
		processor.begin(audio_transform_);
		frame_->process_audio(processor);
		processor.end();
	}
		
	safe_ptr<draw_frame> frame_;
	std::vector<short> audio_data_;
	std::vector<short> override_audio_data_;
	image_transform image_transform_;	
	audio_transform audio_transform_;	
};
	
transform_frame::transform_frame() : impl_(singleton_pool<implementation>::make_shared(draw_frame::empty())){}
transform_frame::transform_frame(const safe_ptr<draw_frame>& frame) : impl_(singleton_pool<implementation>::make_shared(frame)){}
transform_frame::transform_frame(safe_ptr<draw_frame>&& frame) : impl_(singleton_pool<implementation>::make_shared(std::move(frame))){}
transform_frame::transform_frame(const transform_frame& other) : impl_(singleton_pool<implementation>::make_shared(*other.impl_)){}
void transform_frame::swap(transform_frame& other){impl_.swap(other.impl_);}
transform_frame& transform_frame::operator=(const transform_frame& other)
{
	transform_frame temp(other);
	temp.swap(*this);
	return *this;
}
transform_frame::transform_frame(transform_frame&& other) : impl_(std::move(other.impl_)){}
transform_frame& transform_frame::operator=(transform_frame&& other)
{
	transform_frame temp(std::move(other));
	temp.swap(*this);
	return *this;
}
void transform_frame::process_image(image_processor& processor){impl_->process_image(processor);}
void transform_frame::process_audio(audio_processor& processor){impl_->process_audio(processor);}
void transform_frame::audio_volume(double volume){impl_->audio_transform_.volume = volume;}
void transform_frame::translate(double x, double y){impl_->image_transform_.pos = boost::make_tuple(x, y);}
void transform_frame::texcoord(double left, double top, double right, double bottom){impl_->image_transform_.uv = boost::make_tuple(left, top, right, bottom);}
void transform_frame::video_mode(video_mode::type mode){impl_->image_transform_.mode = mode;}
void transform_frame::alpha(double value){impl_->image_transform_.alpha = value;}
}}