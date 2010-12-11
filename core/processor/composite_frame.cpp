#include "../StdAfx.h"

#include "producer_frame.h"
#include "composite_frame.h"
#include "transform_frame.h"
#include "../../common/gl/utility.h"

#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm_ext/erase.hpp>

#include <algorithm>
#include <numeric>

#include <tbb/parallel_for.h>

namespace caspar { namespace core {
	
struct composite_frame::implementation : boost::noncopyable
{	
	implementation(const std::vector<producer_frame>& frames) : frames_(frames)	{}
	
	void begin_write()
	{
		for(size_t n = 0; n < frames_.size(); ++n)
			frames_[n].begin_write();
	}

	void end_write()
	{
		for(size_t n = 0; n < frames_.size(); ++n)
			frames_[n].end_write();	
	}
	
	void draw(frame_shader& shader)
	{
		for(size_t n = 0; n < frames_.size(); ++n)
			frames_[n].draw(shader);
	}
			
	std::vector<short>& audio_data()
	{
		if(!audio_data_.empty() || frames_.empty())
			return audio_data_;
			
		audio_data_.resize(1920*2, 0);
				
		for(size_t n = 0; n < frames_.size(); ++n)
		{
			auto& frame = frames_[n];
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

		return audio_data_;
	}
	
	std::vector<producer_frame> frames_;
	std::vector<short> audio_data_;
};

#if defined(_MSC_VER)
#pragma warning (disable : 4355) // 'this' : used in base member initializer list
#endif

composite_frame::composite_frame(const std::vector<producer_frame>& frames) : impl_(new implementation(frames)){}
composite_frame::composite_frame(composite_frame&& other) : impl_(std::move(other.impl_)){}
composite_frame& composite_frame::operator=(composite_frame&& other)
{
	impl_ = std::move(other.impl_);
	return *this;
}

composite_frame::composite_frame(const producer_frame& frame1, const producer_frame& frame2)
{
	std::vector<producer_frame> frames;
	frames.push_back(frame1);
	frames.push_back(frame2);
	impl_.reset(new implementation(frames));
}

void composite_frame::begin_write(){impl_->begin_write();}
void composite_frame::end_write(){impl_->end_write();}	
void composite_frame::draw(frame_shader& shader){impl_->draw(shader);}
std::vector<short>& composite_frame::audio_data(){return impl_->audio_data();}
const std::vector<short>& composite_frame::audio_data() const{return impl_->audio_data();}

composite_frame composite_frame::interlace(producer_frame&& frame1, producer_frame&& frame2, video_mode::type mode)
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

	std::vector<producer_frame> frames;
	frames.push_back(std::move(my_frame1));
	frames.push_back(std::move(my_frame2));
	return composite_frame(frames);
}

}}