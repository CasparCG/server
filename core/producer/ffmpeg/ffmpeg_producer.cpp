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
#include "video/video_transformer.h"

#include "../../video/video_format.h"
#include "../../../common/utility/find_file.h"
#include "../../../common/utility/memory.h"
#include "../../../common/utility/scope_exit.h"
#include "../../server.h"

#include <tbb/mutex.h>
#include <tbb/parallel_invoke.h>
#include <tbb/task_group.h>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <boost/thread/once.hpp>

using namespace boost::assign;

namespace caspar { namespace core { namespace ffmpeg{
	
struct ffmpeg_producer : public frame_producer
{
public:
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
		video_decoder_.reset(new video_decoder());
		video_transformer_.reset(new video_transformer());
		audio_decoder_.reset(new audio_decoder());
		has_audio_ = input_->get_audio_codec_context() != nullptr;

		auto seek = std::find(params.begin(), params.end(), L"SEEK");
		if(seek != params.end() && ++seek != params.end())
		{
			if(!input_->seek(boost::lexical_cast<unsigned long long>(*seek)))
				CASPAR_LOG(warning) << "Failed to seek file: " << filename_  << "to frame" << *seek;
		}

		input_->start();
	}
		
	void initialize(const frame_processor_device_ptr& frame_processor)
	{
		video_transformer_->initialize(frame_processor);
	}
		
	frame_ptr render_frame()
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
					auto frame = video_transformer_->execute(video_packet)->frame;
					video_frame_channel_.push_back(std::move(frame));	
				}
			}, 
			[&] 
			{ // Audio Decoding
				auto audio_packet = input_->get_audio_packet();
				if(audio_packet)
				{
					audio_decoder_->execute(audio_packet);
					for(size_t n = 0; n < audio_packet->audio_chunks.size(); ++n)
						audio_chunk_channel_.push_back(std::move(audio_packet->audio_chunks[n]));
				}
			});

			while(!video_frame_channel_.empty() && (!audio_chunk_channel_.empty() || !has_audio_))
			{
				if(has_audio_)
				{
					video_frame_channel_.front()->audio_data() = std::move(audio_chunk_channel_.front());
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
	input_uptr							input_;		

	// Filter 2 : Video Decoding and Scaling
	video_decoder_uptr					video_decoder_;
	video_transformer_uptr				video_transformer_;
	//std::deque<video_packet_ptr>		videoDecodedPacketChannel_;
	std::deque<frame_ptr>			video_frame_channel_;
	
	// Filter 3 : Audio Decoding
	audio_decoder_uptr					audio_decoder_;
	std::deque<std::vector<short>>		audio_chunk_channel_;

	// Filter 4 : Merge Video and Audio
	std::queue<frame_ptr>			ouput_channel_;
	
	std::wstring						filename_;
};

frame_producer_ptr create_ffmpeg_producer(const  std::vector<std::wstring>& params)
{
	std::wstring filename = params[0];
	std::wstring result_filename = common::find_file(server::media_folder() + filename, list_of(L"mpg")(L"avi")(L"mov")(L"dv")(L"wav")(L"mp3")(L"mp4")(L"f4v")(L"flv"));
	if(result_filename.empty())
		return nullptr;

	return std::make_shared<ffmpeg_producer>(result_filename, params);
}

}}}