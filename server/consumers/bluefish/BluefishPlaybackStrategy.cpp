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

#include "..\..\frame\SystemFrameManager.h"
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
#include <numeric>

#include "BluefishPlaybackStrategy.h"
#include "BluefishVideoConsumer.h"

namespace caspar { namespace bluefish {

using namespace caspar::utils;

struct BluefishPlaybackStrategy::Implementation
{	
	Implementation(BlueFishVideoConsumer* pConsumer, unsigned int memFmt, unsigned int updFmt, unsigned vidFmt) 
		: pConsumer_(pConsumer), currentReservedFrameIndex_(0), log_(true), pSDK_(pConsumer->pSDK_), vidFmt_(vidFmt)
	{
		const size_t FRAME_POOL_SIZE = 3;
		const auto fmtDesc = FrameFormatDescription::FormatDescriptions[pConsumer->currentFormat_];

		auto golden = BlueVelvetGolden(vidFmt, memFmt, updFmt); // 5 196 248
		page_locked_buffer::reserve_working_size((golden + MAX_HANC_BUFFER_SIZE) * FRAME_POOL_SIZE + MAX_HANC_BUFFER_SIZE);
		
		for(size_t n = 0; n < FRAME_POOL_SIZE; ++n)
			reservedFrames_.push_back(std::make_shared<blue_dma_buffer>(fmtDesc.size, n));
		
		audio_buffer_ = std::make_shared<page_locked_buffer>(MAX_HANC_BUFFER_SIZE);
						
		if(GetApplication()->GetSetting(L"embedded-audio") == L"true")
			render_func_ = std::bind(&BluefishPlaybackStrategy::Implementation::DoRenderEmbAudio, this, std::placeholders::_1, std::placeholders::_2);	
		else
			render_func_ = std::bind(&BluefishPlaybackStrategy::Implementation::DoRender, this, std::placeholders::_1, std::placeholders::_2);	

		pFrameManager_ = std::make_shared<SystemFrameManager>(fmtDesc);
	}
	
	FramePtr GetReservedFrame() 
	{
		return pFrameManager_->CreateFrame();
	}

	FrameManagerPtr GetFrameManager()
	{
		return pFrameManager_;
	}

	void DisplayFrame(Frame* pFrame)
	{
		if(!pFrame->HasValidDataPtr() || pFrame->GetDataSize() != pFrameManager_->GetFrameFormatDescription().size)
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
		
		MixAudio(reinterpret_cast<BLUE_UINT16*>(audio_buffer_->data()), frame_audio_data, audio_samples, audio_nchannels);		
		EncodeHANC(reinterpret_cast<BLUE_UINT32*>(buffer->hanc_data()), audio_buffer_->data(), audio_samples, audio_nchannels);

		pSDK_->system_buffer_write_async(buffer->image_data(), 
										 buffer->image_size(), 
										 nullptr, 
										 BlueImage_HANC_DMABuffer(buffer->id(), BLUE_DATA_IMAGE));

		pSDK_->system_buffer_write_async(buffer->hanc_data(),
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

	void EncodeHANC(BLUE_UINT32* hanc_data, void* audio_data, size_t audio_samples, size_t audio_nchannels)
	{	
		auto card_type = pSDK_->has_video_cardtype();
		auto vid_fmt = vidFmt_;
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

	void MixAudio(BLUE_UINT16* dest, const AudioDataChunkList& frame_audio_data, size_t audio_samples, size_t audio_nchannels)
	{		
		size_t size = audio_samples*audio_nchannels;
		memset(dest, 0, size*2);
		std::for_each(frame_audio_data.begin(), frame_audio_data.end(), [&](const audio::AudioDataChunkPtr& chunk)
		{
			BLUE_UINT16* src = reinterpret_cast<BLUE_UINT16*>(chunk->GetDataPtr());
			for(size_t n = 0; n < size; ++n)
				dest[n] = static_cast<BLUE_UINT16>(static_cast<BLUE_UINT32>(dest[n])+static_cast<BLUE_UINT32>(src[n]));
		});
	}

	std::function<void(const blue_dma_buffer_ptr&, const AudioDataChunkList&)> render_func_;
	BlueVelvetPtr pSDK_;

	bool log_;
	BlueFishVideoConsumer* pConsumer_;
	std::vector<blue_dma_buffer_ptr> reservedFrames_;
	int currentReservedFrameIndex_;
	
	page_locked_buffer_ptr audio_buffer_;

	SystemFrameManagerPtr pFrameManager_;
	unsigned int vidFmt_;
};

BluefishPlaybackStrategy::BluefishPlaybackStrategy(BlueFishVideoConsumer* pConsumer, unsigned int memFmt, unsigned int updFmt, unsigned vidFmt) 
	: pImpl_(new Implementation(pConsumer, memFmt, updFmt, vidFmt)){}
IVideoConsumer* BluefishPlaybackStrategy::GetConsumer(){return pImpl_->pConsumer_;}
FramePtr BluefishPlaybackStrategy::GetReservedFrame(){return pImpl_->GetReservedFrame();}
FrameManagerPtr BluefishPlaybackStrategy::GetFrameManager(){return pImpl_->GetFrameManager();}
void BluefishPlaybackStrategy::DisplayFrame(Frame* pFrame){return pImpl_->DisplayFrame(pFrame);}
}}