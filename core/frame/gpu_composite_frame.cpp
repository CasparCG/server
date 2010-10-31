#include "../StdAfx.h"

#include "gpu_composite_frame.h"
#include "../../common/gl/utility.h"
#include "../../common/utility/memory.h"

#include <algorithm>
#include <numeric>

namespace caspar { namespace core {
	
struct gpu_composite_frame::implementation : boost::noncopyable
{
	implementation(gpu_composite_frame* self) : self_(self){}

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

	gpu_composite_frame* self_;
	std::vector<gpu_frame_ptr> frames_;
	size_t size_;
};

#pragma warning (disable : 4355)

gpu_composite_frame::gpu_composite_frame(size_t width, size_t height) : gpu_frame(width, height), impl_(new implementation(this)){}
void gpu_composite_frame::write_lock(){impl_->write_lock();}
bool gpu_composite_frame::write_unlock(){return impl_->write_unlock();}	
void gpu_composite_frame::read_lock(GLenum mode){impl_->read_lock(mode);}
bool gpu_composite_frame::read_unlock(){return impl_->read_unlock();}
void gpu_composite_frame::draw(){impl_->draw();}
unsigned char* gpu_composite_frame::data(){return impl_->data();}
void gpu_composite_frame::add(const gpu_frame_ptr& frame){impl_->add(frame);}

gpu_frame_ptr gpu_composite_frame::interlace(const gpu_frame_ptr& frame1 ,const gpu_frame_ptr& frame2, video_mode mode)
{			
	auto result = std::make_shared<gpu_composite_frame>(frame1->width(), frame1->height());
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