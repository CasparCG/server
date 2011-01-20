#include "../../StdAfx.h"

#include "write_frame.h"

#include "draw_frame.h"
#include "pixel_format.h"

#include "../image/image_mixer.h"
#include "../audio/audio_mixer.h"

#include "../gpu/host_buffer.h"

#include <common/gl/gl_check.h>

#include <boost/range/algorithm.hpp>

namespace caspar { namespace core {
																																							
struct write_frame::implementation : boost::noncopyable
{				
	write_frame& self_;
	std::vector<safe_ptr<host_buffer>> buffers_;
	std::vector<short> audio_data_;
	const pixel_format_desc desc_;

public:
	implementation(write_frame& self, const pixel_format_desc& desc, std::vector<safe_ptr<host_buffer>> buffers) 
		: self_(self)
		, desc_(desc)
		, buffers_(buffers){}
	
	void process_image(image_mixer& mixer)
	{
		mixer.begin(self_.get_image_transform());
		mixer.process(desc_, buffers_);
		mixer.end();
	}

	void process_audio(audio_mixer& mixer)
	{
		mixer.begin(self_.get_audio_transform());
		mixer.process(audio_data_);
		mixer.end();
	}

	boost::iterator_range<unsigned char*> image_data(size_t index)
	{
		if(index >= buffers_.size() || !buffers_[index]->data())
			return boost::iterator_range<const unsigned char*>();
		auto ptr = static_cast<unsigned char*>(buffers_[index]->data());
		return boost::iterator_range<unsigned char*>(ptr, ptr+buffers_[index]->size());
	}
	const boost::iterator_range<const unsigned char*> image_data(size_t index) const
	{
		if(index >= buffers_.size() || !buffers_[index]->data())
			return boost::iterator_range<const unsigned char*>();
		auto ptr = static_cast<const unsigned char*>(buffers_[index]->data());
		return boost::iterator_range<const unsigned char*>(ptr, ptr+buffers_[index]->size());
	}
};

#ifdef _MSC_VER
#pragma warning(disable : 4355) // 'this' : used in base member initializer list
#endif
	
write_frame::write_frame(const pixel_format_desc& desc, std::vector<safe_ptr<host_buffer>> buffers) : impl_(new implementation(*this, desc, buffers)){}
write_frame::write_frame(write_frame&& other) : impl_(std::move(other.impl_)){}
void write_frame::swap(write_frame& other){impl_.swap(other.impl_);}
write_frame& write_frame::operator=(write_frame&& other)
{
	write_frame temp(std::move(other));
	temp.swap(*this);
	return *this;
}
void write_frame::process_image(image_mixer& mixer){impl_->process_image(mixer);}
void write_frame::process_audio(audio_mixer& mixer){impl_->process_audio(mixer);}
boost::iterator_range<unsigned char*> write_frame::image_data(size_t index){return impl_->image_data(index);}
std::vector<short>& write_frame::audio_data() { return impl_->audio_data_; }
}}