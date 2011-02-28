#include "../stdafx.h"

#include "image_mixer.h"
#include "image_kernel.h"
#include "image_transform.h"

#include "../gpu/ogl_device.h"
#include "../gpu/host_buffer.h"
#include "../gpu/device_buffer.h"

#include <common/exception/exceptions.h>
#include <common/gl/gl_check.h>
#include <common/concurrency/executor.h>

#include <Glee.h>
#include <SFML/Window/Context.hpp>
#include <unordered_map>

namespace caspar { namespace core {
		
struct image_mixer::implementation : boost::noncopyable
{			
	const video_format_desc format_desc_;
	
	std::stack<image_transform> transform_stack_;

	GLuint fbo_;
	std::array<std::shared_ptr<device_buffer>, 2> render_targets_;

	std::shared_ptr<host_buffer> reading_;

	image_kernel kernel_;

	safe_ptr<ogl_device> context_;

public:
	implementation(const video_format_desc& format_desc) 
		: format_desc_(format_desc)
		, context_(ogl_device::create())
	{
		context_->invoke([]
		{
			if(!GLEE_VERSION_3_0)
				BOOST_THROW_EXCEPTION(not_supported() << msg_info("Missing OpenGL 3.0 support."));
		});
		
		context_->begin_invoke([=]
		{
			transform_stack_.push(image_transform());
			transform_stack_.top().set_mode(video_mode::progressive);

			GL(glEnable(GL_TEXTURE_2D));
			GL(glDisable(GL_DEPTH_TEST));		

			render_targets_[0] = context_->create_device_buffer(format_desc.width, format_desc.height, 4);
			render_targets_[1] = context_->create_device_buffer(format_desc.width, format_desc.height, 4);
			
			GL(glGenFramebuffers(1, &fbo_));		
			GL(glBindFramebuffer(GL_FRAMEBUFFER_EXT, fbo_));
			GL(glReadBuffer(GL_COLOR_ATTACHMENT0_EXT));

			reading_ = context_->create_host_buffer(format_desc_.size, host_buffer::read_only);
		});
	}

	~implementation()
	{
		glDeleteFramebuffersEXT(1, &fbo_);
	}

	void begin(const image_transform& transform)
	{
		transform_stack_.push(transform_stack_.top()*transform);
	}
		
	void render(const pixel_format_desc& desc, std::vector<safe_ptr<host_buffer>>& buffers)
	{
		auto transform = transform_stack_.top();
		context_->begin_invoke([=]
		{
			GL(glColor4d(1.0, 1.0, 1.0, transform.get_opacity()));
			GL(glViewport(0, 0, format_desc_.width, format_desc_.height));
			kernel_.apply(desc, transform);

			std::vector<safe_ptr<device_buffer>> device_buffers;
			for(size_t n = 0; n < buffers.size(); ++n)
			{
				auto texture = context_->create_device_buffer(desc.planes[n].width, desc.planes[n].height, desc.planes[n].channels);
				texture->read(*buffers[n]);
				device_buffers.push_back(texture);
			}

			for(size_t n = 0; n < buffers.size(); ++n)
			{
				GL(glActiveTexture(GL_TEXTURE0+n));
				device_buffers[n]->bind();
			}

			auto m_p = transform.get_mask_translation();
			auto m_s = transform.get_mask_scale();
			double w = static_cast<double>(format_desc_.width);
			double h = static_cast<double>(format_desc_.height);
			
			GL(glEnable(GL_SCISSOR_TEST));
			GL(glScissor(static_cast<size_t>(m_p[0]*w), static_cast<size_t>(m_p[1]*h), static_cast<size_t>(m_s[0]*w), static_cast<size_t>(m_s[1]*h)));
			
			auto f_p = transform.get_image_translation();
			auto f_s = transform.get_image_scale();
			
			glBegin(GL_QUADS);
				glTexCoord2d(0.0, 0.0); glVertex2d( f_p[0]        *2.0-1.0,	 f_p[1]        *2.0-1.0);
				glTexCoord2d(1.0, 0.0); glVertex2d((f_p[0]+f_s[0])*2.0-1.0,  f_p[1]        *2.0-1.0);
				glTexCoord2d(1.0, 1.0); glVertex2d((f_p[0]+f_s[0])*2.0-1.0, (f_p[1]+f_s[1])*2.0-1.0);
				glTexCoord2d(0.0, 1.0); glVertex2d( f_p[0]        *2.0-1.0, (f_p[1]+f_s[1])*2.0-1.0);
			glEnd();
			GL(glDisable(GL_SCISSOR_TEST));
		});
	}

	void end()
	{
		transform_stack_.pop();
	}

	boost::unique_future<safe_ptr<const host_buffer>> begin_pass()
	{
		return context_->begin_invoke([=]() -> safe_ptr<const host_buffer>
		{
			reading_->map();
			render_targets_[0]->attach(0);
			GL(glClear(GL_COLOR_BUFFER_BIT));
			return safe_ptr<const host_buffer>(reading_);
		});
	}

	void end_pass()
	{
		context_->begin_invoke([=]
		{
			reading_ = context_->create_host_buffer(format_desc_.size, host_buffer::read_only);
			render_targets_[0]->write(*reading_);
			std::rotate(render_targets_.begin(), render_targets_.begin() + 1, render_targets_.end());
		});
	}
		
	std::vector<safe_ptr<host_buffer>> create_buffers(const pixel_format_desc& format)
	{
		std::vector<safe_ptr<host_buffer>> buffers;
		std::transform(format.planes.begin(), format.planes.end(), std::back_inserter(buffers), [&](const pixel_format_desc::plane& plane) -> safe_ptr<host_buffer>
		{
			return context_->create_host_buffer(plane.size, host_buffer::write_only);
		});
		return buffers;
	}
};

image_mixer::image_mixer(const video_format_desc& format_desc) : impl_(new implementation(format_desc)){}
void image_mixer::begin(const image_transform& transform) {	impl_->begin(transform);}
void image_mixer::process(const pixel_format_desc& desc, std::vector<safe_ptr<host_buffer>>& buffers){	impl_->render(desc, buffers);}
void image_mixer::end(){impl_->end();}
boost::unique_future<safe_ptr<const host_buffer>> image_mixer::begin_pass(){	return impl_->begin_pass();}
void image_mixer::end_pass(){impl_->end_pass();}
std::vector<safe_ptr<host_buffer>> image_mixer::create_buffers(const pixel_format_desc& format){return impl_->create_buffers(format);}

}}