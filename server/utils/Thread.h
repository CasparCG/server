#ifndef _CASPAR_THREAD_H__
#define _CASPAR_THREAD_H__

#pragma once

#include "runnable.h"
#include "event.h"

namespace caspar {
namespace utils {

class Thread
{
	Thread(const Thread&);
	Thread& operator=(const Thread&);
public:
	Thread();
	~Thread();

	bool Start(IRunnable*);
	bool Stop(bool bWait = true);

	bool IsRunning();

	static void EnableWin32ExceptionHandler(bool bEnable);

	void SetTimeout(DWORD timeout) {
		timeout_ = timeout;
	}
	DWORD GetTimeout() {
		return timeout_;
	}

private:
	static DWORD WINAPI ThreadEntrypoint(LPVOID pParam);
	void Run();

	HANDLE			hThread_;
	IRunnable*		pRunnable_;

	Event			stopEvent_;

	DWORD			timeout_;
	static bool		static_bInstallWin32ExceptionHandler_;
};

}	//namespace utils
}	//namespace caspar


#endif	//_CASPAR_THREAD_H__
