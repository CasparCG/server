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
#if defined(_MSC_VER)
#pragma warning (disable : 4244)
#endif

#include "../stdafx.h"

#include "input.h"
#include "../ffmpeg_error.h"
#include "../tbb_avcodec.h"

#include <core/video_format.h>

#include <common/diagnostics/graph.h>
#include <common/exception/exceptions.h>
#include <common/exception/win32_exception.h>

#include <tbb/concurrent_queue.h>
#include <tbb/atomic.h>

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

namespace caspar {

static const size_t MAX_BUFFER_COUNT = 50;
static const size_t MIN_BUFFER_COUNT = 4;
static const size_t MAX_BUFFER_SIZE  = 32 * 1000000;
	
struct input::implementation : boost::noncopyable
{		
	std::shared_ptr<AVFormatContext>							format_context_; // Destroy this last
	int															default_stream_index_;

	safe_ptr<diagnostics::graph>								graph_;
		
	const std::wstring											filename_;
	const bool													loop_;
	const size_t												start_;		
	const size_t												length_;
	size_t														frame_number_;
	
	tbb::concurrent_bounded_queue<std::shared_ptr<AVPacket>>	buffer_;
	tbb::atomic<size_t>											buffer_size_;
	boost::condition_variable									buffer_cond_;
	boost::mutex												buffer_mutex_;
		
	boost::thread												thread_;
	tbb::atomic<bool>											is_running_;

	tbb::atomic<size_t>											nb_frames_;
	tbb::atomic<size_t>											nb_loops_;

public:
	explicit implementation(const safe_ptr<diagnostics::graph>& graph, const std::wstring& filename, bool loop, size_t start, size_t length) 
		: graph_(graph)
		, loop_(loop)
		, filename_(filename)
		, start_(start)
		, length_(length)
		, frame_number_(0)
	{			
		is_running_ = true;
		nb_frames_	= 0;
		nb_loops_	= 0;
		
		AVFormatContext* weak_format_context_ = nullptr;
		THROW_ON_ERROR2(avformat_open_input(&weak_format_context_, narrow(filename).c_str(), nullptr, nullptr), print());

		format_context_.reset(weak_format_context_, av_close_input_file);

		av_dump_format(weak_format_context_, 0, narrow(filename).c_str(), 0);
			
		THROW_ON_ERROR2(avformat_find_stream_info(format_context_.get(), nullptr), print());
		
		default_stream_index_ = THROW_ON_ERROR2(av_find_default_stream_index(format_context_.get()), print());

		if(start_ > 0)			
			seek_frame(start_);
		
		for(int n = 0; n < 16 && !full(); ++n)
			read_next_packet();
						
		graph_->set_color("seek", diagnostics::color(1.0f, 0.5f, 0.0f));	
		graph_->set_color("buffer-count", diagnostics::color(0.4f, 0.8f, 0.8f));
		graph_->set_color("buffer-size", diagnostics::color(0.2f, 0.4f, 0.8f));	

		thread_ = boost::thread([this]{run();});
	}

	~implementation()
	{
		is_running_ = false;
		buffer_cond_.notify_all();
		thread_.join();
	}
		
	bool try_pop(std::shared_ptr<AVPacket>& packet)
	{
		const bool result = buffer_.try_pop(packet);

		if(result)
		{
			if(packet)
				buffer_size_ -= packet->size;
			buffer_cond_.notify_all();
		}

		graph_->update_value("buffer-size", (static_cast<double>(buffer_size_)+0.001)/MAX_BUFFER_SIZE);
		graph_->update_value("buffer-count", (static_cast<double>(buffer_.size()+0.001)/MAX_BUFFER_COUNT));

		return result;
	}

	size_t nb_frames() const
	{
		return nb_frames_;
	}

	size_t nb_loops() const
	{
		return nb_loops_;
	}

private:
	
	void run()
	{		
		caspar::win32_exception::install_handler();

		try
		{
			CASPAR_LOG(info) << print() << " Thread Started.";

			while(is_running_)
			{
				{
					boost::unique_lock<boost::mutex> lock(buffer_mutex_);
					while(full())
						buffer_cond_.timed_wait(lock, boost::posix_time::millisec(20));
				}
				read_next_packet();			
			}

			CASPAR_LOG(info) << print() << " Thread Stopped.";
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
			is_running_ = false;
		}
	}
			
	void read_next_packet()
	{		
		int ret = 0;

		std::shared_ptr<AVPacket> read_packet(new AVPacket, [](AVPacket* p)
		{
			av_free_packet(p);
			delete p;
		});
		av_init_packet(read_packet.get());

		ret = av_read_frame(format_context_.get(), read_packet.get()); // read_packet is only valid until next call of av_read_frame. Use av_dup_packet to extend its life.	
		
		if(is_eof(ret))														     
		{
			++nb_loops_;
			frame_number_ = 0;

			if(loop_)
			{
				int flags = AVSEEK_FLAG_BACKWARD;

				int vid_stream_index = av_find_best_stream(format_context_.get(), AVMEDIA_TYPE_VIDEO, -1, -1, 0, 0);
				if(vid_stream_index >= 0)
				{
					auto codec_id = format_context_->streams[vid_stream_index]->codec->codec_id;
					if(codec_id == CODEC_ID_VP6A || codec_id == CODEC_ID_VP6F || codec_id == CODEC_ID_VP6)
						flags |= AVSEEK_FLAG_BYTE;
				}

				seek_frame(start_, flags);
				graph_->add_tag("seek");		
				CASPAR_LOG(trace) << print() << " Looping.";			
			}	
			else
			{
				is_running_ = false;
				CASPAR_LOG(trace) << print() << " Stopping.";
			}
		}
		else
		{		
			THROW_ON_ERROR(ret, print(), "av_read_frame");

			if(read_packet->stream_index == default_stream_index_)
			{
				if(nb_loops_ == 0)
					++nb_frames_;
				++frame_number_;
			}

			THROW_ON_ERROR2(av_dup_packet(read_packet.get()), print());
				
			// Make sure that the packet is correctly deallocated even if size and data is modified during decoding.
			auto size = read_packet->size;
			auto data = read_packet->data;

			read_packet = std::shared_ptr<AVPacket>(read_packet.get(), [=](AVPacket*)
			{
				read_packet->size = size;
				read_packet->data = data;
			});

			buffer_.try_push(read_packet);
			buffer_size_ += read_packet->size;
				
			graph_->update_value("buffer-size", (static_cast<double>(buffer_size_)+0.001)/MAX_BUFFER_SIZE);
			graph_->update_value("buffer-count", (static_cast<double>(buffer_.size()+0.001)/MAX_BUFFER_COUNT));
		}			
	}

	bool full() const
	{
		return is_running_ && (buffer_size_ > MAX_BUFFER_SIZE || buffer_.size() > MAX_BUFFER_COUNT) && buffer_.size() > MIN_BUFFER_COUNT;
	}

	void seek_frame(int64_t frame, int flags = 0)
	{  							
		THROW_ON_ERROR2(av_seek_frame(format_context_.get(), default_stream_index_, frame, flags), print());		
		buffer_.push(nullptr);
	}		

	bool is_eof(int ret)
	{
		if(ret == AVERROR(EIO))
			CASPAR_LOG(trace) << print() << " Received EIO, assuming EOF. " << nb_frames_;
		if(ret == AVERROR_EOF)
			CASPAR_LOG(trace) << print() << " Received EOF. " << nb_frames_;

		return ret == AVERROR_EOF || ret == AVERROR(EIO) || frame_number_ >= length_; // av_read_frame doesn't always correctly return AVERROR_EOF;
	}
	
	std::wstring print() const
	{
		return L"ffmpeg_input[" + filename_ + L")]";
	}
};

input::input(const safe_ptr<diagnostics::graph>& graph, const std::wstring& filename, bool loop, size_t start, size_t length) 
	: impl_(new implementation(graph, filename, loop, start, length)){}
bool input::eof() const {return !impl_->is_running_;}
bool input::try_pop(std::shared_ptr<AVPacket>& packet){return impl_->try_pop(packet);}
safe_ptr<AVFormatContext> input::context(){return make_safe(impl_->format_context_);}
size_t input::nb_frames() const {return impl_->nb_frames();}
size_t input::nb_loops() const {return impl_->nb_loops();}
}