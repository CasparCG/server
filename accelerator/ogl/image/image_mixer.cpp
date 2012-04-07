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
#include "../../stdafx.h"

#include "image_mixer.h"

#include "image_kernel.h"

#include "../util/device.h"
#include "../util/buffer.h"
#include "../util/texture.h"

#include <common/gl/gl_check.h>
#include <common/future.h>
#include <common/array.h>

#include <core/frame/frame.h>
#include <core/frame/frame_transform.h>
#include <core/frame/pixel_format.h>
#include <core/video_format.h>

#include <asmlib.h>

#include <gl/glew.h>

#include <boost/foreach.hpp>
#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/thread/future.hpp>

#include <algorithm>
#include <vector>

using namespace boost::assign;

namespace caspar { namespace accelerator { namespace ogl {
		
typedef boost::shared_future<spl::shared_ptr<texture>> future_texture;

struct item
{
	core::pixel_format_desc								pix_desc;
	std::vector<future_texture>							textures;
	core::image_transform								transform;

	item()
		: pix_desc(core::pixel_format::invalid)
	{
	}
};

struct layer
{
	std::vector<item>	items;
	core::blend_mode	blend_mode;

	layer()
		: blend_mode(core::blend_mode::normal)
	{
	}

	layer(std::vector<item> items, core::blend_mode blend_mode)
		: items(std::move(items))
		, blend_mode(blend_mode)
	{
	}
};

class image_renderer
{
	spl::shared_ptr<device>	ogl_;
	image_kernel			kernel_;
public:
	image_renderer(const spl::shared_ptr<device>& ogl)
		: ogl_(ogl)
		, kernel_(ogl_)
	{
	}
	
	boost::unique_future<array<const std::uint8_t>> operator()(std::vector<layer> layers, const core::video_format_desc& format_desc)
	{	
		if(layers.empty())
		{ // Bypass GPU with empty frame.
			auto buffer = spl::make_shared<const std::vector<uint8_t, tbb::cache_aligned_allocator<uint8_t>>>(format_desc.size, 0);
			return async(launch::deferred, [=]
			{
				return array<const std::uint8_t>(buffer->data(), format_desc.size, true, buffer);
			});
		}		

		if(format_desc.field_mode != core::field_mode::progressive)
		{ // Remove jitter from still.
			BOOST_FOREACH(auto& layer, layers)
			{	
				// Remove first field stills.
				boost::range::remove_erase_if(layer.items, [&](const item& item)
				{
					return item.transform.is_still && item.transform.field_mode == format_desc.field_mode; // only us last field for stills.
				});
		
				// Stills are progressive
				BOOST_FOREACH(auto& item, layer.items)
				{
					if(item.transform.is_still)
						item.transform.field_mode = core::field_mode::progressive;
				}
			}
		}

		return flatten(ogl_->begin_invoke([=]() mutable -> boost::shared_future<array<const std::uint8_t>>
		{
			auto target_texture = ogl_->create_texture(format_desc.width, format_desc.height, 4);

			if(format_desc.field_mode != core::field_mode::progressive)
			{
				draw(target_texture, layers,			format_desc, core::field_mode::upper);
				draw(target_texture, std::move(layers), format_desc, core::field_mode::lower);
			}
			else			
				draw(target_texture, std::move(layers), format_desc, core::field_mode::progressive);
								
			return ogl_->copy_async(target_texture);
		}));
	}

private:	
	
	void draw(spl::shared_ptr<texture>&			target_texture, 
			  std::vector<layer>				layers, 
			  const core::video_format_desc&	format_desc,
			  core::field_mode					field_mode)
	{
		std::shared_ptr<texture> layer_key_texture;

		BOOST_FOREACH(auto& layer, layers)
			draw(target_texture, std::move(layer), layer_key_texture, format_desc, field_mode);
	}

	void draw(spl::shared_ptr<texture>&			target_texture,
			  layer								layer, 
			  std::shared_ptr<texture>&			layer_key_texture,
			  const core::video_format_desc&	format_desc,
			  core::field_mode					field_mode)
	{			
		// REMOVED: This is done in frame_muxer. 
		// Fix frames
		//BOOST_FOREACH(auto& item, layer.items)		
		//{
			//if(std::abs(item.transform.fill_scale[1]-1.0) > 1.0/target_texture->height() ||
			//   std::abs(item.transform.fill_translation[1]) > 1.0/target_texture->height())		
			//	CASPAR_LOG(warning) << L"[image_mixer] Frame should be deinterlaced. Send FILTER DEINTERLACE_BOB when creating producer.";	

			//if(item.pix_desc.planes.at(0).height == 480) // NTSC DV
			//{
			//	item.transform.fill_translation[1] += 2.0/static_cast<double>(format_desc.height);
			//	item.transform.fill_scale[1] *= 1.0 - 6.0*1.0/static_cast<double>(format_desc.height);
			//}
	
			//// Fix field-order if needed
			//if(item.field_mode == core::field_mode::lower && format_desc.field_mode == core::field_mode::upper)
			//	item.transform.fill_translation[1] += 1.0/static_cast<double>(format_desc.height);
			//else if(item.field_mode == core::field_mode::upper && format_desc.field_mode == core::field_mode::lower)
			//	item.transform.fill_translation[1] -= 1.0/static_cast<double>(format_desc.height);
		//}

		// Mask out fields
		BOOST_FOREACH(auto& item, layer.items)				
			item.transform.field_mode &= field_mode;
		
		// Remove empty items.
		boost::range::remove_erase_if(layer.items, [&](const item& item)
		{
			return item.transform.field_mode == core::field_mode::empty;
		});
		
		if(layer.items.empty())
			return;

		std::shared_ptr<texture> local_key_texture;
		std::shared_ptr<texture> local_mix_texture;
				
		if(layer.blend_mode != core::blend_mode::normal)
		{
			auto layer_texture = ogl_->create_texture(target_texture->width(), target_texture->height(), 4);

			BOOST_FOREACH(auto& item, layer.items)
				draw(layer_texture, std::move(item), layer_key_texture, local_key_texture, local_mix_texture);	
		
			draw(layer_texture, std::move(local_mix_texture), core::blend_mode::normal);							
			draw(target_texture, std::move(layer_texture), layer.blend_mode);
		}
		else // fast path
		{
			BOOST_FOREACH(auto& item, layer.items)		
				draw(target_texture, std::move(item), layer_key_texture, local_key_texture, local_mix_texture);		
					
			draw(target_texture, std::move(local_mix_texture), core::blend_mode::normal);
		}					

		layer_key_texture = std::move(local_key_texture);
	}

	void draw(spl::shared_ptr<texture>&	target_texture, 
			  item						item, 
		      std::shared_ptr<texture>&	layer_key_texture, 
			  std::shared_ptr<texture>&	local_key_texture, 
			  std::shared_ptr<texture>&	local_mix_texture)
	{			
		draw_params draw_params;
		draw_params.pix_desc	= std::move(item.pix_desc);
		draw_params.transform	= std::move(item.transform);

		BOOST_FOREACH(auto& future_texture, item.textures)
			draw_params.textures.push_back(future_texture.get());

		if(item.transform.is_key)
		{
			local_key_texture = local_key_texture ? local_key_texture : ogl_->create_texture(target_texture->width(), target_texture->height(), 1);

			draw_params.background			= local_key_texture;
			draw_params.local_key			= nullptr;
			draw_params.layer_key			= nullptr;

			kernel_.draw(std::move(draw_params));
		}
		else if(item.transform.is_mix)
		{
			local_mix_texture = local_mix_texture ? local_mix_texture : ogl_->create_texture(target_texture->width(), target_texture->height(), 4);

			draw_params.background			= local_mix_texture;
			draw_params.local_key			= std::move(local_key_texture);
			draw_params.layer_key			= layer_key_texture;

			draw_params.keyer				= keyer::additive;

			kernel_.draw(std::move(draw_params));
		}
		else
		{
			draw(target_texture, std::move(local_mix_texture), core::blend_mode::normal);
			
			draw_params.background			= target_texture;
			draw_params.local_key			= std::move(local_key_texture);
			draw_params.layer_key			= layer_key_texture;

			kernel_.draw(std::move(draw_params));
		}	
	}

	void draw(spl::shared_ptr<texture>&	 target_texture, 
			  std::shared_ptr<texture>&& source_buffer, 
			  core::blend_mode			 blend_mode = core::blend_mode::normal)
	{
		if(!source_buffer)
			return;

		draw_params draw_params;
		draw_params.pix_desc.format		= core::pixel_format::bgra;
		draw_params.pix_desc.planes		= list_of(core::pixel_format_desc::plane(source_buffer->width(), source_buffer->height(), 4));
		draw_params.textures			= list_of(source_buffer);
		draw_params.transform			= core::image_transform();
		draw_params.blend_mode			= blend_mode;
		draw_params.background			= target_texture;

		kernel_.draw(std::move(draw_params));
	}
};
		
struct image_mixer::impl : public core::frame_factory
{	
	spl::shared_ptr<device>				ogl_;
	image_renderer						renderer_;
	std::vector<core::image_transform>	transform_stack_;
	std::vector<layer>					layers_; // layer/stream/items
public:
	impl(const spl::shared_ptr<device>& ogl) 
		: ogl_(ogl)
		, renderer_(ogl)
		, transform_stack_(1)	
	{
		CASPAR_LOG(info) << L"Initialized OpenGL Accelerated GPU Image Mixer";
	}

	void begin_layer(core::blend_mode blend_mode)
	{
		layers_.push_back(layer(std::vector<item>(), blend_mode));
	}
		
	void push(const core::frame_transform& transform)
	{
		transform_stack_.push_back(transform_stack_.back()*transform.image_transform);
	}
		
	void visit(const core::const_frame& frame)
	{			
		if(frame.pixel_format_desc().format == core::pixel_format::invalid)
			return;

		if(frame.pixel_format_desc().planes.empty())
			return;

		if(transform_stack_.back().field_mode == core::field_mode::empty)
			return;

		item item;
		item.pix_desc	= frame.pixel_format_desc();
		item.transform	= transform_stack_.back();
		
		// NOTE: Once we have copied the arrays they are no longer valid for reading!!! Check for alternative solution e.g. transfer with AMD_pinned_memory.
		for(int n = 0; n < static_cast<int>(item.pix_desc.planes.size()); ++n)
			item.textures.push_back(ogl_->copy_async(frame.image_data(n), item.pix_desc.planes[n].width, item.pix_desc.planes[n].height, item.pix_desc.planes[n].stride));
		
		layers_.back().items.push_back(item);
	}

	void pop()
	{
		transform_stack_.pop_back();
	}

	void end_layer()
	{		
	}
	
	boost::unique_future<array<const std::uint8_t>> render(const core::video_format_desc& format_desc)
	{
		return renderer_(std::move(layers_), format_desc);
	}
	
	core::mutable_frame create_frame(const void* tag, const core::pixel_format_desc& desc) override
	{
		std::vector<array<std::uint8_t>> buffers;
		BOOST_FOREACH(auto& plane, desc.planes)		
			buffers.push_back(ogl_->create_array(plane.size));		

		return core::mutable_frame(std::move(buffers), core::audio_buffer(), tag, desc);
	}
};

image_mixer::image_mixer(const spl::shared_ptr<device>& ogl) : impl_(new impl(ogl)){}
image_mixer::~image_mixer(){}
void image_mixer::push(const core::frame_transform& transform){impl_->push(transform);}
void image_mixer::visit(const core::const_frame& frame){impl_->visit(frame);}
void image_mixer::pop(){impl_->pop();}
boost::unique_future<array<const std::uint8_t>> image_mixer::operator()(const core::video_format_desc& format_desc){return impl_->render(format_desc);}
void image_mixer::begin_layer(core::blend_mode blend_mode){impl_->begin_layer(blend_mode);}
void image_mixer::end_layer(){impl_->end_layer();}
core::mutable_frame image_mixer::create_frame(const void* tag, const core::pixel_format_desc& desc) {return impl_->create_frame(tag, desc);}

}}}