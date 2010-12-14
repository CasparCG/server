#include "../StdAfx.h"

#include "frame_renderer.h"

#include "frame_shader.h"
#include "write_frame.h"
#include "read_frame.h"

#include "../format/video_format.h"

#include "../../common/exception/exceptions.h"
#include "../../common/gl/utility.h"
#include "../../common/gl/frame_buffer_object.h"

#include <Glee.h>
#include <SFML/Window.hpp>

#include <tbb/concurrent_queue.h>

#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/range/algorithm.hpp>

#include <functional>

namespace caspar { namespace core {

struct ogl_context
{
	ogl_context(){context_.SetActive(true);};
	sf::Context context_;
};

struct frame_renderer::implementation : boost::noncopyable
{		
	implementation(const video_format_desc& format_desc) : shader_(format_desc), format_desc_(format_desc),
		fbo_(format_desc.width, format_desc.height), writing_(draw_frame::empty()), reading_(create_reading())
	{	
		reading_->unmap();
	}
				
	safe_ptr<const read_frame> render(safe_ptr<draw_frame>&& frame)
	{								
		frame->prepare(); // Start transfer from system memory to texture. End in next tick.
						
		reading_->map(); // Map texture to system memory.
		auto result = reading_;
						
		GL(glClear(GL_COLOR_BUFFER_BIT));
						
		writing_->draw(shader_); // Draw to frame buffer. Maps texture to system memory when done.
		writing_ = frame;				

		reading_ = create_reading();
		reading_->unmap(); // Start transfer from frame buffer to texture. End in next tick.
		reading_->audio_data(writing_->audio_data());						

		return result;
	}

	safe_ptr<read_frame> create_reading()
	{
		std::shared_ptr<read_frame> frame;
		if(!pool_.try_pop(frame))		
			frame = std::make_shared<read_frame>(format_desc_.width, format_desc_.height);
		return safe_ptr<read_frame>(frame.get(), [=](read_frame*){pool_.push(frame);});
	}

	const ogl_context context_;
	const gl::fbo fbo_;
	const video_format_desc format_desc_;

	tbb::concurrent_bounded_queue<std::shared_ptr<read_frame>> pool_;
	
	safe_ptr<read_frame>	reading_;	
	safe_ptr<draw_frame>	writing_;
	
	frame_shader shader_;
};
	
frame_renderer::frame_renderer(const video_format_desc& format_desc) : impl_(new implementation(format_desc)){}
safe_ptr<const read_frame> frame_renderer::render(safe_ptr<draw_frame>&& frame){return impl_->render(std::move(frame));}
}}