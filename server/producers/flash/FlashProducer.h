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
#ifdef ENABLE_MONITOR
class Monitor;
#endif

class FlashProducer;
typedef std::tr1::shared_ptr<FlashProducer> FlashProducerPtr;

typedef std::tr1::function<void()> EmptyCallback;

class FlashProducer : public MediaProducer, FrameMediaController, utils::IRunnable
{
	FlashProducer(const FlashProducer&);

public:
#ifdef ENABLE_MONITOR
	static FlashProducerPtr Create(const tstring& filename, Monitor* pMonitor=0);
#else
	static FlashProducerPtr Create(const tstring& filename);
#endif
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

	FramePtr pCurrentFrame_;

	FramePtr RenderFrame() const;
	void WriteFields(void);
	void WriteFrame(void);
	std::tr1::function<void()> pFnRenderer_;

	FrameManagerPtr pFrameManager_;
#ifdef ENABLE_MONITOR
	Monitor* pMonitor_;
#endif
	//void CopyFieldToFrameBuffer(FramePtr frame1, FramePtr frame2, size_t fieldIndex, size_t width, size_t height);
};

typedef bool (FlashProducer::*FlashMemberFnPtr)(const tstring&);

}

#endif