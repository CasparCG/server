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

#include "AMCPCommandQueue.h"

namespace caspar { namespace amcp {

using namespace common;

AMCPCommandQueue::AMCPCommandQueue() : newCommandEvent_(FALSE, FALSE) 
{}

AMCPCommandQueue::~AMCPCommandQueue() 
{
	Stop();
}

bool AMCPCommandQueue::Start() 
{
	if(commandPump_.IsRunning())
		return false;

	return commandPump_.Start(this);
}

void AMCPCommandQueue::Stop() 
{
	commandPump_.Stop();
}

void AMCPCommandQueue::AddCommand(AMCPCommandPtr pNewCommand)
{
	{
		tbb::mutex::scoped_lock lock(mutex_);

		if(pNewCommand->GetScheduling() == ImmediatelyAndClear) {
			//Clears the queue, objects are deleted automatically
			commands_.clear();

			commands_.push_back(pNewCommand);
			CASPAR_LOG(info) << "Cleared queue and added command";
		}
		else {
			commands_.push_back(pNewCommand);
			CASPAR_LOG(info) << "Added command to end of queue";
		}
	}

	SetEvent(newCommandEvent_);
}

void AMCPCommandQueue::Run(HANDLE stopEvent)
{
	bool logTemporarilyBadState = true;
	AMCPCommandPtr pCurrentCommand;

	CASPAR_LOG(info) << "AMCP CommandPump started";

	while(WaitForSingleObject(stopEvent, 0) != WAIT_OBJECT_0)
	{
		DWORD waitResult = WaitForSingleObject(newCommandEvent_, 50);
		if(waitResult == WAIT_OBJECT_0) 
		{
			tbb::mutex::scoped_lock lock(mutex_);

			if(commands_.size() > 0)
			{
				CASPAR_LOG(debug) << "Found " << commands_.size() << " commands in queue";

				AMCPCommandPtr pNextCommand = commands_.front();

				if(pCurrentCommand == 0 || pNextCommand->GetScheduling() == ImmediatelyAndClear) {
					pCurrentCommand = pNextCommand;
					commands_.pop_front();
				}
			}
		}

		if(pCurrentCommand != 0) 
		{
			if(pCurrentCommand->Execute()) 
				CASPAR_LOG(info) << "Executed command";
			else 
				CASPAR_LOG(info) << "Failed to executed command";
				
			pCurrentCommand->SendReply();
			pCurrentCommand.reset();

			newCommandEvent_.Set();
			logTemporarilyBadState = true;

			CASPAR_LOG(info) << "Ready for a new command";
		}
	}

	CASPAR_LOG(info) << "CommandPump ended";
}

bool AMCPCommandQueue::OnUnhandledException(const std::exception& ex) throw() 
{
	bool bDoRestart = true;

	try 
	{
		CASPAR_LOG(fatal) << "UNHANDLED EXCEPTION in commandqueue. Message: " << ex.what();
	}
	catch(...)
	{
		bDoRestart = false;
	}

	return bDoRestart;
}

}}