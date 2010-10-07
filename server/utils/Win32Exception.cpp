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