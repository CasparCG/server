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

#include "../../StdAfx.h"

#include "input.h"

#include "../util/util.h"
#include "../../ffmpeg_error.h"
#include "../../ffmpeg.h"

#include <common/diagnostics/graph.h>
#include <common/executor.h>
#include <common/lock.h>
//#include <common/except.h>
#include <common/os/general_protection_fault.h>
#include <common/log.h>

#include <core/video_format.h>

#include <tbb/concurrent_queue.h>
#include <tbb/atomic.h>
#include <tbb/recursive_mutex.h>

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

static const int MAX_PUSH_WITHOUT_POP = 200;
static const int MIN_FRAMES = 25;

class stream
{
	stream(const stream&);
	stream& operator=(const stream&);

	typedef tbb::concurrent_bounded_queue<std::shared_ptr<AVPacket>>::size_type size_type;

	int															index_;
	tbb::concurrent_bounded_queue<std::shared_ptr<AVPacket>>	packets_;
	tbb::atomic<int>											push_since_pop_;
public:

	stream(int index) 
		: index_(index)
	{
		push_since_pop_ = 0;
	}

	stream(stream&&) = default;

	bool is_available() const
	{
		return index_ >= 0;
	}

	int index() const
	{
		return index_;
	}
	
	void push(const std::shared_ptr<AVPacket>& packet)
	{
		if(packet && packet->data && packet->stream_index != index_)
			return;

		if (++push_since_pop_ > MAX_PUSH_WITHOUT_POP) // Out of memory protection for streams never being used.
		{
			return;
		}

		packets_.push(packet);
	}

	bool try_pop(std::shared_ptr<AVPacket>& packet)
	{
		push_since_pop_ = 0;

		return packets_.try_pop(packet);
	}

	void clear()
	{
		std::shared_ptr<AVPacket> packet;
		push_since_pop_ = 0;
		while(packets_.try_pop(packet));
	}
		
	size_type size() const
	{
		return is_available() ? packets_.size() : std::numeric_limits<size_type>::max();
	}
};
		
struct input::impl : boost::noncopyable
{		
	const spl::shared_ptr<diagnostics::graph>	graph_;

	const std::wstring					  		filename_;
	const spl::shared_ptr<AVFormatContext>		format_context_			= open_input(filename_); // Destroy this last
	const int							  		default_stream_index_	= av_find_default_stream_index(format_context_.get());

	tbb::atomic<uint32_t>				  		start_;		
	tbb::atomic<uint32_t>				  		length_;
	tbb::atomic<bool>					  		loop_;
	tbb::atomic<bool>					  		eof_;
	bool										thumbnail_mode_;
	double								  		fps_					= read_fps(*format_context_, 0.0);
	uint32_t							  		frame_number_			= 0;

	stream								  		video_stream_			{							av_find_best_stream(format_context_.get(), AVMEDIA_TYPE_VIDEO, -1, -1, 0, 0) };
	std::vector<stream>							audio_streams_;

	boost::optional<uint32_t>			  		seek_target_;

	tbb::atomic<bool>					  		is_running_;
	boost::mutex						  		mutex_;
	boost::condition_variable			  		cond_;
	boost::thread						  		thread_;
	
	impl(
			const spl::shared_ptr<diagnostics::graph> graph,
			const std::wstring& filename,
			const bool loop,
			const uint32_t start,
			const uint32_t length,
			bool thumbnail_mode)
		: graph_(graph)
		, filename_(filename)
		, thumbnail_mode_(thumbnail_mode)
	{
		start_			= start;
		length_			= length;
		loop_			= loop;
		eof_			= false;
		is_running_		= true;

		if(start_ != 0)
			seek_target_ = start_;
														
		graph_->set_color("seek", diagnostics::color(1.0f, 0.5f, 0.0f));

		if (!thumbnail_mode_)
			for (unsigned i = 0; i < format_context_->nb_streams; ++i)
				if (format_context_->streams[i]->codec->codec_type == AVMediaType::AVMEDIA_TYPE_AUDIO)
					audio_streams_.emplace_back(i);

		for (int i = 0; i < audio_streams_.size(); ++i)
			graph_->set_color("audio-buffer" + boost::lexical_cast<std::string>(i + 1), diagnostics::color(0.7f, 0.4f, 0.4f));

		if (video_stream_.is_available())
			graph_->set_color("video-buffer", diagnostics::color(1.0f, 1.0f, 0.0f));
		
		for(int n = 0; n < 8; ++n)
			tick();

		if (!thumbnail_mode)
			thread_	= boost::thread([this]{run();});
	}

	~impl()
	{
		is_running_ = false;
		cond_.notify_one();

		if (!thumbnail_mode_)
			thread_.join();
	}
	
	bool try_pop_video(std::shared_ptr<AVPacket>& packet)
	{
		if (!video_stream_.is_available())
			return false;

		if (thumbnail_mode_)
		{
			int ticks = 0;
			while (!video_stream_.try_pop(packet))
			{
				tick();
				if (++ticks > 32) // Infinite loop should not be possible
					return false;

				// Play nice
				boost::this_thread::sleep_for(boost::chrono::milliseconds(5));
			}

			return true;
		}

		bool result = video_stream_.try_pop(packet);

		if(result)
			cond_.notify_one();
		
		graph_->set_value("video-buffer", std::min(1.0, static_cast<double>(video_stream_.size())/MIN_FRAMES));
				
		return result;
	}
	
	bool try_pop_audio(std::shared_ptr<AVPacket>& packet, int audio_stream_index)
	{
		if (audio_streams_.size() < audio_stream_index + 1)
			return false;

		auto& audio_stream = audio_streams_.at(audio_stream_index);
		bool result = audio_stream.try_pop(packet);
		if(result)
			cond_.notify_one();

		auto buffer_nr = boost::lexical_cast<std::string>(audio_stream_index + 1);
		graph_->set_value("audio-buffer" + buffer_nr, std::min(1.0, static_cast<double>(audio_stream.size())/MIN_FRAMES));

		return result;
	}

	void seek(uint32_t target)
	{
		{
			boost::lock_guard<boost::mutex> lock(mutex_);

			seek_target_ = target;
			video_stream_.clear();

			for (auto& audio_stream : audio_streams_)
				audio_stream.clear();
		}

		cond_.notify_one();
	}

	int get_actual_audio_stream_index(int audio_stream_index) const
	{
		if (audio_stream_index + 1 > audio_streams_.size())
			CASPAR_THROW_EXCEPTION(averror_stream_not_found());

		return audio_streams_.at(audio_stream_index).index();
	}
		
	std::wstring print() const
	{
		return L"ffmpeg_input[" + filename_ + L")]";
	}

private:
	void internal_seek(uint32_t target)
	{
		eof_ = false;
		graph_->set_tag(diagnostics::tag_severity::INFO, "seek");

		if (is_logging_quiet_for_thread())
			CASPAR_LOG(trace) << print() << " Seeking: " << target;
		else
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
		
		auto stream				= format_context_->streams[default_stream_index_];
		auto fps				= read_fps(*format_context_, 0.0);
		auto target_timestamp = static_cast<int64_t>((target / fps * stream->time_base.den) / stream->time_base.num);
		
		THROW_ON_ERROR2(avformat_seek_file(
				format_context_.get(),
				default_stream_index_,
				std::numeric_limits<int64_t>::min(),
				target_timestamp,
				std::numeric_limits<int64_t>::max(),
				0), print());
		
		video_stream_.push(nullptr);

		for (auto& audio_stream : audio_streams_)
			audio_stream.push(nullptr);
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
			if (loop_)
				internal_seek(start_);
			else
			{
				eof_ = true;
			}
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

				for (auto& audio_stream : audio_streams_)
					audio_stream.push(packet);
			}
		}	

		if (video_stream_.is_available())
			graph_->set_value("video-buffer", std::min(1.0, static_cast<double>(video_stream_.size())/MIN_FRAMES));

		for (int i = 0; i < audio_streams_.size(); ++i)
			graph_->set_value(
					"audio-buffer" + boost::lexical_cast<std::string>(i + 1),
					std::min(1.0, static_cast<double>(audio_streams_[i].size())/MIN_FRAMES));
	}
			
	bool full() const
	{
		bool video_full = video_stream_.size() >= MIN_FRAMES;

		if (!video_full)
			return false;

		for (auto& audio_stream : audio_streams_)
			if (audio_stream.size() < MIN_FRAMES)
				return false;

		return true;
	}

	void run()
	{
		ensure_gpf_handler_installed_for_thread(u8(print()).c_str());

		while(is_running_)
		{
			try
			{
				
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

input::input(const spl::shared_ptr<diagnostics::graph>& graph, const std::wstring& filename, bool loop, uint32_t start, uint32_t length, bool thumbnail_mode)
	: impl_(new impl(graph, filename, loop, start, length, thumbnail_mode)){}
int input::get_actual_audio_stream_index(int audio_stream_index) const { return impl_->get_actual_audio_stream_index(audio_stream_index); };
int input::num_audio_streams() const { return static_cast<int>(impl_->audio_streams_.size()); }
bool input::try_pop_video(std::shared_ptr<AVPacket>& packet){return impl_->try_pop_video(packet);}
bool input::try_pop_audio(std::shared_ptr<AVPacket>& packet, int audio_stream_index){return impl_->try_pop_audio(packet, audio_stream_index);}
AVFormatContext& input::context(){return *impl_->format_context_;}
void input::loop(bool value){impl_->loop_ = value;}
bool input::loop() const{return impl_->loop_;}
void input::seek(uint32_t target){impl_->seek(target);}
void input::start(uint32_t value){impl_->start_ = value;}
uint32_t input::start() const{return impl_->start_;}
void input::length(uint32_t value){impl_->length_ = value;}
uint32_t input::length() const{return impl_->length_;}
bool input::eof() const { return impl_->eof_; }
}}
