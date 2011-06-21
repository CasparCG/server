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

#include "video_decoder.h"
#include "../util.h"

#include "../../ffmpeg_error.h"
#include "../filter/filter.h"

#include <common/memory/memcpy.h>

#include <core/video_format.h>
#include <core/producer/frame/basic_frame.h>
#include <core/mixer/write_frame.h>
#include <core/producer/frame/image_transform.h>
#include <core/producer/frame/frame_factory.h>

#include <tbb/parallel_for.h>

#include <boost/range/algorithm_ext.hpp>

#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4244)
#endif
extern "C" 
{
	#define __STDC_CONSTANT_MACROS
	#define __STDC_LIMIT_MACROS
	#include <libavformat/avformat.h>
	#include <libavcodec/avcodec.h>
}
#if defined(_MSC_VER)
#pragma warning (pop)
#endif

namespace caspar {
		
struct video_decoder::implementation : boost::noncopyable
{
	input& input_;
	const safe_ptr<core::frame_factory>		frame_factory_;
	AVCodecContext&							codec_context_;
	size_t									frame_number_;

	std::shared_ptr<filter>					filter_;
	int										eof_count_;

	std::string								filter_str_;

public:
	explicit implementation(input& input, const safe_ptr<core::frame_factory>& frame_factory, const std::string& filter_str) 
		: input_(input)
		, frame_factory_(frame_factory)
		, codec_context_(*input_.get_video_codec_context())
		, frame_number_(0)
		, filter_(filter_str.empty() ? nullptr : new filter(filter_str))
		, filter_str_(filter_str)
		, eof_count_(std::numeric_limits<int>::max())
	{
	}

	std::deque<std::pair<int, safe_ptr<core::write_frame>>> receive()
	{
		std::deque<std::pair<int, safe_ptr<core::write_frame>>> result;
		
		std::shared_ptr<AVPacket> pkt;
		for(int n = 0; n < 32 && result.empty() && input_.try_pop_video_packet(pkt); ++n)	
			boost::range::push_back(result, decode(pkt));

		return result;
	}

	std::deque<std::pair<int, safe_ptr<core::write_frame>>> decode(const std::shared_ptr<AVPacket>& video_packet)
	{			
		std::deque<std::pair<int, safe_ptr<core::write_frame>>> result;

		if(!video_packet) // eof
		{
			eof_count_ = frame_number_ + (filter_ ? filter_->delay()+1 : 0);
			avcodec_flush_buffers(&codec_context_);
			return result;
		}		
		
		frame_number_ = frame_number_ % eof_count_;
		
		const void* tag = this;

		if(filter_)
		{							
			std::shared_ptr<AVFrame> frame;
		
			tbb::parallel_invoke(
			[&]
			{
				frame = decode_frame(video_packet);
			},
			[&]
			{		
				boost::range::transform(filter_->poll(), std::back_inserter(result), [&](const safe_ptr<AVFrame>& frame)
				{
					return std::make_pair(frame_number_, make_write_frame(tag, frame, frame_factory_));
				});
		
				if(!result.empty())
					++frame_number_;
			});		

			if(frame)
				filter_->push(make_safe(frame));
		}
		else
		{
			auto frame = decode_frame(video_packet);
			
			if(frame)
				result.push_back(std::make_pair(frame_number_++, make_write_frame(tag, make_safe(frame), frame_factory_)));
		}

		return result;
	}
			
	std::shared_ptr<AVFrame> decode_frame(const std::shared_ptr<AVPacket>& video_packet)
	{
		std::shared_ptr<AVFrame> decoded_frame(avcodec_alloc_frame(), av_free);

		int frame_finished = 0;
		const int errn = avcodec_decode_video2(&codec_context_, decoded_frame.get(), &frame_finished, video_packet.get());
		
		if(errn < 0)
		{
			BOOST_THROW_EXCEPTION(
				invalid_operation() <<
				msg_info(av_error_str(errn)) <<
				boost::errinfo_api_function("avcodec_decode_video") <<
				boost::errinfo_errno(AVUNERROR(errn)));
		}

		if(frame_finished == 0)		
			decoded_frame = nullptr;
		
		return decoded_frame;
	}
};

video_decoder::video_decoder(input& input, const safe_ptr<core::frame_factory>& frame_factory, const std::string& filter_str) : impl_(new implementation(input, frame_factory, filter_str)){}
std::deque<std::pair<int, safe_ptr<core::write_frame>>> video_decoder::receive(){return impl_->receive();}

}