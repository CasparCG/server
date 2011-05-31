#include "..\..\stdafx.h"

#include "FlashCommand.h"
#include "FlashManager.h"
#include "FlashProducer.h"

namespace caspar
{

//////////////////////
// FlashCommand
FlashCommand::FlashCommand(FlashProducer* pHost) : pHost_(pHost), eventDone_(FALSE, FALSE) 
{}

FlashCommand::~FlashCommand() {
}

void FlashCommand::Execute()
{
	if(pHost_ != 0)
		result = DoExecute();

	eventDone_.Set();
}

void FlashCommand::Cancel() {
	eventDone_.Set();
}

bool FlashCommand::Wait(DWORD timeout)
{
	if(!pHost_->worker_.IsRunning())
		return false;

	HRESULT result = WaitForSingleObject(eventDone_, timeout);
	return (result == WAIT_OBJECT_0);
}

/////////////////////////
//  GenericFlashCommand
GenericFlashCommand::GenericFlashCommand(caspar::FlashProducer *pHost, caspar::FlashMemberFnPtr pFunction) : FlashCommand(pHost), pFunction_(pFunction) {
}
GenericFlashCommand::GenericFlashCommand(caspar::FlashProducer *pHost, caspar::FlashMemberFnPtr pFunction, const tstring &parameter) : FlashCommand(pHost), pFunction_(pFunction), parameter_(parameter) {
}
GenericFlashCommand::~GenericFlashCommand() {
}

bool GenericFlashCommand::DoExecute() {
	if(pFunction_ != 0)
		return (GetHost()->*pFunction_)(parameter_);
	
	return false;
}

////////////////////////////
//  InitializeFlashCommand
InitializeFlashCommand::InitializeFlashCommand(FlashProducer *pHost, FrameManagerPtr pFrameManager) : FlashCommand(pHost), pFrameManager_(pFrameManager) {
}

InitializeFlashCommand::~InitializeFlashCommand() {
}

bool InitializeFlashCommand::DoExecute() {
	return GetHost()->DoInitialize(pFrameManager_);
}

}