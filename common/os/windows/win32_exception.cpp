#include "../../stdafx.h"

#include "win32_exception.h"

#include <boost/lexical_cast.hpp>

#include "../../thread_info.h"
#include "windows.h"

thread_local bool installed = false;

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

void install_gpf_handler()
{
//#ifndef _DEBUG
	_set_se_translator(win32_exception::Handler);
    installed = true;
//#endif
}

void ensure_gpf_handler_installed_for_thread(
		const char* thread_description)
{
	if (!installed)
	{
		install_gpf_handler();

		if (thread_description)
		{
			detail::SetThreadName(GetCurrentThreadId(), thread_description);
			get_thread_info().name = thread_description;
		}
	}
}

msg_info_t generate_message(const EXCEPTION_RECORD& info)
{
	switch (info.ExceptionCode)
	{
	case EXCEPTION_ACCESS_VIOLATION:
		{
			bool is_write = info.ExceptionInformation[0] == 1;
			auto bad_address = reinterpret_cast<const void*>(info.ExceptionInformation[1]);
			auto location = info.ExceptionAddress;

			return "Access violation at " + boost::lexical_cast<std::string>(location) + " trying to " + (is_write ? "write " : "read ") + boost::lexical_cast<std::string>(bad_address);
		}
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:
	case EXCEPTION_INT_DIVIDE_BY_ZERO:
		return "Divide by zero";
	default:
		return "Win32 exception";
	}
}

void win32_exception::Handler(unsigned int errorCode, EXCEPTION_POINTERS* pInfo) {
	switch(errorCode)
	{
	case EXCEPTION_ACCESS_VIOLATION:
		CASPAR_THROW_EXCEPTION(win32_access_violation() << generate_message(*(pInfo->ExceptionRecord)));
	case EXCEPTION_STACK_OVERFLOW:
		throw "Stack overflow. Not generating stack trace to protect from further overflowing the stack";
	default:
		CASPAR_THROW_EXCEPTION(win32_exception() << generate_message(*(pInfo->ExceptionRecord)));
	}
}

}
