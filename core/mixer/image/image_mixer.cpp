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
#include "../gpu/gpu_write_frame.h"

#include <common/concurrency/executor.h>
#include <common/exception/exceptions.h>
#include <common/gl/gl_check.h>

#include <core/producer/frame/image_transform.h>
#include <core/producer/frame/pixel_format.h>
#include <core/video_format.h>

#include <boost/foreach.hpp>

#include <Glee.h>
#include <SFML/Window/Context.hpp>

#include <array>
#include <unordered_map>

namespace caspar { namespace mixer {
		
struct image_mixer::implementation : boost::noncopyable
{	
	struct render_item
	{
		core::pixel_format_desc desc;
		std::vector<safe_ptr<device_buffer>> textures;
		core::image_transform transform;
	};

	const core::video_format_desc format_desc_;
	
	std::stack<core::image_transform> transform_stack_;

	bool is_key_;

	safe_ptr<device_buffer>	front_target_;
	safe_ptr<device_buffer>	back_target_;
	safe_ptr<device_buffer>	key_target_;

	safe_ptr<host_buffer>	reading_;

	image_kernel kernel_;
		
	std::vector<render_item> waiting_queue_;
	std::vector<render_item> render_queue_;

public:
	implementation(const core::video_format_desc& format_desc) 
		: format_desc_(format_desc)
		, is_key_(false)
		, back_target_(ogl_device::create_device_buffer(format_desc.width, format_desc.height, 4))
		, front_target_(ogl_device::create_device_buffer(format_desc.width, format_desc.height, 4))
		, key_target_(ogl_device::create_device_buffer(format_desc.width, format_desc.height, 4))
		, reading_(ogl_device::create_host_buffer(format_desc_.size, host_buffer::read_only))
	{
		transform_stack_.push(core::image_transform());
	}
	
	void begin(const core::basic_frame& frame)
	{
		transform_stack_.push(transform_stack_.top()*frame.get_image_transform());
	}
		
	void visit(core::write_frame& frame)
	{		
		auto gpu_frame = dynamic_cast<gpu_write_frame*>(&frame);
		if(!gpu_frame)
			return;
		
		auto desc = gpu_frame->get_pixel_format_desc();
		auto buffers = gpu_frame->get_plane_buffers();
		auto transform = transform_stack_.top();

		ogl_device::begin_invoke([=]
		{
			render_item item;

			item.desc = desc;
			item.transform = transform;
			
			for(size_t n = 0; n < buffers.size(); ++n)
			{
				GL(glActiveTexture(GL_TEXTURE0+n));
				auto texture = ogl_device::create_device_buffer(desc.planes[n].width, desc.planes[n].height, desc.planes[n].channels);
				texture->read(*buffers[n]);
				item.textures.push_back(texture);
			}	

			waiting_queue_.push_back(item);
		});
	}

	void end()
	{
		transform_stack_.pop();
	}

	boost::unique_future<safe_ptr<const host_buffer>> render()
	{
		auto result = ogl_device::begin_invoke([=]() -> safe_ptr<const host_buffer>
		{
			reading_->map(); // Might block.
			return reading_;
		});
			
		ogl_device::begin_invoke([=]
		{
			is_key_ = false;

			// Clear and bind frame-buffers.

			key_target_->attach();
			GL(glClear(GL_COLOR_BUFFER_BIT));	

			front_target_->attach();
			GL(glClear(GL_COLOR_BUFFER_BIT));

			// Render items.

			BOOST_FOREACH(auto item, render_queue_)
				render(item);			

			// Move waiting items to queue.

			render_queue_ = std::move(waiting_queue_);

			// Start read-back.

			reading_ = ogl_device::create_host_buffer(format_desc_.size, host_buffer::read_only);
			front_target_->attach();
			front_target_->write(*reading_);
			std::swap(front_target_, back_target_);
		});

		return std::move(result);
	}

	void render(const render_item& item)
	{		
		const auto desc		 = item.desc;
		auto	   textures	 = item.textures;
		const auto transform = item.transform;
				
		// Bind textures

		for(size_t n = 0; n < textures.size(); ++n)
		{
			GL(glActiveTexture(GL_TEXTURE0+n));
			textures[n]->bind();
		}		

		// Setup key and kernel

		bool has_separate_key = false;

		if(transform.get_is_key())
		{
			if(!is_key_)
			{
				key_target_->attach();
				is_key_ = true;
				GL(glClear(GL_COLOR_BUFFER_BIT));		
			}
		}		
		else
		{						
			if(is_key_)
			{
				has_separate_key = true;
				is_key_ = false;

				front_target_->attach();			
				GL(glActiveTexture(GL_TEXTURE0+3));
				key_target_->bind();
			}	
		}	

		// Draw

		kernel_.draw(format_desc_.width, format_desc_.height, desc, transform, has_separate_key);	
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

}}