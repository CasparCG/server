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
