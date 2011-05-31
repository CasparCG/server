#ifndef _COMMANDQUEUE_H__
#define _COMMANDQUEUE_H__

#pragma once

#include <list>
#include "thread.h"
#include "Lockable.h"

namespace caspar {
namespace utils {

//TODO: Add idle-processing. preferably as a functor suplied at construction-time

//CommandQueue is a 
template<typename T>
class CommandQueue
{
	template<typename U>
	class WorkerThread : public utils::IRunnable, private utils::LockableObject
	{
	public:
		WorkerThread() : commandAvailibleEvent_(TRUE, FALSE)
		{}

		void AddCommand(U pCommand);
		virtual void Run(HANDLE stopEvent);
		virtual bool OnUnhandledException(const std::exception& ex) throw();

	private:
		utils::Event	commandAvailibleEvent_;

		//Needs synro-protection
		std::list<U> commands_;
	};

	CommandQueue(const CommandQueue&);
	CommandQueue& operator=(const CommandQueue&);
public:
	CommandQueue() : pWorker_(new WorkerThread<T>())
	{}
	~CommandQueue() 
	{}

	bool Start();
	void Stop();
	void AddCommand(T pCommand);

private:
	utils::Thread	commandPump_;
	std::tr1::shared_ptr<WorkerThread<T> > pWorker_;

};

template<typename T>
bool CommandQueue<T>::Start() {
	return commandPump_.Start(pWorker_.get());
}

template<typename T>
void CommandQueue<T>::Stop() {
	commandPump_.Stop();
}

template<typename T>
void CommandQueue<T>::AddCommand(T pCommand) {
	pWorker_->AddCommand(pCommand);
}

template<typename T>
template<typename U>
void CommandQueue<T>::WorkerThread<U>::AddCommand(U pCommand) {
	Lock lock(*this);

	commands_.push_back(pCommand);
	commandAvailibleEvent_.Set();
}

template<typename T>
template<typename U>
void CommandQueue<T>::WorkerThread<U>::Run(HANDLE stopEvent)
{
	HANDLE events[2] = {commandAvailibleEvent_, stopEvent};
	U pCommand;

	while(true) {
		DWORD waitResult = WaitForMultipleObjects(2, events, FALSE, 1000);
		int result = waitResult - WAIT_OBJECT_0;

		if(result == 1) {
			break;
		}
		else if(result == 0) {
			Lock lock(*this);

			if(commands_.size() > 0) {
				pCommand = commands_.front();
				commands_.pop_front();

				if(commands_.size() == 0)
					commandAvailibleEvent_.Reset();
			}
		}

		if(pCommand != 0) {
			pCommand->Execute();
			pCommand.reset();
		}
	}
}

template<typename T>
template<typename U>
bool CommandQueue<T>::WorkerThread<U>::OnUnhandledException(const std::exception& ex) throw() {
	bool bDoRestart = true;

	try 
	{
		LOG << LogLevel::Critical << TEXT("UNHANDLED EXCEPTION in commandqueue. Message: ") << ex.what() << LogStream::Flush;
	}
	catch(...)
	{
		bDoRestart = false;
	}

	return bDoRestart;
}

}	//namespace utils
}	//namespace caspar

#endif	//_COMMANDQUEUE_H__