#include "..\stdafx.h"
#include "win32exception.h"

void Win32Exception::InstallHandler() {
	_set_se_translator(Win32Exception::Handler);
}

void Win32Exception::Handler(unsigned int errorCode, EXCEPTION_POINTERS* pInfo) {
	switch(errorCode)
	{
	case EXCEPTION_ACCESS_VIOLATION:
		throw Win32AccessViolationException(*(pInfo->ExceptionRecord));
		break;

	default:
		throw Win32Exception(*(pInfo->ExceptionRecord));
	}
}

Win32Exception::Win32Exception(const EXCEPTION_RECORD& info) : message_("Win32 exception"), location_(info.ExceptionAddress), errorCode_(info.ExceptionCode)
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

Win32AccessViolationException::Win32AccessViolationException(const EXCEPTION_RECORD& info) : Win32Exception(info), isWrite_(false), badAddress_(0) 
{
	isWrite_ = info.ExceptionInformation[0] == 1;
	badAddress_ = reinterpret_cast<Win32Exception::Address>(info.ExceptionInformation[1]);
}

const char* Win32AccessViolationException::what() const {
	sprintf_s<>(messageBuffer_, "Access violation at %p, trying to %s %p", Location(), isWrite_?"write":"read", badAddress_);

	return messageBuffer_;
}