#ifndef _CASPAR_FRAMEPLAYBACKCONTROL_H__
#define _CASPAR_FRAMEPLAYBACKCONTROL_H__

#pragma once

#include <memory>
#include <queue>
#include "..\PlaybackControl.h"
#include "..\cg\cgcontrol.h"
#include "FramePlaybackStrategy.h"
#include "..\utils\thread.h"
#include "..\utils\lockable.h"
#include "..\utils\taskqueue.h"
#include "ClipInfo.h"
#include "systemframemanager.h"

namespace caspar {

namespace CG
{ 
class FlashCGProxy;
typedef std::tr1::shared_ptr<FlashCGProxy> FlashCGProxyPtr;
}

class FramePlaybackControl : public IPlaybackControl, public CG::ICGControl, public utils::IRunnable, private utils::LockableObject
{
public:
	explicit FramePlaybackControl(FramePlaybackStrategyPtr);
	virtual ~FramePlaybackControl();

	void Start();
	void Stop();

	//IPlaybackControl
	virtual bool Load(MediaProducerPtr pFP, bool loop);
	virtual bool LoadBackground(MediaProducerPtr pFP, const TransitionInfo& transitionInfo, bool loop);
	virtual bool Play();
	//virtual bool Pause();
	virtual bool StopPlayback(bool block = false);
	virtual bool IsRunning();
	virtual bool Param(const tstring& param);
	virtual CG::ICGControl* GetCGControl() {
		return this;
	}

	//ICGControl
	virtual void Add(int layer, const tstring& templateName, bool playOnLoad, const tstring& label, const tstring& data);
	virtual void Remove(int layer);
	virtual void Clear();
	virtual void Play(int layer);
	virtual void Stop(int layer, unsigned int mixOutDuration);
	virtual void Next(int layer);
	virtual void Update(int layer, const tstring& data);
	virtual void Invoke(int layer, const tstring& label);

	void DoResetCGProducer(CG::FlashCGProxyPtr pNewCGProducer);
	void OnCGEmpty();

	//IRunnable
	virtual void Run(HANDLE stopEvent);
	virtual bool OnUnhandledException(const std::exception& ex) throw();

private:
	FramePlaybackStrategyPtr pStrategy_;
	SystemFrameManagerPtr pSystemFrameManager_;

	volatile bool bPlaybackRunning_;
	volatile LONG isCGEmpty_;
	
	utils::Event eventStoppedPlayback_;
	utils::Thread worker_;
	ClipInfo activeClip_;
	ClipInfo backgroundClip_;
	CG::FlashCGProxyPtr pCGProducer_;

	utils::Event eventLoad_;
	utils::Event eventRender_;
	utils::Event eventStartPlayback_;
	utils::Event eventPausePlayback_;
	utils::Event eventStopPlayback_;

	std::queue<FramePtr> frameQueue_;

	FramePtr pLastCGFrame_;
	FramePtr pLastVideoFrame_;

	bool DoLoad(HANDLE& handle);
	bool DoStartPlayback(HANDLE& handle);
	void DoStopPlayback(HANDLE& handle);
	void DoRender(HANDLE& handle, bool bPureCG);
	bool DoGetFrame(HANDLE& handle, bool bForceVideoOutput);

	//cg-tasks
	utils::TaskQueue taskQueue_;

	void DoAdd(int layer, tstring templateName, bool playOnLoad, tstring label, tstring data);
	void DoRemove(int layer);
	void DoClear();
	void DoPlay(int layer);
	void DoStop(int layer, unsigned int mixOutDuration);
	void DoNext(int layer);
	void DoUpdate(int layer, tstring data);
	void DoInvoke(int layer, tstring label);
};

typedef std::tr1::shared_ptr<FramePlaybackControl> FramePlaybackControlPtr;

}	//namespace caspar

#endif	//_CASPAR_FRAMEPLAYBACKCONTROL_H__