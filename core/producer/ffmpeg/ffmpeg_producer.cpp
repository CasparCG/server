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

#include "../../format/video_format.h"
#include "../../processor/draw_frame.h"
#include "../../processor/transform_Frame.h"
#include "../../processor/draw_frame.h"
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
	ffmpeg_producer(const std::wstring& filename, const  std::vector<std::wstring>& params) : filename_(filename), underrun_count_(0)
	{
		if(!boost::filesystem::exists(filename))
			BOOST_THROW_EXCEPTION(file_not_found() <<  boost::errinfo_file_name(common::narrow(filename)));
		
		static boost::once_flag av_register_all_flag = BOOST_ONCE_INIT;
		boost::call_once(av_register_all, av_register_all_flag);	
		
		static boost::once_flag avcodec_init_flag = BOOST_ONCE_INIT;
		boost::call_once(avcodec_init, avcodec_init_flag);	
				
		input_.reset(new input());
		input_->set_loop(std::find(params.begin(), params.end(), L"LOOP") != params.end());
		input_->load(common::narrow(filename_));
		video_decoder_.reset(new video_decoder(input_->get_video_codec_context().get()));
		video_transformer_.reset(new video_transformer(input_->get_video_codec_context().get()));
		audio_decoder_.reset(new audio_decoder(input_->get_audio_codec_context().get()));
		has_audio_ = input_->get_audio_codec_context() != nullptr;

		auto seek = std::find(params.begin(), params.end(), L"SEEK");
		if(seek != params.end() && ++seek != params.end())
		{
			if(!input_->seek(boost::lexical_cast<unsigned long long>(*seek)))
				CASPAR_LOG(warning) << "Failed to seek file: " << filename_  << "to frame" << *seek;
		}
	}
		
	void initialize(const frame_processor_device_ptr& frame_processor)
	{
		format_desc_ = frame_processor->get_video_format_desc();
		video_transformer_->initialize(frame_processor);
	}
		
	draw_frame receive()
	{
		while(ouput_channel_.empty() && !input_->is_eof())
		{	
			auto video_packet = input_->get_video_packet();		
			auto audio_packet = input_->get_audio_packet();		
			tbb::parallel_invoke(
			[&]
			{ // Video Decoding and Scaling
				if(!video_packet.empty())
				{
					auto decoded_frame = video_decoder_->execute(video_packet);
					auto frame = video_transformer_->execute(decoded_frame);
					video_frame_channel_.push_back(std::move(frame));	
				}
			}, 
			[&] 
			{ // Audio Decoding
				if(!audio_packet.empty())
				{
					auto chunks = audio_decoder_->execute(audio_packet);
					audio_chunk_channel_.insert(audio_chunk_channel_.end(), chunks.begin(), chunks.end());
				}
			});

			while(!video_frame_channel_.empty() && (!audio_chunk_channel_.empty() || !has_audio_))
			{
				std::vector<short> audio_data;
				if(has_audio_) 
				{
					audio_data = std::move(audio_chunk_channel_.front());
					audio_chunk_channel_.pop_front();
				}
							
				auto transform = transform_frame(std::move(video_frame_channel_.front()), std::move(audio_data));
				video_frame_channel_.pop_front();
		
				// TODO: Make generic for all formats and modes.
				if(input_->get_video_codec_context()->codec_id == CODEC_ID_DVVIDEO) // Move up one field		
					transform.translate(0.0f, 1.0/static_cast<double>(format_desc_.height));	
				
				ouput_channel_.push(std::move(transform));
			}				

			if(ouput_channel_.empty() && video_packet.empty() && audio_packet.empty())
			{
				if(underrun_count_++ == 0)
					CASPAR_LOG(warning) << "### File read underflow has STARTED.";

				return last_frame_;
			}
			else if(underrun_count_ > 0)
			{
				CASPAR_LOG(trace) << "### File Read Underrun has ENDED with " << underrun_count_ << " ticks.";
				underrun_count_ = 0;
			}
		}

		draw_frame result = last_frame_;
		if(!ouput_channel_.empty())
		{
			result = std::move(ouput_channel_.front());
			last_frame_ = transform_frame(result, std::vector<short>()); // last_frame should not have audio, override it!
			ouput_channel_.pop();
		}
		else if(input_->is_eof())
			last_frame_ = draw_frame::eof();

		return result;
	}

	std::wstring print() const
	{
		return L"ffmpeg_producer. filename " + filename_;
	}
			
	bool has_audio_;

	input_uptr							input_;		

	video_decoder_uptr					video_decoder_;
	video_transformer_uptr				video_transformer_;
	std::deque<draw_frame>				video_frame_channel_;
	
	audio_decoder_ptr					audio_decoder_;
	std::deque<std::vector<short>>		audio_chunk_channel_;

	std::queue<draw_frame>				ouput_channel_;
	
	std::wstring						filename_;

	long								underrun_count_;

	draw_frame							last_frame_;

	video_format_desc					format_desc_;
};

frame_producer_ptr create_ffmpeg_producer(const  std::vector<std::wstring>& params)
{	
	static const std::vector<std::wstring> extensions = list_of(L"mpg")(L"avi")(L"mov")(L"dv")(L"wav")(L"mp3")(L"mp4")(L"f4v")(L"flv");
	std::wstring filename = server::media_folder() + L"\\" + params[0];
	
	auto ext = std::find_if(extensions.begin(), extensions.end(), [&](const std::wstring& ex) -> bool
		{					
			return boost::filesystem::is_regular_file(boost::filesystem::wpath(filename).replace_extension(ex));
		});

	if(ext == extensions.end())
		return nullptr;

	return std::make_shared<ffmpeg_producer>(filename + L"." + *ext, params);
}

}}}