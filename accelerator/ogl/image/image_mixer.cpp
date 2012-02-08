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

#include "../util/write_frame.h"
#include "../util/context.h"
#include "../util/host_buffer.h"
#include "../util/device_buffer.h"

#include <common/gl/gl_check.h>
#include <common/concurrency/async.h>
#include <common/memory/memcpy.h>

#include <core/frame/write_frame.h>
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
		
typedef boost::shared_future<spl::shared_ptr<device_buffer>> future_texture;

struct item
{
	core::pixel_format_desc						pix_desc;
	std::vector<spl::shared_ptr<host_buffer>>	buffers;
	std::vector<future_texture>					textures;
	core::frame_transform						transform;


	item()
		: pix_desc(core::pixel_format::invalid)
	{
	}
};

bool operator==(const item& lhs, const item& rhs)
{
	return lhs.buffers == rhs.buffers && lhs.transform == rhs.transform;
}

bool operator!=(const item& lhs, const item& rhs)
{
	return !(lhs == rhs);
}

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

bool operator==(const layer& lhs, const layer& rhs)
{
	return lhs.items == rhs.items && lhs.blend_mode == rhs.blend_mode;
}

bool operator!=(const layer& lhs, const layer& rhs)
{
	return !(lhs == rhs);
}

class image_renderer
{
	spl::shared_ptr<context>																		ogl_;
	image_kernel																					kernel_;
	std::pair<std::vector<layer>, boost::shared_future<boost::iterator_range<const uint8_t*>>>		last_image_;
public:
	image_renderer(const spl::shared_ptr<context>& ogl)
		: ogl_(ogl)
		, kernel_(ogl_)
	{
	}
	
	boost::shared_future<boost::iterator_range<const uint8_t*>> operator()(std::vector<layer> layers, const core::video_format_desc& format_desc)
	{	
		if(last_image_.first == layers && last_image_.second.has_value())
			return last_image_.second;

		auto image	= render(layers, format_desc);
		last_image_ = std::make_pair(std::move(layers), image);
		return image;
	}

private:	
	boost::shared_future<boost::iterator_range<const uint8_t*>> render(std::vector<layer> layers, const core::video_format_desc& format_desc)
	{	
		static const auto empty = spl::make_shared<const std::vector<uint8_t, tbb::cache_aligned_allocator<uint8_t>>>(2048*2048*4, 0);
		CASPAR_VERIFY(empty->size() >= format_desc.size);
		
		if(layers.empty())
		{ // Bypass GPU with empty frame.
			return async(launch_policy::deferred, [=]
			{
				return boost::iterator_range<const uint8_t*>(empty->data(), empty->data() + format_desc.size);
			});
		}
		else if(has_uswc_memcpy() &&				
				layers.size()				== 1 &&
			    layers.at(0).items.size()	== 1 &&
			   (kernel_.has_blend_modes() && layers.at(0).blend_mode != core::blend_mode::normal) == false &&
			    layers.at(0).items.at(0).pix_desc.format		== core::pixel_format::bgra &&
			    layers.at(0).items.at(0).buffers.at(0)->size() == format_desc.size &&
			    layers.at(0).items.at(0).transform				== core::frame_transform())
		{ // Bypass GPU using streaming loads to cachable memory.
			auto uswc_buffer = layers.at(0).items.at(0).buffers.at(0);
			auto buffer		 = std::make_shared<std::vector<uint8_t, tbb::cache_aligned_allocator<uint8_t>>>(uswc_buffer->size());

			uswc_memcpy(buffer->data(), uswc_buffer->data(), uswc_buffer->size());

			return async(launch_policy::deferred, [=]
			{
				return boost::iterator_range<const uint8_t*>(buffer->data(), buffer->data() + buffer->size());
			});
		}
		else
		{	
			// Start host->device transfers.

			std::map<const host_buffer*, future_texture> buffer_map;

			BOOST_FOREACH(auto& layer, layers)
			{
				BOOST_FOREACH(auto& item, layer.items)
				{
					auto host_buffers = boost::get<std::vector<spl::shared_ptr<host_buffer>>>(item.buffers);
					auto textures	  = std::vector<future_texture>();

					for(size_t n = 0; n < host_buffers.size(); ++n)	
					{
						auto buffer	= host_buffers[n];
						auto it		= buffer_map.find(buffer.get());
						if(it == buffer_map.end())
						{
							auto plane			= item.pix_desc.planes[n];
							auto future_texture	= ogl_->copy_async(buffer, plane.width, plane.height, plane.channels);
							it = buffer_map.insert(std::make_pair(buffer.get(), std::move(future_texture))).first;
						}
						item.textures.push_back(it->second);
					}	
					item.buffers.clear();
				}
			}	
			
			// Draw
			boost::shared_future<spl::shared_ptr<host_buffer>> buffer = ogl_->begin_invoke([=]() mutable -> spl::shared_ptr<host_buffer>
			{
				auto draw_buffer = create_mixer_buffer(4, format_desc);

				if(format_desc.field_mode != core::field_mode::progressive)
				{
					auto upper = layers;
					auto lower = std::move(layers);

					BOOST_FOREACH(auto& layer, upper)
					{
						BOOST_FOREACH(auto& item, layer.items)
							item.transform.field_mode &= core::field_mode::upper;
					}

					BOOST_FOREACH(auto& layer, lower)
					{
						BOOST_FOREACH(auto& item, layer.items)
							item.transform.field_mode &= core::field_mode::lower;
					}

					draw(std::move(upper), draw_buffer, format_desc);
					draw(std::move(lower), draw_buffer, format_desc);
				}
				else
				{
					draw(std::move(layers), draw_buffer, format_desc);
				}
			
				auto result = ogl_->create_host_buffer(static_cast<int>(format_desc.size), host_buffer::usage::read_only); 
				draw_buffer->copy_to(*result);							
				return result;
			});
		
			// Defer memory mapping.
			return async(launch_policy::deferred, [=]() mutable -> boost::iterator_range<const uint8_t*>
			{
				const auto& buf = buffer.get();
				if(!buf->data())
					ogl_->invoke(std::bind(&host_buffer::map, std::ref(buf)), task_priority::high_priority);

				auto ptr = reinterpret_cast<const uint8_t*>(buf->data()); // .get() and ->data() can block calling thread, ->data() can also block OpenGL thread, defer it as long as possible.
				return boost::iterator_range<const uint8_t*>(ptr, ptr + buffer.get()->size());
			});
		}
	}
	
	void draw(std::vector<layer>&&				layers, 
			  spl::shared_ptr<device_buffer>&	draw_buffer, 
			  const core::video_format_desc&	format_desc)
	{
		std::shared_ptr<device_buffer> layer_key_buffer;

		BOOST_FOREACH(auto& layer, layers)
			draw_layer(std::move(layer), draw_buffer, layer_key_buffer, format_desc);
	}

	void draw_layer(layer&&								layer, 
					spl::shared_ptr<device_buffer>&		draw_buffer,
					std::shared_ptr<device_buffer>&		layer_key_buffer,
					const core::video_format_desc&		format_desc)
	{		
		// Remove empty items.
		boost::range::remove_erase_if(layer.items, [](const item& item)
		{
			return item.transform.field_mode == core::field_mode::empty;
		});

		if(layer.items.empty())
			return;

		std::shared_ptr<device_buffer> local_key_buffer;
		std::shared_ptr<device_buffer> local_mix_buffer;
				
		if(layer.blend_mode != core::blend_mode::normal)
		{
			auto layer_draw_buffer = create_mixer_buffer(4, format_desc);

			BOOST_FOREACH(auto& item, layer.items)
				draw_item(std::move(item), layer_draw_buffer, layer_key_buffer, local_key_buffer, local_mix_buffer, format_desc);	
		
			draw_mixer_buffer(layer_draw_buffer, std::move(local_mix_buffer), core::blend_mode::normal);							
			draw_mixer_buffer(draw_buffer, std::move(layer_draw_buffer), layer.blend_mode);
		}
		else // fast path
		{
			BOOST_FOREACH(auto& item, layer.items)		
				draw_item(std::move(item), draw_buffer, layer_key_buffer, local_key_buffer, local_mix_buffer, format_desc);		
					
			draw_mixer_buffer(draw_buffer, std::move(local_mix_buffer), core::blend_mode::normal);
		}					

		layer_key_buffer = std::move(local_key_buffer);
	}

	void draw_item(item&&							item, 
				   spl::shared_ptr<device_buffer>&	draw_buffer, 
				   std::shared_ptr<device_buffer>&	layer_key_buffer, 
				   std::shared_ptr<device_buffer>&	local_key_buffer, 
				   std::shared_ptr<device_buffer>&	local_mix_buffer,
				   const core::video_format_desc&	format_desc)
	{			
		draw_params draw_params;
		draw_params.pix_desc	= std::move(item.pix_desc);
		draw_params.transform	= std::move(item.transform);
		BOOST_FOREACH(auto& future_texture, item.textures)
			draw_params.textures.push_back(future_texture.get());

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
			draw_mixer_buffer(draw_buffer, std::move(local_mix_buffer), core::blend_mode::normal);
			
			draw_params.background			= draw_buffer;
			draw_params.local_key			= std::move(local_key_buffer);
			draw_params.layer_key			= layer_key_buffer;

			kernel_.draw(std::move(draw_params));
		}	
	}

	void draw_mixer_buffer(spl::shared_ptr<device_buffer>&	draw_buffer, 
						   std::shared_ptr<device_buffer>&& source_buffer, 
						   core::blend_mode					blend_mode = core::blend_mode::normal)
	{
		if(!source_buffer)
			return;

		draw_params draw_params;
		draw_params.pix_desc.format		= core::pixel_format::bgra;
		draw_params.pix_desc.planes		= list_of(core::pixel_format_desc::plane(source_buffer->width(), source_buffer->height(), 4));
		draw_params.textures			= list_of(source_buffer);
		draw_params.transform			= core::frame_transform();
		draw_params.blend_mode			= blend_mode;
		draw_params.background			= draw_buffer;

		kernel_.draw(std::move(draw_params));
	}
			
	spl::shared_ptr<device_buffer> create_mixer_buffer(int stride, const core::video_format_desc& format_desc)
	{
		auto buffer = ogl_->create_device_buffer(format_desc.width, format_desc.height, stride);
		buffer->clear();
		return buffer;
	}
};
		
struct image_mixer::impl : boost::noncopyable
{	
	spl::shared_ptr<context>			ogl_;
	image_renderer						renderer_;
	std::vector<core::frame_transform>	transform_stack_;
	std::vector<layer>					layers_; // layer/stream/items
public:
	impl(const spl::shared_ptr<context>& ogl) 
		: ogl_(ogl)
		, renderer_(ogl)
		, transform_stack_(1)	
	{
	}

	void begin_layer(core::blend_mode blend_mode)
	{
		layers_.push_back(layer(std::vector<item>(), blend_mode));
	}
		
	void push(core::frame_transform& transform)
	{
		transform_stack_.push_back(transform_stack_.back()*transform);
	}
		
	void visit(core::data_frame& frame2)
	{			
		write_frame* frame = dynamic_cast<write_frame*>(&frame2);
		if(frame == nullptr)
			return;

		if(frame->get_pixel_format_desc().format == core::pixel_format::invalid)
			return;

		if(frame->get_buffers().empty())
			return;

		if(transform_stack_.back().field_mode == core::field_mode::empty)
			return;

		item item;
		item.pix_desc			= frame->get_pixel_format_desc();
		item.buffers			= frame->get_buffers();				
		item.transform			= transform_stack_.back();
		item.transform.volume	= core::frame_transform().volume; // Set volume to default since we don't care about it here.

		layers_.back().items.push_back(item);
	}

	void pop()
	{
		transform_stack_.pop_back();
	}

	void end_layer()
	{		
	}
	
	boost::shared_future<boost::iterator_range<const uint8_t*>> render(const core::video_format_desc& format_desc)
	{
		// Remove empty layers.
		boost::range::remove_erase_if(layers_, [](const layer& layer)
		{
			return layer.items.empty();
		});

		return renderer_(std::move(layers_), format_desc);
	}
	
	virtual spl::shared_ptr<ogl::write_frame> create_frame(const void* tag, const core::pixel_format_desc& desc)
	{
		return spl::make_shared<ogl::write_frame>(ogl_, tag, desc);
	}
};

image_mixer::image_mixer(const spl::shared_ptr<context>& ogl) : impl_(new impl(ogl)){}
void image_mixer::push(core::frame_transform& transform){impl_->push(transform);}
void image_mixer::visit(core::data_frame& frame){impl_->visit(frame);}
void image_mixer::pop(){impl_->pop();}
boost::shared_future<boost::iterator_range<const uint8_t*>> image_mixer::operator()(const core::video_format_desc& format_desc){return impl_->render(format_desc);}
void image_mixer::begin_layer(core::blend_mode blend_mode){impl_->begin_layer(blend_mode);}
void image_mixer::end_layer(){impl_->end_layer();}
spl::shared_ptr<core::write_frame> image_mixer::create_frame(const void* tag, const core::pixel_format_desc& desc) {return impl_->create_frame(tag, desc);}

}}}