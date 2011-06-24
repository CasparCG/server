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

#include "../../video_channel_context.h"

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
		
struct render_item
{
	pixel_format_desc					 desc;
	std::vector<safe_ptr<device_buffer>> textures;
	core::image_transform				 transform;
	int									 tag;
};

struct image_mixer::implementation : boost::noncopyable
{	
	static const size_t LOCAL_KEY_INDEX = 3;
	static const size_t LAYER_KEY_INDEX = 4;
	
	video_channel_context&					channel_;
	
	std::stack<core::image_transform>		transform_stack_;

	std::queue<std::queue<std::queue<render_item>>>	render_queue_; // layer/stream/items
	
	image_kernel							kernel_;
		
	safe_ptr<device_buffer>					draw_buffer_;
	safe_ptr<device_buffer>					write_buffer_;

	safe_ptr<device_buffer>					stream_key_buffer_;
	safe_ptr<device_buffer>					layer_key_buffer_;

	bool									local_key_;
	bool									layer_key_;
	
public:
	implementation(video_channel_context& video_channel) 
		: channel_(video_channel)
		, draw_buffer_(video_channel.ogl().create_device_buffer(video_channel.get_format_desc().width, channel_.get_format_desc().height, 4))
		, write_buffer_	(video_channel.ogl().create_device_buffer(video_channel.get_format_desc().width, channel_.get_format_desc().height, 4))
		, stream_key_buffer_(video_channel.ogl().create_device_buffer(video_channel.get_format_desc().width, channel_.get_format_desc().height, 1))
		, layer_key_buffer_(video_channel.ogl().create_device_buffer(video_channel.get_format_desc().width, channel_.get_format_desc().height, 1))
		, local_key_(false)
		, layer_key_(false)
	{
		transform_stack_.push(core::image_transform());

		channel_.ogl().invoke([=]
		{
			if(!GLEE_VERSION_3_0)
				CASPAR_LOG(warning) << "Missing OpenGL 3.0 support.";//BOOST_THROW_EXCEPTION(not_supported() << msg_info("Missing OpenGL 3.0 support."));
		});
	}

	void begin(core::basic_frame& frame)
	{
		transform_stack_.push(transform_stack_.top()*frame.get_image_transform());
	}
		
	void visit(core::write_frame& frame)
	{			
		render_item item = {frame.get_pixel_format_desc(), frame.get_textures(), transform_stack_.top()*frame.get_image_transform(), frame.tag()};	

		auto& layer = render_queue_.back();

		if(layer.empty() || (!layer.back().empty() && layer.back().back().tag != frame.tag()))
			layer.push(std::queue<render_item>());
		
		layer.back().push(item);
	}

	void end()
	{
		transform_stack_.pop();
	}

	void begin_layer()
	{
		render_queue_.push(std::queue<std::queue<render_item>>());
	}

	void end_layer()
	{
	}

	void reinitialize_buffers()
	{
		draw_buffer_	   = channel_.ogl().create_device_buffer(channel_.get_format_desc().width, channel_.get_format_desc().height, 4);
		write_buffer_	   = channel_.ogl().create_device_buffer(channel_.get_format_desc().width, channel_.get_format_desc().height, 4);
		stream_key_buffer_ = channel_.ogl().create_device_buffer(channel_.get_format_desc().width, channel_.get_format_desc().height, 1);
		layer_key_buffer_  = channel_.ogl().create_device_buffer(channel_.get_format_desc().width, channel_.get_format_desc().height, 1);
		channel_.ogl().gc();
	}

	safe_ptr<host_buffer> render()
	{		
		auto render_queue = std::move(render_queue_);

		return channel_.ogl().invoke([=]() mutable -> safe_ptr<host_buffer>
		{
			if(draw_buffer_->width() != channel_.get_format_desc().width || draw_buffer_->height() != channel_.get_format_desc().height)
				reinitialize_buffers();
			
			return do_render(std::move(render_queue));
		});
	}
	
	safe_ptr<host_buffer> do_render(std::queue<std::queue<std::queue<render_item>>>&& render_queue)
	{
		auto read_buffer = channel_.ogl().create_host_buffer(channel_.get_format_desc().size, host_buffer::read_only);

		layer_key_buffer_->clear();
		draw_buffer_->clear();

		while(!render_queue.empty())
		{			
			stream_key_buffer_->clear();

			auto layer = std::move(render_queue.front());
			render_queue.pop();
											
			while(!layer.empty())
			{
				auto stream = std::move(layer.front());
				layer.pop();

				bool last_is_key = stream.empty() ? false : stream.back().transform.get_is_key();

				while(!stream.empty())
				{
					auto item = std::move(stream.front());
					stream.pop();

					render(item);

					channel_.ogl().yield(); // Allow quick buffer allocation to execute.
				}		

				if(!last_is_key)
					stream_key_buffer_->clear();
			}

			std::swap(stream_key_buffer_, layer_key_buffer_);
		}

		std::swap(draw_buffer_, write_buffer_);

		// Start transfer from device to host.				
		write_buffer_->write(*read_buffer);

		return read_buffer;
	}

	void render(render_item& item)
	{
		for(size_t n = 0; n < item.textures.size(); ++n)
			item.textures[n]->bind(n);	
				
		if(item.transform.get_is_key())
		{
			stream_key_buffer_->attach();

			kernel_.draw(channel_.get_format_desc().width, channel_.get_format_desc().height, item.desc, item.transform, false, false);	
		}
		else
		{
			stream_key_buffer_->bind(LOCAL_KEY_INDEX);	
			layer_key_buffer_->bind(LAYER_KEY_INDEX);
				
			draw_buffer_->attach();	

			kernel_.draw(channel_.get_format_desc().width, channel_.get_format_desc().height, item.desc, item.transform, !stream_key_buffer_->empty(), !layer_key_buffer_->empty());	
		}
	}
				
	safe_ptr<write_frame> create_frame(const void* tag, const core::pixel_format_desc& desc)
	{
		return make_safe<write_frame>(channel_.ogl(), reinterpret_cast<int>(tag), desc);
	}
};

image_mixer::image_mixer(video_channel_context& video_channel) : impl_(new implementation(video_channel)){}
void image_mixer::begin(core::basic_frame& frame){impl_->begin(frame);}
void image_mixer::visit(core::write_frame& frame){impl_->visit(frame);}
void image_mixer::end(){impl_->end();}
safe_ptr<host_buffer> image_mixer::render(){return impl_->render();}
safe_ptr<write_frame> image_mixer::create_frame(const void* tag, const core::pixel_format_desc& desc){return impl_->create_frame(tag, desc);}
void image_mixer::begin_layer(){impl_->begin_layer();}
void image_mixer::end_layer(){impl_->end_layer();}
image_mixer& image_mixer::operator=(image_mixer&& other)
{
	impl_ = std::move(other.impl_);
	return *this;
}

}}