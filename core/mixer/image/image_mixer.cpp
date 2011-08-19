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

#include <gl/glew.h>

#include <boost/foreach.hpp>
#include <boost/range.hpp>
#include <boost/range/algorithm/find.hpp>

#include <algorithm>
#include <array>
#include <deque>
#include <unordered_map>

namespace caspar { namespace core {
		
struct image_mixer::implementation : boost::noncopyable
{		
	typedef std::deque<render_item>			layer;

	video_channel_context&					channel_;

	std::vector<image_transform>			transform_stack_;
	std::vector<video_mode::type>			mode_stack_;

	std::deque<std::deque<render_item>>		layers_; // layer/stream/items
	
	image_kernel							kernel_;		

	std::shared_ptr<device_buffer>			active_buffer_;
public:
	implementation(video_channel_context& video_channel) 
		: channel_(video_channel)
		, transform_stack_(1)
		, mode_stack_(1, video_mode::progressive)
	{
	}

	~implementation()
	{
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
		
		core::render_item item(frame.get_pixel_format_desc(), frame.get_textures(), transform_stack_.back(), mode_stack_.back(), frame.tag());	

		auto& layer = layers_.back();

		if(boost::range::find(layer, item) == layer.end())
			layer.push_back(item);
	}

	void end()
	{
		transform_stack_.pop_back();
		mode_stack_.pop_back();
	}

	void begin_layer()
	{
		layers_.push_back(layer());
	}

	void end_layer()
	{
	}
	
	boost::unique_future<safe_ptr<host_buffer>> render()
	{		
		auto layers = std::move(layers_);
		return channel_.ogl().begin_invoke([=]() mutable
		{
			return render(std::move(layers));
		});
	}
	
	safe_ptr<host_buffer> render(std::deque<layer>&& layers)
	{
		std::shared_ptr<device_buffer> layer_key_buffer;

		auto draw_buffer = create_device_buffer(4);
				
		BOOST_FOREACH(auto& layer, layers)
			draw(std::move(layer), draw_buffer, layer_key_buffer);
		
		auto host_buffer = channel_.ogl().create_host_buffer(channel_.get_format_desc().size, host_buffer::read_only);
		channel_.ogl().attach(*draw_buffer);
		host_buffer->begin_read(draw_buffer->width(), draw_buffer->height(), format(draw_buffer->stride()));
		
		active_buffer_ = draw_buffer;

		channel_.ogl().flush(); // NOTE: This is important, otherwise fences will deadlock.
			
		return host_buffer;
	}

	// TODO: We might have more overlaps for opacity transitions
	// TODO: What about blending modes, are they ok? Maybe only overlap detection is required for opacity?
	void draw(layer&& layer, const safe_ptr<device_buffer>& draw_buffer, std::shared_ptr<device_buffer>& layer_key_buffer)
	{				
		if(layer.empty())
			return;

		std::pair<int, std::shared_ptr<device_buffer>> local_key_buffer;
					
		//if(has_overlapping_items(layer, layer.front().transform.get_blend_mode()))
		//{
		//	auto local_draw_buffer = create_device_buffer(4);	

		//	auto local_blend_mode = layer.front().transform.get_blend_mode();

		//	int fields = 0;
		//	BOOST_FOREACH(auto& item, layer)
		//	{
		//		if(fields & item.mode)
		//			item.transform.set_blend_mode(image_transform::blend_mode::normal); // Disable blending, it will be used when merging back into render stack.
		//		else
		//		{
		//			item.transform.set_blend_mode(image_transform::blend_mode::replace); // Target field is empty, no blending, just copy
		//			fields |= item.mode;
		//		}

		//		draw_item(std::move(item), local_draw_buffer, local_key_buffer, layer_key_buffer);		
		//	}

		//	kernel_.draw(channel_.ogl(), create_render_item(local_draw_buffer, local_blend_mode), draw_buffer, nullptr, nullptr);
		//}
		//else // fast path
		//{
			BOOST_FOREACH(auto& item, layer)		
				draw_item(std::move(item), draw_buffer, local_key_buffer, layer_key_buffer);		
		//}					

		CASPAR_ASSERT(local_key_buffer.first == 0 || local_key_buffer.first == core::video_mode::progressive);

		std::swap(local_key_buffer.second, layer_key_buffer);
	}

	void draw_item(render_item&&									item, 
				   const safe_ptr<device_buffer>&					draw_buffer, 
				   std::pair<int, std::shared_ptr<device_buffer>>&	local_key_buffer, 
				   std::shared_ptr<device_buffer>&					layer_key_buffer)
	{											
		if(item.transform.get_is_key())
		{
			if(!local_key_buffer.second)
			{
				local_key_buffer.first = 0;
				local_key_buffer.second = create_device_buffer(1);
			}

			// No transparency for key
			item.transform.set_opacity(1.0);
			item.transform.set_blend_mode(image_transform::blend_mode::normal);

			local_key_buffer.first |= item.mode;
			kernel_.draw(channel_.ogl(), std::move(item), make_safe(local_key_buffer.second), nullptr, nullptr);
		}
		else
		{
			kernel_.draw(channel_.ogl(), std::move(item), draw_buffer, local_key_buffer.second, layer_key_buffer);
			local_key_buffer.first ^= item.mode;
			
			if(local_key_buffer.first == 0)
			{
				local_key_buffer.first = 0;
				local_key_buffer.second.reset();
			}
		}
	}

	//// TODO: Optimize
	//bool has_overlapping_items(const layer& layer, image_transform::blend_mode::type blend_mode)
	//{
	//	if(layer.size() < 2)
	//		return false;	
	//	
	//	implementation::layer fill;

	//	std::copy_if(layer.begin(), layer.end(), std::back_inserter(fill), [&](const render_item& item)
	//	{
	//		return !item.transform.get_is_key();
	//	});
	//		
	//	if(blend_mode == image_transform::blend_mode::normal) // Only overlap if opacity
	//	{
	//		return std::any_of(fill.begin(), fill.end(), [&](const render_item& item)
	//		{
	//			return item.transform.get_opacity() < 1.0 - 0.001;
	//		});
	//	}

	//	// Simple solution, just check if we have differnt video streams / tags.
	//	return std::any_of(fill.begin(), fill.end(), [&](const render_item& item)
	//	{
	//		return item.tag != fill.front().tag;
	//	});
	//}			
	//		
	//render_item create_render_item(const safe_ptr<device_buffer>& buffer, image_transform::blend_mode::type blend_mode)
	//{
	//	CASPAR_ASSERT(buffer->stride() == 4 && "Only used for bgra textures");

	//	pixel_format_desc desc;
	//	desc.pix_fmt = pixel_format::bgra;
	//	desc.planes.push_back(pixel_format_desc::plane(channel_.get_format_desc().width, channel_.get_format_desc().height, 4));

	//	std::vector<safe_ptr<device_buffer>> textures;
	//	textures.push_back(buffer);
	//			
	//	image_transform transform;
	//	transform.set_blend_mode(blend_mode);

	//	return render_item(desc, std::move(textures), transform, video_mode::progressive, nullptr);		 
	//}

	safe_ptr<device_buffer> create_device_buffer(size_t stride)
	{
		auto buffer = channel_.ogl().create_device_buffer(channel_.get_format_desc().width, channel_.get_format_desc().height, stride);
		channel_.ogl().clear(*buffer);
		return buffer;
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