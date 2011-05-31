#ifndef _AMCPCOMMANDQUEUE_H__
#define _AMCPCOMMANDQUEUE_H__

#pragma once

#include <list>
#include "..\utils\thread.h"
#include "..\utils\Lockable.h"

#include "AMCPCommand.h"

namespace caspar {
namespace amcp {

class AMCPCommandQueue : public utils::IRunnable, private utils::LockableObject
{
	AMCPCommandQueue(const AMCPCommandQueue&);
	AMCPCommandQueue& operator=(const AMCPCommandQueue&);
public:
	AMCPCommandQueue();
	~AMCPCommandQueue();

	bool Start();
	void Stop();
	void AddCommand(AMCPCommandPtr pCommand);

private:
	utils::Thread				commandPump_;
	virtual void Run(HANDLE stopEvent);
	virtual bool OnUnhandledException(const std::exception& ex) throw();

	utils::Event newCommandEvent_;

	//Needs synro-protection
	std::list<AMCPCommandPtr>	commands_;
};
typedef std::tr1::shared_ptr<AMCPCommandQueue> AMCPCommandQueuePtr;

}	//namespace amcp
}	//namespace caspar

#endif	//_AMCPCOMMANDQUEUE_H__