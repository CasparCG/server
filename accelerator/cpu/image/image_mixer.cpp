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
#include "image_mixer.h"

#include "../util/xmm.h"

#include <common/assert.h>
#include <common/gl/gl_check.h>
#include <common/future.h>
#include <common/array.h>

#include <core/frame/frame.h>
#include <core/frame/frame_transform.h>
#include <core/frame/pixel_format.h>
#include <core/video_format.h>

#include <GL/glew.h>

#include <tbb/parallel_for.h>
#include <tbb/parallel_for_each.h>
#include <tbb/concurrent_queue.h>

#include <boost/range/algorithm_ext/erase.hpp>

#include <tbb/concurrent_unordered_map.h>

#include <algorithm>
#include <cstdint>
#include <vector>
#include <set>
#include <array>

#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4244)
#endif
extern "C"
{
	#include <libswscale/swscale.h>
	#include <libavcodec/avcodec.h>
	#include <libswscale/swscale.h>
}
#if defined(_MSC_VER)
#pragma warning (pop)
#endif

namespace caspar { namespace accelerator { namespace cpu {

typedef std::vector<uint8_t> buffer;

struct item
{
	std::shared_ptr<AVFrame>     	frame;
	core::image_transform			transform;
};

bool operator==(const item& lhs, const item& rhs)
{
	return lhs.frame == rhs.frame && lhs.transform == rhs.transform;
}

bool operator!=(const item& lhs, const item& rhs)
{
	return !(lhs == rhs);
}

// 100% accurate blending with correct rounding.
inline xmm::s8_x blend(xmm::s8_x d, xmm::s8_x s)
{
	using namespace xmm;

	// C(S, D) = S + D - (((T >> 8) + T) >> 8);
	// T(S, D) = S * D[A] + 0x80

	auto aaaa   = s8_x::shuffle(d, s8_x(15, 15, 15, 15, 11, 11, 11, 11, 7, 7, 7, 7, 3, 3, 3, 3));
	d			= s8_x(u8_x::min(u8_x(d), u8_x(aaaa))); // Overflow guard. Some source files have color values which incorrectly exceed pre-multiplied alpha values, e.g. red(255) > alpha(254).

	auto xaxa	= s16_x(aaaa) >> 8;

	auto t1		= s16_x::multiply_low(s16_x(s) & 0x00FF, xaxa) + 0x80;
	auto t2		= s16_x::multiply_low(s16_x(s) >> 8    , xaxa) + 0x80;

	auto xyxy	= s8_x(((t1 >> 8) + t1) >> 8);
	auto yxyx	= s8_x((t2 >> 8) + t2);
	auto argb   = s8_x::blend(xyxy, yxyx, s8_x(-1, 0, -1, 0));

	return s8_x(s) + (d - argb);
}

template<typename temporal, typename alignment>
static void kernel(uint8_t* dest, const uint8_t* source, size_t count)
{
	using namespace xmm;

	for(auto n = 0; n < count; n += 32)
	{
		auto s0 = s8_x::load<temporal_tag, alignment>(dest+n+0);
		auto s1 = s8_x::load<temporal_tag, alignment>(dest+n+16);

		auto d0 = s8_x::load<temporal_tag, alignment>(source+n+0);
		auto d1 = s8_x::load<temporal_tag, alignment>(source+n+16);

		auto argb0 = blend(d0, s0);
		auto argb1 = blend(d1, s1);

		s8_x::store<temporal, alignment>(argb0, dest+n+0 );
		s8_x::store<temporal, alignment>(argb1, dest+n+16);
	}
}

template<typename temporal>
static void kernel(uint8_t* dest, const uint8_t* source, size_t count)
{
	using namespace xmm;

	if(reinterpret_cast<std::uint64_t>(dest) % 16 != 0 || reinterpret_cast<std::uint64_t>(source) % 16 != 0)
		kernel<temporal_tag, unaligned_tag>(dest, source, count);
	else
		kernel<temporal_tag, aligned_tag>(dest, source, count);
}

class image_renderer
{
	tbb::concurrent_unordered_map<int64_t, tbb::concurrent_bounded_queue<std::shared_ptr<SwsContext>>>	sws_devices_;
	tbb::concurrent_bounded_queue<spl::shared_ptr<buffer>>												temp_buffers_;
	core::video_format_desc																				format_desc_;
public:
	std::future<array<const std::uint8_t>> operator()(std::vector<item> items, const core::video_format_desc& format_desc)
	{
		if (format_desc != format_desc_)
		{
			format_desc_ = format_desc;
			sws_devices_.clear();
		}

		convert(items, format_desc.width, format_desc.height);

		auto result = std::shared_ptr<uint8_t>(new uint8_t[format_desc.size]);
		memset(result.get(), 0, format_desc.size);
		if(format_desc.field_mode != core::field_mode::progressive)
		{
			draw(items, result.get(), format_desc.width, format_desc.height, core::field_mode::upper);
			draw(items, result.get(), format_desc.width, format_desc.height, core::field_mode::lower);
		}
		else
		{
			draw(items, result.get(), format_desc.width, format_desc.height,  core::field_mode::progressive);
		}

		temp_buffers_.clear();

		return make_ready_future(array<const std::uint8_t>(result.get(), format_desc.size, true, result));
	}

private:

	void draw(std::vector<item> items, uint8_t* dest, std::size_t width, std::size_t height, core::field_mode field_mode)
	{
		for (auto& item : items)
			item.transform.field_mode &= field_mode;

		// Remove empty items.
		boost::range::remove_erase_if(items, [&](const item& item)
		{
			return item.transform.field_mode == core::field_mode::empty;
		});

		if(items.empty())
			return;

		auto start = field_mode == core::field_mode::lower ? 1 : 0;
		auto step  = field_mode == core::field_mode::progressive ? 1 : 2;

		// TODO: Add support for fill translations.
		// TODO: Add support for mask rect.
		// TODO: Add support for opacity.
		// TODO: Add support for mix transition.
		// TODO: Add support for push transition.
		// TODO: Add support for wipe transition.
		// TODO: Add support for slide transition.
		tbb::parallel_for(tbb::blocked_range<std::size_t>(0, height/step), [&](const tbb::blocked_range<std::size_t>& r)
		{
			for(auto i = r.begin(); i != r.end(); ++i)
			{
				auto y = i*step+start;

				for(std::size_t n = 0; n < items.size()-1; ++n)
					kernel<xmm::temporal_tag>(dest + y*width*4, items[n].frame->data[0] + y*width*4, width*4);

				std::size_t n = items.size()-1;
				kernel<xmm::nontemporal_tag>(dest + y*width*4, items[n].frame->data[0] + y*width*4, width*4);
			}

			_mm_mfence();
		});
	}

	void convert(std::vector<item>& source_items, int width, int height)
	{
		std::set<std::shared_ptr<AVFrame>> buffers;

		for (auto& item : source_items)
			buffers.insert(item.frame);

		auto dest_items = source_items;

		tbb::parallel_for_each(buffers.begin(), buffers.end(), [&](const std::shared_ptr<AVFrame>& in_frame)
		{
            auto out_frame = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame* ptr)
            {
                av_frame_free(&ptr);
            });
            out_frame->width = width;
            out_frame->height = height;
            out_frame->format = AV_PIX_FMT_BGRA;
            if (av_frame_get_buffer(out_frame.get(), 32) < 0) {
                CASPAR_THROW_EXCEPTION(bad_alloc());
            }

            int64_t key = ((static_cast<int64_t>(in_frame->width) << 32) & 0xFFFF00000000) |
                ((static_cast<int64_t>(in_frame->height) << 16) & 0xFFFF0000) |
                ((static_cast<int64_t>(in_frame->format) << 8) & 0xFF00);

            auto& pool = sws_devices_[key];

            std::shared_ptr<SwsContext> sws_device;
            if (!pool.try_pop(sws_device)) {
                double param;
                sws_device.reset(sws_getContext(in_frame->width, in_frame->height, static_cast<AVPixelFormat>(in_frame->format), width, height, AV_PIX_FMT_BGRA, SWS_BILINEAR, nullptr, nullptr, &param), sws_freeContext);
            }

            if (!sws_device) {
                CASPAR_THROW_EXCEPTION(operation_failed() << msg_info("Could not create software scaling device.") << boost::errinfo_api_function("sws_getContext"));
            }

            sws_scale(sws_device.get(), in_frame->data, in_frame->linesize, 0, in_frame->height, out_frame->data, out_frame->linesize);
            pool.push(sws_device);

			for(std::size_t n = 0; n < source_items.size(); ++n)
			{
				if(source_items[n].frame == in_frame)
				{
					dest_items[n].frame     = out_frame;
					dest_items[n].transform = source_items[n].transform;
				}
			}
		});

		source_items = std::move(dest_items);
	}
};

struct image_mixer::impl : boost::noncopyable
{
	image_renderer						renderer_;
	std::vector<core::image_transform>	transform_stack_;
	std::vector<item>					items_; // layer/stream/items
public:
	impl(int channel_id)
		: transform_stack_(1)
	{
		CASPAR_LOG(info) << L"Initialized Streaming SIMD Extensions Accelerated CPU Image Mixer for channel " << channel_id;
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

		if(frame.pixel_format_desc().planes.at(0).size < 16)
			return;

		if(transform_stack_.back().field_mode == core::field_mode::empty)
			return;

        auto in_frame = std::shared_ptr<AVFrame>(av_frame_alloc(), [frame](AVFrame* ptr)
        {
            av_frame_free(&ptr);
        });

        auto pix_desc = frame.pixel_format_desc();
        for (auto n = 0UL; n < pix_desc.planes.size(); ++n) {
            in_frame->data[n] = const_cast<uint8_t*>(frame.image_data(n).begin());
            in_frame->linesize[n] = pix_desc.planes[n].linesize;
        }

        switch (pix_desc.format) {
        case core::pixel_format::abgr:
            in_frame->format = AV_PIX_FMT_ABGR;
            break;
        case core::pixel_format::bgra:
            in_frame->format = AV_PIX_FMT_BGRA;
            break;
        case core::pixel_format::rgba:
            in_frame->format = AV_PIX_FMT_RGBA;
            break;
        case core::pixel_format::argb:
            in_frame->format = AV_PIX_FMT_ARGB;
            break;
        case core::pixel_format::ycbcr: {
            int y_w = pix_desc.planes[0].width;
            int y_h = pix_desc.planes[0].height;
            int c_w = pix_desc.planes[1].width;
            int c_h = pix_desc.planes[1].height;

            if (c_h == y_h && c_w == y_w)
                in_frame->format = AV_PIX_FMT_YUV444P;
            else if (c_h == y_h && c_w * 2 == y_w)
                in_frame->format = AV_PIX_FMT_YUV422P;
            else if (c_h == y_h && c_w * 4 == y_w)
                in_frame->format = AV_PIX_FMT_YUV411P;
            else if (c_h * 2 == y_h && c_w * 2 == y_w)
                in_frame->format = AV_PIX_FMT_YUV420P;
            else if (c_h * 2 == y_h && c_w * 4 == y_w)
                in_frame->format = AV_PIX_FMT_YUV410P;
            else
                return;

            break;
        }
        case core::pixel_format::ycbcra: {
            int y_w = pix_desc.planes[0].width;
            int y_h = pix_desc.planes[0].height;
            int c_w = pix_desc.planes[1].width;
            int c_h = pix_desc.planes[1].height;

            if (c_h == y_h && c_w == y_w)
                in_frame->format = AV_PIX_FMT_YUVA444P;
            else if (c_h == y_h && c_w * 2 == y_w)
                in_frame->format = AV_PIX_FMT_YUVA422P;
            else if (c_h == y_h && c_w * 4 == y_w)
                return;
            else if (c_h * 2 == y_h && c_w * 2 == y_w)
                in_frame->format = AV_PIX_FMT_YUVA420P;
            else if (c_h * 2 == y_h && c_w * 4 == y_w)
                return;
            else
                return;

            break;
        }
        default:
            return;
        }

        in_frame->width = pix_desc.planes[0].width;
        in_frame->height = pix_desc.planes[0].height;

		item item;
		item.frame    	= in_frame;
		item.transform	= transform_stack_.back();;

		items_.push_back(item);
	}

	void pop()
	{
		transform_stack_.pop_back();
	}

	std::future<array<const std::uint8_t>> render(const core::video_format_desc& format_desc)
	{
		return renderer_(std::move(items_), format_desc);
	}

	core::mutable_frame create_frame(const void* tag, const core::pixel_format_desc& desc)
	{
		std::vector<array<std::uint8_t>> buffers;
		for (auto& plane : desc.planes)
		{
			auto buf = spl::make_shared<buffer>(plane.size);
			buffers.push_back(array<std::uint8_t>(buf->data(), plane.size, true, buf));
		}
		return core::mutable_frame(std::move(buffers), core::mutable_audio_buffer(), tag, desc);
	}
};

image_mixer::image_mixer(int channel_id) : impl_(new impl(channel_id)){}
image_mixer::~image_mixer(){}
void image_mixer::push(const core::frame_transform& transform){impl_->push(transform);}
void image_mixer::visit(const core::const_frame& frame){impl_->visit(frame);}
void image_mixer::pop(){impl_->pop();}
std::future<array<const std::uint8_t>> image_mixer::operator()(const core::video_format_desc& format_desc, bool /* straighten_alpha */){return impl_->render(format_desc);}
core::mutable_frame image_mixer::create_frame(const void* tag, const core::pixel_format_desc& desc) {return impl_->create_frame(tag, desc);}

}}}
