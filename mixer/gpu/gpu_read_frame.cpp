#include "../stdafx.h"

#include "gpu_read_frame.h"

#include "../gpu/host_buffer.h"

#include <common/gl/gl_check.h>

namespace caspar { namespace mixer {
																																							
struct gpu_read_frame::implementation : boost::noncopyable
{
	safe_ptr<const host_buffer> image_data_;
	std::vector<short> audio_data_;

public:
	implementation(safe_ptr<const host_buffer>&& image_data, std::vector<short>&& audio_data) 
		: image_data_(std::move(image_data))
		, audio_data_(std::move(audio_data)){}	
};

gpu_read_frame::gpu_read_frame(safe_ptr<const host_buffer>&& image_data, std::vector<short>&& audio_data) : impl_(new implementation(std::move(image_data), std::move(audio_data))){}

const boost::iterator_range<const unsigned char*> gpu_read_frame::image_data() const
{
	if(!impl_->image_data_->data())
		return boost::iterator_range<const unsigned char*>();
	auto ptr = static_cast<const unsigned char*>(impl_->image_data_->data());
	return boost::iterator_range<const unsigned char*>(ptr, ptr + impl_->image_data_->size());
}
const boost::iterator_range<const short*> gpu_read_frame::audio_data() const
{
	return boost::iterator_range<const short*>(impl_->audio_data_.data(), impl_->audio_data_.data() + impl_->audio_data_.size());
}

}}