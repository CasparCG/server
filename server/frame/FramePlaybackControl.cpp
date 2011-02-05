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
 
#include "..\stdafx.h"
#include "..\VideoConsumer.h"
#include "FrameMediaController.h"
#include "FramePlaybackStrategy.h"
#include "FramePlaybackControl.h"
#include "..\cg\flashcgproxy.h"
#include "..\producers\composites\TransitionProducer.h"
#include "..\Application.h"
#include "..\MediaProducerInfo.h"
#include "..\utils\image\image.hpp"

#include "..\monitor.h"

namespace caspar {

using namespace caspar::utils;

using std::tr1::cref;
using std::tr1::bind;

FramePlaybackControl::FramePlaybackControl(FramePlaybackStrategyPtr pStrategy) : pStrategy_(pStrategy), bPlaybackRunning_(false), isCGEmpty_(TRUE),
eventLoad_(FALSE, FALSE), eventRender_(FALSE, FALSE), eventStartPlayback_(FALSE, FALSE), eventPausePlayback_(FALSE, FALSE), eventStopPlayback_(FALSE, FALSE), eventStoppedPlayback_(FALSE, FALSE), pMonitor_(0)
{
	if(pStrategy_ == 0)
		throw std::exception("No valid FramePlaybackStrategy provided");

	pSystemFrameManager_.reset(new SystemFrameManager(pStrategy_->GetFrameManager()->GetFrameFormatDescription()));
	if(pSystemFrameManager_ == 0)
		throw std::exception("Failed to create SystemFrameManager");
}

FramePlaybackControl::~FramePlaybackControl()
{
	Stop();
}

void FramePlaybackControl::Start()
{
	worker_.Start(this);
}

void FramePlaybackControl::Stop()
{
	worker_.Stop();
	backgroundClip_.Clear();
}


////////////////////////////
// IPlaybackControl methods
bool FramePlaybackControl::Load(MediaProducerPtr pFP, bool loop)
{
	if(pFP == 0)
		return false;

	pFP->SetLoop(loop);

	FrameMediaController* pMediaController = dynamic_cast<FrameMediaController*>(pFP->QueryController(TEXT("FrameController")));
	if(pMediaController == 0)
		return false;

	if(!pMediaController->Initialize(pStrategy_->GetFrameManager()))
		return false;

	GetApplication()->GetAudioManager()->CueAudio(pMediaController);

	eventStopPlayback_.Set();

	{
		Lock lock(*this);
		backgroundClip_ = ClipInfo(pFP, pMediaController);
		eventLoad_.Set();
	}

	return true;
}

bool FramePlaybackControl::LoadBackground(MediaProducerPtr pFP, const TransitionInfo& transitionInfo, bool loop)
{
	MediaProducerPtr pMediaProducer;
	if(pFP == 0)
		return false;

	pFP->SetLoop(loop);

	ITransitionController* pTransitionController = 0;
	if(transitionInfo.type_ != Cut && transitionInfo.duration_ > 0) {
		//prepare transition
		TransitionProducerPtr pTransitionProducer(new TransitionProducer(pFP, transitionInfo, pStrategy_->GetFrameManager()->GetFrameFormatDescription()));
		pTransitionController = pTransitionProducer.get();
		pMediaProducer = pTransitionProducer;
	}
	else {
		pMediaProducer = pFP;
	}

	FrameMediaController* pMediaController = dynamic_cast<FrameMediaController*>(pMediaProducer->QueryController(TEXT("FrameController")));
	if(pMediaController == 0)
		return false;

	if(!pMediaController->Initialize(pStrategy_->GetFrameManager()))
		return false;

	GetApplication()->GetAudioManager()->CueAudio(pMediaController);

	{
		Lock lock(*this);
		backgroundClip_ = ClipInfo(pMediaProducer, pMediaController, pTransitionController);
	}

	return true;
}

bool FramePlaybackControl::Play()
{
	if(!backgroundClip_.IsEmpty()) {
		eventStartPlayback_.Set();
	}
	else {
		if(!bPlaybackRunning_) {
			LOG << LogLevel::Verbose << TEXT("Playing active clip");
			eventStartPlayback_.Set();
		}
		else {
			LOG << LogLevel::Verbose << TEXT("Failed to play. Already playing");
			return false;
		}
	}

	return true;
}

//bool FramePlaybackControl::Pause()
//{
//	eventPausePlayback_.Set();
//	return true;
//}

bool FramePlaybackControl::StopPlayback(bool block)
{
	eventStopPlayback_.Set();

	if(block)
	{
		if(::WaitForSingleObject(this->eventStoppedPlayback_, 1000) != WAIT_OBJECT_0)
			return false;
	}

	if(pMonitor_)
		pMonitor_->Inform(STOPPED);

	return true;
}

bool FramePlaybackControl::IsRunning()
{
	return bPlaybackRunning_;
}

bool FramePlaybackControl::Param(const tstring& param)
{
	return false;
}


////////////////////////////
// ICGControl methods
void FramePlaybackControl::Add(int layer, const tstring& templateName, bool playOnLoad, const tstring& label, const tstring& data) {
	if(isCGEmpty_ != FALSE) {
		CG::FlashCGProxyPtr pNewCGProducer = CG::FlashCGProxy::Create(pMonitor_);
		if(pNewCGProducer) {
			if(pNewCGProducer->Initialize(pSystemFrameManager_)) {
				taskQueue_.push_back(bind(&FramePlaybackControl::DoResetCGProducer, this, pNewCGProducer));
			}
			else {
				LOG << "Frameplayback: Failed to initialize new CGProducer";
				return;
			}
		}
	}

	//TODO: don't use bind to generate functor with string-arguments. Does it even work with const-reference parameters?
	taskQueue_.push_back(bind(&FramePlaybackControl::DoAdd, this, layer, tstring(templateName), playOnLoad, tstring(label), tstring(data)));
}
void FramePlaybackControl::Remove(int layer) {
	taskQueue_.push_back(bind(&FramePlaybackControl::DoRemove, this, layer));
}
void FramePlaybackControl::Clear() {
	taskQueue_.push_back(bind(&FramePlaybackControl::DoClear, this));
}
void FramePlaybackControl::LoadEmpty() {
	taskQueue_.push_back(bind(&FramePlaybackControl::DoLoadEmpty, this));
}
void FramePlaybackControl::Play(int layer) {
	taskQueue_.push_back(bind(&FramePlaybackControl::DoPlay, this, layer));
}
void FramePlaybackControl::Stop(int layer, unsigned int mixOutDuration) {
	taskQueue_.push_back(bind(&FramePlaybackControl::DoStop, this, layer, mixOutDuration));
}
void FramePlaybackControl::Next(int layer) {
	taskQueue_.push_back(bind(&FramePlaybackControl::DoNext, this, layer));
}
void FramePlaybackControl::Update(int layer, const tstring& data) {
	//NOTE: don't use bind to generate functor with string-arguments. Does it even work with const-reference parameters?
	taskQueue_.push_back(bind(&FramePlaybackControl::DoUpdate, this, layer, tstring(data)));
}
void FramePlaybackControl::Invoke(int layer, const tstring& label) {
	//NOTE: don't use bind to generate functor with string-arguments. Does it even work with const-reference parameters?
	taskQueue_.push_back(bind(&FramePlaybackControl::DoInvoke, this, layer, tstring(label)));
}

////////////////////////////
// IRunnable methods
void FramePlaybackControl::Run(HANDLE stopEvent)
{
	const int WaitHandleCount = 8;

	HANDLE waitHandles[WaitHandleCount] = { stopEvent, eventStopPlayback_, eventStartPlayback_, eventPausePlayback_, eventLoad_, taskQueue_.GetWaitEvent(), eventRender_, 0 };

	bool bQuit = false, bSingleFrame = false, bPureCG = false;
    while(!bQuit)
    {
		if((bPlaybackRunning_ || bSingleFrame) && !activeClip_.IsEmpty()) {
			waitHandles[WaitHandleCount - 1] = activeClip_.pFrameController_->GetFrameBuffer().GetWaitHandle();
			bPureCG = false;
		}
		else if(pCGProducer_) {
			waitHandles[WaitHandleCount - 1] = pCGProducer_->GetFrameBuffer().GetWaitHandle();
			bPureCG = true;
		}
		else
			waitHandles[WaitHandleCount - 1] = 0;
		
		int realWaitHandleCount = (WaitHandleCount-1);
		if(waitHandles[WaitHandleCount - 1] != 0)
			++realWaitHandleCount;

		DWORD waitResult = WaitForMultipleObjects(realWaitHandleCount, waitHandles, FALSE, 2500);
		switch(waitResult) {
			case WAIT_OBJECT_0:		//stopEvent
				bQuit = true;
				break;

			case WAIT_OBJECT_0 + 1:	//stopPlayback
				DoStopPlayback(waitHandles[WaitHandleCount - 1]);
				break;

			case WAIT_OBJECT_0 + 2:	//startPlayback
				if(DoStartPlayback(waitHandles[WaitHandleCount - 1]))
					bSingleFrame = true;
				break;

			case WAIT_OBJECT_0 + 3:	//pausePlayback
				//DoPausePlayback();
				break;

			case WAIT_OBJECT_0 + 4:	//load
				if(DoLoad(waitHandles[WaitHandleCount - 1]))
					bSingleFrame = true;
				break;

			case WAIT_OBJECT_0 + 5:	//cgtask waitEvent
				{
					Task task;
					taskQueue_.pop_front(task);
					if(task)
						task();
				}
				break;

			case WAIT_OBJECT_0 + 6:	//render
				DoRender(waitHandles[WaitHandleCount - 1], false);
				break;

			case WAIT_OBJECT_0 + 7:	//frame availible
				if(!bPureCG) {
					if(DoGetFrame(waitHandles[WaitHandleCount - 1], bSingleFrame))
						DoRender(waitHandles[WaitHandleCount - 1], false);
					bSingleFrame = false;
				}
				else {
					DoRender(waitHandles[WaitHandleCount - 1], true);
				}
				break;

			case WAIT_TIMEOUT:
				break;

			case WAIT_FAILED:
				bQuit = true;
				LOG << LogLevel::Critical << TEXT("Wait failed in FramePlayback. Aborting");
				break;
		}
	}

	activeClip_.Clear();
}

bool FramePlaybackControl::DoStartPlayback(HANDLE& handle) 
{
	bool bForceUpdate = false;

	{
		Lock lock(*this);

		bPlaybackRunning_ = true;
		if(!backgroundClip_.IsEmpty())
		{
			if(backgroundClip_.pTransitionController_ != 0) {
				if(!backgroundClip_.pTransitionController_->Start(activeClip_)) {
					backgroundClip_.lastFetchResult_ = FetchEOF;
					bForceUpdate = true;
				}
			}
			activeClip_ = backgroundClip_;
			backgroundClip_.Clear();
		}
	}

	if(pMonitor_)
		pMonitor_->Inform(PLAY);

	return bForceUpdate;
}

void FramePlaybackControl::DoStopPlayback(HANDLE& handle) 
{
	activeClip_.lastFetchResult_ = FetchEOF;
	if(!activeClip_.IsEmpty())
		GetApplication()->GetAudioManager()->StopAudio(activeClip_.pFrameController_);

	while(!frameQueue_.empty())
		frameQueue_.pop();

	bPlaybackRunning_ = false;
	this->eventStoppedPlayback_.Set();
}

bool FramePlaybackControl::DoLoad(HANDLE& handle)
{
	Lock lock(*this);

	if(!backgroundClip_.IsEmpty())
	{
		activeClip_ = backgroundClip_;
		backgroundClip_.Clear();
		return true;
	}

	return false;
}

void FramePlaybackControl::DoLoadEmpty()
{
	this->emptyProducer_ = GetApplication()->GetColorMediaManager()->CreateProducer(TEXT("#00000000"));
	this->Load(this->emptyProducer_, false);
}

void FramePlaybackControl::OnCGEmpty() {
	InterlockedExchange(&isCGEmpty_, TRUE);
	LOG << LogLevel::Debug << TEXT("Frameplayback: Flagged CGProducer as empty");
}

void FramePlaybackControl::DoResetCGProducer(CG::FlashCGProxyPtr pNewCGProducer) {
	if(isCGEmpty_ != FALSE) {
		LOG << LogLevel::Debug << TEXT("Frameplayback: Using new CGProducer");
		pCGProducer_ = pNewCGProducer;
		if(pCGProducer_) {
			InterlockedExchange(&isCGEmpty_, FALSE);
			pCGProducer_->SetEmptyAlert(bind(&FramePlaybackControl::OnCGEmpty, this));
		}
	}
}

void FramePlaybackControl::DoAdd(int layer, tstring templateName, bool playOnLoad, tstring label, tstring data) {
	if(pCGProducer_)	
		pCGProducer_->Add(layer, templateName, playOnLoad, label, data);
}

void FramePlaybackControl::DoRemove(int layer) {
	if(pCGProducer_)
		pCGProducer_->Remove(layer);
}


void FramePlaybackControl::DoClear() {
	if(pCGProducer_)
		pCGProducer_->Stop();
	else {
		OnCGEmpty();
	}
}

void FramePlaybackControl::DoPlay(int layer) {
	if(pCGProducer_)
		pCGProducer_->Play(layer);
}

void FramePlaybackControl::DoStop(int layer, unsigned int mixOutDuration) {
	if(pCGProducer_)
		pCGProducer_->Stop(layer, mixOutDuration);
}

void FramePlaybackControl::DoNext(int layer) {
	if(pCGProducer_)
		pCGProducer_->Next(layer);
}

void FramePlaybackControl::DoUpdate(int layer, tstring data) {
	if(pCGProducer_)
		pCGProducer_->Update(layer, data);
}

void FramePlaybackControl::DoInvoke(int layer, tstring label) {
	if(pCGProducer_)
		pCGProducer_->Invoke(layer, label);
}

bool FramePlaybackControl::DoGetFrame(HANDLE& handle, bool bSingleFrame)
{
	bool bDoRender = false;
	if(!activeClip_.IsEmpty()) 
	{
		bool bEOF = false;
		FramePtr pFrame = activeClip_.pFrameController_->GetFrameBuffer().front();

		if(pFrame != 0)
		{
			activeClip_.pLastFrame_ = pFrame;

			frameQueue_.push(pFrame);
			if(frameQueue_.size() >= 3)
				bDoRender = true;

			//Queue audio in the audioplayback-worker
			if(!bSingleFrame) {
				GetApplication()->GetAudioManager()->PushAudioData(activeClip_.pFrameController_, pFrame);

				//check for end of file
				activeClip_.lastFetchResult_ = activeClip_.pFrameController_->GetFrameBuffer().pop_front();
				if(activeClip_.lastFetchResult_ == FetchEOF) {
					bEOF = true;
				}
			}
			else
				bDoRender = true;
		}
		else {
			activeClip_.lastFetchResult_ = FetchEOF;
			bEOF = true;
		}

		if(bEOF) {
			//this producer is finnished, check if we should continue with another
			bPlaybackRunning_ = false;
			activeClip_.bStopped_ = true;

			MediaProducerPtr pFollowingProducer(activeClip_.pFP_->GetFollowingProducer());
			if(pFollowingProducer != 0) {
				FrameMediaController* pMediaController = dynamic_cast<FrameMediaController*>(pFollowingProducer->QueryController(TEXT("FrameController")));
				if(pMediaController != 0)
				{
					//reinitialize following producer with the correct framemanager
					if(pMediaController->Initialize(pStrategy_->GetFrameManager()))
					{
						//make following producer current
						activeClip_ = ClipInfo(pFollowingProducer, pMediaController);
						activeClip_.pLastFrame_ = pFrame;
						handle = activeClip_.pFrameController_->GetFrameBuffer().GetWaitHandle();
						bPlaybackRunning_ = true;
						return bDoRender;
					}
				}
			}
		
			if(pMonitor_)
				pMonitor_->Inform(STOPPED);

			bDoRender = true;
		}
	}

	return bDoRender;
}

void FramePlaybackControl::DoRender(HANDLE& handle, bool bPureCG) 
{
	//Get next CG-frame if we have a CGProducer
	FramePtr pCGFrame;
	if(pCGProducer_) {
		pCGFrame = pCGProducer_->GetFrameBuffer().front();
		FrameBufferFetchResult fetchResult = pCGProducer_->GetFrameBuffer().pop_front();
		if(pCGFrame != 0) {
			pLastCGFrame_ = pCGFrame;
		}
		else if(fetchResult != FetchEOF) {
			pCGFrame = pLastCGFrame_;
		}
		else {
			pCGProducer_.reset();
			LOG << LogLevel::Debug << TEXT("Frameplayback: Cleared CGProducer");
			OnCGEmpty();
			pLastCGFrame_.reset();
		}
	}

	//Get next video frame unless we're in PureCG-mode
	FramePtr pVideoFrame;
	if(!bPureCG || activeClip_.bStopped_) {
		if(frameQueue_.size() > 0) {
			pVideoFrame = frameQueue_.front();
			frameQueue_.pop();

			if(pVideoFrame != 0) {
				pLastVideoFrame_ = pVideoFrame;
			}
		}
		else {
			pVideoFrame = pLastVideoFrame_;
		}

		if(activeClip_.bStopped_ && !frameQueue_.empty())
			eventRender_.Set();
	}
	else {
		pVideoFrame = pLastVideoFrame_;
	}

	//combine and send to consumer
	FramePtr pResultFrame;
	if(pCGFrame) {
		if(pVideoFrame && this->activeClip_.pFP_ != this->emptyProducer_) {
			pResultFrame = pStrategy_->GetReservedFrame();
			if(pResultFrame) {
				utils::image::PreOver(pResultFrame->GetDataPtr(), pVideoFrame->GetDataPtr(), pCGFrame->GetDataPtr(), pResultFrame->GetDataSize());
			}
		}
		else
			pResultFrame = pCGFrame;
	}
	else
		pResultFrame = pVideoFrame;

	if(pResultFrame)
		pStrategy_->DisplayFrame(pResultFrame);
	else if(bPureCG) {
		pResultFrame = pStrategy_->GetReservedFrame();
		if(pResultFrame) {
			utils::image::Clear(pResultFrame->GetDataPtr(), pResultFrame->GetDataSize());
			pStrategy_->DisplayFrame(pResultFrame);
		}
	}
}

bool FramePlaybackControl::OnUnhandledException(const std::exception& ex) throw()
{
	bool bDoRestart = true;

	try 
	{
		LOG << LogLevel::Critical << TEXT("UNHANDLED EXCEPTION in frameplayback thread. Message: ") << ex.what();
	}
	catch(...)
	{
		bDoRestart = false;
	}

	return bDoRestart;
}

}	//namespace caspar