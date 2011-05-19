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
		core::pixel_format_desc desc;
		std::vector<safe_ptr<device_buffer>> textures;
		core::image_transform transform;
	};

	const core::video_format_desc format_desc_;
	
	std::stack<core::image_transform>		transform_stack_;
	std::vector<std::vector<render_item>>	transferring_queue_;
	std::vector<std::vector<render_item>>	render_queue_;
	
	image_kernel kernel_;
		
	safe_ptr<host_buffer>	read_buffer_;
	
	safe_ptr<device_buffer>	draw_buffer_;
	safe_ptr<device_buffer>	write_buffer_;

	safe_ptr<device_buffer>	local_key_buffer_;
	safe_ptr<device_buffer>	layer_key_buffer_;

	bool local_key_;
	bool layer_key_;

public:
	implementation(const core::video_format_desc& format_desc) 
		: format_desc_(format_desc)
		, read_buffer_(ogl_device::create_host_buffer(format_desc_.size, host_buffer::read_only))
		, draw_buffer_(ogl_device::create_device_buffer(format_desc.width, format_desc.height, 4))
		, write_buffer_	(ogl_device::create_device_buffer(format_desc.width, format_desc.height, 4))
		, local_key_buffer_(ogl_device::create_device_buffer(format_desc.width, format_desc.height, 1))
		, layer_key_buffer_(ogl_device::create_device_buffer(format_desc.width, format_desc.height, 1))
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
		auto desc		= frame.get_pixel_format_desc();
		auto buffers	= frame.get_plane_buffers();
		auto transform	= transform_stack_.top()*frame.get_image_transform();

		ogl_device::begin_invoke([=]
		{
			render_item item;

			item.desc = desc;
			item.transform = transform;
			
			// Start transfer from host to device.

			for(size_t n = 0; n < buffers.size(); ++n)
			{
				auto texture = ogl_device::create_device_buffer(desc.planes[n].width, desc.planes[n].height, desc.planes[n].channels);
				texture->read(*buffers[n]);
				item.textures.push_back(texture);
			}	

			transferring_queue_.back().push_back(item);
		});
	}

	void end()
	{
		transform_stack_.pop();
	}

	void begin_layer()
	{
		ogl_device::begin_invoke([=]
		{
			transferring_queue_.push_back(std::vector<render_item>());
		});
	}

	void end_layer()
	{
	}

	boost::unique_future<safe_ptr<const host_buffer>> render()
	{
		auto result = ogl_device::begin_invoke([=]() -> safe_ptr<const host_buffer>
		{
			read_buffer_->map(); // Might block.
			return read_buffer_;
		});
			
		ogl_device::begin_invoke([=]
		{
			local_key_ = false;
			layer_key_ = false;

			// Clear buffers.
			local_key_buffer_->clear();
			layer_key_buffer_->clear();
			draw_buffer_->clear();

			// Draw items in device.

			BOOST_FOREACH(auto layer, render_queue_)
			{
				draw_buffer_->attach();	

				BOOST_FOREACH(auto item, layer)			
					draw(item);	
								
				layer_key_ = local_key_; // If there was only key in last layer then use it as key for the entire next layer.
				local_key_ = false;

				std::swap(local_key_buffer_, layer_key_buffer_);
			}

			// Move waiting items to queue.

			render_queue_ = std::move(transferring_queue_);

			// Start transfer from device to host.

			read_buffer_ = ogl_device::create_host_buffer(format_desc_.size, host_buffer::read_only);
			draw_buffer_->write(*read_buffer_);

			std::swap(draw_buffer_, write_buffer_);
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
			
	std::vector<safe_ptr<host_buffer>> create_buffers(const core::pixel_format_desc& format)
	{
		std::vector<safe_ptr<host_buffer>> buffers;
		std::transform(format.planes.begin(), format.planes.end(), std::back_inserter(buffers), [&](const core::pixel_format_desc::plane& plane)
		{
			return ogl_device::create_host_buffer(plane.size, host_buffer::write_only);
		});
		return buffers;
	}
};

image_mixer::image_mixer(const core::video_format_desc& format_desc) : impl_(new implementation(format_desc)){}
void image_mixer::begin(const core::basic_frame& frame){impl_->begin(frame);}
void image_mixer::visit(core::write_frame& frame){impl_->visit(frame);}
void image_mixer::end(){impl_->end();}
boost::unique_future<safe_ptr<const host_buffer>> image_mixer::render(){return impl_->render();}
std::vector<safe_ptr<host_buffer>> image_mixer::create_buffers(const core::pixel_format_desc& format){return impl_->create_buffers(format);}
void image_mixer::begin_layer(){impl_->begin_layer();}
void image_mixer::end_layer(){impl_->end_layer();}

}}