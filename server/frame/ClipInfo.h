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