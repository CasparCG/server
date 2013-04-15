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

#include "input.h"

#include "../util/util.h"
#include "../util/flv.h"
#include "../util/ffmpeg_producer_params.h"
#include "../../ffmpeg_error.h"

#include <core/video_format.h>

#include <common/diagnostics/graph.h>
#include <common/concurrency/executor.h>
#include <common/exception/exceptions.h>
#include <common/exception/win32_exception.h>
#include <common/utility/string.h>

#include <tbb/concurrent_queue.h>
#include <tbb/atomic.h>
#include <tbb/recursive_mutex.h>

#include <boost/range/algorithm.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4244)
#endif
extern "C" 
{
	#define __STDC_CONSTANT_MACROS
	#define __STDC_LIMIT_MACROS
	#include <libavformat/avformat.h>
}
#if defined(_MSC_VER)
#pragma warning (pop)
#endif

static const size_t MAX_BUFFER_COUNT = 100;
static const size_t MIN_BUFFER_COUNT = 50;
static const size_t MAX_BUFFER_SIZE  = 64 * 1000000;

namespace caspar { namespace ffmpeg {
		
struct input::implementation : boost::noncopyable
{		
	const safe_ptr<diagnostics::graph>							graph_;

	const safe_ptr<AVFormatContext>								format_context_; // Destroy this last
	const int													default_stream_index_;
	
	const std::shared_ptr<ffmpeg_producer_params>				params_;
	tbb::atomic<bool>											loop_;
	
	uint32_t													frame_number_;
	
	tbb::concurrent_bounded_queue<std::shared_ptr<AVPacket>>	buffer_;
	tbb::atomic<size_t>											buffer_size_;
		
	executor													executor_;
	
	explicit implementation(
		const safe_ptr<diagnostics::graph> graph, 
		const std::shared_ptr<ffmpeg_producer_params>& params
	) : graph_(graph),
		params_(params),
		format_context_(open_input(params)),
		default_stream_index_(av_find_default_stream_index(format_context_.get())),
		frame_number_(0),
		executor_(print())
	{		
		loop_			= params->loop;
		buffer_size_	= 0;

		if(params_->start > 0)			
			queued_seek(params_->start);
								
		graph_->set_color("seek", diagnostics::color(1.0f, 0.5f, 0.0f));	
		graph_->set_color("buffer-count", diagnostics::color(0.7f, 0.4f, 0.4f));
		graph_->set_color("buffer-size", diagnostics::color(1.0f, 1.0f, 0.0f));	

		tick();
	}
	
	bool try_pop(std::shared_ptr<AVPacket>& packet)
	{
		auto result = buffer_.try_pop(packet);
		
		if(result)
		{
			if(packet)
				buffer_size_ -= packet->size;
			tick();
		}

		graph_->set_value("buffer-size", (static_cast<double>(buffer_size_)+0.001)/MAX_BUFFER_SIZE);
		graph_->set_value("buffer-count", (static_cast<double>(buffer_.size()+0.001)/MAX_BUFFER_COUNT));
		
		return result;
	}

	void seek(uint32_t target)
	{
		executor_.begin_invoke([=]
		{
			std::shared_ptr<AVPacket> packet;
			while(buffer_.try_pop(packet) && packet)
				buffer_size_ -= packet->size;

			queued_seek(target);

			tick();
		}, high_priority);
	}
	
	std::wstring print() const
	{
		return L"ffmpeg_input[" + params_->resource_name + L")]";
	}
	
	bool full() const
	{
		return (buffer_size_ > MAX_BUFFER_SIZE || buffer_.size() > MAX_BUFFER_COUNT) && buffer_.size() > MIN_BUFFER_COUNT;
	}

	void tick()
	{	
		if(!executor_.is_running())
			return;
		
		executor_.begin_invoke([this]
		{			
			if(full())
				return;

			try
			{
				auto packet = create_packet();
		
				auto ret = av_read_frame(format_context_.get(), packet.get()); // packet is only valid until next call of av_read_frame. Use av_dup_packet to extend its life.	
		
				if(is_eof(ret))														     
				{
					frame_number_	= 0;

					if(params_->loop)
					{
						queued_seek(params_->start);
						graph_->set_tag("seek");		
						CASPAR_LOG(trace) << print() << " Looping.";			
					}		
					else
						executor_.stop();
				}
				else
				{		
					THROW_ON_ERROR(ret, "av_read_frame", print());

					if(packet->stream_index == default_stream_index_)
						++frame_number_;

					THROW_ON_ERROR2(av_dup_packet(packet.get()), print());
				
					// Make sure that the packet is correctly deallocated even if size and data is modified during decoding.
					auto size = packet->size;
					auto data = packet->data;
			
					packet = safe_ptr<AVPacket>(packet.get(), [packet, size, data](AVPacket*)
					{
						packet->size = size;
						packet->data = data;				
					});

					buffer_.try_push(packet);
					buffer_size_ += packet->size;
				
					graph_->set_value("buffer-size", (static_cast<double>(buffer_size_)+0.001)/MAX_BUFFER_SIZE);
					graph_->set_value("buffer-count", (static_cast<double>(buffer_.size()+0.001)/MAX_BUFFER_COUNT));
				}	
		
				tick();		
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
				executor_.stop();
			}
		});
	}	
	
	safe_ptr<AVFormatContext> open_input(const std::shared_ptr<ffmpeg_producer_params> params)
	{
		AVFormatContext* weak_context = nullptr;

		switch (params->resource_type) {
		case FFMPEG_FILE:
			THROW_ON_ERROR2(avformat_open_input(&weak_context, narrow(params->resource_name).c_str(), nullptr, nullptr), params->resource_name);
			break;
		case FFMPEG_DEVICE: {
			AVDictionary* format_options = NULL;
			av_dict_set(&format_options, "video_size", narrow(params->size_str).c_str(), 0); // 640x360 for 16:9
			av_dict_set(&format_options, "pixel_format", narrow(params->pixel_format).c_str(), 0); // yuyv422 for sony
			av_dict_set(&format_options, "avioflags", "direct", 0);
			// Don't set framerate here, it is typically set to a fixed rate anyway. Better to insert a fps filter to adjust the frame rate.
			// av_dict_set(&format_options, "framerate", narrow(params->frame_rate).c_str(), 0);
			AVInputFormat* input_format = av_find_input_format("dshow");
			THROW_ON_ERROR2(avformat_open_input(&weak_context, narrow(params->resource_name).c_str(), input_format, &format_options), params->resource_name);
			av_dict_free(&format_options);
			} break;
		case FFMPEG_STREAM:
			THROW_ON_ERROR2(avformat_open_input(&weak_context, narrow(params->resource_name).c_str(), nullptr, nullptr), params->resource_name);
			break;
		};
		safe_ptr<AVFormatContext> context(weak_context, av_close_input_file);			
		THROW_ON_ERROR2(avformat_find_stream_info(weak_context, nullptr), params->resource_name);
		fix_meta_data(*context);
		return context;
	}

	void fix_meta_data(AVFormatContext& context)
	{
		auto video_index = av_find_best_stream(&context, AVMEDIA_TYPE_VIDEO, -1, -1, 0, 0);

		if(video_index > -1)
		{
			auto video_stream   = context.streams[video_index];
			auto video_context  = context.streams[video_index]->codec;
						
			if(boost::filesystem2::path(context.filename).extension() == ".flv")
			{
				try
				{
					auto meta = read_flv_meta_info(context.filename);
					double fps = boost::lexical_cast<double>(meta["framerate"]);
					video_stream->nb_frames = static_cast<int64_t>(boost::lexical_cast<double>(meta["duration"])*fps);
				}
				catch(...){}
			}
			else
			{
				auto stream_time = video_stream->time_base;
				auto duration	 = video_stream->duration;
				auto codec_time  = video_context->time_base;
				auto ticks		 = video_context->ticks_per_frame;

				if(video_stream->nb_frames == 0)
					video_stream->nb_frames = (duration*stream_time.num*codec_time.den)/(stream_time.den*codec_time.num*ticks);	
			}
		}
	}

	void queued_seek(const uint32_t target)
	{  	
		CASPAR_LOG(debug) << print() << " Seeking: " << target;

		int flags = AVSEEK_FLAG_FRAME;
		if(target == 0)
		{
			// Fix VP6 seeking
			int vid_stream_index = av_find_best_stream(format_context_.get(), AVMEDIA_TYPE_VIDEO, -1, -1, 0, 0);
			if(vid_stream_index >= 0)
			{
				auto codec_id = format_context_->streams[vid_stream_index]->codec->codec_id;
				if(codec_id == CODEC_ID_VP6A || codec_id == CODEC_ID_VP6F || codec_id == CODEC_ID_VP6)
					flags = AVSEEK_FLAG_BYTE;
			}
		}
		
		auto stream = format_context_->streams[default_stream_index_];
		auto codec  = stream->codec;
		auto fixed_target = (target*stream->time_base.den*codec->time_base.num)/(stream->time_base.num*codec->time_base.den)*codec->ticks_per_frame;
		
		THROW_ON_ERROR2(avformat_seek_file(format_context_.get(), default_stream_index_, std::numeric_limits<int64_t>::min(), fixed_target, std::numeric_limits<int64_t>::max(), 0), print());		
		
		auto flush_packet	= create_packet();
		flush_packet->data	= nullptr;
		flush_packet->size	= 0;
		flush_packet->pos	= target;

		buffer_.push(flush_packet);
	}	

	bool is_eof(int ret)
	{
		if(ret == AVERROR(EIO))
			CASPAR_LOG(trace) << print() << " Received EIO, assuming EOF. ";
		if(ret == AVERROR_EOF)
			CASPAR_LOG(trace) << print() << " Received EOF. ";

		return ret == AVERROR_EOF || ret == AVERROR(EIO) || frame_number_ >= params_->length; // av_read_frame doesn't always correctly return AVERROR_EOF;
	}
};

input::input(
	const safe_ptr<diagnostics::graph>& graph, 
	const std::shared_ptr<ffmpeg_producer_params>& params
) : impl_(new implementation(graph, params)){}
bool input::eof() const {return !impl_->executor_.is_running();}
bool input::try_pop(std::shared_ptr<AVPacket>& packet){return impl_->try_pop(packet);}
safe_ptr<AVFormatContext> input::context(){return impl_->format_context_;}
void input::loop(bool value){impl_->loop_ = value;}
bool input::loop() const{return impl_->loop_;}
void input::seek(uint32_t target){impl_->seek(target);}
}}
