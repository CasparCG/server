#include "../stdafx.h"

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
	std::vector<safe_ptr<host_buffer>> buffers_;
	std::vector<short> audio_data_;
	const pixel_format_desc desc_;
	int tag_;

public:
	implementation(const pixel_format_desc& desc, std::vector<safe_ptr<host_buffer>> buffers) 
		: desc_(desc)
		, buffers_(buffers)
		, tag_(std::numeric_limits<int>::min()){}
	
	void accept(write_frame& self, frame_visitor& visitor)
	{
		visitor.begin(self);
		visitor.visit(self);
		visitor.end();
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
	
write_frame::write_frame(const pixel_format_desc& desc, std::vector<safe_ptr<host_buffer>> buffers) : impl_(new implementation(desc, buffers)){}
write_frame::write_frame(write_frame&& other) : impl_(std::move(other.impl_)){}
void write_frame::swap(write_frame& other){impl_.swap(other.impl_);}
write_frame& write_frame::operator=(write_frame&& other)
{
	write_frame temp(std::move(other));
	temp.swap(*this);
	return *this;
}
void write_frame::accept(frame_visitor& visitor){impl_->accept(*this, visitor);}
boost::iterator_range<unsigned char*> write_frame::image_data(size_t index){return impl_->image_data(index);}
std::vector<short>& write_frame::audio_data() { return impl_->audio_data_; }
boost::iterator_range<const unsigned char*>  write_frame::image_data(size_t index) const{return impl_->image_data(index);}
const std::vector<short>& write_frame::audio_data() const { return impl_->audio_data_; }
void write_frame::tag(int tag) { impl_->tag_ = tag;}
int write_frame::tag() const {return impl_->tag_;}
const pixel_format_desc& write_frame::get_pixel_format_desc() const{return impl_->desc_;}
std::vector<safe_ptr<host_buffer>>& write_frame::buffers(){return impl_->buffers_;}
}}