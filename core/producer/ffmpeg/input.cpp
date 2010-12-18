#include "..\..\stdafx.h"

#include "input.h"

#include "../../format/video_format.h"
#include "../../../common/utility/scope_exit.h"
#include "../../../common/concurrency/concurrent_queue.h"

#include <tbb/concurrent_queue.h>
#include <tbb/queuing_mutex.h>

#include <boost/exception/error_info.hpp>
#include <boost/utility/value_init.hpp>

#include <errno.h>
#include <system_error>
		
#if defined(_MSC_VER)
#pragma warning (disable : 4244)
#endif

#include <boost/thread.hpp>

extern "C" 
{
	#define __STDC_CONSTANT_MACROS
	#define __STDC_LIMIT_MACROS
	#include <libavformat/avformat.h>
}

namespace caspar { namespace core { namespace ffmpeg{
		
struct input::implementation : public std::enable_shared_from_this<implementation>, boost::noncopyable
{
	static const size_t BUFFER_SIZE = 2 << 25;

	implementation(const std::string& filename) : video_s_index_(-1), audio_s_index_(-1), filename_(filename)
	{
		loop_ = false;	
		
		int errn;
		AVFormatContext* weak_format_context_;
		if((errn = -av_open_input_file(&weak_format_context_, filename.c_str(), nullptr, 0, nullptr)) > 0)
			BOOST_THROW_EXCEPTION(
				file_read_error() << 
				msg_info("No format context found.") << 
				boost::errinfo_api_function("av_open_input_file") <<
				boost::errinfo_errno(errn) <<
				boost::errinfo_file_name(filename));

		format_context_.reset(weak_format_context_, av_close_input_file);
			
		if((errn = -av_find_stream_info(format_context_.get())) > 0)
			BOOST_THROW_EXCEPTION(
				file_read_error() << 
				boost::errinfo_api_function("av_find_stream_info") <<
				msg_info("No stream found.") << 
				boost::errinfo_errno(errn));

		video_codec_context_ = open_stream(CODEC_TYPE_VIDEO, video_s_index_);
		if(!video_codec_context_)
			CASPAR_LOG(warning) << "No video stream found.";
		
		audio_codex_context_ = open_stream(CODEC_TYPE_AUDIO, audio_s_index_);
		if(!audio_codex_context_)
			CASPAR_LOG(warning) << "No audio stream found.";

		if(!video_codec_context_ && !audio_codex_context_)
			BOOST_THROW_EXCEPTION(file_read_error() << msg_info("No video or audio codec context found."));		
				
		is_running_ = true;
		io_thread_ = boost::thread([=]{read_file();});
	}

	void stop()
	{		
		is_running_ = false;
		buffer_size_ = 0;
		cond_.notify_one();
	}
				
	std::shared_ptr<AVCodecContext> open_stream(int codec_type, int& s_index)
	{		
		AVStream** streams_end = format_context_->streams+format_context_->nb_streams;
		AVStream** stream = std::find_if(format_context_->streams, streams_end, 
			[&](AVStream* stream) { return stream != nullptr && stream->codec->codec_type == codec_type ;});
		
		if(stream == streams_end) 
			return nullptr;

		s_index = (*stream)->index;
		
		auto codec = avcodec_find_decoder((*stream)->codec->codec_id);			
		if(codec == nullptr)
			return nullptr;
			
		if((-avcodec_open((*stream)->codec, codec)) > 0)		
			return nullptr;

		return std::shared_ptr<AVCodecContext>((*stream)->codec, avcodec_close);
	}
		
	void read_file()
	{	
		static tbb::atomic<size_t> instances(boost::initialized_value); // Dangling threads debug info.

		CASPAR_LOG(info) << "Started ffmpeg_producer::read_file Thread for " << filename_.c_str() << " instances: " << ++instances;
		win32_exception::install_handler();
		
		try
		{
			auto keep_alive = shared_from_this(); // keep alive this while thread is running.
			while(keep_alive->is_running_)
			{
				{
					AVPacket tmp_packet;
					safe_ptr<AVPacket> read_packet(&tmp_packet, av_free_packet);	
					tbb::queuing_mutex::scoped_lock lock(seek_mutex_);	

					if (av_read_frame(format_context_.get(), read_packet.get()) >= 0) // NOTE: read_packet is only valid until next call of av_safe_ptr<read_frame> or av_close_input_file
					{
						auto packet = aligned_buffer(read_packet->data, read_packet->data + read_packet->size);
						if(read_packet->stream_index == video_s_index_) 						
						{
							buffer_size_ += packet.size();
							video_packet_buffer_.try_push(std::move(packet));						
						}
						else if(read_packet->stream_index == audio_s_index_) 	
						{
							buffer_size_ += packet.size();
							audio_packet_buffer_.try_push(std::move(packet));
						}
					}
					else if(!loop_ || av_seek_frame(format_context_.get(), -1, 0, AVSEEK_FLAG_BACKWARD) < 0) // TODO: av_seek_frame does not work for all formats
						is_running_ = false;
				}

				if(!need_packet() && is_running_)
					boost::this_thread::sleep(boost::posix_time::milliseconds(10)); // Read packets in max 100 pps

				if(buffer_size_ >= BUFFER_SIZE)
				{
					boost::unique_lock<boost::mutex> lock(mut_);
					while(buffer_size_ >= BUFFER_SIZE && !need_packet() && is_running_)
						cond_.wait(lock);
				}
			}
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
		}
		
		is_running_ = false;
		
		CASPAR_LOG(info) << " Ended ffmpeg_producer::read_file Thread for " << filename_.c_str() << " instances: " << --instances;
	}

	bool need_packet()
	{
		return video_packet_buffer_.size() < 3 || audio_packet_buffer_.size() < 3;
	}
	
	aligned_buffer get_video_packet()
	{
		return get_packet(video_packet_buffer_);
	}

	aligned_buffer get_audio_packet()
	{
		return get_packet(audio_packet_buffer_);
	}
	
	aligned_buffer get_packet(concurrent_bounded_queue_r<aligned_buffer>& buffer)
	{
		aligned_buffer packet;
		if(buffer.try_pop(packet))
		{
			buffer_size_ -= packet.size();
			cond_.notify_one();
		}
		return std::move(packet);
	}

	bool is_eof() const
	{
		return !is_running_ && video_packet_buffer_.empty() && audio_packet_buffer_.empty();
	}
		
	// TODO: Not properly done.
	bool seek(unsigned long long seek_target)
	{
		tbb::queuing_mutex::scoped_lock lock(seek_mutex_);
		if(av_seek_frame(format_context_.get(), -1, seek_target*AV_TIME_BASE, 0) < 0)
			return false;
		
		video_packet_buffer_.clear();
		audio_packet_buffer_.clear();
		// TODO: Not sure its enough to just flush in input class
		if(video_codec_context_)
			avcodec_flush_buffers(video_codec_context_.get());
		if(audio_codex_context_)
			avcodec_flush_buffers(audio_codex_context_.get());
		return true;
	}
				
	std::shared_ptr<AVFormatContext>	format_context_;	// Destroy this last

	tbb::queuing_mutex					seek_mutex_;

	const std::string					filename_;

	std::shared_ptr<AVCodecContext>		video_codec_context_;

	std::shared_ptr<AVCodecContext>		audio_codex_context_;

	tbb::atomic<bool>					loop_;
	int									video_s_index_;
	int									audio_s_index_;
	
	concurrent_bounded_queue_r<aligned_buffer> video_packet_buffer_;
	concurrent_bounded_queue_r<aligned_buffer> audio_packet_buffer_;
	
	boost::mutex				mut_;
	boost::condition_variable	cond_;
	tbb::atomic<size_t>			buffer_size_;

	boost::thread	io_thread_;
	tbb::atomic<bool> is_running_;
};

input::input(const std::string& filename) : impl_(new implementation(filename)){}
input::~input(){impl_->stop();}
void input::set_loop(bool value){impl_->loop_ = value;}
const std::shared_ptr<AVCodecContext>& input::get_video_codec_context() const{return impl_->video_codec_context_;}
const std::shared_ptr<AVCodecContext>& input::get_audio_codec_context() const{return impl_->audio_codex_context_;}
bool input::is_eof() const{return impl_->is_eof();}
aligned_buffer input::get_video_packet(){return impl_->get_video_packet();}
aligned_buffer input::get_audio_packet(){return impl_->get_audio_packet();}
bool input::seek(unsigned long long frame){return impl_->seek(frame);}
}}}