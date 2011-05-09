#include "../../stdafx.h"

#include "FFMPEGException.h"
#include "FFMPEGProducer.h"
#include "FFMPEGInput.h"
#include "FFMPEGFrameOutput.h"
#include "FFMPEGOutput.h"

#include "video/FFMPEGVideoDecoder.h"
#include "video/FFMPEGVideoScaler.h"
#include "video/FFMPEGVideoDeinterlacer.h"
#include "audio/FFMPEGAudioDecoder.h"

#include "../../Application.h"
#include "../../MediaProducerInfo.h"
#include "../../string_convert.h"
#include "../../frame/FrameManager.h"
#include "../../frame/FrameMediaController.h"
#include "../../audio/audiomanager.h"
#include "../../utils/Logger.h"
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
 
#include "../../utils/LogLevel.h"
#include "../../utils/UnhandledException.h"
#include "../../utils/scope_exit.h"
#include "../../utils/image/image.hpp"
#include "../../string_convert.h"

#include <tbb/task_group.h>
#include <tbb/pipeline.h>
#include <tbb/mutex.h>

#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>

#include <string>
#include <queue>
#include <functional>

namespace caspar{ namespace ffmpeg{

struct FFMPEGProducer::Implementation : public FrameMediaController, private utils::Noncopyable
{
public:
	Implementation(const tstring& filename, FFMPEGProducer* self) : self_(self)
	{
		if(filename.empty())
			throw std::exception("[FFMPEGProducer::FFMPEGProducer] Invalid Argument: filename");

		isRunning_ = false;

		output_.SetCapacity(GetApplication()->GetSetting<size_t>(TEXT("ffmpeg::buffersize"), FFMPEGProducer::DEFAULT_BUFFER_SIZE));
		
		input_.Load(utils::narrow(filename));
	}

	~Implementation() 
	{		
		output_.Stop();
		input_.Stop();

		if(pPipelineThread_ && !pPipelineThread_->timed_join(boost::posix_time::millisec(FFMPEGProducer::THREAD_TIMEOUT_MS)) && isRunning_)
			LOG << utils::LogLevel::Critical << TEXT("[FFMPEGProducer::~FFMPEGProducer] Thread timed-out!");
	}
		
	bool Initialize(FrameManagerPtr pFrameManager) 
	{			
		if(pFrameManager == nullptr)
			return false;
		
		input_.SetFactory(pFrameManager);
		
		if(!pPipelineThread_)
			pPipelineThread_.reset(new boost::thread([this]{ Run(); }));
		
		return true;
	}
		
	FrameBuffer& GetFrameBuffer()
	{
		return output_.GetFrameBuffer();
	}

	bool GetProducerInfo(MediaProducerInfo* pInfo) 
	{
		if(pInfo == nullptr)
			return false;
		
		pInfo->HaveVideo = input_.VideoCodecContext() != nullptr;
		pInfo->HaveAudio = input_.AudioCodecContext() != nullptr;
		if(pInfo->HaveAudio)
		{
			pInfo->AudioChannels = input_.AudioCodecContext()->channels;
			pInfo->AudioSamplesPerSec = input_.AudioCodecContext()->sample_rate;
			pInfo->BitsPerAudioSample = 16;
		}

		return true;
	}

	void Run()
	{			
		CASPAR_THREAD_GUARD(0, L"[FFMPEGProducer::Run]", [=]
		{
			CASPAR_SCOPE_EXIT([=]
			{
				isRunning_ = false;
			});

			class VideoDecodeAndScaleFilter : public tbb::filter
			{
			public:
				VideoDecodeAndScaleFilter() : tbb::filter(tbb::filter::serial_in_order)	{}
				void* operator()(void* pToken) { return videoScaler(videoDecoder(pToken)); }
			private:
				VideoPacketDecoderFilter			videoDecoder;
				VideoPacketScalerFilter				videoScaler;
			};

			VideoDecodeAndScaleFilter			videoDecoderAndScaler; // Decoder and scaler have to run in same thread
			VideoPacketDeinterlacerFilter		videoDeinterlacer;
			AudioPacketDecoderFilter			audioDecoder;
			FrameOutputFilter					frameOutput;	

			isRunning_ = true;

			tbb::pipeline pipeline;
						
			pipeline.add_filter(input_);		
			pipeline.add_filter(audioDecoder);					
			pipeline.add_filter(videoDecoderAndScaler);
			pipeline.add_filter(frameOutput);
			pipeline.add_filter(output_);
						
			output_.SetMaster(tbb::this_tbb_thread::get_id());
			pipeline.run(FFMPEGProducer::MAX_TOKENS);
			output_.Flush();

			pipeline.clear();
		});
		
	}

	void SetLoop(bool loop)
	{
		input_.SetLoop(loop);
	}	
		
	InputFilter							input_;	
	OutputFilter						output_;	
	std::unique_ptr<boost::thread>		pPipelineThread_;
	
	FFMPEGProducer*						self_;
	tbb::atomic<bool>					isRunning_;
};

FFMPEGProducer::FFMPEGProducer(const tstring& filename) :	pImpl_(new Implementation(filename, this))
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