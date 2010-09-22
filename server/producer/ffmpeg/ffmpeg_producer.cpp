#include "../../stdafx.h"

#include "ffmpeg_producer.h"

#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4244)
#endif

extern "C" 
{
	#define __STDC_CONSTANT_MACROS
	#define __STDC_LIMIT_MACROS
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libavutil/avutil.h>
	#include <libswscale/swscale.h>
}

#if defined(_MSC_VER)
#pragma warning (pop)
#endif

#include "input.h"

#include "audio/audio_decoder.h"
#include "video/video_decoder.h"
#include "video/video_deinterlacer.h"
#include "video/video_scaler.h"

#include "../../frame/format.h"
#include "../../../common/utility/find_file.h"
#include "../../server.h"
#include "../../../common/image/image.h"
#include "../../../common/utility/scope_exit.h"

#include <tbb/mutex.h>
#include <tbb/parallel_invoke.h>
#include <tbb/task_group.h>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <boost/thread/once.hpp>

using namespace boost::assign;

namespace caspar{ namespace ffmpeg{
	
struct ffmpeg_producer : public frame_producer
{
public:
	static const size_t MAX_TOKENS = 5;
	static const size_t MIN_BUFFER_SIZE = 2;
	static const size_t DEFAULT_BUFFER_SIZE = 8;
	static const size_t MAX_BUFFER_SIZE = 64;
	static const size_t LOAD_TARGET_BUFFER_SIZE = 4;
	static const size_t THREAD_TIMEOUT_MS = 1000;

	ffmpeg_producer(const std::wstring& filename, const  std::vector<std::wstring>& params) 
		: filename_(filename)
	{
    	if(!boost::filesystem::exists(filename))
    		BOOST_THROW_EXCEPTION(file_not_found() <<  boost::errinfo_file_name(common::narrow(filename)));
		
		static boost::once_flag flag = BOOST_ONCE_INIT;
		boost::call_once(av_register_all, flag);	
		
		input_.reset(new input());
		input_->set_loop(std::find(params.begin(), params.end(), L"LOOP") != params.end());
		input_->load(common::narrow(filename_));

		sound_channel_info_ptr snd_channel_info = input_->get_audio_codec_context() != nullptr ? 
				std::make_shared<sound_channel_info>
				(
					input_->get_audio_codec_context()->channels, 
					input_->get_audio_codec_context()->bits_per_coded_sample, 
					input_->get_audio_codec_context()->sample_rate
				) : nullptr;
		
		video_decoder_.reset(new video_decoder());
		video_scaler_.reset(new video_scaler());
		audio_decoder_.reset(new audio_decoder(snd_channel_info));
		has_audio_ = input_->get_audio_codec_context() != nullptr;
	}
		
	frame_ptr get_frame()
	{
		while(ouput_channel_.empty() && !input_->is_eof())
		{										
			tbb::parallel_invoke(
			[&]
			{ // Video Decoding and Scaling
				auto video_packet = input_->get_video_packet();
				if(video_packet)
				{
					video_packet = video_decoder_->execute(video_packet);
					auto frame = video_scaler_->execute(video_packet)->frame;
					video_frame_channel_.push_back(std::move(frame));	
				}
			}, 
			[&] 
			{ // Audio Decoding
				auto audio_packet = input_->get_audio_packet();
				if(audio_packet)
				{
					auto audio_chunks = audio_decoder_->execute(audio_packet);
					audio_chunk_channel_.insert(audio_chunk_channel_.end(), audio_packet->audio_chunks.begin(), audio_packet->audio_chunks.end());
				}
			});

			while(!video_frame_channel_.empty() && (!audio_chunk_channel_.empty() || !has_audio_))
			{
				if(has_audio_)
				{
					video_frame_channel_.front()->audio_data().push_back(audio_chunk_channel_.front());
					audio_chunk_channel_.pop_front();
				}
				
				frame_ptr frame = video_frame_channel_.front();
				video_frame_channel_.pop_front();
				ouput_channel_.push(std::move(frame));
			}				
		}

		frame_ptr frame;
		if(!ouput_channel_.empty())
		{
			frame = ouput_channel_.front();
			ouput_channel_.pop();
		}
		return frame;
	}
			
	bool has_audio_;

	// Filter 1 : Input
	input_uptr						input_;		

	// Filter 2 : Video Decoding and Scaling
	video_decoder_uptr				video_decoder_;
	video_scaler_uptr				video_scaler_;
	//std::deque<video_packet_ptr>					videoDecodedPacketChannel_;
	std::deque<frame_ptr>			video_frame_channel_;
	
	// Filter 3 : Audio Decoding
	audio_decoder_uptr				audio_decoder_;
	std::deque<audio_chunk_ptr>		audio_chunk_channel_;

	// Filter 4 : Merge Video and Audio
	std::queue<frame_ptr>			ouput_channel_;
	
	std::wstring					filename_;
};

frame_producer_ptr create_ffmpeg_producer(const  std::vector<std::wstring>& params, const frame_format_desc& format_desc)
{
	std::wstring filename = params[0];
	std::wstring result_filename = common::find_file(server::media_folder() + filename, list_of(L"mpg")(L"avi")(L"mov")(L"dv")(L"wav")(L"mp3")(L"mp4")(L"f4v")(L"flv"));
	if(result_filename.empty())
		return nullptr;

	return std::make_shared<ffmpeg_producer>(result_filename, params);
}

}}