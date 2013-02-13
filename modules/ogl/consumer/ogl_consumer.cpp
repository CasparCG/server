/*
* Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
*
* This file is part of CasparCG (www.casparcg.com).
*
* CasparCG is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* CasparCG is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
*
* Author: Robert Nagy, ronag89@gmail.com
*/

#include "ogl_consumer.h"

#include <GL/glew.h>
#include <SFML/Window.hpp>

#include <common/diagnostics/graph.h>
#include <common/gl/gl_check.h>
#include <common/log/log.h>
#include <common/memory/safe_ptr.h>
#include <common/memory/memcpy.h>
#include <common/memory/memshfl.h>
#include <common/utility/timer.h>
#include <common/utility/string.h>
#include <common/concurrency/future_util.h>

#include <ffmpeg/producer/filter/filter.h>

#include <core/video_format.h>
#include <core/mixer/read_frame.h>
#include <core/consumer/frame_consumer.h>

#include <boost/timer.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/property_tree/ptree.hpp>

#include <tbb/atomic.h>
#include <tbb/concurrent_queue.h>

#include <boost/assign.hpp>

#include <algorithm>
#include <vector>

#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4244)
#endif
extern "C" 
{
	#define __STDC_CONSTANT_MACROS
	#define __STDC_LIMIT_MACROS
	#include <libavcodec/avcodec.h>
	#include <libavutil/imgutils.h>
}
#if defined(_MSC_VER)
#pragma warning (pop)
#endif

typedef int (*PFNWGLEXTGETSWAPINTERVALPROC) (void);
 
namespace caspar { namespace ogl {
		
enum stretch
{
	none,
	uniform,
	fill,
	uniform_to_fill
};

struct configuration
{
	enum aspect_ratio
	{
		aspect_4_3 = 0,
		aspect_16_9,
		aspect_invalid,
	};
		
	std::wstring	name;
	size_t			screen_index;
	stretch			stretch;
	bool			windowed;
	bool			auto_deinterlace;
	bool			key_only;
	aspect_ratio	aspect;	
	bool			vsync;

	configuration()
		: name(L"ogl")
		, screen_index(0)
		, stretch(fill)
		, windowed(true)
		, auto_deinterlace(true)
		, key_only(false)
		, aspect(aspect_invalid)
		, vsync(false)
	{
	}
};

struct ogl_consumer : boost::noncopyable
{		
	const configuration		config_;
	core::video_format_desc format_desc_;
	int						channel_index_;

	GLuint					texture_;
	std::vector<GLuint>		pbos_;
			
	float					width_;
	float					height_;	
	unsigned int			screen_x_;
	unsigned int			screen_y_;
	unsigned int			screen_width_;
	unsigned int			screen_height_;
	size_t					square_width_;
	size_t					square_height_;				
	
	sf::Window				window_;
	
	safe_ptr<diagnostics::graph>	graph_;
	boost::timer					perf_timer_;
	boost::timer					tick_timer_;

	caspar::high_prec_timer	wait_timer_;

	tbb::concurrent_bounded_queue<safe_ptr<core::read_frame>>	frame_buffer_;

	boost::thread			thread_;
	tbb::atomic<bool>		is_running_;
	
	ffmpeg::filter			filter_;
public:
	ogl_consumer(const configuration& config, const core::video_format_desc& format_desc, int channel_index) 
		: config_(config)
		, format_desc_(format_desc)
		, channel_index_(channel_index)
		, texture_(0)
		, pbos_(2, 0)	
		, screen_width_(format_desc.width)
		, screen_height_(format_desc.height)
		, square_width_(format_desc.square_width)
		, square_height_(format_desc.square_height)
		, filter_(format_desc.field_mode == core::field_mode::progressive || !config.auto_deinterlace ? L"" : L"YADIF=1:-1", boost::assign::list_of(PIX_FMT_BGRA))
	{		
		if(format_desc_.format == core::video_format::ntsc && config_.aspect == configuration::aspect_4_3)
		{
			// Use default values which are 4:3.
		}
		else
		{
			if(config_.aspect == configuration::aspect_16_9)
				square_width_ = (format_desc.height*16)/9;
			else if(config_.aspect == configuration::aspect_4_3)
				square_width_ = (format_desc.height*4)/3;
		}

		frame_buffer_.set_capacity(2);
		
		graph_->set_color("tick-time", diagnostics::color(0.0f, 0.6f, 0.9f));	
		graph_->set_color("frame-time", diagnostics::color(0.1f, 1.0f, 0.1f));
		graph_->set_color("dropped-frame", diagnostics::color(0.3f, 0.6f, 0.3f));

		graph_->set_text(print());
		diagnostics::register_graph(graph_);
									
		DISPLAY_DEVICE d_device = {sizeof(d_device), 0};			
		std::vector<DISPLAY_DEVICE> displayDevices;
		for(int n = 0; EnumDisplayDevices(NULL, n, &d_device, NULL); ++n)
			displayDevices.push_back(d_device);

		if(config_.screen_index >= displayDevices.size())
			CASPAR_LOG(warning) << print() << L" Invalid screen-index: " << config_.screen_index;
		
		DEVMODE devmode = {};
		if(!EnumDisplaySettings(displayDevices[config_.screen_index].DeviceName, ENUM_CURRENT_SETTINGS, &devmode))
			CASPAR_LOG(warning) << print() << L" Could not find display settings for screen-index: " << config_.screen_index;
		
		screen_x_		= devmode.dmPosition.x;
		screen_y_		= devmode.dmPosition.y;
		screen_width_	= config_.windowed ? square_width_ : devmode.dmPelsWidth;
		screen_height_	= config_.windowed ? square_height_ : devmode.dmPelsHeight;

		is_running_ = true;
		thread_ = boost::thread([this]{run();});
	}
	
	~ogl_consumer()
	{
		is_running_ = false;
		frame_buffer_.try_push(make_safe<core::read_frame>());
		thread_.join();
	}

	void init()
	{
		if(!GLEW_VERSION_2_1)
			BOOST_THROW_EXCEPTION(not_supported() << msg_info("Missing OpenGL 2.1 support."));

		window_.Create(sf::VideoMode(screen_width_, screen_height_, 32), narrow(L"Screen consumer " + channel_and_format()), config_.windowed ? sf::Style::Resize | sf::Style::Close : sf::Style::Fullscreen);
		window_.ShowMouseCursor(false);
		window_.SetPosition(screen_x_, screen_y_);
		window_.SetSize(screen_width_, screen_height_);
		window_.SetActive();
		GL(glEnable(GL_TEXTURE_2D));
		GL(glDisable(GL_DEPTH_TEST));		
		GL(glClearColor(0.0, 0.0, 0.0, 0.0));
		GL(glViewport(0, 0, format_desc_.width, format_desc_.height));
		GL(glLoadIdentity());
				
		calculate_aspect();
			
		GL(glGenTextures(1, &texture_));
		GL(glBindTexture(GL_TEXTURE_2D, texture_));
		GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
		GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
		GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP));
		GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP));
		GL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, format_desc_.width, format_desc_.height, 0, GL_BGRA, GL_UNSIGNED_BYTE, 0));
		GL(glBindTexture(GL_TEXTURE_2D, 0));
					
		GL(glGenBuffers(2, pbos_.data()));
			
		glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pbos_[0]);
		glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, format_desc_.size, 0, GL_STREAM_DRAW_ARB);
		glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pbos_[1]);
		glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, format_desc_.size, 0, GL_STREAM_DRAW_ARB);
		glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
				
		if(config_.vsync)
		{
			auto wglSwapIntervalEXT = reinterpret_cast<void(APIENTRY*)(int)>(wglGetProcAddress("wglSwapIntervalEXT"));
			if(wglSwapIntervalEXT)
			{
				wglSwapIntervalEXT(1);
				CASPAR_LOG(info) << print() << " Successfully enabled vsync.";
			}
			else
				CASPAR_LOG(info) << print() << " Failed to enable vsync.";
		}

		CASPAR_LOG(info) << print() << " Successfully Initialized.";
	}

	void uninit()
	{		
		if(texture_)
			glDeleteTextures(1, &texture_);

		BOOST_FOREACH(auto& pbo, pbos_)
		{
			if(pbo)
				glDeleteBuffers(1, &pbo);
		}
	}

	void run()
	{
		try
		{
			init();

			while(is_running_)
			{			
				try
				{

					sf::Event e;		
					while(window_.GetEvent(e))
					{
						if(e.Type == sf::Event::Resized)
							calculate_aspect();
						else if(e.Type == sf::Event::Closed)
							is_running_ = false;
					}
			
					safe_ptr<core::read_frame> frame;

					frame_buffer_.pop(frame);

					render_and_draw_frame(frame);
					
					graph_->set_value("tick-time", tick_timer_.elapsed()*format_desc_.fps*0.5);	
					tick_timer_.restart();
				}
				catch(...)
				{
					CASPAR_LOG_CURRENT_EXCEPTION();
					is_running_ = false;
				}
			}

			uninit();
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
		}
	}

	void try_sleep_almost_until_vblank()
	{
		static const double THRESHOLD = 0.003;
		double threshold = config_.vsync ? THRESHOLD : 0.0;

		auto frame_time = 1.0 / (format_desc_.fps * format_desc_.field_count);

		wait_timer_.tick(frame_time - threshold);
	}

	void wait_for_vblank_and_display()
	{
		try_sleep_almost_until_vblank();
		window_.Display();
		// Make sure that the next tick measures the duration from this point in time.
		wait_timer_.tick(0.0);
	}
	
	safe_ptr<AVFrame> get_av_frame()
	{		
		safe_ptr<AVFrame> av_frame(avcodec_alloc_frame(), av_free);	
		avcodec_get_frame_defaults(av_frame.get());
						
		av_frame->linesize[0]		= format_desc_.width*4;			
		av_frame->format			= PIX_FMT_BGRA;
		av_frame->width				= format_desc_.width;
		av_frame->height			= format_desc_.height;
		av_frame->interlaced_frame	= format_desc_.field_mode != core::field_mode::progressive;
		av_frame->top_field_first	= format_desc_.field_mode == core::field_mode::upper ? 1 : 0;

		return av_frame;
	}

	void render_and_draw_frame(const safe_ptr<core::read_frame>& frame)
	{
		if(static_cast<size_t>(frame->image_data().size()) != format_desc_.size)
			return;
					
		perf_timer_.restart();
		auto av_frame = get_av_frame();
		av_frame->data[0] = const_cast<uint8_t*>(frame->image_data().begin());

		filter_.push(av_frame);
		auto frames = filter_.poll_all();

		if (frames.empty())
			return;

		if (frames.size() == 1)
		{
			render(frames[0], frame->image_data().size());
			graph_->set_value("frame-time", perf_timer_.elapsed() * format_desc_.fps * 0.5);

			wait_for_vblank_and_display(); // progressive frame
		}
		else if (frames.size() == 2)
		{
			render(frames[0], frame->image_data().size());
			double perf_elapsed = perf_timer_.elapsed();

			wait_for_vblank_and_display(); // field1

			perf_timer_.restart();
			render(frames[1], frame->image_data().size());
			perf_elapsed += perf_timer_.elapsed();
			graph_->set_value("frame-time", perf_elapsed * format_desc_.fps * 0.5);

			wait_for_vblank_and_display(); // field2
		}
	}

	void render(safe_ptr<AVFrame> av_frame, int image_data_size)
	{
		if(av_frame->linesize[0] != static_cast<int>(format_desc_.width*4))
		{
			const uint8_t *src_data[4] = {0};
			memcpy(const_cast<uint8_t**>(&src_data[0]), av_frame->data, 4);
			const int src_linesizes[4] = {0};
			memcpy(const_cast<int*>(&src_linesizes[0]), av_frame->linesize, 4);

			auto av_frame2 = get_av_frame();
			av_image_alloc(av_frame2->data, av_frame2->linesize, av_frame2->width, av_frame2->height, PIX_FMT_BGRA, 16);
			av_frame = safe_ptr<AVFrame>(av_frame2.get(), [=](AVFrame*)
			{
				av_freep(&av_frame2->data[0]);
			});

			av_image_copy(av_frame2->data, av_frame2->linesize, src_data, src_linesizes, PIX_FMT_BGRA, av_frame2->width, av_frame2->height);
		}

		glBindTexture(GL_TEXTURE_2D, texture_);

		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbos_[0]);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, format_desc_.width, format_desc_.height, GL_BGRA, GL_UNSIGNED_BYTE, 0);

		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbos_[1]);
		glBufferData(GL_PIXEL_UNPACK_BUFFER, format_desc_.size, 0, GL_STREAM_DRAW);

		auto ptr = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
		if(ptr)
		{
			if(config_.key_only)
				fast_memshfl(reinterpret_cast<char*>(ptr), av_frame->data[0], image_data_size, 0x0F0F0F0F, 0x0B0B0B0B, 0x07070707, 0x03030303);
			else
				fast_memcpy(reinterpret_cast<char*>(ptr), av_frame->data[0], image_data_size);

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

	boost::unique_future<bool> send(const safe_ptr<core::read_frame>& frame)
	{
		if (!frame_buffer_.try_push(frame))
			graph_->set_tag("dropped-frame"); 

		return wrap_as_future(is_running_.load());
	}

	std::wstring channel_and_format() const
	{
		return L"[" + boost::lexical_cast<std::wstring>(channel_index_) + L"|" + format_desc_.name + L"]";
	}
		
	std::wstring print() const
	{	
		return config_.name + channel_and_format();
	}
	
	void calculate_aspect()
	{
		if(config_.windowed)
		{
			screen_height_ = window_.GetHeight();
			screen_width_ = window_.GetWidth();
		}
		
		GL(glViewport(0, 0, screen_width_, screen_height_));

		std::pair<float, float> target_ratio = None();
		if(config_.stretch == fill)
			target_ratio = Fill();
		else if(config_.stretch == uniform)
			target_ratio = Uniform();
		else if(config_.stretch == uniform_to_fill)
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
};


struct ogl_consumer_proxy : public core::frame_consumer
{
	const configuration config_;
	std::unique_ptr<ogl_consumer> consumer_;

public:

	ogl_consumer_proxy(const configuration& config)
		: config_(config){}
	
	~ogl_consumer_proxy()
	{
		if(consumer_)
		{
			auto str = print();
			consumer_.reset();
			CASPAR_LOG(info) << str << L" Successfully Uninitialized.";	
		}
	}

	// frame_consumer

	virtual void initialize(const core::video_format_desc& format_desc, int channel_index) override
	{
		consumer_.reset();
		consumer_.reset(new ogl_consumer(config_, format_desc, channel_index));
		CASPAR_LOG(info) << print() << L" Successfully Initialized.";	
	}
	
	virtual boost::unique_future<bool> send(const safe_ptr<core::read_frame>& frame) override
	{
		return consumer_->send(frame);
	}
	
	virtual std::wstring print() const override
	{
		return consumer_ ? consumer_->print() : L"[ogl_consumer]";
	}

	virtual boost::property_tree::wptree info() const override
	{
		boost::property_tree::wptree info;
		info.add(L"type", L"ogl-consumer");
		info.add(L"key-only", config_.key_only);
		info.add(L"windowed", config_.windowed);
		info.add(L"auto-deinterlace", config_.auto_deinterlace);
		return info;
	}

	virtual bool has_synchronization_clock() const override
	{
		return false;
	}
	
	virtual size_t buffer_depth() const override
	{
		return 1;
	}

	virtual int index() const override
	{
		return 600 + config_.screen_index;
	}
};	

safe_ptr<core::frame_consumer> create_consumer(const std::vector<std::wstring>& params)
{
	if(params.size() < 1 || params[0] != L"SCREEN")
		return core::frame_consumer::empty();
	
	configuration config;
		
	if(params.size() > 1)
		config.screen_index = lexical_cast_or_default<int>(params[1], config.screen_index);

	auto device_it = std::find(params.begin(), params.end(), L"DEVICE");
	if(device_it != params.end() && ++device_it != params.end())
		config.screen_index = boost::lexical_cast<int>(*device_it);
		
	config.windowed = std::find(params.begin(), params.end(), L"FULLSCREEN") == params.end();
	config.key_only = std::find(params.begin(), params.end(), L"KEY_ONLY") != params.end();

	auto name_it	= std::find(params.begin(), params.end(), L"NAME");
	if(name_it != params.end() && ++name_it != params.end())
		config.name = *name_it;

	return make_safe<ogl_consumer_proxy>(config);
}

safe_ptr<core::frame_consumer> create_consumer(const boost::property_tree::wptree& ptree) 
{
	configuration config;
	config.name				= ptree.get(L"name",	 config.name);
	config.screen_index		= ptree.get(L"device",   config.screen_index+1)-1;
	config.windowed			= ptree.get(L"windowed", config.windowed);
	config.key_only			= ptree.get(L"key-only", config.key_only);
	config.auto_deinterlace	= ptree.get(L"auto-deinterlace", config.auto_deinterlace);
	config.vsync			= ptree.get(L"vsync", config.vsync);

	auto stretch_str = ptree.get(L"stretch", L"default");
	if(stretch_str == L"uniform")
		config.stretch = stretch::uniform;
	else if(stretch_str == L"uniform_to_fill")
		config.stretch = stretch::uniform_to_fill;

	auto aspect_str = ptree.get(L"aspect-ratio", L"default");
	if(aspect_str == L"16:9")
		config.aspect = configuration::aspect_16_9;
	else if(aspect_str == L"4:3")
		config.aspect = configuration::aspect_4_3;
	
	return make_safe<ogl_consumer_proxy>(config);
}

}}