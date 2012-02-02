#include "stdafx.h"

#include "except.h"

#include "os/windows/windows.h"

namespace caspar {
	
void win32_exception::install_handler() 
{
//#ifndef _DEBUG
	_set_se_translator(win32_exception::Handler);
//#endif
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