#include "../StdAfx.h"

#include "write_frame.h"

#include "draw_frame.h"
#include "frame_shader.h"

#include "../format/pixel_format.h"
#include "../../common/gl/utility.h"
#include "../../common/gl/pixel_buffer_object.h"
#include "../../common/utility/singleton_pool.h"

#include <boost/range/algorithm.hpp>

namespace caspar { namespace core {
																																							
struct write_frame::implementation : boost::noncopyable
{
	implementation(const pixel_format_desc& desc) : desc_(desc)
	{
		CASPAR_LOG(trace) << "Allocated write_frame.";

		static GLenum mapping[] = {GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_BGR, GL_BGRA};
		std::transform(desc_.planes.begin(), desc_.planes.end(), std::back_inserter(pbos_), [&](const pixel_format_desc::plane& plane)
		{
			return gl::pbo(plane.width, plane.height, mapping[plane.channels-1]);
		});
		boost::range::for_each(pbos_, std::mem_fn(&gl::pbo::map_write));
	}
	
	void prepare()
	{
		boost::range::for_each(pbos_, std::mem_fn(&gl::pbo::unmap_write));
	}

	void draw(frame_shader& shader)
	{
		for(size_t n = 0; n < pbos_.size(); ++n)
		{
			glActiveTexture(GL_TEXTURE0+n);
			pbos_[n].bind_texture();
		}
		shader.render(desc_);
		boost::range::for_each(pbos_, std::mem_fn(&gl::pbo::map_write));
		audio_data_.clear();
	}

	boost::iterator_range<unsigned char*> pixel_data(size_t index)
	{
		if(index >= pbos_.size() || !pbos_[index].data())
			return boost::iterator_range<const unsigned char*>();
		auto ptr = static_cast<unsigned char*>(pbos_[index].data());
		return boost::iterator_range<unsigned char*>(ptr, ptr+pbos_[index].size());
	}
	const boost::iterator_range<const unsigned char*> pixel_data(size_t index) const
	{
		if(index >= pbos_.size() || !pbos_[index].data())
			return boost::iterator_range<const unsigned char*>();
		auto ptr = static_cast<const unsigned char*>(pbos_[index].data());
		return boost::iterator_range<const unsigned char*>(ptr, ptr+pbos_[index].size());
	}
				
	std::vector<gl::pbo> pbos_;
	std::vector<short> audio_data_;
	const pixel_format_desc desc_;
};
	
write_frame::write_frame(const pixel_format_desc& desc) : impl_(singleton_pool<implementation>::make_shared(desc)){}
write_frame::write_frame(write_frame&& other) : impl_(std::move(other.impl_)){}
void write_frame::swap(write_frame& other){impl_.swap(other.impl_);}
write_frame& write_frame::operator=(write_frame&& other)
{
	write_frame temp(std::move(other));
	temp.swap(*this);
	return *this;
}
void write_frame::prepare(){impl_->prepare();}	
void write_frame::draw(frame_shader& shader){impl_->draw(shader);}
boost::iterator_range<unsigned char*> write_frame::pixel_data(size_t index){return impl_->pixel_data(index);}
const boost::iterator_range<const unsigned char*> write_frame::pixel_data(size_t index) const {return impl_->pixel_data(index);}
std::vector<short>& write_frame::audio_data() { return impl_->audio_data_; }
const std::vector<short>& write_frame::audio_data() const { return impl_->audio_data_; }
}}