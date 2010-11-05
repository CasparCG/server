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

#if defined(_MSC_VER)
#pragma warning (disable : 4244)
#endif

#include "ogl_consumer.h"

#include "../../frame/frame_format.h"
#include "../../frame/gpu_frame.h"
#include "../../../common/utility/memory.h"
#include "../../../common/gl/gl_check.h"
#include "../../../common/gl/pixel_buffer_object.h"

#include <boost/thread.hpp>

#include <Glee.h>
#include <SFML/Window.hpp>

#include <windows.h>

namespace caspar { namespace core { namespace ogl{	

struct consumer::implementation : boost::noncopyable
{	
	implementation(const frame_format_desc& format_desc, unsigned int screen_index, stretch stretch, bool windowed) 
		: index_(0), format_desc_(format_desc), stretch_(stretch), screen_width_(0), screen_height_(0), windowed_(windowed)
	{		
#ifdef _WIN32
		DISPLAY_DEVICE dDevice;			
		memset(&dDevice,0,sizeof(dDevice));
		dDevice.cb = sizeof(dDevice);

		std::vector<DISPLAY_DEVICE> displayDevices;
		for(int n = 0; EnumDisplayDevices(NULL, n, &dDevice, NULL); ++n)
		{
			displayDevices.push_back(dDevice);
			memset(&dDevice,0,sizeof(dDevice));
			dDevice.cb = sizeof(dDevice);
		}

		if(screen_index >= displayDevices.size())
			BOOST_THROW_EXCEPTION(out_of_range() << arg_name_info("screen_index_"));
		
		DEVMODE devmode;
		memset(&devmode,0,sizeof(devmode));
		
		if(!EnumDisplaySettings(displayDevices[screen_index].DeviceName, ENUM_CURRENT_SETTINGS, &devmode))
			BOOST_THROW_EXCEPTION(invalid_operation() << arg_name_info("screen_index") << msg_info("EnumDisplaySettings"));
		
		screen_width_ = windowed ? format_desc_.width : devmode.dmPelsWidth;
		screen_height_ = windowed ? format_desc_.height : devmode.dmPelsHeight;
		screenX_ = devmode.dmPosition.x;
		screenY_ = devmode.dmPosition.y;
#else
		if(!windowed)
			BOOST_THROW_EXCEPTION(not_supported() << msg_info("OGLConsumer doesn't support non-Win32 fullscreen"));

		if(screen_index != 0)
			CASPAR_LOG(warning) << "OGLConsumer only supports screen_index=0 for non-Win32";
		
		screen_width_ = format_desc_.width;
		screen_height_ = format_desc_.height;
		screenX_ = 0;
		screenY_ = 0;
#endif
		frame_buffer_.set_capacity(1);
		thread_ = boost::thread([=]{run();});
	}
	
	~implementation()
	{
		frame_buffer_.push(nullptr),
		thread_.join();
	}

	void init()	
	{
		window_.Create(sf::VideoMode(format_desc_.width, format_desc_.height, 32), "CasparCG", windowed_ ? sf::Style::Titlebar : sf::Style::Fullscreen);
		window_.ShowMouseCursor(false);
		window_.SetPosition(screenX_, screenY_);
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
					
		pbos_[0].create(format_desc_.width, format_desc_.height);
		pbos_[1].create(format_desc_.width, format_desc_.height);
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

	void render(const gpu_frame_ptr& frame)
	{		
		index_ = (index_ + 1) % 2;
		int next_index = (index_ + 1) % 2;
				
		auto ptr = pbos_[index_].end_write();
		common::aligned_memcpy(ptr, frame->data(), frame->size());

		GL(glClear(GL_COLOR_BUFFER_BIT));	
		pbos_[next_index].bind_texture();				
		glBegin(GL_QUADS);
				glTexCoord2f(0.0f,	  hratio_);	glVertex2f(-wSize_, -hSize_);
				glTexCoord2f(wratio_, hratio_);	glVertex2f( wSize_, -hSize_);
				glTexCoord2f(wratio_, 0.0f);	glVertex2f( wSize_,  hSize_);
				glTexCoord2f(0.0f,	  0.0f);	glVertex2f(-wSize_,  hSize_);
		glEnd();

		pbos_[next_index].begin_write();
	}
			
	void display(const gpu_frame_ptr& frame)
	{
		if(frame == nullptr)
			return;		

		if(exception_ != nullptr)
			std::rethrow_exception(exception_);

		frame_buffer_.push(frame);
	}

	void run()
	{			
		init();
				
		gpu_frame_ptr frame;
		do
		{
			try
			{		
				frame_buffer_.pop(frame);
				if(frame != nullptr)
				{
					sf::Event e;
					while(window_.GetEvent(e)){}
					window_.SetActive();
					render(frame);
					window_.Display();
				}
			}
			catch(...)
			{
				exception_ = std::current_exception();
			}
		}		
		while(frame != nullptr);
	}		

	float wratio_;
	float hratio_;
	
	float wSize_;
	float hSize_;

	int index_;
	common::gl::pixel_buffer_object pbos_[2];

	bool windowed_;
	unsigned int screen_width_;
	unsigned int screen_height_;
	unsigned int screenX_;
	unsigned int screenY_;
				
	stretch stretch_;
	frame_format_desc format_desc_;

	std::exception_ptr exception_;
	boost::thread thread_;
	tbb::concurrent_bounded_queue<gpu_frame_ptr> frame_buffer_;

	sf::Window window_;
};

consumer::consumer(const frame_format_desc& format_desc, unsigned int screen_index, stretch stretch, bool windowed)
: impl_(new implementation(format_desc, screen_index, stretch, windowed)){}
const frame_format_desc& consumer::get_frame_format_desc() const{return impl_->format_desc_;}
void consumer::display(const gpu_frame_ptr& frame){impl_->display(frame);}
}}}
