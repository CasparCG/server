#include "../StdAfx.h"

#include "image_processor.h"
#include "image_kernel.h"

#include "ogl_device.h"
#include "host_buffer.h"
#include "device_buffer.h"

#include <common/exception/exceptions.h>
#include <common/gl/utility.h>
#include <common/concurrency/executor.h>

#include <Glee.h>
#include <SFML/Window/Context.hpp>
#include <unordered_map>

namespace caspar { namespace core {
		
image_transform& image_transform::operator*=(const image_transform &other)
{
	alpha *= other.alpha;
	mode = other.mode;
	uv.get<0>() += other.uv.get<0>();
	uv.get<1>() += other.uv.get<1>();
	uv.get<2>() += other.uv.get<2>();
	uv.get<3>() += other.uv.get<3>();
	pos.get<0>() += other.pos.get<0>();
	pos.get<1>() += other.pos.get<1>();
	return *this;
}

const image_transform image_transform::operator*(const image_transform &other) const
{
	return image_transform(*this) *= other;
}

struct image_processor::implementation : boost::noncopyable
{	
	implementation(const video_format_desc& format_desc) : format_desc_(format_desc)
	{
		context_.begin_invoke([=]
		{
			transform_stack_.push(image_transform());
			transform_stack_.top().mode = video_mode::progressive;
			transform_stack_.top().uv = boost::make_tuple(0.0, 1.0, 1.0, 0.0);

			GL(glEnable(GL_TEXTURE_2D));
			GL(glDisable(GL_DEPTH_TEST));		

			render_targets_[0] = context_.create_device_buffer(format_desc.width, format_desc.height, 4);
			render_targets_[1] = context_.create_device_buffer(format_desc.width, format_desc.height, 4);
			
			GL(glGenFramebuffers(1, &fbo_));		
			GL(glBindFramebuffer(GL_FRAMEBUFFER_EXT, fbo_));
			GL(glReadBuffer(GL_COLOR_ATTACHMENT0_EXT));

			reading_ = context_.create_host_buffer(format_desc_.size, host_buffer::read_only);
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
		context_.begin_invoke([=]
		{
			GL(glLoadIdentity());
			GL(glTranslated(transform.pos.get<0>()*2.0, transform.pos.get<1>()*2.0, 0.0));
			GL(glColor4d(1.0, 1.0, 1.0, transform.alpha));
			GL(glViewport(0, 0, format_desc_.width, format_desc_.height));
			kernel_.use(desc.pix_fmt, transform.mode);

			std::vector<safe_ptr<device_buffer>> device_buffers;
			for(size_t n = 0; n < buffers.size(); ++n)
			{
				auto texture = context_.create_device_buffer(desc.planes[n].width, desc.planes[n].height, desc.planes[n].channels);
				texture->read(*buffers[n]);
				device_buffers.push_back(texture);
			}

			for(size_t n = 0; n < buffers.size(); ++n)
			{
				glActiveTexture(GL_TEXTURE0+n);
				device_buffers[n]->bind();
			}

			auto t = transform;
			glBegin(GL_QUADS);
				glTexCoord2d(t.uv.get<0>(), t.uv.get<3>()); glVertex2d(-1.0, -1.0);
				glTexCoord2d(t.uv.get<2>(), t.uv.get<3>()); glVertex2d( 1.0, -1.0);
				glTexCoord2d(t.uv.get<2>(), t.uv.get<1>()); glVertex2d( 1.0,  1.0);
				glTexCoord2d(t.uv.get<0>(), t.uv.get<1>()); glVertex2d(-1.0,  1.0);
			glEnd();
		});
	}

	void end()
	{
		transform_stack_.pop();
	}

	boost::unique_future<safe_ptr<const host_buffer>> begin_pass()
	{
		return context_.begin_invoke([=]() -> safe_ptr<const host_buffer>
		{
			reading_->map();
			render_targets_[0]->attach(0);
			GL(glClear(GL_COLOR_BUFFER_BIT));
			return safe_ptr<const host_buffer>(reading_);
		});
	}

	void end_pass()
	{
		context_.begin_invoke([=]
		{
			reading_ = context_.create_host_buffer(format_desc_.size, host_buffer::read_only);
			render_targets_[0]->write(*reading_);
			std::rotate(render_targets_.begin(), render_targets_.begin() + 1, render_targets_.end());
		});
	}
		
	std::vector<safe_ptr<host_buffer>> create_buffers(const pixel_format_desc& format)
	{
		std::vector<safe_ptr<host_buffer>> buffers;
		std::transform(format.planes.begin(), format.planes.end(), std::back_inserter(buffers), [&](const pixel_format_desc::plane& plane) -> safe_ptr<host_buffer>
		{
			return context_.create_host_buffer(plane.size, host_buffer::write_only);
		});
		return buffers;
	}
		
	ogl_device context_;

	const video_format_desc format_desc_;
	
	std::stack<image_transform> transform_stack_;

	GLuint fbo_;
	std::array<std::shared_ptr<device_buffer>, 2> render_targets_;

	std::shared_ptr<host_buffer> reading_;

	image_kernel kernel_;
};

image_processor::image_processor(const video_format_desc& format_desc) : impl_(new implementation(format_desc)){}
void image_processor::begin(const image_transform& transform) {	impl_->begin(transform);}
void image_processor::process(const pixel_format_desc& desc, std::vector<safe_ptr<host_buffer>>& buffers){	impl_->render(desc, buffers);}
void image_processor::end(){impl_->end();}
boost::unique_future<safe_ptr<const host_buffer>> image_processor::begin_pass(){	return impl_->begin_pass();}
void image_processor::end_pass(){impl_->end_pass();}
std::vector<safe_ptr<host_buffer>> image_processor::create_buffers(const pixel_format_desc& format){return impl_->create_buffers(format);}

}}