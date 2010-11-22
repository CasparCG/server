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