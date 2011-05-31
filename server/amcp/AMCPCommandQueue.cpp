#include "..\stdafx.h"

#include "AMCPCommandQueue.h"

namespace caspar {
namespace amcp {

using namespace utils;

AMCPCommandQueue::AMCPCommandQueue() : newCommandEvent_(FALSE, FALSE) {
}

AMCPCommandQueue::~AMCPCommandQueue() {
	Stop();
}

bool AMCPCommandQueue::Start() {
	if(commandPump_.IsRunning())
		return false;

	return commandPump_.Start(this);
}

void AMCPCommandQueue::Stop() {
	commandPump_.Stop();
}

void AMCPCommandQueue::AddCommand(AMCPCommandPtr pNewCommand)
{
	{
		Lock lock(*this);

		if(pNewCommand->GetScheduling() == ImmediatelyAndClear) {
			//Clears the queue, objects are deleted automatically
			commands_.clear();

			commands_.push_back(pNewCommand);
			LOG << LogLevel::Verbose << TEXT("Cleared queue and added command");
		}
		else {
			commands_.push_back(pNewCommand);
			LOG << LogLevel::Verbose << TEXT("Added command to end of queue");
		}
	}

	SetEvent(newCommandEvent_);
}

void AMCPCommandQueue::Run(HANDLE stopEvent)
{
	bool logTemporarilyBadState = true;
	AMCPCommandPtr pCurrentCommand;

	LOG << LogLevel::Verbose << TEXT("CommandPump started");

	while(WaitForSingleObject(stopEvent, 0) != WAIT_OBJECT_0) {
		DWORD waitResult = WaitForSingleObject(newCommandEvent_, 50);
		if(waitResult == WAIT_OBJECT_0) {
			Lock lock(*this);

			if(commands_.size() > 0) {
				LOG << LogLevel::Debug << TEXT("Found ") << commands_.size() << TEXT(" commands in queue");

				AMCPCommandPtr pNextCommand = commands_.front();

				if(pCurrentCommand == 0 || pNextCommand->GetScheduling() == ImmediatelyAndClear) {
					pCurrentCommand = pNextCommand;
					commands_.pop_front();
				}
			}
		}

		if(pCurrentCommand != 0) {
			AMCPCommandCondition condition = pCurrentCommand->CheckConditions();
			if(condition == ConditionTemporarilyBad) {
				if(logTemporarilyBadState) {
					LOG << LogLevel::Debug << TEXT("Cound not execute command right now, waiting a sec");
					logTemporarilyBadState = false;
				}

				//don't fail, just wait for a while and then try again
				continue;
			}
			else if(condition == ConditionGood) {
				try
				{
					if(pCurrentCommand->Execute()) {
						LOG << LogLevel::Verbose << TEXT("Executed command");
					}
					else {
						LOG << LogLevel::Verbose << TEXT("Failed to executed command");
					}		
				}
				catch(...)
				{
					LOG << LogLevel::Verbose << TEXT("UNEXPECTED EXCEPTION: Failed to executed command");
				}
			}
			else {	//condition == ConditionPermanentlyBad
				LOG << TEXT("Invalid commandobject");
			}

			pCurrentCommand->SendReply();
			pCurrentCommand.reset();

			newCommandEvent_.Set();
			logTemporarilyBadState = true;

			LOG << LogLevel::Debug << TEXT("Ready for a new command");
		}
	}

	LOG << LogLevel::Verbose << TEXT("CommandPump ended");
}

bool AMCPCommandQueue::OnUnhandledException(const std::exception& ex) throw() {
	bool bDoRestart = true;

	try 
	{
		LOG << LogLevel::Critical << TEXT("UNHANDLED EXCEPTION in commandqueue. Message: ") << ex.what();
	}
	catch(...)
	{
		bDoRestart = false;
	}

	return bDoRestart;
}

}	//namespace amcp
}	//namespace caspar