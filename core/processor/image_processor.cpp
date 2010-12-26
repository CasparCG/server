#include "../StdAfx.h"

#include "image_processor.h"
#include "image_kernel.h"

#include "host_buffer.h"
#include "device_buffer.h"

#include <common/exception/exceptions.h>
#include <common/gl/utility.h>
#include <common/concurrency/executor.h>

#include <Glee.h>
#include <SFML/Window.hpp>

#include <boost/noncopyable.hpp>

#include <unordered_map>

namespace caspar { namespace core {
		
class ogl_context
{	
public:
	
	ogl_context()
	{
		executor_.start();
		invoke([=]
		{
			context_.reset(new sf::Context());
			context_->SetActive(true);
		});
	}

	template<typename Func>
	auto begin_invoke(Func&& func) -> boost::unique_future<decltype(func())> // noexcept
	{			
		return executor_.begin_invoke(std::forward<Func>(func));
	}
	
	template<typename Func>
	auto invoke(Func&& func) -> decltype(func())
	{
		return executor_.invoke(std::forward<Func>(func));
	}
		
	safe_ptr<device_buffer> create_device_buffer(size_t width, size_t height, size_t stride)
	{
		auto pool = &device_pools_[stride][((width << 12) & 0x00FFF000) | ((height << 0) & 0x00000FFF)];
		std::shared_ptr<device_buffer> buffer;
		if(!pool->try_pop(buffer))		
		{
			executor_.invoke([&]
			{
				buffer = std::make_shared<device_buffer>(width, height, stride);
			});	
		}
		
		return safe_ptr<device_buffer>(buffer.get(), [=](device_buffer*){pool->push(buffer);});
	}
	
	safe_ptr<host_buffer> create_host_buffer(size_t size, host_buffer::usage_t usage)
	{
		auto pool = &host_pools_[usage][size];
		std::shared_ptr<host_buffer> buffer;
		if(!pool->try_pop(buffer))
		{
			executor_.invoke([&]
			{
				buffer = std::make_shared<host_buffer>(size, usage);
				if(usage == host_buffer::write_only)
					buffer->map();
				else
					buffer->unmap();
			});	
		}

		return safe_ptr<host_buffer>(buffer.get(), [=](host_buffer*)
		{
			executor_.begin_invoke([=]
			{
				if(usage == host_buffer::write_only)
					buffer->map();
				else
					buffer->unmap();
				pool->push(buffer);
			});
		});
	}
private:
	executor executor_;
	std::unique_ptr<sf::Context> context_;
	
	std::array<tbb::concurrent_unordered_map<size_t, tbb::concurrent_bounded_queue<std::shared_ptr<device_buffer>>>, 4> device_pools_;
	std::array<tbb::concurrent_unordered_map<size_t, tbb::concurrent_bounded_queue<std::shared_ptr<host_buffer>>>, 2> host_pools_;
};

image_transform& image_transform::operator*=(const image_transform &other)
{
	alpha *= other.alpha;
	mode = other.mode;
	uv.get<0>() += other.uv.get<0>();
	uv.get<1>() += other.uv.get<1>();
	uv.get<2>() += other.uv.get<2>();
	uv.get<3>() += other.uv.get<3>();
	return *this;
}

const image_transform image_transform::operator*(const image_transform &other) const
{
	return image_transform(*this) *= other;
}

struct image_processor::implementation
{	
	implementation(const video_format_desc& format_desc) : format_desc_(format_desc)
	{
		context_.begin_invoke([=]
		{
			transform_stack_.push(image_transform());
			transform_stack_.top().mode = video_mode::progressive;
			transform_stack_.top().uv = boost::make_tuple(0.0, 1.0, 1.0, 0.0);

			GL(glEnable(GL_POLYGON_STIPPLE));
			GL(glEnable(GL_TEXTURE_2D));
			GL(glEnable(GL_BLEND));
			GL(glDisable(GL_DEPTH_TEST));
			GL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));			

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
		context_.begin_invoke([=]
		{
			glPushMatrix();
			transform_stack_.push(transform_stack_.top()*transform);

			glColor4d(1.0, 1.0, 1.0, transform_stack_.top().alpha);
			glTranslated(transform.pos.get<0>()*2.0, transform.pos.get<1>()*2.0, 0.0);
		});
	}
		
	void render(const pixel_format_desc& desc, std::vector<safe_ptr<host_buffer>>& buffers)
	{
		context_.begin_invoke([=]
		{
			GL(glViewport(0, 0, format_desc_.width, format_desc_.height));
			kernel_.use(desc.pix_fmt, transform_stack_.top().mode);

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

			auto t = transform_stack_.top();
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
		context_.begin_invoke([=]
		{
			transform_stack_.pop();
			glColor4d(1.0, 1.0, 1.0, transform_stack_.top().alpha);
			glPopMatrix();
		});
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
		
	const video_format_desc format_desc_;

	ogl_context context_;

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