#include "..\..\StdAfx.h"

#include "..\..\utils\image\Image.hpp"
#include <BlueVelvet.h>

#include "BluefishPlaybackStrategy.h"
#include "BluefishVideoConsumer.h"

namespace caspar {
namespace bluefish {

using namespace caspar::utils;

BluefishPlaybackStrategy::BluefishPlaybackStrategy(BlueFishVideoConsumer* pConsumer) : pConsumer_(pConsumer), currentReservedFrameIndex_(0) {
	FramePtr pFrame = pConsumer->pFrameManager_->CreateFrame();
	if(pFrame != 0 && pFrame->HasValidDataPtr())
		reservedFrames_.push_back(pFrame);
	else {
		throw std::exception("Failed to reserve temporary bluefishframe");
	}
	pFrame.reset();

	pFrame = pConsumer->pFrameManager_->CreateFrame();
	if(pFrame != 0 && pFrame->HasValidDataPtr())
		reservedFrames_.push_back(pFrame);
	else {
		throw std::exception("Failed to reserve temporary bluefishframe");
	}
}

BluefishPlaybackStrategy::~BluefishPlaybackStrategy()
{}

IVideoConsumer* BluefishPlaybackStrategy::GetConsumer()
{
	return pConsumer_;
}

FrameManagerPtr BluefishPlaybackStrategy::GetFrameManager()
{
	return pConsumer_->pFrameManager_;
}

FramePtr BluefishPlaybackStrategy::GetReservedFrame() {
	FramePtr pFrame = reservedFrames_[currentReservedFrameIndex_];
	currentReservedFrameIndex_ ^= 1;
	return pFrame;
}

void BluefishPlaybackStrategy::DisplayFrame(Frame* pFrame)
{
	if(pFrame->HasValidDataPtr()) {
		if(GetFrameManager()->Owns(*pFrame)) {
			DoRender(pFrame);
		}
		else {
			FramePtr pTempFrame = reservedFrames_[currentReservedFrameIndex_];
			if(pFrame->GetDataSize() == pTempFrame->GetDataSize()) {
				utils::image::Copy(pTempFrame->GetDataPtr(), pFrame->GetDataPtr(), pTempFrame->GetDataSize());
				DoRender(pTempFrame.get());
			}

			currentReservedFrameIndex_ ^= 1;
		}
	}
	else {
		LOG << TEXT("BLUEFISH: Tried to render frame with no data");
	}
}

void BluefishPlaybackStrategy::DoRender(Frame* pFrame) {
	static bool doLog = true;
	// video synch
	unsigned long fieldCount = 0;
	pConsumer_->pSDK_->wait_output_video_synch(UPD_FMT_FRAME, fieldCount);

	// Host->PCI in_Frame buffer to the card buffer
	pConsumer_->pSDK_->system_buffer_write_async(pFrame->GetDataPtr(), pFrame->GetDataSize(), 0, reinterpret_cast<long>(pFrame->GetMetadata()), 0);
	if(BLUE_FAIL(pConsumer_->pSDK_->render_buffer_update(reinterpret_cast<long>(pFrame->GetMetadata())))) {
		if(doLog) {
			LOG << TEXT("BLUEFISH: render_buffer_update failed");
			doLog = false;
		}
	}
	else
		doLog = true;
}

}	//namespace bluefish
}	//namespace caspar