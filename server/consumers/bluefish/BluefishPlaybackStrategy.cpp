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
#include <BlueVelvet4.h>
#include <BlueHancUtils.h>

#include <vector>

#include "BluefishPlaybackStrategy.h"
#include "BluefishVideoConsumer.h"

namespace caspar {
namespace bluefish {

using namespace caspar::utils;

struct BluefishPlaybackStrategy::Implementation
{
	Implementation(BlueFishVideoConsumer* pConsumer) : pConsumer_(pConsumer), currentReservedFrameIndex_(0)
	{
		//FramePtr pFrame = pConsumer->pFrameManager_->CreateFrame();
		//if(pFrame != 0 && pFrame->HasValidDataPtr())
		//	reservedFrames_.push_back(pFrame);
		//else {
		//	throw std::exception("Failed to reserve temporary bluefishframe");
		//}
		//pFrame.reset();

		//pFrame = pConsumer->pFrameManager_->CreateFrame();
		//if(pFrame != 0 && pFrame->HasValidDataPtr())
		//	reservedFrames_.push_back(pFrame);
		//else {
		//	throw std::exception("Failed to reserve temporary bluefishframe");
		//}

		for(int n = 0; n < 3; ++n)
			reservedFrames_.push_back(pConsumer_->pFrameManager_->CreateFrame());
		
		const FrameFormatDescription& fmtDesc = FrameFormatDescription::FormatDescriptions[pConsumer->currentFormat_];
		frameManager_ = std::make_shared<SystemFrameManager>(fmtDesc);

		memset(&hancStreamInfo_, 0, sizeof(hancStreamInfo_));
	}
	FramePtr GetReservedFrame() {
		//FramePtr pFrame = reservedFrames_[currentReservedFrameIndex_];
		//currentReservedFrameIndex_ ^= 1;
		return frameManager_->CreateFrame();
	}
	FrameManagerPtr GetFrameManager()
	{
		return frameManager_;
	}
	void DisplayFrame(Frame* pFrame)
	{
		FramePtr reservedFrame = reservedFrames_[currentReservedFrameIndex_];
		currentReservedFrameIndex_ = (currentReservedFrameIndex_ + 1) % reservedFrames_.size();

		utils::image::Copy(reservedFrame->GetDataPtr(), pFrame->GetDataPtr(), pFrame->GetDataSize());
		DoRender(reservedFrame.get());

		//if(pFrame->HasValidDataPtr()) {
		//	if(GetFrameManager()->Owns(*pFrame)) {
		//		DoRender(pFrame);
		//	}
		//	else {
		//		FramePtr pTempFrame = reservedFrames_[currentReservedFrameIndex_];
		//		if(pFrame->GetDataSize() == pTempFrame->GetDataSize()) {
		//			utils::image::Copy(pTempFrame->GetDataPtr(), pFrame->GetDataPtr(), pTempFrame->GetDataSize());
		//			DoRender(pTempFrame.get());
		//		}

		//		currentReservedFrameIndex_ ^= 1;
		//	}
		//}
		//else {
		//	LOG << TEXT("BLUEFISH: Tried to render frame with no data");
		//}
	}

	void DoRender(Frame* pFrame) {
		static bool doLog = true;
		static int frameID = 0;
		// video synch
		unsigned long fieldCount = 0;
		pConsumer_->pSDK_->wait_output_video_synch(UPD_FMT_FRAME, fieldCount);

		// Host->PCI in_Frame buffer to the card buffer
		pConsumer_->pSDK_->system_buffer_write_async(pFrame->GetDataPtr(), pFrame->GetDataSize(), 0, reinterpret_cast<long>(pFrame->GetMetadata()), 0);
		if(BLUE_FAIL(pConsumer_->pSDK_->render_buffer_update(reinterpret_cast<long>(pFrame->GetMetadata())))) {
		/*pConsumer_->pSDK_->system_buffer_write_async(pFrame->GetDataPtr(), pFrame->GetDataSize(), 0, frameID, 0);
		if(BLUE_FAIL(pConsumer_->pSDK_->render_buffer_update(frameID))) {*/
			if(doLog) {
				LOG << TEXT("BLUEFISH: render_buffer_update failed");
				doLog = false;
			}
		}
		else
			doLog = true;

		frameID = (frameID+1) & 3;
	}

	BlueFishVideoConsumer* pConsumer_;
	
	std::vector<FramePtr> reservedFrames_;
	int currentReservedFrameIndex_;

	hanc_stream_info_struct hancStreamInfo_;

	SystemFrameManagerPtr frameManager_;
};

BluefishPlaybackStrategy::BluefishPlaybackStrategy(BlueFishVideoConsumer* pConsumer) : pImpl_(new Implementation(pConsumer)) 
{ }

BluefishPlaybackStrategy::~BluefishPlaybackStrategy()
{ }

IVideoConsumer* BluefishPlaybackStrategy::GetConsumer()
{
	return pImpl_->pConsumer_;
}

FramePtr BluefishPlaybackStrategy::GetReservedFrame() {
	return pImpl_->GetReservedFrame();
}

FrameManagerPtr BluefishPlaybackStrategy::GetFrameManager()
{
	return pImpl_->GetFrameManager();
}

void BluefishPlaybackStrategy::DisplayFrame(Frame* pFrame) 
{
	return pImpl_->DisplayFrame(pFrame);
}

}	//namespace bluefish
}	//namespace caspar