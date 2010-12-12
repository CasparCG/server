#include "../StdAfx.h"

#include "write_frame.h"

#include "draw_frame.h"
#include "frame_shader.h"

#include "../format/pixel_format.h"
#include "../../common/gl/utility.h"
#include "../../common/gl/pixel_buffer_object.h"

#include <boost/range/algorithm.hpp>

namespace caspar { namespace core {
																																							
struct write_frame::implementation : boost::noncopyable
{
	implementation(std::vector<common::gl::pbo_ptr>&& pbos, const pixel_format_desc& desc) : pbos_(std::move(pbos)), desc_(desc){}
	
	void begin_write()
	{
		boost::range::for_each(pbos_, std::mem_fn(&common::gl::pbo::begin_write));
	}

	void end_write()
	{
		boost::range::for_each(pbos_, std::mem_fn(&common::gl::pbo::end_write));
	}

	void draw(frame_shader& shader)
	{
		for(size_t n = 0; n < pbos_.size(); ++n)
		{
			glActiveTexture(GL_TEXTURE0+n);
			pbos_[n]->bind_texture();
		}
		shader.render(desc_);
	}

	boost::iterator_range<unsigned char*> pixel_data(size_t index)
	{
		auto ptr = static_cast<unsigned char*>(pbos_[index]->data());
		return boost::iterator_range<unsigned char*>(ptr, ptr+pbos_[index]->size());
	}
	const boost::iterator_range<const unsigned char*> pixel_data(size_t index) const
	{
		auto ptr = static_cast<const unsigned char*>(pbos_[index]->data());
		return boost::iterator_range<const unsigned char*>(ptr, ptr+pbos_[index]->size());
	}
			
	std::vector<common::gl::pbo_ptr> pbos_;
	std::vector<short> audio_data_;
	const pixel_format_desc desc_;
};
	
write_frame::write_frame(std::vector<common::gl::pbo_ptr>&& pbos, const pixel_format_desc& desc) : impl_(new implementation(std::move(pbos), desc)){}
write_frame::write_frame(write_frame&& other) : impl_(std::move(other.impl_)){}
write_frame& write_frame::operator=(write_frame&& other)
{
	impl_ = std::move(other.impl_);
	return *this;
}
void write_frame::begin_write(){impl_->begin_write();}
void write_frame::end_write(){impl_->end_write();}	
void write_frame::draw(frame_shader& shader){impl_->draw(shader);}
boost::iterator_range<unsigned char*> write_frame::pixel_data(size_t index){return impl_->pixel_data(index);}
const boost::iterator_range<const unsigned char*> write_frame::pixel_data(size_t index) const {return impl_->pixel_data(index);}
std::vector<short>& write_frame::audio_data() { return impl_->audio_data_; }
const std::vector<short>& write_frame::audio_data() const { return impl_->audio_data_; }
}}