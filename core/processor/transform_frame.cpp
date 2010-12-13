#include "../StdAfx.h"

#include "transform_frame.h"

#include "draw_frame.h"
#include "frame_shader.h"

#include "../format/pixel_format.h"
#include "../../common/gl/utility.h"
#include "../../common/gl/pixel_buffer_object.h"
#include "../../common/utility/singleton_pool.h"

#include <boost/range/algorithm.hpp>

#include <tbb/parallel_for.h>

namespace caspar { namespace core {
																																						
struct transform_frame::implementation
{
	implementation(const safe_ptr<draw_frame>& frame) : frame_(frame), audio_volume_(255){}
	implementation(const safe_ptr<draw_frame>& frame, std::vector<short>&& audio_data) : frame_(frame), audio_volume_(255), override_audio_data_(std::move(audio_data)){}
	implementation(safe_ptr<draw_frame>&& frame) : frame_(std::move(frame)), audio_volume_(255){}
	
	void begin_write(){frame_->begin_write();}
	void end_write(){frame_->end_write();}

	void draw(frame_shader& shader)
	{
		shader.begin(transform_);
		frame_->draw(shader);
		shader.end();
	}

	void audio_volume(const unsigned char volume)
	{
		if(volume == audio_volume_)
			return;
		
		audio_volume_ = volume;

		auto& source = override_audio_data_;
		if(override_audio_data_.empty())
			source = frame_->audio_data();

		audio_data_.resize(source.size());
		tbb::parallel_for
		(
			tbb::blocked_range<size_t>(0, audio_data_.size()),
			[&](const tbb::blocked_range<size_t>& r)
			{
				for(size_t n = r.begin(); n < r.end(); ++n)					
					audio_data_[n] = static_cast<short>((static_cast<int>(source[n])*volume)>>8);						
			}
		);
	}
		
	const std::vector<short>& audio_data() const 
	{
		return !audio_data_.empty() ? audio_data_ : (!override_audio_data_.empty() ? override_audio_data_ : frame_->audio_data());
	}
		
	unsigned char audio_volume_;
	safe_ptr<draw_frame> frame_;
	std::vector<short> audio_data_;
	std::vector<short> override_audio_data_;
	shader_transform transform_;	
};
	
transform_frame::transform_frame() : impl_(singleton_pool<implementation>::make_shared(draw_frame::empty())){}
transform_frame::transform_frame(const safe_ptr<draw_frame>& frame) : impl_(singleton_pool<implementation>::make_shared(frame)){}
transform_frame::transform_frame(const safe_ptr<draw_frame>& frame, std::vector<short>&& audio_data) : impl_(singleton_pool<implementation>::make_shared(frame, std::move(audio_data))){}
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
void transform_frame::begin_write(){impl_->begin_write();}
void transform_frame::end_write(){impl_->end_write();}	
void transform_frame::draw(frame_shader& shader){impl_->draw(shader);}
void transform_frame::audio_volume(unsigned char volume){impl_->audio_volume(volume);}
void transform_frame::translate(double x, double y){impl_->transform_.pos = boost::make_tuple(x, y);}
void transform_frame::texcoord(double left, double top, double right, double bottom){impl_->transform_.uv = boost::make_tuple(left, top, right, bottom);}
void transform_frame::video_mode(video_mode::type mode){impl_->transform_.mode = mode;}
void transform_frame::alpha(double value){impl_->transform_.alpha = value;}
const std::vector<short>& transform_frame::audio_data() const { return impl_->audio_data(); }
}}