#include "../StdAfx.h"

#include "transform_frame.h"

#include "producer_frame.h"
#include "frame_shader.h"

#include "../format/pixel_format.h"
#include "../../common/gl/utility.h"
#include "../../common/gl/pixel_buffer_object.h"

#include <boost/range/algorithm.hpp>

#include <tbb/parallel_for.h>

namespace caspar { namespace core {
																																						
struct transform_frame::implementation : boost::noncopyable
{
	implementation(const producer_frame& frame) : frame_(frame), audio_volume_(255), calculated_audio_volume_(audio_volume_){}
	
	void begin_write(){frame_.begin_write();}
	void end_write(){frame_.end_write();}

	void draw(frame_shader& shader)
	{
		if(frame_ == producer_frame::empty() || frame_ == producer_frame::eof())
			return;

		shader.begin(transform_);
		frame_.draw(shader);
		shader.end();
	}

	std::vector<short>& audio_data()
	{
		if(!audio_data_.empty() && calculated_audio_volume_ == audio_volume_)
			return audio_data_;
		
		calculated_audio_volume_ = audio_volume_;
		audio_data_ = frame_.audio_data();
		tbb::parallel_for
		(
			tbb::blocked_range<size_t>(0, audio_data_.size()),
			[&](const tbb::blocked_range<size_t>& r)
			{
				for(size_t n = r.begin(); n < r.end(); ++n)					
					audio_data_[n] = static_cast<short>((static_cast<int>(audio_data_[n])*audio_volume_)>>8);						
			}
		);
		
		return audio_data_;
	}
		
	std::vector<short> audio_data_;
	
	shader_transform transform_;
	
	unsigned char calculated_audio_volume_;
	unsigned char audio_volume_;
	producer_frame frame_;
};
	
transform_frame::transform_frame(const producer_frame& frame) : impl_(new implementation(frame)){}
void transform_frame::begin_write(){impl_->begin_write();}
void transform_frame::end_write(){impl_->end_write();}	
void transform_frame::draw(frame_shader& shader){impl_->draw(shader);}
void transform_frame::audio_volume(unsigned char volume){impl_->audio_volume_ = volume;}
void transform_frame::translate(double x, double y){impl_->transform_.pos = boost::make_tuple(x, y);}
void transform_frame::texcoord(double left, double top, double right, double bottom){impl_->transform_.uv = boost::make_tuple(left, top, right, bottom);}
void transform_frame::video_mode(video_mode::type mode){impl_->transform_.mode = mode;}
void transform_frame::alpha(double value){impl_->transform_.alpha = value;}
std::vector<short>& transform_frame::audio_data() { return impl_->audio_data(); }
const std::vector<short>& transform_frame::audio_data() const { return impl_->audio_data(); }
}}