#include "../StdAfx.h"

#include "write_frame.h"

#include "frame_shader.h"

#include "../format/pixel_format.h"
#include "../../common/gl/utility.h"
#include "../../common/gl/pixel_buffer_object.h"

#include <boost/range/algorithm.hpp>

namespace caspar { namespace core {
																																							
struct write_frame::implementation : boost::noncopyable
{
	implementation(const pixel_format_desc& desc) : desc_(desc)
	{			
		assert(desc_.planes.size() <= 4);

		std::fill(pixel_data_.begin(), pixel_data_.end(), nullptr);

		for(size_t n = 0; n < desc_.planes.size(); ++n)
		{
			if(desc_.planes[n].size == 0)
				break;

			GLuint format = [&]() -> GLuint
			{
				switch(desc_.planes[n].channels)
				{
				case 1: return GL_LUMINANCE;
				case 2: return GL_LUMINANCE_ALPHA;
				case 3: return GL_BGR;
				case 4: return GL_BGRA;
				default: BOOST_THROW_EXCEPTION(out_of_range() << msg_info("1-4 channels are supported") << arg_name_info("desc.planes.channels")); 
				}
			}();

			pbo_.push_back(std::make_shared<common::gl::pixel_buffer_object>(desc_.planes[n].width, desc_.planes[n].height, format));
			pbo_.back()->is_smooth(true);
		}
		end_write();
	}
	
	void begin_write()
	{
		std::fill(pixel_data_.begin(), pixel_data_.end(), nullptr);
		boost::range::for_each(pbo_, std::mem_fn(&common::gl::pixel_buffer_object::begin_write));
	}

	void end_write()
	{
		boost::range::transform(pbo_, pixel_data_.begin(), std::mem_fn(&common::gl::pixel_buffer_object::end_write));
	}

	void draw(frame_shader& shader)
	{
		assert(pbo_.size() == desc_.planes.size());

		for(size_t n = 0; n < pbo_.size(); ++n)
		{
			glActiveTexture(GL_TEXTURE0+n);
			pbo_[n]->bind_texture();
		}
		shader.render(desc_);
	}

	unsigned char* data(size_t index)
	{
		return static_cast<unsigned char*>(pixel_data_[index]);
	}

	void reset()
	{
		audio_data_.clear();
		end_write();
	}
	
	std::vector<common::gl::pixel_buffer_object_ptr> pbo_;
	std::array<void*, 4> pixel_data_;	
	std::vector<short> audio_data_;
	
	const pixel_format_desc desc_;
};
	
write_frame::write_frame(const pixel_format_desc& desc) : impl_(new implementation(desc)){}
void write_frame::begin_write(){impl_->begin_write();}
void write_frame::end_write(){impl_->end_write();}	
void write_frame::draw(frame_shader& shader){impl_->draw(shader);}
boost::iterator_range<unsigned char*> write_frame::data(size_t index)
{
	auto ptr = static_cast<unsigned char*>(impl_->pixel_data_[index]);
	return boost::iterator_range<unsigned char*>(ptr, ptr+impl_->desc_.planes[index].size);
}
const boost::iterator_range<const unsigned char*> write_frame::data(size_t index) const
{
	auto ptr = static_cast<const unsigned char*>(impl_->pixel_data_[index]);
	return boost::iterator_range<const unsigned char*>(ptr, ptr+impl_->desc_.planes[index].size);
}

std::vector<short>& write_frame::audio_data() { return impl_->audio_data_; }
const std::vector<short>& write_frame::audio_data() const { return impl_->audio_data_; }
void write_frame::reset(){impl_->reset();}
}}