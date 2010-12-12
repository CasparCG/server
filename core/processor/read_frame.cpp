#include "../StdAfx.h"

#include "read_frame.h"
#include "../format/pixel_format.h"
#include "../../common/gl/utility.h"
#include "../../common/gl/pixel_buffer_object.h"

#include <boost/range/algorithm.hpp>

namespace caspar { namespace core {
																																							
struct read_frame_impl::implementation : boost::noncopyable
{
	implementation(size_t width, size_t height) : pixel_data_(nullptr), pbo_(width, height, GL_BGRA){}
		
	void begin_read()
	{	
		pixel_data_ = nullptr;
		pbo_.begin_read();
	}

	void end_read()
	{
		pixel_data_ = pbo_.end_read();
	}
		
	common::gl::pixel_buffer_object pbo_;
	void* pixel_data_;	
	std::vector<short> audio_data_;
};
	
read_frame_impl::read_frame_impl(size_t width, size_t height) : impl_(new implementation(width, height)){}
void read_frame_impl::begin_read(){impl_->begin_read();}
void read_frame_impl::end_read(){impl_->end_read();}
const boost::iterator_range<const unsigned char*> read_frame_impl::pixel_data() const
{
	if(impl_->pixel_data_ == nullptr)
		return boost::iterator_range<const unsigned char*>();

	auto ptr = static_cast<const unsigned char*>(impl_->pixel_data_);
	return boost::iterator_range<const unsigned char*>(ptr, ptr+impl_->pbo_.size());
}
const std::vector<short>& read_frame_impl::audio_data() const { return impl_->audio_data_; }
std::vector<short>& read_frame_impl::audio_data() { return impl_->audio_data_; }

read_frame::read_frame(read_frame_impl_ptr&& frame) : impl_(std::move(frame)){}
	
const boost::iterator_range<const unsigned char*> read_frame::pixel_data() const
{
	return impl_ ? impl_->pixel_data() : boost::iterator_range<const unsigned char*>();
}

const std::vector<short>& read_frame::audio_data() const 
{
	static std::vector<short> audio_data;
	return impl_ ? impl_->audio_data() : audio_data;
}
}}