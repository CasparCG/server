#include "../../StdAfx.h"

#include "image_mixer.h"
#include "image_kernel.h"

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
		
image_transform::image_transform() 
	: opacity_(1.0)
	, gain_(1.0)
	, mode_(video_mode::invalid)
{
	std::fill(pos_.begin(), pos_.end(), 0.0);
	std::fill(uv_.begin(), uv_.end(), 0.0);
}

void image_transform::set_opacity(double value)
{
	tbb::spin_mutex::scoped_lock(mutex_);	
	opacity_ = value;
}

double image_transform::get_opacity() const
{
	tbb::spin_mutex::scoped_lock(mutex_);
	return opacity_;
}

void image_transform::set_gain(double value)
{
	tbb::spin_mutex::scoped_lock(mutex_);
	gain_ = value;
}

double image_transform::get_gain() const
{
	tbb::spin_mutex::scoped_lock(mutex_);
	return gain_;
}

void image_transform::set_position(double x, double y)
{
	std::array<double, 2> value = {x, y};
	tbb::spin_mutex::scoped_lock(mutex_);
	pos_ = value;
}

std::array<double, 2> image_transform::get_position() const
{
	tbb::spin_mutex::scoped_lock(mutex_);
	return pos_;
}

void image_transform::set_uv(double left, double top, double right, double bottom)
{
	std::array<double, 4> value = {left, top, right, bottom};
	tbb::spin_mutex::scoped_lock(mutex_);
	uv_ = value;
}

std::array<double, 4> image_transform::get_uv() const
{
	tbb::spin_mutex::scoped_lock(mutex_);
	return uv_;
}

void image_transform::set_mode(video_mode::type mode)
{
	tbb::spin_mutex::scoped_lock(mutex_);
	mode_ = mode;
}

video_mode::type image_transform::get_mode() const
{
	tbb::spin_mutex::scoped_lock(mutex_);
	return mode_;
}


image_transform& image_transform::operator*=(const image_transform &other)
{
	tbb::spin_mutex::scoped_lock(mutex_);
	opacity_ *= other.opacity_;
	mode_ = other.mode_;
	gain_ *= other.gain_;
	uv_[0] += other.uv_[0];
	uv_[1] += other.uv_[1];
	uv_[2] += other.uv_[2];
	uv_[3] += other.uv_[3];
	pos_[0] += other.pos_[0];
	pos_[1] += other.pos_[1];
	return *this;
}

const image_transform image_transform::operator*(const image_transform &other) const
{
	return image_transform(*this) *= other;
}

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
		context_->begin_invoke([=]
		{
			transform_stack_.push(image_transform());
			transform_stack_.top().set_mode(video_mode::progressive);
			transform_stack_.top().set_uv(0.0, 1.0, 1.0, 0.0);

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
			GL(glLoadIdentity());
			GL(glTranslated(transform.get_position()[0]*2.0, transform.get_position()[1]*2.0, 0.0));
			GL(glColor4d(1.0, 1.0, 1.0, transform.get_opacity()));
			GL(glViewport(0, 0, format_desc_.width, format_desc_.height));
			kernel_.apply(desc.pix_fmt, transform);

			std::vector<safe_ptr<device_buffer>> device_buffers;
			for(size_t n = 0; n < buffers.size(); ++n)
			{
				auto texture = context_->create_device_buffer(desc.planes[n].width, desc.planes[n].height, desc.planes[n].channels);
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
				glTexCoord2d(t.get_uv()[0], t.get_uv()[3]); glVertex2d(-1.0, -1.0);
				glTexCoord2d(t.get_uv()[2], t.get_uv()[3]); glVertex2d( 1.0, -1.0);
				glTexCoord2d(t.get_uv()[2], t.get_uv()[1]); glVertex2d( 1.0,  1.0);
				glTexCoord2d(t.get_uv()[0], t.get_uv()[1]); glVertex2d(-1.0,  1.0);
			glEnd();
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