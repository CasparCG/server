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
#include "../write_frame.h"
#include "../gpu/ogl_device.h"
#include "../gpu/host_buffer.h"
#include "../gpu/device_buffer.h"
#include "../../video_channel_context.h"

#include <common/exception/exceptions.h>
#include <common/gl/gl_check.h>
#include <common/utility/move_on_copy.h>

#include <core/producer/frame/frame_transform.h>
#include <core/producer/frame/pixel_format.h>
#include <core/video_format.h>

#include <gl/glew.h>

#include <boost/foreach.hpp>

#include <algorithm>
#include <deque>

using namespace boost::assign;

namespace caspar { namespace core {
	
struct layer
{
	std::vector<render_item> items;
	blend_mode::type		 blend_mode;

	layer(blend_mode::type blend_mode) : blend_mode(blend_mode)
	{
	}
};

class image_renderer
{
	video_channel_context&					channel_;	
	image_kernel							kernel_;	
	std::shared_ptr<device_buffer>			active_buffer_;
public:
	image_renderer(video_channel_context& channel)
		: channel_(channel)
	{
	}
	
	boost::unique_future<safe_ptr<host_buffer>> render(std::vector<layer>&& layers)
	{		
		auto layers2 = make_move_on_copy(std::move(layers));
		return channel_.ogl().begin_invoke([=]
		{
			return do_render(std::move(layers2.value));
		});
	}
	
private:
	safe_ptr<host_buffer> do_render(std::vector<layer>&& layers)
	{
		std::shared_ptr<device_buffer> layer_key_buffer;

		auto draw_buffer = create_device_buffer(4);
				
		BOOST_FOREACH(auto& layer, layers)
			draw_layer(std::move(layer), draw_buffer, layer_key_buffer);
		
		auto host_buffer = channel_.ogl().create_host_buffer(channel_.get_format_desc().size, host_buffer::read_only);
		channel_.ogl().attach(*draw_buffer);
		host_buffer->begin_read(draw_buffer->width(), draw_buffer->height(), format(draw_buffer->stride()));
		
		active_buffer_ = draw_buffer;

		channel_.ogl().flush(); // NOTE: This is important, otherwise fences will deadlock.
			
		return host_buffer;
	}

	void draw_layer(layer&& layer, const safe_ptr<device_buffer>& draw_buffer, std::shared_ptr<device_buffer>& layer_key_buffer)
	{				
		if(layer.items.empty())
			return;

		std::pair<int, std::shared_ptr<device_buffer>> local_key_buffer = std::make_pair(0, nullptr); // int is fields flag
				
		if(layer.blend_mode != blend_mode::normal && has_overlapping_items(layer))
		{
			auto layer_draw_buffer = create_device_buffer(4); // int is fields flag

			BOOST_FOREACH(auto& item, layer.items)
				draw_item(std::move(item), *layer_draw_buffer, local_key_buffer, layer_key_buffer);		
						
			render_item item;
			item.pix_desc.pix_fmt	= pixel_format::bgra;
			item.pix_desc.planes	= list_of(pixel_format_desc::plane(channel_.get_format_desc().width, channel_.get_format_desc().height, 4));
			item.textures			= list_of(layer_draw_buffer);
			item.transform			= frame_transform();
			item.blend_mode			= layer.blend_mode;

			kernel_.draw(channel_.ogl(), std::move(item), *draw_buffer, nullptr, nullptr);
		}
		else // fast path
		{
			BOOST_FOREACH(auto& item, layer.items)		
				draw_item(std::move(item), *draw_buffer, local_key_buffer, layer_key_buffer);		
		}					

		CASPAR_ASSERT(local_key_buffer.first == 0 || local_key_buffer.first == core::field_mode::progressive);

		std::swap(local_key_buffer.second, layer_key_buffer);
	}

	void draw_item(render_item&&									item, 
				   device_buffer&									draw_buffer, 
				   std::pair<int, std::shared_ptr<device_buffer>>&	local_key_buffer, 
				   std::shared_ptr<device_buffer>&					layer_key_buffer)
	{											
		if(item.transform.is_key)
		{
			if(!local_key_buffer.second)
			{
				local_key_buffer.first = 0;
				local_key_buffer.second = create_device_buffer(1);
			}
			
			local_key_buffer.first |= item.transform.field_mode; // Add field to flag.
			kernel_.draw(channel_.ogl(), std::move(item), *local_key_buffer.second, nullptr, nullptr);
		}
		else
		{
			kernel_.draw(channel_.ogl(), std::move(item), draw_buffer, local_key_buffer.second, layer_key_buffer);
			local_key_buffer.first ^= item.transform.field_mode; // Remove field from flag.
			
			if(local_key_buffer.first == 0) // If all fields from key has been used, reset it
			{
				local_key_buffer.first = 0;
				local_key_buffer.second.reset();
			}
		}
	}

	bool has_overlapping_items(const layer& layer)
	{		
		auto upper_count = boost::range::count_if(layer.items, [&](const render_item& item)
		{
			return !item.transform.is_key && (item.transform.field_mode & field_mode::upper);
		});

		auto lower_count = boost::range::count_if(layer.items, [&](const render_item& item)
		{
			return  !item.transform.is_key && (item.transform.field_mode & field_mode::lower);
		});

		return upper_count > 1 || lower_count > 1;
	}			
		
	safe_ptr<device_buffer> create_device_buffer(size_t stride)
	{
		auto buffer = channel_.ogl().create_device_buffer(channel_.get_format_desc().width, channel_.get_format_desc().height, stride);
		channel_.ogl().clear(*buffer);
		return buffer;
	}
};

		
struct image_mixer::implementation : boost::noncopyable
{	
	ogl_device&						ogl_;
	image_renderer					renderer_;
	std::vector<frame_transform>	transform_stack_;
	std::vector<layer>				layers_; // layer/stream/items
public:
	implementation(video_channel_context& video_channel) 
		: ogl_(video_channel.ogl())
		, renderer_(video_channel)
		, transform_stack_(1)	
	{
	}

	void begin_layer(blend_mode::type blend_mode)
	{
		layers_.push_back(layer(blend_mode));
	}
		
	void begin(core::basic_frame& frame)
	{
		transform_stack_.push_back(transform_stack_.back()*frame.get_frame_transform());
	}
		
	void visit(core::write_frame& frame)
	{	
		if(transform_stack_.back().field_mode == field_mode::empty)
			return;
		
		core::render_item item;
		item.pix_desc	= frame.get_pixel_format_desc();
		item.textures	= frame.get_textures();
		item.transform	= transform_stack_.back();

		layers_.back().items.push_back(item);
	}

	void end()
	{
		transform_stack_.pop_back();
	}

	void end_layer()
	{		
	}
	
	boost::unique_future<safe_ptr<host_buffer>> render()
	{
		return renderer_.render(std::move(layers_));
	}

	safe_ptr<write_frame> create_frame(const void* tag, const core::pixel_format_desc& desc)
	{
		return make_safe<write_frame>(ogl_, tag, desc);
	}
};

image_mixer::image_mixer(video_channel_context& video_channel) : impl_(new implementation(video_channel)){}
void image_mixer::begin(core::basic_frame& frame){impl_->begin(frame);}
void image_mixer::visit(core::write_frame& frame){impl_->visit(frame);}
void image_mixer::end(){impl_->end();}
boost::unique_future<safe_ptr<host_buffer>> image_mixer::render(){return impl_->render();}
safe_ptr<write_frame> image_mixer::create_frame(const void* tag, const core::pixel_format_desc& desc){return impl_->create_frame(tag, desc);}
void image_mixer::begin_layer(blend_mode::type blend_mode){impl_->begin_layer(blend_mode);}
void image_mixer::end_layer(){impl_->end_layer();}
image_mixer& image_mixer::operator=(image_mixer&& other)
{
	impl_ = std::move(other.impl_);
	return *this;
}

}}