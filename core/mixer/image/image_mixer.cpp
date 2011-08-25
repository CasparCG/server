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
#include <boost/range/algorithm_ext/erase.hpp>

#include <algorithm>
#include <deque>

using namespace boost::assign;

namespace caspar { namespace core {
	
struct item
{
	pixel_format_desc						pix_desc;
	std::vector<safe_ptr<device_buffer>>	textures;
	frame_transform							transform;
};

typedef std::pair<blend_mode::type, std::vector<item>> layer;

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
		auto draw_buffer = create_device_buffer(4);

		if(channel_.get_format_desc().field_mode != field_mode::progressive)
		{
			auto upper = layers;
			auto lower = std::move(layers);

			BOOST_FOREACH(auto& layer, upper)
			{
				boost::remove_erase_if(layer.second, [](const item& item){return !(item.transform.field_mode & field_mode::upper);});
				BOOST_FOREACH(auto& item, layer.second)
					item.transform.field_mode = field_mode::upper;
			}

			BOOST_FOREACH(auto& layer, lower)
			{
				boost::remove_erase_if(layer.second, [](const item& item){return !(item.transform.field_mode & field_mode::lower);});
				BOOST_FOREACH(auto& item, layer.second)
					item.transform.field_mode = field_mode::lower;
			}

			draw(std::move(upper), draw_buffer);
			draw(std::move(lower), draw_buffer);
		}
		else
		{
			draw(std::move(layers), draw_buffer);
		}

		auto host_buffer = channel_.ogl().create_host_buffer(channel_.get_format_desc().size, host_buffer::read_only);
		channel_.ogl().attach(*draw_buffer);
		host_buffer->begin_read(draw_buffer->width(), draw_buffer->height(), format(draw_buffer->stride()));
		
		active_buffer_ = draw_buffer;

		channel_.ogl().flush(); // NOTE: This is important, otherwise fences will deadlock.
			
		return host_buffer;
	}

	void draw(std::vector<layer>&&		layers, 
			  safe_ptr<device_buffer>&	draw_buffer)
	{
		std::shared_ptr<device_buffer> layer_key_buffer;

		BOOST_FOREACH(auto& layer, layers)
			draw_layer(std::move(layer), draw_buffer, layer_key_buffer);
	}

	void draw_layer(layer&&							layer, 
					safe_ptr<device_buffer>&		draw_buffer,
					std::shared_ptr<device_buffer>& layer_key_buffer)
	{				
		if(layer.second.empty())
			return;

		std::shared_ptr<device_buffer> local_key_buffer;
		std::shared_ptr<device_buffer> local_mix_buffer;
				
		if(layer.first != blend_mode::normal && layer.second.size() > 1)
		{
			auto layer_draw_buffer = create_device_buffer(4);

			BOOST_FOREACH(auto& item, layer.second)
				draw_item(std::move(item), layer_draw_buffer, layer_key_buffer, local_key_buffer, local_mix_buffer);	
		
			draw_device_buffer(layer_draw_buffer, std::move(local_mix_buffer), blend_mode::normal);							
			draw_device_buffer(draw_buffer, std::move(layer_draw_buffer), layer.first);
		}
		else // fast path
		{
			BOOST_FOREACH(auto& item, layer.second)		
				draw_item(std::move(item), draw_buffer, layer_key_buffer, local_key_buffer, local_mix_buffer);		
					
			draw_device_buffer(draw_buffer, std::move(local_mix_buffer), blend_mode::normal);
		}					

		std::swap(local_key_buffer, layer_key_buffer);
	}

	void draw_item(item&&							item, 
				   safe_ptr<device_buffer>&			draw_buffer, 
				   std::shared_ptr<device_buffer>&	layer_key_buffer, 
				   std::shared_ptr<device_buffer>&	local_key_buffer, 
				   std::shared_ptr<device_buffer>&	local_mix_buffer)
	{			
		draw_params draw_params;
		draw_params.pix_desc				= std::move(item.pix_desc);
		draw_params.textures				= std::move(item.textures);
		draw_params.transform				= std::move(item.transform);

		if(item.transform.is_key)
		{
			local_key_buffer = local_key_buffer ? local_key_buffer : create_device_buffer(1);

			draw_params.background			= local_key_buffer;
			draw_params.local_key			= nullptr;
			draw_params.layer_key			= nullptr;

			kernel_.draw(channel_.ogl(), std::move(draw_params));
		}
		else if(item.transform.is_mix)
		{
			local_mix_buffer = local_mix_buffer ? local_mix_buffer : create_device_buffer(4);

			draw_params.background			= local_mix_buffer;
			draw_params.local_key			= std::move(local_key_buffer);
			draw_params.layer_key			= layer_key_buffer;

			draw_params.keyer				= keyer::additive;

			kernel_.draw(channel_.ogl(), std::move(draw_params));
		}
		else
		{
			draw_device_buffer(draw_buffer, std::move(local_mix_buffer), blend_mode::normal);
			
			draw_params.background			= draw_buffer;
			draw_params.local_key			= std::move(local_key_buffer);
			draw_params.layer_key			= layer_key_buffer;

			kernel_.draw(channel_.ogl(), std::move(draw_params));
		}	
	}

	void draw_device_buffer(safe_ptr<device_buffer>& draw_buffer, std::shared_ptr<device_buffer>&& source_buffer, blend_mode::type blend_mode = blend_mode::normal)
	{
		if(!source_buffer)
			return;

		draw_params draw_params;
		draw_params.pix_desc.pix_fmt	= pixel_format::bgra;
		draw_params.pix_desc.planes		= list_of(pixel_format_desc::plane(source_buffer->width(), source_buffer->height(), 4));
		draw_params.textures			= list_of(source_buffer);
		draw_params.transform			= frame_transform();
		draw_params.blend_mode			= blend_mode;
		draw_params.background			= draw_buffer;

		kernel_.draw(channel_.ogl(), std::move(draw_params));
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
		layers_.push_back(std::make_pair(blend_mode, std::vector<item>()));
	}
		
	void begin(core::basic_frame& frame)
	{
		transform_stack_.push_back(transform_stack_.back()*frame.get_frame_transform());
	}
		
	void visit(core::write_frame& frame)
	{			
		if(frame.get_frame_transform().field_mode == field_mode::empty)
			return;

		item item;
		item.pix_desc	= frame.get_pixel_format_desc();
		item.textures	= frame.get_textures();
		item.transform	= transform_stack_.back();

		layers_.back().second.push_back(item);
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