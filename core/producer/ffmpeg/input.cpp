#include "..\..\stdafx.h"

#include "input.h"

#include "../../format/video_format.h"

#include <common/concurrency/executor.h>

#include <tbb/concurrent_queue.h>
#include <tbb/queuing_mutex.h>

#include <boost/exception/error_info.hpp>
#include <boost/thread/once.hpp>

#include <errno.h>
#include <system_error>
		
#if defined(_MSC_VER)
#pragma warning (disable : 4244)
#endif


extern "C" 
{
	#define __STDC_CONSTANT_MACROS
	#define __STDC_LIMIT_MACROS
	#include <libavformat/avformat.h>
}

namespace caspar { namespace core { namespace ffmpeg{
		
struct input::implementation : boost::noncopyable
{				
	std::shared_ptr<AVFormatContext>	format_context_;	// Destroy this last

	std::shared_ptr<AVCodecContext>		video_codec_context_;
	std::shared_ptr<AVCodecContext>		audio_codex_context_;

	tbb::queuing_mutex					seek_mutex_;

	const std::wstring					filename_;

	tbb::atomic<bool>					loop_;
	int									video_s_index_;
	int									audio_s_index_;

	tbb::atomic<size_t>	buffer_size_;
	
	tbb::concurrent_bounded_queue<std::shared_ptr<aligned_buffer>> video_packet_buffer_;
	tbb::concurrent_bounded_queue<std::shared_ptr<aligned_buffer>> audio_packet_buffer_;
	
	executor executor_;

	static const size_t BUFFER_SIZE = 2 << 25;

public:
	explicit implementation(const std::wstring& filename) 
		: video_s_index_(-1)
		, audio_s_index_(-1)
		, filename_(filename)
	{		
		static boost::once_flag av_register_all_flag = BOOST_ONCE_INIT;
		boost::call_once(av_register_all, av_register_all_flag);	
		
		static boost::once_flag avcodec_init_flag = BOOST_ONCE_INIT;
		boost::call_once(avcodec_init, avcodec_init_flag);	

		loop_ = false;	
		
		int errn;
		AVFormatContext* weak_format_context_;
		if((errn = -av_open_input_file(&weak_format_context_, narrow(filename).c_str(), nullptr, 0, nullptr)) > 0)
			BOOST_THROW_EXCEPTION(
				file_read_error() << 
				msg_info("No format context found.") << 
				boost::errinfo_api_function("av_open_input_file") <<
				boost::errinfo_errno(errn) <<
				boost::errinfo_file_name(narrow(filename)));

		format_context_.reset(weak_format_context_, av_close_input_file);
			
		if((errn = -av_find_stream_info(format_context_.get())) > 0)
			BOOST_THROW_EXCEPTION(
				file_read_error() << 
				boost::errinfo_api_function("av_find_stream_info") <<
				msg_info("No stream found.") << 
				boost::errinfo_errno(errn));

		video_codec_context_ = open_stream(CODEC_TYPE_VIDEO, video_s_index_);
		if(!video_codec_context_)
			CASPAR_LOG(warning) << "Could not open any video stream.";
		
		audio_codex_context_ = open_stream(CODEC_TYPE_AUDIO, audio_s_index_);
		if(!audio_codex_context_)
			CASPAR_LOG(warning) << "Could not open any audio stream.";

		if(!video_codec_context_ && !audio_codex_context_)
			BOOST_THROW_EXCEPTION(file_read_error() << msg_info("No video or audio codec context found."));		
			
		executor_.start();
		executor_.begin_invoke([this]{read_file();});
		CASPAR_LOG(info) << print() << " started.";
	}

	~implementation()
	{
		CASPAR_LOG(info) << print() << " ended.";
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
		
	void read_file() // For every packet taken: read in a number of packets.
	{		
		for(size_t n = 0; buffer_size_ < BUFFER_SIZE && (n < 3 || video_packet_buffer_.size() < 3 || audio_packet_buffer_.size() < 3) && executor_.is_running(); ++n)
		{
			AVPacket tmp_packet;
			safe_ptr<AVPacket> read_packet(&tmp_packet, av_free_packet);	
			tbb::queuing_mutex::scoped_lock lock(seek_mutex_);	

			if (av_read_frame(format_context_.get(), read_packet.get()) >= 0) // NOTE: read_packet is only valid until next call of av_safe_ptr<read_frame> or av_close_input_file
			{
				auto packet = std::make_shared<aligned_buffer>(read_packet->data, read_packet->data + read_packet->size);
				if(read_packet->stream_index == video_s_index_) 						
				{
					buffer_size_ += packet->size();
					video_packet_buffer_.try_push(std::move(packet));						
				}
				else if(read_packet->stream_index == audio_s_index_) 	
				{
					buffer_size_ += packet->size();
					audio_packet_buffer_.try_push(std::move(packet));
				}
			}
			else if(!loop_ || av_seek_frame(format_context_.get(), -1, 0, AVSEEK_FLAG_BACKWARD) < 0) // TODO: av_seek_frame does not work for all formats
				executor_.stop(executor::no_wait);
		}
	}
		
	aligned_buffer get_video_packet()
	{
		return get_packet(video_packet_buffer_);
	}

	aligned_buffer get_audio_packet()
	{
		return get_packet(audio_packet_buffer_);
	}
	
	aligned_buffer get_packet(tbb::concurrent_bounded_queue<std::shared_ptr<aligned_buffer>>& buffer)
	{
		std::shared_ptr<aligned_buffer> packet;
		if(buffer.try_pop(packet))
		{
			buffer_size_ -= packet->size();
			if(executor_.size() < 4)
				executor_.begin_invoke([this]{read_file();});
			return std::move(*packet);
		}
		return aligned_buffer();
	}

	bool is_eof() const
	{
		return !executor_.is_running() && video_packet_buffer_.empty() && audio_packet_buffer_.empty();
	}
		
	// TODO: Not properly done.
	bool seek(unsigned long long seek_target)
	{
		tbb::queuing_mutex::scoped_lock lock(seek_mutex_);
		if(av_seek_frame(format_context_.get(), -1, seek_target*AV_TIME_BASE, 0) < 0)
			return false;
		
		video_packet_buffer_.clear();
		audio_packet_buffer_.clear();

		return true;
	}

	double fps() const
	{
		return static_cast<double>(video_codec_context_->time_base.den) / static_cast<double>(video_codec_context_->time_base.num);
	}

	std::wstring print() const
	{
		return L"ffmpeg[" + boost::filesystem::wpath(filename_).filename() + L"] Buffer thread";
	}
};

input::input(const std::wstring& filename) : impl_(new implementation(filename)){}
void input::set_loop(bool value){impl_->loop_ = value;}
const std::shared_ptr<AVCodecContext>& input::get_video_codec_context() const{return impl_->video_codec_context_;}
const std::shared_ptr<AVCodecContext>& input::get_audio_codec_context() const{return impl_->audio_codex_context_;}
bool input::is_eof() const{return impl_->is_eof();}
aligned_buffer input::get_video_packet(){return impl_->get_video_packet();}
aligned_buffer input::get_audio_packet(){return impl_->get_audio_packet();}
bool input::seek(unsigned long long frame){return impl_->seek(frame);}
double input::fps() const { return impl_->fps(); }
}}}