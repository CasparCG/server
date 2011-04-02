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
 
#include "..\..\stdafx.h"

#include "..\..\Application.h"
#include "TimerHelper.h"
#include "FlashProducer.h"
#include "..\..\utils\Logger.h"
#include "..\..\utils\LogLevel.h"
#include "..\..\utils\image\Image.hpp"
#include "..\..\application.h"
#include "..\..\frame\BitmapFrameManager.h"
#include <tbb/parallel_invoke.h>

namespace caspar {

using namespace utils;

//////////////////////////////////////////////////////////////////////
// FlashProducer
//////////////////////////////////////////////////////////////////////
FlashProducer::FlashProducer() : pFlashAxContainer_(0), bRunning_(false), frameBuffer_(10), pFnRenderer_(std::tr1::bind(&FlashProducer::WriteFrame, this)), pMonitor_(0), timerCount_(0)
{
}

FlashProducer::~FlashProducer() 
{
	worker_.Stop();
}

FlashProducerPtr FlashProducer::Create(const tstring& filename, Monitor* pMonitor)
{
	FlashProducerPtr result;

	if(filename.length() > 0) {
		result.reset(new FlashProducer());
		result->pMonitor_ = pMonitor;

		if(!(result->Create() && result->Load(filename)))
			result.reset();
	}

	return result;
}

IMediaController* FlashProducer::QueryController(const tstring& id) 
{	
	return id == TEXT("FrameController") ? this : 0;
}

void FlashProducer::Stop() 
{
	worker_.Stop(false);
}

void FlashProducer::Run(HANDLE stopEvent)
{
//#ifdef DEBUG
//	srand(timeGetTime());
//	int frameIndex = 0;
//	int crashIndex = 200+rand()%200;
//#endif

	::OleInitialize(NULL);
	LOG << LogLevel::Verbose << TEXT("Flash readAhead thread started");

	HANDLE waitHandles[3] = { stopEvent, commandQueue_.GetWaitHandle(), frameBuffer_.GetWriteWaitHandle() };
	DWORD waitHandlesCount = sizeof(waitHandles) / sizeof(HANDLE) - 1;

	bool bQuit = false;
	while(!bQuit) 
	{
		DWORD realWaitHandleCount = waitHandlesCount;
		if(bRunning_)
			++realWaitHandleCount;

		HRESULT waitResult = MsgWaitForMultipleObjects(realWaitHandleCount, waitHandles, FALSE, 1000, QS_ALLINPUT);
		switch(waitResult)
		{
		case (WAIT_OBJECT_0+0):	//stop
		case WAIT_FAILED:		//wait failiure
			bQuit = true;
			continue;

		case (WAIT_OBJECT_0+1):	//command
			{
				FlashCommandPtr pFlashCommand = commandQueue_.Pop();
				if(pFlashCommand != 0) {
					pFlashCommand->Execute();
					pFlashCommand.reset();
				}
			}
			continue;

		case (WAIT_TIMEOUT):	//nothing has happened...
			continue;
		}

		//render next frame
		if(bRunning_ && waitResult==(WAIT_OBJECT_0+2)) 
		{
			if(pFlashAxContainer_->bHasNewTiming_) 
			{
				pFlashAxContainer_->bHasNewTiming_ = false;
				const caspar::FrameFormatDescription& fmtDesc = pFrameManager_->GetFrameFormatDescription();

				//render frames if we're playing on a channel with a progressive format OR if the FPS of the flash is half that of the channel
				int flashFPS = pFlashAxContainer_->GetFPS();
				bool bFrames = (fmtDesc.mode == Progressive || ((flashFPS - fmtDesc.fps/2) == 0));
				pFnRenderer_ = std::tr1::bind(bFrames ? &FlashProducer::WriteFrame : &FlashProducer::WriteFields, this);
			}
			
			if(pFlashAxContainer_->IsReadyToRender())
				this->pFnRenderer_();
//#ifdef DEBUG
//			++frameIndex;
//			if(frameIndex >= crashIndex)
//			{
//				//Go down in a ball of fire!
//				int* pCrash = 0;
//				*pCrash = 42;
//			}
//#endif
		}
		
		static int logCount = 0;
		//take care of input (windowmessages)
		if(waitResult == (WAIT_OBJECT_0 + realWaitHandleCount)) 
		{
			MSG msg;
			while(PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE)) 
			{
				if(msg.message != WM_TIMER)
				{
					if(logCount < 1000)
					{
						LOG << TEXT("[FlashProducer] ####### Received MSG message: ") << msg.message << TEXT(" lParam: ") << msg.lParam << TEXT(" wParam: ") << msg.wParam;
						++logCount;
					}
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
		}
	}

	if(pFlashAxContainer_) 
	{
		pFlashAxContainer_->DestroyAxControl();

		pFlashAxContainer_->Release();
		pFlashAxContainer_ = 0;
	}

	commandQueue_.Clear();

	FramePtr pNullFrame;
	frameBuffer_.push_back(pNullFrame);
	LOG << LogLevel::Verbose << TEXT("Flash readAhead thread ended");
	::OleUninitialize();
}

bool FlashProducer::OnUnhandledException(const std::exception& ex) throw() 
{
	try 
	{
		FramePtr pNullFrame;
		frameBuffer_.push_back(pNullFrame);

		LOG << LogLevel::Critical << TEXT("UNHANDLED EXCEPTION in flash readahead-thread. Message: ") << ex.what();

		if(pFlashAxContainer_) {
			pFlashAxContainer_->DestroyAxControl();

			pFlashAxContainer_->Release();
			pFlashAxContainer_ = 0;
		}

		commandQueue_.Clear();
		::OleUninitialize();
	}
	catch(...)
	{
		try 
		{
			pFlashAxContainer_ = 0;
		} 
		catch(...){}
	}

	return false;
}

void FlashProducer::SetEmptyAlert(EmptyCallback callback) 
{
	emptyCallback = callback;
}

bool FlashProducer::IsEmpty() const 
{
	return (pFlashAxContainer_ != 0) ? pFlashAxContainer_->bIsEmpty_ : true;
}

void FlashProducer::WriteFields()
{		
	const caspar::FrameFormatDescription& fmtDesc = pFrameManager_->GetFrameFormatDescription();
	
	FramePtr pNextFrame1 = RenderFrame();
	FramePtr pNextFrame2 = RenderFrame();
	
	if(pNextFrame1 != pNextFrame2)
	{
		FramePtr pNewFrame = pFrameManager_->CreateFrame();
		tbb::parallel_invoke
		(
			[&]{utils::image::CopyField(pNewFrame->GetDataPtr(), pNextFrame1->GetDataPtr(), 0, fmtDesc.width, fmtDesc.height);},
			[&]{utils::image::CopyField(pNewFrame->GetDataPtr(), pNextFrame2->GetDataPtr(), 1, fmtDesc.width, fmtDesc.height);}
		);
		pNextFrame1 = pNewFrame;
	}

	frameBuffer_.push_back(pNextFrame1);
}

void FlashProducer::WriteFrame()
{
	frameBuffer_.push_back(RenderFrame()); // Consumer always expects frameFramePtr pResultFrame = pFrameManager_->CreateFrame();
}

FramePtr FlashProducer::RenderFrame()
{	
	const caspar::FrameFormatDescription& fmtDesc = pFrameManager_->GetFrameFormatDescription();
	
	/// TICK
	if(pFlashAxContainer_->pTimerHelper)
	{
		DWORD time = pFlashAxContainer_->pTimerHelper->Invoke(); // Tick flash
		if(time - timerCount_ >= 400)
		{
			timerCount_ = time;
			HRESULT hr;
			pFlashAxContainer_->m_spInPlaceObjectWindowless->OnWindowMessage(WM_TIMER, 3, 0, &hr);
		}
	}

	/// RENDER
	if(pFlashAxContainer_->bInvalidRect_ || pCurrentFrame_ == NULL)
	{		
		FramePtr pNewFrame = pFrameManager_->CreateFrame();

		image::Clear(pNewFrame->GetDataPtr(), pFrameManager_->GetFrameFormatDescription().size);
		pFlashAxContainer_->DrawControl(reinterpret_cast<HDC>(pNewFrame->GetMetadata())); // error handling?	

		pFlashAxContainer_->bInvalidRect_ = false;	
		
		pCurrentFrame_ = pNewFrame;			
	}	
	assert(pCurrentFrame_);
	return pCurrentFrame_;
}

bool FlashProducer::Create() 
{
	//Create worker-thread
	worker_.Start(this);

	//dispatch DoCreate-command
	FlashCommandPtr pCreateCommand(new GenericFlashCommand(this, &caspar::FlashProducer::DoCreate));

	commandQueue_.Push(pCreateCommand);
	if(pCreateCommand->Wait(INFINITE))
		return pCreateCommand->GetResult();	

	return false;
}

bool FlashProducer::Load(const tstring& filename) 
{
	if(worker_.IsRunning()) 
	{
		//dispatch DoLoad-command
		FlashCommandPtr pLoadCommand(new GenericFlashCommand(this, &caspar::FlashProducer::DoLoad, filename));

		commandQueue_.Push(pLoadCommand);
		if(pLoadCommand->Wait(INFINITE))
			return pLoadCommand->GetResult();		
	}
	return false;
}

bool FlashProducer::Initialize(FrameManagerPtr pFrameManager) 
{
	if(worker_.IsRunning() && pFrameManager != 0) 
	{
		//dispatch Initialize-command
		FlashCommandPtr pInitializeCommand(new InitializeFlashCommand(this, pFrameManager));
		commandQueue_.Push(pInitializeCommand);
		if(pInitializeCommand->Wait(INFINITE)) 
			return pInitializeCommand->GetResult();		
	}
	return false;	
}

bool FlashProducer::Param(const tstring &param) 
{
	if(worker_.IsRunning()) 
	{
		//dispatch DoParam-command
		FlashCommandPtr pParamCommand(new GenericFlashCommand(this, &caspar::FlashProducer::DoParam, param));
		commandQueue_.Push(pParamCommand);
		return true;
	}
	return false;
}

//This is always run from the worker thread
//This is called when the MediaProducer is created
bool FlashProducer::DoCreate(const tstring&) 
{
	HRESULT hr = CComObject<caspar::flash::FlashAxContainer>::CreateInstance(&pFlashAxContainer_);
	if(pFlashAxContainer_) 
	{
		pFlashAxContainer_->pFlashProducer_ = this;
		HRESULT hr = pFlashAxContainer_->CreateAxControl();
		if(FAILED(hr))
			return false;

		CComPtr<IShockwaveFlash> spFlash;
		pFlashAxContainer_->QueryControl(&spFlash);
		if(spFlash)		
			spFlash->put_Playing(TRUE);		
		else
			return false;
	}
	else
		return false;

	return true;
}

//This is always run from the worker thread
//This is called when the MediaProducer is created
bool FlashProducer::DoLoad(const tstring &filename) 
{
	filename_ = filename;
	if(filename_.length() == 0)
		return false;

	CComPtr<IShockwaveFlash> spFlash;
	pFlashAxContainer_->QueryControl(&spFlash);
	if(!spFlash)
		return false;

	spFlash->put_AllowFullScreen(CComBSTR(TEXT("true")));
	HRESULT hrLoad = spFlash->put_Movie(CComBSTR(filename_.c_str()));
	spFlash->put_ScaleMode(2);	//Exact fit. Scale without respect to the aspect ratio
	return SUCCEEDED(hrLoad);
}

//This is always run from the worker thread
//This is called från FrameMediaController::Initialize
bool FlashProducer::DoInitialize(FrameManagerPtr pFrameManager)
{
	int oldWidth = 0, oldHeight = 0;
	if(pFrameManager_ != 0) 
	{
		const caspar::FrameFormatDescription& oldFmtDesc = pFrameManager_->GetFrameFormatDescription();
		oldWidth = oldFmtDesc.width;
		oldHeight = oldFmtDesc.height;
	}

	pFrameManager_.reset(new BitmapFrameManager(pFrameManager->GetFrameFormatDescription(), GetApplication()->GetMainWindow()->getHwnd()));
	const caspar::FrameFormatDescription& fmtDesc = pFrameManager_->GetFrameFormatDescription();
	
	pCurrentFrame_ = FramePtr();

	if(fmtDesc.width != oldWidth || fmtDesc.height != oldHeight) 	
		pFlashAxContainer_->SetFormat(fmtDesc);		

	bRunning_ = true;
	return true;
}

//This is always run from the worker thread
//this can get called at any time after the producer is loaded
bool FlashProducer::DoParam(const tstring& param) 
{
	HRESULT hr;
	CComPtr<IShockwaveFlash> spFlash;
	pFlashAxContainer_->bIsEmpty_ = false;
	pFlashAxContainer_->bCallSuccessful_ = false;
	pFlashAxContainer_->QueryControl(&spFlash);
	CComBSTR request(param.c_str());
	//ATLTRACE(_T("ShockwaveFlash::CallFuntion\n"));

	if(spFlash)
	{
		int retries = 0;
		bool bSuccess = false;

		while(!bSuccess && retries < 5)
		{
			CComBSTR result;
			//LOG << LogLevel::Debug << TEXT("Calling ExternalInterface: ") << param;
			hr = spFlash->CallFunction(request, &result);
			bSuccess = (hr == S_OK);

			if(hr != S_OK) 
			{
				//LOG << LogLevel::Debug << TEXT("Flashproducer: ExternalInterface-call failed. (HRESULT = ") << hr << TEXT(")");
				if(pFlashAxContainer_->bCallSuccessful_)
				{
					bSuccess = true;
				}
				else 
				{
					++retries;
					LOG << LogLevel::Debug << TEXT("Retrying. Count = ") << retries;
				}
			}
		}

		return (hr == S_OK);
	}

	return false;
}

}