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