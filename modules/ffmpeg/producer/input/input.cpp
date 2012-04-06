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
#include "../../ffmpeg_error.h"

#include <common/diagnostics/graph.h>
#include <common/executor.h>
#include <common/lock.h>
#include <common/except.h>
#include <common/log.h>

#include <core/video_format.h>

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

namespace caspar { namespace ffmpeg {

static const int MIN_FRAMES = 25;

class stream
{
	stream(const stream&);
	stream& operator=(const stream&);

	typedef tbb::concurrent_bounded_queue<std::shared_ptr<AVPacket>>::size_type size_type;

	int														 index_;
	tbb::concurrent_bounded_queue<std::shared_ptr<AVPacket>> packets_;
public:

	stream(int index) 
		: index_(index)
	{
	}
	
	void push(const std::shared_ptr<AVPacket>& packet)
	{
		if(packet && packet->data && packet->stream_index != index_)
			return;

		packets_.push(packet);
	}

	bool try_pop(std::shared_ptr<AVPacket>& packet)
	{
		return packets_.try_pop(packet);
	}

	void clear()
	{
		std::shared_ptr<AVPacket> packet;
		while(packets_.try_pop(packet));
	}
		
	size_type size() const
	{
		return index_ != -1 ? packets_.size() : std::numeric_limits<size_type>::max();
	}
};
		
struct input::impl : boost::noncopyable
{		
	const spl::shared_ptr<diagnostics::graph>					graph_;

	const spl::shared_ptr<AVFormatContext>						format_context_; // Destroy this last
	const int													default_stream_index_;
			
	const std::wstring											filename_;
	tbb::atomic<uint32_t>										start_;		
	tbb::atomic<uint32_t>										length_;
	tbb::atomic<bool>											loop_;
	double														fps_;
	uint32_t													frame_number_;
	
	stream														video_stream_;
	stream														audio_stream_;

	boost::optional<uint32_t>									seek_target_;

	tbb::atomic<bool>											is_running_;
	boost::mutex												mutex_;
	boost::condition_variable									cond_;
	boost::thread												thread_;
	
	impl(const spl::shared_ptr<diagnostics::graph> graph, const std::wstring& filename, const bool loop, const uint32_t start, const uint32_t length) 
		: graph_(graph)
		, format_context_(open_input(filename))		
		, default_stream_index_(av_find_default_stream_index(format_context_.get()))
		, filename_(filename)
		, frame_number_(0)
		, fps_(read_fps(*format_context_, 0.0))
		, video_stream_(av_find_best_stream(format_context_.get(), AVMEDIA_TYPE_VIDEO, -1, -1, 0, 0))
		, audio_stream_(av_find_best_stream(format_context_.get(), AVMEDIA_TYPE_AUDIO, -1, -1, 0, 0))
	{		
		start_			= start;
		length_			= length;
		loop_			= loop;
		is_running_		= true;

		if(start_ != 0)
			seek_target_ = start_;
														
		graph_->set_color("seek", diagnostics::color(1.0f, 0.5f, 0.0f));	
		graph_->set_color("audio-buffer", diagnostics::color(0.7f, 0.4f, 0.4f));
		graph_->set_color("video-buffer", diagnostics::color(1.0f, 1.0f, 0.0f));	
		
		for(int n = 0; n < 8; ++n)
			tick();

		thread_	= boost::thread([this]{run();});
	}

	~impl()
	{
		is_running_ = false;
		cond_.notify_one();
		thread_.join();
	}
	
	bool try_pop_video(std::shared_ptr<AVPacket>& packet)
	{				
		bool result = video_stream_.try_pop(packet);
		if(result)
			cond_.notify_one();
		
		graph_->set_value("video-buffer", std::min(1.0, static_cast<double>(video_stream_.size()/MIN_FRAMES)));
				
		return result;
	}
	
	bool try_pop_audio(std::shared_ptr<AVPacket>& packet)
	{				
		bool result = audio_stream_.try_pop(packet);
		if(result)
			cond_.notify_one();
				
		graph_->set_value("audio-buffer", std::min(1.0, static_cast<double>(audio_stream_.size()/MIN_FRAMES)));

		return result;
	}

	void seek(uint32_t target)
	{
		{
			boost::lock_guard<boost::mutex> lock(mutex_);

			seek_target_ = target;
			video_stream_.clear();
			audio_stream_.clear();
		}
		
		cond_.notify_one();
	}
		
	std::wstring print() const
	{
		return L"ffmpeg_input[" + filename_ + L")]";
	}

private:
	void internal_seek(uint32_t target)
	{
		graph_->set_tag("seek");	

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
		
		THROW_ON_ERROR2(avformat_seek_file(format_context_.get(), default_stream_index_, std::numeric_limits<int64_t>::min(), fixed_target, fixed_target, 0), print());		
		
		video_stream_.push(nullptr);
		audio_stream_.push(nullptr);
	}

	void tick()
	{
		if(seek_target_)				
		{
			internal_seek(*seek_target_);
			seek_target_.reset();
		}

		auto packet = create_packet();
		
		auto ret = av_read_frame(format_context_.get(), packet.get()); // packet is only valid until next call of av_read_frame. Use av_dup_packet to extend its life.	
		
		if(is_eof(ret))														     
		{
			video_stream_.push(packet);
			audio_stream_.push(packet);

			if(loop_)			
				internal_seek(start_);				
		}
		else
		{		
			THROW_ON_ERROR(ret, "av_read_frame", print());
					
			THROW_ON_ERROR2(av_dup_packet(packet.get()), print());
				
			// Make sure that the packet is correctly deallocated even if size and data is modified during decoding.
			const auto size = packet->size;
			const auto data = packet->data;
			
			packet = spl::shared_ptr<AVPacket>(packet.get(), [packet, size, data](AVPacket*)
			{
				packet->size = size;
				packet->data = data;				
			});
					
			const auto stream_time_base = format_context_->streams[packet->stream_index]->time_base;
			const auto packet_frame_number = static_cast<uint32_t>((static_cast<double>(packet->pts * stream_time_base.num)/stream_time_base.den)*fps_);

			if(packet->stream_index == default_stream_index_)
				frame_number_ = packet_frame_number;
					
			if(packet_frame_number >= start_ && packet_frame_number < length_)
			{
				video_stream_.push(packet);
				audio_stream_.push(packet);
			}
		}	
						
		graph_->set_value("video-buffer", std::min(1.0, static_cast<double>(video_stream_.size()/MIN_FRAMES)));
		graph_->set_value("audio-buffer", std::min(1.0, static_cast<double>(audio_stream_.size()/MIN_FRAMES)));
	}
			
	bool full() const
	{
		return video_stream_.size() > MIN_FRAMES && audio_stream_.size() > MIN_FRAMES;
	}

	void run()
	{
		win32_exception::install_handler();

		while(is_running_)
		{
			try
			{
				boost::this_thread::sleep(boost::posix_time::milliseconds(1));
				
				{
					boost::unique_lock<boost::mutex> lock(mutex_);

					while(full() && !seek_target_ && is_running_)
						cond_.wait(lock);
					
					tick();
				}
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
				is_running_ = false;
			}
		}
	}
			
	bool is_eof(int ret)
	{
		#pragma warning (disable : 4146)
		return ret == AVERROR_EOF || ret == AVERROR(EIO) || frame_number_ >= length_; // av_read_frame doesn't always correctly return AVERROR_EOF;
	}
};

input::input(const spl::shared_ptr<diagnostics::graph>& graph, const std::wstring& filename, bool loop, uint32_t start, uint32_t length) 
	: impl_(new impl(graph, filename, loop, start, length)){}
bool input::try_pop_video(std::shared_ptr<AVPacket>& packet){return impl_->try_pop_video(packet);}
bool input::try_pop_audio(std::shared_ptr<AVPacket>& packet){return impl_->try_pop_audio(packet);}
AVFormatContext& input::context(){return *impl_->format_context_;}
void input::loop(bool value){impl_->loop_ = value;}
bool input::loop() const{return impl_->loop_;}
void input::seek(uint32_t target){impl_->seek(target);}
void input::start(uint32_t value){impl_->start_ = value;}
uint32_t input::start() const{return impl_->start_;}
void input::length(uint32_t value){impl_->length_ = value;}
uint32_t input::length() const{return impl_->length_;}
}}
