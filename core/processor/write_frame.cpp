#include "../StdAfx.h"

#include "write_frame.h"

#include "draw_frame.h"
#include "image_processor.h"
#include "audio_processor.h"

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
	
	void unmap()
	{
		boost::range::for_each(pbos_, std::mem_fn(&gl::pbo::unmap_write));
	}

	void map()
	{
		boost::range::for_each(pbos_, std::mem_fn(&gl::pbo::map_write));
	}

	void process_image(image_processor& processor)
	{
		processor.process(desc_, pbos_);
	}

	void process_audio(audio_processor& processor)
	{
		processor.process(audio_data_);
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
void write_frame::map(){impl_->map();}	
void write_frame::unmap(){impl_->unmap();}	
void write_frame::process_image(image_processor& processor){impl_->process_image(processor);}
void write_frame::process_audio(audio_processor& processor){impl_->process_audio(processor);}
boost::iterator_range<unsigned char*> write_frame::pixel_data(size_t index){return impl_->pixel_data(index);}
const boost::iterator_range<const unsigned char*> write_frame::pixel_data(size_t index) const {return impl_->pixel_data(index);}
std::vector<short>& write_frame::audio_data() { return impl_->audio_data_; }
}}