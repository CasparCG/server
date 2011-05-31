#ifndef _CASPAR_RUNNABLE_H__
#define _CASPAR_RUNNABLE_H__

#pragma once

namespace caspar {
namespace utils {

class IRunnable
{
public:
	virtual ~IRunnable() {}
	virtual void Run(HANDLE stopEvent) = 0;
	virtual bool OnUnhandledException(const std::exception&) throw() = 0;
};

typedef std::tr1::shared_ptr<IRunnable> RunnablePtr;

}	//namespace utils
}	//namespace caspar

#endif	//_CASPAR_RUNNABLE_H__