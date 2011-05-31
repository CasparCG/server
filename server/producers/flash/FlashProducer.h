#ifndef _FLASHPRODUCER_H_
#define _FLASHPRODUCER_H_

//std include
#include <vector>
#include <string>
#include <functional>

#include "..\..\MediaProducer.h"
#include "..\..\frame\FrameMediaController.h"
#include "FlashAxContainer.h"
#include "FlashCommandQueue.h"
#include "..\..\frame\BitmapFrame.h"
#include "FlashCommand.h"

#include "..\..\utils\thread.h"

namespace caspar
{

/*
	FlashProducer

	Changes:
	2009-06-08 (R.N) :  Changed "WriteFrame" to not copy when no new fram is rendered. 
						Holds the old frame and resends to the consumer until a new one is rendered
	2009-06-08 (R.N) :  Changed "WriteFrame" to not create temporary copy when manager returns bitmap frames
	2009-06-08 (R.N) :  Moved copy and clear function into "utils::image" namespace
*/

class FlashProducer;
typedef std::tr1::shared_ptr<FlashProducer> FlashProducerPtr;

typedef std::tr1::function<void()> EmptyCallback;

class FlashProducer : public MediaProducer, FrameMediaController, utils::IRunnable
{
	FlashProducer(const FlashProducer&);

public:
	static FlashProducerPtr Create(const tstring& filename);

	FlashProducer();
	virtual ~FlashProducer();

	//MediaProducer
	virtual IMediaController* QueryController(const tstring& id);
	virtual bool Param(const tstring& param);

	//FrameMediaController
	virtual bool Initialize(FrameManagerPtr pFrameManager);
	virtual FrameBuffer& GetFrameBuffer() {
		return frameBuffer_;
	}

	bool IsEmpty() const;

	void Stop();
	void SetEmptyAlert(EmptyCallback callback);

	bool Create();
	bool Load(const tstring& filename);

private:

	friend class FlashCommand;
	friend class InitializeFlashCommand;
	friend class flash::FlashAxContainer;
	friend class FullscreenControllerFlashCommand;

	bool DoCreate(const tstring&);
	bool DoLoad(const tstring &);
	bool DoInitialize(FrameManagerPtr pFrameManager);
	bool DoParam(const tstring&);

	virtual void Run(HANDLE stopEvent);
	virtual bool OnUnhandledException(const std::exception& ex) throw();

	EmptyCallback emptyCallback;

	MotionFrameBuffer frameBuffer_;

	utils::Thread worker_;

	tstring filename_;
	bool bRunning_;
	CComObject<caspar::flash::FlashAxContainer>* pFlashAxContainer_;
	caspar::flash::FlashCommandQueue commandQueue_;

	FramePtr		pLastFrame_;
	BitmapHolderPtr pDestBitmapHolder_;

	bool			bitmapFrameManager_;

	bool WriteFields(void);
	bool WriteFrame(void);
	typedef bool (FlashProducer::*FlashRendererFnPtr)(void);
	FlashRendererFnPtr pFnRenderer_;

	FrameManagerPtr pFrameManager_;
};

typedef bool (FlashProducer::*FlashMemberFnPtr)(const tstring&);

}

#endif