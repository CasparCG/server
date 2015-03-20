#include "../../stdafx.h"

#include "win32_exception.h"

#include <boost/thread.hpp>

#include "windows.h"

namespace caspar { namespace detail {

typedef struct tagTHREADNAME_INFO
{
	DWORD dwType; // must be 0x1000
	LPCSTR szName; // pointer to name (in user addr space)
	DWORD dwThreadID; // thread ID (-1=caller thread)
	DWORD dwFlags; // reserved for future use, must be zero
} THREADNAME_INFO;

inline void SetThreadName(DWORD dwThreadID, LPCSTR szThreadName)
{
	THREADNAME_INFO info;
	{
		info.dwType = 0x1000;
		info.szName = szThreadName;
		info.dwThreadID = dwThreadID;
		info.dwFlags = 0;
	}
	__try
	{
		RaiseException( 0x406D1388, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info );
	}
	__except (EXCEPTION_CONTINUE_EXECUTION){}	
}

} // namespace detail

bool& installed_for_thread()
{
	static boost::thread_specific_ptr<bool> installed;

	auto for_thread = installed.get();
	
	if (!for_thread)
	{
		for_thread = new bool(false);
		installed.reset(for_thread);
	}

	return *for_thread;
}

void install_gpf_handler()
{
//#ifndef _DEBUG
	_set_se_translator(win32_exception::Handler);
	installed_for_thread() = true;
//#endif
}

void ensure_gpf_handler_installed_for_thread(
		const char* thread_description)
{
	if (!installed_for_thread())
	{
		install_gpf_handler();

		if (thread_description)
			detail::SetThreadName(GetCurrentThreadId(), thread_description);
	}
}

void win32_exception::Handler(unsigned int errorCode, EXCEPTION_POINTERS* pInfo) {
	switch(errorCode)
	{
	case EXCEPTION_ACCESS_VIOLATION:
		throw win32_access_violation(*(pInfo->ExceptionRecord));
		break;

	default:
		throw win32_exception(*(pInfo->ExceptionRecord));
	}
}

win32_exception::win32_exception(const EXCEPTION_RECORD& info) : message_("Win32 exception"), location_(info.ExceptionAddress), errorCode_(info.ExceptionCode)
{
	switch(info.ExceptionCode)
	{
	case EXCEPTION_ACCESS_VIOLATION:
		message_ = "Access violation";
		break;
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:
	case EXCEPTION_INT_DIVIDE_BY_ZERO:
		message_ = "Divide by zero";
		break;
	}
}

win32_access_violation::win32_access_violation(const EXCEPTION_RECORD& info) : win32_exception(info), isWrite_(false), badAddress_(0) 
{
	isWrite_ = info.ExceptionInformation[0] == 1;
	badAddress_ = reinterpret_cast<win32_exception::address>(info.ExceptionInformation[1]);
}

const char* win32_access_violation::what() const
{
	sprintf_s<>(messageBuffer_, "Access violation at %p, trying to %s %p", location(), isWrite_?"write":"read", badAddress_);

	return messageBuffer_;
}

}
