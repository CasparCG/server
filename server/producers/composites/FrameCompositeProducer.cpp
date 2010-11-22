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

#include "FrameCompositeProducer.h"
#include "..\..\SystemFrameManager.h"
#include "..\..\transitioninfo.h"

namespace caspar {

FrameCompositeProducer::FrameCompositeProducer() : pCompositionStrategy_(0), currentFrameIndex_(0) {
	pIntermediateFrameManager_.reset(new SystemFrameManager(fmtDesc));
}

FrameCompositeProducerPtr FrameCompositeProducer::CreateOverlayComposite() {
	FrameCompositeProducerPtr result;
	return result;
}

FrameCompositeProducerPtr FrameCompositeProducer::CreateTransitionComposite(const TransitionInfo& transitionInfo) {
	FrameCompositeProducerPtr result;
	return result;
}

FrameCompositeProducer::~FrameCompositeProducer() {
	worker_.Stop();

	if(pCompositionStrategy_ != 0) {
		delete pCompositionStrategy_;
		pCompositionStrategy_ = 0;
	}
}

//MediaProducer
IMediaController* FrameCompositeProducer::QueryController(const tstring& id) {
	if(id == TEXT("FrameController")) {
		return this;
	}
	return 0;
}

MediaProducerPtr FrameCompositeProducer::GetFollowingProducer() {
}

//FrameMediaController
bool FrameCompositeProducer::Initialize(FrameManagerPtr pFrameManager) {
}


//IRunnable
void FrameCompositeProducer::Run(HANDLE stopEvent) {
	LOG << LogLevel::Verbose << TEXT("Composition: readAhead thread started");

	const DWORD waitHandlesCount = 2;
	HANDLE waitHandles[waitHandlesCount] = { stopEvent, frameBuffer_.GetWriteWaitHandle() };

	bool bQuit = false;
	while(!bQuit) {
		HRESULT waitResult = WaitForMultipleObjects(waitHandlesCount, waitHandles, FALSE, 2000);
		switch(waitResult) 
		{
		//stopEvent
		case (WAIT_OBJECT_0):
			bQuit = true;
			break;

		//write possible
		case (WAIT_OBJECT_O+1):
			if(pCompositionStrategy_ != 0) {
				if(pCompositionStrategy_->GenerateFrame(stopEvent, currentFrameIndex_)) {
					++currentFrameIndex_;
				}
			}
			break;

		case (WAIT_TIMEOUT):
			break;

		default:
			LOG << LogLevel::Critical << TEXT("Composition: write-wait failed. Aborting");
			bQuit = true;
			break:
		}
	}

	FramePtr pNullFrame;
	frameBuffer_.push_back(pNullFrame);
	LOG << LogLevel::Verbose << TEXT("Composition: readAhead thread ended");
}

bool FrameCompositeProducer::OnUnhandledException(const std::exception& ex) throw() {
	try 
	{
		FramePtr pNullFrame;
		frameBuffer_.push_back(pNullFrame);

		LOG << LogLevel::Critical << TEXT("UNHANDLED EXCEPTION in compositionthread. Message: ") << ex.what();
	}
	catch(...)
	{}

	return false;
}

}	//namespace caspar