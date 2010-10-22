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
 
#include "..\..\StdAfx.h"

#include "..\..\utils\image\Image.hpp"
#include "..\..\audio\AudioManager.h"
#include "..\..\utils\Process.h"
#include "..\..\Application.h"
#include <BlueVelvet4.h>
#include <BlueHancUtils.h>

#include <vector>
#include <functional>

#include "BluefishPlaybackStrategy.h"
#include "BluefishVideoConsumer.h"

namespace caspar { namespace bluefish {

using namespace caspar::utils;

struct VirtualFreeMemRelease
{
	void operator()(LPVOID lpAddress)
	{
		if(lpAddress != nullptr)
			try{::VirtualFree(lpAddress, 0, MEM_RELEASE);}catch(...){}
	}
};

struct BlueFrame
{
public:
	BlueFrame(int dataSize, int bufferID) : dataSize_(dataSize), bufferID_(bufferID)
	{
		pData_.reset(static_cast<unsigned char*>(::VirtualAlloc(NULL, dataSize_, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE)));
		if(!pData_)	
			throw BluefishException("Failed to allocate memory for frame");	
		if(::VirtualLock(pData_.get(), dataSize_) == 0)	
			throw BluefishException("Failed to lock memory for frame");	
	}
			
	AudioDataChunkList& GetAudioData() {return audioData_;}
	unsigned char* GetDataPtr() const {return pData_.get();}
	int GetBufferID() const {return bufferID_;}
	unsigned int GetDataSize() const {return dataSize_;}
private:	
	std::unique_ptr<unsigned char, VirtualFreeMemRelease> pData_;
	int bufferID_;
	int dataSize_;
	AudioDataChunkList audioData_;
};
typedef std::shared_ptr<BlueFrame> BlueFramePtr;

struct BluefishPlaybackStrategy::Implementation
{	
	Implementation(BlueFishVideoConsumer* pConsumer) : pConsumer_(pConsumer), currentReservedFrameIndex_(0), log_(true), pSDK_(pConsumer->pSDK_)
	{
		auto optimalLength = BlueVelvetGolden(pConsumer_->vidFmt_, pConsumer_->memFmt_, pConsumer_->updFmt_);
		auto num_frames = 3;

		SIZE_T workingSetMinSize = 0, workingSetMaxSize = 0;
		if(utils::Process::GetCurrentProcess().GetWorkingSetSize(workingSetMinSize, workingSetMaxSize))
		{
			LOG << utils::LogLevel::Debug << TEXT("WorkingSet size: min = ") << workingSetMinSize << TEXT(", max = ") << workingSetMaxSize;
			
			workingSetMinSize += optimalLength * num_frames + MAX_HANC_BUFFER_SIZE;
			workingSetMaxSize += optimalLength * num_frames + MAX_HANC_BUFFER_SIZE;

			if(!utils::Process::GetCurrentProcess().SetWorkingSetSize(workingSetMinSize, workingSetMaxSize))		
				LOG << utils::LogLevel::Critical << TEXT("Failed to set workingset: min = ") << workingSetMinSize << TEXT(", max = ") << workingSetMaxSize;		
		}

		for(int n = 0; n < num_frames; ++n)
			reservedFrames_.push_back(std::make_shared<BlueFrame>(pConsumer_->pFrameManager_->GetFrameFormatDescription().size, n));
		
		audio_buffer_.reset(reinterpret_cast<BLUE_UINT32*>(::VirtualAlloc(NULL, MAX_HANC_BUFFER_SIZE, MEM_COMMIT, PAGE_READWRITE)));
		if(!audio_buffer_)
			throw BluefishException("Failed to allocate memory for audio buffer");	
		if(::VirtualLock(audio_buffer_.get(), MAX_HANC_BUFFER_SIZE) == 0)	
			throw BluefishException("Failed to lock memory for audio buffer");	
				
		if(GetApplication()->GetSetting(L"embedded-audio") == L"true")
			render_func_ = std::bind(&BluefishPlaybackStrategy::Implementation::DoRenderEmbAudio, this, std::placeholders::_1);	
		else
			render_func_ = std::bind(&BluefishPlaybackStrategy::Implementation::DoRender, this, std::placeholders::_1);	
	}
	
	FramePtr GetReservedFrame() 
	{
		return pConsumer_->pFrameManager_->CreateFrame();
	}

	FrameManagerPtr GetFrameManager()
	{
		return pConsumer_->pFrameManager_;
	}

	void DisplayFrame(Frame* pFrame)
	{
		if(!pFrame->HasValidDataPtr() || pFrame->GetDataSize() != pConsumer_->pFrameManager_->GetFrameFormatDescription().size)
		{			
			LOG << TEXT("BLUEFISH: Tried to render frame with no data or invalid data size");
			return;
		}
		
		auto pBlueFrame = reservedFrames_[currentReservedFrameIndex_];
		utils::image::Copy(pBlueFrame->GetDataPtr(), pFrame->GetDataPtr(), pBlueFrame->GetDataSize());
		pBlueFrame->GetAudioData() = pFrame->GetAudioData();
		
		currentReservedFrameIndex_ = (currentReservedFrameIndex_+1) % reservedFrames_.size();		
			
		render_func_(pBlueFrame);
	}
	
	void DoRender(const BlueFramePtr& pFrame) 
	{
		unsigned long fieldCount = 0;
		pSDK_->wait_output_video_synch(UPD_FMT_FRAME, fieldCount);
		
		pSDK_->system_buffer_write_async(pFrame->GetDataPtr(), pFrame->GetDataSize(), 0, pFrame->GetBufferID(), 0);
		if(BLUE_FAIL(pSDK_->render_buffer_update(pFrame->GetBufferID())))
		{
			if(log_) 
			{
				LOG << TEXT("BLUEFISH: render_buffer_update failed");
				log_ = false;
			}
		}
		else
			log_ = true;
	}
	
	static const int MAX_HANC_BUFFER_SIZE = 256*1024;
	void DoRenderEmbAudio(const BlueFramePtr& pFrame) 
	{
		unsigned long fieldCount = 0;
		pSDK_->wait_output_video_synch(UPD_FMT_FRAME, fieldCount);
		
		auto vid_fmt = pConsumer_->vidFmt_;
		auto sample_type = (AUDIO_CHANNEL_16BIT | AUDIO_CHANNEL_LITTLEENDIAN);
		auto card_type = pSDK_->has_video_cardtype();
		
		hanc_stream_info_struct hanc_stream_info;
		memset(&hanc_stream_info, 0, sizeof(hanc_stream_info));

		hanc_stream_info.AudioDBNArray[0] = -1;
		hanc_stream_info.AudioDBNArray[1] = -1;
		hanc_stream_info.AudioDBNArray[2] = -1;
		hanc_stream_info.AudioDBNArray[3] = -1;
		hanc_stream_info.hanc_data_ptr = reinterpret_cast<unsigned int*>(pFrame->GetDataPtr());
		hanc_stream_info.video_mode = vid_fmt;
		
		auto emb_audio_flag = (blue_emb_audio_enable | blue_emb_audio_group1_enable);
		
		// Mix sound
		auto audio_samples = 1920;
		auto audio_nchannels = 2;
		
		auto frame_audio_data = pFrame->GetAudioData();

		if(frame_audio_data.size() > 1)
		{
			std::vector<short> audio_data(audio_samples*audio_nchannels);
			for(int s = 0; s < audio_samples; ++s)
			{				
				for(int n = 0; n < frame_audio_data.size(); ++n)				
				{
					auto frame_audio = reinterpret_cast<short*>(frame_audio_data[n]->GetDataPtr());
					float volume = frame_audio_data[n]->GetVolume();
					audio_data[s*2+0] += static_cast<short>(static_cast<float>(frame_audio[s*2+0])*volume);					
					audio_data[s*2+1] += static_cast<short>(static_cast<float>(frame_audio[s*2+1])*volume);		
				}
			}
			memcpy(audio_buffer_.get(), audio_data.data(), audio_data.size()*audio_nchannels);
		}
		else if(frame_audio_data.size() == 1)
			memcpy(audio_buffer_.get(), frame_audio_data[0]->GetDataPtr(), frame_audio_data[0]->GetLength());
		else
			audio_nchannels = 0;
				
		// First write pixel data
		pSDK_->system_buffer_write_async(pFrame->GetDataPtr(), pFrame->GetDataSize(), 0, pFrame->GetBufferID(), 0);
		
		// Then encode and write hanc data
		if (card_type != CRD_BLUE_EPOCH_2K &&	
			card_type != CRD_BLUE_EPOCH_HORIZON && 
			card_type != CRD_BLUE_EPOCH_2K_CORE &&  
			card_type != CRD_BLUE_EPOCH_2K_ULTRA && 
			card_type != CRD_BLUE_EPOCH_CORE && 
			card_type != CRD_BLUE_EPOCH_ULTRA)
		{
			encode_hanc_frame(&hanc_stream_info,
							  audio_buffer_.get(),
							  audio_nchannels,
							  audio_samples,
							  sample_type,
							  emb_audio_flag);
		}
		else
		{
			encode_hanc_frame_ex(card_type,
								 &hanc_stream_info,
								 audio_buffer_.get(),
								 audio_nchannels,
								 audio_samples,
								 sample_type,
								 emb_audio_flag);
		}				
		
		pSDK_->system_buffer_write_async(reinterpret_cast<PBYTE>(hanc_stream_info.hanc_data_ptr),
										 MAX_HANC_BUFFER_SIZE, 
										 NULL,                 
										 BlueImage_HANC_DMABuffer(pFrame->GetBufferID(), BLUE_DATA_HANC));

		if(BLUE_FAIL(pSDK_->render_buffer_update(BlueBuffer_Image_HANC(pFrame->GetBufferID()))))
		{
			if(log_) 
			{
				LOG << TEXT("BLUEFISH: render_buffer_update failed");
				log_ = false;
			}
		}
		else
			log_ = true;
	}

	std::function<void(const BlueFramePtr&)> render_func_;
	BlueVelvetPtr pSDK_;

	bool log_;
	BlueFishVideoConsumer* pConsumer_;
	std::vector<BlueFramePtr> reservedFrames_;
	int currentReservedFrameIndex_;
	
	std::unique_ptr<BLUE_UINT32, VirtualFreeMemRelease> audio_buffer_;
};

BluefishPlaybackStrategy::BluefishPlaybackStrategy(BlueFishVideoConsumer* pConsumer) : pImpl_(new Implementation(pConsumer)){}
IVideoConsumer* BluefishPlaybackStrategy::GetConsumer(){return pImpl_->pConsumer_;}
FramePtr BluefishPlaybackStrategy::GetReservedFrame(){return pImpl_->GetReservedFrame();}
FrameManagerPtr BluefishPlaybackStrategy::GetFrameManager(){return pImpl_->GetFrameManager();}
void BluefishPlaybackStrategy::DisplayFrame(Frame* pFrame){return pImpl_->DisplayFrame(pFrame);}
}}