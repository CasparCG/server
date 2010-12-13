#include "../StdAfx.h"

#include "draw_frame.h"
#include "composite_frame.h"
#include "transform_frame.h"
#include "frame_shader.h"
#include "../../common/utility/singleton_pool.h"

#include <boost/range/algorithm.hpp>

#include <tbb/parallel_for.h>

namespace caspar { namespace core {
	
struct composite_frame::implementation
{	
	implementation(std::vector<draw_frame>&& frames) : frames_(std::move(frames))
	{		
		boost::range::for_each(frames_, [&](const draw_frame& frame)
		{
			if(audio_data_.empty())
				audio_data_ = frame.audio_data();
			else
			{
				tbb::parallel_for
				(
					tbb::blocked_range<size_t>(0, frame.audio_data().size()),
					[&](const tbb::blocked_range<size_t>& r)
					{
						for(size_t n = r.begin(); n < r.end(); ++n)					
							audio_data_[n] = static_cast<short>((static_cast<int>(audio_data_[n]) + static_cast<int>(frame.audio_data()[n])) & 0xFFFF);						
					}
				);
			}
		});
	}
	
	void begin_write()
	{
		boost::range::for_each(frames_, std::bind(&detail::draw_frame_access::begin_write, std::placeholders::_1));
	}

	void end_write()
	{
		boost::range::for_each(frames_, std::bind(&detail::draw_frame_access::end_write, std::placeholders::_1));
	}
	
	void draw(frame_shader& shader)
	{
		boost::range::for_each(frames_, std::bind(&detail::draw_frame_access::draw, std::placeholders::_1, std::ref(shader)));
	}
				
	std::vector<short> audio_data_;
	std::vector<draw_frame> frames_;
};

composite_frame::composite_frame(std::vector<draw_frame>&& frames) : impl_(common::singleton_pool<implementation>::make_shared(std::move(frames))){}
composite_frame::composite_frame(composite_frame&& other) : impl_(std::move(other.impl_)){}
composite_frame::composite_frame(const composite_frame& other) : impl_(common::singleton_pool<implementation>::make_shared(*other.impl_)){}
composite_frame& composite_frame::operator=(const composite_frame& other)
{
	composite_frame temp(other);
	temp.impl_.swap(impl_);
	return *this;
}
composite_frame& composite_frame::operator=(composite_frame&& other)
{
	impl_ = std::move(other.impl_);
	return *this;
}

composite_frame::composite_frame(draw_frame&& frame1, draw_frame&& frame2)
{
	std::vector<draw_frame> frames;
	frames.push_back(std::move(frame1));
	frames.push_back(std::move(frame2));
	impl_.reset(new implementation(std::move(frames)));
}

void composite_frame::begin_write(){impl_->begin_write();}
void composite_frame::end_write(){impl_->end_write();}	
void composite_frame::draw(frame_shader& shader){impl_->draw(shader);}
const std::vector<short>& composite_frame::audio_data() const {return impl_->audio_data_;}

composite_frame composite_frame::interlace(draw_frame&& frame1, draw_frame&& frame2, video_mode::type mode)
{			
	transform_frame my_frame1 = std::move(frame1);
	transform_frame my_frame2 = std::move(frame2);
	if(mode == video_mode::upper)
	{
		my_frame1.video_mode(video_mode::upper);
		my_frame2.video_mode(video_mode::lower);
	}
	else
	{
		my_frame1.video_mode(video_mode::lower);
		my_frame2.video_mode(video_mode::upper);
	}

	std::vector<draw_frame> frames;
	frames.push_back(std::move(my_frame1));
	frames.push_back(std::move(my_frame2));
	return composite_frame(std::move(frames));
}

}}