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

#include "../util/write_frame.h"
#include "../util/blend.h"

#include <common/assert.h>
#include <common/gl/gl_check.h>
#include <common/concurrency/async.h>
#include <common/memory/memcpy.h>

#include <core/frame/write_frame.h>
#include <core/frame/frame_transform.h>
#include <core/frame/pixel_format.h>
#include <core/video_format.h>

#include <modules/ffmpeg/producer/util/util.h>

#include <asmlib.h>

#include <gl/glew.h>

#include <tbb/cache_aligned_allocator.h>
#include <tbb/parallel_for_each.h>

#include <boost/assign.hpp>
#include <boost/foreach.hpp>
#include <boost/range.hpp>
#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/thread/future.hpp>

#include <algorithm>
#include <vector>

#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4244)
#endif
extern "C" 
{
	#include <libswscale/swscale.h>
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
}
#if defined(_MSC_VER)
#pragma warning (pop)
#endif

namespace caspar { namespace accelerator { namespace cpu {
		
struct item
{
	core::pixel_format_desc						pix_desc;
	std::vector<spl::shared_ptr<host_buffer>>	buffers;
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

class image_renderer
{
	std::pair<std::vector<item>, boost::shared_future<boost::iterator_range<const uint8_t*>>>		last_image_;
	tbb::concurrent_unordered_map<int, tbb::concurrent_bounded_queue<std::shared_ptr<SwsContext>>>	sws_contexts_;
public:	
	boost::shared_future<boost::iterator_range<const uint8_t*>> operator()(std::vector<item> items, const core::video_format_desc& format_desc)
	{	
		if(last_image_.first == items && last_image_.second.has_value())
			return last_image_.second;

		auto image	= render(items, format_desc);
		last_image_ = std::make_pair(std::move(items), image);
		return image;
	}

private:
	boost::shared_future<boost::iterator_range<const uint8_t*>> render(std::vector<item> items, const core::video_format_desc& format_desc)
	{
		convert(items, format_desc.width, format_desc.height);		
		
		auto result = spl::make_shared<host_buffer>(format_desc.size, 0);
		if(format_desc.field_mode != core::field_mode::progressive)
		{
			auto upper = items;
			auto lower = items;

			BOOST_FOREACH(auto& item, upper)
				item.transform.field_mode &= core::field_mode::upper;
					
			BOOST_FOREACH(auto& item, lower)
				item.transform.field_mode &= core::field_mode::lower;
			
			draw(upper, result->data(), format_desc.width, format_desc.height);
			draw(lower, result->data(), format_desc.width, format_desc.height);
		}
		else
		{
			draw(items, result->data(), format_desc.width, format_desc.height);
		}
		
		return async(launch_policy::deferred, [=]
		{
			return boost::iterator_range<const uint8_t*>(result->data(), result->data() + format_desc.size);
		});		
	}

	void draw(std::vector<item>& items, uint8_t* dest, int width, int height)
	{
		BOOST_FOREACH(auto& item, items)
		{
			auto field_mode = item.transform.field_mode;		

			if(field_mode == core::field_mode::empty)
				continue;

			auto start = field_mode == core::field_mode::lower ? 1 : 0;
			auto step  = field_mode == core::field_mode::progressive ? 1 : 2;

			auto source2 = item.buffers.at(0)->data();

			tbb::parallel_for(start, height, step, [&](int y)
			{
				cpu::blend(dest + y*width*4, dest + y*width*4, source2 + y*width*4, width*4);
			});
		}
	}
	
	void convert(std::vector<item>& items, int width, int height)
	{
		tbb::parallel_for_each(items.begin(), items.end(), [&](item& item)
		{
			if(item.pix_desc.format == core::pixel_format::bgra && 
			   item.pix_desc.planes.at(0).width == width &&
			   item.pix_desc.planes.at(0).height == height)
				return;

			auto input_av_frame = ffmpeg::make_av_frame(item.buffers, item.pix_desc);
								
			int key = ((input_av_frame->width << 22) & 0xFFC00000) | ((input_av_frame->height << 6) & 0x003FC000) | ((input_av_frame->format << 7) & 0x00007F00);
						
			auto& pool = sws_contexts_[key];

			std::shared_ptr<SwsContext> sws_context;
			if(!pool.try_pop(sws_context))
			{
				double param;
				sws_context.reset(sws_getContext(input_av_frame->width, input_av_frame->height, static_cast<PixelFormat>(input_av_frame->format), width, height, PIX_FMT_BGRA, SWS_BILINEAR, nullptr, nullptr, &param), sws_freeContext);
			}
			
			if(!sws_context)				
				BOOST_THROW_EXCEPTION(operation_failed() << msg_info("Could not create software scaling context.") << boost::errinfo_api_function("sws_getContext"));				
		
			auto dest = spl::make_shared<host_buffer>(width*height*4);

			spl::shared_ptr<AVFrame> av_frame(avcodec_alloc_frame(), av_free);	
			avcodec_get_frame_defaults(av_frame.get());			
			avpicture_fill(reinterpret_cast<AVPicture*>(av_frame.get()), dest->data(), PIX_FMT_BGRA, width, height);
				
			sws_scale(sws_context.get(), input_av_frame->data, input_av_frame->linesize, 0, input_av_frame->height, av_frame->data, av_frame->linesize);	

			item.buffers.clear();
			item.buffers.push_back(dest);
			item.pix_desc = core::pixel_format_desc(core::pixel_format::bgra);
			item.pix_desc.planes.clear();
			item.pix_desc.planes.push_back(core::pixel_format_desc::plane(width, height, 4));

			pool.push(sws_context);
		});
	}
};
		
struct image_mixer::impl : boost::noncopyable
{	
	image_renderer						renderer_;
	std::vector<core::frame_transform>	transform_stack_;
	std::vector<item>					items_; // layer/stream/items
public:
	impl() 
		: transform_stack_(1)	
	{
		CASPAR_LOG(info) << L"Initialized CPU Accelerated Image Mixer";
	}

	void begin_layer(core::blend_mode blend_mode)
	{
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

		items_.push_back(item);
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
		return renderer_(std::move(items_), format_desc);
	}
	
	virtual spl::shared_ptr<cpu::write_frame> create_frame(const void* tag, const core::pixel_format_desc& desc)
	{
		return spl::make_shared<cpu::write_frame>(tag, desc);
	}
};

image_mixer::image_mixer() : impl_(new impl()){}
void image_mixer::push(core::frame_transform& transform){impl_->push(transform);}
void image_mixer::visit(core::data_frame& frame){impl_->visit(frame);}
void image_mixer::pop(){impl_->pop();}
boost::shared_future<boost::iterator_range<const uint8_t*>> image_mixer::operator()(const core::video_format_desc& format_desc){return impl_->render(format_desc);}
void image_mixer::begin_layer(core::blend_mode blend_mode){impl_->begin_layer(blend_mode);}
void image_mixer::end_layer(){impl_->end_layer();}
spl::shared_ptr<core::write_frame> image_mixer::create_frame(const void* tag, const core::pixel_format_desc& desc) {return impl_->create_frame(tag, desc);}

}}}