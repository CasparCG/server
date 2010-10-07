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
 
#include "..\StdAfx.h"

#include "thread.h"
#include "win32exception.h"

/*

*/

namespace caspar {
namespace utils {

bool Thread::static_bInstallWin32ExceptionHandler_ = true;

Thread::Thread() : pRunnable_(0), hThread_(0), stopEvent_(TRUE, FALSE), timeout_(10000)  {
}

Thread::~Thread() {
	Stop();
}

bool Thread::IsRunning() {
	if(hThread_ != 0) {
		if(WaitForSingleObject(hThread_, 0) == WAIT_OBJECT_0) {
			CloseHandle(hThread_);
			hThread_ = 0;
			pRunnable_ = 0;
		}
	}

	return (hThread_ != 0);
}

bool Thread::Start(IRunnable* pRunnable) {
	if(hThread_ == 0) {
		if(pRunnable != 0) {
			pRunnable_ = pRunnable;
			stopEvent_.Reset();
			hThread_ = CreateThread(0, 0, ThreadEntrypoint, this, 0, 0);

			if(hThread_ == 0)
				pRunnable_ = 0;

			return (hThread_ != 0);
		}
	}
	return false;
}

bool Thread::Stop(bool bWait) {
	bool returnValue = true;

	if(hThread_ != 0) {
		stopEvent_.Set();

		if(bWait) {
			DWORD successCode = WaitForSingleObject(hThread_, timeout_);
			if(successCode != WAIT_OBJECT_0)
				returnValue = false;
		}
		CloseHandle(hThread_);

		hThread_ = 0;
		pRunnable_ = 0;
	}

	return returnValue;
}

void Thread::EnableWin32ExceptionHandler(bool bEnable) {
	static_bInstallWin32ExceptionHandler_ = bEnable;
}

DWORD WINAPI Thread::ThreadEntrypoint(LPVOID pParam) {
	Thread* pThis = reinterpret_cast<Thread*>(pParam);
	
	if(Thread::static_bInstallWin32ExceptionHandler_)
		Win32Exception::InstallHandler();

	_configthreadlocale(_DISABLE_PER_THREAD_LOCALE);

	pThis->Run();

	return 0;
}

void Thread::Run() {
	bool bDoRestart = false;

	do {
		try {
			bDoRestart = false;
			stopEvent_.Reset();
			pRunnable_->Run(stopEvent_);
		}
		catch(const std::exception& e) {
			bDoRestart = pRunnable_->OnUnhandledException(e);
		}
	}while(bDoRestart);
}

}	//namespace utils
}	//namespace caspar