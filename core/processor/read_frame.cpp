#include "../StdAfx.h"

#include "read_frame.h"
#include "../format/pixel_format.h"
#include "../../common/gl/utility.h"
#include "../../common/gl/pixel_buffer_object.h"
#include "../../common/utility/singleton_pool.h"

#include <boost/range/algorithm.hpp>

namespace caspar { namespace core {
																																							
struct read_frame::implementation : boost::noncopyable
{
	implementation(size_t width, size_t height) : pbo_(width, height, GL_BGRA)
	{
		CASPAR_LOG(trace) << "Allocated read_frame.";
	}				

	const boost::iterator_range<const unsigned char*> pixel_data() const
	{
		if(!pbo_.data())
			return boost::iterator_range<const unsigned char*>();

		auto ptr = static_cast<const unsigned char*>(pbo_.data());
		return boost::iterator_range<const unsigned char*>(ptr, ptr+pbo_.size());
	}

	gl::pbo pbo_;
	std::vector<short> audio_data_;
};

read_frame::read_frame(size_t width, size_t height) : impl_(singleton_pool<implementation>::make_shared(width, height)){}
const boost::iterator_range<const unsigned char*> read_frame::pixel_data() const{return impl_->pixel_data();}
const std::vector<short>& read_frame::audio_data() const { return impl_->audio_data_; }
void read_frame::audio_data(const std::vector<short>& audio_data) { impl_->audio_data_ = audio_data; }
void read_frame::begin_read(){impl_->pbo_.begin_read();}
void read_frame::end_read(){impl_->pbo_.end_read();}

}}