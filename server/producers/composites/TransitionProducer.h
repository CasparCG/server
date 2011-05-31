#pragma once

#include "..\..\MediaProducer.h"
#include "..\..\frame\FrameManager.h"
#include "..\..\frame\FrameMediaController.h"
#include "..\..\frame\SystemFrameManager.h"
#include "..\..\utils\runnable.h"
#include "..\..\utils\Noncopyable.hpp"

#include <vector>

namespace caspar {

	class ClipInfo;
	class TransitionInfo;

namespace utils {
	class PixmapData;
	typedef std::tr1::shared_ptr<PixmapData> PixmapDataPtr;
}

// TODO: Put into its own header? (R.N)
class ITransitionController
{
public:
	virtual bool Start(const ClipInfo& srcClipInfo) = 0;
};

class TransitionProducer : public MediaProducer, public FrameMediaController, public ITransitionController, public utils::IRunnable, private utils::Noncopyable
{
public:
	TransitionProducer(MediaProducerPtr pDestination, const TransitionInfo& transitionInfo, const FrameFormatDescription& fmtDesc);
	virtual ~TransitionProducer();

	//MediaProducer
	virtual IMediaController* QueryController(const tstring& id);
	virtual MediaProducerPtr GetFollowingProducer();

	//FrameMediaController
	virtual bool Initialize(FrameManagerPtr pFrameManager);
	virtual FrameBuffer& GetFrameBuffer();

	//ITransitionController
	virtual bool Start(const ClipInfo& srcClipInfo);

	//IRunnable
	virtual void Run(HANDLE stopEvent);
	virtual bool OnUnhandledException(const std::exception& ex) throw();

private:

	struct Implementation;
	std::tr1::shared_ptr<Implementation> pImpl_;
};
typedef std::tr1::shared_ptr<TransitionProducer> TransitionProducerPtr;

}	//namespace caspar