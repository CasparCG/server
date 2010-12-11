#include "../StdAfx.h"

#include "read_frame.h"
#include "../format/pixel_format.h"
#include "../../common/gl/utility.h"
#include "../../common/gl/pixel_buffer_object.h"

#include <boost/range/algorithm.hpp>

namespace caspar { namespace core {
																																							
struct read_frame::implementation : boost::noncopyable
{
	implementation(size_t width, size_t height) : pixel_data_(nullptr)
	{
		if(width > 2 || height > 2)
			pbo_.reset(new common::gl::pixel_buffer_object(width, height, GL_BGRA));
	}
		
	void begin_read()
	{	
		if(!pbo_)
			return;
		pixel_data_ = nullptr;
		pbo_->begin_read();
	}

	void end_read()
	{
		if(!pbo_)
			return;
		pixel_data_ = pbo_->end_read();
	}
		
	std::unique_ptr<common::gl::pixel_buffer_object> pbo_;
	void* pixel_data_;	
	std::vector<short> audio_data_;
};
	
read_frame::read_frame(size_t width, size_t height) : impl_(new implementation(width, height)){}
void read_frame::begin_read(){impl_->begin_read();}
void read_frame::end_read(){impl_->end_read();}
const boost::iterator_range<const unsigned char*> read_frame::pixel_data() const
{
	auto ptr = static_cast<const unsigned char*>(impl_->pixel_data_);
	return boost::iterator_range<const unsigned char*>(ptr, ptr+impl_->pbo_->size());
}
const std::vector<short>& read_frame::audio_data() const { return impl_->audio_data_; }
std::vector<short>& read_frame::audio_data() { return impl_->audio_data_; }
}}