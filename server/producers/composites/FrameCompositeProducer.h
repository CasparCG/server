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
 
#pragma once

#include "..\..\MediaProducer.h"
#include "..\..\FrameManager.h"
#include "..\..\FrameMediaController.h"
#include "..\..\SystemFrameManager.h"
#include "..\..\utils\thread.h"

namespace caspar {

class TransitionInfo;

class IFrameCompositionStrategy
{
public:
	bool GenerateFrame(HANDLE stopEvent, unsigned int frameIndex) = 0;
};

class FrameCompositeProducer : public MediaProducer, public FrameMediaController, public utils::IRunnable
{
	FrameCompositeProducer();

public:
	static FrameCompositeProducerPtr CreateOverlayComposite();
	static FrameCompositeProducerPtr CreateTransitionComposite(const TransitionInfo& transitionInfo);
	virtual ~FrameCompositeProducer();

	//MediaProducer
	virtual IMediaController* QueryController(const tstring& id);
	virtual MediaProducerPtr GetFollowingProducer();

	//FrameMediaController
	virtual bool Initialize(FrameManagerPtr pFrameManager);
	virtual FrameBuffer& GetFrameBuffer() {
		return frameBuffer_;
	}

	//IRunnable
	virtual void Run(HANDLE stopEvent);
	virtual bool OnUnhandledException(const std::exception& ex) throw();

private:
	IFrameCompositionStrategy* pCompositionStrategy_;
	unsigned int currentFrameIndex_;

	FrameManagerPtr			pResultFrameManager_;
	SystemFrameManagerPtr	pIntermediateFrameManager_;

	utils::Thread worker_;
	MotionFrameBuffer frameBuffer_;
};

}	//namespace caspar