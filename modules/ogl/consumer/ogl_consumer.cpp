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

#define NOMINMAX

#include <windows.h>
#include <Glee.h>
#include <SFML/Window.hpp>

#include "ogl_consumer.h"

#include <core/video_format.h>
#include <core/mixer/read_frame.h>

#include <common/gl/gl_check.h>
#include <common/concurrency/executor.h>
#include <common/memory/safe_ptr.h>
#include <common/memory/memcpy.h>
#include <common/diagnostics/graph.h>
#include <common/utility/timer.h>

#include <boost/timer.hpp>
#include <boost/circular_buffer.hpp>

#include <algorithm>
#include <array>

namespace caspar {
		
enum stretch
{
	none,
	uniform,
	fill,
	uniform_to_fill
};

struct ogl_consumer : boost::noncopyable
{		
	float					width_;
	float					height_;

	GLuint					texture_;
	std::array<GLuint, 2>	pbos_;

	const bool				windowed_;
	unsigned int			screen_width_;
	unsigned int			screen_height_;
	const unsigned int		screen_index_;
				
	const stretch			stretch_;
	core::video_format_desc format_desc_;
	
	sf::Window				window_;
	
	safe_ptr<diagnostics::graph> graph_;
	boost::timer			perf_timer_;

	size_t					square_width_;
	size_t					square_height_;

	boost::circular_buffer<safe_ptr<core::read_frame>> frame_buffer_;

	executor				executor_;
public:
	ogl_consumer(unsigned int screen_index, stretch stretch, bool windowed, const core::video_format_desc& format_desc) 
		: stretch_(stretch)
		, windowed_(windowed)
		, texture_(0)
		, screen_index_(screen_index)
		, format_desc_(format_desc_)
		, graph_(diagnostics::create_graph(narrow(print())))
		, frame_buffer_(core::consumer_buffer_depth())
		, executor_(print())
	{		
		graph_->add_guide("frame-time", 0.5);
		graph_->set_color("frame-time", diagnostics::color(1.0f, 0.0f, 0.0f));
		
		format_desc_ = format_desc;

		square_width_ = format_desc_.width;
		square_height_ = format_desc_.height;
						
		if(format_desc_.format == core::video_format::pal)
		{
			square_width_ = 768;
			square_height_ = 576;
		}
		else if(format_desc_.format == core::video_format::ntsc)
		{
			square_width_ = 720;
			square_height_ = 547;
		}

		screen_width_ = format_desc.width;
		screen_height_ = format_desc.height;
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

		if(screen_index_ >= displayDevices.size())
			BOOST_THROW_EXCEPTION(out_of_range() << arg_name_info("screen_index_") << msg_info(narrow(print())));
		
		DEVMODE devmode;
		memset(&devmode, 0, sizeof(devmode));
		
		if(!EnumDisplaySettings(displayDevices[screen_index_].DeviceName, ENUM_CURRENT_SETTINGS, &devmode))
			BOOST_THROW_EXCEPTION(invalid_operation() << arg_name_info("screen_index") << msg_info(narrow(print()) + " EnumDisplaySettings"));
		
		screen_width_ = windowed_ ? square_width_ : devmode.dmPelsWidth;
		screen_height_ = windowed_ ? square_height_ : devmode.dmPelsHeight;
#else
		if(!windowed)
			BOOST_THROW_EXCEPTION(not_supported() << msg_info(narrow(print() + " doesn't support non-Win32 fullscreen"));

		if(screen_index != 0)
			CASPAR_LOG(warning) << print() << " only supports screen_index=0 for non-Win32";
#endif		
		executor_.invoke([=]
		{
			if(!GLEE_VERSION_2_1)
				BOOST_THROW_EXCEPTION(not_supported() << msg_info("Missing OpenGL 2.1 support."));

			window_.Create(sf::VideoMode(screen_width_, screen_height_, 32), narrow(print()), windowed_ ? sf::Style::Resize : sf::Style::Fullscreen);
			window_.ShowMouseCursor(false);
			window_.SetPosition(devmode.dmPosition.x, devmode.dmPosition.y);
			window_.SetSize(screen_width_, screen_height_);
			window_.SetActive();
			GL(glEnable(GL_TEXTURE_2D));
			GL(glDisable(GL_DEPTH_TEST));		
			GL(glClearColor(0.0, 0.0, 0.0, 0.0));
			GL(glViewport(0, 0, format_desc_.width, format_desc_.height));
			glLoadIdentity();
				
			calculate_aspect();
			
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
		CASPAR_LOG(info) << print() << " Sucessfully Initialized.";
	}
	
	const core::video_format_desc& get_video_format_desc() const
	{
		return format_desc_;
	}

	void calculate_aspect()
	{
		if(windowed_)
		{
			screen_height_ = window_.GetHeight();
			screen_width_ = window_.GetWidth();
		}
		
		GL(glViewport(0, 0, screen_width_, screen_height_));

		std::pair<float, float> target_ratio = None();
		if(stretch_ == fill)
			target_ratio = Fill();
		else if(stretch_ == uniform)
			target_ratio = Uniform();
		else if(stretch_ == uniform_to_fill)
			target_ratio = UniformToFill();

		width_ = target_ratio.first;
		height_ = target_ratio.second;
	}
		
	std::pair<float, float> None()
	{
		float width = static_cast<float>(square_width_)/static_cast<float>(screen_width_);
		float height = static_cast<float>(square_height_)/static_cast<float>(screen_height_);

		return std::make_pair(width, height);
	}

	std::pair<float, float> Uniform()
	{
		float aspect = static_cast<float>(square_width_)/static_cast<float>(square_height_);
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
		float wr = static_cast<float>(square_width_)/static_cast<float>(screen_width_);
		float hr = static_cast<float>(square_height_)/static_cast<float>(screen_height_);
		float r_inv = 1.0f/std::min(wr, hr);

		float width = wr*r_inv;
		float height = hr*r_inv;

		return std::make_pair(width, height);
	}

	void render(const safe_ptr<core::read_frame>& frame)
	{			
		glBindTexture(GL_TEXTURE_2D, texture_);

		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbos_[0]);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, format_desc_.width, format_desc_.height, GL_BGRA, GL_UNSIGNED_BYTE, 0);

		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbos_[1]);
		glBufferData(GL_PIXEL_UNPACK_BUFFER, format_desc_.size, 0, GL_STREAM_DRAW);

		auto ptr = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
		if(ptr)
		{
			fast_memcpy(reinterpret_cast<char*>(ptr), frame->image_data().begin(), frame->image_data().size());
			glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER); // release the mapped buffer
		}

		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
				
		GL(glClear(GL_COLOR_BUFFER_BIT));			
		glBegin(GL_QUADS);
				glTexCoord2f(0.0f,	  1.0f);	glVertex2f(-width_, -height_);
				glTexCoord2f(1.0f,	  1.0f);	glVertex2f( width_, -height_);
				glTexCoord2f(1.0f,	  0.0f);	glVertex2f( width_,  height_);
				glTexCoord2f(0.0f,	  0.0f);	glVertex2f(-width_,  height_);
		glEnd();
		
		glBindTexture(GL_TEXTURE_2D, 0);

		std::rotate(pbos_.begin(), pbos_.begin() + 1, pbos_.end());
	}

	void send(const safe_ptr<core::read_frame>& frame)
	{
		frame_buffer_.push_back(frame);

		if(frame_buffer_.full())
			do_send(frame_buffer_.front());
	}
		
	void do_send(const safe_ptr<core::read_frame>& frame)
	{		
		executor_.try_begin_invoke([=]
		{
			perf_timer_.restart();
			sf::Event e;
			while(window_.GetEvent(e))
			{
				if(e.Type == sf::Event::Resized)
					 calculate_aspect();
			}
			render(frame);
			window_.Display();
			graph_->update_value("frame-time", static_cast<float>(perf_timer_.elapsed()*format_desc_.fps*0.5));
		});
	}

	std::wstring print() const
	{	
		return  L"ogl[" + boost::lexical_cast<std::wstring>(screen_index_) + L"|" + format_desc_.name + L"]";
	}

	size_t buffer_depth() const{return 2;}
};


struct ogl_consumer_proxy : public core::frame_consumer
{
	size_t screen_index_;
	caspar::stretch stretch_;
	bool windowed_;
	bool key_only_;

	std::unique_ptr<ogl_consumer> consumer_;

public:

	ogl_consumer_proxy(size_t screen_index, stretch stretch, bool windowed, bool key_only)
		: screen_index_(screen_index)
		, stretch_(stretch)
		, windowed_(windowed)
		, key_only_(key_only){}
	
	virtual void initialize(const core::video_format_desc& format_desc)
	{
		consumer_.reset(new ogl_consumer(screen_index_, stretch_, windowed_, format_desc));
	}
	
	virtual void send(const safe_ptr<core::read_frame>& frame)
	{
		consumer_->send(frame);
	}
	
	virtual std::wstring print() const
	{
		return consumer_->print();
	}

	virtual bool key_only() const
	{
		return key_only_;
	}

	virtual bool has_synchronization_clock() const 
	{
		return false;
	}

	virtual const core::video_format_desc& get_video_format_desc() const
	{
		return consumer_->get_video_format_desc();
	}
};	

safe_ptr<core::frame_consumer> create_ogl_consumer(const std::vector<std::wstring>& params)
{
	if(params.size() < 1 || params[0] != L"SCREEN")
		return core::frame_consumer::empty();
	
	size_t screen_index = 0;
	stretch stretch = stretch::fill;
	bool windowed = true;
	
	if(params.size() > 1) 
		screen_index = lexical_cast_or_default<int>(params[2], screen_index);

	if(params.size() > 2) 
		windowed = lexical_cast_or_default<bool>(params[3], windowed);

	bool key_only = std::find(params.begin(), params.end(), L"KEY_ONLY") != params.end();

	return make_safe<ogl_consumer_proxy>(screen_index, stretch, windowed, key_only);
}

safe_ptr<core::frame_consumer> create_ogl_consumer(const boost::property_tree::ptree& ptree) 
{
	size_t screen_index = ptree.get("device", 0);
	bool windowed =ptree.get("windowed", true);
	
	stretch stretch = stretch::fill;
	auto key_str = ptree.get("stretch", "default");
	if(key_str == "uniform")
		stretch = stretch::uniform;
	else if(key_str == "uniform_to_fill")
		stretch = stretch::uniform_to_fill;

	bool key_only = ptree.get("key-only", false);

	return make_safe<ogl_consumer_proxy>(screen_index, stretch, windowed, key_only);
}

}