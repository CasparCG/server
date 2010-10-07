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

#include "../../Application.h"
#include "../../MediaProducerInfo.h"
#include "../../string_convert.h"
#include "../../frame/buffers/MotionFrameBuffer.h"
#include "../../frame/FrameManager.h"
#include "../../frame/FrameMediaController.h"
#include "../../audio/audiomanager.h"
#include "../../utils/Logger.h"

#include <tbb/mutex.h>
#include <tbb/parallel_invoke.h>
#include <tbb/task_group.h>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <boost/thread/once.hpp>
#include <boost/filesystem.hpp>
#include <boost/assign.hpp>

using namespace boost::assign;

namespace caspar{ namespace ffmpeg{
	
struct FFMPEGProducer::Implementation : public FrameMediaController, boost::noncopyable
{
public:
	Implementation(const std::wstring& filename) 
		: filename_(filename)
	{
    	if(!boost::filesystem::exists(filename))
    		throw std::runtime_error("File not found");

		frameBuffer_.SetCapacity(4);
	}
	
	~Implementation()
	{
		frameBuffer_.clear();
		input_->stop();
		thread_.join();
	}
		
	FramePtr get_frame()
	{
		while(ouput_channel_.empty() && !input_->is_eof())
		{										
			tbb::parallel_invoke(
			[&]
			{ // Video Decoding and Scaling
				auto video_packet = input_->get_video_packet();
				if(video_packet)
				{
					video_packet = video_decoder_.execute(video_packet);
					auto frame = video_transformer_.execute(video_packet)->frame;
					video_frame_channel_.push_back(std::move(frame));	
				}
			}, 
			[&] 
			{ // Audio Decoding
				auto audio_packet = input_->get_audio_packet();
				if(audio_packet)
				{
					auto audio_chunks = audio_decoder_.execute(audio_packet);
					audio_chunk_channel_.insert(audio_chunk_channel_.end(), audio_packet->audio_chunks.begin(), audio_packet->audio_chunks.end());
				}
			});

			while(!video_frame_channel_.empty() && (!audio_chunk_channel_.empty() || !has_audio_))
			{
				if(has_audio_)
				{
					video_frame_channel_.front()->GetAudioData().push_back(audio_chunk_channel_.front());
					audio_chunk_channel_.pop_front();
				}
				
				FramePtr frame = video_frame_channel_.front();
				video_frame_channel_.pop_front();
				ouput_channel_.push(std::move(frame));
			}				
		}

		FramePtr frame;
		if(!ouput_channel_.empty())
		{
			frame = ouput_channel_.front();
			ouput_channel_.pop();
		}
		return frame;
	}
	
	bool Initialize(FrameManagerPtr pFrameManager) 
	{		
		try
		{
			video_transformer_.set_factory(pFrameManager);
			input_.reset(new input());
			input_->load(utils::narrow(filename_));
			input_->set_loop(loop_);
			has_audio_ = input_->get_audio_codec_context() != nullptr;
			thread_ = boost::thread([=]{run();});
		}
		catch(std::exception& ex)
		{
			LOG << "FFMPEGProducer::Initialize Exception what: " << ex.what();
			return false;
		}
		catch(...)
		{
			return false;
		}

		return true;
	}

	void SetLoop(bool value)
	{
		loop_ = value;
		if(input_)
			input_->set_loop(value);
	}
	
	FrameBuffer& GetFrameBuffer()
	{
		return frameBuffer_;
	}

	bool GetProducerInfo(MediaProducerInfo* pInfo) 
	{
		if(pInfo == nullptr)
			return false;
		
		pInfo->HaveVideo = input_->get_video_codec_context() != nullptr;
		pInfo->HaveAudio = input_->get_audio_codec_context() != nullptr;
		if(pInfo->HaveAudio)
		{
			pInfo->AudioChannels = input_->get_audio_codec_context()->channels;
			pInfo->AudioSamplesPerSec = input_->get_audio_codec_context()->sample_rate;
			pInfo->BitsPerAudioSample = 16;
		}

		return true;
	}

	void run()
	{
		LOG << "Started FFMPEGProducer thread";

		try
		{
			while(!input_->is_eof())			
				frameBuffer_.push_back(get_frame());			
		}
		catch(std::exception& ex)
		{
			LOG << "Exception in FFMPEGProducer thread what:" << ex.what();
		}
		catch(...)
		{
			LOG << "Exception in FFMPEGProducer thread";
		}
		
		LOG << "Ended FFMPEGProducer thread";
	}

	boost::thread thread_;
			
	bool loop_;
	bool has_audio_;

	// Filter 1 : Input
	input_uptr						input_;		

	// Filter 2 : Video Decoding and Scaling
	video_decoder					video_decoder_;
	video_transformer				video_transformer_;
	std::deque<FramePtr>			video_frame_channel_;
	
	// Filter 3 : Audio Decoding
	audio_decoder							audio_decoder_;
	std::deque<audio::AudioDataChunkPtr>	audio_chunk_channel_;

	// Filter 4 : Merge Video and Audio
	std::queue<FramePtr>			ouput_channel_;
	
	std::wstring					filename_;

	MotionFrameBuffer				frameBuffer_;
};

FFMPEGProducer::FFMPEGProducer(const tstring& filename) : pImpl_(new Implementation(filename))
{
}

IMediaController* FFMPEGProducer::QueryController(const tstring& id)
{
	return id == TEXT("FrameController") ? pImpl_.get() : nullptr;
}

bool FFMPEGProducer::GetProducerInfo(MediaProducerInfo* pInfo)
{
	return pImpl_->GetProducerInfo(pInfo);
}

void FFMPEGProducer::SetLoop(bool loop)
{
	pImpl_->SetLoop(loop);
}

}}