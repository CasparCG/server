/*
* Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
*
* This file is part of CasparCG (www.casparcg.com).
*
* CasparCG is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* CasparCG is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
*
* Author: Robert Nagy, ronag89@gmail.com
*/

#include "../../../stdafx.h"

#include "image_mixer.h"

#include "image_kernel.h"

#include "../write_frame.h"
#include "../accelerator.h"
#include "../host_buffer.h"
#include "../device_buffer.h"

#include <common/gl/gl_check.h>

#include <core/frame/frame_transform.h>
#include <core/frame/pixel_format.h>
#include <core/video_format.h>

#include <gl/glew.h>

#include <boost/foreach.hpp>
#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/thread/future.hpp>

#include <algorithm>
#include <vector>

using namespace boost::assign;

namespace caspar { namespace core { namespace gpu {
	
struct item
{
	pixel_format_desc											pix_desc;
	std::vector<boost::shared_future<safe_ptr<device_buffer>>>	textures;
	frame_transform												transform;

	item()
		: pix_desc(pixel_format::invalid)
	{
	}
};

typedef std::pair<blend_mode, std::vector<item>> layer;

class image_renderer
{
	safe_ptr<accelerator>			ogl_;
	image_kernel					kernel_;	
public:
	image_renderer(const safe_ptr<accelerator>& ogl)
		: ogl_(ogl)
		, kernel_(ogl_)
	{
	}
	
	boost::unique_future<safe_ptr<host_buffer>> operator()(std::vector<layer> layers, const video_format_desc& format_desc)
	{	
		return ogl_->begin_invoke([=]() mutable -> safe_ptr<host_buffer>
		{
			auto draw_buffer = create_mixer_buffer(4, format_desc);

			if(format_desc.field_mode != field_mode::progressive)
			{
				auto upper = layers;
				auto lower = std::move(layers);

				BOOST_FOREACH(auto& layer, upper)
				{
					BOOST_FOREACH(auto& item, layer.second)
						item.transform.field_mode = static_cast<field_mode>(item.transform.field_mode & field_mode::upper);
				}

				BOOST_FOREACH(auto& layer, lower)
				{
					BOOST_FOREACH(auto& item, layer.second)
						item.transform.field_mode = static_cast<field_mode>(item.transform.field_mode & field_mode::lower);
				}

				draw(std::move(upper), draw_buffer, format_desc);
				draw(std::move(lower), draw_buffer, format_desc);
			}
			else
			{
				draw(std::move(layers), draw_buffer, format_desc);
			}
			
			auto result = ogl_->create_host_buffer(static_cast<int>(format_desc.size), host_buffer::usage::read_only);
			draw_buffer->copy_to(result);							
			return result;
		});
	}

private:

	void draw(std::vector<layer>&&		layers, 
			  safe_ptr<device_buffer>&	draw_buffer, 
			  const video_format_desc& format_desc)
	{
		std::shared_ptr<device_buffer> layer_key_buffer;

		BOOST_FOREACH(auto& layer, layers)
			draw_layer(std::move(layer), draw_buffer, layer_key_buffer, format_desc);
	}

	void draw_layer(layer&&							layer, 
					safe_ptr<device_buffer>&		draw_buffer,
					std::shared_ptr<device_buffer>& layer_key_buffer,
					const video_format_desc&		format_desc)
	{				
		boost::remove_erase_if(layer.second, [](const item& item){return item.transform.field_mode == field_mode::empty;});

		if(layer.second.empty())
			return;

		std::shared_ptr<device_buffer> local_key_buffer;
		std::shared_ptr<device_buffer> local_mix_buffer;
				
		if(layer.first != blend_mode::normal)
		{
			auto layer_draw_buffer = create_mixer_buffer(4, format_desc);

			BOOST_FOREACH(auto& item, layer.second)
				draw_item(std::move(item), layer_draw_buffer, layer_key_buffer, local_key_buffer, local_mix_buffer, format_desc);	
		
			draw_mixer_buffer(layer_draw_buffer, std::move(local_mix_buffer), blend_mode::normal);							
			draw_mixer_buffer(draw_buffer, std::move(layer_draw_buffer), layer.first);
		}
		else // fast path
		{
			BOOST_FOREACH(auto& item, layer.second)		
				draw_item(std::move(item), draw_buffer, layer_key_buffer, local_key_buffer, local_mix_buffer, format_desc);		
					
			draw_mixer_buffer(draw_buffer, std::move(local_mix_buffer), blend_mode::normal);
		}					

		layer_key_buffer = std::move(local_key_buffer);
	}

	void draw_item(item&&							item, 
				   safe_ptr<device_buffer>&			draw_buffer, 
				   std::shared_ptr<device_buffer>&	layer_key_buffer, 
				   std::shared_ptr<device_buffer>&	local_key_buffer, 
				   std::shared_ptr<device_buffer>&	local_mix_buffer,
				   const video_format_desc&			format_desc)
	{			
		draw_params draw_params;
		draw_params.pix_desc				= std::move(item.pix_desc);
		BOOST_FOREACH(auto& future_texture, item.textures)
			draw_params.textures.push_back(future_texture.get());
		draw_params.transform				= std::move(item.transform);

		if(item.transform.is_key)
		{
			local_key_buffer = local_key_buffer ? local_key_buffer : create_mixer_buffer(1, format_desc);

			draw_params.background			= local_key_buffer;
			draw_params.local_key			= nullptr;
			draw_params.layer_key			= nullptr;

			kernel_.draw(std::move(draw_params));
		}
		else if(item.transform.is_mix)
		{
			local_mix_buffer = local_mix_buffer ? local_mix_buffer : create_mixer_buffer(4, format_desc);

			draw_params.background			= local_mix_buffer;
			draw_params.local_key			= std::move(local_key_buffer);
			draw_params.layer_key			= layer_key_buffer;

			draw_params.keyer				= keyer::additive;

			kernel_.draw(std::move(draw_params));
		}
		else
		{
			draw_mixer_buffer(draw_buffer, std::move(local_mix_buffer), blend_mode::normal);
			
			draw_params.background			= draw_buffer;
			draw_params.local_key			= std::move(local_key_buffer);
			draw_params.layer_key			= layer_key_buffer;

			kernel_.draw(std::move(draw_params));
		}	
	}

	void draw_mixer_buffer(safe_ptr<device_buffer>&			draw_buffer, 
						   std::shared_ptr<device_buffer>&& source_buffer, 
						   blend_mode						blend_mode = blend_mode::normal)
	{
		if(!source_buffer)
			return;

		draw_params draw_params;
		draw_params.pix_desc.format		= pixel_format::bgra;
		draw_params.pix_desc.planes		= list_of(pixel_format_desc::plane(source_buffer->width(), source_buffer->height(), 4));
		draw_params.textures			= list_of(source_buffer);
		draw_params.transform			= frame_transform();
		draw_params.blend_mode			= blend_mode;
		draw_params.background			= draw_buffer;

		kernel_.draw(std::move(draw_params));
	}
			
	safe_ptr<device_buffer> create_mixer_buffer(int stride, const video_format_desc& format_desc)
	{
		auto buffer = ogl_->create_device_buffer(format_desc.width, format_desc.height, stride);
		ogl_->clear(*buffer);
		return buffer;
	}
};
		
struct image_mixer::impl : boost::noncopyable
{	
	safe_ptr<accelerator>			ogl_;
	image_renderer					renderer_;
	std::vector<frame_transform>	transform_stack_;
	std::vector<layer>				layers_; // layer/stream/items
public:
	impl(const safe_ptr<accelerator>& ogl) 
		: ogl_(ogl)
		, renderer_(ogl)
		, transform_stack_(1)	
	{
	}

	void begin_layer(blend_mode blend_mode)
	{
		layers_.push_back(std::make_pair(blend_mode, std::vector<item>()));
	}
		
	void begin(draw_frame& frame)
	{
		transform_stack_.push_back(transform_stack_.back()*frame.get_frame_transform());
	}
		
	void visit(write_frame& frame)
	{			
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
	
	boost::unique_future<safe_ptr<host_buffer>> render(const video_format_desc& format_desc)
	{
		return renderer_(std::move(layers_), format_desc);
	}
};

image_mixer::image_mixer(const safe_ptr<accelerator>& ogl) : impl_(new impl(ogl)){}
void image_mixer::begin(draw_frame& frame){impl_->begin(frame);}
void image_mixer::visit(write_frame& frame){impl_->visit(frame);}
void image_mixer::end(){impl_->end();}
boost::unique_future<safe_ptr<host_buffer>> image_mixer::operator()(const video_format_desc& format_desc){return impl_->render(format_desc);}
void image_mixer::begin_layer(blend_mode blend_mode){impl_->begin_layer(blend_mode);}
void image_mixer::end_layer(){impl_->end_layer();}

}}}