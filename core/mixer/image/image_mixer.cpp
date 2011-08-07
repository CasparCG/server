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
#include <boost/range.hpp>

#include <algorithm>
#include <array>
#include <unordered_map>

namespace caspar { namespace core {
		
struct image_mixer::implementation : boost::noncopyable
{		
	typedef std::deque<render_item>					layer;

	video_channel_context&							channel_;

	std::vector<image_transform>					transform_stack_;
	std::vector<video_mode::type>					mode_stack_;

	std::queue<std::deque<render_item>>				layers_; // layer/stream/items
	
	image_kernel									kernel_;
		
	std::array<std::shared_ptr<device_buffer>,2>	draw_buffer_;
	std::shared_ptr<device_buffer>					write_buffer_;

	std::array<std::shared_ptr<device_buffer>,2>	stream_key_buffer_;
	std::shared_ptr<device_buffer>					layer_key_buffer_;
	
public:
	implementation(video_channel_context& video_channel) 
		: channel_(video_channel)
		, transform_stack_(1)
		, mode_stack_(1, video_mode::progressive)
	{
		initialize_buffers();
	}

	~implementation()
	{
		channel_.ogl().gc();
	}

	void initialize_buffers()
	{
		write_buffer_			= channel_.ogl().create_device_buffer(channel_.get_format_desc().width, channel_.get_format_desc().height, 4);
		layer_key_buffer_		= channel_.ogl().create_device_buffer(channel_.get_format_desc().width, channel_.get_format_desc().height, 1);
		draw_buffer_[0]			= channel_.ogl().create_device_buffer(channel_.get_format_desc().width, channel_.get_format_desc().height, 4);
		draw_buffer_[1]			= channel_.ogl().create_device_buffer(channel_.get_format_desc().width, channel_.get_format_desc().height, 4);
		stream_key_buffer_[0]	= channel_.ogl().create_device_buffer(channel_.get_format_desc().width, channel_.get_format_desc().height, 1);
		stream_key_buffer_[1]	= channel_.ogl().create_device_buffer(channel_.get_format_desc().width, channel_.get_format_desc().height, 1);
		channel_.ogl().gc();
	}

	void begin(core::basic_frame& frame)
	{
		transform_stack_.push_back(transform_stack_.back()*frame.get_image_transform());
		mode_stack_.push_back(frame.get_mode() == video_mode::progressive ? mode_stack_.back() : frame.get_mode());
	}
		
	void visit(core::write_frame& frame)
	{	
		CASPAR_ASSERT(!layers_.empty());

		// Check if frame has been discarded by interlacing
		if(boost::range::find(mode_stack_, video_mode::upper) != mode_stack_.end() && boost::range::find(mode_stack_, video_mode::lower) != mode_stack_.end())
			return;
		
		core::render_item item = {frame.get_pixel_format_desc(), frame.get_textures(), transform_stack_.back(), mode_stack_.back(), frame.tag()};	

		auto& layer = layers_.back();

		auto it = boost::range::find(layer, item);
		if(it == layer.end())
			layer.push_back(item);
	}

	void end()
	{
		transform_stack_.pop_back();
		mode_stack_.pop_back();
	}

	void begin_layer()
	{
		layers_.push(layer());
	}

	void end_layer()
	{
	}
	
	boost::unique_future<safe_ptr<host_buffer>> render()
	{		
		auto layers = std::move(layers_);

		return channel_.ogl().begin_invoke([=]() mutable -> safe_ptr<host_buffer>
		{			
			if(channel_.get_format_desc().width != write_buffer_->width() || channel_.get_format_desc().height != write_buffer_->height())
				initialize_buffers();

			return do_render(std::move(layers));
		});
	}
	
	safe_ptr<host_buffer> do_render(std::queue<layer>&& layers)
	{
		auto read_buffer = channel_.ogl().create_host_buffer(channel_.get_format_desc().size, host_buffer::read_only);

		layer_key_buffer_->clear();
		draw_buffer_[0]->clear();
		draw_buffer_[1]->clear();
		stream_key_buffer_[0]->clear();
		stream_key_buffer_[1]->clear();

		bool local_key = false;
		bool layer_key = false;

		while(!layers.empty())
		{			
			stream_key_buffer_[0]->clear();

			auto layer = std::move(layers.front());
			layers.pop();
			
			while(!layer.empty())
			{
				auto item = std::move(layer.front());
				layer.pop_front();
											
				if(item.transform.get_is_key())
				{
					render_item(stream_key_buffer_, std::move(item), nullptr, nullptr);
					local_key = true;
				}
				else
				{
					render_item(draw_buffer_, std::move(item), local_key ? stream_key_buffer_[0] : nullptr, layer_key ? layer_key_buffer_ : nullptr);	
					stream_key_buffer_[0]->clear();
					local_key = false;
				}
				channel_.ogl().yield(); // Return resources to pool as early as possible.
			}

			layer_key = local_key;
			local_key = false;
			std::swap(stream_key_buffer_[0], layer_key_buffer_);
		}

		std::swap(draw_buffer_[0], write_buffer_);

		// device -> host.			
		read_buffer->read(*write_buffer_);

		return read_buffer;
	}
	
	void render_item(std::array<std::shared_ptr<device_buffer>,2>& targets, render_item&& item, const std::shared_ptr<device_buffer>& local_key, const std::shared_ptr<device_buffer>& layer_key)
	{
		if(!std::all_of(item.textures.begin(), item.textures.end(), std::mem_fn(&device_buffer::ready)))
		{
			CASPAR_LOG(warning) << L"[image_mixer] Performance warning. Host to device transfer not complete, GPU will be stalled";
			channel_.ogl().yield(); // Try to give it some more time.
		}		

		targets[1]->attach();
			
		kernel_.draw(item, make_safe(targets[0]), local_key, layer_key);
		
		targets[0]->bind();

		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, targets[0]->width(), targets[0]->height());
		
		std::swap(targets[0], targets[1]);
	}
				
	safe_ptr<write_frame> create_frame(const void* tag, const core::pixel_format_desc& desc)
	{
		return make_safe<write_frame>(channel_.ogl(), tag, desc);
	}
};

image_mixer::image_mixer(video_channel_context& video_channel) : impl_(new implementation(video_channel)){}
void image_mixer::begin(core::basic_frame& frame){impl_->begin(frame);}
void image_mixer::visit(core::write_frame& frame){impl_->visit(frame);}
void image_mixer::end(){impl_->end();}
boost::unique_future<safe_ptr<host_buffer>> image_mixer::render(){return impl_->render();}
safe_ptr<write_frame> image_mixer::create_frame(const void* tag, const core::pixel_format_desc& desc){return impl_->create_frame(tag, desc);}
void image_mixer::begin_layer(){impl_->begin_layer();}
void image_mixer::end_layer(){impl_->end_layer();}
image_mixer& image_mixer::operator=(image_mixer&& other)
{
	impl_ = std::move(other.impl_);
	return *this;
}

}}