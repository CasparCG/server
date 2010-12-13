#include "../StdAfx.h"

#include "frame_renderer.h"

#include "frame_shader.h"
#include "write_frame.h"
#include "read_frame.h"
#include "draw_frame.h"
#include "read_frame.h"

#include "../format/video_format.h"

#include "../../common/exception/exceptions.h"
#include "../../common/gl/utility.h"
#include "../../common/gl/frame_buffer_object.h"

#include <Glee.h>

#include <tbb/concurrent_queue.h>

#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/range/algorithm.hpp>

#include <functional>

namespace caspar { namespace core {

struct frame_renderer::implementation : boost::noncopyable
{		
	implementation(const video_format_desc& format_desc) : shader_(format_desc), format_desc_(format_desc),
		reading_(create_reading()), fbo_(format_desc.width, format_desc.height), writing_(draw_frame::empty()), drawing_(draw_frame::empty())
	{	
		GL(glEnable(GL_POLYGON_STIPPLE));
		GL(glEnable(GL_TEXTURE_2D));
		GL(glEnable(GL_BLEND));
		GL(glDisable(GL_DEPTH_TEST));
		GL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));			
		GL(glViewport(0, 0, format_desc.width, format_desc.height));
		reading_->begin_read();
	}
				
	safe_ptr<read_frame> render(const safe_ptr<draw_frame>& frame)
	{
		safe_ptr<read_frame> result = create_reading();
		try
		{
			drawing_ = writing_;
			writing_ = frame;
						
			writing_->begin_write();
						
			reading_->end_read();
						
			GL(glClear(GL_COLOR_BUFFER_BIT));
						
			drawing_->draw(shader_);
				
			result.swap(reading_);
			reading_->begin_read();
			reading_->audio_data(drawing_->audio_data());
						
			drawing_ = draw_frame::empty();
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
		}

		return result;
	}

	safe_ptr<read_frame> create_reading()
	{
		std::shared_ptr<read_frame> frame;
		if(!pool_.try_pop(frame))		
			frame = std::make_shared<read_frame>(format_desc_.width, format_desc_.height);
		frame = std::shared_ptr<read_frame>(frame.get(), [=](read_frame*){pool_.push(frame);});
		return safe_ptr<read_frame>::from_shared(frame);
	}
	
	const video_format_desc format_desc_;
	const gl::fbo fbo_;

	tbb::concurrent_bounded_queue<std::shared_ptr<read_frame>> pool_;
	
	safe_ptr<read_frame>	reading_;	
	safe_ptr<draw_frame>	writing_;
	safe_ptr<draw_frame>	drawing_;
	
	frame_shader shader_;
};
	
frame_renderer::frame_renderer(const video_format_desc& format_desc) : impl_(new implementation(format_desc)){}
safe_ptr<read_frame> frame_renderer::render(const safe_ptr<draw_frame>& frame){return impl_->render(std::move(frame));}
}}