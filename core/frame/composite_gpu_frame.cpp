#include "../StdAfx.h"

#include "composite_gpu_frame.h"
#include "../../common/gl/utility.h"
#include "../../common/utility/memory.h"

#include <algorithm>
#include <numeric>

namespace caspar { namespace core {
	
struct composite_gpu_frame::implementation : boost::noncopyable
{
	implementation(composite_gpu_frame* self) : self_(self){}

	void write_lock()
	{
		std::for_each(frames_.begin(), frames_.end(), std::mem_fn(&gpu_frame::write_lock));		
	}

	bool write_unlock()
	{
		return std::all_of(frames_.begin(), frames_.end(), std::mem_fn(&gpu_frame::write_unlock));			
	}
	
	void read_lock(GLenum mode)
	{	
		std::for_each(frames_.begin(), frames_.end(), std::bind(&gpu_frame::read_lock, std::placeholders::_1, mode));		
	}

	bool read_unlock()
	{
		return std::all_of(frames_.begin(), frames_.end(), std::mem_fn(&gpu_frame::read_unlock));		
	}

	void draw()
	{
		glPushMatrix();
		glTranslated(self_->x()*2.0, self_->y()*2.0, 0.0);
		std::for_each(frames_.begin(), frames_.end(), std::mem_fn(&gpu_frame::draw));
		glPopMatrix();
	}
		
	void add(const gpu_frame_ptr& frame)
	{
		frames_.push_back(frame);

		if(self_->audio_data().empty())
			self_->audio_data() = std::move(frame->audio_data());
		else
		{
			for(size_t n = 0; n < frame->audio_data().size(); ++n)
				self_->audio_data()[n] = static_cast<short>(static_cast<int>(self_->audio_data()[n]) + static_cast<int>(frame->audio_data()[n]) & 0xFFFF);				
		}
	}

	unsigned char* data()
	{
		BOOST_THROW_EXCEPTION(invalid_operation());
	}

	composite_gpu_frame* self_;
	std::vector<gpu_frame_ptr> frames_;
	size_t size_;
};

#pragma warning (disable : 4355)

composite_gpu_frame::composite_gpu_frame(size_t width, size_t height) : gpu_frame(width, height), impl_(new implementation(this)){}
void composite_gpu_frame::write_lock(){impl_->write_lock();}
bool composite_gpu_frame::write_unlock(){return impl_->write_unlock();}	
void composite_gpu_frame::read_lock(GLenum mode){impl_->read_lock(mode);}
bool composite_gpu_frame::read_unlock(){return impl_->read_unlock();}
void composite_gpu_frame::draw(){impl_->draw();}
unsigned char* composite_gpu_frame::data(){return impl_->data();}
void composite_gpu_frame::add(const gpu_frame_ptr& frame){impl_->add(frame);}

gpu_frame_ptr composite_gpu_frame::interlace(const gpu_frame_ptr& frame1 ,const gpu_frame_ptr& frame2, video_mode mode)
{			
	auto result = std::make_shared<composite_gpu_frame>(frame1->width(), frame1->height());
	result->add(frame1);
	result->add(frame2);
	if(mode == video_mode::upper)
	{
		frame1->mode(video_mode::upper);
		frame2->mode(video_mode::lower);
	}
	else
	{
		frame1->mode(video_mode::lower);
		frame2->mode(video_mode::upper);
	}
	return result;
}

}}