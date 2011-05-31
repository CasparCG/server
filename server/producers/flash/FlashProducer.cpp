#include "..\..\stdafx.h"

#include "..\..\Application.h"
#include "TimerHelper.h"
#include "FlashProducer.h"
#include "..\..\utils\Logger.h"
#include "..\..\utils\LogLevel.h"
#include "..\..\utils\image\Image.hpp"
#include "..\..\application.h"

namespace caspar {

using namespace utils;

//////////////////////////////////////////////////////////////////////
// FlashProducer
//////////////////////////////////////////////////////////////////////
FlashProducer::FlashProducer() :	pFlashAxContainer_(0), bRunning_(false), frameBuffer_(2), 
									pFnRenderer_(&FlashProducer::WriteFrame)
{
}

FlashProducer::~FlashProducer() {
	worker_.Stop();
}

FlashProducerPtr FlashProducer::Create(const tstring& filename)
{
	FlashProducerPtr result;

	if(filename.length() > 0) {
		result.reset(new FlashProducer());

		if(!(result->Create() && result->Load(filename)))
			result.reset();
	}

	return result;
}

IMediaController* FlashProducer::QueryController(const tstring& id) {
	if(id == TEXT("FrameController"))
		return this;
	
	return 0;
}

void FlashProducer::Stop() {
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
	while(!bQuit) {
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
		if(bRunning_ && waitResult==(WAIT_OBJECT_0+2)) {
			if(pFlashAxContainer_->bHasNewTiming_) {
				pFlashAxContainer_->bHasNewTiming_ = false;
				const caspar::FrameFormatDescription& fmtDesc = pFrameManager_->GetFrameFormatDescription();

				//render frames if we're playing on a channel with a progressive format OR if the FPS of the flash is half that of the channel
				int flashFPS = pFlashAxContainer_->GetFPS();
				bool bFrames = (fmtDesc.mode == Progressive || ((flashFPS - fmtDesc.fps/2) == 0));
				pFnRenderer_ = bFrames ? (&FlashProducer::WriteFrame) : (&FlashProducer::WriteFields);
			}
			(this->*pFnRenderer_)();
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

		//take care of input (windowmessages)
		if(waitResult == (WAIT_OBJECT_0 + realWaitHandleCount)) {
			MSG msg;
			while(PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	if(pFlashAxContainer_) {
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

bool FlashProducer::OnUnhandledException(const std::exception& ex) throw() {
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
		try {
			pFlashAxContainer_ = 0;
		} catch(...){}
	}

	return false;
}

void FlashProducer::SetEmptyAlert(EmptyCallback callback) {
	emptyCallback = callback;
}

bool FlashProducer::IsEmpty() const {
	return (pFlashAxContainer_ != 0) ? pFlashAxContainer_->bIsEmpty_ : true;
}

//FIELDS RENDERING
// TODO (R.N) : rewrite. See: WriteFrame
// TODO (R.N) : use entire frame copy instead of field copy when its faster? See: WriteFrame 
bool FlashProducer::WriteFields()
{	
	const caspar::FrameFormatDescription& fmtDesc = pFrameManager_->GetFrameFormatDescription();

	FramePtr pResultFrame = pFrameManager_->CreateFrame();

	if(pResultFrame != 0 && pResultFrame->HasValidDataPtr())
	{
		//Render two fields
		for(int fieldIndex = 0; fieldIndex <= 1; ++fieldIndex) {
			if(pFlashAxContainer_->pTimerHelper)
				pFlashAxContainer_->pTimerHelper->Invoke();

			if(pFlashAxContainer_->bInvalidRect_) {
				if(!pFlashAxContainer_->IsReadyToRender()) {
					return false;
				}
				else {
					pFlashAxContainer_->bInvalidRect_ = false;
					image::Clear(pDestBitmapHolder_->GetPtr(), pFrameManager_->GetFrameFormatDescription().size);
					pFlashAxContainer_->DrawControl(pDestBitmapHolder_->GetDC());
				}
			}

			//copy every other row from field one
			utils::image::CopyField(pResultFrame->GetDataPtr(), pDestBitmapHolder_->GetPtr(), fieldIndex, fmtDesc.width, fmtDesc.height);
		}

		frameBuffer_.push_back(pResultFrame);
	}
	else {
		return false;
	}

	return true;
}

//FRAME RENDERING
bool FlashProducer::WriteFrame()
{
	//FIX: New but not quite working

	//FramePtr pResultFrame;

	//if(pFlashAxContainer_->bInvalidRect_)
	//{
	//	if(pFlashAxContainer_->IsReadyToRender())
	//	{
	//		pFlashAxContainer_->bInvalidRect_ = false;

	//		pResultFrame = pFrameManager_->CreateFrame();	

	//		if(pResultFrame != NULL && pResultFrame->GetDataPtr() != NULL)
	//		{			
	//			if(!bitmapFrameManager_)
	//			{				
	//				image::Clear(pDestBitmapHolder_->GetPtr(), pFrameManager_->GetFrameFormatDescription().size);
	//				pFlashAxContainer_->DrawControl(pDestBitmapHolder_->GetDC());	
	//				image::Copy(pResultFrame->GetDataPtr(), pDestBitmapHolder_->GetPtr(), pResultFrame->GetDataSize());
	//			}
	//			else
	//			{
	//				image::Clear(pResultFrame->GetDataPtr(), pFrameManager_->GetFrameFormatDescription().size);
	//				pFlashAxContainer_->DrawControl(reinterpret_cast<HDC>(pResultFrame->GetMetadata()));	
	//			}
	//		}	
	//	}
	//}	
	//else
	//	pResultFrame = pLastFrame_;

	//if(pResultFrame == NULL || pResultFrame->GetDataPtr() == NULL)
	//	return false;

	//if(pFlashAxContainer_->pTimerHelper)
	//	pFlashAxContainer_->pTimerHelper->Invoke();

	//pLastFrame_ = pResultFrame;

	//frameBuffer_.push_back(pResultFrame);

	//return true;


	// SAME THING BUT OLD, and working
	FramePtr pResultFrame = pFrameManager_->CreateFrame();

	if(pResultFrame != 0 && pResultFrame->GetDataPtr() != 0)
	{
		if(pFlashAxContainer_->pTimerHelper)
			pFlashAxContainer_->pTimerHelper->Invoke();

		if(pFlashAxContainer_->bInvalidRect_) {
			pFlashAxContainer_->bInvalidRect_ = false;

			if(!pFlashAxContainer_->IsReadyToRender()) {
				return false;
			}
			else {
				image::Clear(pDestBitmapHolder_->GetPtr(), pFrameManager_->GetFrameFormatDescription().size);
				pFlashAxContainer_->DrawControl(pDestBitmapHolder_->GetDC());					
			}
		}
		image::Copy(pResultFrame->GetDataPtr(), pDestBitmapHolder_->GetPtr(), pResultFrame->GetDataSize());
		frameBuffer_.push_back(pResultFrame);
	}
	else {
		return false;
	}

	return true;
}

bool FlashProducer::Create() {
	//Create worker-thread
	worker_.Start(this);

	//dispatch DoCreate-command
	FlashCommandPtr pCreateCommand(new GenericFlashCommand(this, &caspar::FlashProducer::DoCreate));

	commandQueue_.Push(pCreateCommand);
	if(pCreateCommand->Wait(INFINITE)) {
		return pCreateCommand->GetResult();
	}

	return false;
}

bool FlashProducer::Load(const tstring& filename) {
	if(worker_.IsRunning()) {
		//dispatch DoLoad-command
		FlashCommandPtr pLoadCommand(new GenericFlashCommand(this, &caspar::FlashProducer::DoLoad, filename));

		commandQueue_.Push(pLoadCommand);
		if(pLoadCommand->Wait(INFINITE)) {
			return pLoadCommand->GetResult();
		}
	}
	return false;
}

bool FlashProducer::Initialize(FrameManagerPtr pFrameManager) {
	if(worker_.IsRunning() && pFrameManager != 0) {
		//dispatch Initialize-command
		FlashCommandPtr pInitializeCommand(new InitializeFlashCommand(this, pFrameManager));
		commandQueue_.Push(pInitializeCommand);
		if(pInitializeCommand->Wait(INFINITE)) {
			return pInitializeCommand->GetResult();
		}
	}
	return false;	
}

bool FlashProducer::Param(const tstring &param) {
	if(worker_.IsRunning()) {
		//dispatch DoParam-command
		FlashCommandPtr pParamCommand(new GenericFlashCommand(this, &caspar::FlashProducer::DoParam, param));
		commandQueue_.Push(pParamCommand);
		return true;
	}
	return false;
}

//This is always run from the worker thread
//This is called when the MediaProducer is created
bool FlashProducer::DoCreate(const tstring&) {
	HRESULT hr = CComObject<caspar::flash::FlashAxContainer>::CreateInstance(&pFlashAxContainer_);
	if(pFlashAxContainer_) {
		pFlashAxContainer_->pFlashProducer_ = this;
		HRESULT hr = pFlashAxContainer_->CreateAxControl();

		CComPtr<IShockwaveFlash> spFlash;
		pFlashAxContainer_->QueryControl(&spFlash);
		if(SUCCEEDED(hr) && spFlash)
		{
			spFlash->put_Playing(TRUE);
		}
		else
			return false;
	}
	else
		return false;

	return true;
}

//This is always run from the worker thread
//This is called when the MediaProducer is created
bool FlashProducer::DoLoad(const tstring &filename) {
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
bool FlashProducer::DoInitialize(FrameManagerPtr pFrameManager) {

	int oldWidth = 0, oldHeight = 0;
	if(pFrameManager_ != 0) {
		const caspar::FrameFormatDescription& oldFmtDesc = pFrameManager_->GetFrameFormatDescription();
		oldWidth = oldFmtDesc.width;
		oldHeight = oldFmtDesc.height;
	}

	pFrameManager_ = pFrameManager;
	const caspar::FrameFormatDescription& fmtDesc = pFrameManager_->GetFrameFormatDescription();

	if(fmtDesc.width != oldWidth || fmtDesc.height != oldHeight) 
	{
		try
		{
			pDestBitmapHolder_.reset(new BitmapHolder(GetApplication()->GetMainWindow()->getHwnd(), fmtDesc.width, fmtDesc.height));
		}
		catch(std::exception& exception)
		{
			LOG << LogLevel::Critical << TEXT("Failed to create bitmap for flashproducer. Error: ") << GetLastError();
			caspar::GetApplication()->GetTerminateEvent().Set();
		}

		bitmapFrameManager_ = pFrameManager->HasFeature("BITMAP_FRAME");

		pFlashAxContainer_->SetFormat(fmtDesc);
	}

	bRunning_ = true;
	return true;
}

//This is always run from the worker thread
//this can get called at any time after the producer is loaded
bool FlashProducer::DoParam(const tstring& param) {

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

			if(hr != S_OK) {
				//LOG << LogLevel::Debug << TEXT("Flashproducer: ExternalInterface-call failed. (HRESULT = ") << hr << TEXT(")");
				if(pFlashAxContainer_->bCallSuccessful_)
					bSuccess = true;
				else {
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