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
 
#include "../../StdAfx.h"

#include "../../../common/image/image.h"
#include <BlueVelvet4.h>
#include <BlueHancUtils.h>

#include <vector>

#include "BluefishPlaybackStrategy.h"
#include "BluefishVideoConsumer.h"

namespace caspar { namespace bluefish {

using namespace caspar::common;

struct BluefishPlaybackStrategy::Implementation
{
	Implementation(BlueFishVideoConsumer* pConsumer) : pConsumer_(pConsumer), currentReservedFrameIndex_(0)
	{
		auto frame = pConsumer->pFrameManager_->CreateFrame();
		if(frame != 0)
			reservedFrames_.push_back(frame);
		else {
			throw std::exception("Failed to reserve temporary bluefishframe");
		}
		frame.reset();

		frame = pConsumer->pFrameManager_->CreateFrame();
		if(frame != 0)
			reservedFrames_.push_back(frame);
		else {
			throw std::exception("Failed to reserve temporary bluefishframe");
		}

		memset(&hancStreamInfo_, 0, sizeof(hancStreamInfo_));
	}
	std::shared_ptr<BluefishVideoFrame> GetReservedFrame() {
		std::shared_ptr<BluefishVideoFrame> frame = reservedFrames_[currentReservedFrameIndex_];
		currentReservedFrameIndex_ ^= 1;
		return frame;
	}

	void DisplayFrame(const frame_ptr& frame)
	{
		if(frame != nullptr) {
			if(pConsumer_->pFrameManager_.get() == reinterpret_cast<BluefishFrameManager*>(frame->tag())) {
				DoRender(std::static_pointer_cast<BluefishVideoFrame>(frame));
			}
			else {
				std::shared_ptr<BluefishVideoFrame> pTempFrame = reservedFrames_[currentReservedFrameIndex_];
				if(frame->size() == pTempFrame->size()) {
					common::image::copy(pTempFrame->data(), frame->data(), pTempFrame->size());
					DoRender(pTempFrame);
				}

				currentReservedFrameIndex_ ^= 1;
			}
		}
		else {
			CASPAR_LOG(error) << "BLUEFISH: Tried to render frame with no data";
		}
	}

	void DoRender(const std::shared_ptr<BluefishVideoFrame>& frame) {
		static bool doLog = true;
		static int frameID = 0;
		// video synch
		unsigned long fieldCount = 0;
		pConsumer_->pSDK_->wait_output_video_synch(UPD_FMT_FRAME, fieldCount);

		// Host->PCI in_Frame buffer to the card buffer
		pConsumer_->pSDK_->system_buffer_write_async(frame->data(), frame->size(), 0, frame->meta_data(), 0);
		if(BLUE_FAIL(pConsumer_->pSDK_->render_buffer_update(frame->meta_data()))) {
		/*pConsumer_->pSDK_->system_buffer_write_async(frame->data(), frame->size(), 0, frameID, 0);
		if(BLUE_FAIL(pConsumer_->pSDK_->render_buffer_update(frameID))) {*/
			if(doLog) {
				CASPAR_LOG(error) << "BLUEFISH: render_buffer_update failed";
				doLog = false;
			}
		}
		else
			doLog = true;

		frameID = (frameID+1) & 3;
	}

	BlueFishVideoConsumer* pConsumer_;
	std::vector<std::shared_ptr<BluefishVideoFrame>> reservedFrames_;
	int currentReservedFrameIndex_;

	hanc_stream_info_struct hancStreamInfo_;
};

BluefishPlaybackStrategy::BluefishPlaybackStrategy(BlueFishVideoConsumer* pConsumer) : pImpl_(new Implementation(pConsumer)) 
{ }

BluefishPlaybackStrategy::~BluefishPlaybackStrategy()
{ }
//
//FrameConsumer* BluefishPlaybackStrategy::GetConsumer()
//{
//	return pImpl_->pConsumer_;
//}
//
//frame_ptr BluefishPlaybackStrategy::GetReservedFrame() {
//	return pImpl_->GetReservedFrame();
//}

void BluefishPlaybackStrategy::display(const frame_ptr& frame) 
{
	return pImpl_->DisplayFrame(frame);
}

}	//namespace bluefish
}	//namespace caspar