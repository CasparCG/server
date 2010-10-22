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
#include "BluefishUtil.h"
#include "BluefishMemory.h"
#include <BlueVelvet4.h>
#include <BlueHancUtils.h>

#include <vector>
#include <functional>

#include "BluefishPlaybackStrategy.h"
#include "BluefishVideoConsumer.h"

namespace caspar { namespace bluefish {

using namespace caspar::utils;

struct BluefishPlaybackStrategy::Implementation
{	
	Implementation(BlueFishVideoConsumer* pConsumer) : pConsumer_(pConsumer), currentReservedFrameIndex_(0), log_(true), pSDK_(pConsumer->pSDK_)
	{
		auto golden = BlueVelvetGolden(pConsumer_->vidFmt_, pConsumer_->memFmt_, pConsumer_->updFmt_); // 5 196 248
		auto num_frames = 3;

		page_locked_buffer::reserve_working_size((golden + MAX_HANC_BUFFER_SIZE) * num_frames + MAX_HANC_BUFFER_SIZE);
		
		for(int n = 0; n < num_frames; ++n)
			reservedFrames_.push_back(std::make_shared<blue_dma_buffer>(pConsumer_->pFrameManager_->GetFrameFormatDescription().size, n));
		
		audio_buffer_ = std::make_shared<page_locked_buffer>(MAX_HANC_BUFFER_SIZE);
						
		if(GetApplication()->GetSetting(L"embedded-audio") == L"true")
			render_func_ = std::bind(&BluefishPlaybackStrategy::Implementation::DoRenderEmbAudio, this, std::placeholders::_1, std::placeholders::_2);	
		else
			render_func_ = std::bind(&BluefishPlaybackStrategy::Implementation::DoRender, this, std::placeholders::_1, std::placeholders::_2);	
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
		
		auto buffer = reservedFrames_[currentReservedFrameIndex_];
		utils::image::Copy(buffer->image_data(), pFrame->GetDataPtr(), buffer->image_size());
		
		currentReservedFrameIndex_ = (currentReservedFrameIndex_+1) % reservedFrames_.size();		
			
		render_func_(buffer, pFrame->GetAudioData());
	}
	
	void DoRender(const blue_dma_buffer_ptr& buffer, const AudioDataChunkList& frame_audio_data) 
	{
		unsigned long fieldCount = 0;
		pSDK_->wait_output_video_synch(UPD_FMT_FRAME, fieldCount);
		
		pSDK_->system_buffer_write_async(buffer->image_data(), buffer->image_size(), 0, buffer->id(), 0);
		if(BLUE_FAIL(pSDK_->render_buffer_update(buffer->id())))
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
	
	void DoRenderEmbAudio(const blue_dma_buffer_ptr& buffer, const AudioDataChunkList& frame_audio_data) 
	{
		unsigned long fieldCount = 0;
		pSDK_->wait_output_video_synch(UPD_FMT_FRAME, fieldCount);
				
		static size_t audio_samples = 1920;
		static size_t audio_nchannels = 2;
		
		MixAudio(audio_buffer_->data(), frame_audio_data, audio_samples, audio_nchannels);		
		EncodeHANC(buffer->hanc_data(), audio_buffer_->data(), audio_samples, audio_nchannels);

		pSDK_->system_buffer_write_async(buffer->image_data(), 
										 buffer->image_size(), 
										 nullptr, 
										 BlueImage_HANC_DMABuffer(buffer->id(), BLUE_DATA_IMAGE));

		pSDK_->system_buffer_write_async(reinterpret_cast<PBYTE>(buffer->hanc_data()),
										 buffer->hanc_size(), 
										 nullptr,                 
										 BlueImage_HANC_DMABuffer(buffer->id(), BLUE_DATA_HANC));

		if(BLUE_FAIL(pSDK_->render_buffer_update(BlueBuffer_Image_HANC(buffer->id()))))
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

	void EncodeHANC(unsigned int* hanc_data, void* audio_data, size_t audio_samples, size_t audio_nchannels)
	{	
		auto card_type = pSDK_->has_video_cardtype();
		auto vid_fmt = pConsumer_->vidFmt_;
		auto sample_type = (AUDIO_CHANNEL_16BIT | AUDIO_CHANNEL_LITTLEENDIAN);
		
		hanc_stream_info_struct hanc_stream_info;
		memset(&hanc_stream_info, 0, sizeof(hanc_stream_info));

		hanc_stream_info.AudioDBNArray[0] = -1;
		hanc_stream_info.AudioDBNArray[1] = -1;
		hanc_stream_info.AudioDBNArray[2] = -1;
		hanc_stream_info.AudioDBNArray[3] = -1;
		hanc_stream_info.hanc_data_ptr = hanc_data;
		hanc_stream_info.video_mode = vid_fmt;
		
		auto emb_audio_flag = (blue_emb_audio_enable | blue_emb_audio_group1_enable);

		if (!is_epoch_card(card_type))
		{
			encode_hanc_frame(&hanc_stream_info,
							  audio_data,
							  audio_nchannels,
							  audio_samples,
							  sample_type,
							  emb_audio_flag);
		}
		else
		{
			encode_hanc_frame_ex(card_type,
								 &hanc_stream_info,
								 audio_data,
								 audio_nchannels,
								 audio_samples,
								 sample_type,
								 emb_audio_flag);
		}						
	}

	void MixAudio(void* dest, const AudioDataChunkList& frame_audio_data, size_t audio_samples, size_t audio_nchannels)
	{		
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
			memcpy(dest, audio_data.data(), audio_data.size()*audio_nchannels);
		}
		else if(frame_audio_data.size() == 1)
			memcpy(dest, frame_audio_data[0]->GetDataPtr(), frame_audio_data[0]->GetLength());
		else
			memset(dest, 0, audio_samples*audio_nchannels*2);
	}

	std::function<void(const blue_dma_buffer_ptr&, const AudioDataChunkList&)> render_func_;
	BlueVelvetPtr pSDK_;

	bool log_;
	BlueFishVideoConsumer* pConsumer_;
	std::vector<blue_dma_buffer_ptr> reservedFrames_;
	int currentReservedFrameIndex_;
	
	page_locked_buffer_ptr audio_buffer_;
};

BluefishPlaybackStrategy::BluefishPlaybackStrategy(BlueFishVideoConsumer* pConsumer) : pImpl_(new Implementation(pConsumer)){}
IVideoConsumer* BluefishPlaybackStrategy::GetConsumer(){return pImpl_->pConsumer_;}
FramePtr BluefishPlaybackStrategy::GetReservedFrame(){return pImpl_->GetReservedFrame();}
FrameManagerPtr BluefishPlaybackStrategy::GetFrameManager(){return pImpl_->GetFrameManager();}
void BluefishPlaybackStrategy::DisplayFrame(Frame* pFrame){return pImpl_->DisplayFrame(pFrame);}
}}