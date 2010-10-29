#include "../StdAfx.h"

#include "composite_gpu_frame.h"
#include "../../common/gl/utility.h"
#include "../../common/utility/memory.h"

#include <algorithm>
#include <numeric>

namespace caspar {
	
struct composite_gpu_frame::implementation
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

		if(audio_data_.empty())
			audio_data_ = std::move(frame->audio_data());
		else
		{
			for(size_t n = 0; n < frame->audio_data().size(); ++n)
				audio_data_[n] = static_cast<short>(static_cast<int>(audio_data_[n]) + static_cast<int>(frame->audio_data()[n]) & 0xFFFF);				
		}
	}

	unsigned char* data()
	{
		BOOST_THROW_EXCEPTION(invalid_operation());
	}

	composite_gpu_frame* self_;
	std::vector<gpu_frame_ptr> frames_;
	size_t size_;
	std::vector<short> audio_data_;
};

#pragma warning (disable : 4355)

composite_gpu_frame::composite_gpu_frame(size_t width, size_t height) : gpu_frame(width, height), impl_(new implementation(this)){}
void composite_gpu_frame::write_lock(){impl_->write_lock();}
bool composite_gpu_frame::write_unlock(){return impl_->write_unlock();}	
void composite_gpu_frame::read_lock(GLenum mode){impl_->read_lock(mode);}
bool composite_gpu_frame::read_unlock(){return impl_->read_unlock();}
void composite_gpu_frame::draw(){impl_->draw();}
unsigned char* composite_gpu_frame::data(){return impl_->data();}
const std::vector<short>& composite_gpu_frame::audio_data() const { return impl_->audio_data_; }	
std::vector<short>& composite_gpu_frame::audio_data() { return impl_->audio_data_; }
void composite_gpu_frame::reset(){impl_->audio_data_.clear();}
void composite_gpu_frame::add(const gpu_frame_ptr& frame){impl_->add(frame);}

}