/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/
 
#include "../../StdAfx.h"

#include "ogl_consumer.h"

#include "../../format/video_format.h"

#include <common/gl/utility.h>
#include <common/concurrency/executor.h>

#include <boost/thread.hpp>

#include <Glee.h>
#include <SFML/Window.hpp>

#include <windows.h>

#include <algorithm>

namespace caspar { namespace core { namespace ogl{	

struct consumer::implementation : boost::noncopyable
{	
	implementation(const video_format_desc& format_desc, unsigned int screen_index, stretch stretch, bool windowed) 
		: format_desc_(format_desc), stretch_(stretch), screen_width_(0), screen_height_(0), windowed_(windowed), texture_(0)
	{		
#ifdef _WIN32
		DISPLAY_DEVICE d_device;			
		memset(&d_device, 0, sizeof(d_device));
		d_device.cb = sizeof(d_device);

		std::vector<DISPLAY_DEVICE> displayDevices;
		for(int n = 0; EnumDisplayDevices(NULL, n, &d_device, NULL); ++n)
		{
			displayDevices.push_back(d_device);
			memset(&d_device, 0, sizeof(d_device));
			d_device.cb = sizeof(d_device);
		}

		if(screen_index >= displayDevices.size())
			BOOST_THROW_EXCEPTION(out_of_range() << arg_name_info("screen_index_"));
		
		DEVMODE devmode;
		memset(&devmode, 0, sizeof(devmode));
		
		if(!EnumDisplaySettings(displayDevices[screen_index].DeviceName, ENUM_CURRENT_SETTINGS, &devmode))
			BOOST_THROW_EXCEPTION(invalid_operation() << arg_name_info("screen_index") << msg_info("EnumDisplaySettings"));
		
		screen_width_ = windowed ? format_desc_.width : devmode.dmPelsWidth;
		screen_height_ = windowed ? format_desc_.height : devmode.dmPelsHeight;
		screen_x_ = devmode.dmPosition.x;
		screen_y_ = devmode.dmPosition.y;
#else
		if(!windowed)
			BOOST_THROW_EXCEPTION(not_supported() << msg_info("OGLConsumer doesn't support non-Win32 fullscreen"));

		if(screen_index != 0)
			CASPAR_LOG(warning) << "OGLConsumer only supports screen_index=0 for non-Win32";
		
		screen_width_ = format_desc_.width;
		screen_height_ = format_desc_.height;
		screen_x_ = 0;
		screen_y_ = 0;
#endif

		executor_.start();
		executor_.invoke([=]
		{
			window_.Create(sf::VideoMode(format_desc_.width, format_desc_.height, 32), "CasparCG", windowed_ ? sf::Style::Titlebar : sf::Style::Fullscreen);
			window_.ShowMouseCursor(false);
			window_.SetPosition(screen_x_, screen_y_);
			window_.SetSize(screen_width_, screen_height_);
			window_.SetActive();
			GL(glEnable(GL_TEXTURE_2D));
			GL(glDisable(GL_DEPTH_TEST));		
			GL(glClearColor(0.0, 0.0, 0.0, 0.0));
			GL(glViewport(0, 0, format_desc_.width, format_desc_.height));
			glLoadIdentity();
				
			wratio_ = static_cast<float>(format_desc_.width)/static_cast<float>(format_desc_.width);
			hratio_ = static_cast<float>(format_desc_.height)/static_cast<float>(format_desc_.height);

			std::pair<float, float> target_ratio = None();
			if(stretch_ == ogl::fill)
				target_ratio = Fill();
			else if(stretch_ == ogl::uniform)
				target_ratio = Uniform();
			else if(stretch_ == ogl::uniform_to_fill)
				target_ratio = UniformToFill();

			wSize_ = target_ratio.first;
			hSize_ = target_ratio.second;
			
			glGenTextures(1, &texture_);
			glBindTexture(GL_TEXTURE_2D, texture_);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, format_desc_.width, format_desc_.height, 0, GL_BGRA, GL_UNSIGNED_BYTE, 0);
			glBindTexture(GL_TEXTURE_2D, 0);
			
			GL(glGenBuffers(2, pbos_.data()));
			
			glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pbos_[0]);
			glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, format_desc_.size, 0, GL_STREAM_DRAW_ARB);
			glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pbos_[1]);
			glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, format_desc_.size, 0, GL_STREAM_DRAW_ARB);
			glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
		});
	}
	
	std::pair<float, float> None()
	{
		float width = static_cast<float>(format_desc_.width)/static_cast<float>(screen_width_);
		float height = static_cast<float>(format_desc_.height)/static_cast<float>(screen_height_);

		return std::make_pair(width, height);
	}

	std::pair<float, float> Uniform()
	{
		float aspect = static_cast<float>(format_desc_.width)/static_cast<float>(format_desc_.height);
		float width = std::min(1.0f, static_cast<float>(screen_height_)*aspect/static_cast<float>(screen_width_));
		float height = static_cast<float>(screen_width_*width)/static_cast<float>(screen_height_*aspect);

		return std::make_pair(width, height);
	}

	std::pair<float, float> Fill()
	{
		return std::make_pair(1.0f, 1.0f);
	}

	std::pair<float, float> UniformToFill()
	{
		float wr = static_cast<float>(format_desc_.width)/static_cast<float>(screen_width_);
		float hr = static_cast<float>(format_desc_.height)/static_cast<float>(screen_height_);
		float r_inv = 1.0f/std::min(wr, hr);

		float width = wr*r_inv;
		float height = hr*r_inv;

		return std::make_pair(width, height);
	}

	void render(const safe_ptr<const read_frame>& frame)
	{			
		glBindTexture(GL_TEXTURE_2D, texture_);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbos_[0]);

		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, format_desc_.width, format_desc_.height, GL_BGRA, GL_UNSIGNED_BYTE, 0);

		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbos_[1]);

		glBufferData(GL_PIXEL_UNPACK_BUFFER, format_desc_.size, 0, GL_STREAM_DRAW);

		auto ptr = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
		if(ptr)
		{
			std::copy_n(frame->image_data().begin(), frame->image_data().size(), reinterpret_cast<char*>(ptr));
			glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER); // release the mapped buffer
		}

		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
				
		GL(glClear(GL_COLOR_BUFFER_BIT));			
		glBegin(GL_QUADS);
				glTexCoord2f(0.0f,	  hratio_);	glVertex2f(-wSize_, -hSize_);
				glTexCoord2f(wratio_, hratio_);	glVertex2f( wSize_, -hSize_);
				glTexCoord2f(wratio_, 0.0f);	glVertex2f( wSize_,  hSize_);
				glTexCoord2f(0.0f,	  0.0f);	glVertex2f(-wSize_,  hSize_);
		glEnd();
		
		glBindTexture(GL_TEXTURE_2D, 0);

		std::rotate(pbos_.begin(), pbos_.begin() + 1, pbos_.end());
	}
		
	void send(const safe_ptr<const read_frame>& frame)
	{
		active_ = executor_.begin_invoke([=]
		{
			sf::Event e;
			while(window_.GetEvent(e)){}
			render(frame);
			window_.Display();
		});
	}

	frame_consumer::sync_mode synchronize()
	{
		active_.get();
		return frame_consumer::ready;
	}

	size_t buffer_depth() const
	{
		return 2;
	}
		
	boost::unique_future<void> active_;
	executor executor_;
	
	float wratio_;
	float hratio_;
	
	float wSize_;
	float hSize_;

	GLuint				  texture_;
	std::array<GLuint, 2> pbos_;

	bool windowed_;
	unsigned int screen_width_;
	unsigned int screen_height_;
	unsigned int screen_x_;
	unsigned int screen_y_;
				
	stretch stretch_;
	const video_format_desc format_desc_;
	
	sf::Window window_;
};

consumer::consumer(consumer&& other) : impl_(std::move(other.impl_)){}
consumer::consumer(const video_format_desc& format_desc, unsigned int screen_index, stretch stretch, bool windowed)
: impl_(new implementation(format_desc, screen_index, stretch, windowed)){}
void consumer::send(const safe_ptr<const read_frame>& frame){impl_->send(frame);}
frame_consumer::sync_mode consumer::synchronize(){return impl_->synchronize();}
size_t consumer::buffer_depth() const{return impl_->buffer_depth();}
}}}
