#include "..\MediaProducer.h"
#include "frame.h"

#pragma once

namespace caspar {

class FrameMediaController;
class ITransitionController;

class ClipInfo
{
public:
	ClipInfo() : pFrameController_(0), pTransitionController_(0), lastFetchResult_(FetchWait), bStopped_(false)
	{}
	ClipInfo(MediaProducerPtr pFP, FrameMediaController* pController) : pFP_(pFP), pFrameController_(pController), pTransitionController_(0), lastFetchResult_(FetchWait), bStopped_(false)
	{}
	ClipInfo(MediaProducerPtr pFP, FrameMediaController* pController, ITransitionController* pTransitionController) : pFP_(pFP), pFrameController_(pController), pTransitionController_(pTransitionController), lastFetchResult_(FetchWait), bStopped_(false)
	{}

	ClipInfo(const ClipInfo& clipInfo) : pFP_(clipInfo.pFP_), pFrameController_(clipInfo.pFrameController_), pTransitionController_(clipInfo.pTransitionController_), pLastFrame_(clipInfo.pLastFrame_), lastFetchResult_(clipInfo.lastFetchResult_), bStopped_(clipInfo.bStopped_)
	{}

	ClipInfo& operator=(const ClipInfo& clipInfo) {
		pFP_ = clipInfo.pFP_;
		pFrameController_ = clipInfo.pFrameController_;
		pTransitionController_ = clipInfo.pTransitionController_;
		pLastFrame_ = clipInfo.pLastFrame_;
		lastFetchResult_ = clipInfo.lastFetchResult_;
		bStopped_ = clipInfo.bStopped_;

		return *this;
	}

	bool IsEmpty() {
		return (pFrameController_ == 0);
	}

	~ClipInfo() {
		Clear();
	}

	void Clear() {
		pFP_.reset();
		pFrameController_ = 0;
		pTransitionController_ = 0;
		pLastFrame_.reset();
		lastFetchResult_ = FetchWait;
		bStopped_ = false;
	}

	MediaProducerPtr		pFP_;
	FrameMediaController*	pFrameController_;
	ITransitionController*	pTransitionController_;
	FramePtr				pLastFrame_;
	FrameBufferFetchResult	lastFetchResult_;
	bool					bStopped_;
};

}	//namespace caspar