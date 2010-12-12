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

using namespace common::gl;
	
struct frame_renderer::implementation : boost::noncopyable
{	
	typedef std::pair<pbo_ptr, std::vector<short>> reading_frame;
	
	implementation(const video_format_desc& format_desc) : shader_(format_desc), format_desc_(format_desc),
		reading_(create_reading(std::vector<short>())), fbo_(format_desc.width, format_desc.height)
	{	
		GL(glEnable(GL_POLYGON_STIPPLE));
		GL(glEnable(GL_TEXTURE_2D));
		GL(glEnable(GL_BLEND));
		GL(glDisable(GL_DEPTH_TEST));
		GL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));			
		GL(glViewport(0, 0, format_desc.width, format_desc.height));
		reading_.first->begin_read();
	}
				
	read_frame render(const draw_frame& frame)
	{
		reading_frame result;
		try
		{
			detail::draw_frame_access::end_write(drawing_); // Map data pointer before returning to pool.
			drawing_ = writing_;
			writing_ = frame;
						
			detail::draw_frame_access::begin_write(writing_);
						
			reading_.first->end_read();
			result = reading_; 
						
			GL(glClear(GL_COLOR_BUFFER_BIT));
						
			detail::draw_frame_access::draw(drawing_, shader_);
				
			reading_ = create_reading(drawing_.audio_data());			
			reading_.first->begin_read();
						
			drawing_ = draw_frame::empty();
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
		}

		return read_frame(std::move(result.first), std::move(result.second));
	}

	reading_frame create_reading(const std::vector<short>& audio_data)
	{
		pbo_ptr new_pbo;
		if(!pbo_pool_.try_pop(new_pbo))		
			new_pbo = std::make_shared<pbo>(format_desc_.width, format_desc_.height, GL_BGRA);		
		new_pbo = pbo_ptr(new_pbo.get(), [=](pbo*){pbo_pool_.push(new_pbo);});
		return reading_frame(std::move(new_pbo), audio_data);
	}
	
	const video_format_desc format_desc_;
	const fbo fbo_;

	tbb::concurrent_bounded_queue<pbo_ptr> pbo_pool_;
	
	reading_frame	reading_;	
	draw_frame		writing_;
	draw_frame		drawing_;
	
	frame_shader shader_;
};
	
frame_renderer::frame_renderer(const video_format_desc& format_desc) : impl_(new implementation(format_desc)){}
read_frame frame_renderer::render(const draw_frame& frame){return impl_->render(std::move(frame));}
}}