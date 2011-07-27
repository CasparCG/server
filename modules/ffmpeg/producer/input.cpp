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

#include "..\stdafx.h"

#include "input.h"
#include "../ffmpeg_error.h"
#include "../tbb_avcodec.h"

#include <core/video_format.h>

#include <common/concurrency/executor.h>
#include <common/diagnostics/graph.h>

#include <tbb/concurrent_queue.h>
#include <tbb/atomic.h>

#include <boost/range/algorithm.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/range/iterator_range.hpp>

extern "C" 
{
	#define __STDC_CONSTANT_MACROS
	#define __STDC_LIMIT_MACROS
	#include <libavformat/avformat.h>
}

namespace caspar {

static const size_t MAX_BUFFER_COUNT = 128;
static const size_t MAX_BUFFER_SIZE  = 32 * 1000000;
	
struct input::implementation : boost::noncopyable
{		
	safe_ptr<diagnostics::graph> graph_;

	std::shared_ptr<AVFormatContext> format_context_;	// Destroy this last
		
	const std::wstring			filename_;
	const bool					loop_;
	const int					start_;		

	size_t						buffer_size_limit_;
	tbb::atomic<size_t>			buffer_size_;
	boost::condition_variable	cond_;
	boost::mutex				mutex_;

	tbb::concurrent_bounded_queue<std::shared_ptr<AVPacket>> buffer_;
		
	executor executor_;
public:
	explicit implementation(const safe_ptr<diagnostics::graph>& graph, const std::wstring& filename, bool loop, int start) 
		: graph_(graph)
		, loop_(loop)
		, filename_(filename)
		, executor_(print())
		, start_(std::max(start, 0))
	{			
		int errn;

		buffer_.set_capacity(MAX_BUFFER_COUNT);

		AVFormatContext* weak_format_context_ = nullptr;
		errn = av_open_input_file(&weak_format_context_, narrow(filename).c_str(), nullptr, 0, nullptr);
		if(errn < 0 || weak_format_context_ == nullptr)
		{	
			BOOST_THROW_EXCEPTION(
				file_read_error() << 
				source_info(narrow(print())) << 
				msg_info(av_error_str(errn)) <<
				boost::errinfo_api_function("av_open_input_file") <<
				boost::errinfo_errno(AVUNERROR(errn)) <<
				boost::errinfo_file_name(narrow(filename)));
		}

		format_context_.reset(weak_format_context_, av_close_input_file);
			
		errn = av_find_stream_info(format_context_.get());
		if(errn < 0)
		{	
			BOOST_THROW_EXCEPTION(
				file_read_error() << 
				source_info(narrow(print())) << 
				msg_info(av_error_str(errn)) <<
				boost::errinfo_api_function("av_find_stream_info") <<
				boost::errinfo_errno(AVUNERROR(errn)));
		}
		
		if(start_ != 0)			
			seek_frame(start_);
				
		graph_->set_color("seek", diagnostics::color(0.5f, 1.0f, 0.5f));	
		graph_->set_color("buffer-count", diagnostics::color(0.2f, 0.8f, 1.0f));
		graph_->set_color("buffer-size", diagnostics::color(0.2f, 0.4f, 1.0f));	

		executor_.begin_invoke([this]{read_file();});
		CASPAR_LOG(info) << print() << " Started.";
	}

	~implementation()
	{
		stop();
		// Unblock thread.
		std::shared_ptr<AVPacket> packet;
		buffer_.try_pop(packet);
		buffer_size_ = 0;
		cond_.notify_all();
	}
		
	bool try_pop(std::shared_ptr<AVPacket>& packet)
	{
		bool result = buffer_.try_pop(packet);
		graph_->update_value("buffer-count", MAX_BUFFER_SIZE/static_cast<double>(buffer_.size()));
		if(packet)
		{
			buffer_size_ -= packet->size;
			graph_->update_value("buffer-size", MAX_BUFFER_SIZE/static_cast<double>(buffer_size_));
			cond_.notify_all();
		}
		return result;
	}

private:

	void stop()
	{
		executor_.stop();
		
		CASPAR_LOG(info) << print() << " Stopping.";
	}

	void read_file()
	{		
		read_next_packet();
		executor_.begin_invoke([this]{read_file();});
	}
			
	void read_next_packet()
	{		
		try
		{
			std::shared_ptr<AVPacket> read_packet(new AVPacket, [](AVPacket* p)
			{
				av_free_packet(p);
				delete p;
			});
			av_init_packet(read_packet.get());

			const int errn = av_read_frame(format_context_.get(), read_packet.get()); // read_packet is only valid until next call of av_read_frame.
			if(is_eof(errn))														  // Use av_dup_packet to extend its life.
			{
				if(loop_)
				{
					seek_frame(start_, AVSEEK_FLAG_BACKWARD);
					graph_->add_tag("seek");		
					//CASPAR_LOG(info) << print() << " Received EOF. Looping.";			
				}	
				else
				{
					stop();
					//CASPAR_LOG(info) << print() << " Received EOF. Stopping.";
				}
			}
			else if(errn < 0)
			{
				BOOST_THROW_EXCEPTION(
					file_read_error() <<
					msg_info(av_error_str(errn)) <<
					source_info(narrow(print())) << 
					boost::errinfo_api_function("av_read_frame") <<
					boost::errinfo_errno(AVUNERROR(errn)));
			}
			else
			{
				av_dup_packet(read_packet.get());
				buffer_.push(read_packet);

				graph_->update_value("buffer-count", MAX_BUFFER_SIZE/static_cast<double>(buffer_.size()));
				
				boost::unique_lock<boost::mutex> lock(mutex_);
				while(buffer_size_ > MAX_BUFFER_SIZE && buffer_.size() > 2)
					cond_.wait(lock);

				buffer_size_ += read_packet->size;

				graph_->update_value("buffer-size", MAX_BUFFER_SIZE/static_cast<double>(buffer_size_));
			}							
		}
		catch(...)
		{
			stop();
			CASPAR_LOG_CURRENT_EXCEPTION();
			return;
		}	
	}

	void seek_frame(int64_t frame, int flags = 0)
	{  	
		static const AVRational base_q = {1, AV_TIME_BASE};

		// Convert from frames into seconds.
		auto seek_target = frame;//*static_cast<int64_t>(AV_TIME_BASE/fps_);

		int stream_index = -1;//video_stream_.index() >= 0 ? video_stream_.index() : audio_stream_.index();

		//if(stream_index >= 0)		
		//	seek_target = av_rescale_q(seek_target, base_q, format_context_->streams[stream_index]->time_base);

		const int errn = av_seek_frame(format_context_.get(), stream_index, seek_target, flags);
		if(errn < 0)
		{	
			BOOST_THROW_EXCEPTION(
				invalid_operation() << 
				source_info(narrow(print())) << 
				msg_info(av_error_str(errn)) <<
				boost::errinfo_api_function("av_seek_frame") <<
				boost::errinfo_errno(AVUNERROR(errn)));
		}

		buffer_.push(nullptr);
	}		

	bool is_eof(int errn)
	{
		//if(errn == AVERROR(EIO))
		//	CASPAR_LOG(warning) << print() << " Received EIO, assuming EOF";

		return errn == AVERROR_EOF || errn == AVERROR(EIO); // av_read_frame doesn't always correctly return AVERROR_EOF;
	}
	
	std::wstring print() const
	{
		return L"ffmpeg_input[" + filename_ + L")]";
	}
};

input::input(const safe_ptr<diagnostics::graph>& graph, const std::wstring& filename, bool loop, int start, int length) 
	: impl_(new implementation(graph, filename, loop, start)){}
bool input::eof() const {return !impl_->executor_.is_running();}
bool input::try_pop(std::shared_ptr<AVPacket>& packet){return impl_->try_pop(packet);}
std::shared_ptr<AVFormatContext> input::context(){return impl_->format_context_;}
}