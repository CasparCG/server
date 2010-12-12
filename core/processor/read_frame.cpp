#include "../StdAfx.h"

#include "read_frame.h"
#include "../format/pixel_format.h"
#include "../../common/gl/utility.h"
#include "../../common/gl/pixel_buffer_object.h"

#include <boost/range/algorithm.hpp>

namespace caspar { namespace core {
																																							
struct read_frame::implementation : boost::noncopyable
{
	implementation(common::gl::pbo_ptr&& pbo, std::vector<short>&& audio_data) : pbo_(std::move(pbo)), audio_data_(std::move(audio_data)){}				
	common::gl::pbo_ptr pbo_;
	std::vector<short> audio_data_;
};
	
read_frame::read_frame(common::gl::pbo_ptr&& pbo, std::vector<short>&& audio_data) : impl_(new implementation(std::move(pbo), std::move(audio_data))){}
const boost::iterator_range<const unsigned char*> read_frame::pixel_data() const
{
	if(!impl_->pbo_ || !impl_->pbo_->data())
		return boost::iterator_range<const unsigned char*>();

	auto ptr = static_cast<const unsigned char*>(impl_->pbo_->data());
	return boost::iterator_range<const unsigned char*>(ptr, ptr+impl_->pbo_->size());
}
const std::vector<short>& read_frame::audio_data() const { return impl_->audio_data_; }

}}