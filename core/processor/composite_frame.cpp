#include "../StdAfx.h"

#include "composite_frame.h"
#include "../../common/gl/utility.h"

#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm_ext/erase.hpp>

#include <algorithm>
#include <numeric>

#include <tbb/parallel_for.h>

namespace caspar { namespace core {
	
struct composite_frame::implementation : boost::noncopyable
{	
	implementation(composite_frame* self, const std::vector<frame_ptr>& frames) : self_(self)
	{
		boost::range::transform(frames, std::back_inserter(frames_), std::static_pointer_cast<internal_frame, frame>);
		prepare();
	}

	implementation(composite_frame* self, const frame_ptr& frame1, const frame_ptr& frame2) : self_(self)
	{
		frames_.push_back(std::static_pointer_cast<internal_frame>(frame1));
		frames_.push_back(std::static_pointer_cast<internal_frame>(frame2));
		prepare();
	}

	void prepare()
	{
		boost::range::remove_erase(frames_, nullptr);
		boost::range::remove_erase(frames_, frame::empty());
		boost::for_each(frames_, [&](const frame_ptr& frame)
		{
			if(self_->get_audio_data().empty())
				self_->get_audio_data() = std::move(frame->get_audio_data());
			else
			{
				tbb::parallel_for
				(
					tbb::blocked_range<size_t>(0, frame->get_audio_data().size()),
					[&](const tbb::blocked_range<size_t>& r)
					{
						for(size_t n = r.begin(); n < r.end(); ++n)
							self_->get_audio_data()[n] = static_cast<short>(static_cast<int>(self_->get_audio_data()[n]) + static_cast<int>(frame->get_audio_data()[n]) & 0xFFFF);	
					}
				);
			}
		});
	}

	void begin_write()
	{
		boost::range::for_each(frames_, std::mem_fn(&internal_frame::begin_write));		
	}

	void end_write()
	{
		boost::range::for_each(frames_, std::mem_fn(&internal_frame::end_write));				
	}
	
	void begin_read()
	{	
		boost::range::for_each(frames_, std::mem_fn(&internal_frame::begin_read));		
	}

	void end_read()
	{
		boost::range::for_each(frames_, std::mem_fn(&internal_frame::end_read));	
	}

	void draw(frame_shader& shader)
	{
		glPushMatrix();
		glTranslated(self_->get_render_transform().pos.get<0>()*2.0, self_->get_render_transform().pos.get<1>()*2.0, 0.0);
		boost::range::for_each(frames_, std::bind(&internal_frame::draw, std::placeholders::_1, shader));
		glPopMatrix();
	}
	
	composite_frame* self_;
	std::vector<internal_frame_ptr> frames_;
};

#if defined(_MSC_VER)
#pragma warning (disable : 4355) // 'this' : used in base member initializer list
#endif

composite_frame::composite_frame(const std::vector<frame_ptr>& frames) : internal_frame(pixel_format_desc()), impl_(new implementation(this, frames)){}
composite_frame::composite_frame(const frame_ptr& frame1, const frame_ptr& frame2) : internal_frame(pixel_format_desc()), impl_(new implementation(this, frame1, frame2)){}
void composite_frame::begin_write(){impl_->begin_write();}
void composite_frame::end_write(){impl_->end_write();}	
void composite_frame::begin_read(){impl_->begin_read();}
void composite_frame::end_read(){impl_->end_read();}
void composite_frame::draw(frame_shader& shader){impl_->draw(shader);}

frame_ptr composite_frame::interlace(const frame_ptr& frame1, const frame_ptr& frame2, video_mode::type mode)
{			
	auto result = std::make_shared<composite_frame>(frame1, frame2);
	if(mode == video_mode::upper)
	{
		frame1->get_render_transform().mode = video_mode::upper;
		frame2->get_render_transform().mode = video_mode::lower;
	}
	else
	{
		frame1->get_render_transform().mode = video_mode::lower;
		frame2->get_render_transform().mode = video_mode::upper;
	}
	return result;
}

}}