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
#include "../../stdafx.h"

#include "image_mixer.h"
#include "image_kernel.h"

#include "../gpu/ogl_device.h"
#include "../gpu/host_buffer.h"
#include "../gpu/device_buffer.h"
#include "../write_frame.h"

#include <common/concurrency/executor.h>
#include <common/exception/exceptions.h>
#include <common/gl/gl_check.h>

#include <core/producer/frame/image_transform.h>
#include <core/producer/frame/pixel_format.h>
#include <core/video_format.h>

#include <boost/foreach.hpp>

#include <array>
#include <unordered_map>

namespace caspar { namespace core {
		
struct image_mixer::implementation : boost::noncopyable
{	
	static const size_t LOCAL_KEY_INDEX = 3;
	static const size_t LAYER_KEY_INDEX = 4;

	struct render_item
	{
		pixel_format_desc					 desc;
		std::vector<safe_ptr<device_buffer>> textures;
		core::image_transform				 transform;
	};

	const core::video_format_desc format_desc_;
	
	std::stack<core::image_transform>		transform_stack_;
	std::queue<std::queue<render_item>>		render_queue_;
	
	image_kernel kernel_;
	
	safe_ptr<ogl_device> ogl_;
	
	safe_ptr<host_buffer>	read_buffer_;
	safe_ptr<device_buffer>	draw_buffer_;
	safe_ptr<device_buffer>	write_buffer_;

	safe_ptr<device_buffer>	local_key_buffer_;
	safe_ptr<device_buffer>	layer_key_buffer_;

	bool local_key_;
	bool layer_key_;
	
public:
	implementation(const core::video_format_desc& format_desc, const safe_ptr<ogl_device>& ogl) 
		: format_desc_(format_desc)
		, ogl_(ogl)
		, read_buffer_(ogl_->create_host_buffer(format_desc_.size, host_buffer::read_only))
		, draw_buffer_(ogl_->create_device_buffer(format_desc.width, format_desc.height, 4))
		, write_buffer_	(ogl_->create_device_buffer(format_desc.width, format_desc.height, 4))
		, local_key_buffer_(ogl_->create_device_buffer(format_desc.width, format_desc.height, 1))
		, layer_key_buffer_(ogl_->create_device_buffer(format_desc.width, format_desc.height, 1))
		, local_key_(false)
		, layer_key_(false)
	{
		transform_stack_.push(core::image_transform());
	}
	
	void begin(const core::basic_frame& frame)
	{
		transform_stack_.push(transform_stack_.top()*frame.get_image_transform());
	}
		
	void visit(core::write_frame& frame)
	{			
		render_item item = {frame.get_pixel_format_desc(), frame.get_textures(), transform_stack_.top()*frame.get_image_transform()};	
		render_queue_.back().push(item);
	}

	void end()
	{
		transform_stack_.pop();
	}

	void begin_layer()
	{
		render_queue_.push(std::queue<render_item>());
	}

	void end_layer()
	{
	}

	boost::unique_future<safe_ptr<host_buffer>> render()
	{		
		auto read_buffer = read_buffer_;
		auto result = ogl_->begin_invoke([=]()  -> safe_ptr<host_buffer>
		{
			read_buffer->map();
			return read_buffer;
		});

		auto render_queue = std::move(render_queue_);

		ogl_->begin_invoke([=]() mutable
		{
			local_key_ = false;
			layer_key_ = false;

			// Clear buffers.
			local_key_buffer_->clear();
			layer_key_buffer_->clear();
			draw_buffer_->clear();

			// Draw items in device.

			while(!render_queue.empty())
			{
				auto layer = render_queue.front();
				render_queue.pop();
				
				draw_buffer_->attach();	

				while(!layer.empty())
				{
					draw(layer.front());
					layer.pop();
					ogl_->yield(); // Allow quick buffer allocation to execute.
				}

				layer_key_ = local_key_; // If there was only key in last layer then use it as key for the entire next layer.
				local_key_ = false;

				std::swap(local_key_buffer_, layer_key_buffer_);
			}

			std::swap(draw_buffer_, write_buffer_);

			// Start transfer from device to host.	
			read_buffer_ = ogl_->create_host_buffer(format_desc_.size, host_buffer::read_only);					
			write_buffer_->write(*read_buffer_);
		});

		return std::move(result);
	}
	
	void draw(const render_item& item)
	{		
		// Bind textures

		for(size_t n = 0; n < item.textures.size(); ++n)
			item.textures[n]->bind(n);		

		// Setup key and kernel

		bool local_key = false;
		bool layer_key = false;
		
		if(item.transform.get_is_key()) // This is a key frame, render it to the local_key buffer for later use.
		{
			if(!local_key_) // Initialize local-key if it is not active.
			{
				local_key_buffer_->clear();
				local_key_buffer_->attach();
				local_key_ = true;
			}
		}		
		else // This is a normal frame. Use key buffers if they are active.
		{		
			local_key = local_key_;
			layer_key = layer_key_;

			if(local_key_) // Use local key if we have it.
			{
				local_key_buffer_->bind(LOCAL_KEY_INDEX);
				draw_buffer_->attach();	
				local_key_ = false; // Use it only one time.
			}		

			if(layer_key_) // Use layer key if we have it.
				layer_key_buffer_->bind(LAYER_KEY_INDEX);
		}	

		// Draw

		kernel_.draw(format_desc_.width, format_desc_.height, item.desc, item.transform, local_key, layer_key);	
	}
			
	safe_ptr<write_frame> create_frame(void* tag, const core::pixel_format_desc& desc)
	{
		std::vector<safe_ptr<host_buffer>> buffers;
		std::vector<safe_ptr<device_buffer>> textures;
		ogl_->invoke([&]
		{
			std::transform(desc.planes.begin(), desc.planes.end(), std::back_inserter(buffers), [&](const core::pixel_format_desc::plane& plane)
			{
				return ogl_->create_host_buffer(plane.size, host_buffer::write_only);
			});
			std::transform(desc.planes.begin(), desc.planes.end(), std::back_inserter(textures), [&](const core::pixel_format_desc::plane& plane)
			{
				return ogl_->create_device_buffer(plane.width, plane.height, plane.channels);
			});
		});

		return make_safe<write_frame>(reinterpret_cast<int>(tag), desc, buffers, textures, ogl_);
	}
};

image_mixer::image_mixer(const core::video_format_desc& format_desc, const safe_ptr<ogl_device>& ogl) : impl_(new implementation(format_desc, ogl)){}
void image_mixer::begin(const core::basic_frame& frame){impl_->begin(frame);}
void image_mixer::visit(core::write_frame& frame){impl_->visit(frame);}
void image_mixer::end(){impl_->end();}
boost::unique_future<safe_ptr<host_buffer>> image_mixer::render(){return impl_->render();}
safe_ptr<write_frame> image_mixer::create_frame(void* tag, const core::pixel_format_desc& desc){return impl_->create_frame(tag, desc);}
void image_mixer::begin_layer(){impl_->begin_layer();}
void image_mixer::end_layer(){impl_->end_layer();}

}}