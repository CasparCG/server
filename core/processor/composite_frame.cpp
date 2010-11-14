#include "../StdAfx.h"

#include "composite_frame.h"
#include "../../common/gl/utility.h"
#include "../../common/utility/memory.h"

#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm_ext/erase.hpp>

#include <algorithm>
#include <numeric>

#include <tbb/parallel_for.h>

namespace caspar { namespace core {
	
struct composite_frame::implementation : boost::noncopyable
{
	implementation(composite_frame* self) : self_(self){}
	
	implementation(composite_frame* self, const std::vector<frame_ptr>& frames) : self_(self), frames_(frames)
	{
		boost::range::remove_erase(frames_, nullptr);
		boost::range::remove_erase(frames_, frame::empty());
		boost::for_each(frames_, [&](const frame_ptr& frame)
		{
			if(self_->audio_data().empty())
				self_->audio_data() = std::move(frame->audio_data());
			else
			{
				tbb::parallel_for
				(
					tbb::blocked_range<size_t>(0, frame->audio_data().size()),
					[&](const tbb::blocked_range<size_t>& r)
					{
						for(size_t n = r.begin(); n < r.end(); ++n)
							self_->audio_data()[n] = static_cast<short>(	static_cast<int>(self_->audio_data()[n]) + static_cast<int>(frame->audio_data()[n]) & 0xFFFF);	
					}
				);
			}
		});
	}

	void begin_write()
	{
		boost::range::for_each(frames_, std::mem_fn(&frame::begin_write));		
	}

	void end_write()
	{
		boost::range::for_each(frames_, std::mem_fn(&frame::end_write));				
	}
	
	void begin_read()
	{	
		boost::range::for_each(frames_, std::mem_fn(&frame::begin_read));		
	}

	void end_read()
	{
		boost::range::for_each(frames_, std::mem_fn(&frame::end_read));	
	}

	void draw(const frame_shader_ptr& shader)
	{
		glPushMatrix();
		glTranslated(self_->x()*2.0, self_->y()*2.0, 0.0);
		boost::range::for_each(frames_, std::bind(&frame::draw, std::placeholders::_1, shader));
		glPopMatrix();
	}

	unsigned char* data(size_t)
	{
		BOOST_THROW_EXCEPTION(invalid_operation());
	}

	composite_frame* self_;
	std::vector<frame_ptr> frames_;
	size_t size_;
};

#if defined(_MSC_VER)
#pragma warning (disable : 4355) // 'this' : used in base member initializer list
#endif

composite_frame::composite_frame(const std::vector<frame_ptr>& frames) 
	: frame(0, 0), impl_(new implementation(this, frames)){}
void composite_frame::begin_write(){impl_->begin_write();}
void composite_frame::end_write(){impl_->end_write();}	
void composite_frame::begin_read(){impl_->begin_read();}
void composite_frame::end_read(){impl_->end_read();}
void composite_frame::draw(const frame_shader_ptr& shader){impl_->draw(shader);}
unsigned char* composite_frame::data(size_t index){return impl_->data(index);}

frame_ptr composite_frame::interlace(const frame_ptr& frame1, const frame_ptr& frame2, video_update_format::type mode)
{			
	std::vector<frame_ptr> frames;
	frames.push_back(frame1);
	frames.push_back(frame2);
	auto result = std::make_shared<composite_frame>(frames);
	if(mode == video_update_format::upper)
	{
		frame1->update_fmt(video_update_format::upper);
		frame2->update_fmt(video_update_format::lower);
	}
	else
	{
		frame1->update_fmt(video_update_format::lower);
		frame2->update_fmt(video_update_format::upper);
	}
	return result;
}

}}